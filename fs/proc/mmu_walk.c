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
#include <linux/bitmap.h>
#include <linux/types.h>

static DEFINE_MUTEX(mmu_walk_mutex);
static ssize_t mmu_walk_len;
extern struct mm_struct init_mm;
extern void *vmalloc(unsigned long size);

static unsigned long get_bits_val(unsigned long *addr, unsigned long start_bit,
		    unsigned long end_bit)
{
	unsigned long val = 0;
	int loop;
	for (loop = start_bit; loop <= end_bit; loop++) {
		if (test_bit(loop, addr)) {
			set_bit(loop, &val);
		}
	}
	return val;
}

/**
 * Translation table lookup with 4KB pages:
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 * |63    56|55    48|47    40|39    32|31    24|23    16|15     8|7      0|
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 *  |                 |         |         |         |         |
 *  |                 |         |         |         |         v
 *  |                 |         |         |         |   [11:0]  in-page offset
 *  |                 |         |         |         +-> [20:12] L3 index
 *  |                 |         |         +-----------> [29:21] L2 index
 *  |                 |         +---------------------> [38:30] L1 index
 *  |                 +-------------------------------> [47:39] L0 index
 *  +-------------------------------------------------> [63] TTBR0/1
 */
static unsigned long m_virt2_phys(unsigned long pgd, unsigned long virt_addr)
{
	unsigned long phys;
	unsigned long *pgd_p = pgd;
	unsigned long *pud_p;
	unsigned long *pmd_p;
	unsigned long *pte_p;
	unsigned long l0_index = get_bits_val(virt_addr, 39, 47);
	unsigned long l1_index = get_bits_val(virt_addr, 38, 30); /* 9bits 1G*/
	unsigned long l2_index = get_bits_val(virt_addr, 29, 21); /* 9bits 2M*/
	unsigned long l3_index = get_bits_val(virt_addr, 20, 12); /* 9bits 4K*/
	pr_err("l0 l1 l2 l3 index %lx %lx %lx %lx\n", l0_index, l1_index, l2_index, l3_index);
	/*l1_arrary = l0_arrary[l0_index];*/
	/*l2_arrary = l1_arrary[l1_index];*/
	/*l3_arrary = l2_arrary[l2_index];*/
	/*phys = l3_arrary[l3_index];*/
	return phys;
}

static unsigned long virt2_phys(pgd_t *pgd, unsigned long addr)
{
	/*init_mm;*/
	/*pgd_t *pgd;*/
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	char buf[4996] = {0};
	int size = 0;
	if (pgd_none(*pgd)) {
		pr_err("puck %s %d\n", __func__, __LINE__);
		goto out;
	}

	if (pgd_bad(*pgd)) {
		size += snprintf(buf + size, PAGE_SIZE - size, "(bad)");
		pr_err("puck %s %d\n", __func__, __LINE__);
		goto out;
	}

	pud = pud_offset(pgd, addr);
	if (PTRS_PER_PUD != 1)
		size += snprintf(buf + size, PAGE_SIZE - size,
				", *pud=%08llx", (long long)pud_val(*pud));

	if (pud_none(*pud)) {
		goto out;
	}

	if (pud_bad(*pud)) {
		size += snprintf(buf + size, PAGE_SIZE - size, "(bad)");
		goto out;
	}

	pmd = pmd_offset(pud, addr);
	if (PTRS_PER_PMD != 1)
		size += snprintf(buf + size, PAGE_SIZE - size,
				", *pmd=%08llx", (long long)pmd_val(*pmd));

	if (pmd_none(*pmd))
		goto out;

	if (pmd_bad(*pmd)) {
		size += snprintf(buf + size, PAGE_SIZE - size, "(bad)");
		goto out;
	}

	pte = pte_offset_map(pmd, addr);
	size += snprintf(buf + size, PAGE_SIZE - size, ", *pte=%08llx",
			(long long)pte_val(*pte));
	pte_unmap(pte);
	size += snprintf(buf + size, PAGE_SIZE - size, "\n");

out:
	pr_err("%s\n", buf);
	return size;
}

static unsigned long *get_pgd_addr(void)
{
	unsigned long *ttbr1;
	asm volatile("mrs %0, sp_el0" : "=r" (ttbr1));
	return ttbr1;
}

static loff_t mmu_walk_llseek(struct file *file, loff_t offset, int origin)
{
	return generic_file_llseek_size(file, offset, origin,
					MAX_LFS_FILESIZE, mmu_walk_len);
}

static ssize_t read_mmu_walk(struct file *file, char __user *buf,
			       size_t count, loff_t *ppos)
{
	pr_err("%s %d\n", __func__, __LINE__);
	return 0;
}

static ssize_t write_mmu_walk(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	pr_err("%s %d\n", __func__, __LINE__);
	return 0;
}

static int mmu_walk_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
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

static long mmu_walk_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;

	mutex_lock(&mmu_walk_mutex);
	ret = mmu_walk_ioctl(file, cmd, arg);
	mutex_unlock(&mmu_walk_mutex);

	return ret;
}

