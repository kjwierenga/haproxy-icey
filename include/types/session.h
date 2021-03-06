/*
  include/types/session.h
  This file defines everything related to sessions.

  Copyright (C) 2000-2008 Willy Tarreau - w@1wt.eu

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, version 2.1
  exclusively.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _TYPES_SESSION_H
#define _TYPES_SESSION_H


#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <common/config.h>
#include <common/mini-clist.h>

#include <types/buffers.h>
#include <types/proto_http.h>
#include <types/proxy.h>
#include <types/queue.h>
#include <types/server.h>
#include <types/stream_interface.h>
#include <types/task.h>


/* various session flags, bits values 0x01 to 0x100 (shift 0) */
#define SN_DIRECT	0x00000001	/* connection made on the server matching the client cookie */
#define SN_ASSIGNED	0x00000002	/* no need to assign a server to this session */
#define SN_ADDR_SET	0x00000004	/* this session's server address has been set */
#define SN_BE_ASSIGNED	0x00000008	/* a backend was assigned. Conns are accounted. */
#define SN_CONN_CLOSED	0x00000010	/* "Connection: close" was present or added */
#define SN_MONITOR	0x00000020	/* this session comes from a monitoring system */
#define SN_CURR_SESS	0x00000040	/* a connection is currently being counted on the server */
#define SN_FRT_ADDR_SET	0x00000080	/* set if the frontend address has been filled */
#define SN_REDISP	0x00000100	/* set if this session was redispatched from one server to another */
#define SN_CONN_TAR	0x00000200	/* set if this session is turning around before reconnecting */
#define SN_REDIRECTABLE	0x00000400	/* set if this session is redirectable (GET or HEAD) */
/* unused:              0x00000800 */

/* session termination conditions, bits values 0x1000 to 0x7000 (0-7 shift 12) */
#define SN_ERR_NONE     0x00000000
#define SN_ERR_CLITO	0x00001000	/* client time-out */
#define SN_ERR_CLICL	0x00002000	/* client closed (read/write error) */
#define SN_ERR_SRVTO	0x00003000	/* server time-out, connect time-out */
#define SN_ERR_SRVCL	0x00004000	/* server closed (connect/read/write error) */
#define SN_ERR_PRXCOND	0x00005000	/* the proxy decided to close (deny...) */
#define SN_ERR_RESOURCE	0x00006000	/* the proxy encountered a lack of a local resources (fd, mem, ...) */
#define SN_ERR_INTERNAL	0x00007000	/* the proxy encountered an internal error */
#define SN_ERR_MASK	0x00007000	/* mask to get only session error flags */
#define SN_ERR_SHIFT	12		/* bit shift */
/* unused:              0x00008000 */

/* session state at termination, bits values 0x10000 to 0x70000 (0-7 shift 16) */
#define SN_FINST_R	0x00010000	/* session ended during client request */
#define SN_FINST_C	0x00020000	/* session ended during server connect */
#define SN_FINST_H	0x00030000	/* session ended during server headers */
#define SN_FINST_D	0x00040000	/* session ended during data phase */
#define SN_FINST_L	0x00050000	/* session ended while pushing last data to client */
#define SN_FINST_Q	0x00060000	/* session ended while waiting in queue for a server slot */
#define SN_FINST_T	0x00070000	/* session ended tarpitted */
#define SN_FINST_MASK	0x00070000	/* mask to get only final session state flags */
#define	SN_FINST_SHIFT	16		/* bit shift */
/* unused:              0x00080000 */

/* WARNING: if new fields are added, they must be initialized in event_accept()
 * and freed in session_free() !
 */

/* Termination sequence tracing.
 *
 * These values have to be set into the field term_trace of a session when
 * closing a session (half or full). They are only meant for post-mortem
 * analysis. The value must be assigned this way :
 *    trace_term(s, TT_XXX);
 *
 * One TT_XXX value is assigned to each location in the code which may be
 * involved in a connection closing. Since a full session close generally
 * involves 4 steps, we will be able to read these steps afterwards by simply
 * checking the code. Value TT_NONE is zero and must never be set, as it means
 * the connection was not closed. Value TT_ANON must be used when no value was
 * assigned to a specific code part. Never ever reuse an already assigned code
 * as it will defeat the purpose of this trace. It is wise to use a per-file
 * anonymous value though.
 */
#define TT_BIT_SHIFT 8
enum {
	TT_NONE     = 0,
	TT_ANON     = 1,
	TT_CLIENT   = 0x10,
	TT_CLIENT_1,
	TT_CLIENT_2,
	TT_HTTP_CLI = 0x20,
	TT_HTTP_CLI_1,
	TT_HTTP_CLI_2,
	TT_HTTP_CLI_3,
	TT_HTTP_CLI_4,
	TT_HTTP_CLI_5,
	TT_HTTP_CLI_6,
	TT_HTTP_CLI_7,
	TT_HTTP_CLI_8,
	TT_HTTP_CLI_9,
	TT_HTTP_CLI_10,
	TT_HTTP_SRV = 0x30,
	TT_HTTP_SRV_1,
	TT_HTTP_SRV_2,
	TT_HTTP_SRV_3,
	TT_HTTP_SRV_4,
	TT_HTTP_SRV_5,
	TT_HTTP_SRV_6,
	TT_HTTP_SRV_7,
	TT_HTTP_SRV_8,
	TT_HTTP_SRV_9,
	TT_HTTP_SRV_10,
	TT_HTTP_SRV_11,
	TT_HTTP_SRV_12,
	TT_HTTP_SRV_13,
	TT_HTTP_SRV_14,
	TT_HTTP_CNT = 0x40,
	TT_HTTP_CNT_1,
	TT_HTTP_URI = 0x50,
	TT_HTTP_URI_1,
};


