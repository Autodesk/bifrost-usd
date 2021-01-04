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

"""Command line interface to difflib.py providing diffs in four formats:

* ndiff:    lists every line and highlights interline changes.
* context:  highlights clusters of changes in a before/after format.
* unified:  highlights clusters of changes in an inline format.
* html:     generates side by side comparison with change highlights.

The script also supports a mode that automatically updates the files
representing the expected output of regression tests. The mode is enabled by
setting the BIFUSD_DIFF_UPDATE_EXPECTED_FILE environment variable to a
non-zero value. That would typically be done when invoking 'ctest' to run the
regression tests. In that case, if 'diff.py' detects a comparison failure, it
will copy the 'from_file' to the 'to_file' thus updating the expected file. The
comparison is still made to avoid updating the expected files when the only
differences are being filtered out. The developer can then review the updated
expected files in 'git' to validate the new baseline.
"""

import argparse
import difflib
import errno
import os
import re
import shutil
import sys
import time

#-------------------------------------------------------------------------------
#
def parse_args():
    """Parse the arguments"""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__)

    parser.add_argument(
        "-c", action="store_true", default=False,
        help='Produce a context format diff (default)')
    parser.add_argument(
        "-u", action="store_true", default=False,
        help='Produce a unified format diff')
    parser.add_argument(
        "-m", action="store_true", default=False,
        help='Produce HTML side by side diff (can use -c and -l in conjunction)')
    parser.add_argument(
        "-n", action="store_true", default=False,
        help='Produce a ndiff format diff')
    parser.add_argument(
        "-l", "--lines", type=int, default=3,
        help='Set number of context lines (default 3)')
    parser.add_argument(
        "--from_subst", nargs=2, metavar=('pattern', 'subst'), action='append',
        help='Substitution regex pattern to apply to the "from_file" before '
        'comparing files')
    parser.add_argument(
        "--to_subst", nargs=2, metavar=('pattern', 'subst'), action='append',
        help='Substitution regex pattern to apply to the "to_file" before '
        'comparing files')

    parser.add_argument(
        '-D', dest='defines', metavar='VAR=value', action='append',
        help='Variable replacement')

    parser.add_argument(
        'from_file_name', type=str, metavar='from_file',
        help='to file')

    parser.add_argument(
        'to_file_name', type=str, metavar='to_file',
        help='to file')

    args = parser.parse_args()

    return args

#-------------------------------------------------------------------------------
#
def apply_macro_substitutions(filename, lines, defines):
    """Apply all macro substitutions"""

    if not defines:
        return lines

    replacements = dict()
    for repl in defines:
        i = repl.find('=')
        if i != -1:
            var = repl[:i]
            val = repl[i+1:]
            replacements[var] = val

    pattern = re.compile(r'\$\{(\w+)\}')
    def replace(x):
        return replacements[x.group(1)]

    linenum = 0
    result = []
    for line in lines:
        try:
            linenum += 1
            newline = pattern.sub(replace, line)
            result.append(newline)
        except KeyError as e:
            sys.stderr.write("%s:%d:1: error: Undefined variable %s in line:\n" %
                             (filename, linenum, e.args[0]))
            sys.stderr.write(line)

    return result

#-------------------------------------------------------------------------------
#
def apply_pattern_substitutions(filename, lines, pattern_subst_list):
    """Apply all pattern substitutions"""

    if not pattern_subst_list:
        return lines

    pattern_substs = [(re.compile(pattern), subst)
                      for pattern, subst in pattern_subst_list]

    result = []
    for line in lines:
        newline = line
        for pattern, subst in pattern_substs:
            def replace(x):
                # FIXME: It would be nice to eventually have sed-like
                # substitutions. Might consider using the pysed package. That
                # would probably also allow us to efficiently apply all
                # substitutions at once instead of doing a for loop.
                return subst

            newline = pattern.sub(replace, newline)
        # Don't append a line which is entirely empty after substitution.
        # This allows one to completely ignore some lines.
        if newline:
            result.append(newline)

    return result

#-------------------------------------------------------------------------------
#
def main():
    args = parse_args()

    n         = args.lines
    from_name = args.from_file_name
    to_name   = args.to_file_name

    # Should the 'to_file' be upated ?
    update_expect_file = (
        'BIFUSD_DIFF_UPDATE_EXPECTED_FILE' in os.environ and
        int(os.environ['BIFUSD_DIFF_UPDATE_EXPECTED_FILE']) > 0)

    try:
        from_file = open(from_name, 'r', newline=None,
                        encoding='utf-8',
                        errors='surrogateescape')
    except IOError as e:
        raise

    try:
        to_file = open(to_name, 'r', newline=None,
                        encoding='utf-8',
                        errors='surrogateescape')
    except IOError as e:
        if e.errno == errno.ENOENT and update_expect_file:
            # No such file or directory
            sys.stderr.write(
                "The file 'to_file' doesn't exist. Copying '%s' to '%s'.\n" %
                (from_name, to_name))
            shutil.copyfile(from_name, to_name)
            return 1
        raise

    # we're passing these as arguments to the diff function
    from_date = time.ctime(os.stat(from_name).st_mtime)
    to_date   = time.ctime(os.stat(to_name).st_mtime)

    from_lines = from_file.readlines()
    to_lines   = to_file.readlines()

    to_lines = apply_macro_substitutions(to_name, to_lines, args.defines)

    from_lines = apply_pattern_substitutions(
        from_name, from_lines, args.from_subst)
    to_lines   = apply_pattern_substitutions(
        to_name,   to_lines,   args.to_subst)

    if args.u:
        diff = difflib.unified_diff(
            from_lines, to_lines, from_name, to_name,
            from_date, to_date, n=n)
    elif args.n:
        diff = difflib.ndiff(from_lines, to_lines)
    elif args.m:
        diff = difflib.HtmlDiff().make_file(
            from_lines, to_lines, from_name, to_name,
            context=args.c, numlines=n)
    else:
        diff = difflib.context_diff(
            from_lines, to_lines, from_name, to_name,
            from_date, to_date, n=n)

    # Materialize the generator
    diff = list(diff)

    if not update_expect_file:
        for diff_line in diff:
            diff_line_utf8 = diff_line.encode( encoding='utf-8', errors='replace')
            sys.stdout.writelines(diff_line_utf8)
    else:
        if (diff):
            sys.stderr.write("Files differ. Updating '%s' with '%s'.\n" %
                             (from_name, to_name))
            shutil.copyfile(from_name, to_name)

    if (diff):
        return 1
    else:
        return 0

#-------------------------------------------------------------------------------
#
if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(1)
