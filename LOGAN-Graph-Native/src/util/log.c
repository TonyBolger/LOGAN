/*
 * log.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include "../common.h"

int logLevel = LOG_TRACE;

static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

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


void _log(int level, int decorate, char *file, int line, const char *fmt, ...)
{
	va_list arglist;

#ifdef LOCK_LOG
	pthread_mutex_lock(&logMutex);
#endif

	struct timeval nowTv;
	gettimeofday(&nowTv, NULL);

	double timestamp=((double)(nowTv.tv_usec - startTv.tv_usec))/1000000 + (double)(nowTv.tv_sec-startTv.tv_sec);

	if (decorate)
	{
		switch (level)
		{
		case LOG_CRITICAL:
			fprintf(stdout, "LOGAN CRITICAL %s(%i) %f: ", file, line, timestamp);
			break;
		case LOG_ERROR:
			fprintf(stdout, "LOGAN ERROR %s(%i) %f: ", file, line, timestamp);
			break;
		case LOG_WARNING:
			fprintf(stdout, "LOGAN WARNING %s(%i) %f: ", file, line, timestamp);
			break;
		case LOG_INFO:
			fprintf(stdout, "LOGAN INFO %s(%i) %f: ", file, line, timestamp);
			break;
		case LOG_TRACE:
			fprintf(stdout, "LOGAN TRACE %s(%i) %f: ", file, line, timestamp);
			break;
		default:
			fprintf(stdout, "LOGAN Level %i %s(%i) %f: ", level, file, line, timestamp);
			break;
		}
	}

	va_start(arglist,fmt);
	vfprintf(stdout, fmt, arglist);
	va_end(arglist);

	fprintf(stdout, "\n");
	//fflush(stderr);

#ifdef LOCK_LOG
	pthread_mutex_unlock(&logMutex);
#endif
}