/*
 * Note: some session flags have dependencies :
 *  - SN_DIRECT cannot exist without SN_ASSIGNED, because a server is
 *    immediately assigned when SN_DIRECT is determined. Both must be cleared
 *    when clearing SN_DIRECT (eg: redispatch).
 *  - ->srv has no meaning without SN_ASSIGNED and must not be checked without
 *    it. ->prev_srv should be used to check previous ->srv. If SN_ASSIGNED is
 *    set and sess->srv is NULL, then it is a dispatch or proxy mode.
 *  - a session being processed has srv_conn set.
 *  - srv_conn might remain after SN_DIRECT has been reset, but the assigned
 *    server should eventually be released.
 */
struct session {
	struct list list;			/* position in global sessions list */
	struct list back_refs;			/* list of users tracking this session */
	struct task *task;			/* the task associated with this session */
	/* application specific below */
	struct listener *listener;		/* the listener by which the request arrived */
	struct proxy *fe;			/* the proxy this session depends on for the client side */
	struct proxy *be;			/* the proxy this session depends on for the server side */
	int conn_retries;			/* number of connect retries left */
	int flags;				/* some flags describing the session */
	unsigned term_trace;			/* term trace: 4*8 bits indicating which part of the code closed */
	struct buffer *req;			/* request buffer */
	struct buffer *rep;			/* response buffer */
	struct stream_interface si[2];          /* client and server stream interfaces */
	struct sockaddr_storage cli_addr;	/* the client address */
	struct sockaddr_storage frt_addr;	/* the frontend address reached by the client if SN_FRT_ADDR_SET is set */
	struct sockaddr_in srv_addr;		/* the address to connect to */
	struct server *srv;			/* the server the session will be running or has been running on */
	struct server *srv_conn;		/* session already has a slot on a server and is not in queue */
	struct server *prev_srv;		/* the server the was running on, after a redispatch, otherwise NULL */
	struct pendconn *pend_pos;		/* if not NULL, points to the position in the pending queue */
	struct http_txn txn;			/* current HTTP transaction being processed. Should become a list. */
	int ana_state;				/* analyser state, used by analysers, always set to zero between them */
	struct {
		int logwait;			/* log fields waiting to be collected : LW_* */
		struct timeval accept_date;	/* date of the accept() in user date */
		struct timeval tv_accept;	/* date of the accept() in internal date (monotonic) */
		struct timeval tv_request;	/* date the request arrives, {0,0} if never occurs */
		long  t_queue;			/* delay before the session gets out of the connect queue, -1 if never occurs */
		long  t_connect;		/* delay before the connect() to the server succeeds, -1 if never occurs */
		long  t_data;			/* delay before the first data byte from the server ... */
		unsigned long t_close;		/* total session duration */
		unsigned long srv_queue_size;	/* number of sessions waiting for a connect slot on this server at accept() time (in direct assignment) */
		unsigned long prx_queue_size;	/* overall number of sessions waiting for a connect slot on this instance at accept() time */
		long long bytes_in;		/* number of bytes transferred from the client to the server */
		long long bytes_out;		/* number of bytes transferred from the server to the client */
	} logs;
	void (*do_log)(struct session *s);	/* the function to call in order to log (or NULL) */
	void (*srv_error)(struct session *s,	/* the function to call upon unrecoverable server errors (or NULL) */
			  struct stream_interface *si);
	short int data_source;			/* where to get the data we generate ourselves */
	short int data_state;			/* where to get the data we generate ourselves */
	union {
		struct {
			struct proxy *px;
			struct server *sv;
			short px_st, sv_st;	/* DATA_ST_INIT or DATA_ST_DATA */
			unsigned int flags;	/* STAT_* */
			int iid, type, sid;	/* proxy id, type and service id if bounding of stats is enabled */
		} stats;
		struct {
			struct bref bref;
		} sess;
		struct {
			int iid;		/* if >= 0, ID of the proxy to filter on */
			struct proxy *px;	/* current proxy being dumped, NULL = not started yet. */
			unsigned int buf;	/* buffer being dumped, 0 = req, 1 = rep */
			unsigned int sid;	/* session ID of error being dumped */
			int ptr;		/* <0: headers, >=0 : text pointer to restart from */
			int bol;		/* pointer to beginning of current line */
		} errors;
	} data_ctx;				/* used by produce_content to dump the stats right now */
	unsigned int uniq_id;			/* unique ID used for the traces */
};


#endif /* _TYPES_SESSION_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
