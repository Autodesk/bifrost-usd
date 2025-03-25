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
from typing import Dict, List, Optional, Set
from dataclasses import dataclass
import contextlib

from maya import cmds
import ufe
import mayaUsd

from bifrost_usd.constants import (
    kAddToStage,
    kAddToStageInVariant,
    kAddToStageInVariantNodeDef,
    kBifrostBoard,
    kBifrostGraphShape,
    kCreateUsdPrim,
    kDefinePrim,
    kDefinePrimHierarchy,
    kDefineUsdCurves,
    kDefineMaterialBinding,
    kDefineUsdMesh,
    kDefineUsdPointInstancer,
    kDefineUsdPreviewSurface,
    kMayaUsdProxyShape,
    kOverridePrim,
    kPathExpression,
    kUsdStringPathsToArray,
)

from bifrost_usd.node_def import NodeDef

from bifrost_usd.graph_api import (
    error,
    get_graph_selection,
    GraphAPI,
    GraphEditorSelection,
    GraphPaused,
    open_bifrost_graph,
    warning,
)

from bifrost_usd.maya_usd_custom_attributes import get_parent_dag_path
from bifrost_usd.maya_usd_custom_attributes import get_all_prim_types


def log(args):
    pass
    # print(args)


@contextlib.contextmanager
def CurrentMayaSelection(sel):
    """Restore original Maya selection"""
    yield
    cmds.select(sel, replace=True)


graphAPI = GraphAPI()


@dataclass
class StagePortData:
    node: str
    port: str


def get_connected_stage_port(
    graph_name: str, current_compound: str, node_name: str
) -> StagePortData:
    """Find the node downstream with an input port connected to the output of the "stage node".
    A "stage node" is a node with an output port named "out_stage" or "stage".
    """
    if not (rtn := graphAPI.connexions(node_name, "out_stage", graph_name)):
        rtn = graphAPI.connexions(node_name, "stage", graph_name)

    stagePortData = StagePortData("", "")
    if rtn:
        stagePortData.node = rtn[0].split(".")[0]
        stagePortData.port = rtn[0].split(".")[1]

    return stagePortData


def insert_stage_node(
    graph_selection: GraphEditorSelection, new_node_def: NodeDef
) -> str:
    """Insert a node downstream of the selected node.

    :return: The inserted node.
    """
    graphSelection = _get_graph_selection_if_needed(graph_selection)
    if not graphSelection.nodeSelection:
        return ""

    srcNode = graphSelection.nodeSelection[0]

    # no explicit output, then take last output of the node.
    if not graphSelection.output:
        if connectedPorts := graphAPI.connected_ports(srcNode):
            # assume there is only one connected output
            # TODO: Add query of port type to vnnNode command.
            output = connectedPorts[-1]
            graphSelection.output = output.split(".")[-1]
        else:
            warning("Selected node is not connected")
            return ""

    if not graphSelection.output:
        warning("Selected node is not connected")
        return ""

    if not (newNode := graphAPI.add_node(new_node_def.type_name)):
        # if there is no new node, Maya should print an error.
        return ""

    # store the connection graph_selection of source node before connecting it to a new node downstream
    cnxInfo = get_connected_stage_port(
        graphSelection.dgContainerFullPath, graphSelection.currentCompound, srcNode
    )

    # Connect output of source node to input of new node
    graphAPI.connect(srcNode, graphSelection.output, newNode, new_node_def.input_name)

    if not new_node_def.is_terminal:
        # Connect output of new node to input of node connected to source node
        if cnxInfo.node == graphSelection.dgContainerName:
            graphAPI.connect(newNode, new_node_def.output_name, "", cnxInfo.port)
        else:
            newNodeName = cnxInfo.node
            graphAPI.connect(
                newNode, new_node_def.output_name, newNodeName, cnxInfo.port
            )

    return newNode


def insert_prim_node(
    graph_selection: GraphEditorSelection, new_node_def: NodeDef
) -> str:
    primPath = ""
    if ufeSelection := cmds.ls(selection=True, ufe=True):
        segments = ufeSelection[0].split(",")
        if len(segments) > 1:
            primPath = segments[-1]

    graphSelection = _get_graph_selection_if_needed(graph_selection)
    newNode = insert_stage_node(graph_selection, new_node_def)
    if not newNode:
        error(f"Unexpected error. Can't not insert node {new_node_def.type_name}")

    if primPath:
        graphAPI.set_param(
            newNode,
            (new_node_def.prim_path_param_name, primPath),
            current_compound=graphSelection.currentCompound,
        )
    else:
        warning(
            f"No prim selected, can't set '{new_node_def.prim_path_param_name}' parameter in '{newNode}' node."
        )

    return newNode


