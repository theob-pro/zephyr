/* Copyright (c) 2023 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/buf.h>

struct btsnoop_hdr {
        char id_pattern[8];
        uint32_t version;
        uint32_t datalink_type;
} __packed;

struct btsnoop_pkt_record {
        uint32_t original_len;
        uint32_t included_len;
        uint32_t flags;
        uint32_t drops;
        int64_t ts;
} __packed;

enum bt_btsnoop_flag {
	BT_BTSNOOP_FLAG_ACL_RX_PKT = 0x00,
	BT_BTSNOOP_FLAG_ACL_TX_PKT = 0x01,
	BT_BTSNOOP_FLAG_CMD_PKT = 0x02,
	BT_BTSNOOP_FLAG_EVT_PKT = 0x03,
};

static inline enum bt_btsnoop_flag bt_btsnoop_get_flag(struct net_buf *buf)
{
        switch (bt_buf_get_type(buf)) {
        case BT_BUF_CMD:
                return BT_BTSNOOP_FLAG_CMD_PKT;
        case BT_BUF_EVT:
                return BT_BTSNOOP_FLAG_EVT_PKT;
        case BT_BUF_ACL_IN:
                return BT_BTSNOOP_FLAG_ACL_RX_PKT;
        case BT_BUF_ACL_OUT:
		return BT_BTSNOOP_FLAG_ACL_TX_PKT;
	case BT_BUF_H4:
	case BT_BUF_ISO_IN:
	case BT_BUF_ISO_OUT:
                break;
	}

        k_oops();
	return 0xff;
}

void btsnoop_init(void);
void btsnoop_write_hdr(struct btsnoop_hdr hdr);
void btsnoop_write(struct btsnoop_pkt_record pkt, const void *data, size_t len);

void bt_btsnoop_write(enum bt_btsnoop_flag flag, const void *data, size_t len);
