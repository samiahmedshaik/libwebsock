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
#include <unistd.h>
#include <signal.h>

#include "websock.h"
#include "logger.h"
#include "sha1.h"
#include "base64.h"
#include <arpa/inet.h>

//Define these here to avoid risk of collision if websock.h included in client program
#define AA libwebsock_dispatch_message
#define BB libwebsock_handle_control_frame
#define CC libwebsock_new_continuation_frame
#define DD libwebsock_fail_and_cleanup

#define MAX_SUB_PROTOCOL_LENGTH 1024

static inline int libwebsock_read_header(libwebsock_frame *frame)
{
	int i, new_size;
	enum WS_FRAME_STATE state;

	state = frame->state;
	switch (state)
	{
	case sw_start:
		if (frame->rawdata_idx < 2)
		{
			return 0;
		}
		frame->state = sw_got_two;
	case sw_got_two:
		frame->mask_offset = 2;
		frame->fin = (*(frame->rawdata) & 0x80) == 0x80 ? 1 : 0;
		frame->opcode = *(frame->rawdata) & 0xf;
		frame->payload_len_short = *(frame->rawdata + 1) & 0x7f;
		frame->state = sw_got_short_len;
	case sw_got_short_len:
		switch (frame->payload_len_short)
		{
		case 126:
			if (frame->rawdata_idx < 4)
			{
				return 0;
			}
			frame->mask_offset += 2;
			frame->payload_offset = frame->mask_offset + MASK_LENGTH;
			frame->payload_len = ntohs(
				*((unsigned short int *)(frame->rawdata + 2)));
			frame->state = sw_got_full_len;
			break;
		case 127:
			if (frame->rawdata_idx < 10)
			{
				return 0;
			}
			frame->mask_offset += 8;
			frame->payload_offset = frame->mask_offset + MASK_LENGTH;
			frame->payload_len = ntohl(*((unsigned int *)(frame->rawdata + 6)));
			frame->state = sw_got_full_len;
			break;
		default:
			frame->payload_len = frame->payload_len_short;
			frame->payload_offset = frame->mask_offset + MASK_LENGTH;
			frame->state = sw_got_full_len;
			break;
		}
	case sw_got_full_len:
		if (frame->rawdata_idx < frame->payload_offset)
		{
			return 0;
		}
		for (i = 0; i < MASK_LENGTH; i++)
		{
			frame->mask[i] = *(frame->rawdata + frame->mask_offset + i) & 0xff;
		}
		frame->state = sw_loaded_mask;
		frame->size = frame->payload_offset + frame->payload_len;
		if (frame->size > frame->rawdata_sz)
		{
			new_size = frame->size;
			new_size--;
			new_size |= new_size >> 1;
			new_size |= new_size >> 2;
			new_size |= new_size >> 4;
			new_size |= new_size >> 8;
			new_size |= new_size >> 16;
			new_size++;
			frame->rawdata_sz = new_size;
			frame->rawdata = (char *)lws_realloc(frame->rawdata, new_size);
		}
		return 1;
	case sw_loaded_mask:
		return 1;
	}
	return 0;
}

void libwebsock_populate_close_info_from_frame(libwebsock_close_info **info,
											   libwebsock_frame *close_frame)
{
	unsigned short code_be;
	if (close_frame->payload_len < 2)
	{
		return;
	}

	libwebsock_close_info *new_info = (libwebsock_close_info *)lws_calloc(
		sizeof(libwebsock_close_info));

	memcpy(&code_be, close_frame->rawdata + close_frame->payload_offset, 2);
	int at_most = close_frame->payload_len - 2;
	at_most = at_most > 124 ? 124 : at_most;
	new_info->code = ntohs(code_be);
	if (close_frame->payload_len - 2 > 0)
	{
		memcpy(new_info->reason,
			   close_frame->rawdata + close_frame->payload_offset + 2, at_most);
	}
	*info = new_info;
}

int libwebsock_error(libwebsock_client_state *state,
					 unsigned short close_code)
{
	int retval = 0;

	logdebug("failing connection with close code %d...", close_code);
	state->flags |= STATE_PROCESSING_ERROR;

	if (state->onerror)
	{
		logdebug("calling onerror callback");
		retval = state->onerror(state, close_code);
	}

	libwebsock_free_all_frames(state);
	state->current_frame = NULL;

	return retval;
}

