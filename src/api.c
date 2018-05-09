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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#ifndef _WIN32
#include <netdb.h>
#endif

#include "websock.h"
#include "logger.h"

const char *
libwebsock_version_string(void)
{
  return WEBSOCK_PACKAGE_STRING;
}

void libwebsock_dump_frame(libwebsock_client_state *state, libwebsock_frame *frame)
{
  int i;
  logdebug("FIN: %d", frame->fin);
  logdebug("Opcode: %d", frame->opcode);
  logdebug("mask_offset: %d", frame->mask_offset);
  logdebug("payload_offset: %d", frame->payload_offset);
  logdebug("rawdata_idx: %d", frame->rawdata_idx);
  logdebug("rawdata_sz: %d", frame->rawdata_sz);
  logdebug("payload_len: %u", frame->payload_len);
  logdebug("Has previous frame: %d", frame->prev_frame != NULL ? 1 : 0);
  logdebug("Has next frame: %d", frame->next_frame != NULL ? 1 : 0);
  logdebug("Raw data:");
  logdebug("%02x", *(frame->rawdata) & 0xff);
  for (i = 1; i < frame->rawdata_idx; i++)
  {
    logdebug(":%02x", *(frame->rawdata + i) & 0xff);
  }
}

int libwebsock_make_ping_frame(libwebsock_client_state *state)
{
  logdebug("ping frame");
  return libwebsock_make_fragment(state, NULL, 0, 0x89);
}

int libwebsock_make_pong_frame(libwebsock_client_state *state, const char *data, unsigned int len)
{
  logdebug("pong frame");
  return libwebsock_make_fragment(state, data, len, 0x8a);
}

int libwebsock_make_close_frame(libwebsock_client_state *state)
{
  return libwebsock_make_close_frame_with_reason(state, WS_CLOSE_NORMAL, NULL);
}

int libwebsock_make_close_frame_with_reason(libwebsock_client_state *state, unsigned short code, const char *reason)
{
  unsigned int len = 2;
  unsigned short code_be = htobe16(code);
  char buf[128] = {'\0'};
  memcpy(buf, &code_be, 2);

  if (reason)
  {
    len += snprintf(buf + 2, 124, "%s", reason);
    logdebug("close frame with code %u and reason %s", code, reason);
  }
  else
  {
    logdebug("close frame with code %u", code);
  }

  int flags = WS_FRAGMENT_FIN | WS_OPCODE_CLOSE;
  int ret = libwebsock_make_fragment(state, buf, len, flags);

  state->flags &= ~STATE_PROCESSING_ERROR;
  state->flags |= STATE_SENT_CLOSE_FRAME;
  return ret;
}

int libwebsock_make_text_data_frame_with_length(libwebsock_client_state *state, char *strdata, unsigned int payload_len)
{
  logdebug("text data frame for payload of size %u", payload_len);
  int flags = WS_FRAGMENT_FIN | WS_OPCODE_TEXT;
  return libwebsock_make_fragment(state, strdata, payload_len, flags);
}

int libwebsock_make_text_data_frame(libwebsock_client_state *state, char *strdata)
{
  unsigned int len = strlen(strdata);
  int flags = WS_FRAGMENT_FIN | WS_OPCODE_TEXT;
  return libwebsock_make_fragment(state, strdata, len, flags);
}

int libwebsock_make_binary_data_frame(libwebsock_client_state *state, char *in_data, unsigned int payload_len)
{
  logdebug("binary data frame for payload of size %u", payload_len);
  int flags = WS_FRAGMENT_FIN | WS_OPCODE_BINARY;
  return libwebsock_make_fragment(state, in_data, payload_len, flags);
}

libwebsock_client_state *libwebsock_client_init(void)
{
  libwebsock_client_state *state = (libwebsock_client_state *)lws_calloc(sizeof(libwebsock_client_state));
  state->onopen = libwebsock_default_onopen_callback;
  state->oncontrol = libwebsock_default_control_callback;
  state->onmessage = libwebsock_default_onmessage_callback;
  state->onping = libwebsock_default_onping_callback;
  state->onclose = libwebsock_default_onclose_callback;
  state->onerror = libwebsock_default_onerror_callback;
  state->onpong = NULL;
  state->flags |= STATE_CONNECTING;

  loginfo("websocket client initialized");
  return state;
}

void libwebsock_client_destroy(libwebsock_client_state *state)
{
  if (state)
  {
    if (state->close_info)
    {
      lws_free(state->close_info);
      state->close_info = NULL;
    }
    libwebsock_cleanup_outdata(state);
    libwebsock_cleanup_indata(state);
    libwebsock_free_all_frames(state);
    lws_free(state);
  }

  loginfo("websocket client destroyed");
}