def set_point_instancer_invisible_ids(
    graph_selection: GraphEditorSelection = GraphEditorSelection(),
) -> bool:
    graphSelection = _get_graph_selection_if_needed(graph_selection)
    if not graphSelection.nodeSelection:
        return False

    pInstancerPath, ids = get_point_instancer_instance_id_from_selection()
    if not ids:
        warning("No Instances selected")
        return False

    setPointInstancesInvisibleNodeDef = NodeDef(
        type_name="BifrostGraph,USD::PointInstancer,set_usd_point_instances_invisible",
        input_name="stage",
        output_name="out_stage",
        prim_path_param_name="prim_path",
    )
    setPointInstancesInvisibleNode = insert_stage_node(
        graphSelection, setPointInstancesInvisibleNodeDef
    )
    stringToArrayNode = graphAPI.add_node("BifrostGraph,Core::String,string_to_array")
    graphAPI.connect(
        stringToArrayNode,
        "long_int_array",
        setPointInstancesInvisibleNode,
        "invisible_ids",
    )
    graphAPI.set_param(setPointInstancesInvisibleNode, ("prim_path", pInstancerPath))
    graphAPI.set_param(stringToArrayNode, ("comma_separated_string", ",".join(ids)))

    return True


def _get_maya_paths_from_selection() -> tuple[list, list]:
    meshesPaths = []
    leafPaths = []

    def _get_children(node, paths):
        children = cmds.listRelatives(node, type="transform", fullPath=True)
        if children:
            paths += children
            for child in children:
                _get_children(child, paths)

        return paths

    transformList = cmds.ls(type="transform", selection=True, long=True)
    if not transformList:
        warning("Please select a Maya transform node")

    for transform in transformList:
        paths: list = []
        _get_children(transform, paths)
        for item in paths:
            meshes = cmds.listRelatives(
                item, type="mesh", noIntermediate=True, fullPath=True
            )
            if meshes:
                meshesPaths += meshes
            else:
                if not cmds.listRelatives(
                    item, allDescendents=True, noIntermediate=True
                ):
                    leafPaths.append(item)

    if not meshesPaths:
        for transform in cmds.ls(type="transform", selection=True, long=True):
            # listRelatives can returns None in python
            rtn = cmds.listRelatives(
                transform, type="mesh", noIntermediate=True, fullPath=True
            )

            if rtn:
                meshesPaths += rtn

    return meshesPaths, leafPaths


def _get_connected_add_to_stage(node_name: str) -> str:
    addToStageNode = ""
    if nodes := graphAPI.connexions(node_name):
        for node in nodes:
            if graphAPI.type_name(node) == "BifrostGraph,USD::Stage,add_to_stage":
                addToStageNode = node
                break

    return addToStageNode


def resolve_prim_path(
    dg_container: str, add_to_stage_path: str, prim_path: str
) -> tuple[str, str]:
    """When a "define prim" compound is connected to an add_to_stage compound with its 'parent_path' parameter set to
    a value not equal to "/" or to an empty string, the value of its 'path' parameter should be relative to the 'parent_path'.


    :param: dg_container The Maya dag path
    :param: add_to_stage_path The 'add_to_stage' compound path
    :param: prim_path The full path of the prim

    :return: The parent path and the path without the parent path.
    """

    primPath = prim_path
    parentPath = graphAPI.param(add_to_stage_path, "parent_path")
    parentPartsLenght = len(parentPath.split("/"))
    if parentPath and parentPath != "/":
        tokens = primPath.split("/")
        if len(tokens) > 2:
            primPath = "/" + "/".join(prim_path.split("/")[parentPartsLenght:])

    return parentPath, primPath