static pmd_t *mm_find_phys(struct mm_struct *mm, unsigned long address)
{
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;
	unsigned long phys = NULL;
	unsigned long pgd_val;
	unsigned long pte_val;
	unsigned long pmd_val;
	unsigned long pte_phys;
	unsigned long pgd_out;
	unsigned long pmd_out;
	unsigned long pte_out;

        pgd = pgd_offset(mm, address);
        if (!pgd_present(*pgd))
                goto out;
	pgd_val = pgd_val(*pgd);
	if (!test_bit(1, &pgd_val)) {
		pgd_out = get_bits_val(&pgd_val, 30, 47);
		pr_err("block pgd_val %lx out %lx offset %lx phys %lx\n", pgd_val, pgd_out, address & (SZ_1G - 1), pgd_out + (address & (SZ_1G - 1)));
		return (pgd_out + (address & (SZ_1G - 1)));
	}
	pgd_out = get_bits_val(&pgd_val, 12, 47);
	pr_err("table pgd_val %lx out %lx %d\n", pgd_val, pgd_out, pgd_val & 0x3);

        pud = pud_offset(pgd, address);
        if (!pud_present(*pud))
                goto out;

        pmd = pmd_offset(pud, address);
        if (!pmd_present(*pmd))
		goto out;

	pmd_val = pmd_val(*pmd);
	if (!test_bit(1, &pmd_val)) {
		pmd_out = get_bits_val(&pmd_val, 21, 47);
		pr_err("block pmd_val %lx out %lx offset %lx phys %lx\n", pmd_val, pmd_out, address & (SZ_2M - 1), pmd_out + (address & (SZ_2M - 1)));
		return (pmd_out + (address & (SZ_2M - 1)));
	}
	pmd_out = get_bits_val(&pmd_val, 12, 47);
	pr_err("table pmd_val %lx out %lx %d\n", pmd_val, pmd_out, pmd_val & 0x3);

	pte = pte_offset_map(pmd, address);
	if (!pte_present(*pte))
		goto out;

	pte_val = pte_val(*pte);
	pte_out = get_bits_val(&pte_val, 12, 47);

	pr_err("virt %lx pte_val %lx pte_out %lx offset %lx  phy %lx\n",
		address, pte_val, pte_out, (address & (SZ_4K - 1)), pte_out + (address & (SZ_4K - 1)));

out:
        return (pte_out + address & (SZ_4K - 1));
}

static int mmu_walk_open(struct inode *inode, struct file *file)
{
	unsigned long *pgd;
	unsigned long data;
	unsigned long phys;
	unsigned long address = (unsigned long)&data;
	unsigned long find_phys;
	pmd_t *pmd;
	pte_t *pte;
	pgd = get_pgd_addr();
	pr_err("%s\n", __func__);
	pr_err("%lx\n", pgd);
	pr_err("virt %lx phys %lx\n", &data, virt_to_phys(&data));

	unsigned long address_2 = vmalloc(12);
	find_phys = mm_find_phys(&init_mm, address);
	pr_err("find virt %lx phys %lx\n", address, find_phys);

	find_phys = mm_find_phys(&init_mm, address_2);
	pr_err("find virt %lx phys %lx\n", address_2, find_phys);

	int i;
	for (i = 0; i < 10; i++) {
		struct page *mpage = alloc_pages(GFP_KERNEL | __GFP_ZERO | __GFP_DMA, i);
		if (mpage) {
			unsigned long address_3 = page_address(mpage);
			find_phys = mm_find_phys(&init_mm, address_3);
			pr_err("alloc virt %lx phys %lx find %lx\n", address_3, virt_to_phys(address_3), find_phys);
		}
	}
	/*phys = virt2_phys(init_mm.pgd, &data);*/
	/*pr_err("virt %lx phys %lx\n", &data, phys);*/
	return 0;
}

struct file_operations mmu_walk_fops = {
	.owner		= THIS_MODULE,
	.llseek		= mmu_walk_llseek,
	.read		= read_mmu_walk,
	.write		= write_mmu_walk,
	.open    	= mmu_walk_open,
	.unlocked_ioctl	= mmu_walk_unlocked_ioctl,
};

static struct miscdevice mmu_walk_dev = {
	MISC_DYNAMIC_MINOR,
	"mmu_walk",
	&mmu_walk_fops
};

int __init mmu_walk_init(void)
{
	int ret = 0;

	printk(KERN_INFO "Generic mmu walk init");
	pr_err("CONFIG_PGTABLE_LEVELS %d VA_BITS %d\n"
	       "PTRS_PER_PGD %d PTRS_PER_PUD %d PTRS_PER_PMD %d PTRS_PER_PTE %d\n"
	       "PGDIR_SIZE %d PUD_SIZE %d PMD_SIZE %d\n"
	       "PGDIR_SHIFT %d PUD_SHIFT %d PMD_SHIFT %d\n",
	       CONFIG_PGTABLE_LEVELS, VA_BITS,
	       PTRS_PER_PGD, PTRS_PER_PUD, PTRS_PER_PMD, PTRS_PER_PTE,
	       PGDIR_SIZE, PUD_SIZE, PMD_SIZE,
	       PGDIR_SHIFT, PUD_SHIFT, PMD_SHIFT);
	ret = misc_register(&mmu_walk_dev);
	if (ret != 0)
		goto out;
out:
	return ret;
}

void __exit mmu_walk_cleanup(void)
{
	misc_deregister(&mmu_walk_dev);
}

module_init(mmu_walk_init);
module_exit(mmu_walk_cleanup);
MODULE_LICENSE("GPL");