int libwebsock_dispatch_message(libwebsock_client_state *state)
{
	logdebug("dispatching message...");

	unsigned int current_payload_len;
	unsigned long long message_payload_len;
	int message_opcode, i;
	libwebsock_frame *current = state->current_frame;
	char *message_payload, *message_payload_orig, *rawdata_ptr;
	int retval = 0;
	libwebsock_message *msg = NULL;

	state->flags &= ~STATE_RECEIVING_FRAGMENT;
	if (state->flags & STATE_SENT_CLOSE_FRAME)
	{
		logdebug("nothing to do as close frame is already sent");
		return retval;
	}

	libwebsock_frame *first = NULL;
	if (current == NULL)
	{
		logerror("Somehow, null pointer passed to libwebsock_dispatch_message.");
		return -1;
	}

	message_payload_len = 0;
	for (; current->prev_frame != NULL; current = current->prev_frame)
	{
		message_payload_len += current->payload_len;
	}

	message_payload_len += current->payload_len;
	first = current;
	message_opcode = current->opcode;
	message_payload = (char *)lws_malloc(message_payload_len + 1);
	message_payload_orig = message_payload;

	for (; current != NULL; current = current->next_frame)
	{
		current_payload_len = current->payload_len;
		rawdata_ptr = current->rawdata + current->payload_offset;
		for (i = 0; i < current_payload_len; i++)
		{
			*message_payload++ = *rawdata_ptr++ ^ current->mask[i & 3];
		}
	}

	*(message_payload) = '\0';

	if (message_opcode == WS_OPCODE_TEXT)
	{
		if (!validate_utf8_sequence((uint8_t *)message_payload_orig))
		{
			logerror("Error validating UTF-8 sequence.");
			lws_free(message_payload_orig);
			return libwebsock_error(state, WS_CLOSE_WRONG_TYPE);
		}
	}

	libwebsock_cleanup_frames(state, first);
	state->current_frame = NULL;

	msg = (libwebsock_message *)lws_malloc(sizeof(libwebsock_message));
	msg->opcode = message_opcode;
	msg->payload_len = message_payload_len;
	msg->payload = lws_calloc(message_payload_len + 1);
	memcpy(msg->payload, message_payload_orig, message_payload_len);
	msg->payload[message_payload_len] = '\0';

	if (state->onmessage)
	{
		logdebug("calling the onmessage callback");
		retval = state->onmessage(state, msg);
	}

	lws_free(message_payload_orig);
	lws_free(msg->payload);
	lws_free(msg);
	return retval;
}

void libwebsock_cleanup_outdata(libwebsock_client_state *state)
{
	if (state->out_data)
	{
		if (state->out_data->data)
		{
			lws_free((void *)state->out_data->data);
			state->out_data->data = NULL;
		}
		state->out_data->data_sz = 0;
		lws_free((void *)state->out_data);
		state->out_data = NULL;
	}
}

int libwebsock_make_fragment(libwebsock_client_state *state, const char *data,
							 unsigned int len, int flags)
{

	unsigned int *payload_len_32_be;
	unsigned short int *payload_len_short_be;
	unsigned char finNopcode, payload_len_small;
	unsigned int payload_offset = 2;
	unsigned int frame_size, current_size = 0;

	logdebug("called with len %u, flags are as follows:", len);

	if ((state->flags & STATE_SENT_CLOSE_FRAME) != 0)
	{
		logdebug("|STATE_SENT_CLOSE_FRAME");
	}
	if ((state->flags & STATE_CONNECTING) != 0)
	{
		logdebug("|STATE_CONNECTING");
	}
	if ((state->flags & STATE_CONNECTED) != 0)
	{
		logdebug("|STATE_CONNECTED");
	}
	if ((state->flags & STATE_NEEDS_MORE_DATA) != 0)
	{
		logdebug("|STATE_NEEDS_MORE_DATA");
	}
	if ((state->flags & STATE_RECEIVING_FRAGMENT) != 0)
	{
		logdebug("|STATE_RECEIVING_FRAGMENT");
	}
	if ((state->flags & STATE_RECEIVED_CLOSE_FRAME) != 0)
	{
		logdebug("|STATE_RECEIVED_CLOSE_FRAME");
	}
	if ((state->flags & STATE_PROCESSING_ERROR) != 0)
	{
		logdebug("|STATE_PROCESSING_ERROR");
	}

	if ((state->flags & STATE_SENT_CLOSE_FRAME) != 0 && (state->flags & STATE_RECEIVED_CLOSE_FRAME) != 0)
	{
		logerror("failed to make the fragment as the close frame has been sent/received");
		return -1;
	}

	if ((state->flags & STATE_CONNECTED) == 0)
	{
		logerror("failed to make the fragment as the client state is not connected");
		return -1;
	}

	finNopcode = flags & 0xff;
	if (len <= 125)
	{
		frame_size = 2 + len;
		payload_len_small = len & 0xff;
	}
	else if (len > 125 && len <= 0xffff)
	{
		frame_size = 4 + len;
		payload_len_small = 126;
		payload_offset += 2;
	}
	else if (len > 0xffff && len <= 0xfffffff0)
	{
		frame_size = 10 + len;
		payload_len_small = 127;
		payload_offset += 8;
	}
	else
	{
		logerror(
			"libwebsock does not support frame payload sizes over %u bytes long",
			0xfffffff0);
		return -1;
	}

	if (!state->out_data)
	{
		state->out_data = (libwebsock_string *)lws_calloc(sizeof(libwebsock_string));
		state->out_data->data_sz = frame_size;
		state->out_data->data = (char *)lws_calloc(frame_size);
	}
	else
	{
		current_size = state->out_data->data_sz;
		state->out_data->data = lws_realloc(state->out_data->data, current_size + frame_size);
		state->out_data->data_sz = current_size + frame_size;
	}

	char *frame = (char *)state->out_data->data + current_size;
	payload_len_small &= 0x7f;
	*frame = finNopcode;
	*(frame + 1) = payload_len_small;
	if (payload_len_small == 126)
	{
		len &= 0xffff;
		payload_len_short_be = (unsigned short *)((char *)frame + 2);
		*payload_len_short_be = htons(len);
	}
	if (payload_len_small == 127)
	{
		payload_len_32_be = (unsigned int *)((char *)frame + 2);
		*payload_len_32_be++ = 0;
		*payload_len_32_be = htonl(len);
	}
	memcpy(frame + payload_offset, data, len);
	return frame_size;
}