def _add_maya_mesh_to_stage(
    mesh_path: str,
    graph_selection: GraphEditorSelection,
    add_to_stage_path: str,
    index: int,
    mergeTransformAndShape: Optional[bool] = True,
) -> None:
    ioNode = graphAPI.add_node("Input")

    # TODO: vnn command renaming a node should return the new name.
    ioNodeName = "Input_by_Path_USD" + str(index)
    suffix = index
    while ioNodeName in find_all_input_by_path_nodes():
        suffix += index
        ioNodeName = "Input_by_Path_USD" + str(suffix)

    meshName = "mesh" + str(suffix)

    graphAPI.rename_node(ioNode, ioNodeName)

    pathInfo = "pathinfo={path=" + mesh_path + ";setOperation=+;active=true}"
    graphAPI.create_output_port(ioNodeName, (meshName, "Object"), pathInfo)
    graphAPI.set_metadata(ioNodeName, ("bifrostUSD", "input_by_path"))
    graphAPI.create_input_port(
        add_to_stage_path, (f"prim_definitions.mesh{str(suffix)}", "auto")
    )

    defineHierarchy = graphAPI.add_node(kDefinePrimHierarchy)
    graphAPI.connect(
        src_node=defineHierarchy,
        out_port="prim_definitions",
        target_node=add_to_stage_path,
        in_port=f"prim_definitions.mesh{str(suffix)}",
    )

    primPath = mesh_path.replace("|", "/")
    if mergeTransformAndShape:
        primPath = "/".join(primPath.split("/")[:-1])

    parentPath, primPath = resolve_prim_path(
        graph_selection.dgContainerFullPath, add_to_stage_path, primPath
    )

    graphAPI.set_param(defineHierarchy, ("path", primPath))

    primTypesList = get_all_prim_types(get_parent_dag_path(mesh_path))
    primTypesList.reverse()
    primTypesStr = " ".join(primTypesList)
    graphAPI.set_param(defineHierarchy, ("types", primTypesStr))

    graphAPI.set_param(defineHierarchy, ("parent_is_scope", "1" if parentPath else "0"))

    graphAPI.connect("", meshName, defineHierarchy, "leaf_mesh")

    # collapse node
    graphAPI.set_metadata(defineHierarchy, ("DisplayMode", "1"))


def _add_maya_leaf_xform_to_stage(
    xform_path: str,
    graph_selection: GraphEditorSelection,
    add_to_stage_path: str,
    index: int,
) -> None:
    suffix = str(index)
    graphAPI.create_input_port(
        add_to_stage_path, (f"prim_definitions.xform{suffix}", "auto")
    )
    defineHierarchy = graphAPI.add_node(kDefinePrimHierarchy)
    graphAPI.connect(
        defineHierarchy,
        "prim_definitions",
        add_to_stage_path,
        f"prim_definitions.xform{suffix}",
    )

    primPath = xform_path.replace("|", "/")
    _, primPath = resolve_prim_path(
        graph_selection.dgContainerFullPath, add_to_stage_path, primPath
    )

    graphAPI.set_param(defineHierarchy, ("path", primPath))

    primTypesList = get_all_prim_types(xform_path)
    primTypesList.reverse()
    primTypesStr = " ".join(primTypesList)
    graphAPI.set_param(defineHierarchy, ("types", primTypesStr))

    graphAPI.set_param(defineHierarchy, ("parent_is_scope", "0"))


def _add_maya_selection_to_stage(
    graph_selection: GraphEditorSelection, add_to_stage_path: str
):
    meshSelection, xfoLeafSelection = _get_maya_paths_from_selection()
    for index, meshPath in enumerate(meshSelection):
        index += 1
        _add_maya_mesh_to_stage(meshPath, graph_selection, add_to_stage_path, index)

    for index, xfoPath in enumerate(xfoLeafSelection):
        index += 1
        _add_maya_leaf_xform_to_stage(
            xfoPath, graph_selection, add_to_stage_path, index
        )


def add_maya_selection_to_stage(
    graph_selection: Optional[GraphEditorSelection] = None,
) -> bool:
    if not graph_selection:
        graph_selection = get_graph_selection()

    if not (nodeSelection := graph_selection.nodeSelection):
        warning("Please select a add_to_stage compound")
        return False

    def _valid_node_type(node: str) -> bool:
        if graphAPI.type_name(node) == kAddToStage:
            return True
        elif graphAPI.type_name(node) == kAddToStageInVariant:
            return True
        return False

    srcNode = nodeSelection[0]
    if not _valid_node_type(srcNode):
        warning("Please select a add_to_stage compound")
        return False

    with GraphPaused(graph_selection.dgContainerFullPath):
        _add_maya_selection_to_stage(
            graph_selection, graph_selection.currentCompound + srcNode
        )

    return True


