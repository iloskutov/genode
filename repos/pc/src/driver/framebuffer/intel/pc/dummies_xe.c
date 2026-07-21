#include <lx_emul/debug.h>
#include <lx_emul/io_mem.h>

#include <linux/pci.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>


struct xe_device;
struct xe_tile;
struct xe_gt;
struct xe_pmu;
struct xe_hw_engine;


void copy_page(void *to, void *from)
{
	lx_emul_trace_and_stop(__func__);
}


int pci_msix_vec_count(struct pci_dev * dev)
{
	printk("%s: claim only to have MSI support\n", __func__);
	return -EINVAL;
}


void xe_observation_sysctl_unregister(void)
{
	lx_emul_trace(__func__);
}


int xe_observation_sysctl_register(void)
{
	lx_emul_trace(__func__);
	return 0;
}


u32 pci_rebar_get_possible_sizes(struct pci_dev * pdev,int bar)
{
	lx_emul_trace(__func__);
	return 0;
}


int xe_device_sysfs_init(struct xe_device * xe)
{
	lx_emul_trace(__func__);
	return 0;
}


int xe_tile_sysfs_init(struct xe_tile * tile)
{
	lx_emul_trace(__func__);
	return 0;
}


int xe_gt_sysfs_init(struct xe_gt * gt)
{
	lx_emul_trace(__func__);
	return 0;
}


int xe_hw_engine_class_sysfs_init(struct xe_gt * gt)
{
	lx_emul_trace(__func__);
	return 0;
}


int xe_gt_ccs_mode_sysfs_init(struct xe_gt * gt)
{
	lx_emul_trace(__func__);
	return 0;
}


int xe_pmu_register(struct xe_pmu * pmu)
{
	lx_emul_trace(__func__);
	return 0;
}


void __iomem * devm_ioremap_wc(struct device * dev,
                               resource_size_t offset,
                               resource_size_t size)
{
	void * res = lx_emul_io_mem_map(offset, size, true);

	printk("%s: dev=%px offset=%llx+%llx -> %px\n",
	       __func__, dev, offset, size, res);

	return res;
}


struct task_struct * get_pid_task(struct pid * pid,enum pid_type type)
{
	lx_emul_trace(__func__);
	return NULL;
}


void unmap_mapping_range(struct address_space * mapping,
                         loff_t const holebegin,
                         loff_t const holelen,
                         int even_cows)
{
	lx_emul_trace(__func__);
}


int set_pages_array_wc(struct page ** pages, int numpages)
{
	printk("%s: not supported\n", __func__);
	return 0;
}
