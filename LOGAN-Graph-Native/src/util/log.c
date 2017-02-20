/*
 * log.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */


#include "common.h"

int logLevel = LOG_TRACE;

#ifdef LOCK_LOG
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static int logInitFlag=0;
static struct timeval startTv;

void logInit()
{
	if(!logInitFlag)
		{
		gettimeofday(&startTv, NULL);
		logInitFlag=1;
		}
}


void _log(int level, int format, char *file, int line, const char *fmt, ...)
{
	va_list arglist;

#ifdef LOCK_LOG
	pthread_mutex_lock(&logMutex);
#endif

	if (format>0)
		{
		struct timeval nowTv;
		gettimeofday(&nowTv, NULL);

		double timestamp=((double)(nowTv.tv_usec - startTv.tv_usec))/1000000 + (double)(nowTv.tv_sec-startTv.tv_sec);

		switch (level)
			{
			case LOG_CRITICAL:
				fprintf(stderr, "LOGAN CRITICAL %s(%i) %f: ", file, line, timestamp);
				break;
			case LOG_ERROR:
				fprintf(stderr, "LOGAN ERROR %s(%i) %f: ", file, line, timestamp);
				break;
			case LOG_WARNING:
				fprintf(stderr, "LOGAN WARNING %s(%i) %f: ", file, line, timestamp);
				break;
			case LOG_INFO:
				fprintf(stderr, "LOGAN INFO %s(%i) %f: ", file, line, timestamp);
				break;
			case LOG_TRACE:
				fprintf(stderr, "LOGAN TRACE %s(%i) %f: ", file, line, timestamp);
				break;
			default:
				fprintf(stderr, "LOGAN Level %i %s(%i) %f: ", level, file, line, timestamp);
				break;
			}
		}

	va_start(arglist,fmt);
	vfprintf(stderr, fmt, arglist);
	va_end(arglist);

	if(format>=0)
		fprintf(stderr, "\n");

	//fflush(stderr);

#ifdef LOCK_LOG
	pthread_mutex_unlock(&logMutex);
#endif

	if(level==LOG_CRITICAL)
		raise(SIGSEGV);
}

