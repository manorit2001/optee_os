/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef TI_SCI_HANDOFF_H
#define TI_SCI_HANDOFF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct k3_sec_proxy_config {
	uint32_t host_id;
	uint32_t rx_thread;
	uint32_t tx_request;
	uint32_t tx_request_high;
	uint32_t tx_request_low;
};

bool k3_sec_proxy_handoff_parse(const void *data, size_t buffer_size,
				uint32_t max_thread,
				struct k3_sec_proxy_config *config);
uint8_t k3_sec_proxy_handoff_host_id(void);
uint16_t k3_sec_proxy_handoff_rx_thread(void);
uint16_t k3_sec_proxy_handoff_tx_thread(void);

#endif /* TI_SCI_HANDOFF_H */
