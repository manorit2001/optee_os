// SPDX-License-Identifier: BSD-2-Clause

#include <util.h>
#include <kernel/transfer_list.h>

#include "ti_sci_handoff.h"

#define K3_SEC_PROXY_HANDOFF_TAG 0xfff000
#define K3_SEC_PROXY_HANDOFF_CONSUMER_OPTEE 3
#define K3_SEC_PROXY_HANDOFF_MAX_TX 3
#define K3_SEC_PROXY_HANDOFF_MAX_SIZE 4096U

struct k3_sec_proxy_config {
	uint32_t host_id;
	uint32_t rx_thread;
	uint32_t tx_request;
	uint32_t tx_request_high;
	uint32_t tx_request_low;
};

static struct k3_sec_proxy_config handoff_config;

static uint32_t read_le32(const uint8_t *data)
{
	return (uint32_t)data[0] | (uint32_t)data[1] << 8 |
	       (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

static bool parse_consumer(const uint8_t *data, size_t size,
			   uint32_t max_thread,
			   struct k3_sec_proxy_config *config)
{
	uint32_t tx_threads[K3_SEC_PROXY_HANDOFF_MAX_TX] = { 0 };
	uint32_t count = 0;
	uint32_t roles = 0;
	size_t expected_size = 0;
	size_t i = 0;

	if (size < 16)
		return false;

	config->host_id = read_le32(data + 4);
	config->rx_thread = read_le32(data + 8);
	count = read_le32(data + 12);
	if (count == 0 || count > K3_SEC_PROXY_HANDOFF_MAX_TX)
		return false;
	expected_size = 16 + count * 8;
	if (size != expected_size || config->host_id > UINT8_MAX ||
	    config->rx_thread > UINT16_MAX || config->rx_thread >= max_thread)
		return false;

	config->tx_request = UINT32_MAX;
	config->tx_request_high = UINT32_MAX;
	config->tx_request_low = UINT32_MAX;
	for (i = 0; i < count; i++) {
		uint32_t role = read_le32(data + 16 + i * 8);
		uint32_t thread = read_le32(data + 20 + i * 8);
		uint32_t bit = 0;
		uint32_t *slot = NULL;

		if (role > 2 || thread > UINT16_MAX || thread >= max_thread ||
		    thread == config->rx_thread)
			return false;
		for (size_t j = 0; j < i; j++) {
			if (thread == tx_threads[j])
				return false;
		}
		tx_threads[i] = thread;
		bit = 1U << role;
		if (roles & bit)
			return false;
		roles |= bit;
		switch (role) {
		case 0:
			slot = &config->tx_request;
			break;
		case 1:
			slot = &config->tx_request_high;
			break;
		case 2:
			slot = &config->tx_request_low;
			break;
		default:
			return false;
		}
		*slot = thread;
	}

	return roles == (1U << 0) || roles == ((1U << 1) | (1U << 2));
}

bool k3_sec_proxy_handoff_init(const void *data, size_t buffer_size,
			       uint32_t max_thread)
{
	struct transfer_list_header *tl = (void *)data;
	struct k3_sec_proxy_config selected = { 0 };
	uint32_t consumers_seen = 0;
	bool found_optee = false;
	struct transfer_list_entry *entry = NULL;

	if (!tl || max_thread == 0 ||
	    !transfer_list_validate(data, buffer_size) ||
	    tl->version != TRANSFER_LIST_VERSION ||
	    tl->hdr_size != sizeof(*tl) ||
	    tl->alignment != TRANSFER_LIST_INIT_MAX_ALIGN ||
	    tl->flags != TL_FLAGS_HAS_CHECKSUM || tl->reserved)
		return false;
	while ((entry = transfer_list_next(tl, entry))) {
		uint32_t tag = ((uint32_t)entry->reserved0 << 16) |
			       entry->tag_id;
		uint8_t *payload = transfer_list_entry_data(entry);
		uint32_t consumer_id = 0;
		struct k3_sec_proxy_config record = { 0 };
		bool is_optee = false;

		if (tag != K3_SEC_PROXY_HANDOFF_TAG)
			continue;
		if (entry->data_size < sizeof(uint32_t))
			return false;
		consumer_id = read_le32(payload);
		if (!consumer_id)
			return false;
		if (consumer_id > 6)
			continue;
		if (consumers_seen & (1U << (consumer_id - 1)))
			return false;
		consumers_seen |= 1U << (consumer_id - 1);
		is_optee = consumer_id == K3_SEC_PROXY_HANDOFF_CONSUMER_OPTEE;
		if (!parse_consumer(payload, entry->data_size,
				    is_optee ? max_thread : UINT16_MAX + 1U,
				    &record))
			return false;
		if (is_optee) {
			selected = record;
			found_optee = true;
		}
	}

	if (!found_optee)
		return false;
	handoff_config = selected;
	return true;
}

uint8_t k3_sec_proxy_handoff_host_id(void)
{
	return handoff_config.host_id;
}

uint16_t k3_sec_proxy_handoff_rx_thread(void)
{
	return handoff_config.rx_thread;
}

uint16_t k3_sec_proxy_handoff_tx_thread(void)
{
	if (handoff_config.tx_request != UINT32_MAX)
		return handoff_config.tx_request;
	return handoff_config.tx_request_low;
}
