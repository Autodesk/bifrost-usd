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
import os
import sys
import shutil
import tempfile
import os.path
import random

from pxr import Sdf, Usd, UsdShade

from maya import cmds
from maya import mel

import ufe

from mayaUsd import lib as mayaUsdLib

from bifrost_usd.constants import (
    kBifrostBoard,
    kMayaUsdProxyShape,
)

from bifrost_usd.component_creator.constants import (
    kArnoldMeshPrimvarsCompound,
    kComponentStageShape,
    kComponentStageShapeFullName,
    kCreateComponentCompound,
    kDefaultMatLibFileName,
    kGraphName,
    kHUD_UsdComponentVariants,
    kLookVariantCompound,
    kMaterialBindingCompound,
    kMatLibName,
    kMatLibShapeFullName,
    kModelVariantCompound,
    kPathExpressionCompound,
)

from bifrost_usd.component_creator import asset_template
from bifrost_usd.component_creator import purpose

from bifrost_usd import graph_api


def find_bifrost_component_graph() -> str:
    return kGraphName


graphAPI = graph_api.GraphAPI(find_bifrost_component_graph)


def source_mel_script() -> None:
    """On right click in the viewport, the Maya USD plugin calls a mel procedure
    called "USDUserMenuProc". This function is sourcing such procedure located in
    the script directory of the Maya plugin.
    """
    packageDir = os.path.dirname(os.path.realpath(__file__))
    menuProc = os.path.join(
        packageDir, "..", "..", "..", "scripts", "USDUserMenuProc.mel"
    )
    menuProc = menuProc.replace("\\", "/")
    mel.eval(f'source "{menuProc}"')


def load_plugin_if_needed(plugin_name: str) -> None:
    if cmds.pluginInfo(plugin_name, query=True, loaded=True):
        return
    try:
        cmds.loadPlugin(plugin_name, quiet=True)

    except RuntimeError:
        cmds.error(f"Can't load plugin {plugin_name}")

    # [TODO] Since LookdevX plugin is also using a USDUserMenuProc menu
    # we are forcing re-sourcing ours. It is a temporary solution until
    # we can register custom menu.
    source_mel_script()


# More specific API related to the "create_usd_component" workfows.


def hasComponentCreatorGraph() -> bool:
    """Returns True if there is a valid Bifrost Graph called "componentCreator".
    A valid graph
    """
    if cmds.ls(kGraphName):
        return True
    return False


def pause_graph(value):
    cmds.setAttr(f"{kGraphName}.runOnDemand", value)


def re_run_graph():
    if hasComponentCreatorGraph():
        pause_graph(True)
        pause_graph(False)
        run_graph()


def name() -> str:
    return graphAPI.param("create_usd_component", "name")


def set_name(name: str) -> None:
    graphAPI.set_param("create_usd_component", ("name", name))


def model_version() -> str:
    return graphAPI.param("create_usd_component", "version")


def set_version(version: str) -> None:
    graphAPI.set_param("create_usd_component", ("version", version))


def search_mode() -> str:
    return graphAPI.param("create_usd_component", "search_mode")


def set_search_mode(value: str):
    graphAPI.set_param("create_usd_component", ("search_mode", value))


def get_model_dir() -> str:
    if search_mode() == "Custom":
        return os.path.join(sub_directory(), name(), model_version())
    return ""


def get_geo_dir() -> str:
    if search_mode() == "Custom":
        return os.path.join(
            sub_directory(), name(), model_version(), geometry_scope_name()
        )
    return ""


def get_mtl_dir() -> str:
    if search_mode() == "Custom":
        return os.path.join(
            sub_directory(), name(), model_version(), material_scope_name()
        )
    return ""


def sub_directory() -> str:
    return graphAPI.param("create_usd_component", "sub_directory")


def set_sub_directory(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("sub_directory", value))


def geometry_scope_name() -> str:
    if hasComponentCreatorGraph():
        return graphAPI.param("create_usd_component", "geometry_scope_name")

    # For unit tests not requiring the Bifrost graph
    return "geo"


def set_geometry_scope_name(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("geometry_scope_name", value))


def model_variant_set_name() -> str:
    return graphAPI.param("create_usd_component", "model_variant_set_name")


def set_model_variant_set_name(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("model_variant_set_name", value))


def look_variant_set_name() -> str:
    return graphAPI.param("create_usd_component", "look_variant_set_name")


def set_look_variant_set_name(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("look_variant_set_name", value))


def default_model_variant() -> str:
    return graphAPI.param("create_usd_component", "default_model_variant")


def set_default_model_variant(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("default_model_variant", value))
    look_variants = get_look_variant_names()

    for name in look_variants:
        missingLook = True
        for node in get_look_variant_nodes(default_model_variant()):
            if graphAPI.param(node, "variant_name") == name:
                missingLook = False
        if missingLook:
            add_look(name)
        missingLook = True

    set_heads_up_display()


def default_look_variant() -> str:
    return graphAPI.param("create_usd_component", "default_look_variant")


def set_default_look_variant(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("default_look_variant", value))
    set_heads_up_display()


def material_library_file() -> str:
    matLibPath = graphAPI.param("create_usd_component", "material_library_file")

    # if just the file name is set, resolve to the file path
    if matLibPath and not os.path.dirname(matLibPath):
        matLibPath = os.path.join(get_mtl_dir(), matLibPath)

    unixMatLibPath = matLibPath.replace("\\", "/")
    # on Windows, USD returns drive letter in lowercase
    if sys.platform == "win32":
        tokens = unixMatLibPath.split(":")
        tokens[0] = tokens[0].lower()
        tokens = [tokens[0]] + tokens[1:]
        unixMatLibPath = ":".join(tokens)

    return unixMatLibPath


def set_material_library_file(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("material_library_file", value))


def material_scope_name() -> str:
    return graphAPI.param("create_usd_component", "material_scope_name")


def set_material_scope_name(value: str) -> None:
    graphAPI.set_param("create_usd_component", ("material_scope_name", value))


def get_path_in_scope(prim_path: str, scope: str) -> str:
    beforeAndAfterGeoScope = prim_path.split(f"/{scope}/")
    assert (
        len(beforeAndAfterGeoScope) == 2
    ), f"Invalid path, no children under scope for path: '{prim_path}'"

    return beforeAndAfterGeoScope[1]


