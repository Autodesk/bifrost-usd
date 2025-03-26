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
import contextlib
from dataclasses import dataclass, field
from maya import cmds

from bifrost_usd.constants import kMayaUsdProxyShape


def bifrost_version() -> str:
    return cmds.pluginInfo("bifrostGraph", query=True, version=True)


def warning(msg):
    cmds.warning(f"[Bifrost USD] {msg}")


def error(msg):
    cmds.error(f"[Bifrost USD] {msg}")


@contextlib.contextmanager
def GraphPaused(graph):
    runOnDemandValue = cmds.getAttr(f"{graph}.runOnDemand")

    cmds.setAttr(f"{graph}.runOnDemand", True)
    yield
    cmds.setAttr(f"{graph}.runOnDemand", runOnDemandValue)


def find_bifrost_usd_graph() -> str:
    """We consider a Bifrost graph to be a USD graph if there is one output
    of a bifrostGraphShape connected to the stageCacheId plug of a mayaUsdProxyShape node"""
    graphList = cmds.ls(type="bifrostGraphShape")
    for graph in graphList:
        cnx = cmds.listConnections(graph, type=kMayaUsdProxyShape, shapes=True)
        if cnx:
            usdProxyShapeName = cnx[0]
            plugList = cmds.listConnections(graph, type=kMayaUsdProxyShape, plugs=True)
            if plugList:
                if f"{usdProxyShapeName}.stageCacheId" in plugList:
                    return graph

    return ""


def has_bifrost_usd_graph() -> bool:
    if find_bifrost_usd_graph():
        return True
    return False


def open_bifrost_graph(graph) -> None:
    if cmds.about(batch=True):
        warning("Can not open Bifrost Graph Editor in batch mode")
        return

    cmds.vnnCompoundEditor(
        name="bifrostGraphEditorControl",
        title="Bifrost Graph Editor",
        edit=graph,
    )


def open_bifrost_usd_graph() -> None:
    if cmds.about(batch=1):
        return

    if graph := find_bifrost_usd_graph():
        open_bifrost_graph(graph)


@dataclass
class GraphEditorSelection:
    dgContainerFullPath: str = ""
    dgContainerName: str = ""
    currentCompound: str = ""
    output: str = ""
    nodeSelection: list[str] = field(default_factory=list)


def get_graph_selection(refresh: bool = True) -> GraphEditorSelection:
    selection = GraphEditorSelection()

    if refresh:
        cmds.refresh(force=True)

    try:
        rtn = cmds.vnnCompoundEditor(
            query=True, dgContainer=True, currentCompound=True, nodeSelection=True
        )
        selection.dgContainerFullPath = rtn[0]
        selection.dgContainerName = rtn[0].split("|")[-1]
        selection.currentCompound = rtn[1]

        if len(rtn) == 2:
            selection.nodeSelection = []
        elif len(rtn) == 3:
            selection.nodeSelection = [rtn[-1]]
        else:
            selection.nodeSelection = rtn[(len(rtn) - 2) :]

    except RuntimeError:
        cmds.error("You need to open a Bifrost Graph.")

    return selection


