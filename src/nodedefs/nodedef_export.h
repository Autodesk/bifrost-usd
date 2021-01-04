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

//
/// \file nodedef_export.h
///
/// \brief Definition of macros for symbol visibility.
///
#ifndef USD_NODEDEF_EXPORT_H
#define USD_NODEDEF_EXPORT_H

#if defined(_WIN32)
#define NODEDEF_EXPORT __declspec(dllexport)
#define NODEDEF_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#define NODEDEF_EXPORT __attribute__((visibility("default")))
#define NODEDEF_IMPORT __attribute__((visibility("default")))
#else
#define NODEDEF_EXPORT
#define NODEDEF_IMPORT
#endif

#if defined(USD_BUILD_NODEDEF_DLL)
#define USD_NODEDEF_DECL NODEDEF_EXPORT
#else
#define USD_NODEDEF_DECL NODEDEF_IMPORT
#endif

#endif
