/*
 * \brief  Dummy definitions of Linux Kernel functions
 * \author Automatically generated file - do no edit
 * \date   2026-09-10
 */

#include <lx_emul.h>


#include <linux/dma-fence-unwrap.h>

struct dma_fence * __dma_fence_unwrap_merge(unsigned int num_fences,struct dma_fence ** fences,struct dma_fence_unwrap * iter)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

int __mm_populate(unsigned long start,unsigned long len,int ignore_errors)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sched/mm.h>

void __mmdrop(struct mm_struct * mm)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/percpu_counter.h>

s64 __percpu_counter_sum(struct percpu_counter * fbc)
{
	lx_emul_trace_and_stop(__func__);
}


extern struct sched_entity * __pick_first_entity(struct cfs_rq * cfs_rq);
struct sched_entity * __pick_first_entity(struct cfs_rq * cfs_rq)
{
	lx_emul_trace_and_stop(__func__);
}


extern struct sched_entity * __pick_last_entity(struct cfs_rq * cfs_rq);
struct sched_entity * __pick_last_entity(struct cfs_rq * cfs_rq)
{
	lx_emul_trace_and_stop(__func__);
}


extern struct sched_entity * __pick_root_entity(struct cfs_rq * cfs_rq);
struct sched_entity * __pick_root_entity(struct cfs_rq * cfs_rq)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/vmalloc.h>

void * __vmalloc_noprof(unsigned long size,gfp_t gfp_mask)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

int access_process_vm(struct task_struct * tsk,unsigned long addr,void * buf,int len,unsigned int gup_flags)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/anon_inodes.h>

struct file * anon_inode_getfile(const char * name,const struct file_operations * fops,void * priv,int flags)
{
	lx_emul_trace_and_stop(__func__);
}


extern u64 avg_vruntime(struct cfs_rq * cfs_rq);
u64 avg_vruntime(struct cfs_rq * cfs_rq)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/security.h>

int cap_settime(const struct timespec64 * ts,const struct timezone * tz)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/console.h>

void console_lock(void)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/console.h>

int console_trylock(void)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/console.h>

void console_unlock(void)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/uaccess.h>

long copy_from_kernel_nofault(void * dst,const void * src,size_t size)
{
	lx_emul_trace_and_stop(__func__);
}


extern unsigned long __must_check copy_mc_to_kernel(void * dst,const void * src,unsigned len);
unsigned long __must_check copy_mc_to_kernel(void * dst,const void * src,unsigned len)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/dcache.h>

char * d_path(const struct path * path,char * buf,int buflen)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fs.h>

struct file * dentry_open(const struct path * path,int flags,const struct cred * cred)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/dma-buf.h>

struct dma_buf_attachment * dma_buf_attach(struct dma_buf * dmabuf,struct device * dev)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/dma-buf.h>

int dma_buf_begin_cpu_access(struct dma_buf * dmabuf,enum dma_data_direction direction)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/dma-buf.h>

int dma_buf_end_cpu_access(struct dma_buf * dmabuf,enum dma_data_direction direction)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/dma-buf.h>

struct dma_buf * dma_buf_export(const struct dma_buf_export_info * exp_info)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/dma-buf.h>

struct dma_buf * dma_buf_get(int fd)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/dma-buf.h>

struct sg_table * dma_buf_map_attachment_unlocked(struct dma_buf_attachment * attach,enum dma_data_direction direction)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

unsigned long do_mmap(struct file * file,unsigned long addr,unsigned long len,unsigned long prot,unsigned long flags,vm_flags_t vm_flags,unsigned long pgoff,unsigned long * populate,struct list_head * uf)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/restart_block.h>

long do_no_restart_syscall(struct restart_block * param)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_format_helper.h>

void drm_format_conv_state_copy(struct drm_format_conv_state * state,const struct drm_format_conv_state * old_state)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_format_helper.h>

void drm_format_conv_state_init(struct drm_format_conv_state * state)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_format_helper.h>

void drm_format_conv_state_release(struct drm_format_conv_state * state)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_plane_helper.h>

void drm_plane_helper_destroy(struct drm_plane * plane)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_plane_helper.h>

int drm_plane_helper_disable_primary(struct drm_plane * plane,struct drm_modeset_acquire_ctx * ctx)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_plane_helper.h>

