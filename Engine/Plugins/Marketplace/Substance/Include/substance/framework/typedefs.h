//! @file typedefs.h
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#ifndef _SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H
#define _SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H

#include <assert.h>
#include <cstddef>
#include <cinttypes>

// Detect Modern C++ Features
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1900)	//MSVC 2015
#	define AIR_CPP_MODERN_MEMORY
#endif

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include <stdlib.h>

#ifdef __linux__
#	include <string.h>
#endif

#include "memory.h"

namespace SubstanceAir
{

//! @brief basic types used in the integration framework
typedef unsigned int UInt;
typedef unsigned long long UInt64;

//! @brief STL shared_ptr adjusted to use our allocators
template<typename T> using shared_ptr = std::shared_ptr<T>;

//! @brief STL make_shared, construct a shared_ptr using this helper. This is preferred to allocating 
//! by yourself because make_shared should make one allocation for both the object and the control block
template<typename T, class... Args> inline shared_ptr<T> make_shared(Args&& ... args)
{
	return std::allocate_shared<T>(aligned_allocator<T, AIR_DEFAULT_ALIGNMENT>(), std::forward<Args>(args)...);
}

//! @brief STL alias for unique_ptr
template<typename T> using unique_ptr = std::unique_ptr<T, deleter<T>>;

//! @brief create a unique ptr via template
template<typename T, typename... Args> unique_ptr<T> make_unique(Args&&... args)
{
	return unique_ptr<T>(AIR_NEW(T)(std::forward<Args>(args)...));
}

//! @brief safely cast a unique ptr between types. This will invalidate the caster unique_ptr
template<typename U, typename T> unique_ptr<U> static_unique_ptr_cast(unique_ptr<T>&& base)
{
	return unique_ptr<U>(static_cast<U*>(base.release()));
}

//! @brief make_shared and make_unique can only construct classes from public constructors, 
//! so you can use this type to convert a protected construct to public
//!	e.g. std::make_shared<make_constructible<SubstanceAir::InputImage>>(args)
template<typename T> struct make_constructible : public T
{
	template<typename... Args> make_constructible(Args... args)
		: T(std::forward<Args>(args)...)
	{
	}
};

//! @brief STL atomics mapped to our namespace
template<typename T>
using atomic = std::atomic<T>;
//! @brief STL atomic specialization mapped to our namespace
using atomic_bool = std::atomic_bool;
//! @brief STL atomic specialization mapped to our namespace
using atomic_uint = std::atomic_uint;

//! @brief STL weak_ptr mapped to our namespace
template<typename T>
using weak_ptr = std::weak_ptr<T>;

//! @brief STL containers mapped to substance air allocators
template<typename T, std::size_t ALIGNMENT> 
using aligned_vector = std::vector<T, aligned_allocator<T, ALIGNMENT>>;

template<typename T> 
using deque = std::deque<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT>>;

template<typename K, typename V> 
using map = std::map<K, V, std::less<K>, aligned_allocator<std::pair<const K, V>, AIR_DEFAULT_ALIGNMENT>>;

template<typename T> 
using vector = aligned_vector<T, AIR_DEFAULT_ALIGNMENT>;

typedef std::basic_string<char, std::char_traits<char>, aligned_allocator<char, AIR_DEFAULT_ALIGNMENT>> string;
typedef std::basic_stringstream<char, std::char_traits<char>, aligned_allocator<char, AIR_DEFAULT_ALIGNMENT>> stringstream;

static inline string to_string(std::string& str)
{
	return string(str.c_str());
}

static inline string to_string(const std::string& str)
{
	return string(str.c_str());
}

static inline string to_string(const char* str)
{
	return string(str);
}

static inline std::string to_std_string(string& str)
{
	return std::string(str.c_str());
}

static inline std::string to_std_string(const string& str)
{
	return std::string(str.c_str());
}

}  // namespace SubstanceAir

#include "vector.h"

namespace SubstanceAir
{
//! @brief containers used in the integration framework
typedef vector<unsigned char> BinaryData;
typedef vector<UInt> Uids;

// Vectors
typedef Math::Vector2<float> Vec2Float;
typedef Math::Vector3<float> Vec3Float;
typedef Math::Vector4<float> Vec4Float;
typedef Math::Vector2<int> Vec2Int;
typedef Math::Vector3<int> Vec3Int;
typedef Math::Vector4<int> Vec4Int;
}

#endif //_SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H