def get_port_children(node: str, port_name: str, style: str = "") -> list[str]:
    ports = graphAPI.port_children(node, port_name)

    if style == "node_parentport_port":
        for i in range(len(ports)):
            ports[i] = node + "." + port_name + "." + ports[i]
    elif style == "node_port":
        for i in range(len(ports)):
            ports[i] = node + "." + ports[i]

    return ports


def get_create_usd_component_node() -> str:
    if nodes := graphAPI.find_nodes(kCreateComponentCompound):
        return nodes[0]

    return ""


def _create_material_library_proxy_shape(material_library_path: str) -> None:
    if cmds.ls(kMatLibShapeFullName):
        shape = kMatLibShapeFullName
    else:
        # Set the name of the shape at creation time instead of renaming the transform / shape
        # with the rename command because that will cause Maya to crash in UFE during a undo operation.
        shape = cmds.createNode(kMayaUsdProxyShape, skipSelect=True, name=f"{kMatLibName}Shape")

        transform = cmds.listRelatives(shape, parent=True)[0]

        hideInOutliner = False
        if os.getenv("BIFROST_USD_LAB_HIDE_MATERIAL_LIBRARY_STAGE", ""):
            hideInOutliner = True

        cmds.setAttr(f"{transform}.hiddenInOutliner", hideInOutliner)
        cmds.setAttr(f"{shape}.intermediateObject", hideInOutliner)

    assert cmds.ls(shape) == [f"{kMatLibName}Shape"], "Material Library shape not found"

    cmds.setAttr(f"{shape}.filePath", material_library_path, type="string")
    cmds.setAttr(f"{shape}.filePathRelative", False)


def _create_empty_graph() -> str:
    graph = cmds.createNode(kBifrostBoard)
    graph = cmds.rename(graph, kGraphName)
    graphAPI.remove_node("input")

    return graph


def _create_component_compound(graph) -> None:
    if not get_create_usd_component_node():
        graphAPI.add_node(kCreateComponentCompound)


def _create_component_stage(graph) -> None:
    graphAPI.create_input_port("output", ("stage", "BifrostUsd::Stage"))
    graphAPI.connect("create_usd_component", "stage", "", "stage")

    if connected_nodes := cmds.listConnections(graph):
        stageShape = cmds.rename(connected_nodes[0], f"{kGraphName}Stage")
        cmds.setAttr(f"{stageShape}.shareStage", True)


def _create_component_graph(picked_dir: str, version: str) -> tuple[str, dict]:
    graph = _create_empty_graph()
    _create_component_compound(graph)

    name = os.path.basename(picked_dir)

    set_name(name)
    set_version(version)
    set_search_mode("Custom")
    set_sub_directory(os.path.dirname(picked_dir))

    assetDir = os.path.join(picked_dir, version)

    geoDirName = "geo"
    # If there is "Geometries" or "Geos" directory, use it
    # instead of the "geo" one.
    for path in os.scandir(assetDir):
        if path.is_dir() and path.name == "Geometries":
            geoDirName = path.name
            break
        if path.is_dir() and path.name == "Geos":
            geoDirName = path.name
            break

    set_geometry_scope_name(geoDirName)

    if not os.path.isdir((geoDirPath := os.path.join(picked_dir, version, geoDirName))):
        cmds.warning(f"Directory {geoDirName} not found. Can not create USD component.")
        return "", {}

    geoDirData = asset_template.get_geo_dir_data(geoDirPath)

    return graph, geoDirData


def _get_validated_purposes_path(
    variant: asset_template.PurposesFilePaths,
) -> tuple[str, str, str]:
    """The "proxy", "default" and "render" purposes of the variant
    require a validation.

       :return: the relative file paths of the proxy, default and render purposes
                if they are a valid geo layer, else it returns empty string(s)
    """
    proxy, default, render = ("", "", "")

    if variant.proxy:
        errorMsg = _validate_geo_layer(os.path.join(get_model_dir(), variant.proxy))
        if not errorMsg:
            proxy = variant.proxy
        else:
            cmds.confirmDialog(
                title="Validation Error",
                message=f"The 'proxy' purpose is not valid. {errorMsg}",
                button=["Continue"],
                dismissString="No",
            )

    if variant.default:
        errorMsg = _validate_geo_layer(os.path.join(get_model_dir(), variant.default))
        if not errorMsg:
            default = variant.default
        else:
            cmds.confirmDialog(
                title="Validation Error",
                message=f"The 'default' purpose is not valid. {errorMsg}",
                button=["Continue"],
                dismissString="No",
            )

    if variant.render:
        errorMsg = _validate_geo_layer(os.path.join(get_model_dir(), variant.render))
        if not errorMsg:
            render = variant.render
        else:
            cmds.confirmDialog(
                title="Validation Error",
                message=f"The 'render' purpose is not valid. {errorMsg}",
                button=["Continue"],
                dismissString="No",
            )

    return proxy, default, render


def _create_all_model_variant_compounds(graph: str, geoDirData: dict) -> str:
    variantNames = []

    for variantName in geoDirData:
        proxy, default, render = _get_validated_purposes_path(geoDirData[variantName])

        variantNames.append(variantName)
        _create_model_variant_compound(
            graph,
            variantName,
            geoDirData[variantName].guide,
            proxy,
            default,
            render,
        )

    # if there is a render purpose, we display it, overwise the contextual
    # menu to assign material will not get the correct prim path as by default
    # Maya USD shows the proxy representation)
    if geoDirData[variantName].render:
        purpose.showRenderPurposes()

    if variantNames:
        return sorted(variantNames)[0]

    graphAPI.connect_to_fanin_port(
        graph, "create_usd_component", "model_variants", "model_variant"
    )

    return ""


def _create_model_variant_compound(
    graph: str,
    variant_name: str,
    guide_file: str = "",
    proxy_file: str = "",
    default_file: str = "",
    render_file: str = "",
    connect=True,
) -> None:
    defineModelVariant = graphAPI.add_node(kModelVariantCompound)

    if connect:
        graphAPI.connect_to_fanin_port(
            defineModelVariant,
            "create_usd_component",
            "model_variants",
            "model_variant",
        )

    graphAPI.set_param(defineModelVariant, ("geo_variant_name", variant_name))
    graphAPI.set_param(defineModelVariant, ("guide_geo_file", guide_file))
    graphAPI.set_param(defineModelVariant, ("proxy_geo_file", proxy_file))
    graphAPI.set_param(defineModelVariant, ("default_geo_file", default_file))
    graphAPI.set_param(defineModelVariant, ("render_geo_file", render_file))