int libwebsock_handle_recv(libwebsock_client_state *state, const char *data, size_t len)
{
	logdebug("received data of size %u", len);

	libwebsock_frame *current = NULL;
	int i, err, in_fragment;

	if (len == 0)
	{
		return -1;
	}

	int (*frame_fn)(libwebsock_client_state * state);
	static int (*const libwebsock_frame_lookup_table[512])(
		libwebsock_client_state * state) = {
		DD, CC, CC, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //00..0f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //10..1f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //20..2f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //30..3f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //40..4f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //50..5f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //60..6f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //70..7f
		DD, AA, AA, DD, DD, DD, DD, DD, BB, BB, BB, DD, DD, DD, DD, DD, //80..8f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //90..9f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //a0..af
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //b0..bf
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //c0..cf
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //d0..df
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //e0..ef
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //f0..ff
		CC, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //100..10f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //110..11f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //120..12f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //130..13f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //140..14f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //150..15f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //160..16f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //170..17f
		AA, DD, DD, DD, DD, DD, DD, DD, BB, BB, BB, DD, DD, DD, DD, DD, //180..18f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //190..19f
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //1a0..1af
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //1b0..1bf
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //1c0..1cf
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, //1d0..1df
		DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD, DD  //1f0..1ff
	};

	int retval = -1;
	const char *buf = data;
	for (i = 0; i < len;)
	{
		if (state->flags & STATE_PROCESSING_ERROR)
		{
			return 0;
		}

		// reset the needs more data flag
		if (state->flags & STATE_NEEDS_MORE_DATA)
		{
			state->flags &= ~STATE_NEEDS_MORE_DATA;
		}

		current = state->current_frame;
		if (current == NULL)
		{
			current = (libwebsock_frame *)lws_calloc(sizeof(libwebsock_frame));
			current->payload_len = -1;
			current->rawdata_sz = FRAME_CHUNK_LENGTH;
			current->rawdata = (char *)lws_calloc(FRAME_CHUNK_LENGTH);
			state->current_frame = current;
		}

		*(current->rawdata + current->rawdata_idx++) = *buf++;
		i++;

		if (current->state != sw_loaded_mask)
		{
			err = libwebsock_read_header(current);
			if (err == 0)
			{
				continue;
			}
		}

		if (current->rawdata_idx < current->size)
		{
			if (len - i >= current->size - current->rawdata_idx)
			{ //remaining in current vector completes frame.  Copy remaining frame size
				memcpy(current->rawdata + current->rawdata_idx, buf,
					   current->size - current->rawdata_idx);
				buf += current->size - current->rawdata_idx;
				i += current->size - current->rawdata_idx;
				current->rawdata_idx = current->size;
			}
			else
			{ //not complete frame, copy the rest of this vector into frame.
				memcpy(current->rawdata + current->rawdata_idx, buf, len - i);
				current->rawdata_idx += len - i;
				i = len;
				state->flags |= STATE_NEEDS_MORE_DATA;
				continue;
			}
		}

		in_fragment = (state->flags & STATE_RECEIVING_FRAGMENT) ? 256 : 0;
		frame_fn = libwebsock_frame_lookup_table[in_fragment | (*current->rawdata & 0xff)];

		retval = frame_fn(state);
		if (retval == -1)
		{
			break;
		}
	}

	// didn't get the full length
	if (state->current_frame != NULL && state->current_frame->state <= sw_got_full_len)
	{
		state->flags |= STATE_NEEDS_MORE_DATA;
	}

	return retval;
}

