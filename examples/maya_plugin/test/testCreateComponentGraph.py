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

from bifrost_usd.graph_api import bifrost_version


class CreateComponentGraphTestCase(unittest.TestCase):
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

    def _testCreateComponent(self, name, version=""):
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
        self.assertTrue(
            os.path.isfile(maya_scene_file_path),
            f"(this is not a file: {maya_scene_file_path})",
        )

        cmds.file(maya_scene_file_path, force=True, open=True)
        cmds.optionVar(iv=("mayaUsd_MakePathRelativeToSceneFile", 0))

        # Check that binding files are not present before the Bifrost graph is executed
        bindings_dir = os.path.join(test_model_dir, version, "bnd")

        mdl1_look1_path = os.path.join(bindings_dir, "mdl1_look1.usd")
        self.assertFalse(os.path.isfile(mdl1_look1_path))
        mdl1_look2_path = os.path.join(bindings_dir, "mdl1_look2.usd")
        self.assertFalse(os.path.isfile(mdl1_look2_path))
        mdl2_look1_path = os.path.join(bindings_dir, "mdl2_look1.usd")
        self.assertFalse(os.path.isfile(mdl2_look1_path))
        mdl2_look2_path = os.path.join(bindings_dir, "mdl2_look2.usd")
        self.assertFalse(os.path.isfile(mdl2_look2_path))

        # Check that component and its payload are not present before the Bifrost graph is executed
        component_path = os.path.join(test_model_dir, version, f"{name}.usd")
        self.assertFalse(os.path.isfile(component_path))

        payload_path = os.path.join(test_model_dir, version, "payload.usd")
        self.assertFalse(os.path.isfile(payload_path))

        # Enable save mode before pulling the graph
        cmds.vnnNode(
            "|bifrostGraph1|bifrostGraphShape1",
            "/create_usd_component",
            setPortDefaultValues=("save_model", "1"),
        )
        # Pull the graph
        stageId = cmds.getAttr("|bifrostGraph1|bifrostGraphShape1.stage")
        self.assertGreater(stageId, 9223001)

        # Check that files are created
        self.assertTrue(os.path.isfile(mdl1_look1_path))
        self.assertTrue(os.path.isfile(mdl1_look2_path))
        self.assertTrue(os.path.isfile(mdl2_look1_path))
        self.assertTrue(os.path.isfile(mdl2_look2_path))

        self.assertTrue(os.path.isfile(component_path))
        self.assertTrue(os.path.isfile(payload_path))

        # Check content of the files
        from pxr import Usd, UsdShade, Sdf

        def check_material_binding(stage, prim_path, material_path):
            prim = stage.GetPrimAtPath(Sdf.Path(prim_path))
            self.assertTrue(prim)

            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            self.assertTrue(bindingAPI)
            bindingRel = bindingAPI.GetDirectBindingRel()
            self.assertEqual(bindingRel.GetTargets(), [Sdf.Path(material_path)])

        stage = Usd.Stage.Open(mdl1_look1_path)
        check_material_binding(stage, "/mdl1/geo/sphere1", "/mdl1/mtl/Red")
        check_material_binding(stage, "/mdl1/geo/sphere2", "/mdl1/mtl/Blue")

        stage = Usd.Stage.Open(mdl1_look2_path)
        check_material_binding(stage, "/mdl1/geo/sphere1", "/mdl1/mtl/Red")
        check_material_binding(stage, "/mdl1/geo/sphere2", "/mdl1/mtl/Red")

        stage = Usd.Stage.Open(mdl2_look1_path)
        check_material_binding(stage, "/mdl2/geo/sphere1", "/mdl2/mtl/Red")
        check_material_binding(stage, "/mdl2/geo/sphere2", "/mdl2/mtl/Blue")

        stage = Usd.Stage.Open(mdl2_look2_path)
        check_material_binding(stage, "/mdl2/geo/sphere1", "/mdl2/mtl/Red")
        check_material_binding(stage, "/mdl2/geo/sphere2", "/mdl2/mtl/Red")

        stage = Usd.Stage.Open(component_path)

        metadata = stage.GetMetadata("customLayerData").get("Bifrost::usd_component")

        self.assertTrue(metadata)
        self.assertEqual(metadata["geo_scope_name"], "geo")
        self.assertEqual(metadata["mtl_scope_name"], "mtl")
        self.assertEqual(metadata["host_scene"], "02_create_component.ma")

        expectedMayaDirPath = os.path.join(test_model_dir, version, "maya")
        # Before Bifrost 2.11, the scene_info node was adding a path separator at the end of host scene dir
        _, major, minor, patch = bifrost_version().split(".")
        if int(major) < 11:
            expectedMayaDirPath += os.sep
        self.assertEqual(metadata["host_scene_directory"], expectedMayaDirPath)

        default_prim = stage.GetDefaultPrim()
        self.assertEqual(default_prim.GetPath(), Sdf.Path("/spheres"))

        model_variant_set = default_prim.GetVariantSet("Model")
        self.assertEqual(model_variant_set.GetVariantSelection(), "mdl1")

        look_variant_set = default_prim.GetVariantSet("Look")
        self.assertEqual(look_variant_set.GetVariantSelection(), "look1")

        render_model_prim_path = (
            default_prim.GetPath().AppendChild("render").AppendChild("mdl1")
        )
        render_model_prim = stage.GetPrimAtPath(render_model_prim_path)
        self.assertTrue(render_model_prim)

        self.assertEqual(len(render_model_prim.GetPrimStack()), 4)
        self.assertEqual(
            render_model_prim.GetPrimStack()[0].layer.identifier,
            os.path.join(test_model_dir, version, "payload.usd"),
        )

        geo_dir = os.path.join(test_model_dir, version, "geo")
        self.assertEqual(
            render_model_prim.GetPrimStack()[3].layer.identifier,
            os.path.join(geo_dir, "spheres_render.usd"),
        )

        render_mtl_prim = stage.GetPrimAtPath(render_model_prim_path.AppendChild("mtl"))
        self.assertTrue(render_mtl_prim)

        self.assertEqual(
            render_mtl_prim.GetPrimStack()[0].layer.identifier,
            os.path.join(test_model_dir, version, "payload.usd"),
        )
        self.assertEqual(
            render_mtl_prim.GetPrimStack()[1].layer.identifier,
            os.path.join(test_model_dir, version, "payload.usd"),
        )
        mtl_dir = os.path.join(test_model_dir, version, "mtl")
        self.assertEqual(
            render_mtl_prim.GetPrimStack()[2].layer.identifier,
            os.path.join(mtl_dir, "material_library.usda"),
        )

    def testCreateSpheresComponent(self):
        self._testCreateComponent("spheres")

    def testCreateSpheresVersionnedComponent(self):
        self._testCreateComponent("spheres", "1")


if __name__ == "__main__":
    unittest.main(verbosity=2)
