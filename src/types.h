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

#ifndef TYPES_H_
#define TYPES_H_

#define MAX_SUB_PROTOCOLS 8

enum WS_FRAME_STATE
{
        sw_start = 0,
        sw_got_two,
        sw_got_short_len,
        sw_got_full_len,
        sw_loaded_mask
};

typedef struct _libwebsock_frame
{
        unsigned int fin;
        unsigned int opcode;
        unsigned int mask_offset;
        unsigned int payload_offset;
        unsigned int rawdata_idx;
        unsigned int rawdata_sz;
        unsigned int size;
        unsigned int payload_len_short;
        unsigned int payload_len;
        char *rawdata;
        struct _libwebsock_frame *next_frame;
        struct _libwebsock_frame *prev_frame;
        unsigned char mask[4];
        enum WS_FRAME_STATE state;
} libwebsock_frame;

typedef struct _libwebsock_string
{
        char *data;
        int length;
        int idx;
        int data_sz;
} libwebsock_string;

typedef struct _libwebsock_message
{
        unsigned int opcode;
        unsigned long long payload_len;
        char *payload;
} libwebsock_message;

typedef struct _libwebsock_close_info
{
        unsigned short code;
        char reason[124];
} libwebsock_close_info;

enum libwebsock_loglevel
{
        ERROR = 0,
        INFO = 1,
        DEBUG = 2
};

typedef struct _libwebsock_logger
{
        enum libwebsock_loglevel level;
        const char *filename;
} libwebsock_logger;

typedef struct _libwebsock_client_state
{
        int flags;
        char hostname[64];
        libwebsock_string *out_data;
        libwebsock_frame *current_frame;        
        int (*onmessage)(struct _libwebsock_client_state *, libwebsock_message *);
        int (*oncontrol)(struct _libwebsock_client_state *, libwebsock_frame *);
        int (*onclose)(struct _libwebsock_client_state *);
        int (*onpong)(struct _libwebsock_client_state *);
        int (*onping)(struct _libwebsock_client_state *);
        int (*onerror)(struct _libwebsock_client_state *, unsigned short code);
        libwebsock_close_info *close_info;
        libwebsock_logger logger;
        const char *supported_sub_protocols[MAX_SUB_PROTOCOLS];
        void (*writelog)(libwebsock_logger logger, enum libwebsock_loglevel level, const char *function, const char *fmt, ...);

} libwebsock_client_state;

#endif /* TYPES_H_ */
k_client_state;

#endif /* TYPES_H_ */
