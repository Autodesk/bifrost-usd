//-
// Copyright 2022 Autodesk, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//+
/// \file  CfgCompilerMacros.h
/// \brief Macro definition for identifying compilers and there capabilities.

#ifndef BIFUSD_CFG_COMPILER_MACROS_H
#define BIFUSD_CFG_COMPILER_MACROS_H


//==============================================================================
// PLATFORM DETECTION
//==============================================================================

#if defined(_WIN32)
#define BIFUSD_WINDOWS
#elif defined(__linux__)
#define BIFUSD_LINUX
#elif defined(__APPLE__)
#define BIFUSD_OSX
#else
#error "Unsupported platform..."
#endif

//==============================================================================
// COMPILER DETECTION
//==============================================================================

#if defined(__clang__)
//  Clang C++ emulates GCC and MSC, so it has to appear early.
#define BIFUSD_IS_CLANG 1
#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
//  Intel emulates MSC, so it has to appear early.
#define BIFUSD_IS_INTEL 1
#elif defined(__GNUC__)
//  GNU C++.
#define BIFUSD_IS_GCC   1
#elif defined(_MSC_VER)
//  Microsoft visual studio
#define BIFUSD_IS_MSC   1
#else
#error "Unknown compiler configuration..."
#endif

// Default values...
#ifndef BIFUSD_IS_CLANG
#define BIFUSD_IS_CLANG 0
#endif

#ifndef BIFUSD_IS_GCC
#define BIFUSD_IS_GCC 0
#endif

#ifndef BIFUSD_IS_INTEL
#define BIFUSD_IS_INTEL 0
#endif

#ifndef BIFUSD_IS_MSC
#define BIFUSD_IS_MSC 0
#endif

//==============================================================================
// FEATURE DETECTION
//==============================================================================

// Stub out __has_feature when not supported by a given compiler
#ifndef __has_feature
#define __has_feature(x) 0
#endif

//==============================================================================
// COMPILER CAPABILITIES
//==============================================================================

/// \brief Probe for C++ RTTI support
///
/// Set to `1` if the compiler supports C++ RTTI; `0` otherwise. This depends
/// on the compiler flags being used.
#if __has_feature(cxx_rtti) || defined(__GXX_RTTI) || defined(_CPPRTTI)
#define BIFUSD_HAS_CXX_RTTI 1
#else
#define BIFUSD_HAS_CXX_RTTI 0
#endif

/// \brief Force function inlining
///
/// This will provide a hint to the compiler that a method must be inline.
/// Please refer to your compiler's documentation for additional information
/// regarding how "force inline" behave for your compiler.
#if BIFUSD_IS_MSC || BIFUSD_IS_INTEL

#define BIFUSD_FORCEINLINE __forceinline

#elif BIFUSD_IS_GCC || BIFUSD_IS_CLANG

#define BIFUSD_FORCEINLINE inline __attribute__((always_inline))

#endif

/// \brief Enable Empty Base Class Optimization (ECBO)
///
/// This is used to enable ECBO for a given class being declared. For example:
///
/// \code{.cpp}
///     class BIFUSD_ENABLE_ECBO MyClass : public EmptyBassClass,
///                                         public OtherClass
///     {
///        ...
///     };
/// \endcode
///
/// \see https://devblogs.microsoft.com/cppblog/optimizing-the-layout-of-empty-base-classes-in-vs2015-update-2-3
#if BIFUSD_IS_MSC
#define BIFUSD_ENABLE_ECBO __declspec(empty_bases)
#else
#define BIFUSD_ENABLE_ECBO
#endif

//==============================================================================
// EXPORT DIRECTIVES
//==============================================================================

/// \brief Marks a symbol as being externally visible on UNIX platforms
///
/// \warning This macro is only required in special cases. One of those cases
///          is when manually exported templates must be declared as externally
///          visible for the compiler to instantiate them.
#if BIFUSD_IS_GCC || BIFUSD_IS_CLANG
#define BIFUSD_EXPORT_UNIX __attribute__((visibility("default")))
#else
#define BIFUSD_EXPORT_UNIX
#endif

#endif
