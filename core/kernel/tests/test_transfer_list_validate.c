// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <util.h>
#include <kernel/transfer_list.h>
#include <mm/core_mmu.h>

static _Alignas(4096) uint8_t ram[8192];
static const paddr_t ram_pa = 0x50000000;
static unsigned int map_calls;
static unsigned int fail_map_call;
static bool change_header;
static bool guard_tail;
static bool expect_panic;
static jmp_buf panic_env;

void test_panic(void)
{
	if (expect_panic)
		longjmp(panic_env, 1);
	abort();
}

void *core_mmu_add_mapping(int type, paddr_t pa, size_t size)
{
	assert(type == MEM_AREA_TRANSFER_LIST && pa == ram_pa);
	map_calls++;
	if (map_calls == fail_map_call || size > sizeof(ram))
		return NULL;
	if (guard_tail && size > SMALL_PAGE_SIZE)
		assert(!mprotect(ram + SMALL_PAGE_SIZE, SMALL_PAGE_SIZE,
				 PROT_READ | PROT_WRITE));
	if (change_header && map_calls == 2)
		((struct transfer_list_header *)ram)->max_size++;
	return ram;
}

int core_mmu_remove_mapping(int type, void *va, size_t size)
{
	assert(type == MEM_AREA_TRANSFER_LIST && va == ram && size);
	return 0;
}

paddr_t virt_to_phys(const void *va)
{
	assert((const uint8_t *)va >= ram &&
	       (const uint8_t *)va < ram + sizeof(ram));
	return ram_pa + ((const uint8_t *)va - ram);
}

int cache_op_inner(int op, void *va, size_t size)
{
	(void)op;
	(void)va;
	(void)size;
	return 0;
}

int cache_op_outer(int op, paddr_t pa, size_t size)
{
	(void)op;
	(void)pa;
	(void)size;
	return 0;
}

static void put_le32(uint8_t *data, uint32_t val)
{
	data[0] = val;
	data[1] = val >> 8;
	data[2] = val >> 16;
	data[3] = val >> 24;
}

static void finish_list(uint8_t *tl, size_t used, size_t total,
			uint8_t hdr_size)
{
	uint8_t sum = 0;
	size_t i = 0;

	put_le32(tl, TRANSFER_LIST_SIGNATURE);
	tl[4] = 0;
	tl[5] = TRANSFER_LIST_VERSION;
	tl[6] = hdr_size;
	tl[7] = TRANSFER_LIST_INIT_MAX_ALIGN;
	put_le32(tl + 8, used);
	put_le32(tl + 12, total);
	put_le32(tl + 16, TL_FLAGS_HAS_CHECKSUM);
	for (i = 0; i < used; i++)
		sum += tl[i];
	tl[4] = -sum;
}

int main(void)
{
	_Alignas(8) uint8_t tl[64] = { 0 };
	struct transfer_list_header *list = NULL;
	struct transfer_list_entry *entry = NULL;
	uint8_t payload[5000] = { 1 };
	size_t len = 0;

	tl[24 + 3] = sizeof(struct transfer_list_entry);
	put_le32(tl + 24 + 4, 3);
	tl[24 + 8] = 1;
	tl[24 + 9] = 2;
	tl[24 + 10] = 3;
	finish_list(tl, 40, sizeof(tl), 24);
	assert(transfer_list_validate(tl, sizeof(tl)));
	assert(!transfer_list_validate(tl, sizeof(tl) - 1));

	/* Preserve larger-alignment and newer-version support. */
	tl[5]++;
	tl[7]++;
	tl[4] -= 2;
	assert(transfer_list_validate(tl, sizeof(tl)));
	finish_list(tl, 40, sizeof(tl), 24);

	tl[32 + 3] = sizeof(struct transfer_list_entry);
	put_le32(tl + 32 + 4, 3);
	finish_list(tl, 48, sizeof(tl), 32);
	assert(!transfer_list_validate(tl, sizeof(tl)));

	put_le32(tl + 32 + 4, UINT32_MAX);
	finish_list(tl, 48, sizeof(tl), 32);
	assert(!transfer_list_validate(tl, sizeof(tl)));

	put_le32(tl + 32 + 4, 3);
	finish_list(tl, 48, sizeof(tl), 32);
	tl[4]++;
	assert(!transfer_list_validate(tl, sizeof(tl)));

	/* Test the producer and iterator with unpadded final data. */
	for (len = 1; len <= 9; len++) {
		list = transfer_list_init(ram_pa, sizeof(ram));
		assert(list && transfer_list_add(list, 2, len, payload));
		assert(list->size == sizeof(*list) + sizeof(*entry) + len);
		assert(transfer_list_validate(list, sizeof(ram)));
		assert(transfer_list_add(list, 3, len, payload));
		assert(transfer_list_validate(list, sizeof(ram)));
		assert(!transfer_list_validate(list, sizeof(ram) - 1));
	}

	/* A checksum walk into the second page before remapping must fault. */
	assert(transfer_list_add(list, 2, sizeof(payload), payload));
	guard_tail = true;
	assert(!mprotect(ram + SMALL_PAGE_SIZE, SMALL_PAGE_SIZE, PROT_NONE));
	assert(!transfer_list_validate(list, SMALL_PAGE_SIZE));
	map_calls = 0;
	assert(transfer_list_map(ram_pa) == list && map_calls == 2);

	map_calls = 0;
	fail_map_call = 1;
	assert(!transfer_list_map(ram_pa));
	map_calls = 0;
	fail_map_call = 2;
	assert(!transfer_list_map(ram_pa));
	fail_map_call = 0;
	map_calls = 0;
	change_header = true;
	assert(!transfer_list_map(ram_pa));
	change_header = false;
	list->max_size = sizeof(ram);
	list->alignment = sizeof(unsigned long) * 8;
	map_calls = 0;
	assert(!transfer_list_map(ram_pa) && map_calls == 1);
	list->alignment = TRANSFER_LIST_INIT_MAX_ALIGN;
	transfer_list_update_checksum(list);
	list->checksum++;
	assert(!transfer_list_map(ram_pa));

	return 0;
}
