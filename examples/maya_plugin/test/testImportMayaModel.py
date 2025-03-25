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
import unittest
from unittest.mock import patch

import ufe
from maya import cmds
from maya import standalone

from bifrost_usd.constants import kGraphName, kDefinePrimHierarchy
from bifrost_usd import author_usd_graph
from bifrost_usd.author_usd_graph import graphAPI

from bifrost_usd.graph_api import GraphEditorSelection


def hasUfePythonBugFix() -> bool:
    """Tests that are renaming Maya DAG paths will fail if Maya version is less than 2025.2.
    This is related to ticket EMSUSD-1195."""
    return cmds.about(apiVersion=True) >= 20260200


def mock_get_graph_selection() -> GraphEditorSelection:
    """Used to patch "bifrost_usd.author_usd_graph.get_graph_selection" since the Bifrost Graph Editor can't be open in unitests.

    To be passed to the side_effect argument of a unittest.mock.Mock.

    :return: An hard coded selection instead of the one from the Bifrost Graph Editor.
    """
    return GraphEditorSelection(
        f"|{kGraphName}|{kGraphName}Shape",
        f"{kGraphName}Shape",
        "/",
        "",
        ["add_to_stage"],
    )


def _is_prim_type_match(ufe_selection: str, prim_type_to_match: str) -> bool:
    cmds.select(ufe_selection, replace=True)
    ufePath = ufe.PathString.path(ufe_selection[0])
    sceneItem = ufe.Hierarchy.createItem(ufePath)
    return sceneItem.nodeType() == prim_type_to_match


class ImportMayaModelTestCase(unittest.TestCase):
    plugins_loaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.plugins_loaded:
            standalone.initialize("bifrost-usd-import-maya-model")
            cmds.loadPlugin("mayaUsdPlugin", quiet=True)
            cmds.loadPlugin("bifrostGraph", quiet=True)
            cmds.loadPlugin("bifrostUSDExamples.py", quiet=True)
            cls.plugins_loaded = True

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        graphAPI.ufe_observer = False
        cmds.file(f=True, new=True)

        # create the bifrostUsd graph with an add_to_stage compound.
        cmds.bifrostUSDExamples(newStage=True, shape=True)
        cmds.bifrostUSDExamples(
            insertNode=True,
            nodeType="BifrostGraph,USD::Stage,add_to_stage",
            currentCompound="/",
            nodeSelection="create_usd_stage",
            portSelection="stage",
            inputPort="stage",
            outputPort="out_stage",
        )

        # create the Maya model
        cmds.polyCube()
        cmds.group("pCube1", name="geo")
        cmds.group("geo", name="Model")
        cmds.createNode("transform", name="other_geo")
        cmds.parent("other_geo", "Model")

        cmds.select("geo", replace=True)
        cmds.addAttr(longName="USD_typeName", dataType="string", keyable=False)
        cmds.setAttr("geo.USD_typeName", "Scope", type="string")

        cmds.select("Model", replace=True)
        graphAPI.ufe_observer = True

    @patch(
        "bifrost_usd.author_usd_graph.get_graph_selection",
        side_effect=mock_get_graph_selection,
    )
    def testImportModelUnderPseudoRoot(self, mock_get_graph_selection):
        self.assertTrue(author_usd_graph.add_maya_selection_to_stage())
        mock_get_graph_selection.assert_called_once_with()

        self.assertEqual(
            graphAPI.find_nodes(kDefinePrimHierarchy),
            ["define_usd_prim_hierarchy", "define_usd_prim_hierarchy1"],
        )

        self.assertEqual(
            graphAPI.connexions("define_usd_prim_hierarchy", "leaf_mesh"),
            ["bifrostUsdShape.mesh1"],
        )
        self.assertEqual(
            graphAPI.connexions("define_usd_prim_hierarchy", "prim_definitions"),
            ["add_to_stage.prim_definitions.mesh1"],
        )

        self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/Model/geo/pCube1")

        # Check the prims type
        self.assertTrue(
            _is_prim_type_match(["|mayaUsdProxy1|mayaUsdProxyShape1,/Model"], "Xform")
        )
        self.assertTrue(
            _is_prim_type_match(
                ["|mayaUsdProxy1|mayaUsdProxyShape1,/Model/geo"], "Scope"
            )
        )
        self.assertTrue(
            _is_prim_type_match(
                ["|mayaUsdProxy1|mayaUsdProxyShape1,/Model/geo/pCube1"], "Mesh"
            )
        )

        if hasUfePythonBugFix():
            # Check prim path after renaming Maya model
            cmds.rename("Model", "CUBE")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/CUBE/geo/pCube1")

            cmds.rename("geo", "GEOMETRIES")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/CUBE/GEOMETRIES/pCube1")

            cmds.rename("pCube1", "SHAPE")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/CUBE/GEOMETRIES/SHAPE")

            # check that compound is deleted when Maya node is delete
            cmds.delete("SHAPE")
            self.assertEqual(
                graphAPI.find_nodes(kDefinePrimHierarchy), ["define_usd_prim_hierarchy1"]
            )

    @patch(
        "bifrost_usd.author_usd_graph.get_graph_selection",
        side_effect=mock_get_graph_selection,
    )
    def testGroupMesh(self, side_effect):
        if hasUfePythonBugFix():
            author_usd_graph.add_maya_selection_to_stage()
            cmds.group("pCube1", name="NEW_GROUP")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/Model/geo/NEW_GROUP/pCube1")

    @patch(
        "bifrost_usd.author_usd_graph.get_graph_selection",
        side_effect=mock_get_graph_selection,
    )
    def testGroupGeo(self, side_effect):
        if hasUfePythonBugFix():
            author_usd_graph.add_maya_selection_to_stage()

            cmds.parent("pCube1", "other_geo")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/Model/other_geo/pCube1")

            cmds.parent("pCube1", "geo")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/Model/geo/pCube1")

    @patch(
        "bifrost_usd.author_usd_graph.get_graph_selection",
        side_effect=mock_get_graph_selection,
    )
    def testImportModelUnderGeoPrim(self, side_effect):
        cmds.vnnNode(
            f"{kGraphName}Shape",
            "/add_to_stage",
            setPortDefaultValues=("parent_path", "/Cube/geo"),
        )
        self.assertTrue(author_usd_graph.add_maya_selection_to_stage())
        primHierarchyCompounds = graphAPI.find_nodes(kDefinePrimHierarchy)
        self.assertEqual(
            primHierarchyCompounds, ["define_usd_prim_hierarchy", "define_usd_prim_hierarchy1"]
        )

        self.assertEqual(
            graphAPI.connexions("define_usd_prim_hierarchy", "leaf_mesh"),
            ["bifrostUsdShape.mesh1"],
        )
        self.assertEqual(
            graphAPI.connexions("define_usd_prim_hierarchy", "prim_definitions"),
            ["add_to_stage.prim_definitions.mesh1"],
        )

        self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/pCube1")

        if hasUfePythonBugFix():
            # Check prim path after renaming Maya model
            cmds.rename("Model", "CUBE")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/pCube1")

            cmds.rename("geo", "GEOMETRIES")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/pCube1")

            cmds.rename("pCube1", "SHAPE")
            self.assertEqual(graphAPI.param("define_usd_prim_hierarchy", "path"), "/SHAPE")

            # check that compound is deleted when Maya node is delete
            cmds.delete("SHAPE")
            self.assertEqual(
                graphAPI.find_nodes(kDefinePrimHierarchy), ["define_usd_prim_hierarchy1"]
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
