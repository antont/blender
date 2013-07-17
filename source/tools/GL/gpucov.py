#!/usr/bin/env python
# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

import sys
import os
import re
import shutil
import subprocess

# for_all_files will visit every file in a file hierarchy
# doDirCallback  - called on each directory
# doFileCallback - called on each file
# doSkipDir      - should return true if a directory should be skipped
# doSkipFile     - should return true if a file should be skipped

def for_all_files(dir, doDirCallback, doFileCallback, doSkipDir, doSkipFile):
    for_all_files_(dir, dir, doDirCallback, doFileCallback, doSkipDir, doSkipFile)

def for_all_files_(prefix, dir, doDirCallback, doFileCallback, doSkipDir, doSkipFile):
    unprefixed_dir = dir[len(prefix):]

    if doDirCallback:
        doDirCallback(dir, unprefixed_dir)

    subdirlist = []

    for item in os.listdir(dir):
        path = os.path.join(dir, item)

        unprefixed_path = path[len(prefix):]

        if os.path.isfile(path) and (not doSkipFile or not doSkipFile(path, unprefixed_path, item)):
            if doFileCallback:
                doFileCallback(path, unprefixed_path, item)
        elif os.path.isdir(path) and (not doSkipDir or not doSkipDir(path, unprefixed_path, item)):
            subdirlist.append(path)

    subdirlist.sort()

    for subdir in subdirlist:
        for_all_files_(prefix, subdir, doDirCallback, doFileCallback, doSkipDir, doSkipFile)



def printDirectory(dir, unprefixed_dir):
    print("Entering: " + unprefixed_dir + " ...")



def printFilename(path, unprefixed_path, item):
    print(os.path.basename(path))



def never(path, unprefixed_path, item):
    return False


def always(path, unprefixed_path, item):
    return True


def isNotGcdaFile(path, unprefixed_path, item):
    (root, ext) = os.path.splitext(path)
    return not ext == ".gcda"

def isNotGcnoFile(path, unprefixed_path, item):
    (root, ext) = os.path.splitext(path)
    return not ext == ".gcno"

def isNotGcovFile(path, unprefixed_path, item):
    (root, ext) = os.path.splitext(path)
    return not ext == ".gcov"

def isNotCGcovFile(path, unprefixed_path, item):
    (path, ext1) = os.path.splitext(path)
    (path, ext2) = os.path.splitext(path)
    return not (ext1 == ".gcov" and (ext2 in [".c", ".cpp"]))

cmakedirs = {}

def addCmakeDir(abs, unprefixed_path, item):
    global cmakedirs

    path = abs
    rel = ""

    while True:
        (head, tail) = os.path.split(path)

        if tail == "":
            break

        if tail == "CMakeFiles":
            if not rel in cmakedirs:
                cmakedirs[rel] = set()

            cmakedirs[rel].add(abs)

            #print("added " + abs + " to " + rel)

        if not os.path.isfile(path):
            rel = os.path.join(tail, rel)

        path = head



def ensure_dir(f):
    d = os.path.dirname(f)
    if not os.path.exists(d):
        os.makedirs(d)



def usage():
    sys.exit("usage: " + sys.argv[0] + " gcno-dir gcda-dir out-dir")



tokenizer1 = re.compile('^\s*#####:\s*[0-9]+:.*gpu[A-Z][a-zA-Z0-9_]*.*$', flags = re.MULTILINE)
tokenizer2 = re.compile('^\s*[0-9]+:\s*[0-9]+:.*gpu[A-Z][a-zA-Z0-9_]*.*$', flags = re.MULTILINE)

covered = {}
uncovered = {}

covered_count = 0
uncovered_count = 0

def findUncovered(path, unprefixed_path, item):
    global tokenizer1
    global tokenizer2
    global covered_count
    global uncovered_count

    f = open(path)
    s = f.read()
    f.close()

    matches1 = tokenizer1.findall(s)
    matches2 = tokenizer2.findall(s)

    if matches1:
        uncovered[path] = matches1
        uncovered_count = uncovered_count + len(matches1)

    if matches2:
        covered[path] = matches2
        covered_count = covered_count + len(matches2)

def removeFile(path, unprefixed_path, item):
    os.remove(path)



if len(sys.argv) != 4:
    usage()

if not os.path.isdir(sys.argv[1]):
    usage()

if not os.path.isdir(sys.argv[2]):
    usage()

for_all_files(sys.argv[1], None, addCmakeDir, never, isNotGcnoFile)
for_all_files(sys.argv[2], None, addCmakeDir, never, isNotGcdaFile)

for rel in cmakedirs:
    for filename in cmakedirs[rel]:
        dst_path = os.path.join(sys.argv[3], rel)
        base = os.path.basename(filename)
        dst_filepath = os.path.join(dst_path, base)

        ensure_dir(dst_path)
        shutil.copyfile(filename, dst_filepath)

for rel in cmakedirs:
    for filename in cmakedirs[rel]:
        dst_path = os.path.join(sys.argv[3], rel)
        base = os.path.basename(filename)
        (_, ext) = os.path.splitext(base)

        if ext == ".gcda":
            subprocess.Popen(["gcov", base], cwd=dst_path).wait()

            for_all_files(dst_path, None, findUncovered, always, isNotCGcovFile)
            for_all_files(dst_path, None, removeFile, always, isNotGcovFile)

for path in uncovered:
    uncov = str(len(uncovered[path]))

    if path in covered:
        cov = str(len(covered[path]))
    else:
        cov = "0"

    print(path + ": [" + uncov + " uncovered]\n" + "\n".join(uncovered[path]) + "\n")

    if path in covered:
        print(path + ": [" + cov + " covered]\n" + "\n".join(covered[path]) + "\n")

print("total: [" + str(uncovered_count) + " uncovered, " + str(covered_count) + " covered]")
