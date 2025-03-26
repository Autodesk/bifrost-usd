# -
# *****************************************************************************
# Copyright 2024 Autodesk, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# *****************************************************************************
# +
from maya import cmds
import os

thisDir = os.path.dirname(os.path.realpath(__file__))


def open_bifrost_usd_graph_editor_cmd():
    name = "bifrostUsdRtc_OpenBifrostUsdGraphEditor"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Open Bifrost Graph Editor from Selected MayaUsdProxyShape",
            annotation="Open Bifrost Graph Editor from Selected USD Prim",
            tags="BifrostUSD",
            command="from bifrost_usd import author_usd_graph; author_usd_graph.open_bifrost_graph_from_prim_selection()",
            image=os.path.join(
                thisDir, "..", "..", "icons", "out_bifrostGraphShape.png"
            ),
        )


def usd_attribute_quick_look_from_selection_cmd():
    name = "bifrostUsdRtc_USDAttributeQuickLookFromSelection"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Show USD Attributes from selected prim",
            annotation="Show USD Attributes from selected prim",
            tags="BifrostUSD",
            command="from bifrost_usd.ui import usd_attribute_finder_dialog; usd_attribute_finder_dialog.show()",
            image="USD_generic_200.png",
        )


def prim_selection_to_string_array_compound_cmd():
    name = "bifrostUsdRtc_PrimSelectionToStringArrayCompound"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Add Selected Prim Paths to Graph",
            annotation="Add Selected Prim Paths to Graph",
            tags="BifrostUSD",
            command="from bifrost_usd import author_usd_graph; author_usd_graph.add_string_to_array_compound()",
            image="USD_generic_200.png",
        )


def remove_prim_selection_from_string_to_array_compound_cmd():
    name = "bifrostUsdRtc_RemovePrimSelectionFromStringToArrayCompound"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Remove Selected Prim Paths from String Paths to Array Node",
            annotation="Remove Selected Prim Paths from String Paths to Array Node",
            tags="BifrostUSD",
            command="from bifrost_usd import author_usd_graph; author_usd_graph.remove_from_string_to_array_compound()",
            image="USD_generic_200.png",
        )


def select_prims_from_selected_node_cmd():
    name = "bifrostUsdRtc_SelectPrimsFromSelectedNode"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Select Prims from Selected Node (with a prim path port)",
            annotation="Select Prims from Selected Node (with a prim path port)",
            tags="BifrostUSD",
            command="from bifrost_usd import author_usd_graph; author_usd_graph.select_prims_from_selected_node()",
            image="USD_generic_200.png",
        )


def create_new_stage_cmd():
    name = "bifrostUsdRtc_CreateNewStage"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Create New Stage",
            annotation="Creates a stage with a new root layer",
            tags="BifrostUSD",
            command="bifrostUsd -newStage -shape",
            image="USD_generic_200.png",
        )


def create_stage_from_sublayers_cmd():
    name = "bifrostUsdRtc_CreateStageFromSublayers"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Create Stage from Sublayers...",
            annotation="Creates a stage with a new root layer and (read-only) sublayers from files",
            tags="BifrostUSD",
            command="from bifrost_usd.ui import create_from_sublayers_dialog; create_from_sublayers_dialog.show()",
            image="USD_generic_200.png",
        )


def create_lookdev_stage_from_layers_cmd():
    name = "bifrostUsdRtc_CreateLookdevStageFromLayers"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Create Lookdev Workflow Stage",
            annotation="Creates a stage for Lookdev workflow",
            tags="BifrostUSD",
            command="from bifrost_usd.ui import create_from_sublayers_dialog; create_from_sublayers_dialog.show('lookdev')",
            image="material_create.png",
        )


def create_maya_usd_material_library_cmd():
    name = "bifrostUsdRtc_CreateMayaUsdMaterialLibrary"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Create Maya USD Material Library...",
            annotation="Save a stage with a mtl default prim and open it in a new mayaUsdProxyShape",
            tags="BifrostUSD",
            command="from bifrost_usd.ui import create_maya_usd_proxy_shape_dialog; create_maya_usd_proxy_shape_dialog.show('material_library')",
            image="material_create.png",
        )


def open_maya_usd_material_library_cmd():
    name = "bifrostUsdRtc_OpenMayaUsdMaterialLibrary"
    if not cmds.runTimeCommand(name, exists=True):
        cmds.runTimeCommand(
            name,
            default=True,
            label="BifrostUSD: Open Maya USD Material Library...",
            annotation="Open a stage with a mtl default prim and open it in a new mayaUsdProxyShape",
            tags="BifrostUSD",
            command="from bifrost_usd.ui import open_maya_usd_proxy_shape_dialog; open_maya_usd_proxy_shape_dialog.show()",
            image="material_create.png",
        )


if __name__ == "__main__":
    create_stage_from_sublayers_cmd()
