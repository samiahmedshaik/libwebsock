/*
 * This file is part of libwebsock
 *
 * Copyright (C) 2012-2013 Payden Sutherland
 *
 * libwebsock is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * libwebsock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libwebsock; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef API_H_
#define API_H_

#include "types.h"

const char *libwebsock_version_string(void);
int libwebsock_make_ping_frame(libwebsock_client_state *state);
int libwebsock_make_pong_frame(libwebsock_client_state *state, const char *data, unsigned int len);
int libwebsock_make_close_frame(libwebsock_client_state *state);
int libwebsock_make_close_frame_with_reason(libwebsock_client_state *state, unsigned short code, const char *reason);
int libwebsock_make_binary_data_frame(libwebsock_client_state *state, char *in_data, unsigned int payload_len);
int libwebsock_make_text_data_frame(libwebsock_client_state *state, char *strdata);
int libwebsock_make_text_data_frame_with_length(libwebsock_client_state *state, char *strdata, unsigned int payload_len);
int libwebsock_make_init_text_continuation_frame_with_length(libwebsock_client_state *state, char *strdata, unsigned int payload_len);
int libwebsock_make_init_binary_continuation_frame_with_length(libwebsock_client_state *state, char *in_data, unsigned int payload_len);
int libwebsock_make_end_text_continuation_frame_with_length(libwebsock_client_state *state, char *strdata, unsigned int payload_len);
int libwebsock_make_end_binary_continuation_frame_with_length(libwebsock_client_state *state, char *in_data, unsigned int payload_len);
int libwebsock_make_text_continuation_frame_with_length(libwebsock_client_state *state, char *strdata, unsigned int payload_len);
int libwebsock_make_binary_continuation_frame_with_length(libwebsock_client_state *state, char *in_data, unsigned int payload_len);
libwebsock_client_state *libwebsock_client_init(void);
void libwebsock_client_destroy(libwebsock_client_state *state);

#endif /* API_H_ */
