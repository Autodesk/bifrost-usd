//-
// Copyright 2023 Autodesk, Inc.
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

#ifndef BIFROST_HD_GRAPH_EXPORT
#define BIFROST_HD_GRAPH_EXPORT

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(BIFROST_HD_GRAPH_BUILD_DLL)
#define BIFROST_HD_GRAPH_SHARED_DECL __declspec(dllexport)
#else
#define BIFROST_HD_GRAPH_SHARED_DECL __declspec(dllimport)
#endif

#elif defined(__GNUC__)
#if defined(BIFROST_HD_GRAPH_BUILD_DLL)
#define BIFROST_HD_GRAPH_SHARED_DECL __attribute__((visibility("default")))
#else
#define BIFROST_HD_GRAPH_SHARED_DECL
#endif

#endif

#endif // BIFROST_HD_GRAPH_EXPORT
