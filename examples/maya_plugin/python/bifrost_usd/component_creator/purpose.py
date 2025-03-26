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
from maya import cmds

from bifrost_usd.constants import kMayaUsdProxyShape


def setPurposes(purpose, show=True):
    for proxyShapeNodeName in cmds.ls(type=kMayaUsdProxyShape):
        drawPurposeAttrName = f"{proxyShapeNodeName}.draw{purpose}"
        cmds.setAttr(drawPurposeAttrName, show)

    # On a machine without GPU (using maya-batch or mayapy), calling the ogs command would fail.
    if cmds.about(batch=True):
        return

    cmds.ogs(reset=True)


def showGuidePurposes():
    setPurposes("GuidePurpose", show=True)
    setPurposes("ProxyPurpose", show=False)
    setPurposes("RenderPurpose", show=False)


def hideGuidePurposes():
    setPurposes("GuidePurpose", show=False)


def showProxyPurposes():
    setPurposes("GuidePurpose", show=False)
    setPurposes("ProxyPurpose", show=True)
    setPurposes("RenderPurpose", show=False)


def hideProxyPurposes():
    setPurposes("ProxyPurpose", show=False)


def showRenderPurposes():
    setPurposes("GuidePurpose", show=False)
    setPurposes("ProxyPurpose", show=False)
    setPurposes("RenderPurpose", show=True)


def hideRenderPurposes():
    setPurposes("RenderPurpose", show=False)


if __name__ == "__main__":
    hideGuidePurposes()