def add_one_maya_selection_as_variant_to_stage(
    prim_path: str,
    variant_set_name: str,
    maya_path: str,
    graph_selection: GraphEditorSelection,
) -> str:
    """:return: The name of the added add_to_stage_in_variant compound"""
    addToStageInVariantNode = insert_stage_node(
        graph_selection, kAddToStageInVariantNodeDef
    )
    graphAPI.set_param(addToStageInVariantNode, ("parent_path", prim_path))
    graphAPI.set_param(addToStageInVariantNode, ("variant_set_name", variant_set_name))
    graphAPI.set_param(addToStageInVariantNode, ("variant_name", maya_path))
    graph_selection.nodeSelection = [addToStageInVariantNode]
    cmds.select(maya_path, replace=True)
    add_maya_selection_to_stage(graph_selection)
    # and hide Maya selection
    cmds.setAttr(f"{maya_path}.visibility", False)
    return addToStageInVariantNode


def _get_graph_selection_if_needed(
    graph_selection: GraphEditorSelection, warning_msg: str = ""
) -> GraphEditorSelection:
    """When the graph selection is empty, creates a new GraphEditorSelection from
    Bifrost Graph Editor selection, else do nothing and returns the graph selection unchanged.
    """
    if not graph_selection.nodeSelection:
        # keep the port name since it can't be deduced from the selection.
        output = graph_selection.output
        graph_selection = get_graph_selection()
        if not graph_selection.nodeSelection:
            if not warning_msg:
                msg = "You must select a node upstream (with a stage output) in the Bifrost Graph Editor"
            else:
                msg = warning_msg
            warning(msg)

        graph_selection.output = output

    return graph_selection


def insert_maya_variant(graph_selection: GraphEditorSelection) -> str:
    assert (
        graph_selection.output == "out_stage"
    ), "[insert_maya_variant] GraphEditorSelection output should be 'out_stage'"

    graphSelection = _get_graph_selection_if_needed(graph_selection)
    if not graphSelection.nodeSelection:
        return ""

    srcNode = graphSelection.nodeSelection[0]

    def _get_select_prim_path() -> str:
        for item in iter(ufe.GlobalSelection.get()):
            shapePath = item.path().segments[0]
            if mayaUsd.ufe.getStage(str(shapePath)):
                return str(item.path().segments[1])
        return ""

    parentPath = _get_select_prim_path()
    if not parentPath:
        parentPath = graphAPI.param(srcNode, "parent_path")

    variantSetName = "VSet"
    if graphAPI.type_name(srcNode) == kAddToStageInVariant:
        variantSetName = graphAPI.param(srcNode, "variant_set_name")

    mayaSelection = cmds.ls(type="transform", selection=True, long=False)
    if mayaSelection:
        mayaPath = mayaSelection[0]
        return add_one_maya_selection_as_variant_to_stage(
            parentPath, variantSetName, mayaPath, graphSelection
        )
    else:
        warning(
            "No Maya transform selected, can't add new variant to BifrostUSD graph "
        )

    return ""


def _add_prim_definition(add_to_stage_path: str, prim_path: str) -> str:
    graphAPI.create_input_port(
        add_to_stage_path, ("prim_definitions.prim_definition", "auto")
    )
    definePrim = graphAPI.add_node(kDefinePrim)
    graphAPI.connect(
        definePrim,
        "prim_definition",
        add_to_stage_path,
        "prim_definitions.prim_definition",
    )
    graphAPI.set_param(definePrim, ("path", prim_path))
    graphAPI.set_param(definePrim, ("kind", "2"))  # component kind

    return definePrim


def _add_maya_selection_as_variants_to_stage(
    prim_path: str,
    variant_set_name: str,
    graph_selection: GraphEditorSelection,
    add_to_stage_path: str,
) -> None:
    # Store Maya selection
    mayaSelection = cmds.ls(type="transform", selection=True, long=False)
    with CurrentMayaSelection(mayaSelection):
        _add_prim_definition(add_to_stage_path, prim_path)
        # Add variants
        lastAddToStageInVariantNode = ""
        for mayaPath in mayaSelection:
            lastAddToStageInVariantNode = add_one_maya_selection_as_variant_to_stage(
                prim_path, variant_set_name, mayaPath, graph_selection
            )
        if lastAddToStageInVariantNode:
            graphAPI.set_param(lastAddToStageInVariantNode, ("select", "1"))