int drm_plane_helper_update_primary(struct drm_plane * plane,struct drm_crtc * crtc,struct drm_framebuffer * fb,int crtc_x,int crtc_y,unsigned int crtc_w,unsigned int crtc_h,uint32_t src_x,uint32_t src_y,uint32_t src_w,uint32_t src_h,struct drm_modeset_acquire_ctx * ctx)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_self_refresh_helper.h>

void drm_self_refresh_helper_alter_state(struct drm_atomic_state * state)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_self_refresh_helper.h>

void drm_self_refresh_helper_update_avg_times(struct drm_atomic_state * state,unsigned int commit_time_ms,unsigned int new_self_refresh_mask)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_writeback.h>

struct dma_fence * drm_writeback_get_out_fence(struct drm_writeback_connector * wb_connector)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/drm_writeback.h>

int drm_writeback_set_fb(struct drm_connector_state * conn_state,struct drm_framebuffer * fb)
{
	lx_emul_trace_and_stop(__func__);
}


extern void enter_lazy_tlb(struct mm_struct * mm,struct task_struct * tsk);
void enter_lazy_tlb(struct mm_struct * mm,struct task_struct * tsk)
{
	lx_emul_trace_and_stop(__func__);
}


extern int entity_eligible(struct cfs_rq * cfs_rq,struct sched_entity * se);
int entity_eligible(struct cfs_rq * cfs_rq,struct sched_entity * se)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fb.h>

void fb_set_suspend(struct fb_info * info,int state)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/file.h>

void fd_install(unsigned int fd,struct file * file)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/file.h>

struct fd fdget(unsigned int fd)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fs.h>

char * file_path(struct file * filp,char * buf,int buflen)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mmzone.h>

struct pglist_data * first_online_pgdat(void)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sched/task.h>

struct task_struct * __init fork_idle(int cpu)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fb.h>

struct fb_info * framebuffer_alloc(size_t size,struct device * dev)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fb.h>

void framebuffer_release(struct fb_info * info)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

unsigned long free_reserved_area(void * start,void * end,int poison,const char * s)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/string.h>

char * get_options(const char * str,int nints,int * ints)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sched/mm.h>

struct mm_struct * get_task_mm(struct task_struct * task)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/file.h>

int get_unused_fd_flags(unsigned flags)
{
	lx_emul_trace_and_stop(__func__);
}


extern int i915_gem_fb_mmap(struct drm_i915_gem_object * obj,struct vm_area_struct * vma);
int i915_gem_fb_mmap(struct drm_i915_gem_object * obj,struct vm_area_struct * vma)
{
	lx_emul_trace_and_stop(__func__);
}


extern int i915_request_await_dma_fence(struct i915_request * rq,struct dma_fence * fence);
int i915_request_await_dma_fence(struct i915_request * rq,struct dma_fence * fence)
{
	lx_emul_trace_and_stop(__func__);
}


extern int i915_request_await_object(struct i915_request * to,struct drm_i915_gem_object * obj,bool write);
int i915_request_await_object(struct i915_request * to,struct drm_i915_gem_object * obj,bool write)
{
	lx_emul_trace_and_stop(__func__);
}


extern void i915_ttm_buddy_man_avail(struct ttm_resource_manager * man,u64 * avail,u64 * visible_avail);
void i915_ttm_buddy_man_avail(struct ttm_resource_manager * man,u64 * avail,u64 * visible_avail)
{
	lx_emul_trace_and_stop(__func__);
}


extern int i915_ttm_buddy_man_fini(struct ttm_device * bdev,unsigned int type);
int i915_ttm_buddy_man_fini(struct ttm_device * bdev,unsigned int type)
{
	lx_emul_trace_and_stop(__func__);
}


extern int i915_ttm_buddy_man_init(struct ttm_device * bdev,unsigned int type,bool use_tt,u64 size,u64 visible_size,u64 default_page_size,u64 chunk_size);
int i915_ttm_buddy_man_init(struct ttm_device * bdev,unsigned int type,bool use_tt,u64 size,u64 visible_size,u64 default_page_size,u64 chunk_size)
{
	lx_emul_trace_and_stop(__func__);
}


extern int i915_ttm_buddy_man_reserve(struct ttm_resource_manager * man,u64 start,u64 size);
int i915_ttm_buddy_man_reserve(struct ttm_resource_manager * man,u64 start,u64 size)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/pid_types.h>

