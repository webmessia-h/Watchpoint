#include "linux/sysctl.h"
#include <asm/ptrace.h>
#include <linux/hw_breakpoint.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>

static unsigned long watch_address = 0;
module_param(watch_address, ulong, 0644);
MODULE_PARM_DESC(watch_address, "Memory address to set the watchpoint");

static struct kobject *watch_kobj;
static struct perf_event **hw_breakpoint;

static void hw_breakpoint_handler(struct perf_event *bp,
                                  struct perf_sample_data *data,
                                  struct pt_regs *regs) {
  pr_info("Watchpoint triggered at address: 0x%lx\n", watch_address);
  dump_stack();
}

static int set_watchpoint(void) {
  struct perf_event_attr attr;
  int ret;

  memset(&attr, 0, sizeof(struct perf_event_attr));
  attr.type = PERF_TYPE_BREAKPOINT;
  attr.size = sizeof(struct perf_event_attr);
  attr.bp_addr = watch_address;
  attr.bp_len = HW_BREAKPOINT_LEN_4;
  attr.bp_type = HW_BREAKPOINT_W;

  hw_breakpoint =
      register_wide_hw_breakpoint(&attr, hw_breakpoint_handler, NULL);
  if (IS_ERR(hw_breakpoint)) {
    ret = PTR_ERR(hw_breakpoint);
    hw_breakpoint = NULL;
    pr_err("Failed to set watchpoint: %d\n", ret);
    return ret;
  }

  pr_info("Watchpoint set at address: 0x%lx\n", watch_address);
  return 0;
}

static void clear_watchpoint(void) {
  if (hw_breakpoint) {
    unregister_wide_hw_breakpoint(hw_breakpoint);
    hw_breakpoint = NULL;
    pr_info("Watchpoint cleared at address: 0x%lx\n", watch_address);
  }
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

  clear_watchpoint();
  set_watchpoint();

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

  if (watch_address)
    set_watchpoint();

  return 0;
}

static void __exit watchpoint_exit(void) {
  clear_watchpoint();
  kobject_put(watch_kobj);
  printk(KERN_INFO "Watchpoint driver exited.\n");
}

module_init(watchpoint_init);
module_exit(watchpoint_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("webmesiah");
MODULE_DESCRIPTION("Watchpoint Kernel Module");
