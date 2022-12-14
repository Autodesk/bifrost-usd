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

#ifndef USD_MODULE_DECL
#define USD_MODULE_DECL

#if defined(USD_MODULE_API_NO_API)
#define USD_MODULE_API
#elif defined(_WIN32) || defined(__CYGWIN__)
#ifdef USD_MODULE_API_IMPL
#ifdef __GNUC__
#define USD_MODULE_API __attribute__((dllexport))
#else
#define USD_MODULE_API \
    __declspec(        \
        dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define USD_MODULE_API __attribute__((dllimport))
#else
#define USD_MODULE_API \
    __declspec(        \
        dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#else
#if __GNUC__ >= 4
#define USD_MODULE_API __attribute__((visibility("default")))
#else
#define USD_MODULE_API
#endif
#endif

#endif
