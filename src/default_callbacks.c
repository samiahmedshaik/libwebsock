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
#include "websock.h"
#include "logger.h"

int libwebsock_default_onclose_callback(libwebsock_client_state *state)
{
  if (state->close_info)
  {
    return libwebsock_make_close_frame_with_reason(state, state->close_info->code, state->close_info->reason);
  }
  else
  {
    return libwebsock_make_close_frame(state);
  }
}

int libwebsock_default_onmessage_callback(libwebsock_client_state *state, libwebsock_message *msg)
{
  return 0;
}

int libwebsock_default_onping_callback(libwebsock_client_state *state)
{
  if (!state->current_frame)
  {
    logerror("current_frame in the state is NULL");
    return -1;
  }

  libwebsock_frame *frame = state->current_frame;
  return libwebsock_make_pong_frame(state, frame->rawdata + frame->payload_offset, frame->payload_len);
}

int libwebsock_default_onpong_callback(libwebsock_client_state *state)
{
  return 0;
}

int libwebsock_default_onerror_callback(libwebsock_client_state *state, unsigned short code)
{
  return libwebsock_make_close_frame_with_reason(state, code, NULL);
}

int libwebsock_default_control_callback(libwebsock_client_state *state, libwebsock_frame *ctl_frame)
{
  int i;
  int retval = 0;

  /*
   * if the close frame has been sent then no need to process any more data 
   */

  if ((state->flags & STATE_SENT_CLOSE_FRAME) && (ctl_frame->opcode != WS_OPCODE_CLOSE))
  {
    logdebug(" data received after sending close frame. Ignoring the data");
    return retval;
  }

  /*
   * control frame cannot be greater than 125 bytes 
   */

  if (ctl_frame->payload_len > 125)
  {
    logerror("control frame payload greater than 125 bytes - %u", ctl_frame->payload_len);
    return libwebsock_error(state, WS_CLOSE_PROTOCOL_ERROR);
  }

  for (i = 0; i < ctl_frame->payload_len; i++)
  {
    //this demasks the payload
    *(ctl_frame->rawdata + ctl_frame->payload_offset + i) =
        *(ctl_frame->rawdata + ctl_frame->payload_offset + i) ^ (ctl_frame->mask[i % 4] & 0xff);
  }

  switch (ctl_frame->opcode)
  {
  case WS_OPCODE_CLOSE: //close frame

    logdebug("received close frame");

    if (ctl_frame->payload_len == 1)
    {
      logerror("payload len is 1 byte");
      return libwebsock_fail_and_cleanup(state);
    }

    if (!state->close_info && ctl_frame->payload_len >= 2)
    {
      logdebug("populating close info");
      libwebsock_populate_close_info_from_frame(&state->close_info, ctl_frame);
    }

    if (state->close_info)
    {
      unsigned short code = state->close_info->code;

      if ((code >= 0 && code < WS_CLOSE_NORMAL) || code == WS_CLOSE_RESERVED ||
          code == WS_CLOSE_NO_CODE || code == WS_CLOSE_DIRTY || (code > 1011 && code < 3000))
      {
        logerror("Invalid close code %u in the payload", code);
        return libwebsock_error(state, WS_CLOSE_PROTOCOL_ERROR);
      }
      else if (!validate_utf8_sequence((uint8_t *)state->close_info->reason))
      {
        logerror("payload is not valid UTF-8 sequence", code);
        return libwebsock_fail_and_cleanup(state);
      }
    }

    if ((state->flags & STATE_SENT_CLOSE_FRAME) == 0)
    {
      //client request close.  Echo close frame as acknowledgement
      state->flags |= STATE_RECEIVED_CLOSE_FRAME;
      if (state->onclose)
      {
        logdebug("calling onclose callback...");
        retval = state->onclose(state);
      }
    }
    else
    {
      if ((state->flags & STATE_RECEIVED_CLOSE_FRAME) == 0)
      {
        logdebug("received acknowledgement to the sent close frame");
        return -1;
      }
      else
      {
        //received second close frame?
        logdebug("received second close frame");
        return -1;
      }
    }
    break;

  case WS_OPCODE_PING:

    logdebug("received ping frame");
    if (state->onping)
    {
      retval = state->onping(state);
    }

    break;
  case WS_OPCODE_PONG:

    logdebug("received pong frame");
    if (state->onpong)
    {
      retval = state->onpong(state);
    }
    break;

  default:
    logerror("received unknown control frame");
    retval = libwebsock_error(state, WS_CLOSE_PROTOCOL_ERROR);
    break;
  }

  return retval;
}
