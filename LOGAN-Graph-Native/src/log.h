/*
 * log.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __LOG_H
#define __LOG_H

#define LOCK_LOG

#define LOG_CRITICAL 0
#define LOG_ERROR 1
#define LOG_WARNING 2
#define LOG_INFO 3
#define LOG_TRACE 4


extern int logLevel;
#define LOG(L,...) { if(L<=logLevel) _log(L,1,__FILE__, __LINE__, __VA_ARGS__); }
#define LOGN(L,...) { if(L<=logLevel) _log(L,0,__FILE__, __LINE__, __VA_ARGS__); }

void logInit();
void _log(int level, int decorate, char *file, int line, const char *fmt, ...);

#endif
