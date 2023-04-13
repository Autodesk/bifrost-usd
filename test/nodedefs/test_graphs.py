# -
# *****************************************************************************
# Copyright 2022 Autodesk, Inc.
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
import argparse
import logging
import io
import json
import os
import re
from subprocess import Popen, PIPE
import stat
import sys
import tempfile
import unittest


class TestGraphs(unittest.TestCase):
    """Main class for testing multiple compounds."""

    @classmethod
    def setUpClass(cls):
        cls.temp_dir_object = tempfile.TemporaryDirectory(
            prefix="BifrostUsd_TestGraphs_"
        )
        logging.debug(
            "Created a new temporary directory = `%s`",
            cls.temp_dir_object.name
        )

    @classmethod
    def tearDownClass(cls):
        if cls.temp_dir_object:
            logging.debug(
                "Deleting temporary directory = `%s`...",
                cls.temp_dir_object.name
            )
            cls.temp_dir_object.cleanup()
            del cls.temp_dir_object

    def __init__(
        self,
        test_name,
        bifcmd_executable,
        bifrost_lib_config_files,
        test_dir,
        resource_dir,
    ):
        super(TestGraphs, self).__init__(test_name)
        self.bifcmd_exec = bifcmd_executable
        self.lib_config_files = bifrost_lib_config_files
        self.test_dir = test_dir
        self.resource_dir = resource_dir

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def get_bifcmd_env(self):
        """Get environment to use with bifcmd"""
        bifcmd_env = os.environ.copy()
        config_list = ""
        for config_file in self.lib_config_files:
            config_list += config_file + os.pathsep
        bifcmd_env["BIFROST_LIB_CONFIG_FILES"] = config_list
        return bifcmd_env

    def run_bifcmd(self, command_to_run):
        """Run a compound by bifcmd"""
        my_proc = Popen(
            command_to_run, stdout=PIPE, stderr=PIPE, env=self.get_bifcmd_env()
        )
        command_output = my_proc.communicate()

        # To be compatible with python 3, decode('utf-8') is necessary.
        return (
            my_proc.returncode,  # returncode
            command_output[0].decode("utf-8"),  # stdout_data
            command_output[1].decode("utf-8"),  # stderr_data
        )

    # ==========================================================================
    # Helper functions for running tests using Task Descriptions
    # ==========================================================================

    def reportErrorMessages(self, messages, prefix):
        for msg in messages:
            logging.error("%s%s", prefix, msg)

    def checkErrorMessages(self, msg_by_reporters, prefix):
        success = True
        for reporter in list(msg_by_reporters):
            prefix2 = prefix + "[" + reporter + "] "
            msg_by_categories = msg_by_reporters[reporter]
            has_criticals = ("critical" in msg_by_categories) and len(
                msg_by_categories["critical"]
            ) > 0
            if has_criticals:
                self.reportErrorMessages(
                    msg_by_categories["critical"], prefix2 + "Critical: "
                )
            has_errors = ("error" in msg_by_categories) and len(
                msg_by_categories["error"]
            ) > 0
            if has_errors:
                self.reportErrorMessages(
                    msg_by_categories["error"], prefix2 + "Error: "
                )
            success = success and not has_errors and not has_criticals
        return success

    def checkStandardOutputPorts(self, task_report, prefix):
        self.assertTrue("outputPortValueFormat" in task_report)
        self.assertEqual(task_report["outputPortValueFormat"], "summary")
        self.assertTrue("outputPorts" in task_report)
        output_ports = task_report["outputPorts"]
        has_test_info = False
        has_failure_msgs = False
        success = True
        for port in output_ports:
            self.assertTrue("name" in port)
            self.assertTrue("type" in port)
            self.assertTrue("value" in port)
            if port["name"] == "test_info":
                has_test_info = True
                expected_type = "string"
                if not port["type"] == expected_type:
                    logging.error(
                        "%sOutput port `%s` has type `%s` instead of `%s`",
                        prefix,
                        port["name"],
                        port["type"],
                        expected_type,
                    )
                    success = False
            elif port["name"] == "failure_msgs":
                has_failure_msgs = True
                expected_type = "array<string>"
                if not port["type"] == expected_type:
                    logging.error(
                        "%sOutput port `%s` has type `%s` instead of `%s`",
                        prefix,
                        port["name"],
                        port["type"],
                        expected_type,
                    )
                    success = False
                else:
                    value = port["value"]
                    match = re.search(
                        # find array size information, also expecting that all
                        # messages have been printed:
                        r"[^(]*\((?P<size>[0-9]*) out of (?P=size) string\)"
                        # find `[`:
                        r"[\s]*\[",
                        value,
                    )
                    ok = True
                    if match is None:
                        ok = False
                        logging.error(
                            "%sUnable to retrieve array size info in " +
                            "`failure_msgs`. Aborting.",
                            prefix,
                        )
                    else:
                        # Retrieve the number of msgs and their starting pos
                        nb_messages = int(match.group("size"))
                        start = match.end()

                        # Compile regular expressions for intermediate
                        # messages (i) and last message (n) of the array:
                        fail_pat_i = re.compile(r'[\s]*"([^"]+)"[\s]*,')
                        fail_pat_n = re.compile(r'[\s]*"([^"]+)"')
                        succ_pat_i = re.compile(r'[\s]*""[\s]*,')
                        succ_pat_n = re.compile(r'[\s]*""')

                        # Check each failure_msg
                        abort = False
                        for i in range(0, nb_messages):
                            last = i == nb_messages - 1
                            # Check if it matches a non-empty message:
                            if not last:
                                msg_match = fail_pat_i.match(value, start)
                            else:
                                msg_match = fail_pat_n.match(value, start)
                            if msg_match is not None:
                                # It matches a non-empty message: means FAILURE
                                ok = False
                                # Log the message itself:
                                logging.error(
                                    "%sfailure_msgs[%d of %d]=%s",
                                    prefix,
                                    i + 1,
                                    nb_messages,
                                    msg_match.group(1),
                                )
                                start = msg_match.end()
                            else:
                                # Otherwise it should match an empty message:
                                if not last:
                                    msg_match = succ_pat_i.match(value, start)
                                else:
                                    msg_match = succ_pat_n.match(value, start)
                                if msg_match is not None:
                                    # It matches an empty msg: means SUCCESS
                                    start = msg_match.end()
                                else:
                                    # Did not match empty nor non-empty msg.
                                    # Unable to check following msgs either.
                                    # Stop here.
                                    ok = False
                                    abort = True
                                    logging.error(
                                        "%sUnable to retrieve " +
                                        "failure_msgs[%d of %d]. Aborting.",
                                        prefix,
                                        i + 1,
                                        nb_messages,
                                    )
                                    break

                        if not abort:
                            # find `]` that closes the failure_msgs array
                            last_pattern = re.compile(r"[\s]*\]")
                            closing_match = last_pattern.match(value, start)
                            if closing_match is None:
                                ok = False
                                logging.error(
                                    "%sUnable to retrieve closing ']' " +
                                    "in `failure_msgs` array.",
                                    prefix,
                                )
                    success = success and ok

        if not has_test_info:
            logging.error("%sOutput port `test_info` is missing", prefix)
            success = False
        if not has_failure_msgs:
            logging.error("%sOutput port `failure_msgs` is missing", prefix)
            success = False
        return success

    def checkTasksResults(self, json_content, check_std_outputs):
        indent = "   "
        indent2 = indent + indent

        # Make sure we support the schema version of the JSON log file
        self.assertTrue("schemaVersion" in json_content)
        self.assertEqual(json_content["schemaVersion"], "1.0.0")

        # Scan content of JSON Log File
        success = True
        for desc in json_content["taskDescriptions"]:
            # Check if Task Description file was ok, and any error messages
            self.assertTrue("jsonFile" in desc)
            task_desc_file = desc["jsonFile"]
            logging.info(
                "Checking results for Task Description file `%s`...",
                task_desc_file
            )
            for flag in ["jsonFileLoadedOk", "jsonFileParsedOk"]:
                if not desc[flag] is True:
                    logging.error("%s%s: False", indent, flag)
                    success = False
            msg_by_reporters = desc["messages"]
            if not self.checkErrorMessages(msg_by_reporters, indent):
                success = False
            if success:
                # Check every Task Report for this Task Description
                for task_report in desc["taskReports"]:
                    # Check if execution was ok, and any error messages
                    task_name = task_report["taskName"]
                    task_id = task_report["taskId"]
                    logging.info(
                        "%sChecking results for taskId=%d taskName=`%s`...",
                        indent,
                        task_id,
                        task_name,
                    )
                    for flag in [
                        "parametersOk",
                        "definitionFilesLoadedOk",
                        "compoundLoadedOk",
                        "compilationOk",
                        "executionOk",
                    ]:
                        if not task_report[flag] is True:
                            logging.error("%s%s: False", indent2, flag)
                            success = False
                    msg_by_reporters = task_report["messages"]
                    if not self.checkErrorMessages(msg_by_reporters, indent2):
                        success = False

                    # Then check standard outputPorts
                    if check_std_outputs and not self.checkStandardOutputPorts(
                        task_report,
                        indent2
                    ):
                        success = False
        return success

    def checkJSONLogFile(self, json_log_file, check_std_outputs):
        # Open and read JSON file
        logging.debug("Checking JSON Log File `%s`...", json_log_file)
        success = False
        if not os.path.exists(json_log_file):
            logging.error("JSON Log File `%s` does not exist.", json_log_file)
        elif os.path.getsize(json_log_file) == 0:
            logging.error("JSON Log File `%s` is empty.", json_log_file)
        else:
            f = io.open(json_log_file)
            json_content = json.load(f)
            f.close()
            success = self.checkTasksResults(json_content, check_std_outputs)
        return success

    # ==========================================================================
    # Tests
    # ==========================================================================

    def test_multipleTaskDescriptions(self):
        json_log_file = os.path.join(
            self.temp_dir_object.name, "bifcmdLogFile.json"
        )
        logging.info("JSON Log File = %s", json_log_file)

        task_desc_files = [
            # Integration Tests:
            "tasks_landscape_generator.json",
            "tasks_load_all_compounds.json",
            # IO Tests:
            "tasks_read_usd_curves.json",
            "tasks_read_usd_curves_with_color.json",
            "tasks_read_usd_meshes.json",
            "tasks_read_usd_meshes_with_color.json",
            # Layer Tests:
            "tasks_create_empty_layer.json",
            "tasks_export_layer_to_file.json",
            # Primitive Tests
            "tasks_create_prim_hierarchy.json",
            # Stage Tests
            "tasks_add_to_stage.json",
            "tasks_fan_out_stages.json",
            # VariantSet Tests
            "tasks_create_variants.json",
        ]
        cmd = [
            self.bifcmd_exec,
            "--log-level",
            "verbose",
            "--print-count",
            "1000",
            # Redirect all reporting to a specific JSON log file:
            "--log-file",
            json_log_file,
            # Option Ports that are required by some Tasks:
            "--option-port",
            "root_temporary_dir",
            self.temp_dir_object.name,
            "--option-port",
            "root_resource_dir",
            self.resource_dir,
        ]

        # Prepare to launch all Task Descriptions at once:
        for file in task_desc_files:
            task_desc_pathname = os.path.join(
                self.test_dir, "taskDescriptions", file
            )
            cmd += ["--task-description", task_desc_pathname]

        # Launch all Tasks in sequence but in a single run of bifcmd:
        return_code, _, _ = self.run_bifcmd(cmd)
        return_code_ok = (return_code == 0)
        if not return_code_ok:
            logging.error("bifcmd return code=%d", return_code)

        # Inspect the bifcmd's log file to verify all Tasks results:
        check_std_outputs = True
        log_file_ok = self.checkJSONLogFile(json_log_file, check_std_outputs)

        #logging.info("\nOUTPUTS=\n%s\n", outputs)
        #logging.info("\nERRORS=\n%s\n", errors)

        success = return_code_ok and log_file_ok
        self.assertTrue(success)