def component_creator(picked_dirs=None, version="1") -> None:
    if not picked_dirs:
        return

    load_plugin_if_needed("bifrostGraph")

    # "mayaUsdSelectKindSubComponent" not available in batch mode
    if not cmds.about(batch=1):
        # Material assignement should not happen on the root prim of the model
        # [TODO]: block kind selection when in Component Creator mode.
        cmds.mayaUsdSelectKindSubComponent()

    if isinstance(picked_dirs, str):
        modelDir = picked_dirs
    else:
        modelDir = picked_dirs[0]

    graph, geoDirData = _create_component_graph(modelDir, version)
    if not graph:
        return

    defaultGeoVariant = _create_all_model_variant_compounds(graph, geoDirData)
    if defaultGeoVariant:
        set_default_model_variant(defaultGeoVariant)
        set_default_look_variant("")

    _create_component_stage(graph)


def run_graph() -> int:
    return cmds.getAttr(f"{kGraphName}.stage")


def save() -> None:
    if hasComponentCreatorGraph() is False:
        cmds.warning("No Component Creator Graph found")
        return

    rtn = "No"
    if cmds.about(batch=1):
        rtn = "Yes"
    else:
        rtn = cmds.confirmDialog(
            title="USD Component",
            message="Save to disk?",
            button=["Yes", "No"],
            defaultButton="Yes",
            cancelButton="No",
            dismissString="No",
        )

    if rtn == "Yes":
        graphAPI.set_param("create_usd_component", ("save_model", "1"))
        run_graph()
        graphAPI.set_param("create_usd_component", ("save_model", "0"))

        # Save the material library
        matLibPath = material_library_file()
        if matLibPath:
            stage = Usd.Stage.Open(material_library_file())
            stage.Save()

        if not cmds.about(batch=1):
            create_thumbnail()


def create_material_library(library_name: str = kDefaultMatLibFileName) -> None:
    if hasComponentCreatorGraph() is False:
        cmds.warning("No Component Creator Graph found")
        return

    if not default_look_variant():
        add_look("default")

    rootDirectoryPath = sub_directory()

    if os.path.isdir(rootDirectoryPath):
        modelName = name()

        if model_version():
            matLibPath = os.path.join(
                rootDirectoryPath,
                modelName,
                model_version(),
                material_scope_name(),
                library_name,
            )
        else:
            matLibPath = os.path.join(
                rootDirectoryPath,
                modelName,
                material_scope_name(),
                library_name,
            )

        library_exists = False
        if os.path.isfile(matLibPath):
            library_exists = True

        if not library_exists:
            stage = Usd.Stage.CreateNew(matLibPath)
            mtl = stage.DefinePrim(f"/{material_scope_name()}", "Scope")
            stage.SetDefaultPrim(mtl)
            stage.Save()

    set_material_library_file(matLibPath)
    _create_material_library_proxy_shape(matLibPath)


def add_new_material(openLookdevXEditor: bool = False) -> str:
    """Create a new material in the material library layer

    :return: The ufe path of the new material
    """
    if hasComponentCreatorGraph() is False:
        cmds.warning("No Component Creator Graph found")
        return ""

    load_plugin_if_needed("LookdevXMaya")

    if not default_look_variant():
        add_look("default")

    create_material_library()

    mtlPath = ufe.PathString.path(f"{kMatLibShapeFullName},/{material_scope_name()}")
    mtlSceneItem = ufe.Hierarchy.createItem(mtlPath)

    # Create a material.
    stageContextOps = ufe.ContextOps.contextOps(mtlSceneItem)
    stageContextOps.doOp(["Add New Prim", "Material"])

    # Get the UFE scene item of the material.
    materialItem = ufe.GlobalSelection.get().back()

    # Create a shader.
    shaderNodeDef = ufe.NodeDef.definition(
        materialItem.runTimeId(), "ND_standard_surface_surfaceshader"
    )
    shaderItem = shaderNodeDef.createNode(
        materialItem, ufe.PathComponent("standard_surface")
    )

    # Get the 'base_color' attribute of the shader and set it to a random colour.
    shaderAttributes = ufe.Attributes.attributes(shaderItem)
    baseColorAttribute = shaderAttributes.attribute("inputs:base_color")
    baseColorAttribute.set(
        ufe.Color3f(random.random(), random.random(), random.random())
    )

    # Get the 'out' attribute of the shader.
    shaderOutAttribute = shaderAttributes.attribute("outputs:out")

    # Get the 'surface' attribute of the material.
    materialAttributes = ufe.Attributes.attributes(materialItem)
    materialSurfaceAttribute = materialAttributes.attribute("outputs:surface")

    # Connect the shader to the material.
    connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(
        materialItem.runTimeId()
    )
    connectionHandler.connect(shaderOutAttribute, materialSurfaceAttribute)

    shapeAndItem = (
        f"{materialItem.path().segments[0]},{materialItem.path().segments[1]}"
    )
    if openLookdevXEditor:
        cmds.lookdevXGraph(tabName=shaderItem.nodeName(), graphNode=shapeAndItem)

    return shapeAndItem


def get_relative_geo_path(shape_and_prim: str) -> str:
    """Extract the prim path from the full path of the Maya selection and
    then get the path relative to the "geo" Scope prim.

    :param [shape_and_prim]: The full path to the geo prim in the Maya scene.
        Ex. "|componentCreatorStage|componentCreatorStageShape,/dino/render/mdl1/geo/legs"
    :return: The prim path relative to the "geo" Scope prim
    """
    shapeAndItemList = shape_and_prim.split(",")
    assert len(shapeAndItemList) == 2

    geoPath = shapeAndItemList[1]

    return get_path_in_scope(geoPath, geometry_scope_name())


def material_is_in_libary(material_name: str) -> bool:
    if not material_library_file():
        return False

    stage = Usd.Stage.Open(material_library_file())
    if UsdShade.Material.Get(stage, f"/{material_scope_name()}/{material_name}"):
        return True
    return False


def _create_material_if_needed(material_name: str) -> str:
    matName = None
    if material_name:
        matName = material_name
    else:
        shapeAndMat = None
        shapeAndMat = add_new_material()
        assert len(shapeAndMat.split(",")) == 2
        matPath = shapeAndMat.split(",")[1]

        beforeAndAfterMtlScope = matPath.split(f"/{material_scope_name()}/")
        assert len(beforeAndAfterMtlScope) == 2
        matName = beforeAndAfterMtlScope[1]

    return matName