class GraphAPI:
    def __init__(self, get_graph_name_fn=None):
        super(GraphAPI, self).__init__()
        self.get_graph_name_fn = (
            get_graph_name_fn if get_graph_name_fn else find_bifrost_usd_graph
        )
        self.ufe_observer = False

    def _getGraphName(self, graph_name: str = "") -> str:
        if graph_name:
            return graph_name
        elif self.get_graph_name_fn:
            return self.get_graph_name_fn()
        return ""

    @property
    def name(self):
        return self._getGraphName()

    def type_name(
        self, node_name: str, graph_name: str = "", current_compound: str = "/"
    ) -> str:
        nodeName = node_name
        if node_name.startswith("/"):
            nodeName = node_name[1:]
        return cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + nodeName,
            queryTypeName=1,
        )

    def param(
        self,
        node_name: str,
        param_name: str,
        graph_name: str = "",
        current_compound: str = "/",
    ) -> str:
        nodeName = node_name
        if node_name.startswith("/"):
            nodeName = node_name[1:]
        return cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + nodeName,
            queryPortDefaultValues=(param_name),
        )

    def set_param(
        self,
        node_name: str,
        param: tuple[str, str],
        graph_name: str = "",
        current_compound: str = "/",
    ) -> None:
        value = param[1]
        if isinstance(value, bool):
            if value:
                value = "1"
            else:
                value = "0"

        cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + node_name,
            setPortDefaultValues=(param[0], value),
        )

    def metadata(
        self,
        node_name: str,
        metadata_name: str,
        graph_name: str = "",
        current_compound: str = "/",
    ) -> str:
        nodeName = node_name
        if node_name.startswith("/"):
            nodeName = node_name[1:]
        return cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + nodeName,
            queryMetaData=metadata_name,
        )

    def set_metadata(
        self,
        node_name: str,
        metadata: tuple[str, str],
        graph_name: str = "",
        current_compound: str = "/",
    ) -> None:
        cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + node_name,
            setMetaData=metadata,
        )

    def create_input_port(
        self,
        node_name: str,
        data: tuple[str, str],
        graph_name: str = "",
        current_compound: str = "/",
    ) -> None:
        nodeName = node_name
        if node_name.startswith("/"):
            nodeName = node_name[1:]

        cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + nodeName,
            createInputPort=data,
        )

    def create_output_port(
        self,
        node_name: str,
        data: tuple[str, str],
        options: str,
        graph_name: str = "",
        current_compound: str = "/",
    ) -> None:
        cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + node_name,
            createOutputPort=data,
            portOptions=options,
        )

    def delete_graph_input_port(
        self,
        port_name: str,
        graph_name: str = "",
        current_compound: str = "/",
    ) -> None:
        cmds.vnnCompound(
            self._getGraphName(graph_name), current_compound, deletePort=port_name
        )

    def add_node(
        self, node_type_name: str, graph_name: str = "", current_compound: str = "/"
    ) -> str:
        """
        :param [node_type_name]: "Input" for Input by Path node, "Output" for Output node and
                                 Bifrost fully qualified type name for other nodes.
        """
        result = ""
        try:
            if node_type_name == "Input":
                result = cmds.vnnCompound(
                    self._getGraphName(graph_name), current_compound, addIONode=True
                )
            elif node_type_name == "Output":
                result = cmds.vnnCompound(
                    self._getGraphName(graph_name), current_compound, addIONode=False
                )
            else:
                result = cmds.vnnCompound(
                    self._getGraphName(graph_name),
                    current_compound,
                    addNode=node_type_name,
                )
        except RuntimeError as e:
            cmds.warning(e)
            return ""

        return result[0]

    def rename_node(
        self,
        node_name: str,
        new_node_name: str,
        graph_name: str = "",
        current_compound: str = "/",
    ) -> None:
        cmds.vnnCompound(
            self._getGraphName(graph_name),
            current_compound,
            renameNode=(node_name, new_node_name),
        )

    def remove_node(
        self, node_name: str, graph_name: str = "", current_compound: str = "/"
    ) -> None:
        cmds.vnnCompound(
            self._getGraphName(graph_name), current_compound, removeNode=node_name
        )

    def find_nodes(
        self,
        type_name: str = "",
        graph_name: str = "",
        current_compound: str = "/",
    ) -> list[str]:
        allNodes = cmds.vnnCompound(
            self._getGraphName(graph_name), current_compound, listNodes=True
        )

        if not type_name:
            return allNodes

        sameTypeNodes = []
        for node in allNodes:
            if type_name == cmds.vnnNode(
                self._getGraphName(graph_name), f"/{node}", queryTypeName=1
            ):
                sameTypeNodes.append(node)

        return sameTypeNodes

    def connect(
        self,
        src_node: str,
        out_port: str,
        target_node: str,
        in_port: str,
        graph_name: str = "",
        current_compound: str = "/",
    ):
        srcNode = src_node
        if src_node.startswith("/"):
            srcNode = src_node[1:]

        targetNode = target_node
        if target_node.startswith("/"):
            targetNode = target_node[1:]

        currentCompound = current_compound
        if not target_node:
            currentCompound = ""

        srcNodeFullPath = current_compound + srcNode
        if not src_node:
            srcNodeFullPath = ""

        cmds.vnnConnect(
            self._getGraphName(graph_name),
            srcNodeFullPath + "." + out_port,
            currentCompound + targetNode + "." + in_port,
        )

    def disconnect(self, port1: str, port2: str) -> None:
        """Disconnect port1 from port2. There is no order.
        The port path is the full path. For example:
        disconnect("define_usd_material_binding.material_binding",
                   "define_usd_look_variant.material_bindings.material_binding")
        """

        cmds.vnnConnect(self._getGraphName(), f"/{port1}", f"/{port2}", disconnect=True)

    def connected_ports(
        self, node: str, graph_name: str = "", current_compound: str = "/"
    ) -> list:
        return cmds.vnnNode(
            self._getGraphName(graph_name),
            current_compound + node,
            listPorts=1,
            connected=True,
        )

    def port_children(self, node: str, port_name: str) -> list[str]:
        if (
            childPortNames := cmds.vnnNode(
                self._getGraphName(), f"/{node}", listPortChildren=port_name
            )
        ) is None:
            childPortNames = []

        return childPortNames

    def get_new_fanin_name(self, node: str, parent_port: str, from_port: str) -> str:
        childPortNames = self.port_children(node, parent_port)
        if len(childPortNames) == 0:
            fanInName = from_port
        else:
            childPortNames.sort()
            portName = childPortNames[-1]
            suffix = ""
            while portName[-1].isdigit():
                suffix += portName[-1]
                portName = portName[:-1]

            if suffix:
                # revert string
                suffix = suffix[::-1]
                # increment
                suffix = str(int(suffix) + 1)
            else:
                suffix = "1"

            fanInName = from_port + suffix

        return fanInName

    def enable_fanin_port(self, node: str, port_name: str, graph_name: str = "") -> None:
        cmds.vnnPort(
            self._getGraphName(graph_name), f"/{node}.{port_name}", 0, 1, set=2
        )

    def disable_fanin_port(self, node: str, port_name: str, graph_name: str = "") -> None:
        cmds.vnnPort(
            self._getGraphName(graph_name), f"/{node}.{port_name}", 0, 1, clear=2
        )

    def connect_to_fanin_port(
        self, from_node: str, to_node: str, parent_port: str, from_port: str
    ) -> None:
        """[BIFROST-9071]: Since we can't use a vnn command directly to connect
        to a fan-in port, this function does the hard work.

        :param [from_node]: name of the upstream node
        :param [to_node]: name of the downstream node
        :param [parent_port]: fan-in port name
        :param [from_port]: output port name on the upstream node
        """
        fanInName = self.get_new_fanin_name(to_node, parent_port, from_port)

        graphName = self._getGraphName()
        cmds.vnnChangeBracket(graphName, open=True)
        cmds.vnnNode(
            graphName,
            f"/{to_node}",
            createInputPort=(f"{parent_port}.{fanInName}", "auto"),
        )
        cmds.vnnConnect(
            self._getGraphName(),
            f"/{from_node}.{from_port}",
            f"/{to_node}.{parent_port}.{fanInName}",
        )
        cmds.vnnChangeBracket(graphName, close=True)

    def connexions(
        self,
        node_name: str,
        port_name: str = "",
        graph_name: str = "",
        current_compound: str = "/",
    ) -> list[str]:
        if port_name:
            rtn = cmds.vnnNode(
                self._getGraphName(graph_name),
                current_compound + node_name,
                connectedTo=port_name,
                listConnectedNodes=True,
            )
        else:
            rtn = cmds.vnnNode(
                self._getGraphName(graph_name),
                current_compound + node_name,
                listConnectedNodes=True,
            )

        # the vnn command can return None instead of []
        if rtn is None:
            rtn = []

        return rtn

    def auto_layout_all_nodes(self, graph_name: str = ""):
        """It will clear the node selection in order to auto-layout the entire graph."""
        if cmds.about(batch=1):
            return

        # [BIFROST-10282] Workaround to clear the node selection since there is no vnn command for that.
        # We create a temporary node to change the selection and then delete it to clear the selection.
        graphName = self._getGraphName(graph_name)
        mayaCmd = (
            f'cmds.vnnCompound("{graphName}", "/", create="_tmp_node_to_be_deleted_")'
        )
        mayaCmd += ";" + "cmds.refresh(force=True)"
        mayaCmd += (
            ";"
            + f'cmds.vnnCompound("{graphName}", "/", removeNode="_tmp_node_to_be_deleted_")'
        )
        # Use the "L" keyboard shortcut to run the auto-layout
        mayaCmd += (
            ";"
            + 'cmds.vnnCompoundEditor(sendKey=(ord("L"), 0), name="bifrostGraphEditorControl")'
        )
        cmds.evalDeferred(mayaCmd)

    def auto_layout_selected_nodes(self, graph_name: str = ""):
        """It will clear the node selection in order to auto-layout the entire graph."""
        if cmds.about(batch=1):
            return

        # Use the "L" keyboard shortcut to run the auto-layout
        cmds.evalDeferred(
            'cmds.vnnCompoundEditor(sendKey=(ord("L"), 0), name="bifrostGraphEditorControl")'
        )

    def copy(self, source_node: str, graph_name: str = "") -> None:
        cmds.vnnCopy(self._getGraphName(graph_name), ".", sourceNode=source_node)

    def paste(self, location: str = ".", graph_name: str = "") -> None:
        cmds.vnnPaste(self._getGraphName(graph_name), location)
