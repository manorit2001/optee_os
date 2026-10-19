// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <string.h>

#include <util.h>
#include <kernel/transfer_list.h>

#include "../drivers/ti_sci_handoff.h"

#undef assert
#define assert(cond) do { if (!(cond)) return __LINE__; } while (0)

#define TL_HEADER_SIZE 24
#define TE_SIZE 8
#define OPTEE_DATA_SIZE 32
#define UNKNOWN_DATA_SIZE 4

static void put_le32(uint8_t *data, uint32_t val)
{
	data[0] = val;
	data[1] = val >> 8;
	data[2] = val >> 16;
	data[3] = val >> 24;
}

static void add_optee_record(uint8_t *tl, size_t offset, uint32_t tx_role_a,
			     uint32_t tx_role_b)
{
	uint8_t *entry = tl + offset;
	uint8_t *data = entry + TE_SIZE;

	entry[0] = 0x00;
	entry[1] = 0xf0;
	entry[2] = 0xff;
	entry[3] = TE_SIZE;
	put_le32(entry + 4, OPTEE_DATA_SIZE);
	put_le32(data, 3);
	put_le32(data + 4, 12);
	put_le32(data + 8, 6);
	put_le32(data + 12, 2);
	put_le32(data + 16, tx_role_a);
	put_le32(data + 20, 7);
	put_le32(data + 24, tx_role_b);
	put_le32(data + 28, 8);
}

static void add_unknown_consumer(uint8_t *tl, size_t offset)
{
	uint8_t *entry = tl + offset;

	entry[1] = 0xf0;
	entry[2] = 0xff;
	entry[3] = TE_SIZE;
	put_le32(entry + 4, UNKNOWN_DATA_SIZE);
	put_le32(entry + TE_SIZE, 7);
}

static void finish_list(uint8_t *tl, size_t size)
{
	uint8_t sum = 0;
	size_t i = 0;

	put_le32(tl, 0x4a0fb10b);
	tl[4] = 0;
	tl[5] = 2;
	tl[6] = TL_HEADER_SIZE;
	tl[7] = 3;
	put_le32(tl + 8, size);
	put_le32(tl + 12, size);
	put_le32(tl + 16, 1);
	put_le32(tl + 20, 0);
	for (i = 0; i < size; i++)
		sum += tl[i];
	tl[4] = -sum;
}

struct transfer_list_entry *transfer_list_next(struct transfer_list_header *tl,
					       struct transfer_list_entry *last)
{
	uintptr_t start = (uintptr_t)tl + tl->hdr_size;
	uintptr_t end = (uintptr_t)tl + tl->size;
	struct transfer_list_entry *entry = NULL;

	if (last)
		start = ROUNDUP((uintptr_t)last + last->hdr_size +
				last->data_size, TRANSFER_LIST_GRANULE);
	if (start > end || end - start < sizeof(*entry))
		return NULL;
	entry = (struct transfer_list_entry *)start;
	if (entry->hdr_size < sizeof(*entry) ||
	    entry->hdr_size > end - start ||
	    entry->data_size > end - start - entry->hdr_size)
		return NULL;
	return entry;
}

void *transfer_list_entry_data(struct transfer_list_entry *entry)
{
	return (uint8_t *)entry + entry->hdr_size;
}

int main(void)
{
	_Alignas(8) uint8_t tl[TL_HEADER_SIZE + TE_SIZE + OPTEE_DATA_SIZE] = {
		0
	};
	_Alignas(8) uint8_t duplicate[TL_HEADER_SIZE +
				      2 * (TE_SIZE + OPTEE_DATA_SIZE)] = { 0 };
	_Alignas(8) uint8_t future[TL_HEADER_SIZE + 16 + TE_SIZE +
				   OPTEE_DATA_SIZE] = { 0 };
	struct k3_sec_proxy_config config = { 0 };

	add_optee_record(tl, TL_HEADER_SIZE, 1, 2);
	finish_list(tl, sizeof(tl));
	assert(k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	assert(config.host_id == 12 && config.rx_thread == 6);
	assert(config.tx_request_low == 8 && config.tx_request_high == 7);
	assert(k3_sec_proxy_handoff_host_id() == 12);
	assert(k3_sec_proxy_handoff_rx_thread() == 6);
	assert(k3_sec_proxy_handoff_tx_thread() == 8);
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 8, 8);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	add_optee_record(tl, TL_HEADER_SIZE, 1, 2);
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 20, 8);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	add_optee_record(tl, TL_HEADER_SIZE, 1, 2);
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 28, 7);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	add_optee_record(tl, TL_HEADER_SIZE, 1, 2);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl) - 1, 16, &config));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 8, &config));
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 4, 256);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	add_optee_record(tl, TL_HEADER_SIZE, 1, 2);
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 20, 65536);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), UINT32_MAX,
					   &config));
	add_optee_record(tl, TL_HEADER_SIZE, 1, 2);
	finish_list(tl, sizeof(tl));

	tl[4]++;
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	tl[4]--;

	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 12, UINT32_MAX);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	add_optee_record(tl, TL_HEADER_SIZE, 1, 2);
	put_le32(tl + TL_HEADER_SIZE + 4, UINT32_MAX);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));

	add_optee_record(duplicate, TL_HEADER_SIZE, 1, 2);
	add_optee_record(duplicate, TL_HEADER_SIZE + TE_SIZE + OPTEE_DATA_SIZE,
			 1, 2);
	finish_list(duplicate, sizeof(duplicate));
	assert(!k3_sec_proxy_handoff_parse(duplicate, sizeof(duplicate), 16,
					   &config));
	add_unknown_consumer(future, TL_HEADER_SIZE);
	add_optee_record(future, TL_HEADER_SIZE + 16, 1, 2);
	finish_list(future, sizeof(future));
	assert(k3_sec_proxy_handoff_parse(future, sizeof(future), 16, &config));

	add_optee_record(tl, TL_HEADER_SIZE, 0, 2);
	put_le32(tl + TL_HEADER_SIZE + 4, 24);
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 12, 1);
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 20, 5);
	finish_list(tl, TL_HEADER_SIZE + TE_SIZE + 24);
	assert(k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));
	assert(k3_sec_proxy_handoff_tx_thread() == 5);

	add_optee_record(tl, TL_HEADER_SIZE, 2, 2);
	put_le32(tl + TL_HEADER_SIZE + TE_SIZE + 12, 2);
	finish_list(tl, sizeof(tl));
	assert(!k3_sec_proxy_handoff_parse(tl, sizeof(tl), 16, &config));

	return 0;
}