def _get_or_create_pathexpr_node(rel_prim_path: str) -> str:
    pathExprNode = None
    for node in graphAPI.find_nodes(kPathExpressionCompound):
        if graphAPI.param(node, "prim_path") == rel_prim_path:
            pathExprNode = node

    if not pathExprNode:
        pathExprNode = graphAPI.add_node(kPathExpressionCompound)
        assert pathExprNode, "Can not add new create_path_expression node!"
        graphAPI.set_param(pathExprNode, ("prim_path", rel_prim_path))

    return pathExprNode


def _add_bind_node() -> str:
    """Create a "define_usd_material_binding" compound connected to the
    default look variant in the default model variant.

    :return: the name of the new bind node
    """
    bindNode = graphAPI.add_node(kMaterialBindingCompound)

    for node in get_look_variant_nodes(default_model_variant()):
        if graphAPI.param(node, "variant_name") == default_look_variant():
            graphAPI.connect_to_fanin_port(
                bindNode,
                node,
                "material_bindings",
                "material_binding",
            )
            break

    return bindNode


def _find_bind_node_from_material(material_name: str) -> str:
    """Find the define_usd_material_binding compound using this material name
    in the current look variant.

    :return: the name of the bind node
    """
    for node in current_binding_nodes():
        if graphAPI.param(node, "material") == material_name:
            return node

    return ""


def _find_bind_node_from_geo_path(rel_geo_path: str) -> tuple[str, str]:
    bindNode, bindNodePort = ("", "")
    for pathExprNode in graphAPI.find_nodes(kPathExpressionCompound):
        if graphAPI.param(pathExprNode, "prim_path") == rel_geo_path:
            for output in graphAPI.connexions(pathExprNode, "output"):
                tokens = output.split(".")
                connectedNode = tokens[0]
                if connectedNode not in current_binding_nodes():
                    continue
                bindNode = connectedNode
                bindNodePort = tokens[2]

    return bindNode, bindNodePort


def scene_item_to_shape_and_prim(sceneItem) -> str:
    """Format a Ufe SceneItem path into a shape and prim path.
    SceneItem path:      |world|componentCreatorStage|componentCreatorStageShape/Chair/proxy/classic/geo/mesh
    shape and prim path: |componentCreatorStage|componentCreatorStageShape,/Chair/proxy/classic/geo/mesh
    """
    if len((parts := str(sceneItem.path()).split(kComponentStageShape))) > 1:
        parts[0] = (parts[0].split("|world"))[-1] + kComponentStageShape
        return ",".join(parts)

    return ""


def assign_new_material_to_selection() -> str:
    """Assign material to the meshes and curves found in the selection"""
    materialName = ""
    shapeAndPrimList = []
    for sceneItem in ufe.GlobalSelection.get():
        if sceneItem.nodeType() == "Mesh" or sceneItem.nodeType() == "BasisCurves":
            shapeAndPrimList.append(scene_item_to_shape_and_prim(sceneItem))

    if shapeAndPrimList:
        materialName = assign_material(shapeAndPrimList.pop())

        for shapeAndPrim in shapeAndPrimList:
            assign_material(shapeAndPrim, materialName)
    else:
        materialName = add_new_material()

    return materialName


def unassign_material_from_selection():
    for sceneItem in ufe.GlobalSelection.get():
        unassign_material(scene_item_to_shape_and_prim(sceneItem))


def assign_material(
    shape_and_prim: str, material_name: str = "", expression=False
) -> str:
    """Assign a material to a geo in the current variants context.

        The material is found bellow the materials scope (ex. "/Model/render/model1/mtl/Material1")
        The geo is found bellow the geometries scope     (ex. "/Model/render/model1/geo/Mesh1").

    If no material name is provided, a new material will be automatically created.

    If it is the first time the material is assigned, it will create the following chain of nodes:
    "path_expression" -> "define_usd_material_binding" -> "define_usd_look_variant"

    If no default look exists, it will create a new one named "default".
    It will connect the "define_usd_look_variant" to the default look of the default model variant.

    If a path_expression node is already connected to a binding node set with same geo and material names, no nodes will
    be added.
    If there is a path_expression node with same geo but unconnected, it will connect it the the binding node if it is found
    or it will create a new one.

    If a new material binding is created for a geo already connected to a binding node using a different material,
    the binding node will update its material to the new one, only if the path_expression node is the only downstream connexion.
    Overwise a new binding node will be created and the path_expression node will change its connection to the new one.

    :param [shape_and_prim]: The full path to the geo prim in the Maya scene.
        Ex. "|componentCreatorStage|componentCreatorStageShape,/dino/render/mdl1/geo/legs"
    :param [material_name]: The name of the material in the component. Ex. "Material1"
        If no material name is passed, a new material will be used.
    :return: the name of the material.
    """
    load_plugin_if_needed("LookdevXMaya")

    # Unassign from expression is not supported yet.

    matName = ""

    if expression:
        cmds.warning(
            "The expression base Material Binding is experimental!"
            " Matching geometries with exisiting material won't be unassigned"
        )
    if not expression:
        unassign_material(shape_and_prim)
        # if the geo prim is the geo Scope and it is not an expression, assign to direct children.
        if shape_and_prim.endswith(f"/{geometry_scope_name()}"):
            matName = material_name
            for childName in get_geo_children():
                matName = assign_material(shape_and_prim + "/" + childName, matName)

            return matName

    modelVariantNode = current_model_variant_node()
    if not modelVariantNode:
        cmds.warning("Please add a Model Variant first")
        return ""

    if material_name and not material_is_in_libary(material_name):
        cmds.warning(
            f"No material '{material_name}' found in '{material_library_file()}'"
        )
        return ""

    # if there is no material library yet and we want to assign a new material
    # create a default material lib
    if not material_name:
        create_material_library()

    # Create a "default" look if needed
    if not default_look_variant():
        add_look("default")

    matName = _create_material_if_needed(material_name)

    bindNode = _find_bind_node_from_material(matName)

    relPrimPath = ""
    if expression:
        relPrimPath = shape_and_prim
    else:
        relPrimPath = get_relative_geo_path(shape_and_prim)

    prevBindNode, prevBindNodePort = _find_bind_node_from_geo_path(relPrimPath)

    # check if an exisiting bind node is connected to only one path_expression node
    # and that its prim_path value matches our relPrimPath
    allPathExprNodes = graphAPI.find_nodes(kPathExpressionCompound)
    if not bindNode:
        for pathExprNode in allPathExprNodes:
            if graphAPI.param(pathExprNode, "prim_path") != relPrimPath:
                continue

            outputs = graphAPI.connexions(pathExprNode, "output")
            if len(outputs) == 1:
                tokens = outputs[0].split(".")
                connectedNode = tokens[0]
                if connectedNode not in current_binding_nodes():
                    continue
                if graphAPI.type_name(connectedNode) == kMaterialBindingCompound:
                    portChildren = graphAPI.port_children(connectedNode, "prim_paths")
                    # remove old connection
                    inputPort = tokens[0] + "." + "prim_paths" + "." + tokens[2]
                    graphAPI.disconnect(f"{pathExprNode}.output", f"{inputPort}")
                    if len(portChildren) == 1:
                        bindNode = connectedNode
                        graphAPI.set_param(bindNode, ("material", matName))

    if not bindNode:
        bindNode = _add_bind_node()

    for port in get_port_children(bindNode, "prim_paths", style="node_parentport_port"):
        for pathExprNode in allPathExprNodes:
            for cnx in graphAPI.connexions(pathExprNode, "output"):
                if cnx == port:
                    if graphAPI.param(pathExprNode, "prim_path") == relPrimPath:
                        return matName

    pathExpNode = _get_or_create_pathexpr_node(relPrimPath)
    graphAPI.connect_to_fanin_port(pathExpNode, bindNode, "prim_paths", "output")

    if prevBindNode:
        graphAPI.disconnect(
            f"{pathExpNode}.output", f"{prevBindNode}.prim_paths.{prevBindNodePort}"
        )

    graphAPI.set_param(pathExpNode, ("prim_path", relPrimPath))
    graphAPI.set_param(bindNode, ("material", matName))

    return matName