def add_maya_selection_as_variants_to_stage(
    prim_path: str,
    variant_set_name: str,
    graph_selection: Optional[GraphEditorSelection] = None,
) -> bool:
    if not graph_selection:
        graph_selection = get_graph_selection()

    if not (nodeSelection := graph_selection.nodeSelection):
        warning("Please select a add_to_stage compound")
        return False

    srcNode = nodeSelection[0]
    if graphAPI.type_name(srcNode) != "BifrostGraph,USD::Stage,add_to_stage":
        warning("Please select a add_to_stage compound")
        return False

    with GraphPaused(graph_selection.dgContainerFullPath):
        _add_maya_selection_as_variants_to_stage(
            prim_path,
            variant_set_name,
            graph_selection,
            graph_selection.currentCompound + srcNode,
        )
    return True


def find_all_prim_paths(
    node_type: str = kDefinePrimHierarchy,
) -> list[str]:
    paths = []
    for node in graphAPI.find_nodes(node_type):
        paths.append(graphAPI.param(node, "path"))

    return paths


def find_all_input_by_path_nodes() -> list[str]:
    """TODO: Remove this workaround to retrieve input by path nodes"""
    allNodes = graphAPI.find_nodes()

    inputByPathNodes = []
    for node in allNodes:
        rtn = graphAPI.metadata(node, "bifrostUSD")
        if rtn and rtn[0] == "input_by_path":
            inputByPathNodes.append(node)

    return inputByPathNodes


def is_new_path_a_parent(path: str, new_path: str) -> bool:
    log(f"[is_new_path_a_parent]: args: path: {path}, new_path: {new_path}")

    pathTokens = path.split("/")
    newPathTokens = new_path.split("/")

    if newPathTokens[-1] == pathTokens[-1]:
        return True

    return False


def rename_nodes_path_parameter(
    old_prim_path: str, new_prim_path: str, node_type: str = kDefinePrimHierarchy
) -> None:
    allNodes = graphAPI.find_nodes(node_type)
    if allNodes:
        log(f"[rename_nodes_path_parameter] args: {old_prim_path} to {new_prim_path}")

    for node in allNodes:
        rename_path_parameter(node, old_prim_path, new_prim_path)


def rename_path_parameter(node: str, old_prim_path: str, new_prim_path: str) -> None:
    """If the path parameter value of the node is equal to the old prim path, replace it by the new one."""
    pathValue = graphAPI.param(node, "path")
    addToStageNode = _get_connected_add_to_stage(node)

    parentPath, oldPrimPath = resolve_prim_path(
        graphAPI.name, f"/{addToStageNode}", old_prim_path
    )

    _, newPrimPath = resolve_prim_path(
        graphAPI.name, f"/{addToStageNode}", new_prim_path
    )

    log(
        f"[rename_path_parameter] args({old_prim_path}, {new_prim_path}). pathValue: {pathValue} TRY TO REPLACE {oldPrimPath} with {newPrimPath}"
    )
    if pathValue.startswith(oldPrimPath):
        newPath = pathValue.replace(oldPrimPath, newPrimPath)
        log(f"[rename_path_parameter] PATH REPLACED BY {newPath}")
        graphAPI.set_param(node, ("path", newPath))


def reparent_nodes_path_parameter(
    old_prim_path: str, new_prim_path: str, node_type: str = kDefinePrimHierarchy
) -> None:
    allNodes = graphAPI.find_nodes(node_type)
    if allNodes:
        log(f"[reparent_nodes_path_parameter] from {old_prim_path} to {new_prim_path}")

    for node in allNodes:
        log(f"[reparent_nodes_path_parameter]     node {node}")
        reparent_path_parameter(node, old_prim_path, new_prim_path)


def reparent_path_parameter(node: str, old_prim_path: str, new_prim_path: str) -> None:
    """If the path parameter value of the node is equal to the old prim path, replace it by the new one
    if it is a valid reparent path."""
    pathValue = graphAPI.param(node, "path")
    log(f"[reparent_nodes_path_parameter]         pathValue {pathValue}")

    addToStageNode = _get_connected_add_to_stage(node)

    if not addToStageNode:
        return

    parentPath, newPrimPath = resolve_prim_path(
        graphAPI.name, f"/{addToStageNode}", new_prim_path
    )

    if not parentPath and is_new_path_a_parent(pathValue, new_prim_path):
        newPath = pathValue.replace(pathValue, newPrimPath)
        log(f"[reparent_nodes_path_parameter]                 NEW PATH {newPath}")
        graphAPI.set_param(node, ("path", newPath))
        return


