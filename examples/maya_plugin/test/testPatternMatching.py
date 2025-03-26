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


from maya import cmds
from maya import standalone


class PatternMatchingTestCase(unittest.TestCase):
    plugins_loaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.plugins_loaded:
            standalone.initialize("pattern_matching")
            cmds.loadPlugin("bifrostGraph", quiet=True)
            cmds.loadPlugin("bifrostUSDExamples.py", quiet=True)
            cls.plugins_loaded = True

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testMatchByCharOrWildcardCompound(self):
        maya_scene_file_path = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "resources",
            "test_pattern_matching_01.ma",
        )

        cmds.file(maya_scene_file_path, force=True, open=True)

        # Pull the graph
        self.assertTrue(cmds.getAttr("|bifrostGraph1|bifrostGraph1Shape.result"))

if __name__ == "__main__":
    unittest.main(verbosity=2)
