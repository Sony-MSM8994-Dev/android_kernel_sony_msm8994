/*
 *	All files except if stated otherwise in the beginning of the file
 *	are under the ISC license:
 *	----------------------------------------------------------------------
 *	Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *	Copyright (c) 2010-2012 Design Art Networks Ltd.
 *
 *	Permission to use, copy, modify, and/or distribute this software for any
 *	purpose with or without fee is hereby granted, provided that the above
 *	copyright notice and this permission notice appear in all copies.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *	WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *	MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *	ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *	WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *	ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *	OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/



#ifndef __UAPI_LINUX_DANIPC_IOCTL_H__
#define __UAPI_LINUX_DANIPC_IOCTL_H__

#include <linux/types.h>

#define DANIPC_IOCS_REGISTER	(SIOCDEVPRIVATE + 0)
#define DANIPC_IOCG_ADDR2NAME	(SIOCDEVPRIVATE + 1)
#define DANIPC_IOCG_NAME2ADDR	(SIOCDEVPRIVATE + 2)
#define DANIPC_IOCS_MMSGSEND	(SIOCDEVPRIVATE + 3)
#define DANIPC_IOCS_MMSGRECV	(SIOCDEVPRIVATE + 4)
#define DANIPC_IOCG_RECV	(SIOCDEVPRIVATE + 5)
#define DANIPC_IOCS_RECVACK		(SIOCDEVPRIVATE + 6)
#define DANIPC_IOCS_RECVACK_RECV	(SIOCDEVPRIVATE + 7)
#define DANIPC_IOCS_SEND		(SIOCDEVPRIVATE + 8)
#define DANIPC_IOCG_GET_SENDBUF		(SIOCDEVPRIVATE + 9)
#define DANIPC_IOCG_SEND_GET_SENDBUF	(SIOCDEVPRIVATE + 10)
#define DANIPC_IOCS_RET_SENDBUF		(SIOCDEVPRIVATE + 11)
#define DANIPC_IOCS_RECVACK_RECV_MAX	(SIOCDEVPRIVATE + 12)

#define MAX_AGENTS		256
#define MAX_AGENT_NAME		32

#define INVALID_ID		((unsigned)(-1))

struct danipc_reg {
	/* (IN) Agent name */
	char			name[MAX_AGENT_NAME];

	/* (IN) Actually this is priority from enum IPC_trns_priority */
	unsigned		prio;

	/* (IN) If different from INVALID_ID specifies preferred local ID. */
	unsigned		requested_lid;

	/* (OUT) Assigned local ID. */
	unsigned		assigned_lid;

	/* (OUT) Cookie in network format */
	uint16_t		cookie;
};


typedef uint8_t	__bitwise	danipc_addr_t;

struct danipc_name {
	char			name[MAX_AGENT_NAME];
	danipc_addr_t		addr;
};

#define DANIPC_MMAP_AID_SHIFT		24
#define DANIPC_MMAP_LID_SHIFT		16
#define DANIPC_MMAP_AID_OFFSET(aid) (((aid) & 0xFF) << DANIPC_MMAP_AID_SHIFT)
#define DANIPC_MMAP_LID_OFFSET(lid) (((lid) & 0xFF) << DANIPC_MMAP_LID_SHIFT)
#define DANIPC_MMAP_OFFSET(aid, lid) \
	(DANIPC_MMAP_AID_OFFSET(aid) | DANIPC_MMAP_LID_OFFSET(lid))


#define DANIPC_MAX_BUF 1600
#define DANIPC_BUFS_MAX_NUM_BUF 8
#define DANIPC_MMAP_TX_BUF_HEADROOM 0

struct danipc_cdev_msghdr {
	uint8_t		dst;
	uint8_t		src;
	uint16_t	prio;
} __packed;

struct danipc_buf_entry {
	void		*data;
	unsigned	data_len;
};

struct danipc_bufs {
	unsigned		num_entry;
	struct danipc_buf_entry	entry[DANIPC_BUFS_MAX_NUM_BUF];
};

struct danipc_cdev_mmsg {
	struct danipc_cdev_msghdr	hdr;
	struct danipc_bufs		msgs;
};

#endif /* __UAPI_LINUX_DANIPC_IOCTL_H__ */
