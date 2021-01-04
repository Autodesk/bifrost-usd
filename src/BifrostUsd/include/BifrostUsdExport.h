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

#ifndef USD_EXPORT_H
#define USD_EXPORT_H

#if defined(_WIN32)
#define USD_EXPORT __declspec(dllexport)
#define USD_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#define USD_EXPORT __attribute__((visibility("default")))
#define USD_IMPORT __attribute__((visibility("default")))
#else
#define USD_EXPORT
#define USD_IMPORT
#endif

#if defined(USD_BUILD_DLL)
#define USD_DECL USD_EXPORT
#else
#define USD_DECL USD_IMPORT
#endif

#endif
