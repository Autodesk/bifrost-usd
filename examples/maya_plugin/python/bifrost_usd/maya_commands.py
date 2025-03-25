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

import maya.api.OpenMaya as om
from maya import cmds

from bifrost_usd import author_usd_graph
from bifrost_usd import create_stage
from bifrost_usd import graph_api
from bifrost_usd.node_def import NodeDef


class BifrostUsdCmd(om.MPxCommand):
    kPluginCmdName = "bifrostUSDExamples"

    kNewStageFlag = "-n"
    kNewStageFlagLong = "-newStage"

    kGrapShapeFlag = "-s"
    kGrapShapeFlagLong = "-shape"

    kOpenStageFlag = "-o"
    kOpenStageFlagLong = "-openStage"

    kLookdevWorkflowFlag = "-l"
    kLookdevWorkflowFlagLong = "-lookdevWorkflow"
    kFilesFlag = "-f"
    kFilesFlagLong = "-files"

    # Insert node flags
    kInsertNodeFlag = "-i"
    kInsertNodeFlagLong = "-insertNode"

    kCurrentCompoundFlag = "-cc"
    kCurrentCompoundFlagLong = "-currentCompound"

    kNodeSelectionFlag = "-ns"
    kNodeSelectionFlagLong = "-nodeSelection"

    kPortSelectionFlag = "-ps"
    kPortSelectionFlagLong = "-portSelection"

    kNodeTypeFlag = "-nt"
    kNodeTypeFlagLong = "-nodeType"

    kInputPortFlag = "-ip"
    kInputPortFlagLong = "-inputPort"

    kOutputPortFlag = "-op"
    kOutputPortFlagLong = "-outputPort"

    kImportModelToStageFlag = "-im"
    kImportModelToStageFlagLong = "-importModel"

    kVariantSetPrimFlag = "-vp"
    kVariantSetPrimFlagLong = "-variantSetPrim"

    kVariantSetFlag = "-vs"
    kVariantSetFlagLong = "-variantSet"

    kInsertVariantFlag = "-iv"
    kInsertVariantFlagLong = "-insertVariant"

    def __init__(self):
        om.MPxCommand.__init__(self)

    @staticmethod
    def creator():
        return BifrostUsdCmd()

    @staticmethod
    def createSyntax():
        syntax = om.MSyntax()
        syntax.setObjectType(om.MSyntax.kSelectionList)
        syntax.useSelectionAsDefault(True)
        syntax.addFlag(BifrostUsdCmd.kNewStageFlag, BifrostUsdCmd.kNewStageFlagLong)
        syntax.addFlag(BifrostUsdCmd.kGrapShapeFlag, BifrostUsdCmd.kGrapShapeFlagLong)
        syntax.addFlag(BifrostUsdCmd.kOpenStageFlag, BifrostUsdCmd.kOpenStageFlagLong)
        syntax.addFlag(BifrostUsdCmd.kLookdevWorkflowFlag, BifrostUsdCmd.kLookdevWorkflowFlagLong)

        syntax.addFlag(
            BifrostUsdCmd.kFilesFlag, BifrostUsdCmd.kFilesFlagLong, om.MSyntax.kString
        )
        syntax.addFlag(BifrostUsdCmd.kInsertNodeFlag, BifrostUsdCmd.kInsertNodeFlagLong)
        syntax.addFlag(
            BifrostUsdCmd.kNodeTypeFlag,
            BifrostUsdCmd.kNodeTypeFlagLong,
            om.MSyntax.kString,
        )
        syntax.addFlag(
            BifrostUsdCmd.kCurrentCompoundFlag,
            BifrostUsdCmd.kCurrentCompoundFlagLong,
            om.MSyntax.kString,
        )
        syntax.addFlag(
            BifrostUsdCmd.kNodeSelectionFlag,
            BifrostUsdCmd.kNodeSelectionFlagLong,
            om.MSyntax.kString,
        )
        syntax.addFlag(
            BifrostUsdCmd.kPortSelectionFlag,
            BifrostUsdCmd.kPortSelectionFlagLong,
            om.MSyntax.kString,
        )
        syntax.addFlag(
            BifrostUsdCmd.kInputPortFlag,
            BifrostUsdCmd.kInputPortFlagLong,
            om.MSyntax.kString,
        )
        syntax.addFlag(
            BifrostUsdCmd.kOutputPortFlag,
            BifrostUsdCmd.kOutputPortFlagLong,
            om.MSyntax.kString,
        )

        syntax.addFlag(
            BifrostUsdCmd.kImportModelToStageFlag,
            BifrostUsdCmd.kImportModelToStageFlagLong,
        )
        syntax.addFlag(
            BifrostUsdCmd.kVariantSetPrimFlag,
            BifrostUsdCmd.kVariantSetPrimFlagLong,
            om.MSyntax.kString,
        )
        syntax.addFlag(
            BifrostUsdCmd.kVariantSetFlag,
            BifrostUsdCmd.kVariantSetFlagLong,
            om.MSyntax.kString,
        )
        syntax.addFlag(
            BifrostUsdCmd.kInsertVariantFlag,
            BifrostUsdCmd.kInsertVariantFlagLong,
        )

        return syntax

    def doIt(self, args):
        try:
            argdb = om.MArgDatabase(self.syntax(), args)
        except RuntimeError:
            om.MGlobal.displayError(
                "Error while parsing arguments:\n#\t# If passing in list of nodes, also check that node names exist in scene."
            )
            raise

        newStage = argdb.isFlagSet(BifrostUsdCmd.kNewStageFlag)
        asShape = argdb.isFlagSet(BifrostUsdCmd.kGrapShapeFlag)

        openStage = argdb.isFlagSet(BifrostUsdCmd.kOpenStageFlag)
        lookdevWorkflow = argdb.isFlagSet(BifrostUsdCmd.kLookdevWorkflowFlag)
        insertNode = argdb.isFlagSet(BifrostUsdCmd.kInsertNodeFlag)

        importModel = argdb.isFlagSet(BifrostUsdCmd.kImportModelToStageFlag)

        variantSetPrim = argdb.isFlagSet(BifrostUsdCmd.kVariantSetPrimFlag)
        variantSet = argdb.isFlagSet(BifrostUsdCmd.kVariantSetFlag)

        insertVariantFlag = argdb.isFlagSet(BifrostUsdCmd.kInsertVariantFlag)

        if newStage:
            if importModel:
                graph = create_stage.create_graph_with_add_to_stage()
                author_usd_graph.add_maya_selection_to_stage()

            elif variantSetPrim:
                primPath = argdb.flagArgumentString(
                    BifrostUsdCmd.kVariantSetPrimFlag, 0
                )

                variantSetName = "VSet"
                if variantSet:
                    variantSetName = argdb.flagArgumentString(
                        BifrostUsdCmd.kVariantSetFlag, 0
                    )
                with author_usd_graph.CurrentMayaSelection(cmds.ls(selection=True)):
                    graph = create_stage.create_graph_with_add_to_stage()

                    author_usd_graph.add_maya_selection_as_variants_to_stage(
                        primPath, variantSetName
                    )
            else:
                graph = create_stage.create_new_stage_graph(as_shape=asShape)

            author_usd_graph.graphAPI.auto_layout_all_nodes()
            self.setResult(graph)

        else:
            if openStage and argdb.isFlagSet(BifrostUsdCmd.kFilesFlag):
                files = argdb.flagArgumentString(BifrostUsdCmd.kFilesFlag, 0)
                filePaths = files.split(",")

                if lookdevWorkflow:
                    graph = create_stage.create_lookdev_graph_from_usd_files(filePaths)
                else:
                    graph = create_stage.create_graph_from_usd_files(filePaths, as_shape=True)

            elif insertNode:
                dgContainerFullPath = ""
                dgContainerName = ""
                currentCompound = argdb.flagArgumentString(
                    BifrostUsdCmd.kCurrentCompoundFlag, 0
                )
                nodeSelection = argdb.flagArgumentString(
                    BifrostUsdCmd.kNodeSelectionFlag, 0
                )
                portSelection = argdb.flagArgumentString(
                    BifrostUsdCmd.kPortSelectionFlag, 0
                )
                nodeType = argdb.flagArgumentString(BifrostUsdCmd.kNodeTypeFlag, 0)
                inputPort = argdb.flagArgumentString(BifrostUsdCmd.kInputPortFlag, 0)
                outputPort = argdb.flagArgumentString(BifrostUsdCmd.kOutputPortFlag, 0)

                if currentCompound and argdb.getObjectList():
                    dgContainerName = argdb.getObjectList().getSelectionStrings()[0]
                    dgContainerFullPath = cmds.ls(dgContainerName, long=True)[0]

                if nodeSelection == "":
                    nodeSelection = []
                else:
                    nodeSelection = [nodeSelection]

                nodeInfo = graph_api.GraphEditorSelection(
                    dgContainerFullPath,
                    dgContainerName,
                    currentCompound,
                    portSelection,
                    nodeSelection,
                )

                newNodeDef = NodeDef(
                    type_name=nodeType,
                    input_name=inputPort,
                    output_name=outputPort,
                )

                graph = author_usd_graph.insert_stage_node(nodeInfo, newNodeDef)

            elif insertVariantFlag:
                graph = ""
                author_usd_graph.insert_maya_variant(graph_api.GraphEditorSelection("", "", "", "out_stage", []))

            author_usd_graph.graphAPI.auto_layout_selected_nodes()
            self.setResult(graph)

    def redoIt(self):
        self.dgmod.doIt()

    def undoIt(self):
        self.dgmod.undoIt()

    def isUndoable(self):
        return True
