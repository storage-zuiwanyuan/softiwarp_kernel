/*
 * Software iWARP device driver for Linux
 *
 * Authors: Bernard Metzler <bmt@zurich.ibm.com>
 *
 * Copyright (c) 2008-2010, IBM Corporation
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *   - Neither the name of IBM nor the names of its contributors may be
 *     used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SIW_CM_H
#define _SIW_CM_H

#include <rdma/iw_cm.h>


enum siw_cep_state {
	SIW_EPSTATE_IDLE = 1,
	SIW_EPSTATE_LISTENING,
	SIW_EPSTATE_CONNECTING,
	SIW_EPSTATE_AWAIT_MPAREQ,
	SIW_EPSTATE_RECVD_MPAREQ,
	SIW_EPSTATE_AWAIT_MPAREP,
	SIW_EPSTATE_RDMA_MODE,
	SIW_EPSTATE_CLOSED
};

struct siw_mpa_info {
	struct mpa_rr	hdr; 	/* peer mpa hdr in host byte order */
	char		*pdata;
	int		bytes_rcvd;
};

struct siw_llp_info {
	struct socket		*sock;
	struct sockaddr_in	laddr;	/* redundant with socket info above */
	struct sockaddr_in	raddr;	/* dito, consider removal */
	struct siw_sk_upcalls	sk_def_upcalls;
};

struct siw_cep {
	struct iw_cm_id		*cm_id;

	/*
	 * The provider_data element of a listener IWCM ID
	 * refers to a list of one or more listener CEPs
	 */
	struct list_head	list;

	struct siw_cep		*listen_cep;
	struct siw_qp		*qp;
	spinlock_t		lock;
	wait_queue_head_t	waitq;
	struct kref		ref;
	enum siw_cep_state	state;
	short			conn_close; /* sched. for closing or closed */
	short			in_use;
	struct siw_cm_work	*mpa_timer;
	struct list_head	work_freelist;
	struct siw_llp_info	llp;
	struct siw_mpa_info	mpa;
	int			ord;
	int			ird;
	int			sk_error; /* not (yet) used XXX */

	/* Saved upcalls of socket llp.sock */
	void    (*sk_state_change)(struct sock *sk);
	void    (*sk_data_ready)(struct sock *sk, int bytes);
	void    (*sk_write_space)(struct sock *sk);
	void    (*sk_error_report)(struct sock *sk);
};

enum siw_work_type {
	SIW_CM_WORK_ACCEPT 	= 1,
	SIW_CM_WORK_READ_MPAHDR,
	SIW_CM_WORK_CLOSE_LLP,		/* close socket */
	SIW_CM_WORK_PEER_CLOSE,		/* socket indicated peer close */
	SIW_CM_WORK_MPATIMEOUT		/* to be done ! */
};

struct siw_cm_work {
	struct work_struct	work;
	struct list_head	list;
	enum siw_work_type	type;
	struct siw_cep	*cep;
};

int siw_connect(struct iw_cm_id *, struct iw_cm_conn_param *);
int siw_accept(struct iw_cm_id *, struct iw_cm_conn_param *);
int siw_reject(struct iw_cm_id *, const void *, u8);
int siw_create_listen(struct iw_cm_id *, int);
int siw_destroy_listen(struct iw_cm_id *);

int siw_cm_upcall(struct siw_cep *, enum iw_cm_event_type,
			    enum iw_cm_event_status);

void siw_cep_upcall(struct siw_cep *, enum iw_cm_event_type);

void siw_cep_put(struct siw_cep *);
void siw_cep_get(struct siw_cep *);
int siw_cep_in_close(struct siw_cep *);

int siw_cm_queue_work(struct siw_cep *, enum siw_work_type);

int siw_cm_init(void);
void siw_cm_exit(void);

#endif