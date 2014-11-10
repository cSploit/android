/* LICENSE
 * 
 */

#ifndef LOG_H
#define LOG_H

#include <android/log.h>

#include "message.h"

#define LOG_TAG "cSploitClient"

#define LOGS(...) __android_log_print(ANDROID_LOG_SILENT , LOG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL  , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

void android_logger(int level, char *fmt, ...);

#endif