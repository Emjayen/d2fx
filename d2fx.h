#pragma once



void Log(const char* pFormat, ...);


#define LOG_ON

#ifdef LOG_ON
#define LOG(format, ...) Log(format "\r\n", __VA_ARGS__)
#else
#define LOG(format, ...)
#endif