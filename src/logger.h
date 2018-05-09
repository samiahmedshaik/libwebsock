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

#ifndef LOGGER_H_
#define LOGGER_H_

#include "types.h"

#define loginfo(fmt, args...) write_log(state->logger, INFO, __FUNCTION__, fmt, ##args)
#define logdebug(fmt, args...) write_log(state->logger, DEBUG, __FUNCTION__, fmt, ##args)
#define logerror(fmt, args...) write_log(state->logger, ERROR, __FUNCTION__, fmt, ##args)

void write_log(libwebsock_logger logger, enum libwebsock_loglevel level, const char *function, const char *fmt, ...);

#endif /* LOGGER_H_ */