def get_current_geo_path_nodes() -> list[str]:
    """Return the list of relative geo paths in current variants context"""

    geoPathNodes = []
    for bindNode in current_binding_nodes():
        for port in graphAPI.port_children(bindNode, "prim_paths"):
            inPorts = graphAPI.connexions(bindNode, f"prim_paths.{port}")
            if inPorts:
                inNode = inPorts[0].split(".")[0]
            geoPathNodes.append(inNode)

    return geoPathNodes


def current_binding_node_from_geo_path(rel_geo_path: str) -> str:
    """Return the define_usd_binding node connected to this geo"""
    rtn = ""
    for bindNode in current_binding_nodes():
        for port in graphAPI.port_children(bindNode, "prim_paths"):
            inPorts = graphAPI.connexions(bindNode, f"prim_paths.{port}")
            if inPorts:
                geoPathNode = inPorts[0].split(".")[0]
                if graphAPI.param(geoPathNode, "prim_path") == rel_geo_path:
                    rtn = bindNode
                    break

    return rtn


def unassign_material(shape_and_prim: str) -> None:
    if graph_api.bifrost_version().startswith("2.8.0.0"):
        unassign_material_2_8(shape_and_prim)
    else:
        unassign_material_2_10(shape_and_prim)


def unassign_material_2_8(shape_and_prim: str) -> None:
    # If the prim path is the geo scope, unassign material on children
    if shape_and_prim.endswith(f"/{geometry_scope_name()}"):
        for childName in get_geo_children():
            unassign_material(shape_and_prim + "/" + childName)

        return

    for pathExprNode in graphAPI.find_nodes(kPathExpressionCompound):
        if get_relative_geo_path(shape_and_prim) == graphAPI.param(
            pathExprNode, "prim_path"
        ):
            pathExprOutputList = graphAPI.connexions(pathExprNode, "output")
            outputCntInCurrentLook = 0
            bindNodePrimPathsCnt = -1
            for pathExprOutput in pathExprOutputList:
                tokens = pathExprOutput.split(".")
                bindNode = tokens[0]
                if bindNode in current_binding_nodes():
                    if graph_api.bifrost_version().startswith("2.8.0.0"):
                        inputPort = bindNode + "." + "prim_paths" + "." + tokens[1]
                    else:
                        cmds.error(
                            "Replace 'unassign_material' function by 'unassign_material_BIFROST_9048'"
                        )

                    graphAPI.disconnect(f"{pathExprNode}.output", f"{inputPort}")
                    outputCntInCurrentLook += 1
                    bindNodePrimPathsCnt = len(
                        graphAPI.port_children(bindNode, "prim_paths")
                    )

                if bindNodePrimPathsCnt == 0:
                    graphAPI.remove_node(bindNode)

            if len(pathExprOutputList) < 2 and outputCntInCurrentLook == 1:
                graphAPI.remove_node(pathExprNode)


# [BIFROST-9048]: Once this fix is publicly available,
# replace unassign_material by this one:
def unassign_material_2_10(shape_and_prim: str) -> None:
    # If the prim path is the geo scope, unassign material on children
    if shape_and_prim.endswith(f"/{geometry_scope_name()}"):
        for childName in get_geo_children():
            unassign_material(shape_and_prim + "/" + childName)

        return

    relGeoPath = get_relative_geo_path(shape_and_prim)
    bindNode = current_binding_node_from_geo_path(relGeoPath)
    if not bindNode:
        return

    geoPathNode = None
    for port in graphAPI.port_children(bindNode, "prim_paths"):
        inPortList = graphAPI.connexions(bindNode, f"prim_paths.{port}")
        for inPort in inPortList:
            currentGeoPathNode = inPort.split(".")[0]
            if graphAPI.param(currentGeoPathNode, "prim_path") == relGeoPath:
                geoPathNode = currentGeoPathNode
                graphAPI.disconnect(inPort, f"{bindNode}.prim_paths.{port}")
                break

    # Cleanup dangling nodes.
    if geoPathNode:
        if len(graphAPI.connexions(geoPathNode, "output")) == 0:
            graphAPI.remove_node(geoPathNode)

        inPortList = []
        for port in graphAPI.port_children(bindNode, "prim_paths"):
            inPortList = graphAPI.connexions(bindNode, f"prim_paths.{port}")

        if len(inPortList) == 0:
            graphAPI.remove_node(bindNode)


