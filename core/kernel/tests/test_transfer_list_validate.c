// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <util.h>
#include <kernel/transfer_list.h>

#undef assert
#define assert(cond) do { if (!(cond)) return __LINE__; } while (0)

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
	assert(transfer_list_validate(tl, sizeof(tl)));

	put_le32(tl + 32 + 4, UINT32_MAX);
	finish_list(tl, 48, sizeof(tl), 32);
	assert(!transfer_list_validate(tl, sizeof(tl)));

	put_le32(tl + 32 + 4, 3);
	finish_list(tl, 48, sizeof(tl), 32);
	tl[4]++;
	assert(!transfer_list_validate(tl, sizeof(tl)));

	return 0;
}
