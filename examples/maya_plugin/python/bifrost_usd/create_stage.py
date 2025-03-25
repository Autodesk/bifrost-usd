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

from typing import Callable
import os
from maya import cmds
import mayaUsd
import ufe

from bifrost_usd.constants import (
    kBifrostBoard,
    kBifrostGraphShape,
    kConstantString,
    kCreateUsdStage,
    kCreateLookdevWorkflowStage,
    kDefineMaterialBinding,
    kDefaultLayerIdentifier,
    kGraphName,
    kMayaUsdProxyShape,
    kOpenStage,
    kOpenUsdLayer,
    kSceneInfo,
    kStringJoin,
    kAddToStageNodeDef,
    kOpenGraphToModifyStageMsg,
)

from bifrost_usd.graph_api import (
    find_bifrost_usd_graph,
    GraphAPI,
    get_graph_selection,
    open_bifrost_usd_graph,
)

from bifrost_usd.author_usd_graph import (
    insert_stage_node,
    CurrentMayaSelection,
)

graphAPI = GraphAPI()


def _create_empty_graph(as_shape: bool) -> str:
    """Create an new Bifrost graph without input node.

    :param as_shape: To create a bifrostGraphShape or a bifrostBoard node.
    :returns:       The name of the new graph.
    """
    if graph := find_bifrost_usd_graph():
        cmds.warning(f"{graph} found. {kOpenGraphToModifyStageMsg}.")
        return graph

    if not graph:
        graph = cmds.createNode(kBifrostGraphShape if as_shape else kBifrostBoard)

    if as_shape:
        transform = cmds.listRelatives(graph, parent=True, fullPath=True)[0]
        transform = cmds.rename(transform, kGraphName)
        graph = cmds.listRelatives(transform)[0]
    else:
        graph = cmds.rename(graph, kGraphName)

    # To know if a graph was created using the Bifrost USD commands.
    # Not used anymore but kept as an information.
    cmds.addAttr(
        graph,
        shortName="usdw",
        longName="USDWorkflow",
        attributeType="bool",
        defaultValue=True,
        hidden=True,
    )

    graphAPI.remove_node("input", graph)
    return graph


def _set_shared_stage(graph: str, value: bool) -> None:
    connections = cmds.listConnections(graph, type=kMayaUsdProxyShape)
    if connections:
        usdProxyShape = connections[0]
        cmds.setAttr(f"{usdProxyShape}.shareStage", value)


def _output_stage_to_maya(graph: str, node: str, node_output: str = "stage") -> str:
    graphAttribs = cmds.listAttr(graph, readOnly=True, scalar=True, fromPlugin=True)
    hasStageOutput = False
    if graphAttribs:
        if "stage" in graphAttribs:
            hasStageOutput = True

    if not hasStageOutput:
        graphAPI.create_input_port("output", ("stage", "BifrostUsd::Stage"), graph)
    graphAPI.connect(node, node_output, "", "stage", graph)

    _set_shared_stage(graph, False)
    return graph


def create_new_stage_graph(as_shape: bool = True) -> str:
    """Creates a stage with a new root layer and output it from the graph.

    :param as_shape: To create a bifrostGraphShape or a bifrostBoard node.
    :returns:       The name of the new graph.
    """

    if graph := find_bifrost_usd_graph():
        cmds.warning(f"{graph} found. {kOpenGraphToModifyStageMsg}.")
        return graph

    graph = _create_empty_graph(as_shape)
    createStageNode = graphAPI.add_node(kCreateUsdStage, graph)
    graphAPI.set_param(createStageNode, ("layer", kDefaultLayerIdentifier), graph)
    _output_stage_to_maya(graph, createStageNode)

    open_bifrost_usd_graph()

    return graph


def _as_relative_path(file_path):
    result = file_path
    scenePath = cmds.file(query=True, sceneName=True)
    if scenePath:
        dirPath = os.path.dirname(scenePath)
        result = os.path.relpath(file_path, dirPath)
    else:
        result = file_path

    return result


