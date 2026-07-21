// SPDX-License-Identifier: MIT
/*
 * Copyright © 2020 Intel Corporation
 */

#include "xe_migrate.h"

#include <linux/bitfield.h>
#include <linux/sizes.h>

#include <drm/drm_managed.h>
#include <drm/drm_pagemap.h>
#include <drm/ttm/ttm_tt.h>
#include <uapi/drm/xe_drm.h>

#include <generated/xe_wa_oob.h>

#include "instructions/xe_gpu_commands.h"
#include "instructions/xe_mi_commands.h"
#include "regs/xe_gtt_defs.h"
#include "tests/xe_test.h"
#include "xe_assert.h"
#include "xe_bb.h"
#include "xe_bo.h"
#include "xe_exec_queue.h"
#include "xe_ggtt.h"
#include "xe_gt.h"
#include "xe_hw_engine.h"
#include "xe_lrc.h"
#include "xe_map.h"
#include "xe_mocs.h"
#include "xe_pt.h"
#include "xe_res_cursor.h"
#include "xe_sa.h"
#include "xe_sched_job.h"
#include "xe_sync.h"
#include "xe_trace_bo.h"
#include "xe_validation.h"
#include "xe_vm.h"
#include "xe_vram.h"

/**
 * struct xe_migrate - migrate context.
 */
struct xe_migrate {
	/** @q: Default exec queue used for migration */
	struct xe_exec_queue *q;
	/** @tile: Backpointer to the tile this struct xe_migrate belongs to. */
	struct xe_tile *tile;
	/** @job_mutex: Timeline mutex for @eng. */
	struct mutex job_mutex;
	/** @pt_bo: Page-table buffer object. */
	struct xe_bo *pt_bo;
	/** @batch_base_ofs: VM offset of the migration batch buffer */
	u64 batch_base_ofs;
	/** @usm_batch_base_ofs: VM offset of the usm batch buffer */
	u64 usm_batch_base_ofs;
	/** @cleared_mem_ofs: VM offset of @cleared_bo. */
	u64 cleared_mem_ofs;
	/**
	 * @fence: dma-fence representing the last migration job batch.
	 * Protected by @job_mutex.
	 */
	struct dma_fence *fence;
	/**
	 * @vm_update_sa: For integrated, used to suballocate page-tables
	 * out of the pt_bo.
	 */
	struct drm_suballoc_manager vm_update_sa;
	/** @min_chunk_size: For dgfx, Minimum chunk size */
	u64 min_chunk_size;
};


struct xe_migrate *xe_migrate_alloc(struct xe_tile *tile)
{
	struct xe_migrate *m = drmm_kzalloc(&tile_to_xe(tile)->drm, sizeof(*m), GFP_KERNEL);

	if (m)
		m->tile = tile;
	return m;
}


struct dma_fence *xe_migrate_clear(struct xe_migrate *m,
				   struct xe_bo *bo,
				   struct ttm_resource *dst,
				   u32 clear_flags)
{
	printk("%s:%u - skip\n", __func__, __LINE__);
	return NULL;
}
