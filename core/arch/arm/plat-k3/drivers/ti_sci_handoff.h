/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef TI_SCI_HANDOFF_H
#define TI_SCI_HANDOFF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool k3_sec_proxy_handoff_init(const void *data, size_t buffer_size,
			       uint32_t max_thread);
uint8_t k3_sec_proxy_handoff_host_id(void);
uint16_t k3_sec_proxy_handoff_rx_thread(void);
uint16_t k3_sec_proxy_handoff_tx_thread(void);

#endif /* TI_SCI_HANDOFF_H */
