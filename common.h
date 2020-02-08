#pragma once


#ifdef DEBUG
//--------------------------------------------------------------- 
inline void trace(const char* fmt, ...)//format and send string to serial port
{
	const size_t s_size = 160;
	char s[s_size];
	va_list argList;
	va_start(argList, fmt);
	vsnprintf(s, s_size, fmt, argList);
	va_end(argList);
	Serial.print(s);
}

//--------------------------------------------------------------- debug macros
#define toS(x) String(x).c_str()


//--------------------------------------------------------------- debug macros
#define TRACE(...) trace(__VA_ARGS__)
//#define TRACE_PROC(...) trace(__VA_ARGS__)
//#define TRACE_DISTANCE_ANALYZE(...) trace(__VA_ARGS__)
//#define TRACE_DISTANCE_ONLY(...) trace(__VA_ARGS__)
//#define TRACE_HT(...) trace(__VA_ARGS__)
#define TRACE_HT_LOG(...) trace(__VA_ARGS__)
//#define TRACE_TIME(...) trace(__VA_ARGS__)
//#define TRACE_LOOP(...) trace(__VA_ARGS__)
//#define TRACE_ERROR(...) trace(__VA_ARGS__)
//#define TRACE_SIZES(...) trace(__VA_ARGS__)

//#define ASSERT(condition,...) verify(condition, __VA_ARGS__)
//#define VERIFY(condition, ...) verify(condition, __VA_ARGS__)
#else
//#define ASSERT(...) void(0)
#define TRACE(...) void(0)
inline void trace(...) {};
//#define VERIFY(condition, ...) (condition)
#endif

#ifndef TRACE_PROC
#define TRACE_PROC(...) void(0)
#endif

#ifndef TRACE_DISTANCE_ANALYZE
#define TRACE_DISTANCE_ANALYZE(...) void(0)
#endif

#ifndef TRACE_DISTANCE_ONLY
#define TRACE_DISTANCE_ONLY(...) void(0)
#endif

#ifndef TRACE_HT
#define TRACE_HT(...) void(0)
#endif

#ifndef TRACE_HT_LOG
#define TRACE_HT_LOG(...) void(0)
#endif


#ifndef  TRACE_TIME
#define TRACE_TIME(...) void(0)
#endif

#ifndef  TRACE_LOOP
#define TRACE_LOOP(...) void(0)
#endif

#ifndef TRACE_ERROR
#define TRACE_ERROR(...) void(0)
#endif

#ifndef TRACE_SIZES
#define TRACE_SIZES(...) void(0)
#endif


//---------------------------------------------------------------

//---------------------------------------------------------------
