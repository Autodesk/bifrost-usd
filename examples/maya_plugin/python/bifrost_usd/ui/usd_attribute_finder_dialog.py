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
try:
    from PySide2 import QtWidgets
    from PySide2.QtCore import Qt
    from shiboken2 import wrapInstance
except ModuleNotFoundError:
    from PySide6 import QtWidgets
    from PySide6.QtCore import Qt
    from shiboken6 import wrapInstance

from maya import cmds
from maya import OpenMayaUI as omui
import mayaUsd


class UsdAttributeFinderDialog(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super(UsdAttributeFinderDialog, self).__init__(parent)

        mainLayout = QtWidgets.QVBoxLayout()
        mainLayout.addWidget(self._build_table())
        closeBtn = QtWidgets.QPushButton("Close", self)
        closeBtn.clicked.connect(self.close)
        mainLayout.addWidget(closeBtn)

        self.setLayout(mainLayout)

        self.setMinimumWidth(650)
        self.setMinimumHeight(400)
        self.setWindowTitle("USD Attribute Finder")
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

    def _build_table(self):
        table = QtWidgets.QTableWidget()
        table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        attributes = get_attributes_from_selected_prim()
        table.setRowCount(len(attributes))
        table.setColumnCount(3)
        table.setHorizontalHeaderLabels(["Name", "Type", "Value"])

        for i, attrib in enumerate(get_attributes_from_selected_prim()):
            table.setItem(i, 0, QtWidgets.QTableWidgetItem(attrib["name"]))
            table.setItem(i, 1, QtWidgets.QTableWidgetItem(attrib["type"]))
            if attrib["value"]:
                table.setItem(i, 2, QtWidgets.QTableWidgetItem(attrib["value"]))
        return table


def show():
    ptr = omui.MQtUtil.mainWindow()
    widget = wrapInstance(int(ptr), QtWidgets.QWidget)
    dialog = UsdAttributeFinderDialog(widget)
    dialog.show()


def _get_attributes(prim) -> list[dict[str, str]]:
    data: list[dict[str, str]] = []
    for attr in prim.GetAttributes():
        if not attr.HasAuthoredValue():
            continue

        value: str = ""
        typeName = attr.GetTypeName().type.typeName
        if attr.GetTypeName().isArray:
            value = ""
        elif (
            typeName == "TfToken"
            or typeName == "string"
            or typeName == "SdfAssetPath"
            or typeName == "float"
            or typeName == "double"
            or typeName == "int"
            or typeName == "bool"
        ):
            value = str(attr.Get())

        data.append(
            {
                "name": attr.GetName(),
                "type": attr.GetTypeName().type.typeName,
                "value": value,
            }
        )
    return data


def get_attributes_from_selected_prim() -> list[dict[str, str]]:
    data: list[dict[str, str]] = []
    if ufeSelection := cmds.ls(selection=True, ufe=True):
        stage = mayaUsd.ufe.getStage(ufeSelection[0])
        segments = ufeSelection[0].split(",")
        if len(segments) > 1:
            prim = stage.GetPrimAtPath(segments[-1])
            data = _get_attributes(prim)

    return data


if __name__ == "__main__":
    show()
