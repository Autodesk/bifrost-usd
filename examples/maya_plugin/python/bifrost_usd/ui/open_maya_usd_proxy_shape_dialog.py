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

try:
    from PySide2 import QtWidgets
except ModuleNotFoundError:
    from PySide6 import QtWidgets

from maya import cmds
from bifrost_usd import create_stage as cs


class OpenMayaUsdProxyShapeDialog(QtWidgets.QFileDialog):
    currentDir = os.getenv(
        "BIFROST_USD_LAB_COMPONENT_ROOT_DIR", cmds.workspace(q=True, dir=True)
    )

    def __init__(self, parent=None):
        super(OpenMayaUsdProxyShapeDialog, self).__init__(parent)

        self.setWindowTitle("Open USD Stage")

        self.setDirectory(OpenMayaUsdProxyShapeDialog.currentDir)
        self.setNameFilter("USD (*.usd *.usdc *.usda)")
        self.setViewMode(QtWidgets.QFileDialog.Detail)
        self.setFileMode(QtWidgets.QFileDialog.ExistingFile)


def show(layerType="material_library"):
    dialog = OpenMayaUsdProxyShapeDialog(parent=None)
    if dialog.exec_():
        fileNames = dialog.selectedFiles()
        if fileNames:
            cs.create_maya_usd_proxy_shape('materialLibrary', fileNames[0], cs.setup_open_materials_stage)


if __name__ == "__main__":
    show()
