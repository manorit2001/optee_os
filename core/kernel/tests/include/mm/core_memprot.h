/* SPDX-License-Identifier: BSD-2-Clause */
#include <types_ext.h>

#define DCACHE_AREA_CLEAN 0
paddr_t virt_to_phys(const void *va);
int cache_op_inner(int op, void *va, size_t size);
int cache_op_outer(int op, paddr_t pa, size_t size);