struct pid_namespace init_pid_ns;


#include <linux/srcu.h>

int init_srcu_struct(struct srcu_struct * ssp)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_gsc_uc_flush_work(struct intel_gsc_uc * gsc);
void intel_gsc_uc_flush_work(struct intel_gsc_uc * gsc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_gsc_uc_load_start(struct intel_gsc_uc * gsc);
void intel_gsc_uc_load_start(struct intel_gsc_uc * gsc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_gsc_uc_resume(struct intel_gsc_uc * gsc);
void intel_gsc_uc_resume(struct intel_gsc_uc * gsc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_ct_disable(struct intel_guc_ct * ct);
void intel_guc_ct_disable(struct intel_guc_ct * ct)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_init_late(struct intel_guc * guc);
void intel_guc_init_late(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_init_send_regs(struct intel_guc * guc);
void intel_guc_init_send_regs(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern int intel_guc_invalidate_tlb_engines(struct intel_guc * guc);
int intel_guc_invalidate_tlb_engines(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_pm_intrmsk_enable(struct intel_gt * gt);
void intel_guc_pm_intrmsk_enable(struct intel_gt * gt)
{
	lx_emul_trace_and_stop(__func__);
}


extern int intel_guc_resume(struct intel_guc * guc);
int intel_guc_resume(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_submission_cancel_requests(struct intel_guc * guc);
void intel_guc_submission_cancel_requests(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_submission_flush_work(struct intel_guc * guc);
void intel_guc_submission_flush_work(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_submission_reset(struct intel_guc * guc,intel_engine_mask_t stalled);
void intel_guc_submission_reset(struct intel_guc * guc,intel_engine_mask_t stalled)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_submission_reset_finish(struct intel_guc * guc);
void intel_guc_submission_reset_finish(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_guc_submission_reset_prepare(struct intel_guc * guc);
void intel_guc_submission_reset_prepare(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern int intel_guc_suspend(struct intel_guc * guc);
int intel_guc_suspend(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}


extern int intel_guc_wait_for_pending_msg(struct intel_guc * guc,atomic_t * wait_var,bool interruptible,long timeout);
int intel_guc_wait_for_pending_msg(struct intel_guc * guc,atomic_t * wait_var,bool interruptible,long timeout)
{
	lx_emul_trace_and_stop(__func__);
}


extern void intel_huc_fini_late(struct intel_huc * huc);
void intel_huc_fini_late(struct intel_huc * huc)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/uio.h>

void iov_iter_advance(struct iov_iter * i,size_t size)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/uio.h>

ssize_t iov_iter_extract_pages(struct iov_iter * i,struct page *** pages,size_t maxsize,unsigned int maxpages,iov_iter_extraction_t extraction_flags,size_t * offset0)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/interrupt.h>

unsigned int irq_calc_affinity_vectors(unsigned int minvec,unsigned int maxvec,const struct irq_affinity * affd)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/interrupt.h>

struct irq_affinity_desc * irq_create_affinity_masks(unsigned int nvecs,struct irq_affinity * affd)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/page-flags.h>

bool is_free_buddy_page(const struct page * page)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

int is_vmalloc_or_module_addr(const void * x)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fs.h>

ssize_t kernel_write(struct file * file,const void * buf,size_t count,loff_t * pos)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/seq_file.h>

char * mangle_path(char * s,const char * p,const char * esc)
{
	lx_emul_trace_and_stop(__func__);
}


extern void __init memblock_free_pages(struct page * page,unsigned long pfn,unsigned int order);
void __init memblock_free_pages(struct page * page,unsigned long pfn,unsigned int order)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/string.h>

unsigned long long memparse(const char * ptr,char ** retptr)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sched/mm.h>

void mmput(struct mm_struct * mm)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/irqdomain.h>

int msi_device_domain_alloc_wired(struct irq_domain * domain,unsigned int hwirq,unsigned int type)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/console.h>

void nbcon_cpu_emergency_enter(void)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/console.h>

void nbcon_cpu_emergency_exit(void)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mmzone.h>

struct pglist_data * next_online_pgdat(struct pglist_data * pgdat)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/percpu_counter.h>

void percpu_counter_add_batch(struct percpu_counter * fbc,s64 amount,s32 batch)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/pid.h>

struct task_struct * pid_task(struct pid * pid,enum pid_type type)
{
	lx_emul_trace_and_stop(__func__);
}


extern void print_cfs_stats(struct seq_file * m,int cpu);
void print_cfs_stats(struct seq_file * m,int cpu)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/proc_ns.h>

int proc_alloc_inum(unsigned int * inum)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/proc_ns.h>

void proc_free_inum(unsigned int inum)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/file.h>

void put_unused_fd(unsigned int fd)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fb.h>

int register_framebuffer(struct fb_info * fb_info)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/oom.h>

int register_oom_notifier(struct notifier_block * nb)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/vmalloc.h>

int register_vmap_purge_notifier(struct notifier_block * nb)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

void __meminit reserve_bootmem_region(phys_addr_t start,phys_addr_t end,int nid)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/seq_file.h>

void seq_putc(struct seq_file * m,char c)
{
	lx_emul_trace_and_stop(__func__);
}


extern void set_cpus_allowed_common(struct task_struct * p,struct affinity_context * ctx);
void set_cpus_allowed_common(struct task_struct * p,struct affinity_context * ctx)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sched/debug.h>

void show_regs(struct pt_regs * regs)
{
	lx_emul_trace_and_stop(__func__);
}


extern void switch_mm_irqs_off(struct mm_struct * unused,struct mm_struct * next,struct task_struct * tsk);
void switch_mm_irqs_off(struct mm_struct * unused,struct mm_struct * next,struct task_struct * tsk)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sync_file.h>

struct sync_file * sync_file_create(struct dma_fence * fence)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sync_file.h>

struct dma_fence * sync_file_get_fence(int fd)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sysfs.h>

int sysfs_change_owner(struct kobject * kobj,kuid_t kuid,kgid_t kgid)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sysfs.h>

int sysfs_file_change_owner(struct kobject * kobj,const char * name,kuid_t kuid,kgid_t kgid)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sysfs.h>

int sysfs_groups_change_owner(struct kobject * kobj,const struct attribute_group ** groups,kuid_t kuid,kgid_t kgid)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sysfs.h>

int sysfs_link_change_owner(struct kobject * kobj,struct kobject * targ,const char * name,kuid_t kuid,kgid_t kgid)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sysfs.h>

int sysfs_move_dir_ns(struct kobject * kobj,struct kobject * new_parent_kobj,const void * new_ns)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sysfs.h>

int sysfs_rename_dir_ns(struct kobject * kobj,const char * new_name,const void * new_ns)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/sysfs.h>

int sysfs_rename_link_ns(struct kobject * kobj,struct kobject * targ,const char * old,const char * new,const void * new_ns)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/ttm/ttm_bo.h>

void ttm_bo_vunmap(struct ttm_buffer_object * bo,struct iosys_map * map)
{
	lx_emul_trace_and_stop(__func__);
}


#include <drm/ttm/ttm_resource.h>

void ttm_resource_manager_debug(struct ttm_resource_manager * man,struct drm_printer * p)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

void unpin_user_page(struct page * page)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fb.h>

void unregister_framebuffer(struct fb_info * fb_info)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/oom.h>

int unregister_oom_notifier(struct notifier_block * nb)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/vmalloc.h>

int unregister_vmap_purge_notifier(struct notifier_block * nb)
{
	lx_emul_trace_and_stop(__func__);
}


extern s64 update_curr_common(struct rq * rq);
s64 update_curr_common(struct rq * rq)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/timekeeper_internal.h>

void update_vsyscall_tz(void)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mman.h>

s32 vm_committed_as_batch;


#include <linux/mm.h>

pgprot_t vm_get_page_prot(vm_flags_t vm_flags)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/vmalloc.h>

bool vmalloc_dump_obj(void * object)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/mm.h>

struct page * vmalloc_to_page(const void * vmalloc_addr)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/vmalloc.h>

void * vrealloc_node_align_noprof(const void * p,size_t size,unsigned long align,gfp_t flags,int nid)
{
	lx_emul_trace_and_stop(__func__);
}


extern void wake_up_all_tlb_invalidate(struct intel_guc * guc);
void wake_up_all_tlb_invalidate(struct intel_guc * guc)
{
	lx_emul_trace_and_stop(__func__);
}

