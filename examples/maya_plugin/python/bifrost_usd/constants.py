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
from typing import Final

from bifrost_usd.node_def import NodeDef

kBifrostBoard: Final = "bifrostBoard"
kBifrostGraphShape: Final = "bifrostGraphShape"
kConstantString: Final = "BifrostGraph,Core::Constants,string"
kOpenGraphToModifyStageMsg: Final = (
    "Open the Bifrost Graph Editor to modify the USD stage"
)
kCreateUsdStage: Final = "BifrostGraph,USD::Stage,create_usd_stage"
kDefaultLayerIdentifier: Final = "untitled.usd"
kGraphName: Final = "bifrostUsd"
kMayaUsdProxyShape: Final = "mayaUsdProxyShape"
kOpenStage: Final = "BifrostGraph,USD::Stage,open_usd_stage"
kCreateLookdevWorkflowStage: Final = "BifrostGraph,USDLab::Workflow,create_lookdev_workflow_stage"
kDefineMaterialBinding: Final = "BifrostGraph,USD::Model,define_usd_material_binding"
kOpenUsdLayer: Final = "BifrostGraph,USD::Layer,open_usd_layer"
kSceneInfo: Final = "BifrostGraph,File::Project,scene_info"
kStringJoin: Final = "BifrostGraph,Core::String,string_join"

kDefinePrim: Final = "BifrostGraph,USD::Prim,define_usd_prim"
kDefineUsdMesh: Final = "BifrostGraph,USD::Prim,define_usd_mesh"
kDefineUsdCurves: Final = "BifrostGraph,USD::define_usd_curves"
kDefineUsdPointInstancer: Final = "BifrostGraph,USD::Prim,define_usd_point_instancer"
kDefineUsdPreviewSurface: Final = "BifrostGraph,USD::Shading,define_usd_preview_surface"
kCreateUsdPrim: Final = "BifrostGraph,USD::Prim,create_usd_prim"
kOverridePrim: Final = "BifrostGraph,USD::Prim,override_prim"
kDefinePrimHierarchy: Final = "BifrostGraph,USD::Prim,define_usd_prim_hierarchy"
kUsdStringPathsToArray: Final = "BifrostGraph,USD::Utils,usd_string_paths_to_array"
kPathExpression: Final = "BifrostGraph,USDLab::PatternMatching,path_expression"

kPrimNodeList: Final = [
    kDefinePrim,
    kDefineUsdMesh,
    kDefineUsdCurves,
    kDefineUsdPointInstancer,
    kDefineUsdPreviewSurface,
    kCreateUsdPrim,
    kOverridePrim,
]

kAddToStage: Final = "BifrostGraph,USD::Stage,add_to_stage"
kAddToStageInVariant: Final = "BifrostGraph,USD::VariantSet,add_to_stage_in_variant"

kAddToStageNodeDef: Final = NodeDef(
    type_name=kAddToStage,
    input_name="stage",
    output_name="out_stage",
    prim_path_param_name="parent_path",
)

kAddVariantSetNodeDef: Final = NodeDef(
    type_name="BifrostGraph,USD::VariantSet,add_variant_set",
    input_name="stage",
    output_name="out_stage",
    prim_path_param_name="prim_path",
)

kAddToStageInVariantNodeDef: Final = NodeDef(
    type_name=kAddToStageInVariant,
    input_name="stage",
    output_name="out_stage",
    prim_path_param_name="parent_path",
)
