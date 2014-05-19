/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * Header for logger.
 */

//#define ICE9_LOG_DEBUG 1

#ifndef ICE9_LOG_H_
#define ICE9_LOG_H_

#define ICE9_LOG_LEVEL_INFO 1
#define ICE9_LOG_LEVEL_DEBUG 2
#define ICE9_LOG_LEVEL_ERROR 3

#define ICE9_LOG_OUT_STD 1 /* stdout */
#define ICE9_LOG_OUT_FILE 2 /* file */

typedef void * logger;

logger create_logger(const int output_type, const int level, char * path);

int ice9_log(const logger lger, const char * info);
int ice9_log_level(const logger lger, const char * info, const int level);
int ice9_chg_level(const logger lger, const int level);

void destroy_logger(logger lger);


#endif
