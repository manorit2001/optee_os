/* SPDX-License-Identifier: BSD-2-Clause */
#include <types_ext.h>

#define SMALL_PAGE_SIZE 4096
#define MEM_AREA_TRANSFER_LIST 0
void *core_mmu_add_mapping(int type, paddr_t pa, size_t size);
int core_mmu_remove_mapping(int type, void *va, size_t size);