def get_assigned_prim(shape_and_prim: str) -> str:
    primPath = get_relative_geo_path(shape_and_prim)

    # if prim is not direclty assigned, maybe its parent is.
    tokens = primPath.split("/")
    while tokens:
        primPath = "/".join(tokens)
        for pathExprNode in graphAPI.find_nodes(kPathExpressionCompound):
            if graphAPI.param(pathExprNode, "prim_path") == primPath:
                for output in graphAPI.connexions(pathExprNode, "output"):
                    parts = output.split(".")
                    bindNode = parts[0]
                    if bindNode in current_binding_nodes():
                        return primPath

        tokens.pop()

    return ""


def get_material_from_prim(shape_and_prim: str) -> str:
    primPath = get_assigned_prim(shape_and_prim)

    for pathExprNode in graphAPI.find_nodes(kPathExpressionCompound):
        if graphAPI.param(pathExprNode, "prim_path") == primPath:
            for output in graphAPI.connexions(pathExprNode, "output"):
                tokens = output.split(".")
                bindNode = tokens[0]
                if bindNode in current_binding_nodes():
                    return graphAPI.param(bindNode, "material")

    return ""


def open_material_from_prim_in_lookdevx(shape_and_prim: str) -> None:
    matName = get_material_from_prim(shape_and_prim)
    if matName:
        shapeAndItem = f"{kMatLibShapeFullName},/{material_scope_name()}/{matName}"
        cmds.lookdevXGraph(tabName=matName, graphNode=shapeAndItem)


def get_all_materials(nameOnly: bool = False) -> list[str]:
    if not material_library_file():
        return []

    materialPaths = []
    matLibPath = material_library_file()
    stage = Usd.Stage.Open(matLibPath)

    materialsStageShape = None
    for shape in cmds.ls(type=kMayaUsdProxyShape, long=True):
        proxyShapeStage = mayaUsdLib.GetPrim(shape).GetStage()

        if proxyShapeStage.GetRootLayer().identifier == matLibPath:
            materialsStageShape = shape

    if materialsStageShape:
        mtlPrim = stage.GetDefaultPrim()
        if mtlPrim.GetChildren():
            for prim in mtlPrim.GetChildren():
                if prim.GetTypeName() == "Material":
                    if nameOnly:
                        beforeAndAfterMtlScope = prim.GetPath().pathString.split(
                            f"/{material_scope_name()}/"
                        )
                        assert len(beforeAndAfterMtlScope) == 2
                        materialPaths.append(f"{beforeAndAfterMtlScope[1]}")
                    else:
                        materialPaths.append(
                            f"{materialsStageShape},{prim.GetPath().pathString}"
                        )

    return materialPaths


def open_materials_in_lookdevx() -> None:
    create_material_library()

    for materialPath in get_all_materials():
        cmds.lookdevXGraph(tabName="UsdComponentMaterials", graphNode=materialPath)


def add_arnold_node(shape_and_prim: str) -> None:
    """Add a "define_arnold_usd_mesh_primvars" compound to the prim in default
    model and look variant.
    """
    modelVariantNode = current_model_variant_node()
    if not modelVariantNode:
        cmds.warning("Please add a Model Variant first")
        return

    # Create a "default" look if needed
    if not default_look_variant():
        add_look("default")

    relPrimPath = get_relative_geo_path(shape_and_prim)

    allPathExprNodes = graphAPI.find_nodes(kPathExpressionCompound)
    # if there is already an arnold node connected to this prim do nothing
    for pathExprNode in allPathExprNodes:
        if graphAPI.param(pathExprNode, "prim_path") == relPrimPath:
            for output in graphAPI.connexions(pathExprNode, "output"):
                tokens = output.split(".")
                connectedNode = tokens[0]
                if connectedNode in current_arnold_nodes():
                    return

    arnoldNode = graphAPI.add_node(kArnoldMeshPrimvarsCompound)
    for lookVariantNode in get_look_variant_nodes(default_model_variant()):
        if graphAPI.param(lookVariantNode, "variant_name") == default_look_variant():
            graphAPI.connect_to_fanin_port(
                arnoldNode,
                lookVariantNode,
                "primvar_definitions_array",
                "primvar_definitions",
            )

            pathExprNode = _get_or_create_pathexpr_node(relPrimPath)
            graphAPI.connect_to_fanin_port(
                pathExprNode, arnoldNode, "prim_paths", "output"
            )
            break


def remove_arnold_node(shape_and_prim: str) -> None:
    for pathExprNode in graphAPI.find_nodes(kPathExpressionCompound):
        if get_relative_geo_path(shape_and_prim) == graphAPI.param(
            pathExprNode, "prim_path"
        ):
            for output in graphAPI.connexions(pathExprNode, "output"):
                tokens = output.split(".")
                arnoldNode = tokens[0]
                if arnoldNode not in current_arnold_nodes():
                    continue

                inputPort = arnoldNode + "." + "prim_paths" + "." + tokens[1]
                portCnts = len(graphAPI.port_children(arnoldNode, "prim_paths"))
                if portCnts == 1:
                    graphAPI.remove_node(arnoldNode)
                else:
                    graphAPI.disconnect(f"{pathExprNode}.output", f"{inputPort}")

                if not graphAPI.connexions(pathExprNode):
                    graphAPI.remove_node(pathExprNode)


def open_bifrost_usd_component_graph() -> None:
    if cmds.ls(kGraphName):
        graph_api.open_bifrost_graph(kGraphName)
    else:
        cmds.error("No Bifrost Component Creator Graph found in this scene")


def get_model_variant_nodes() -> list[str]:
    """get all the Bifrost "define_usd_model_variant" compounds
    used by the "create_usd_component" compound.

    :return: the names of the "define_usd_model_variant" compounds.
    """

    modelVariantNodes = []

    # [BIFROST-9047]: Once able to use the "listConnectedNodes" flag
    # on fan-in ports, this logic won't be needed anymore. Instead we will
    # be able to search for every nodes connected to the "model_variants" fan-in port
    # of the create_usd_component compound.
    if graph_api.bifrost_version().startswith("2.8.0.0"):
        for node in graphAPI.find_nodes(kModelVariantCompound):
            cnx = graphAPI.connexions(node, "model_variant")
            if cnx and cnx[0].split(".")[0] == "create_usd_component":
                modelVariantNodes.append(node)

    else:
        createComponentCompound = get_create_usd_component_node()
        if createComponentCompound:
            for port in graphAPI.port_children(
                createComponentCompound, "model_variants"
            ):
                inNodes = graphAPI.connexions(
                    createComponentCompound, f"model_variants.{port}"
                )
                modelVariantNodes.append(inNodes[0].split(".")[0])

    return modelVariantNodes