def delete_node(old_prim_path: str, node_type: str) -> None:
    def _delete(node: str) -> bool:
        pathValue = graphAPI.param(node, "path")

        addToStage = _get_connected_add_to_stage(node)

        parentPath, oldPrimPath = resolve_prim_path(
            graphAPI.name, f"/{addToStage}", old_prim_path
        )

        if oldPrimPath == "/":
            return False

        if pathValue == oldPrimPath:
            if graphAPI.connexions(node, "leaf_mesh"):
                for ibp in find_all_input_by_path_nodes():
                    if cnx := graphAPI.connexions(ibp):
                        if cnx[0] == node:
                            graphAPI.remove_node(node)
                            graphAPI.remove_node(ibp)
            else:
                graphAPI.remove_node(node)

            return True

        return False

    for node in graphAPI.find_nodes(node_type):
        if _delete(node):
            break


def belong_to_maya_model(ufe_path_str: str) -> bool:
    # remove |world prefix.
    ufeToMayaPath = ufe_path_str[6:]

    for prim_path in find_all_prim_paths():
        primToMayaPath = prim_path.replace("/", "|")
        if cmds.ls(primToMayaPath, long=True):
            if primToMayaPath == ufeToMayaPath:
                return True

    return False


def is_supported_prim_type(ufe_path_str: str) -> bool:
    ufePath = ufe.PathString.path(ufe_path_str)
    ufeItem = ufe.Hierarchy.createItem(ufePath)

    # if the item is directly under "|world" there is no UFE item
    if not ufeItem:
        # remove |world prefix.
        if cmds.ls(ufe_path_str[6:], type="transform"):
            return True

    if ufeItem and ufeItem.nodeType() in ["transform", "mesh"]:
        return True

    return False


def get_point_instancer_instance_id_from_selection() -> tuple[str, list[str]]:
    """:return: The PointInstancer path and the instances ids"""
    ids = []
    pInstancerPath: str = ""

    for sel in cmds.ls(selection=True, ufe=True):
        primPath = sel.split(",")[-1]
        tokens = primPath.split("/")
        leaf = tokens[-1]
        if leaf.isdigit():
            ids.append(leaf)
            if not pInstancerPath:
                pInstancerPath = "/".join(tokens[:-1])

    return pInstancerPath, ids


def to_prim_path(ufe_path: str) -> str:
    """Convert an UFE absolute path to the value stored in the path
    parameter of a "define prim" compound. If the UFE path point to a mesh
    the shape part of the path is removed."""
    if (ufe_path == "|world|") or (not ufe_path.startswith("|world|")):
        return ""

    mayaPath = "|" + "|".join(ufe_path.split("|")[2:])

    # Since we merge transform and shape on the Bifrost side,
    # remove last name of te path if it is a mesh
    if cmds.ls(mayaPath, type="mesh"):
        mayaPath = "|".join(mayaPath.split("|")[:-1])

    primPath = mayaPath.replace("|", "/")
    return primPath


def get_prim_selection() -> Dict[str, List[str]]:
    """Get the selected prims in a MayaUsdProxyShape.

    Returns a dictionary of MayaUsdProxyShape's name to USD prim's paths.
    """

    selection: Dict[str, List[str]] = {}

    pathList: List[str] = cmds.ls(
        selection=True, type=kMayaUsdProxyShape, ufeObjects=True
    )

    for path in pathList:
        shapeAndPrim = path.split(",")
        if len(shapeAndPrim) > 1:
            node, prim = shapeAndPrim
            selection[node] = selection.get(node, []) + [prim]

    if not selection:
        warning("No Usd Prim selected")

    return selection


def get_prim_selection_as_string() -> str:
    primPaths: str = ""
    selection = get_prim_selection()
    for node in selection:
        primPaths += ", ".join(selection[node])

    return primPaths


