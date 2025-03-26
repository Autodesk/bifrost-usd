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

from maya import cmds
from maya import standalone

import ufe

from bifrost_usd.constants import kGraphName, kAddToStageNodeDef, kPrimNodeList
from bifrost_usd import graph_api, author_usd_graph, create_stage
from bifrost_usd.author_usd_graph import (
    graphAPI,
    get_variant_set_names,
    update_variant_set_names,
    get_prim_paths_from_node_types,
)


class ImportMayaVariantsTestCase(unittest.TestCase):
    plugins_loaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.plugins_loaded:
            standalone.initialize("bifrost-usd-import-maya-variants")
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

        # create the first Maya variant
        cmds.polyCube()
        cmds.group("pCube1", name="geo")
        cmds.group("geo", name="Cube")

        cmds.select("geo", replace=True)
        cmds.addAttr(longName="USD_typeName", dataType="string", keyable=False)
        cmds.setAttr("geo.USD_typeName", "Scope", type="string")

        cmds.select("Cube", replace=True)

        # create the second Maya variant
        cmds.polyTorus()
        cmds.group("pTorus1", name="geo")
        cmds.group("|geo", name="Torus")

        cmds.select("|Torus|geo", replace=True)
        cmds.addAttr(longName="USD_typeName", dataType="string", keyable=False)
        cmds.setAttr("|Torus|geo.USD_typeName", "Scope", type="string")

        graphAPI.ufe_observer = True

    def testImportVariants(self):
        create_stage.create_new_stage_graph(as_shape=True)

        graphSelection = graph_api.GraphEditorSelection(
            f"|{kGraphName}|{kGraphName}Shape",  # dgContainerFullPath
            f"{kGraphName}Shape",  # dgContainerName
            "/",  # currentCompound
            "stage",  # output
            ["create_usd_stage"],  # nodeSelection
        )

        graphSelection.nodeSelection = [
            author_usd_graph.insert_stage_node(graphSelection, kAddToStageNodeDef)
        ]
        self.assertEqual(graphSelection.nodeSelection, ["add_to_stage"])
        graphSelection.output = "out_stage"
        primDefNode = author_usd_graph._add_prim_definition("/add_to_stage", "/Model")
        self.assertEqual(primDefNode, "define_usd_prim")

        addToStageInVariantNode = (
            author_usd_graph.add_one_maya_selection_as_variant_to_stage(
                "/Model", "VSet", "Cube", graphSelection
            )
        )
        graphSelection.nodeSelection = ["add_to_stage_in_variant"]
        addToStageInVariantNode = (
            author_usd_graph.add_one_maya_selection_as_variant_to_stage(
                "/Model", "VSet", "Torus", graphSelection
            )
        )

        graphAPI.set_param(addToStageInVariantNode, ("select", "1"))

        ufeShapePath = ufe.PathString.path(
            "|mayaUsdProxy1|mayaUsdProxyShape1,/Model/geo/pTorus1"
        )
        sceneItem = ufe.Hierarchy.createItem(ufeShapePath)
        self.assertEqual(sceneItem.nodeType(), "Mesh")

        ufeModelPath = ufe.PathString.path("|mayaUsdProxy1|mayaUsdProxyShape1,/Model")
        item = ufe.Hierarchy.createItem(ufeModelPath)
        contextOps = ufe.ContextOps.contextOps(item)
        contextOps.doOp(["Variant Sets", "VSet", "Cube"])

        ufeShapePath = ufe.PathString.path(
            "|mayaUsdProxy1|mayaUsdProxyShape1,/Model/geo/pCube1"
        )
        sceneItem = ufe.Hierarchy.createItem(ufeShapePath)
        self.assertEqual(sceneItem.nodeType(), "Mesh")

        expectedNames = set()
        expectedNames.add("VSet")
        self.assertEqual(expectedNames, get_variant_set_names())

        update_variant_set_names("VSet", "NewVSetName")
        expectedNames = set()
        expectedNames.add("NewVSetName")
        self.assertEqual(expectedNames, get_variant_set_names())

        expectedPaths = set()
        expectedPaths.add("/Model")
        self.assertEqual(expectedPaths, get_prim_paths_from_node_types(kPrimNodeList))


if __name__ == "__main__":
    unittest.main(verbosity=2)
