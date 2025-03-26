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
import unittest
import tempfile
import shutil

from typing import Final

from maya import cmds
from maya import standalone

import ufe
import mayaUsd

from bifrost_usd import graph_api
from bifrost_usd import create_stage
from bifrost_usd import author_usd_graph as aug
from bifrost_usd.node_def import NodeDef


from bifrost_usd.constants import (
    kBifrostBoard,
    kBifrostGraphShape,
    kDefinePrim,
    kGraphName,
    kMayaUsdProxyShape,
)

kCurrentDir: Final = os.path.dirname(os.path.realpath(__file__))


class BifrostStageCmdsTestCase(unittest.TestCase):
    plugins_loaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.plugins_loaded:
            standalone.initialize("bifrost-usd-create-stage-cmds")
            cmds.loadPlugin("mayaUsdPlugin", quiet=True)
            cmds.loadPlugin("bifrostGraph", quiet=True)
            cmds.loadPlugin("bifrostUSDExamples.py", quiet=True)
            cls.plugins_loaded = True

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        cmds.file(f=True, new=True)

    def tearDown(self):
        # Remove the directory after the test
        shutil.rmtree(self.test_dir)

    def testCreateEmptyGraphShape(self):
        graph = create_stage._create_empty_graph(as_shape=True)
        self.assertEqual(cmds.nodeType(graph), kBifrostGraphShape)
        self.assertEqual(graph, f"{kGraphName}Shape")

    def testCreateEmptyGraphBoard(self):
        graph = create_stage._create_empty_graph(as_shape=False)
        self.assertEqual(cmds.nodeType(graph), kBifrostBoard)
        self.assertEqual(graph, kGraphName)

    def testCreateNewStageGraph(self):
        graph = create_stage.create_new_stage_graph()
        self.assertEqual(cmds.nodeType(graph), kBifrostGraphShape)
        self.assertTrue("stage" in cmds.listAttr(graph, scalar=True))

    def testCreateNewBoardStageCommand(self):
        # create a bifrostBoard node
        graph = cmds.bifrostUSDExamples(newStage=True)
        self.assertEqual(graph, [kGraphName])
        self.assertEqual(cmds.nodeType(graph[0]), kBifrostBoard)
        self.assertTrue("stage" in cmds.listAttr(graph, scalar=True))

    def testCreateNewShapeStageCommand(self):
        # no mayaUsdProxyShape should be there
        self.assertFalse(cmds.ls(type=kMayaUsdProxyShape))

        # create a bifrostGraphShape node
        graph = cmds.bifrostUSDExamples(newStage=True, shape=True)
        self.assertEqual(graph, [f"{kGraphName}Shape"])
        self.assertEqual(cmds.nodeType(graph[0]), kBifrostGraphShape)
        self.assertTrue("stage" in cmds.listAttr(graph, scalar=True))

        # a mayaUsdProxyShape should be there
        self.assertTrue(cmds.ls(type=kMayaUsdProxyShape))

    def testCreateGraphFromUsdFile(self):
        filePath = os.path.join(kCurrentDir, "resources", "capsule.usd")
        self.assertTrue(os.path.isfile(filePath))
        graph = create_stage.create_graph_from_usd_files([filePath], as_shape=True)
        self.assertEqual(graph, f"{kGraphName}Shape")

        mayaUsdShapes = cmds.ls(type=kMayaUsdProxyShape, long=True)
        self.assertTrue(mayaUsdShapes)
        proxyShapePath = ufe.PathString.path(mayaUsdShapes[0])
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(proxyShapePath))
        self.assertTrue(stage)
        obj = stage.GetPrimAtPath("/obj")
        self.assertEqual(obj.GetPath(), "/obj")

    def testCreateGraphFromOneFileCommand(self):
        filePath = os.path.join(kCurrentDir, "resources", "capsule.usd")
        graph = cmds.bifrostUSDExamples(openStage=True, files=",".join([filePath]))
        self.assertEqual(graph, [f"{kGraphName}Shape"])

    def testCreateGraphFromTwoFilesCommand(self):
        geoFilePath = os.path.join(kCurrentDir, "resources", "capsule.usd")
        colorsFilePath = os.path.join(kCurrentDir, "resources", "capsule_colors.usd")

        graph = cmds.bifrostUSDExamples(
            openStage=True, files=",".join([geoFilePath, colorsFilePath])
        )
        self.assertEqual(graph, [f"{kGraphName}Shape"])

        # Save the Maya scene to get usd file path relative to scene_info.scene_directory jobPort
        cmds.file(
            rename=os.path.join(self.test_dir, "testCreateGraphFromTwoFilesCommand.ma")
        )
        cmds.file(save=True, type="mayaAscii")

        # check the mayaUsdProxyShape content
        import ufe
        import mayaUsd.ufe

        mayaUsdShapes = cmds.ls(type=kMayaUsdProxyShape, long=True)
        self.assertTrue(mayaUsdShapes)
        proxyShapePath = ufe.PathString.path(mayaUsdShapes[0])

        # share the Bifrost stage with the mayaUsdProxyShape stage to
        # get the same sublayers.
        create_stage._set_shared_stage(graph, True)
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(proxyShapePath))
        self.assertTrue(stage)
        self.assertEqual(
            stage.GetRootLayer().subLayerPaths, [colorsFilePath, geoFilePath]
        )

    def testInsertStageNode(self):
        cmds.bifrostUSDExamples(newStage=True, shape=True)

        self.assertEqual(
            cmds.vnnCompound(f"{kGraphName}Shape", "/", listNodes=True),
            ["output", "create_usd_stage"],
        )

        graphEditorSelection = graph_api.GraphEditorSelection(
            f"|{kGraphName}|{kGraphName}Shape",
            f"{kGraphName}Shape",
            "/",
            "stage",
            ["create_usd_stage"],
        )

        newNodeDef = NodeDef(
            type_name="BifrostGraph,USD::Stage,save_usd_stage",
            input_name="stage",
            output_name="out_stage",
        )

        aug.insert_stage_node(graphEditorSelection, newNodeDef)

        self.assertEqual(
            cmds.vnnCompound(f"{kGraphName}Shape", "/", listNodes=True),
            ["output", "create_usd_stage", "save_usd_stage"],
        )

        newNodeDef.type_name = "BifrostGraph,USD::Stage,add_to_stage"
        aug.insert_stage_node(graphEditorSelection, newNodeDef)

        self.assertEqual(
            cmds.vnnCompound(f"{kGraphName}Shape", "/", listNodes=True),
            ["output", "create_usd_stage", "save_usd_stage", "add_to_stage"],
        )

    def testInsertStageNodeCommand(self):
        cmds.bifrostUSDExamples(newStage=True, shape=True)

        nodeType = "BifrostGraph,USD::Stage,save_usd_stage"
        self.assertEqual(
            cmds.bifrostUSDExamples(
                insertNode=True,
                nodeType=nodeType,
                currentCompound="/",
                nodeSelection="create_usd_stage",
                portSelection="stage",
                inputPort="stage",
                outputPort="out_stage",
            ),
            ["save_usd_stage"],
        )

    def testInsertPrimNode(self):
        # create a Bifrost USD stage
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

        aug.graphAPI.add_node(kDefinePrim)
        self.assertEqual(
            aug.graphAPI.find_nodes(),
            ["output", "create_usd_stage", "add_to_stage", "define_usd_prim"],
        )

        aug.graphAPI.create_input_port(
            "/add_to_stage", ("prim_definitions.prim", "auto")
        )
        aug.graphAPI.connect(
            "/define_usd_prim",
            "prim_definition",
            "/add_to_stage",
            "prim_definitions.prim",
        )

        # Select the USD prim created by the define_usd_prim node
        cmds.select("|mayaUsdProxy1|mayaUsdProxyShape1,/obj", replace=True)

        nodeDef = NodeDef(
            type_name="BifrostGraph,USD::Prim,set_prim_active",
            input_name="stage",
            output_name="out_stage",
        )

        graphEditorSelection = graph_api.GraphEditorSelection(
            f"|{kGraphName}|{kGraphName}Shape",
            f"{kGraphName}Shape",
            "/",
            "out_stage",
            ["add_to_stage"],
        )

        with self.assertRaises(RuntimeError):
            aug.insert_prim_node(graphEditorSelection, NodeDef(type_name="__ThisNodeDoesNotExit__"))

        aug.insert_prim_node(graphEditorSelection, nodeDef)

        self.assertEqual(
            aug.graphAPI.find_nodes(),
            [
                "output",
                "create_usd_stage",
                "add_to_stage",
                "define_usd_prim",
                "set_prim_active",
            ],
        )

        self.assertEqual(aug.graphAPI.param("set_prim_active", "path"), "/obj")

        cmds.select(clear=True)
        graphEditorSelection = graph_api.GraphEditorSelection(
            f"|{kGraphName}|{kGraphName}Shape",
            f"{kGraphName}Shape",
            "/",
            "out_stage",
            ["set_prim_active"],
        )

        aug.insert_prim_node(graphEditorSelection, nodeDef)
        self.assertEqual(
            aug.graphAPI.find_nodes(),
            [
                "output",
                "create_usd_stage",
                "add_to_stage",
                "define_usd_prim",
                "set_prim_active",
                "set_prim_active1",
            ],
        )
        self.assertEqual(aug.graphAPI.param("set_prim_active1", "path"), "")

    def testSetPointInstancerInvisibleIds(self):
        # open USD file with PointInstancer
        filePath = os.path.join(kCurrentDir, "resources", "simple_point_instancer.usd")
        create_stage.create_graph_from_usd_files([filePath], as_shape=True)

        # select point instances to hide
        instancerPath = "|mayaUsdProxy1|mayaUsdProxyShape1,/instancer"
        ids = (1, 2, 3, 4, 5)
        pointInstancesPaths = [instancerPath + "/" + str(i) for i in ids]
        cmds.select(pointInstancesPaths, replace=True)

        # hide point instances
        aug.set_point_instancer_invisible_ids(
            aug.GraphEditorSelection(
                dgContainerFullPath="|bifrostUsd|bifrostUsdShape",
                dgContainerName="bifrostUsdShape",
                currentCompound="/",
                output="",
                nodeSelection=["create_usd_stage"],
            )
        )

        self.assertTrue(
            "set_usd_point_instances_invisible" in aug.graphAPI.find_nodes()
        )

        # check that selected point instances are hidden
        shapePath = "|mayaUsdProxy1|mayaUsdProxyShape1"

        # importing pxr before loading the mayaUsd plugin would fail.
        from pxr import UsdGeom

        def _test_invisible_id_attrs(
            shape_path: str, instancer_path: str, expected_values: tuple[int]
        ) -> None:
            stage = mayaUsd.ufe.getStage(shape_path)
            self.assertTrue(stage)
            prim = stage.GetPrimAtPath(instancer_path)
            instancer = UsdGeom.PointInstancer(prim)
            self.assertEqual(instancer.GetInvisibleIdsAttr().Get(), expected_values)

        _test_invisible_id_attrs(shapePath, "/instancer", ids)

        # select additional point instances to hide
        ids2 = (6, 7, 8, 9, 10)
        pointInstancesPaths = [instancerPath + "/" + str(i) for i in ids2]
        cmds.select(pointInstancesPaths, replace=True)

        # hide additional point instances
        aug.set_point_instancer_invisible_ids(
            aug.GraphEditorSelection(
                dgContainerFullPath="|bifrostUsd|bifrostUsdShape",
                dgContainerName="bifrostUsdShape",
                currentCompound="/",
                output="",
                nodeSelection=["set_usd_point_instances_invisible"],
            )
        )

        self.assertTrue(
            "set_usd_point_instances_invisible1" in aug.graphAPI.find_nodes()
        )

        aug.graphAPI.set_param("set_usd_point_instances_invisible1", ("replace", True))
        _test_invisible_id_attrs(shapePath, "/instancer", ids2)

        aug.graphAPI.set_param("set_usd_point_instances_invisible1", ("replace", False))
        _test_invisible_id_attrs(shapePath, "/instancer", ids2 + ids)


if __name__ == "__main__":
    unittest.main(verbosity=2)
