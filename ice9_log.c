/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * Header for logger.
 */

#include "ice9_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_BUF_SIZE 512

static char * log_f_name="./ice9_debug_log";

typedef struct _logger_st {
	int output_type;
	int log_level;
	FILE * to_file;
} _logger;

logger create_logger(const int output_type, const int level, char * path) {
	_logger * lger;
	char * file_path=NULL;

	if ( output_type < ICE9_LOG_OUT_STD || output_type > ICE9_LOG_OUT_FILE || level < ICE9_LOG_LEVEL_INFO || level > ICE9_LOG_LEVEL_ERROR) {
		return NULL;
	}

	lger = (_logger*) malloc(sizeof(_logger));
	if (lger == NULL) {
		return NULL;
	}
	lger->output_type = output_type;
	lger->log_level = level;

	if(output_type==ICE9_LOG_OUT_STD)
		return (logger) lger;

	if(path!=NULL){
		file_path = path;
	}else{
		file_path = log_f_name;
	}
	lger->to_file = fopen(file_path, "wb+");
	if (lger->to_file == NULL) {
			free(lger);
			return NULL;
	}

	return (logger) lger;
}

int ice9_log(const logger lger, const char * info) {
	if (lger == NULL) {
		return -1;
	}

	return ice9_log_level(lger, info, ( (_logger*)lger )->log_level );
}

int ice9_log_level(const logger lger, const char * info, const int level) {
	_logger * logger = (_logger*) lger;
	char s[LOG_BUF_SIZE];
	memset(s, 0, sizeof(char) * LOG_BUF_SIZE);

	if (logger == NULL) {
		return -1;
	}

	if (level < ICE9_LOG_LEVEL_INFO || level > ICE9_LOG_LEVEL_ERROR) {
		return -1;
	}


	if (level == ICE9_LOG_LEVEL_INFO)
		strcat(s, "LOG_INFO, ");
	if (level == ICE9_LOG_LEVEL_DEBUG)
		strcat(s, "LOG_DEBUG, ");
	if (level == ICE9_LOG_LEVEL_ERROR)
		strcat(s, "LOG_ERROE, ");

	strcat(s, info);
	strcat(s, "\n");

	if(logger->output_type==ICE9_LOG_OUT_STD){
		fprintf(stdout, "%s", s);
	}else{
		fputs(s, logger->to_file);
		fflush( logger->to_file );
	}

	return 0;
}

int ice9_log_chg_level(const logger lger, const int level){
	_logger * logger = (_logger*) lger;
	if (logger == NULL) {
		return -1;
	}
	if (level < ICE9_LOG_LEVEL_INFO || level > ICE9_LOG_LEVEL_ERROR) {
		return -1;
	}
	logger->log_level = level;
	return 0;
}

void destroy_logger(logger lger) {
	_logger * logger = (_logger*) lger;
	if (lger == NULL)
		return;

	if (logger->to_file != NULL)
		fclose(logger->to_file);

	free(logger);
}

