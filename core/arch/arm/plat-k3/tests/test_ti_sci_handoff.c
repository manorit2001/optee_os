// SPDX-License-Identifier: BSD-2-Clause

#include "../drivers/ti_sci_handoff.h"

/* Reuse the real-library mapping harness, not a substitute TL iterator. */
#define main test_transfer_list_main
#include "../../../../kernel/tests/test_transfer_list_validate.c"
#undef main

static void add_record(uint8_t *tl, size_t offset, uint32_t consumer,
		       uint32_t rx, uint32_t tx)
{
	uint8_t *entry = tl + offset;

	put_le32(entry, 0x08fff000);
	put_le32(entry + 4, 24);
	put_le32(entry + 8, consumer);
	put_le32(entry + 12, 12);
	put_le32(entry + 16, rx);
	put_le32(entry + 20, 1);
	put_le32(entry + 24, 0);
	put_le32(entry + 28, tx);
}

int main(void)
{
	_Alignas(8) uint8_t data[88] = { 0 };

	assert(!test_transfer_list_main());
	guard_tail = false;
	map_calls = 0;
	add_record(data, 24, 3, 6, 7);
	finish_list(data, 56, sizeof(data), 24);
	assert(k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	assert(k3_sec_proxy_handoff_host_id() == 12);
	assert(k3_sec_proxy_handoff_rx_thread() == 6);
	assert(k3_sec_proxy_handoff_tx_thread() == 7);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data) - 1, 16));
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 7));
	data[4]++;
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	data[4]--;
	put_le32(data + 36, 256);
	finish_list(data, 56, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	add_record(data, 24, 3, 7, 7);
	finish_list(data, 56, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	add_record(data, 24, 3, 6, 65536);
	finish_list(data, 56, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), UINT32_MAX));
	add_record(data, 24, 3, 6, 7);
	add_record(data, 56, 3, 8, 9);
	finish_list(data, 88, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	/* Other consumers must not inherit OP-TEE's local hardware window. */
	add_record(data, 56, 2, 200, 201);
	finish_list(data, 88, sizeof(data), 24);
	assert(k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	add_record(data, 56, 99, 200, 201);
	finish_list(data, 88, sizeof(data), 24);
	assert(k3_sec_proxy_handoff_init(data, sizeof(data), 16));

	put_le32(data + 64, 0);
	finish_list(data, 88, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	put_le32(data + 56, 0x08abcdef);
	finish_list(data, 88, sizeof(data), 24);
	assert(k3_sec_proxy_handoff_init(data, sizeof(data), 16));

	/* Cover priority roles, duplicate threads and malformed payloads. */
	memset(data, 0, sizeof(data));
	add_record(data, 24, 3, 6, 7);
	put_le32(data + 28, 32);
	put_le32(data + 44, 2);
	put_le32(data + 48, 1);
	put_le32(data + 56, 2);
	put_le32(data + 60, 8);
	finish_list(data, 64, sizeof(data), 24);
	assert(k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	assert(k3_sec_proxy_handoff_tx_thread() == 8);
	put_le32(data + 40, 8);
	finish_list(data, 64, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	put_le32(data + 40, 6);
	put_le32(data + 60, 7);
	finish_list(data, 64, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	put_le32(data + 60, 8);
	for (uint32_t role = 0; role <= 3; role++) {
		if (role == 2)
			continue;
		put_le32(data + 56, role);
		finish_list(data, 64, sizeof(data), 24);
		assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	}
	put_le32(data + 28, 24);
	put_le32(data + 44, 1);
	finish_list(data, 56, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	put_le32(data + 44, UINT32_MAX);
	finish_list(data, 56, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	put_le32(data + 28, UINT32_MAX);
	finish_list(data, 56, sizeof(data), 24);
	assert(!k3_sec_proxy_handoff_init(data, sizeof(data), 16));
	assert(k3_sec_proxy_handoff_tx_thread() == 8);

	return 0;
}
