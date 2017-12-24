#define TEST_IOCTL_VERSION "1.1"

#include <linux/module.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/pagemap.h>
#include <asm/uaccess.h>

static DEFINE_MUTEX(test_ioctl_mutex);
static ssize_t test_ioctl_len;

static loff_t test_ioctl_llseek(struct file *file, loff_t offset, int origin)
{
	return generic_file_llseek_size(file, offset, origin,
					MAX_LFS_FILESIZE, test_ioctl_len);
}

static ssize_t read_test_ioctl(struct file *file, char __user *buf,
			       size_t count, loff_t *ppos)
{
	pr_err("%s %d\n", __func__, __LINE__);
	return 0;
}

static ssize_t write_test_ioctl(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	pr_err("%s %d\n", __func__, __LINE__);
	return 0;
}

static int test_ioctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case 127:
		pr_err("ioctl 127\n");
		break;
	case 128:
		pr_err("ioctl 128\n");
		break;
	case 129:
		pr_err("ioctl 129\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static long test_ioctl_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;

	mutex_lock(&test_ioctl_mutex);
	ret = test_ioctl_ioctl(file, cmd, arg);
	mutex_unlock(&test_ioctl_mutex);

	return ret;
}

static int test_ioctl_open(struct inode *inode, struct file *file)
{
	pr_err("%s\n", __func__);
	return 0;
}

struct file_operations test_ioctl_fops = {
	.owner		= THIS_MODULE,
	.llseek		= test_ioctl_llseek,
	.read		= read_test_ioctl,
	.write		= write_test_ioctl,
	.open    	= test_ioctl_open,
	.unlocked_ioctl	= test_ioctl_unlocked_ioctl,
};

static struct miscdevice test_ioctl_dev = {
	MISC_DYNAMIC_MINOR,
	"test_ioctl",
	&test_ioctl_fops
};

int __init test_ioctl_init(void)
{
	int ret = 0;

	printk(KERN_INFO "Generic non-volatile memory driver v%s\n",
	       TEST_IOCTL_VERSION);
	ret = misc_register(&test_ioctl_dev);
	if (ret != 0)
		goto out;
out:
	return ret;
}

void __exit test_ioctl_cleanup(void)
{
	misc_deregister(&test_ioctl_dev);
}

module_init(test_ioctl_init);
module_exit(test_ioctl_cleanup);
MODULE_LICENSE("GPL");
