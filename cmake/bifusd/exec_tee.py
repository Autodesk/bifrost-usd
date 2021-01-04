#!/usr/bin/env python

#-
#*****************************************************************************
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
#*****************************************************************************
#+

"""exec_tee.py - execute a command and capture its output

This is useful for capturing the output of a test executed by CMake. The output
can then be validated by FileCheck for example.

stdout is captured to the 'file' passed as an argument. It is also echoed back
to stdout. stderr can also be optionally captured.
"""

import argparse
import errno
import os
import os.path
import subprocess # nosec
import sys

#-------------------------------------------------------------------------------
#
def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__)

    parser.add_argument('--stderr', action='store_true', help='also capture stderr')

    parser.add_argument('file', help='output file')
    parser.add_argument('exe',  help='name of the executable')
    parser.add_argument('args', nargs='*', help='arguments')

    args = parser.parse_args()

    return args



#-------------------------------------------------------------------------------
#
def mkdir_p(path):
    """Equivalent to the unix "mkdir -p" command."""
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

#-------------------------------------------------------------------------------
#
def main():
    args = parse_args()

    command = [args.exe] + args.args
    dirpath = os.path.dirname(args.file)
    if dirpath:
        mkdir_p(dirpath)

    with open(args.file, "w") as fout:
        try:
            status = subprocess.call( # nosec
                [args.exe] + args.args,
                stdout=fout,
                stderr=subprocess.STDOUT if args.stderr else None)
        except Exception as e:
            print(f"{' '.join(command)}: {e}", sep='')
            sys.exit(1)

    fout = open(args.file, "r", encoding='utf-8',
                        errors='surrogateescape')

    for line in fout:
        line_utf8 = line.encode( encoding='utf-8', errors='replace')
        print(line_utf8, end='')

    return status;

#-------------------------------------------------------------------------------
#
if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(1)