def _create_sublayers_loader_graph(
    graph: str, file_paths: list[str], relative_path=True
) -> None:
    """Creates a stage with a new root layer and (read-only) sublayers from files.
    The resulting stage is the output of the graph."""
    createStageNode = graphAPI.add_node(kCreateUsdStage, graph)

    relativePath = cmds.file(query=True, sceneName=True) and relative_path
    if relativePath:
        sceneInfoNode = graphAPI.add_node(kSceneInfo, graph)

    graphAPI.set_param(createStageNode, ("layer", kDefaultLayerIdentifier), graph)

    i = ""
    for fpath in file_paths:
        openLayerNode = graphAPI.add_node(kOpenUsdLayer, graph)
        graphAPI.set_metadata(openLayerNode, ("DisplayMode", "1"), graph)

        if relativePath:
            stringJoinNode = graphAPI.add_node(kStringJoin, graph)
            graphAPI.set_metadata(stringJoinNode, ("DisplayMode", "1"), graph)
            graphAPI.set_param(stringJoinNode, ("separator", ""), graph)
            graphAPI.enable_fanin_port(
                stringJoinNode, port_name="strings", graph_name=graph
            )

            # connect scene_info to string_join
            graphAPI.create_input_port(
                stringJoinNode, ("strings.scene_directory", "string"), graph
            )
            graphAPI.connect(
                sceneInfoNode,
                "scene_directory",
                stringJoinNode,
                "strings.scene_directory",
                graph,
            )

            valueNode = graphAPI.add_node(kConstantString, graph)
            graphAPI.set_metadata(valueNode, ("DisplayMode", "1"), graph)
            graphAPI.set_param(valueNode, ("value", _as_relative_path(fpath), graph))
            graphAPI.create_input_port(
                stringJoinNode, ("strings.relative_path", "string"), graph
            )
            graphAPI.connect(
                valueNode, "output", stringJoinNode, "strings.relative_path", graph
            )
            graphAPI.connect(stringJoinNode, "joined", openLayerNode, "file", graph)

        graphAPI.set_param(openLayerNode, ("file", fpath), graph)
        graphAPI.create_input_port(
            createStageNode, (f"sublayers.layer{i}", "auto"), graph
        )
        graphAPI.connect(
            openLayerNode, "layer", createStageNode, f"sublayers.layer{i}", graph
        )

        i = str(int(i) + 1) if i else "1"

    # Output stage to Maya
    graphAPI.create_input_port("output", ("stage", "BifrostUsd::Stage"), graph)
    graphAPI.connect(createStageNode, "stage", "", "stage", graph)

    _set_shared_stage(graph, False)


def _create_open_stage_graph(graph: str, file_path: str) -> None:
    """Creates a stage from a USD file and output it from the graph."""
    openStageNode = graphAPI.add_node(kOpenStage, graph)
    graphAPI.set_param(openStageNode, ("file", file_path), graph)
    _output_stage_to_maya(graph, openStageNode)


def create_graph_from_usd_files(
    file_paths: list[str],
    as_shape: bool = True,
    as_sublayers: bool = True,
) -> str:
    if not file_paths:
        cmds.warning("No file paths found")
        return ""

    graph = _create_empty_graph(as_shape=as_shape)

    if len(file_paths) == 1 and not as_sublayers:
        _create_open_stage_graph(graph, file_paths[0])
    else:
        _create_sublayers_loader_graph(graph, file_paths)

    open_bifrost_usd_graph()

    return graph


def create_graph_with_add_to_stage() -> str:
    with CurrentMayaSelection(cmds.ls(selection=True)):
        # Create a Bifrost USD graph with a create_usd_stage connected to an add_to_stage
        graph = create_new_stage_graph(as_shape=True)
        selection = get_graph_selection()
        selection.nodeSelection = ["create_usd_stage"]
        selection.output = "stage"

        selection.nodeSelection = [insert_stage_node(selection, kAddToStageNodeDef)]

        return graph


