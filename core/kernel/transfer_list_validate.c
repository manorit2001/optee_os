// SPDX-License-Identifier: BSD-2-Clause

#include <util.h>
#include <kernel/transfer_list.h>

static uint32_t read_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
	       (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

bool transfer_list_validate(const void *data, size_t available)
{
	const uint8_t *buf = data;
	uint32_t used = 0;
	uint32_t total = 0;
	uint8_t hdr_size = 0;
	size_t off = 0;
	uint8_t checksum = 0;
	size_t i = 0;

	if (!buf ||
	    ((uintptr_t)buf & (TRANSFER_LIST_GRANULE - 1)) ||
	    available < sizeof(struct transfer_list_header))
		return false;
	used = read_le32(buf + offsetof(struct transfer_list_header, size));
	total = read_le32(buf +
			  offsetof(struct transfer_list_header, max_size));
	hdr_size = buf[offsetof(struct transfer_list_header, hdr_size)];
	if (read_le32(buf) != TRANSFER_LIST_SIGNATURE ||
	    !buf[offsetof(struct transfer_list_header, version)] ||
	    hdr_size < sizeof(struct transfer_list_header) ||
	    buf[offsetof(struct transfer_list_header, alignment)] <
						TRANSFER_LIST_INIT_MAX_ALIGN ||
	    buf[offsetof(struct transfer_list_header, alignment)] >=
						sizeof(unsigned long) * 8 ||
	    used < hdr_size || used > total ||
	    total > available || (used & (TRANSFER_LIST_GRANULE - 1)) ||
	    (total & (TRANSFER_LIST_GRANULE - 1)))
		return false;

	if (read_le32(buf + offsetof(struct transfer_list_header, flags)) &
	    TL_FLAGS_HAS_CHECKSUM) {
		for (i = 0; i < used; i++)
			checksum += buf[i];
		if (checksum)
			return false;
	}

	off = ROUNDUP(hdr_size, TRANSFER_LIST_GRANULE);
	if (off > used)
		return false;
	while (off < used) {
		uint8_t entry_hdr_size = 0;
		uint32_t data_size = 0;
		size_t entry_end = 0;
		size_t padding = 0;

		if (used - off < sizeof(struct transfer_list_entry))
			return false;
		entry_hdr_size = buf[off +
				     offsetof(struct transfer_list_entry,
					      hdr_size)];
		data_size = read_le32(buf + off +
				      offsetof(struct transfer_list_entry,
					       data_size));
		if (entry_hdr_size < sizeof(struct transfer_list_entry) ||
		    entry_hdr_size > used - off ||
		    data_size > used - off - entry_hdr_size)
			return false;
		entry_end = off + entry_hdr_size + data_size;
		padding = (TRANSFER_LIST_GRANULE -
			   (entry_end & (TRANSFER_LIST_GRANULE - 1))) &
			  (TRANSFER_LIST_GRANULE - 1);
		if (padding > used - entry_end)
			return false;
		off = entry_end + padding;
	}

	return off == used;
}
