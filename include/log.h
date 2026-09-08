#pragma once

#if defined(__ANDROID__)
#include <android/log.h>
#define TAG "ViPER"
#if defined(LOG_NDEBUG) && LOG_NDEBUG == 0
#define VIPER_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#else
#define VIPER_LOGD(...) ((void) 0)
#endif
#define VIPER_LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define VIPER_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#include <cstdio>
#define VIPER_LOGD(fmt, ...) fprintf(stderr, "[ViPER][DEBUG] " fmt "\n", ##__VA_ARGS__)
#define VIPER_LOGI(fmt, ...) fprintf(stderr, "[ViPER][INFO] " fmt "\n", ##__VA_ARGS__)
#define VIPER_LOGE(fmt, ...) fprintf(stderr, "[ViPER][ERROR] " fmt "\n", ##__VA_ARGS__)
#endif
