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

import os
import sys
import shutil
import tempfile
from distutils.dir_util import copy_tree

from maya import cmds
from maya import standalone

from bifrost_usd.graph_api import bifrost_version


test_dir = os.path.dirname(os.path.realpath(__file__))
maya_usd_model_dir = os.path.join(test_dir, "..", "python")
sys.path.append(maya_usd_model_dir)


class GetComponentMetadataTestCase(unittest.TestCase):
    plugins_loaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.plugins_loaded:
            standalone.initialize("component_creator")
            cmds.loadPlugin("mayaUsdPlugin", quiet=True)
            cmds.loadPlugin("bifrostGraph", quiet=True)
            cmds.loadPlugin("bifrostUSDExamples.py", quiet=True)
            cls.plugins_loaded = True

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        # Create a temporary directory
        self.test_dir = tempfile.mkdtemp()

    def tearDown(self):
        # Remove the directory after the test
        shutil.rmtree(self.test_dir)

    def _createComponent(self, name, version=""):
        model_dir = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "resources",
            "components",
            name,
        )
        test_model_dir = os.path.join(self.test_dir, name)
        copy_tree(model_dir, test_model_dir)

        maya_scene_file_path = os.path.join(
            test_model_dir, version, "maya", "02_create_component.ma"
        )
        self.assertTrue(os.path.isfile(maya_scene_file_path))
        cmds.file(maya_scene_file_path, force=True, open=True)
        cmds.optionVar(iv=("mayaUsd_MakePathRelativeToSceneFile", 0))

        self.assertTrue(cmds.ls("|bifrostGraph1|bifrostGraphShape1"))
        # Enable save mode before pulling the graph
        cmds.vnnNode(
            "|bifrostGraph1|bifrostGraphShape1",
            "/create_usd_component",
            setPortDefaultValues=("save_model", "1"),
        )

        # Pull the graph
        stageId = cmds.getAttr("|bifrostGraph1|bifrostGraphShape1.stage")
        self.assertGreater(stageId, 9223001)

        # close scene
        cmds.file(force=True, new=True)

    def testGetComponentMetadata(self):
        modelName = "spheres"

        testModelDir = os.path.join(self.test_dir, modelName)
        componentFilePath = os.path.join(testModelDir, f"{modelName}.usd")
        self.assertFalse(os.path.isfile(componentFilePath))
        self._createComponent("spheres")
        self.assertTrue(os.path.isfile(componentFilePath))

        from pxr import Usd
        stage = Usd.Stage.Open(componentFilePath)
        self.assertTrue(stage)

        metadata = stage.GetMetadata("customLayerData").get("Bifrost::usd_component")
        self.assertTrue(metadata)

        self.assertEqual(metadata.get("geo_scope_name"), 'geo')
        self.assertEqual(metadata.get("mtl_scope_name"), 'mtl')
        self.assertEqual(metadata.get("host_scene"), '02_create_component.ma')

        expectedMayaDirPath = os.path.join(testModelDir, "maya")
        # Before Bifrost 2.11, the scene_info node was adding a path separator at the end of host scene dir
        _, major, minor, patch = bifrost_version().split(".")
        if int(major) < 11:
            expectedMayaDirPath += os.sep
        self.assertEqual(metadata["host_scene_directory"], expectedMayaDirPath)


if __name__ == "__main__":
    unittest.main(verbosity=2)
