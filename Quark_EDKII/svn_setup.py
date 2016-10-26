#!/usr/bin/env python

# Copyright (c) 2013 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Intel Corporation nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Marc Herbert

from __future__ import print_function

import os
import shutil
import stat
import sys
import subprocess
import time
import tempfile

DEBUG=False
SVN_EXTERNALS_FILE = "svn_externals.txt"


def isWindows():
    return os.name == 'nt'

def info(*objs):
    print("INFO: ",    *objs, end='\n', file=sys.stdout)
    sys.stdout.flush()
def warning(*objs):
    print("WARNING: ", *objs, end='\n', file=sys.stdout)
    sys.stdout.flush()
def error(*objs):
    print("ERROR: ",   *objs, end='\n', file=sys.stderr)
    sys.stderr.flush()

def die(*objs):
    error(*objs)
    if isWindows():
        subprocess.call("cmd /c PAUSE")
    sys.exit(1)

#import inspect
#info("Running ", os.path.abspath(inspect.getsourcefile(mainf)))    

DOTDOT = os.path.dirname(os.getcwd())
CLONE_BASENAME = os.path.basename(os.getcwd())
ADHOC  = CLONE_BASENAME + "-svn_externals.repo"
# '@' is a special character for subversion (peg revisions)
ADHOC = ADHOC.replace('@', '_at_')
ADHOC_PATH = os.path.join(DOTDOT, ADHOC)

# info("Parent directory:", DOTDOT)
info("Ad-hoc SVN repo:", ADHOC_PATH)


def my_system(args):

    if DEBUG:
        info("About to run:", args)
        time.sleep(2)
    else:
        info("Running:", ' '.join(args))

    # - stdout: Python has no portable, non-blocking read operation on
    # pipes so we cannot capture long, progress information on the
    # fly. So we just let stdout be inherited.
    # http://www.python.org/dev/peps/pep-3145/
        
    # -stderr: On the other hand we assume stderr will be small and
    # final and want to know if there is any error message all, so we
    # block-read on it.  As a bonus feature this gets rid of the extra
    # command window on Windows.
    child = subprocess.Popen(args, stderr=subprocess.PIPE)
    err = child.stderr.read()
    child.wait()

    if err:
        die("stderr output of: ", args, "\n", err)

    if child.returncode:
        die(args, "returned: ", child.returncode)

    if DEBUG:
        info("Done running:", args)
        time.sleep(2)


def setup_externals():
    
    # Try something harmless first to make sure the command line is
    # installed and in the PATH
    try:
        info("SVN version:")
        subprocess.check_call(["svn", "--version", "--quiet"])
    except BaseException as e:
        print (type(e).__name__ + ":", e)
        die("Failed to run Subversion command line, please make sure it is installed and in the PATH")

    # Make room
    for old_dir in [ '.svn', ADHOC_PATH ]:
        if os.path.exists(old_dir):
            warning("Removing older", old_dir)
            for p, dirs, files in os.walk(old_dir):
                for f in files:
                    # print (os.path.join(p,f))
                    os.chmod(os.path.join(p,f), stat.S_IWRITE)

            shutil.rmtree(old_dir)


    my_system([ "svnadmin", "create", ADHOC_PATH ])

    my_system([ "svn", "checkout", "file:///" + ADHOC_PATH, os.path.join(os.getcwd(), ADHOC) ])

    # Don't do this at home
    shutil.move(os.path.join(ADHOC, '.svn'), '.')
    os.rmdir(ADHOC)

    # Note: if you want ever want to do this from TortoiseSVN graphical
    # interface go to the "New->Advanced->Load" menu item in the
    # Properties dialog box
    my_system([ "svn", "propset", "svn:externals", "-F", SVN_EXTERNALS_FILE, "." ])
    my_system([ "svn", "commit", "--depth", "empty", "--message", "Add svn:externals", "." ])


def get_external_dirs():
    "Parses the SVN_EXTERNALS_FILE and returns the list of directories from it"

    def get_dir(external):
        location = external.split()[-1]
        return os.path.split(location)[-1]

    with open(SVN_EXTERNALS_FILE) as f:
        return [ get_dir(lin) for lin in f ]


def parse_arguments():
    from optparse import OptionParser
    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("--gitignore", action="store_true", dest="gitignore",
                  help="Appends SVN objects to .gitignore (does not commit)")
    parser.add_option("--download", action="store_true", dest="download",
                  help="Runs svn update to download externals once setup is complete (takes time)")
    parser.add_option("--delete",  action="store_true", dest="wipeout",
                  help="Deletes everything that is or would be in the way of --download. \
This is a destructive way to help --download succeed. \
Does NOT imply --download: both can be used independently")

    (options, args) = parser.parse_args()

    # Gather parsed command line and defaults and define the parameters 
    global svn_update, wipeout, gitignore
    svn_update, wipeout, gitignore = options.download, options.wipeout, options.gitignore


def mainf(*m_args):
    # Overrides sys.argv when testing (interactive or in main below)
    if m_args:
        info("testing with args:", m_args)
        sys.argv = ["testing main"] + list(m_args)

    parse_arguments()

    setup_externals()

    # svn:ignore the stuff tracked by git
    # TODO: could this somehow work on Windows with Git Extensions?
    if os.path.exists(".git") and not isWindows():
        with tempfile.NamedTemporaryFile(prefix="setupSVNext_") as tempf:
            subprocess.check_call(["git", "ls-tree", "HEAD", "--name-only"], stdout=tempf)
            tempf.write(b'.git')
            my_system([ "svn", "propset", "svn:ignore", "-F", tempf.name, "." ])

    # .gitignore the stuff tracked by svn
    if gitignore:
        info("Updating .gitignore")
        with open (".gitignore", 'a') as gi:
            ignored = get_external_dirs() + [ '.svn' ]
            for dir in ignored:
                print('/' + dir, file=gi )

    if wipeout:
        ext_directories = get_external_dirs()
        for e in ext_directories:
            if os.path.exists(e):
                warning("Now removing previous ", e, "/")
                shutil.rmtree(e)

    if svn_update:
        my_system([ "svn", "update" ])


if __name__ == "__main__":
    if True: # make it False to run test code below
        ret = mainf()
    else:
        # Test/sample invocations (can test multiple in one run)
        # mainf("--help")
        ret = mainf(--gitignore)
        # ret = mainf("--wipeout", "--download")
        # mainf("--download")

    if isWindows():
        # We assume the user double-clicked on the script and we give a chance to see what happened
        # Before the window goes away
        subprocess.call("cmd /c PAUSE")

    sys.exit(ret)
