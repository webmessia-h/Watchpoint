#include "linux/hw_breakpoint.h"
#include "linux/printk.h"
#include "linux/sysctl.h"
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <linux/cpumask.h>
#include <linux/hw_breakpoint.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/ptrace.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>

DEFINE_PER_CPU(unsigned long *, cpu_dr0);
DEFINE_PER_CPU(unsigned long *, cpu_dr1);
DEFINE_PER_CPU(unsigned long *, cpu_dr2);
DEFINE_PER_CPU(unsigned long *, cpu_dr3);
DEFINE_PER_CPU(unsigned long *, cpu_dr6);

static unsigned long watch_address = 0;
static unsigned long test_var = 0;
module_param(watch_address, ulong, 0644);
MODULE_PARM_DESC(watch_address, "Memory address to set the watchpoint");

static struct kobject *watch_kobj;
static struct perf_event *__percpu *hw_breakpoint;
static struct task_struct *__other_task;

static void hw_breakpoint_handler(struct perf_event *bp,
                                  struct perf_sample_data *data,
                                  struct pt_regs *regs) {
  pr_info("Watchpoint triggered at address: 0x%lx\n", watch_address);
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

static int get_test_cpu(int num) {
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
  attr.bp_len = HW_BREAKPOINT_LEN_8;
  attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;
  attr.sample_period = 1;

  hw_breakpoint =
      register_wide_hw_breakpoint(&attr, hw_breakpoint_handler, NULL);
  if (IS_ERR(hw_breakpoint)) {
    ret = PTR_ERR(hw_breakpoint);
    hw_breakpoint = NULL;
    pr_err("Failed to set watchpoint: %d\n", ret);
    return NULL;
  }

  /*perf_event_create_kernel_counter(&attr, cpu, NULL, hw_breakpoint_handler,
                                   NULL //context);*/

  pr_info("Watchpoint set at address: 0x%lx\n", watch_address);

  print_debug_registers();
  return NULL;
}

static void clear_watchpoint(struct perf_event **bp) {
  if (WARN_ON(IS_ERR(*bp)))
    return;
  if (WARN_ON(!*bp))
    return;
  unregister_hw_breakpoint(*bp);
  *bp = NULL;
  pr_info("Watchpoint cleared at address: 0x%lx\n", watch_address);
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
  set_watchpoint(get_test_cpu(0));

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

  watch_address = (unsigned long)&test_var;

  if (watch_address) {
    if (watch_address % HW_BREAKPOINT_LEN_8 != 0) {
      pr_err("Watch address is not aligned correctly.\n");
      return -EINVAL;
    }
    set_watchpoint(get_test_cpu(0));
  }
  printk("size: %zu", sizeof(test_var));
  // Attempt to trigger the watchpoint
  printk("0x%lx\n", watch_address);
  test_var = 42;
  printk("test_var value after write: %lu\n", test_var);
  unsigned long check_read = test_var;
  printk("check_read: %lu\n", check_read);
  return 0;
}

static void __exit watchpoint_exit(void) {
  if (hw_breakpoint)
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