static const char *get_selected_subprotocol(libwebsock_client_state *state, char *client_requested_subprotocols)
{
	char *tok = NULL;
	int i;

	if (client_requested_subprotocols == NULL || *client_requested_subprotocols == '\0')
	{
		return NULL;
	}

	for (tok = strtok(client_requested_subprotocols, ","); tok != NULL; tok = strtok(NULL, ","))
	{
		for (i = 0; i < MAX_SUB_PROTOCOLS && state->supported_sub_protocols[i] != NULL; i++)
		{
			if (strcmp(tok, state->supported_sub_protocols[i]) == 0)
			{
				logdebug("selected protocol is %s", tok);
				return tok;
			}
		}
	}

	return NULL;
}

int libwebsock_populate_handshake(libwebsock_client_state *state, const char *data, size_t len)
{
	if (strstr(data, "\r\n\r\n") == NULL && strstr(data, "\n\n") == NULL)
	{
		logerror("Unable to find any request headers.");
		return -1;
	}

	char buf[2048];
	char sha1buf[45];
	char concat[1024];
	unsigned char sha1mac[20];
	char *tok = NULL, *headers = NULL, *key = NULL;
	char *base64buf = NULL;
	const char *GID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	SHA1Context shactx;
	SHA1Reset(&shactx);
	int n = 0;
	int len_subprotocol = 0;
	char client_sub_protocols[MAX_SUB_PROTOCOL_LENGTH] = {'\0'};

	headers = (char *)lws_calloc(len + 1);
	strncpy(headers, data, len);

	for (tok = strtok(headers, "\r\n"); tok != NULL; tok = strtok(NULL, "\r\n"))
	{
		if (strstr(tok, "Sec-WebSocket-Key: ") != NULL)
		{
			key = (char *)lws_malloc(strlen(tok));
			strncpy(key, tok + strlen("Sec-WebSocket-Key: "), strlen(tok));
			continue;
		}

		// There could be multiples of this header from client upgrade request
		if (strstr(tok, "Sec-WebSocket-Protocol: ") != NULL)
		{
			size_t header_len = strlen("Sec-WebSocket-Protocol: ");
			size_t token_len = strlen(tok);

			if (len_subprotocol + token_len - header_len > MAX_SUB_PROTOCOL_LENGTH - 1)
			{
				continue;
			}

			if (len_subprotocol > 0)
			{
				client_sub_protocols[len_subprotocol++] = ',';
			}

			strncpy(client_sub_protocols + len_subprotocol, tok + header_len, token_len - header_len);
			len_subprotocol += token_len - header_len;
			continue;
		}
	}
	lws_free(headers);

	if (key == NULL)
	{
		logerror("Unable to find key in request headers.");
		return -1;
	}

	memset(concat, 0, sizeof(concat));
	strncat(concat, key, strlen(key));
	strncat(concat, GID, strlen(GID));
	SHA1Input(&shactx, (unsigned char *)concat, strlen(concat));
	SHA1Result(&shactx);
	lws_free(key);
	key = NULL;
	sprintf(sha1buf, "%08x%08x%08x%08x%08x", shactx.Message_Digest[0],
			shactx.Message_Digest[1], shactx.Message_Digest[2],
			shactx.Message_Digest[3], shactx.Message_Digest[4]);
	for (n = 0; n < (strlen(sha1buf) / 2); n++)
	{
		sscanf(sha1buf + (n * 2), "%02hhx", sha1mac + n);
	}
	base64buf = (char *)lws_malloc(256);
	base64_encode(sha1mac, 20, base64buf, 256);
	memset(buf, 0, 1024);

	int buflen = snprintf(buf, 1024, "HTTP/1.1 101 Switching Protocols\r\n"
									 "Server: %s\r\n"
									 "Upgrade: websocket\r\n"
									 "Connection: Upgrade\r\n"
									 "Sec-WebSocket-Accept: %s\r\n",
						  state->hostname,
						  base64buf);

	const char *selected_protocol = get_selected_subprotocol(state, client_sub_protocols);
	if (selected_protocol != NULL)
	{
		snprintf(buf + buflen, 1024, "Sec-WebSocket-Protocol: %s\r\n\r\n", selected_protocol);
	}
	else
	{
		snprintf(buf + buflen, 1024, "\r\n");
	}

	lws_free(base64buf);

	libwebsock_string *str = state->out_data;
	if (!str)
	{
		state->out_data = (libwebsock_string *)lws_calloc(sizeof(libwebsock_string));
		str = state->out_data;
		str->data_sz = strlen(buf);
		str->data = (char *)lws_calloc(str->data_sz);
	}

	memcpy(str->data + str->idx, buf, str->data_sz);
	state->flags = STATE_CONNECTED;
	return 0;
}
