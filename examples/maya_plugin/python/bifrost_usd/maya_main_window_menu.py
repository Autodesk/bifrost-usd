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

kTopMenu = "BifrostUSDMenu"

def create_usd_menu():
    # Do not need the menu in batch mode.
    if cmds.about(batch=1):
        return

    # Bifrost USD menu
    cmds.menu(kTopMenu, label="Bifrost USD", parent="MayaWindow", tearOff=True)

    # File submenu
    cmds.menuItem(
        "BifrostUSDFileMenu", label="File", parent=kTopMenu, subMenu=True, tearOff=True
    )

    cmds.menuItem(
        "CreateNewStage",
        parent="BifrostUSDFileMenu",
        command="bifrostUSDExamples -newStage -shape",
        label="New Stage",
        sourceType="mel",
        image="USD_generic_200.png",
        tearOff=True,
    )

    cmds.menuItem(
        "CreateStageFromSublayers",
        parent="BifrostUSDFileMenu",
        rtc="bifrostUsdRtc_CreateStageFromSublayers",
        label="Stage From File...",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(parent="BifrostUSDFileMenu", divider=True, dividerLabel="Component")
    cmds.menuItem(
        "Modeling Export",
        parent="BifrostUSDFileMenu",
        rtc="bifrostUsdRtc_ExportGeometry",
        label="Export Geometry",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        "Save",
        parent="BifrostUSDFileMenu",
        rtc="bifrostUsdRtc_SaveComponent",
        label="Save Component Model",
        sourceType="mel",
        tearOff=True,
    )

    # Create submenu
    cmds.menuItem(
        "BifrostUSDCreateMenu",
        label="Create",
        parent=kTopMenu,
        subMenu=True,
        tearOff=True,
    )

    cmds.menuItem(
        parent="BifrostUSDCreateMenu", divider=True, dividerLabel="Maya Input"
    )
    cmds.menuItem(
        "ImportMayaModelToNewStage",
        parent="BifrostUSDCreateMenu",
        command="bifrostUSDExamples -newStage -importModel",
        label="Create New Stage from Maya selection",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        "ImportMayaVariantsToNewStage",
        parent="BifrostUSDCreateMenu",
        command='bifrostUSDExamples -newStage -variantSetPrim "/Model" -variantSet "VSet"',
        label="Create New Stage with Variants from Maya Selection",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(parent="BifrostUSDCreateMenu", divider=True, dividerLabel="Component")
    cmds.menuItem(
        "CreateComponentCreatorUI",
        parent="BifrostUSDCreateMenu",
        rtc="bifrostUsdRtc_ComponentCreatorUI",
        label="Component Creator",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        "CreateComponentMaterialLibrary",
        parent="BifrostUSDCreateMenu",
        rtc="bifrostUsdRtc_CreateMaterialLibrary",
        label="Create Material Library",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        "AddLookvariant",
        parent="BifrostUSDCreateMenu",
        rtc="bifrostUsdRtc_AddLook",
        label="Add Look Variant",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "AddNewMaterial",
        parent="BifrostUSDCreateMenu",
        rtc="usdModelCmd_AddNewMaterial",
        label="Add New Material",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(parent="BifrostUSDCreateMenu", divider=True, dividerLabel="Lookdev")
    cmds.menuItem(
        "CreateLookdevStageFromLayers",
        parent="BifrostUSDCreateMenu",
        rtc="bifrostUsdRtc_CreateLookdevStageFromLayers",
        label="Create Lookdev Workflow Stage from USD Files",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        "CreateMayaUsdMaterialLibrary",
        parent="BifrostUSDCreateMenu",
        rtc="bifrostUsdRtc_CreateMayaUsdMaterialLibrary",
        label="Create New USD Material Library",
        sourceType="mel",
        tearOff=True,
    )

    # Modify submenu
    cmds.menuItem(
        "BifrostUSDModifyMenu",
        label="Modify",
        parent=kTopMenu,
        subMenu=True,
        tearOff=True,
    )
    cmds.menuItem(
        parent="BifrostUSDModifyMenu", divider=True, dividerLabel="Maya Input"
    )
    cmds.menuItem(
        "ImportMayaModelToStage",
        parent="BifrostUSDModifyMenu",
        command="from bifrost_usd import author_usd_graph; author_usd_graph.add_maya_selection_to_stage()",
        label="Add Maya Model to Stage",
        sourceType="python",
        tearOff=True,
    )
    cmds.menuItem(
        "ImportMayaVariantsToStage",
        parent="BifrostUSDModifyMenu",
        command='bifrostUSDExamples -insertVariant',
        label="Add Maya Variants to Stage",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        parent="BifrostUSDModifyMenu", divider=True, dividerLabel="Bifrost Node"
    )

    cmds.menuItem(
        "InsertAddToStageNode",
        parent="BifrostUSDModifyMenu",
        command='bifrostUSDExamples -insertNode -nodeType "BifrostGraph,USD::Stage,add_to_stage" -currentCompound "" -nodeSelection "" -portSelection "" -inputPort "stage" -outputPort "out_stage"',
        label="Add to Stage",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "InsertSaveStageNode",
        parent="BifrostUSDModifyMenu",
        command='bifrostUSDExamples -insertNode -nodeType "BifrostGraph,USD::Stage,save_usd_stage" -currentCompound "" -nodeSelection "" -portSelection "" -inputPort "stage" -outputPort "out_stage"',
        label="Save Stage",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "AddSelectedPrimPathsToGraph",
        parent="BifrostUSDModifyMenu",
        rtc="bifrostUsdRtc_PrimSelectionToStringArrayCompound",
        label="Add Selected Prim Paths to Graph",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "RemoveSelectedPrimPathsFromGraph",
        parent="BifrostUSDModifyMenu",
        rtc="bifrostUsdRtc_RemovePrimSelectionFromStringToArrayCompound",
        label="Remove Selected Prim Paths from String to Array Node",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "InsertSetEditLayerNode",
        parent="BifrostUSDModifyMenu",
        command='bifrostUSDExamples -insertNode -nodeType "BifrostGraph,USD::Stage,set_edit_layer" -currentCompound "" -nodeSelection "" -portSelection "" -inputPort "stage" -outputPort "out_stage"',
        label="Set Edit Layer",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "InsertSetPrimAttributeNode",
        parent="BifrostUSDModifyMenu",
        command='from bifrost_usd.node_def import NodeDef; from bifrost_usd import author_usd_graph; author_usd_graph.insert_prim_node(author_usd_graph.GraphEditorSelection(), NodeDef(type_name="BifrostGraph,USD::Attribute,set_prim_attribute", input_name="stage", output_name="out_stage", prim_path_param_name="prim_path"))',
        label="Set Prim Attribute",
        sourceType="python",
        tearOff=True,
    )
    cmds.menuItem(
        "InsertSetPrimActiveNode",
        parent="BifrostUSDModifyMenu",
        command='from bifrost_usd.node_def import NodeDef; from bifrost_usd import author_usd_graph; author_usd_graph.insert_prim_node(author_usd_graph.GraphEditorSelection(), NodeDef(type_name="BifrostGraph,USD::Prim,set_prim_active", input_name="stage", output_name="out_stage"))',
        label="Deactivate Prim",
        sourceType="python",
        tearOff=True,
    )
    cmds.menuItem(
        "InsertAddVariantSetNode",
        parent="BifrostUSDModifyMenu",
        command='from bifrost_usd.node_def import NodeDef; from bifrost_usd import author_usd_graph; author_usd_graph.insert_prim_node(author_usd_graph.GraphEditorSelection(), NodeDef(type_name="BifrostGraph,USD::VariantSet,add_variant_set", input_name="stage", output_name="out_stage", prim_path_param_name="prim_path"))',
        label="Add VariantSet",
        sourceType="python",
        tearOff=True,
    )
    cmds.menuItem(
        "InsertAddVariantNode",
        parent="BifrostUSDModifyMenu",
        command='from bifrost_usd.node_def import NodeDef; from bifrost_usd import author_usd_graph; author_usd_graph.insert_prim_node(author_usd_graph.GraphEditorSelection(), NodeDef(type_name="BifrostGraph,USD::VariantSet,add_variant", input_name="stage", output_name="out_stage", prim_path_param_name="prim_path"))',
        label="Add Variant",
        sourceType="python",
        tearOff=True,
    )
    cmds.menuItem(
        "PointInstancerMenu",
        label="Point Instancer Nodes...",
        parent="BifrostUSDModifyMenu",
        subMenu=True,
        tearOff=True,
    )

    cmds.menuItem(
        "SetPointInstancerInvisibleIds",
        parent="PointInstancerMenu",
        command='from bifrost_usd import author_usd_graph; author_usd_graph.set_point_instancer_invisible_ids(author_usd_graph.GraphEditorSelection())',
        label="Hide Point Instances",
        sourceType="python",
        tearOff=True,
    )
    cmds.menuItem(
        "AddPointInstancerScope",
        parent="PointInstancerMenu",
        command='from bifrost_usd.node_def import NodeDef; from bifrost_usd import author_usd_graph; author_usd_graph.insert_prim_node(author_usd_graph.GraphEditorSelection(), NodeDef(is_terminal=True, type_name="BifrostGraph,USD::PointInstancer,usd_point_instancer_scope", input_name="stage", output_name="", prim_path_param_name="point_instancer_path"))',
        label="Add Point Instancer Scope",
        sourceType="python",
        tearOff=True,
    )

    cmds.menuItem(
        parent="BifrostUSDModifyMenu", divider=True, dividerLabel="Rename"
    )
    cmds.menuItem(
        "ChangePrimPath",
        parent="BifrostUSDModifyMenu",
        command="from bifrost_usd.ui import prim_path_dialog; prim_path_dialog.show()",
        label="Change Prim Path",
        sourceType="python",
        tearOff=True,
    )
    cmds.menuItem(
        "RenameVariantSet",
        parent="BifrostUSDModifyMenu",
        command="from bifrost_usd.ui import variant_set_dialog; variant_set_dialog.show()",
        label="Rename Variant Set",
        sourceType="python",
        tearOff=True,
    )

    cmds.menuItem(parent="BifrostUSDModifyMenu", divider=True, dividerLabel="Component")
    cmds.menuItem(
        "RenameCurrentModelVariant",
        parent="BifrostUSDModifyMenu",
        rtc="bifrostUsdRtc_RenameCurrentModelVariant",
        label="Rename Current Model Variant",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "UnassignMaterial",
        parent="BifrostUSDModifyMenu",
        rtc="usdModelCmd_UnassignMaterial",
        label="Unassign Material",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "DuplicateSelectedMaterial",
        parent="BifrostUSDModifyMenu",
        rtc="usdModelCmd_DuplicateSelectedMaterial",
        label="Duplicate Selected Material",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        parent="BifrostUSDModifyMenu", divider=True, dividerLabel="Maya Attribute"
    )

    cmds.menuItem(
        "AddMaterialHintAttrib",
        parent="BifrostUSDModifyMenu",
        rtc="bifrostUsdRtc_AddMaterialHintAttrib",
        label="Add or Modify material_hint Attribute",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        parent="BifrostUSDModifyMenu", divider=True, dividerLabel="Lookdev"
    )

    cmds.menuItem(
        "OpenMaterialLibraryFromLookdevWorkflow",
        parent="BifrostUSDModifyMenu",
        command='from bifrost_usd import create_stage; create_stage.create_materials_stage_from_selected_node()',
        label="Open Lookdev Workflow Materials in New Stage",
        sourceType="python",
        tearOff=True,
        image="material_create.png",
    )

    cmds.menuItem(
        "InsertApplyUsdMaterialBindingsNode",
        parent="BifrostUSDModifyMenu",
        command='bifrostUSDExamples -insertNode -nodeType "BifrostGraph,USD::Shading,apply_usd_material_bindings" -currentCompound "" -nodeSelection "" -portSelection "" -inputPort "stage" -outputPort "out_stage"',
        label="Apply Material Bindings",
        sourceType="mel",
        tearOff=True,
        image="material_create.png",
    )

    # Display submenu
    cmds.menuItem(
        "BifrostUSDDisplayMenu",
        label="Display",
        parent=kTopMenu,
        subMenu=True,
        tearOff=True,
    )

    cmds.menuItem(
        "ShowPurpose",
        label="Show Purpose...",
        parent="BifrostUSDDisplayMenu",
        subMenu=True,
        tearOff=True,
    )
    cmds.menuItem(
        "Guide",
        parent="ShowPurpose",
        rtc="bifrostUsdRtc_ShowGuidePurposes",
        label="Guide",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "Proxy",
        parent="ShowPurpose",
        rtc="bifrostUsdRtc_ShowProxyPurposes",
        label="Proxy",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "Render",
        parent="ShowPurpose",
        rtc="bifrostUsdRtc_ShowRenderPurposes",
        label="Render",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        parent="BifrostUSDDisplayMenu", divider=True, dividerLabel="Component"
    )
    cmds.menuItem(
        "DrawBoundingBoxMode",
        parent="BifrostUSDDisplayMenu",
        rtc="bifrostUsdRtc_SetSelectionToBoundsMode",
        label="Draw in Bounding Box(es) mode",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "DrawInheritedMode",
        parent="BifrostUSDDisplayMenu",
        rtc="bifrostUsdRtc_SetSelectionToInheritedMode",
        label="Draw as Inherited mode",
        sourceType="mel",
        tearOff=True,
    )

    # Windows submenu
    cmds.menuItem(
        "BifrostUSDWindowsMenu",
        label="Windows",
        parent=kTopMenu,
        subMenu=True,
        tearOff=True,
    )

    cmds.menuItem(
        "Open Bifrost Graph Editor from Selected USD Prim",
        parent="BifrostUSDWindowsMenu",
        rtc="bifrostUsdRtc_OpenBifrostUsdGraphEditor",
        label="Open Bifrost Graph Editor from Selected USD Prim",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        "USD Attribute Quick Look from selection",
        parent="BifrostUSDWindowsMenu",
        rtc="bifrostUsdRtc_USDAttributeQuickLookFromSelection",
        label="Show USD Attributes from Selected Prim",
        sourceType="mel",
        tearOff=True,
    )

    cmds.menuItem(
        parent="BifrostUSDWindowsMenu", divider=True, dividerLabel="Component"
    )
    cmds.menuItem(
        "Open Component Graph Editor",
        parent="BifrostUSDWindowsMenu",
        rtc="bifrostUsdRtc_OpenComponentCreatorGraphEditor",
        label="Open Component Creator Graph Editor",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(
        "Open Component Material(s) Editor",
        parent="BifrostUSDWindowsMenu",
        rtc="bifrostUsdRtc_OpenComponentMaterialsInLookdevX",
        label="Open Component Materials in LookdevX Graph Editor",
        sourceType="mel",
        tearOff=True,
    )

    # Directly under top menu
    cmds.menuItem(
        "Rebuild",
        parent=kTopMenu,
        rtc="bifrostUsdRtc_Refresh",
        label="Re-run Bifrost Graph",
        sourceType="mel",
        tearOff=True,
    )
    cmds.menuItem(parent=kTopMenu, divider=True)

    # Help submenu
    cmds.menuItem(
        "BifrostUSDHelpMenu", label="Help", parent=kTopMenu, subMenu=True, tearOff=True
    )

    cmds.menuItem(
        "CreateComponentHelp",
        parent="BifrostUSDHelpMenu",
        rtc="bifrostUsdRtc_ComponentCreatorHelp",
        label="Component Creator",
        sourceType="mel",
        tearOff=True,
    )


def delete_usd_menu():
    cmds.deleteUI(kTopMenu, menu=True)

if __name__ == "__main__":
    create_usd_menu()
