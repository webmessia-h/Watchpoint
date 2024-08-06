#include "linux/hw_breakpoint.h"
#include "linux/perf_event.h"
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <linux/cpumask.h>
#include <linux/hw_breakpoint.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/ptrace.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>

#define HW_BREAKPOINT_LEN HW_BREAKPOINT_LEN_8

/*
 * on x86 it can be 1,2,4 | on x86_64 can be 1,2,4,8
 * Available HW breakpoint length encodings
 *#define X86_BREAKPOINT_LEN_X		0x40
 *#define X86_BREAKPOINT_LEN_1		0x40
 *#define X86_BREAKPOINT_LEN_2		0x44
 *#define X86_BREAKPOINT_LEN_4		0x4c

 *#ifdef CONFIG_X86_64
 *#define X86_BREAKPOINT_LEN_8		0x48
 *#endif
*/

DEFINE_PER_CPU(unsigned long *, cpu_dr0);
DEFINE_PER_CPU(unsigned long *, cpu_dr1);
DEFINE_PER_CPU(unsigned long *, cpu_dr2);
DEFINE_PER_CPU(unsigned long *, cpu_dr3);
DEFINE_PER_CPU(unsigned long *, cpu_dr6);

static unsigned long watch_address = 0;
// static unsigned long test_var = 0;
module_param(watch_address, ulong, 0644);
MODULE_PARM_DESC(watch_address, "Memory address to set the watchpoint");

static struct kobject *watch_kobj;
static struct perf_event *__percpu
    *hw_breakpoint; /* I think you can split this into separate watchpoints */

static void hw_breakpoint_handler(struct perf_event *bp,
                                  struct perf_sample_data *data,
                                  struct pt_regs *regs) {
  struct perf_event_attr attr = bp->attr;
  struct hw_perf_event hw = bp->hw;

  pr_info("Watchpoint triggered at address: 0x%lx"
          "\n.bp_type %d | .type %d | state "
          "%d | htype %d | hwi %llu",
          watch_address, attr.bp_type, attr.type, hw.state, hw.info.type,
          hw.interrupts);
  if (hw.interrupts == HW_BREAKPOINT_R) {
    pr_info("READ access");
    // place for your READ callback
  } else if (hw.interrupts == HW_BREAKPOINT_W) {
    pr_info("WRITE access");
    // place for your WRITE callback
  } else {
    pr_info("Unknow access type");
  }
  dump_stack();
}

static void print_debug_registers(void) {
  unsigned long dr0, dr1, dr2, dr3, dr6, dr7;
  __asm__("mov %%dr0, %0" : "=r"(dr0));
  __asm__("mov %%dr1, %0" : "=r"(dr1));
  __asm__("mov %%dr2, %0" : "=r"(dr2));
  __asm__("mov %%dr3, %0" : "=r"(dr3));
  __asm__("mov %%dr6, %0" : "=r"(dr6));
  __asm__("mov %%dr7, %0" : "=r"(dr7));
  pr_info("Debug registers: \t\t\t\nDR0=0x%lx | DR1=0x%lx | DR2=0x%lx | "
          "\t\t\t\t\nDR3=0x%lx | DR6=0x%lx | DR7=0x%lx\n",
          dr0, dr1, dr2, dr3, dr6, dr7);
}

static int get_av_cpu(int num) {
  int cpu;

  WARN_ON(num < 0);

  for_each_online_cpu(cpu) {
    if (num-- <= 0)
      break;
  }

  return cpu;
}

static int get_hw_bp_slots(void) {
  static int slots;

  if (!slots)
    slots = hw_breakpoint_slots(TYPE_DATA);

  return slots;
}

static struct perf_event *set_watchpoint(int cpu) {
  if (get_hw_bp_slots() < 1) {
    printk("No available hardware breakpoint slots");
    return NULL;
  }
  pr_info("Setting watchpoint\n");
  print_debug_registers(); // Print registers before setting the watchpoint
  struct perf_event_attr attr = {};
  int ret;
  memset(&attr, 0, sizeof(struct perf_event_attr));
  attr.type = PERF_TYPE_BREAKPOINT;
  attr.size = sizeof(struct perf_event_attr);
  attr.bp_addr = (unsigned long)watch_address;
  attr.bp_len = HW_BREAKPOINT_LEN;
  attr.bp_type = HW_BREAKPOINT_RW;
  attr.sample_period = 1;

  hw_breakpoint =
      register_wide_hw_breakpoint(&attr, hw_breakpoint_handler, NULL);
  if (IS_ERR(hw_breakpoint)) {
    ret = PTR_ERR(hw_breakpoint);
    hw_breakpoint = NULL;
    pr_err("Failed to set watchpoint: %d\n", ret);
    return NULL;
  }
  pr_info("Watchpoint R|W set at address: 0x%lx\n", watch_address);
  print_debug_registers();
  return NULL;
}

static void clear_watchpoint(struct perf_event **bp) {
  if (hw_breakpoint) {
    if (!IS_ERR_OR_NULL(hw_breakpoint))
      unregister_wide_hw_breakpoint(bp);
    else
      pr_err("Invalid breakpoint pointer.\n");
  }

  bp = NULL;
  pr_info("Watchpoint cleared");
}

static ssize_t watch_address_show(struct kobject *kobj,
                                  struct kobj_attribute *attr, char *buf) {
  return sprintf(buf, "%lx\n", watch_address);
}

static ssize_t watch_address_store(struct kobject *kobj,
                                   struct kobj_attribute *attr, const char *buf,
                                   size_t count) {
  int ret = kstrtoul(buf, 0, &watch_address);
  if (ret)
    return ret;

  clear_watchpoint(hw_breakpoint);
  set_watchpoint(get_av_cpu(0));

  return count;
}

static struct kobj_attribute watch_attr =
    __ATTR(watch_address, 0644, watch_address_show, watch_address_store);

static int __init watchpoint_init(void) {

  int ret;
  printk(KERN_INFO "Watchpoint driver loaded successfully.\n");

  watch_kobj = kobject_create_and_add("watchpoint", kernel_kobj);
  if (!watch_kobj)
    return -ENOMEM;

  ret = sysfs_create_file(watch_kobj, &watch_attr.attr);
  if (ret) {
    kobject_put(watch_kobj);
    return ret;
  }

  /*if (watch_address) {
    if (watch_address % HW_BREAKPOINT_LEN != 0) {
      pr_err("Watch address is not aligned correctly.\n");
      return -EINVAL;
    }
    set_watchpoint(get_av_cpu(0));
  }*/
  /* trigger the watchpoint
  test_var = 42;
  unsigned long check_read = test_var;
  */
  return 0;
}

static void __exit watchpoint_exit(void) {
  clear_watchpoint(hw_breakpoint);
  if (watch_kobj) {
    sysfs_remove_file(watch_kobj, &watch_attr.attr);
    kobject_put(watch_kobj);
  }
  printk(KERN_INFO "Watchpoint driver exited.\n");
}

module_init(watchpoint_init);
module_exit(watchpoint_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("webmesiah");
MODULE_DESCRIPTION("Watchpoint Kernel Module");
