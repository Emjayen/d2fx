/*
 * d2fx.h
 *
 */
#pragma once
#include "pce/pce.h"
#include "dbg.h"



void Log(const char* pFormat, ...);


//#define LOG_ON

#ifdef LOG_ON
#define LOG(format, ...) Log(format "\r\n", __VA_ARGS__)
#else
#define LOG(format, ...)
#endif