def get_bifrost_graph_from_prim_selection() -> tuple[str, str]:
    """Get the Bifrost graph connected to the MayaUsdProxyShape of the selected USD prim.

    Returns the name of the graph and the name of the output port of the graph.
    """
    if not (selection := get_prim_selection()):
        warning("No Usd Prim selected")
    else:
        usdProxyShape = list(selection.keys())[0]
        assert cmds.nodeType(usdProxyShape) == kMayaUsdProxyShape

        def get_bifrost_connection_info(
            usdProxyShape: str, bifrostNodeType: str
        ) -> tuple[str, str]:
            if cmds.listConnections(usdProxyShape, type=bifrostNodeType):
                connectionsPlugs = cmds.listConnections(
                    usdProxyShape, type=bifrostNodeType, plugs=True
                )
                if connectionsPlugs:
                    return connectionsPlugs[0].split(".")

            return ("", "")

        graph, plug = get_bifrost_connection_info(usdProxyShape, kBifrostBoard)
        if not graph:
            graph, plug = get_bifrost_connection_info(usdProxyShape, kBifrostGraphShape)

        return graph, plug

    return ("", "")


def open_bifrost_graph_from_prim_selection() -> None:
    graph = get_bifrost_graph_from_prim_selection()[0]
    open_bifrost_graph(graph)


def get_variant_set_names() -> Set[str]:
    names: Set[str] = set()
    for node in graphAPI.find_nodes(kAddToStageInVariant):
        names.add(graphAPI.param(node, "variant_set_name"))

    return names


def update_variant_set_names(
    current_variant_set_name: str, new_variant_set_name: str
) -> None:
    for node in graphAPI.find_nodes(kAddToStageInVariant):
        if graphAPI.param(node, "variant_set_name") == current_variant_set_name:
            graphAPI.set_param(node, ("variant_set_name", new_variant_set_name))


def get_prim_paths_from_node_types(node_types: list[str]) -> Set[str]:
    paths: Set[str] = set()

    portName = ""
    for typeName in node_types:
        if (
            typeName == kDefinePrim
            or typeName == kDefineUsdMesh
            or typeName == kDefineUsdCurves
        ):
            portName = "path"
        elif typeName == kDefineUsdPointInstancer:
            portName = "instancer_path"
        if typeName == kDefineUsdPreviewSurface:
            portName = "path"
        if typeName == kCreateUsdPrim:
            portName = "path"
        if typeName == kOverridePrim:
            portName = "path"
        if typeName == kAddToStageInVariant:
            portName = "parent_path"

        assert portName != "", f"Can not resolve port name for type {typeName}"

        for node in graphAPI.find_nodes(typeName):
            paths.add(graphAPI.param(node, portName))

    return paths


def update_prim_path(current_prim_path: str, new_prim_path: str) -> None:
    for node in graphAPI.find_nodes(kDefinePrim):
        if graphAPI.param(node, "path") == current_prim_path:
            graphAPI.set_param(node, ("path", new_prim_path))

    for node in graphAPI.find_nodes(kDefineUsdMesh):
        if graphAPI.param(node, "path") == current_prim_path:
            graphAPI.set_param(node, ("path", new_prim_path))

    for node in graphAPI.find_nodes(kDefineUsdCurves):
        if graphAPI.param(node, "path") == current_prim_path:
            graphAPI.set_param(node, ("path", new_prim_path))

    for node in graphAPI.find_nodes(kDefineUsdPointInstancer):
        if graphAPI.param(node, "instancer_path") == current_prim_path:
            graphAPI.set_param(node, ("instancer_path", new_prim_path))

    for node in graphAPI.find_nodes(kDefineUsdPreviewSurface):
        if graphAPI.param(node, "path") == current_prim_path:
            graphAPI.set_param(node, ("path", new_prim_path))

    for node in graphAPI.find_nodes(kCreateUsdPrim):
        if graphAPI.param(node, "path") == current_prim_path:
            graphAPI.set_param(node, ("path", new_prim_path))

    for node in graphAPI.find_nodes(kOverridePrim):
        if graphAPI.param(node, "path") == current_prim_path:
            graphAPI.set_param(node, ("path", new_prim_path))

    for node in graphAPI.find_nodes(kAddToStageInVariant):
        if graphAPI.param(node, "parent_path") == current_prim_path:
            graphAPI.set_param(node, ("parent_path", new_prim_path))


