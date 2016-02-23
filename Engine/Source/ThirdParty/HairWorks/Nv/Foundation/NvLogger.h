// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef NV_LOGGER_H
#define NV_LOGGER_H

#include <Nv/Foundation/NvCommon.h>
#include <Nv/Foundation/NvTypeMacros.h>

namespace Nv {

/*! The log severities available */
class LogSeverity { LogSeverity(); public: enum Enum
{
	DEBUG_INFO,			///< Debugging info - only available on debug builds.
	INFO,				///< Informational - noting is wrong.
	WARNING,			///< Warning - something may be 100% correct or optimal. Something might need fixing.
	NON_FATAL_ERROR,			///< Something is wrong and needs fixing, but execution can continue.
	FATAL_ERROR,				///< Something is seriously wrong, execution cannot continue.
	COUNT_OF, 
}; };
typedef LogSeverity::Enum ELogSeverity;

/*! Flags that can be used in Logger implementation to filter results */
class LogSeverityFlag { LogSeverityFlag(); public: enum Enum
{
	DEBUG_INFO		= 1 << Int(LogSeverity::DEBUG_INFO),			
	INFO			= 1 << Int(LogSeverity::INFO),				
	WARNING			= 1 << Int(LogSeverity::WARNING),			
	NON_FATAL_ERROR	= 1 << Int(LogSeverity::NON_FATAL_ERROR),	
	FATAL_ERROR		= 1 << Int(LogSeverity::FATAL_ERROR),				
}; };
typedef LogSeverityFlag::Enum ELogSeverityFlag;

#ifdef NV_DEBUG

#	define NV_LOG_LOCATION NV_FUNCTION_NAME, __FILE__, __LINE__
#	define NV_LOG_ERROR(text)	::Nv::Logger::doLog(::Nv::LogSeverity::NON_FATAL_ERROR, text, NV_LOG_LOCATION);
#	define NV_LOG_WARN(text)	::Nv::Logger::doLog(::Nv::LogSeverity::WARNING, text, NV_LOG_LOCATION);
#	define NV_LOG_INFO(text)	::Nv::Logger::doLog(::Nv::LogSeverity::INFO, text, NV_LOG_LOCATION);
#	define NV_LOG_FATAL(text)  ::Nv::Logger::doLog(::Nv::LogSeverity::FATAL_ERROR, text, NV_LOG_LOCATION);
#	define NV_LOG_DEBUG(text)  ::Nv::Logger::doLog(::Nv::LogSeverity::DEBUG_INFO, text, NV_LOG_LOCATION);

#	define NV_LOG_ERROR_FORMAT(format, ...)	::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::NON_FATAL_ERROR, NV_LOG_LOCATION, format, __VA_ARGS__);
#	define NV_LOG_WARN_FORMAT(format, ...)	::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::WARNING, NV_LOG_LOCATION, format, __VA_ARGS__);
#	define NV_LOG_DEBUG_FORMAT(format, ...)  ::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::DEBUG_INFO, NV_LOG_LOCATION, format, __VA_ARGS__);
#	define NV_LOG_INFO_FORMAT(format, ...)	::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::INFO, NV_LOG_LOCATION, format, __VA_ARGS__);
#	define NV_LOG_FATAL_FORMAT(format, ...)  ::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::FATAL_ERROR, NV_LOG_LOCATION, format, __VA_ARGS__);
#	define NV_LOG_DEBUG_FORMAT(format, ...)  ::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::DEBUG_INFO, NV_LOG_LOCATION, format, __VA_ARGS__);

#else

#	define NV_LOG_ERROR(text)	::Nv::Logger::doLog(::Nv::LogSeverity::NON_FATAL_ERROR, text);
#	define NV_LOG_WARN(text)	::Nv::Logger::doLog(::Nv::LogSeverity::WARNING, text);
#	define NV_LOG_INFO(text)	::Nv::Logger::doLog(::Nv::LogSeverity::INFO, text);
#	define NV_LOG_FATAL(text)  ::Nv::Logger::doLog(::Nv::LogSeverity::FATAL_ERROR, text);
#	define NV_LOG_DEBUG(text)  

#	define NV_LOG_ERROR_FORMAT(format, ...)	::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::NON_FATAL_ERROR, format, __VA_ARGS__);
#	define NV_LOG_WARN_FORMAT(format, ...)	::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::WARNING, format, __VA_ARGS__);
#	define NV_LOG_DEBUG_FORMAT(format, ...)  ::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::DEBUG_INFO, format, __VA_ARGS__);
#	define NV_LOG_INFO_FORMAT(format, ...)	::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::INFO, format, __VA_ARGS__);
#	define NV_LOG_FATAL_FORMAT(format, ...)  ::Nv::Logger::doLogWithFormat(::Nv::LogSeverity::FATAL_ERROR, format, __VA_ARGS__);
#	define NV_LOG_DEBUG_FORMAT(format, ...) 

#endif

/// Logger interface
class Logger
{
	NV_DECLARE_POLYMORPHIC_CLASS_BASE(Logger)

		/// Report a message.
	virtual void log(ELogSeverity severity, const Char* text, const Char* function, const Char* filename, Int lineNumber) = 0;
		/// Flush the contents to storage
	virtual void flush() {}

		/// Log an error (without function, filename or line number)
	void logError(const Char* text);
	void logErrorWithFormat(const Char* format, ...);

		/// Log with format 
	static Void doLogWithFormat(ELogSeverity severity, const Char* function, const Char* filename, int lineNumber, const Char* format, ...);
	static Void doLog(ELogSeverity severity, const Char* msg, const Char* function, const Char* filename, int lineNumber);

	static Void doLogWithFormat(ELogSeverity severity, const Char* format, ...);
	static Void doLog(ELogSeverity severity, const Char* msg);

		/// Get the log severity as text
	static const Char* getText(ELogSeverity severity);

		// Get the current global logging instance
	NV_FORCE_INLINE static Logger* getInstance() { return s_instance; }
		/// Set the global logging instance
	static Void setInstance(Logger* logger) { s_instance = logger; }

		/// Anything sent to the ignore logger will be thrown away
	static Logger* getIgnoreLogger() { return s_ignoreLogger; }

private:
	static Logger* s_instance;
	static Logger* s_ignoreLogger;
};

} // namespace Nv

#endif // NV_LOGGER_H