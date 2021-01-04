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

"""configure-file.py -- Copy a file to another location and modify its contents.

Copies a file <src_file> to file <dst_file> and substitutes variable values
referenced in the file content. If <src_file> is a relative path it is evaluated
with respect to the current source directory. The <src_file> must be a file, not a
directory. If <dst_file> is a relative path it is evaluated with respect to the
current binary directory. If <dst_file> names an existing directory the input
file is placed in that directory with its original name.

This command replaces any variables in the input file referenced as ${VAR} with
their replacement values passed trough -DVAR=value. If a variable is not
defined, an error will be thrown.

"""

import argparse
import pprint
import re
import sys
import os
import shutil

#-------------------------------------------------------------------------------
#
def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__)

    parser.add_argument(
        '-D', dest='defines', metavar='VAR=value', action='append',
        help='Variable replacement')
    parser.add_argument(
        '--preprocessor-line', action='store_true',
        help='Add a #line directive pointing at the original source file')

    parser.add_argument(
        'src_filename',
        help='source file')

    parser.add_argument(
        'dst_filename',
        help='destination file')

    args = parser.parse_args()

    return args

#-------------------------------------------------------------------------------
#
def main():
    pargs = parse_args()

    if not os.path.exists(pargs.src_filename):
        return 1

    if not os.path.exists(pargs.dst_filename):
        try:
            os.makedirs(os.path.dirname(pargs.dst_filename))
        except OSError as e:
            if not os.path.isdir(os.path.dirname(pargs.dst_filename)):
                raise
        shutil.copyfile(pargs.src_filename, pargs.dst_filename)


    src_file = open(pargs.src_filename, "r", encoding='utf-8',
                                                errors='surrogateescape')
    dst_file = open(pargs.dst_filename, "w", encoding='utf-8',
                                                errors='surrogateescape')

    if pargs.preprocessor_line:
        dst_file.write('#line 1 "%s"\n' % pargs.src_filename)

    replacements = dict()
    for repl in pargs.defines or []:
        i = repl.find('=')
        if i != -1:
            var = repl[:i]
            val = repl[i+1:]
            replacements[var] = val

    pattern = re.compile(r'\$\{(\w+)\}')
    def replace(x):
        return replacements[x.group(1)]

    retval = None
    linenum = 0

    for line in src_file:
        try:
            linenum += 1
            newline = pattern.sub(replace, line)
            dst_file.write(newline)
        except KeyError as e:
            sys.stderr.write("%s:%d:1: error: Undefined variable %s in line:\n" %
                             (pargs.src_filename, linenum, e.args[0]))

            # Force utf-8
            line_utf8 = line.encode( encoding='utf-8', errors='replace')
            sys.stderr.write(line_utf8)

            retval = 1

    return retval


#-------------------------------------------------------------------------------
#
if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(1)
