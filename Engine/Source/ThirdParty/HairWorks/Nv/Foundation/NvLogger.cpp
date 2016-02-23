// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#include "NvLogger.h"

// Needed for var args
#include <stdarg.h>
#include <stdio.h>

namespace Nv {

class IgnoreLogger: public Logger
{
	NV_DECLARE_POLYMORPHIC_CLASS(IgnoreLogger, Logger);
	void log(ELogSeverity severity, const Char* text, const Char* function, const Char* filename, Int lineNumber) NV_OVERRIDE 
	{
		NV_UNUSED(severity)
		NV_UNUSED(text)
		NV_UNUSED(function)
		NV_UNUSED(filename)
		NV_UNUSED(lineNumber)
	}
	void flush() NV_OVERRIDE {}
};

static IgnoreLogger s_ignoreLoggerInstance;

/* static */Logger* Logger::s_instance = NV_NULL;
/* static */Logger* Logger::s_ignoreLogger = &s_ignoreLoggerInstance;

void Logger::logError(const Char* text)
{
	log(LogSeverity::NON_FATAL_ERROR, text, NV_NULL, NV_NULL, 0);
}

void Logger::logErrorWithFormat(const Char* format, ...)
{
	va_list va;
	va_start(va, format);
	char msg[1024];
	vsprintf_s(msg, NV_COUNT_OF(msg), format, va);
	va_end(va);

	log(LogSeverity::NON_FATAL_ERROR, msg, NV_NULL, NV_NULL, 0);
}

/* static */Void Logger::doLogWithFormat(ELogSeverity severity, const Char* funcName, const Char* filename, int lineNumber, const Char* format, ...)
{
	ThisType* instance = getInstance();
	if (instance)
	{
		va_list va;
		va_start(va, format);
		char msg[1024];
		vsprintf_s(msg, NV_COUNT_OF(msg), format, va);
		va_end(va);

		instance->log(severity, msg, funcName, filename, lineNumber);
	}
}

/* static */ Void Logger::doLog(ELogSeverity severity, const Char* msg, const Char* funcName, const Char* filename, int lineNumber)
{
	ThisType* instance = getInstance();
	if (instance)
	{
		instance->log(severity, msg, funcName, filename, lineNumber);
	}
}

/* static */ Void Logger::doLogWithFormat(ELogSeverity severity, const Char* format, ...)
{
	ThisType* instance = getInstance();
	if (instance)
	{
		va_list va;
		va_start(va, format);
		char msg[1024];
		vsprintf_s(msg, NV_COUNT_OF(msg), format, va);
		va_end(va);

		instance->log(severity, msg, NV_NULL, NV_NULL, 0);
	}
}

/* static */ Void Logger::doLog(ELogSeverity severity, const Char* msg)
{
	ThisType* instance = getInstance();
	if (instance)
	{
		instance->log(severity, msg, NV_NULL, NV_NULL, 0);
	}
}

/* static */ const Char* Logger::getText(ELogSeverity severity)
{
	switch (severity)
	{
		case LogSeverity::DEBUG_INFO:	return "DEBUG";
		case LogSeverity::INFO:			return "INFO";
		case LogSeverity::WARNING:		return "WARN";
		case LogSeverity::NON_FATAL_ERROR: return "ERROR";
		case LogSeverity::FATAL_ERROR:	return "FATAL";
		default: break;
	}
	return "?";
}

} // namespace Nv

