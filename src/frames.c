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

int libwebsock_fail_and_cleanup(libwebsock_client_state *state)
{
  state->flags |= STATE_SHOULD_CLOSE;
  return libwebsock_error(state, WS_CLOSE_PROTOCOL_ERROR);
}

int libwebsock_new_continuation_frame(libwebsock_client_state *state)
{
  logdebug("creating continuation frame");
  libwebsock_frame *current = state->current_frame;
  libwebsock_frame *new = (libwebsock_frame *)lws_calloc(sizeof(libwebsock_frame));
  new->rawdata = (char *)lws_malloc(FRAME_CHUNK_LENGTH);
  new->rawdata_sz = FRAME_CHUNK_LENGTH;
  new->prev_frame = current;
  current->next_frame = new;
  state->current_frame = new;
  state->flags |= STATE_RECEIVING_FRAGMENT;
  return 0;
}

void libwebsock_free_all_frames(libwebsock_client_state *state)
{
  logdebug("freeing all frames");
  libwebsock_frame *current, *next;
  if (state != NULL)
  {
    current = state->current_frame;
    if (current)
    {
      for (; current->prev_frame != NULL; current = current->prev_frame)
        ;
      while (current != NULL)
      {
        next = current->next_frame;
        if (current->rawdata)
        {
          lws_free(current->rawdata);
        }

        lws_free(current);
        current = next;
      }
    }
  }
}

int libwebsock_handle_control_frame(libwebsock_client_state *state)
{
  int retval = 0;
  libwebsock_frame *ctl_frame = state->current_frame;
  logdebug("calling control callback method");
  retval = state->oncontrol(state, ctl_frame);
  
  if (state->current_frame != NULL)
  {
    state->current_frame->state = 0;
    state->current_frame->rawdata_idx = 0;
  }
  return retval;
}

void libwebsock_cleanup_frames(libwebsock_client_state *state, libwebsock_frame *first)
{
  logdebug("freeing frames from given entry");
  libwebsock_frame *this = NULL;
  libwebsock_frame *next = first;
  while (next != NULL)
  {
    this = next;
    next = this->next_frame;
    if (this->rawdata != NULL)
    {
      lws_free(this->rawdata);
    }
    lws_free(this);
  }
}
