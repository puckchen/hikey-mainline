#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>

#define MAX_LENGTH 64

static struct proc_dir_entry *stack_buffer_proc_entry;

ssize_t proc_entry_write(struct file *file, const char __user *ubuf, size_t count, loff_t *data)
{
	char buf[MAX_LENGTH];

	pr_err("ubuf %lx count %d\n", ubuf, count);

	if (copy_from_user(&buf, ubuf, count)) {
		pr_err("error copying from user data\n");
		return -EFAULT;
	}
	dump_stack();

	return count;
}

ssize_t proc_entry_read(struct file *fid, char __user *buf, size_t size, loff_t * ppos)
{
        return 0;
}

static int proc_entry_show(struct seq_file *m, void *v)
{
	return 0;
}

static int proc_entry_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_entry_show, NULL);
}


static const struct file_operations buffer_operations = {
	.open           = proc_entry_open,
	.read 		= proc_entry_read,
	.write  	= proc_entry_write,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static int __init stack_buffer_proc_init(void)
{

	proc_create("stack_buffer_overflow", 0666, NULL, &buffer_operations);

	pr_err("created /proc/stack_buffer_overflow\n");
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
