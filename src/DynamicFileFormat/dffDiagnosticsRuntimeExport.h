//-
// Copyright 2026 Autodesk, Inc.
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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_RUNTIME_EXPORT_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_RUNTIME_EXPORT_H

#if defined(_WIN32)
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_EXPORT __declspec(dllexport)
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_EXPORT \
    __attribute__((visibility("default")))
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_IMPORT \
    __attribute__((visibility("default")))
#else
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_EXPORT
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_IMPORT
#endif

#if defined(BIFROSTUSD_DFF_DIAGNOSTICS_BUILD_SHARED)
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL \
    BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_EXPORT
#else
#define BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL \
    BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_IMPORT
#endif

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_RUNTIME_EXPORT_H
