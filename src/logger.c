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
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "logger.h"

const char *strloglevel[3] = {"ERROR", "INFO", "DEBUG"};
#define LOG_BUF_SIZE (2 * 1024)

void write_log(libwebsock_logger logger, enum libwebsock_loglevel level, const char *function, const char *fmt, ...)
{
	if (level > logger.level)
		return;

	char cbuffer[LOG_BUF_SIZE + 1];

	va_list argList;
	va_start(argList, fmt);
	vsnprintf(cbuffer, LOG_BUF_SIZE, fmt, argList);
	va_end(argList);

	char strTime[32];
	struct tm localtimeBuffer;
	struct timeval cur;

	gettimeofday(&cur, NULL);
	localtime_r(&cur.tv_sec, &localtimeBuffer);
	strftime(strTime, sizeof(strTime), "%F %T", &localtimeBuffer);

	if (logger.filename == NULL || logger.filename[0] == '\0')
	{
		fprintf(stderr, "%s %6d %-5s %s: %s\n",
				strTime,
				getpid(),
				strloglevel[level],
				function,
				cbuffer);
	}
	else
	{

		//don't care performance, safety first
		FILE *file = fopen(logger.filename, "a");
		if (file != 0)
		{
			fprintf(file, "%s %6d %-5s %s: %s\n",
					strTime,
					getpid(),
					strloglevel[level],
					function,
					cbuffer);

			if (file != NULL)
			{
				fclose(file);
				file = NULL;
			}
		}
		else
		{
			fprintf(stderr, "%s %6d %-5s %s: %s\n",
					strTime,
					getpid(),
					strloglevel[level],
					function,
					cbuffer);
		}
	}
}