class BifCmdException(Exception):
    """Exception class for bifcmd executable related errors"""


def get_suite(
    bifcmd_executable, bifrost_lib_config_files, test_dir, resource_dir,
    test_names=None
):
    """Create and return the test suite"""
    # Obtain all test names
    test_loader = unittest.TestLoader()
    if not test_names:
        test_names = test_loader.getTestCaseNames(TestGraphs)
    # Create the TestSuite and initialize each test with required arguments:
    my_suite = unittest.TestSuite()
    for test_name in test_names:
        my_suite.addTest(
            TestGraphs(
                test_name,
                bifcmd_executable,
                bifrost_lib_config_files,
                test_dir,
                resource_dir,
            )
        )

    return my_suite


def parse_arguments():
    """Parse the command line arguments, returns the arg parser"""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--bifcmd-location",
        dest="bifcmd_location",
        help="The fully-qualified path to the directory"
        " where is installed the Bifrost's bifcmd executable.",
        required=True,
    )
    parser.add_argument(
        "--bifrost-lib-config-files",
        dest="bifrost_lib_config_files",
        help="The fully-qualified path(s) to the configuration file(s)"
        " to define folders for test graphs and their required"
        " compounds.",
        nargs="*",
        default=[],
        required=True,
    )
    parser.add_argument(
        "--resource-dir",
        dest="resource_dir",
        help="The directory storing resources like usd or png files",
        required=True,
    )
    parser.add_argument(
        "--test-dir",
        dest="test_dir",
        help="The directory storing test data, like `compounds` and " +
             "`taskDescriptions` sub-folders.",
        required=True,
    )
    parser.add_argument(
        "--testcase-names",
        dest="testcase_names",
        help="The name of the tests you want to run."
        "If not set, all the tests will be executed.",
        nargs="*",
        default=[],
        required=False,
    )
    parser.add_argument(
        "--debug", help="To enable debug logging.", action="store_true"
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_arguments()

    # Configure logging:
    logging.basicConfig(
        format="%(levelname)s: %(message)s",
        level=(logging.DEBUG if args.debug else logging.INFO),
    )

    # Fully-qualified pathname to the bifcmd executable:
    bifcmd_pathname = None
    # Config Files to be loaded by Bifrost for test compounds and their
    # required compound locations:
    lib_config_files = []

    bifcmd_dir = None
    if args.bifcmd_location:
        bifcmd_dir = args.bifcmd_location
    if args.bifrost_lib_config_files:
        lib_config_files = args.bifrost_lib_config_files

    if (
        bifcmd_dir is None
        or not os.path.isdir(bifcmd_dir)
        or not os.path.isabs(bifcmd_dir)
    ):
        # For maximum reliability, Python recommends to use a fully-qualified
        # path to the child program to launch. So if the bifcmd_location arg
        # is not a fully-qualified path, we exit:
        raise BifCmdException(
            f"--bifcmd-location argument `{bifcmd_dir}` is not a " \
            f"fully-qualified path to a directory."
        )

    bifcmd_filename = "bifcmd" + (".exe" if os.name == "nt" else "")
    bifcmd_pathname = os.path.join(bifcmd_dir, bifcmd_filename)

    logging.info("Bifrost's bifcmd executable = `%s`", bifcmd_pathname)
    logging.info("Bifrost Lib Config Files    = `%s`", lib_config_files)

    if not os.path.isfile(bifcmd_pathname):
        raise BifCmdException(
            f"`{bifcmd_filename}` executable cannot be found in directory " \
            f"`{bifcmd_dir}`."
        )

    bifcmd_statinfo = os.stat(bifcmd_pathname)
    if not bifcmd_statinfo.st_mode & stat.S_IXUSR:
        raise BifCmdException(
            f"ERROR: permission to execute `{bifcmd_pathname}` is denied."
        )

    runner = unittest.TextTestRunner()
    suite = get_suite(
        bifcmd_pathname,
        lib_config_files,
        args.test_dir,
        args.resource_dir,
        args.testcase_names,
    )
    testResult = runner.run(suite)

    sys.exit(0 if testResult.wasSuccessful() else 1)
