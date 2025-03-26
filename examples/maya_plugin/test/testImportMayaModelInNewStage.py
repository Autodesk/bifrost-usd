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

from maya import cmds
from maya import standalone

from bifrost_usd.constants import kGraphName, kDefinePrimHierarchy
from bifrost_usd.author_usd_graph import graphAPI

from bifrost_usd.graph_api import GraphEditorSelection


def get_graph_selection() -> GraphEditorSelection:
    """Used to patch "bifrost_usd.author_usd_graph.get_graph_selection" since
    the Bifrost Graph Editor can't be open in unitests.

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


mock_create_stage_get_graph_selection = get_graph_selection
mock_author_usd_graph_get_graph_selection = get_graph_selection


class ImportMayaModelTestCaseInNewStage(unittest.TestCase):
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
        "bifrost_usd.create_stage.get_graph_selection",
        side_effect=mock_create_stage_get_graph_selection,
    )
    @patch(
        "bifrost_usd.author_usd_graph.get_graph_selection",
        side_effect=mock_author_usd_graph_get_graph_selection,
    )
    def testImportModelUnderPseudoRoot(
        self,
        mock_create_stage_get_graph_selection,
        mock_author_usd_graph_get_graph_selection,
    ):
        # create the bifrostUsd graph with an add_to_stage compound.
        cmds.bifrostUSDExamples(newStage=True, importModel=True)
        mock_create_stage_get_graph_selection.assert_called_once_with()
        mock_author_usd_graph_get_graph_selection.assert_called_once_with()

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


if __name__ == "__main__":
    unittest.main(verbosity=2)
