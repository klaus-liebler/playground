#pragma once
#ifdef ESP_PLATFORM
#include <esp_log.h>
#define LOGI( tag, format, ... ) ESP_LOGI( tag, format, ... )
#define LOGW( tag, format, ... ) ESP_LOGW( tag, format, ... )
#define LOGE( tag, format, ... ) ESP_LOGE( tag, format, ... )
#else
#include <cstdio>
#define LOGI( tag, format, ... ) printf(format "\n",  ##__VA_ARGS__)
#define LOGW( tag, format, ... ) printf(format "\n",  ##__VA_ARGS__)
#define LOGE( tag, format, ... ) printf(format "\n",  ##__VA_ARGS__)
#endif

class iLog{
public:
virtual bool IsDebugEnabled()=0;
virtual bool IsErrorEnabled()=0;
virtual bool IsFatalEnabled()=0;
virtual bool IsInfoEnabled()=0;
virtual bool IsWarnEnabled()=0;

virtual void Debug(const char* format, ...)=0;
virtual void Error(const char* format, ...)=0;
virtual void Fatal(const char* format, ...)=0;
virtual void Info(const char* format, ...)=0;
virtual void Warn(const char* format, ...)=0;
};