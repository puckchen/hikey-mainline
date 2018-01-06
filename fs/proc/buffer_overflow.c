#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>

#define MAX_LENGTH 64

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ryan Welton");
MODULE_DESCRIPTION("Stack Buffer Overflow Example");

static struct proc_dir_entry *stack_buffer_proc_entry;

/*ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *)*/
ssize_t proc_entry_write(struct file *file, const char __user *ubuf, size_t count, loff_t *data)
{
	char buf[MAX_LENGTH];

	pr_err("ubuf %lx count %d\n", ubuf, count);

	if (copy_from_user(&buf, ubuf, count)) {
		printk(KERN_INFO "stackBufferProcEntry: error copying data from userspace\n");
		return -EFAULT;
	}

	return count;
}

static const struct file_operations buffer_operations = {
	.write  = proc_entry_write,
	.open           = single_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static int __init stack_buffer_proc_init(void)
{

	stack_buffer_proc_entry = proc_create("stack_buffer_overflow", 0666, NULL, &buffer_operations);

	printk(KERN_INFO "created /proc/stack_buffer_overflow\n");

	return 0;
}

static void __exit stack_buffer_proc_exit(void)
{
	if (stack_buffer_proc_entry) {
		remove_proc_entry("stack_buffer_overflow", stack_buffer_proc_entry);
	}

	printk(KERN_INFO "vuln_stack_proc_entry removed\n");
}

module_init(stack_buffer_proc_init);
module_exit(stack_buffer_proc_exit);