def get_look_variant_nodes(model_variant: str = "") -> list[str]:
    """Return every instances of the "define_usd_look_variant"
    that are connected to a downstream node.

    [TODO] To validate that the node is really used by the graph,
    we should check that the full chain looks like that:
    "define_usd_look_variant" -> "define_usd_model_variant" -> "create_usd_component" -> "output"
    """
    lookVariantNodes = []

    for node in graphAPI.find_nodes(kLookVariantCompound):
        lookVariantCnx = graphAPI.connexions(node, "look_variant")
        for cnx in lookVariantCnx:
            connectedNode = cnx.split(".")[0]
            typeName = graphAPI.type_name(connectedNode)
            if typeName == kModelVariantCompound:
                if model_variant:
                    if model_variant == graphAPI.param(
                        connectedNode, "geo_variant_name"
                    ):
                        lookVariantNodes.append(node)
                        break
                else:
                    lookVariantNodes.append(node)
                    break

    return lookVariantNodes


def get_binding_nodes(variants: tuple = ("", "")) -> list[str]:
    model_variant = variants[0]
    look_variant = variants[1]

    lookVariantNodes = []
    if model_variant:
        lookVariantNodes = get_look_variant_nodes(model_variant)

    bindingNodes = []
    for node in graphAPI.find_nodes(kMaterialBindingCompound):
        if (model_variant == "") and (look_variant == ""):
            # get every binding nodes in the graph
            bindingNodes.append(node)
        else:
            for cnx in graphAPI.connexions(node, "material_binding"):
                connectedNode = cnx.split(".")[0]
                typeName = graphAPI.type_name(connectedNode)
                if typeName == kLookVariantCompound:
                    if look_variant and (connectedNode in lookVariantNodes):
                        if look_variant == graphAPI.param(
                            connectedNode, "variant_name"
                        ):
                            bindingNodes.append(node)
                            break

    return bindingNodes


def current_binding_nodes() -> list[str]:
    variants = (default_model_variant(), default_look_variant())
    return get_binding_nodes(variants)


def get_arnold_nodes(variants: tuple = ("", "")) -> list[str]:
    model_variant = variants[0]
    look_variant = variants[1]

    lookVariantNodes = []
    if model_variant:
        lookVariantNodes = get_look_variant_nodes(model_variant)

    arnoldNodes = []
    for node in graphAPI.find_nodes(kArnoldMeshPrimvarsCompound):
        if (model_variant == "") and (look_variant == ""):
            # get every arnold nodes in the graph
            arnoldNodes.append(node)
        else:
            for cnx in graphAPI.connexions(node, "primvar_definitions"):
                connectedNode = cnx.split(".")[0]
                typeName = graphAPI.type_name(connectedNode)
                if typeName == kLookVariantCompound:
                    if look_variant and (connectedNode in lookVariantNodes):
                        if look_variant == graphAPI.param(
                            connectedNode, "variant_name"
                        ):
                            arnoldNodes.append(node)
                            break

    return arnoldNodes


def current_arnold_nodes() -> list[str]:
    variants = (default_model_variant(), default_look_variant())
    return get_arnold_nodes(variants)


def current_model_variant_node() -> str:
    nodes = get_model_variant_nodes()
    modelVariantNode = ""
    for node in nodes:
        if graphAPI.param(node, "geo_variant_name") == default_model_variant():
            modelVariantNode = node
            break

    return modelVariantNode


def add_model(
    name: str,
    guide_geo_file: str = "",
    proxy_geo_file: str = "",
    default_geo_file: str = "",
    render_geo_file: str = "",
) -> None:
    name = name.replace(" ", "_")

    modelVariantNodes = graphAPI.find_nodes(kModelVariantCompound)
    for node in modelVariantNodes:
        if graphAPI.param(node, "geo_variant_name") == name:
            return

    modelVariant = graphAPI.add_node(kModelVariantCompound)
    graphAPI.connect_to_fanin_port(
        modelVariant, "create_usd_component", "model_variants", "model_variant"
    )

    graphAPI.set_param(modelVariant, ("geo_variant_name", name))
    graphAPI.set_param(modelVariant, ("guide_geo_file", guide_geo_file))
    graphAPI.set_param(modelVariant, ("proxy_geo_file", proxy_geo_file))
    graphAPI.set_param(modelVariant, ("default_geo_file", default_geo_file))
    graphAPI.set_param(modelVariant, ("render_geo_file", render_geo_file))

    set_default_model_variant(name)


def rename_default_model_variant(new_name: str) -> None:
    oldName = default_model_variant()
    newName = new_name.replace(" ", "_")

    for node in graphAPI.find_nodes(kModelVariantCompound):
        if graphAPI.param(node, "geo_variant_name") == oldName:
            graphAPI.set_param(node, ("geo_variant_name", newName))
            set_default_model_variant(newName)
            break


def add_look(name: str) -> None:
    validName = name.replace(" ", "_")
    modelVariantNode = current_model_variant_node()

    if modelVariantNode:
        lookVariant = graphAPI.add_node(kLookVariantCompound)
        graphAPI.connect_to_fanin_port(
            lookVariant, modelVariantNode, "look_variants", "look_variant"
        )

        graphAPI.set_param(lookVariant, ("variant_name", validName))

        set_default_look_variant(validName)


def rename_default_look_variant(new_name: str) -> None:
    oldName = default_look_variant()
    newName = new_name.replace(" ", "_")

    for node in graphAPI.find_nodes(kLookVariantCompound):
        if graphAPI.param(node, "variant_name") == oldName:
            graphAPI.set_param(node, ("variant_name", newName))
            set_default_look_variant(newName)


def get_look_variant_names() -> list[str]:
    names = []
    for node in get_look_variant_nodes():
        name = graphAPI.param(node, "variant_name")
        if name and name not in names:
            names.append(name)

    return names


def get_model_variant_names() -> list[str]:
    names = []
    for node in get_model_variant_nodes():
        name = graphAPI.param(node, "geo_variant_name")
        if name and name not in names:
            names.append(name)

    return names


