/*
  include/proto/proto_http.h
  This file contains HTTP protocol definitions.

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

#ifndef _PROTO_PROTO_HTTP_H
#define _PROTO_PROTO_HTTP_H

#include <common/config.h>
#include <types/proto_http.h>
#include <types/session.h>
#include <types/task.h>

/*
 * some macros used for the request parsing.
 * from RFC2616:
 *   CTL                 = <any US-ASCII control character (octets 0 - 31) and DEL (127)>
 *   SEP                 = one of the 17 defined separators or SP or HT
 *   LWS                 = CR, LF, SP or HT
 *   SPHT                = SP or HT. Use this macro and not a boolean expression for best speed.
 *   CRLF                = CR or LF. Use this macro and not a boolean expression for best speed.
 *   token               = any CHAR except CTL or SEP. Use this macro and not a boolean expression for best speed.
 *
 * added for ease of use:
 *   ver_token           = 'H', 'P', 'T', '/', '.', and digits.
 */

extern const char http_is_ctl[256];
extern const char http_is_sep[256];
extern const char http_is_lws[256];
extern const char http_is_spht[256];
extern const char http_is_crlf[256];
extern const char http_is_token[256];
extern const char http_is_ver_token[256];

#define HTTP_IS_CTL(x)   (http_is_ctl[(unsigned char)(x)])
#define HTTP_IS_SEP(x)   (http_is_sep[(unsigned char)(x)])
#define HTTP_IS_LWS(x)   (http_is_lws[(unsigned char)(x)])
#define HTTP_IS_SPHT(x)  (http_is_spht[(unsigned char)(x)])
#define HTTP_IS_CRLF(x)  (http_is_crlf[(unsigned char)(x)])
#define HTTP_IS_TOKEN(x) (http_is_token[(unsigned char)(x)])
#define HTTP_IS_VER_TOKEN(x) (http_is_ver_token[(unsigned char)(x)])

int event_accept(int fd);
int process_cli(struct session *t);
int process_srv_data(struct session *t);
int process_srv_conn(struct session *t);
int http_process_request(struct session *t, struct buffer *req);
int http_process_tarpit(struct session *s, struct buffer *req);
int http_process_request_body(struct session *s, struct buffer *req);
int process_response(struct session *t);

void produce_content(struct session *s, struct buffer *rep);
int produce_content_stats(struct session *s);
int produce_content_stats_proxy(struct session *s, struct proxy *px);
void debug_hdr(const char *dir, struct session *t, const char *start, const char *end);
void get_srv_from_appsession(struct session *t, const char *begin, int len);
int apply_filter_to_req_headers(struct session *t, struct buffer *req, struct hdr_exp *exp);
int apply_filter_to_req_line(struct session *t, struct buffer *req, struct hdr_exp *exp);
int apply_filters_to_request(struct session *t, struct buffer *req, struct hdr_exp *exp);
int apply_filters_to_response(struct session *t, struct buffer *rtr, struct hdr_exp *exp);
void manage_client_side_cookies(struct session *t, struct buffer *req);
void manage_server_side_cookies(struct session *t, struct buffer *rtr);
void check_response_for_cacheability(struct session *t, struct buffer *rtr);
int stats_check_uri_auth(struct session *t, struct proxy *backend);
void init_proto_http();
int http_find_header2(const char *name, int len,
		      const char *sol, struct hdr_idx *idx,
		      struct hdr_ctx *ctx);
void http_sess_log(struct session *s);
void perform_http_redirect(struct session *s, struct stream_interface *si);
void http_return_srv_error(struct session *s, struct stream_interface *si);
void http_capture_bad_message(struct error_snapshot *es, struct session *s,
                              struct buffer *buf, struct http_msg *msg,
			      struct proxy *other_end);

#endif /* _PROTO_PROTO_HTTP_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
