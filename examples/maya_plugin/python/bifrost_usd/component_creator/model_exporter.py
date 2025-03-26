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
from typing import Tuple

from maya import cmds

from bifrost_usd.component_creator import asset_template


class ModelExporter(object):
    """Export Maya selection as a USD layer in 'ModelName/geo' directory.
    Checks that the exported hierarchy includes a 'geo' prim with a
    "USD_typeName" custom attribute.
    """

    @staticmethod
    def get_valid_selection() -> str:
        if cmds.ls(selection=True):
            selection = cmds.ls(selection=True, long=True)[0]
            # Selection can't have shapes bellow.
            if (shapes := cmds.listRelatives(selection, shapes=True)):
                cmds.warning(
                    f"'{shapes}' should be in a 'geo' group placed under the selected model."
                )
                return ""

            if len((children := cmds.listRelatives(selection, path=True))) != 1:
                cmds.warning(
                    "More than one child found in selection, please group the geometries."
                )
                return ""

            if (shapes := cmds.listRelatives(children, shapes=True)):
                cmds.warning(
                    f"'{shapes}' should be in a 'geo' group placed under the selected model."
                )
                return ""

            geoScopePath = children[0]
            if (geoScopeName := geoScopePath.split("|")[-1]) == "geo" or geoScopeName == "Geometries":
                pass
            else:
                cmds.warning(f"Renaming {geoScopeName} to 'geo'")
                geoScopePath = cmds.rename((geoScopePath, "geo"))

            rtn = cmds.listAttr(geoScopePath, userDefined=True)
            if not rtn or "USD_typeName" not in rtn:
                cmds.addAttr(geoScopePath, ln="USD_typeName", dt="string")
                cmds.setAttr(f"{geoScopePath}.USD_typeName", "Scope", type="string")

            return selection

        cmds.warning("Selection is not valid.")
        return ""

    @classmethod
    def get_model_name_info(self) -> Tuple[str, str, str]:
        validSection = ModelExporter.get_valid_selection()
        if validSection:
            # strip the "|" prefix
            name = validSection[1:]
            return asset_template.get_model_name_info(name)
        return ("", "", "")

    @classmethod
    def unsafe_get_geo_scope_name(self) -> str:
        """ It should never be called before get_valid_selection."""
        selection = cmds.ls(selection=True, long=True)[0]
        children = cmds.listRelatives(selection)
        return children[0]

    def __init__(
        self,
        models_dir: str,
        model_name: str = "",
        version: str = "",
        variant: str = "default",
        purpose: str = "default",
        autoNaming: bool = False,
    ):
        super(ModelExporter, self).__init__()

        self.models_dir = models_dir
        self.version = version

        self.model_path = self.get_valid_selection()

        # remove the first "|" from the path
        self.selectionName = self.model_path[1:]

        if autoNaming:
            names = self.get_model_name_info()
            self.model_name = names[0]
            self.variant = names[1]
            self.purpose = "" if names[2] == "default" else names[2]
        else:
            self.model_name = self.selectionName if model_name == "" else model_name
            self.variant = "default" if variant == "" else variant
            self.purpose = "" if purpose == "default" else purpose

    @property
    def file_path(self):
        if self.purpose:
            fileName = f"{self.variant}_{self.purpose}.usd"
        else:
            fileName = f"{self.variant}.usd"

        return os.path.join(
            self.models_dir,
            self.model_name,
            self.version,
            self.unsafe_get_geo_scope_name(),
            fileName,
        )

    def do(self):
        if self.model_path:
            cmds.select(self.model_path, replace=True)
            shadingMode = "none"

            convertMaterialsTo = []
            # Uncomment to export materials
            # if self.purpose != "render":
            #     shadingMode = "useRegistry"
            #     convertMaterialsTo = ["MaterialX"]

            cmds.mayaUSDExport(
                self.model_path,
                file=self.file_path,
                exportUVs=True,
                exportDisplayColor=True,
                exportColorSets=True,
                exportComponentTags=True,
                defaultMeshScheme=None,
                defaultUSDFormat="usdc",
                defaultPrim=self.selectionName,
                parentScope=None,
                exportInstances=True,
                exportVisibility=True,
                mergeTransformAndShape=True,
                stripNamespaces=False,
                frameRange=(0, 0),
                shadingMode=shadingMode,
                exportRelativeTextures="automatic",
                convertMaterialsTo=convertMaterialsTo,
            )


if __name__ == "__main__":
    pass