def add_string_to_array_compound() -> None:
    graphSelection = _get_graph_selection_if_needed(
        GraphEditorSelection(),
        warning_msg="You must select a 'define_usd_prim', "
        "'define_usd_material_binding' or 'path_expression' node in the Bifrost Graph Editor\n"
        "          from which a 'string_to_array' node will be connected. Or select a 'string_to_array' "
        "to append prim paths to it.",
    )
    if not graphSelection.nodeSelection:
        return

    selectedNode = graphSelection.nodeSelection[0]
    if graphAPI.type_name(selectedNode) == kUsdStringPathsToArray:
        previousPaths = graphAPI.param(selectedNode, "paths")
        newPaths = previousPaths + ", " + get_prim_selection_as_string()
        graphAPI.set_param(
            selectedNode,
            ("paths", newPaths),
        )
        return

    stringToArrayNode = ""
    typeName = graphAPI.type_name(selectedNode)

    if typeName == kDefineMaterialBinding:
        stringToArrayNode = graphAPI.add_node(kUsdStringPathsToArray)
        graphAPI.connect_to_fanin_port(
            stringToArrayNode,
            selectedNode,
            "prim_paths",
            "path_array",
        )

    elif typeName == kDefinePrim:
        stringToArrayNode = graphAPI.add_node(kUsdStringPathsToArray)
        graphAPI.connect(stringToArrayNode, "path_array", selectedNode, "path")
    elif typeName == kPathExpression:
        stringToArrayNode = graphAPI.add_node(kUsdStringPathsToArray)
        graphAPI.connect(stringToArrayNode, "path_array", selectedNode, "prim_path")

    if stringToArrayNode:
        graphAPI.set_param(
            stringToArrayNode, ("paths", get_prim_selection_as_string())
        )
    else:
        warning("Selected node is not supported")


def remove_from_string_to_array_compound() -> None:
    graphSelection = _get_graph_selection_if_needed(
        GraphEditorSelection(),
        warning_msg="You must select a 'string_to_array' node in the Bifrost Graph Editor",
    )

    if not graphSelection.nodeSelection:
        return

    selectedNode = graphSelection.nodeSelection[0]
    if graphAPI.type_name(selectedNode) != kUsdStringPathsToArray:
        return

    originalValue = graphAPI.param(selectedNode, ("paths"))

    originalTokens = originalValue.split(",")
    originalTokens = [token.replace(" ", "") for token in originalTokens]
    tokensToRemove = get_prim_selection_as_string().split(",")
    tokensToRemove = [token.replace(" ", "") for token in tokensToRemove]
    newTokens = []
    for token in originalTokens:
        keep = True
        for badToken in tokensToRemove:
            if token == badToken:
                keep = False
                break
        if keep:
            newTokens.append(token)

    graphAPI.set_param(selectedNode, ("paths", ", ".join(newTokens)))


def get_maya_usd_proxy_shape_from_bifrost_usd_graph(graph: str) -> str:
    cnx = cmds.listConnections(graph, type=kMayaUsdProxyShape, shapes=True)
    if cnx:
        return cmds.ls(cnx[0], long=True)[0]

    return ""


def select_prims_from_selected_node() -> None:
    primPaths: list[str] = []
    mayaUsdProxyShape = get_maya_usd_proxy_shape_from_bifrost_usd_graph(
        graphAPI._getGraphName()
    )
    if not mayaUsdProxyShape:
        return

    graphSelection = _get_graph_selection_if_needed(
        GraphEditorSelection(),
        warning_msg="You must select a node in the Bifrost Graph Editor",
    )

    if not graphSelection.nodeSelection:
        return

    selectedNode = graphSelection.nodeSelection[0]
    if graphAPI.type_name(selectedNode) == kUsdStringPathsToArray:
        primPaths = graphAPI.param(selectedNode, "paths").split(" ")
        primPaths = [token.replace(" ", "") for token in primPaths]
        primPaths = [token.replace(",", "") for token in primPaths]

        if primPaths:
            cmds.select(clear=True)

        for item in primPaths:
            cmds.select(f"{mayaUsdProxyShape},{item}", add=True)
        return

    elif graphAPI.type_name(selectedNode) == "BifrostGraph,USD::Model,define_usd_material_binding":
        primPath = graphAPI.param(selectedNode, "material")
        if primPath:
            cmds.select(clear=True)
            cmds.select(f"{mayaUsdProxyShape},{primPath}", add=True)

    elif graphAPI.type_name(selectedNode) in (
        kDefinePrim,
        kDefineUsdMesh,
        kDefineUsdCurves,
        kDefineUsdPointInstancer,
        kDefinePrimHierarchy,
    ):
        primPath = graphAPI.param(selectedNode, "path")
        if primPath:
            cmds.select(clear=True)
            cmds.select(f"{mayaUsdProxyShape},{primPath}", add=True)


if __name__ == "__main__":
    add_string_to_array_compound()