def get_geo_children() -> list[str]:
    stageShape = cmds.ls(kComponentStageShapeFullName, type=kMayaUsdProxyShape)
    if not stageShape:
        cmds.warning("No Component Stage found")

    stage = mayaUsdLib.GetPrim(stageShape[0]).GetStage()
    defaultPrim = stage.GetDefaultPrim()

    renderScope = stage.GetPrimAtPath(defaultPrim.GetPath().AppendChild("render"))
    if not renderScope:
        renderScope = stage.GetPrimAtPath(defaultPrim.GetPath().AppendChild("default"))

    modelVariant = stage.GetPrimAtPath(
        renderScope.GetPath().AppendChild(default_model_variant())
    )
    geoScope = stage.GetPrimAtPath(
        modelVariant.GetPath().AppendChild(geometry_scope_name())
    )

    return geoScope.GetChildrenNames()


def copy_look(
    model_variant: str,
    look_variant: str,
    target_model_variant: str,
    target_look_variant: str,
) -> bool:
    nodesToCopy = []
    bindingNodes = get_binding_nodes((model_variant, look_variant))

    targetModelVariantNode = None
    for node in get_model_variant_nodes():
        if graphAPI.param(node, "geo_variant_name") == target_model_variant:
            targetModelVariantNode = node

    assert (
        targetModelVariantNode
    ), f"Could not find Model variant {target_model_variant}"

    targetLookVariant = None
    for node in get_look_variant_nodes(target_model_variant):
        if graphAPI.param(node, "variant_name") == target_look_variant:
            targetLookVariant = node

    if not targetLookVariant:
        cmds.warning(f"Could not find Look Variant '{target_look_variant}' in Model Variant '{target_model_variant}'")
        return False

    pathExprNodes = []
    for pathExprNode in graphAPI.find_nodes(kPathExpressionCompound):
        for output in graphAPI.connexions(pathExprNode, "output"):
            tokens = output.split(".")
            connectedNode = tokens[0]
            if connectedNode in bindingNodes:
                pathExprNodes.append(pathExprNode)

    nodesToCopy = bindingNodes + pathExprNodes
    graphAPI.copy(nodesToCopy)

    allNodesBefore = graphAPI.find_nodes()
    graphAPI.paste()
    allNodesAfter = graphAPI.find_nodes()

    setDif = set(allNodesBefore).symmetric_difference(set(allNodesAfter))
    newNodes = list(setDif)

    for node in newNodes:
        if graphAPI.type_name(node) == kMaterialBindingCompound:
            graphAPI.connect_to_fanin_port(
                node, targetLookVariant, "material_bindings", "material_binding"
            )

    return True


def append_path_to_geo_scope_full_path(
    shape_and_prim: str, child_relative_path: str
) -> str:
    tokens = shape_and_prim.split(geometry_scope_name())
    assert (
        len(tokens) > 1
    ), f"Can't find geometry scope '{geometry_scope_name()}' in '{shape_and_prim}'"
    geoPath = f"{tokens[0]}{geometry_scope_name()}"
    return f"{geoPath}/{child_relative_path}"


def _validate_geo_layer(layer_path: str) -> str:
    """Check that the USD file storing a modeling geo includes
    a default prim and a geo prim bellow it.
    """
    stage = Usd.Stage.Open(layer_path)
    defaultPrim = stage.GetDefaultPrim()
    if not defaultPrim:
        msg = f"Missing default prim in geo layer {layer_path}"
        return msg

    geoPath = defaultPrim.GetPath().AppendChild(geometry_scope_name())
    geoPrim = stage.GetPrimAtPath(geoPath)
    if not geoPrim:
        msg = f"Missing '{geometry_scope_name()}' prim in geo layer {layer_path}"
        return msg

    return ""


def duplicate_selected_material() -> None:
    if hasComponentCreatorGraph() is False:
        cmds.warning("No Component Creator Graph found")
        return

    materialItem = ufe.GlobalSelection.get().back()
    if not materialItem:
        return

    srcSdfPath = str(materialItem.path().segments[1])
    srcProxyShape = str(materialItem.path().segments[0])
    srcProxyShape = srcProxyShape[6:]
    srcStage = mayaUsdLib.GetPrim(srcProxyShape).GetStage()
    if not srcStage:
        return

    srcMaterialPrim = srcStage.GetPrimAtPath(srcSdfPath)
    if srcMaterialPrim.GetTypeName() != "Material":
        cmds.warning("No Material selected")
        return

    srcLayer = srcStage.GetRootLayer()

    dstStage = mayaUsdLib.GetPrim(kMatLibShapeFullName).GetStage()
    dstMaterialPrim = dstStage.GetPrimAtPath(srcSdfPath)
    dstSdfPath = srcSdfPath
    if dstMaterialPrim:
        cmds.warning("Material with same name already exist")
        # TODO: check if path exists
        dstSdfPath = dstSdfPath + "_COPY"

    Sdf.CopySpec(srcLayer, srcSdfPath, dstStage.GetRootLayer(), dstSdfPath)


def create_thumbnail():
    tmpImageDir = tempfile.mkdtemp()
    imageFilePath = os.path.join(tmpImageDir, "thumbnail")

    cmds.playblast(
        startTime=1,
        endTime=1,
        format="image",
        filename=imageFilePath,
        sequenceTime=0,
        clearCache=1,
        viewer=1,
        showOrnaments=1,
        fp=4,
        percent=50,
        compression="png",
        quality=70,
    )

    if os.path.isfile(f"{imageFilePath}.0001.png"):
        shutil.copyfile(
            f"{imageFilePath}.0001.png", os.path.join(get_model_dir(), "thumbnail.png")
        )

    shutil.rmtree(tmpImageDir)


def get_variants_text() -> str:
    variants = ""
    if hasComponentCreatorGraph():
        variants = (
            "Model: " + default_model_variant() + " Look: " + default_look_variant()
        )
    return variants


def set_heads_up_display():
    # remove previous Variants HUD
    if cmds.headsUpDisplay(kHUD_UsdComponentVariants, query=True, exists=True):
        cmds.headsUpDisplay(kHUD_UsdComponentVariants, rem=True)

    cmds.headsUpDisplay(
        kHUD_UsdComponentVariants,
        section=3,
        block=0,
        blockSize="medium",
        label="Current Variants: ",
        labelFontSize="small",
        command=get_variants_text,
        event="idle",
    )


if __name__ == "__main__":
    # cmds.file(f=True, new=True)
    pass
