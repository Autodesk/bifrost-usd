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
/// \file  CfgWarningMacros.h
/// \brief This file defines the generic exclusion for warnings
///
/// Here's an example of how to use the macros defined in this file:
///
/// \code
///      // Push the current set of warnings.
///      BIFUSD_WARNING_PUSH
///
///      // Disable this particular warning when compiling using the
///      // MSC compiler.
///      BIFUSD_WARNING_DISABLE_MSC(4267)
///
///      /* Code for which we want to disable a particular warning. */
///      ...
///
///      // Restore the previous set of warnings.
///      BIFUSD_WARNING_POP
/// \endcode
///
/// \see BaPushLLVMWarnings.h BaPopLLVMWarnings.h

#ifndef BIFUSD_CFG_WARNINGS_MACROS_H
#define BIFUSD_CFG_WARNINGS_MACROS_H

#include "CfgCompilerMacros.h"

/// \brief Helper defining the various warning macros.
///
/// It encapsulates the compiler differences. For gcc and clang, the macro uses
/// the C99 _Pragma() preprocessor directive. It also takes care of stringizing
/// the argument passed to _Pragma(). For MSC, this is using the Microsoft
/// specific __pragma() preprocessor extension.
#if BIFUSD_IS_CLANG || BIFUSD_IS_GCC
#define BIFUSD_DO_PRAGMA(a) _Pragma(#a)
#elif BIFUSD_IS_MSC
#define BIFUSD_DO_PRAGMA(a) __pragma(a)
#else
#define BIFUSD_DO_PRAGMA(a) /* empty */
#endif

/// \brief Warning Push
///
/// This is to be called before a collection of warnings which are to
/// be ignored, and when used should be closed with the
/// BIFUSD_WARNING_POP.
///
/// Any additional compilers that are added to this macro should have
/// an equivalent statement included in the disable and pop macro
/// respectively.
#if BIFUSD_IS_CLANG || BIFUSD_IS_GCC
    #define BIFUSD_WARNING_PUSH BIFUSD_DO_PRAGMA(GCC diagnostic push)
#elif BIFUSD_IS_MSC
    #define BIFUSD_WARNING_PUSH BIFUSD_DO_PRAGMA( warning( push ) )
#else
    #define BIFUSD_WARNING_PUSH
#endif

/// \brief Warning Disable
///
/// This macro should be called within a compiler or OS specific set
/// of warnings, as it does not distinguish which warning identifiers
/// correspond to which compiler. It can be called once for every
/// warning to be disabled.
///
/// Assure that any call of this macro is surrounded with a push and
/// pop macro.
#if BIFUSD_IS_CLANG || BIFUSD_IS_GCC
    #define BIFUSD_WARNING_DISABLE(a) BIFUSD_DO_PRAGMA(GCC diagnostic ignored #a)
#elif BIFUSD_IS_MSC
    #define BIFUSD_WARNING_DISABLE(a) BIFUSD_DO_PRAGMA( warning( disable : a ) )
#else
    #define BIFUSD_WARNING_DISABLE(a)
#endif

/// \brief Disable specific warnings on specific compilers
/// \{
#if BIFUSD_IS_MSC
    #define BIFUSD_WARNING_DISABLE_MSC(a) BIFUSD_WARNING_DISABLE(a)
#else
    #define BIFUSD_WARNING_DISABLE_MSC(a)
#endif

#if BIFUSD_IS_CLANG
    #define BIFUSD_WARNING_DISABLE_CLANG(a) BIFUSD_WARNING_DISABLE(a)
#else
    #define BIFUSD_WARNING_DISABLE_CLANG(a)
#endif

#if BIFUSD_IS_CLANG && (__clang_major__ >= 8)
    #define BIFUSD_WARNING_DISABLE_CLANG_80(a) BIFUSD_WARNING_DISABLE(a)
#else
    #define BIFUSD_WARNING_DISABLE_CLANG_80(a)
#endif

#if BIFUSD_IS_GCC
    #define BIFUSD_WARNING_DISABLE_GCC(a) BIFUSD_WARNING_DISABLE(a)
#else
    #define BIFUSD_WARNING_DISABLE_GCC(a)
#endif
/// \}

/// \brief Warning Pop
///
/// This will end a selection of disabled warnings, and must match to
/// BIFUSD_WARNING_PUSH, after a selection (possibly empty) of
/// BIFUSD_WARNING_DISABLE.
#if BIFUSD_IS_CLANG || BIFUSD_IS_GCC
    #define BIFUSD_WARNING_POP BIFUSD_DO_PRAGMA(GCC diagnostic pop)
#elif BIFUSD_IS_MSC
    #define BIFUSD_WARNING_POP BIFUSD_DO_PRAGMA( warning( pop ) )
#else
    #define BIFUSD_WARNING_POP
#endif

/// \brief Warning Push System/Library Header
///
///    This is to be called when all warnings should be ignored because we are
///    about to parse a system or a library header. The assumption is that there
///    is nothing that can be done about these, so why bother... When used, it
///    should be closed with a matching BIFUSD_WARNING_POP_SYSTEM_HEADER_MSC.
///
/// \warning This macros is only implemented for the MSC compiler. The other
/// compilers support system header through command line options. This is well
/// integrated and supported by CMake and this mechanisms should be used
/// instead.
#if BIFUSD_IS_MSC
    #define BIFUSD_WARNING_PUSH_SYSTEM_HEADER_MSC BIFUSD_DO_PRAGMA( warning(push, 0) )
#else
    #define BIFUSD_WARNING_PUSH_SYSTEM_HEADER_MSC
#endif

/// \brief Warning Pop System/Library Header
///
/// This will end a section of disabled warnings, and must match to
/// BIFUSD_WARNING_PUSH_SYSTEM_HEADER.
#if BIFUSD_IS_MSC
    #define BIFUSD_WARNING_POP_SYSTEM_HEADER_MSC BIFUSD_DO_PRAGMA( warning( pop ) )
#else
    #define BIFUSD_WARNING_POP_SYSTEM_HEADER_MSC
#endif

#endif
