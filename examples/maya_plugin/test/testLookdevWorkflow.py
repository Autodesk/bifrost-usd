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
import shutil
import tempfile
from distutils.dir_util import copy_tree

from maya import cmds
from maya import standalone

from pxr import Usd, UsdShade, Sdf


class LookdevWorkflowTestCase(unittest.TestCase):
    plugins_loaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.plugins_loaded:
            standalone.initialize("component_creator")
            cmds.loadPlugin("mayaUsdPlugin", quiet=True)
            cmds.loadPlugin("bifrostGraph", quiet=True)
            cmds.loadPlugin("bifrostUSDExamples.py", quiet=True)
            # cmds.loadPlugin("mtoa", quiet=True)
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

    def testLookdevOnSpheres(self):
        lkdv_dir = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "resources",
            "lookdev_workflow",
        )
        copy_tree(lkdv_dir, self.test_dir)

        maya_scene_file_path = os.path.join(self.test_dir, "geometries.ma")
        self.assertTrue(
            os.path.isfile(maya_scene_file_path),
            f"(this is not a file: {maya_scene_file_path})",
        )

        cmds.file(maya_scene_file_path, force=True, open=True)

        # save the geometries layer
        cmds.vnnNode(
            "|bifrostGraph1|bifrostGraphShape1",
            "/stage_selector",
            setPortDefaultValues=("selection", "geometries"),
        )
        cmds.vnnNode(
            "|bifrostGraph1|bifrostGraphShape1",
            "/save_usd_stage",
            setPortDefaultValues=("enable", "1"),
        )

        # Pull the graph
        stageId = cmds.getAttr("|bifrostGraph1|bifrostGraphShape1.stage")
        self.assertGreater(stageId, 9223001)

        cmds.vnnNode(
            "|bifrostGraph1|bifrostGraphShape1",
            "/save_usd_stage",
            setPortDefaultValues=("enable", "0"),
        )
        cmds.vnnNode(
            "|bifrostGraph1|bifrostGraphShape1",
            "/stage_selector",
            setPortDefaultValues=("selection", "lookdev"),
        )
        cmds.vnnNode(
            "|bifrostGraph1|bifrostGraphShape1",
            "/create_lookdev_workflow_stage",
            setPortDefaultValues=("enable", "1"),
        )

        # Pull the graph
        stageId = cmds.getAttr("|bifrostGraph1|bifrostGraphShape1.stage")
        self.assertGreater(stageId, 9223001)

        usd_scene_file_path = os.path.join(self.test_dir, "scene.usda")
        self.assertTrue(
            os.path.isfile(usd_scene_file_path),
            f"(this is not a file: {usd_scene_file_path})",
        )

        def check_direct_binding(
            stage: Usd.Stage, prim_path: str, material_path: str
        ) -> None:
            prim = stage.GetPrimAtPath(Sdf.Path(prim_path))
            self.assertTrue(prim)

            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            self.assertTrue(bindingAPI)
            bindingRel = bindingAPI.GetDirectBindingRel()
            self.assertEqual(bindingRel.GetTargets(), [Sdf.Path(material_path)])

        def check_collection_material_binding(
            stage: Usd.Stage, prim_path: str, material_path: str
        ) -> None:
            prim = stage.GetPrimAtPath(Sdf.Path(prim_path))
            self.assertTrue(prim)

            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            self.assertTrue(bindingAPI)
            bindingRel = bindingAPI.GetCollectionBindingRel("last_ball_row")

            self.assertEqual(
                bindingRel.GetTargets(),
                [
                    Sdf.Path(f"{prim_path}.collection:last_ball_row"),
                    Sdf.Path(material_path),
                ],
            )

        # Check content of the generated usd file
        stage = Usd.Stage.Open(usd_scene_file_path)
        check_direct_binding(stage, "/geometries/balls/Ball", "/geometries/mtl/red")
        check_direct_binding(stage, "/geometries/balls/Ball1", "/geometries/mtl/green")
        check_direct_binding(stage, "/geometries/balls/Ball2", "/geometries/mtl/blue")
        check_direct_binding(
            stage, "/geometries/balls/Ball3", "/geometries/mtl/my_float_0"
        )
        check_direct_binding(stage, "/geometries/balls/Ball4", "/geometries/mtl/red")

        for i in range(5, 14):
            check_direct_binding(
                stage, f"/geometries/balls/Ball{i}", "/geometries/mtl/my_float_0"
            )

        for i in range(15, 19):
            check_direct_binding(
                stage, f"/geometries/balls/Ball{i}", "/geometries/mtl/my_float_1"
            )

        check_collection_material_binding(stage, "/geometries", "/geometries/mtl/blue")

        check_direct_binding(
            stage, "/geometries/pills/grp1/pile1", "/geometries/mtl/red"
        )

        check_direct_binding(
            stage, "/geometries/pills/grp2/pile1", "/geometries/mtl/red"
        )

        check_direct_binding(
            stage, "/geometries/pills/_grp/pile1", "/geometries/mtl/green"
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