def create_lookdev_graph_from_usd_files(
    file_paths: list[str],
    as_shape: bool = True,
) -> str:
    if not file_paths:
        cmds.warning("No file paths found")
        return ""

    graph = _create_empty_graph(as_shape=as_shape)

    lookdevWorkflowNode = graphAPI.add_node(kCreateLookdevWorkflowStage, graph)
    graphAPI.set_param(lookdevWorkflowNode, ("geometry_layer", file_paths[0]), graph)

    rootDir = os.path.dirname(file_paths[0])
    graphAPI.set_param(
        lookdevWorkflowNode,
        ("root_layer", os.path.join(rootDir, "untitled.usd")),
        graph,
    )
    graphAPI.set_param(
        lookdevWorkflowNode, ("look_layer", os.path.join(rootDir, "look.usd")), graph
    )

    materialsLayer: str
    if len(file_paths) == 1:
        materialsLayer = os.path.join(rootDir, "material_library.usd")
        setup_new_material_stage(materialsLayer)
        graphAPI.set_param(
            lookdevWorkflowNode, ("materials_layer", materialsLayer), graph
        )
    elif len(file_paths) > 1:
        materialsLayer = file_paths[1]

    graphAPI.set_param(lookdevWorkflowNode, ("materials_layer", materialsLayer), graph)

    materialNames = []
    from pxr import Usd
    stage = Usd.Stage.Open(materialsLayer)
    mtlScope = stage.GetDefaultPrim()
    if mtlScope:
        materialNames = mtlScope.GetChildrenNames()

    _output_stage_to_maya(graph, lookdevWorkflowNode, node_output="out_stage")

    if materialNames:
        for name in materialNames:
            materialBindingNode = graphAPI.add_node(kDefineMaterialBinding, graph)
            graphAPI.set_param(materialBindingNode, ("material", name), graph)
            graphAPI.connect_to_fanin_port(
                materialBindingNode,
                lookdevWorkflowNode,
                "material_bindings",
                "material_binding",
            )

    open_bifrost_usd_graph()

    return graph


def create_maya_usd_proxy_shape(
    name: str,
    file_path: str,
    stage_setup_func: Callable[[str], bool] = lambda file_path: True,
) -> str:
    """Create a mayaUsdProxyShape.

    1) If the USD file path does not exist, creates a new one.
    2) Call optional stage setup function.

    :param name:             The name of the new mayaUsdProxyShape node.
    :param file_path:        The USD file path.
    :param stage_setup_func: Optional function used to setup the stage.
    :returns:                The new mayaUsdProxyShape node path.
    """

    createNewFile = False

    # We need to create a new file before creating the mayaUsdProxyShape
    # since opening a non existing file would create an anonymous root layer
    if not os.path.isfile(file_path):
        stage_setup_func(file_path)
        createNewFile = True

    shape = cmds.createNode(kMayaUsdProxyShape, skipSelect=True)
    transform = cmds.listRelatives(shape, parent=True)[0]
    transformName = cmds.rename(transform, f"{name}")
    shapeName = cmds.listRelatives(transformName, children=True)[0]
    proxyShapePath = f"|{transformName}|{shapeName}"

    cmds.setAttr(f"{proxyShapePath}.filePath", file_path, type="string")
    cmds.setAttr(f"{proxyShapePath}.filePathRelative", False)

    if not createNewFile:
        stage_setup_func(proxyShapePath)

    return proxyShapePath


def create_materials_stage_from_selected_node() -> str:
    selection = get_graph_selection()

    if not selection.nodeSelection:
        cmds.warning("Select a 'create_lookdev_workflow_stage' compound from the Bifrost Graph Editor")
        return ""

    lookdevWorkflowNode = selection.nodeSelection[0]

    materialsLayer = graphAPI.param(lookdevWorkflowNode, ("materials_layer"), selection.dgContainerFullPath)
    return create_maya_usd_proxy_shape('materialLibrary', materialsLayer, setup_open_materials_stage)


# Setup functions for create_maya_usd_proxy_shape


def setup_new_material_stage(matLibPath: str) -> bool:
    from pxr import Usd
    stage = Usd.Stage.CreateNew(matLibPath)
    mtl = stage.DefinePrim("/mtl", "Scope")
    stage.SetDefaultPrim(mtl)
    stage.Save()
    return True


def setup_open_materials_stage(proxyShapePath: str) -> bool:
    stage = mayaUsd.ufe.getStage(proxyShapePath)
    stage = mayaUsd.ufe.getStage(
        ufe.PathString.string(ufe.PathString.path(proxyShapePath))
    )
    mtl = stage.GetPrimAtPath("/mtl")
    if not mtl:
        cmds.warning(f"'mtl' prim not found in Material Library {proxyShapePath} node")
        return False

    return True


if __name__ == "__main__":
    pass
