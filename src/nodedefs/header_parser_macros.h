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

/// \file header_parser_macros.h
/// \brief Helper node annotation macros.

#ifndef USD_NODEDEF_HEADER_PARSER_MACROS
#define USD_NODEDEF_HEADER_PARSER_MACROS

#include <Amino/Cpp/Annotate.h>

#define DOC_PATH "\\.\\./docs/\\$\\{language\\}/"

/// \brief Set annotation with node's name, documentation, icon and more.
///
/// Annotate a node definition with its name, tis documentation file name, its
/// icon file name and any number of extra annotation parameters such as
/// Amino::DefaultOverload, Amino::Converter, Amino::Promoter, etc...
#define USDNODE_DOC_ICON_X(NAME, DOC_FILENAME, ICON_FILENAME, EXTRA)           \
    AMINO_ANNOTATE("Amino::Node " EXTRA " name=" NAME                          \
                   " metadata=[{documentation, string, " DOC_PATH DOC_FILENAME \
                   ".md},"                                                     \
                   "{icon, string, \\../icons/" ICON_FILENAME "}]")

/// \brief Set annotation with node's name, documentation and icon.
///
/// Annotate a node definition with its name, its documentation file name and
/// its icon file name.
#define USDNODE_DOC_ICON(NAME, DOC_FILENAME, ICON_FILENAME) \
    USDNODE_DOC_ICON_X(NAME, DOC_FILENAME, ICON_FILENAME, )

/// \brief Set annotation for a usd open file browser widget.
///
/// Annotate a port definition with a UIWidget metadata used to create a
/// file browser to open files using USD formats
#define USDNODE_FILE_BROWSER_OPEN                                             \
    AMINO_ANNOTATE(                                                           \
        "Amino::Port metadata=[{UIWidget, string, FileBrowserWidget}, "       \
        "{UIWidgetProp, string, browserMode=open;filter=\"USD (*.usd *.usda " \
        "*.usdc *.usdz);;All (*.*)\"}] ")

#endif

/// \brief Set annotation for a usd save file browser widget.
///
/// Annotate a port definition with a UIWidget metadata used to create a
/// file browser to save files using USD formats
#define USDNODE_FILE_BROWSER_SAVE                                             \
    AMINO_ANNOTATE(                                                           \
        "Amino::Port metadata=[{UIWidget, string, FileBrowserWidget}, "       \
        "{UIWidgetProp, string, browserMode=save;filter=\"USD (*.usd *.usda " \
        "*.usdc);;All (*.*)\"}] ")

/// \brief Hide node from the graph library, with extra annotations.
#define USDNODE_INTERNAL_X(NAME, DOC_FILENAME, EXTRA)                 \
    AMINO_ANNOTATE("Amino::Node " EXTRA " name=" NAME                 \
                   " metadata=[{documentation," DOC_PATH DOC_FILENAME \
                   ".md}, {internal, bool, true}]")

/// \brief Hide node from the graph library.
#define USDNODE_INTERNAL(NAME, DOC_FILENAME) \
    USDNODE_INTERNAL_X(NAME, DOC_FILENAME, )

/// \brief Set annotation for in/out port name.
#define USDPORT_INOUT(OUT_NAME) AMINO_ANNOTATE("Amino::InOut outName=" OUT_NAME)
