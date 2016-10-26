#!/usr/bin/env python

# Copyright(c) 2013 Intel Corporation. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

from __future__ import print_function
import os
import sys
#import logging
import subprocess
from optparse import OptionParser
from ConfigParser import ConfigParser, NoOptionError

def get_or_none(cfg, secname, propname):
    '''Converts an exception into None'''
    try:
         return cfg.get(secname, propname)
    except NoOptionError:
         return None

def get_options():
    '''Process the command line args'''

    parser = OptionParser(usage='usage: %prog [options]')
    parser.add_option('-c', '--config', action='store', dest='config',
                      default='upstream.cfg')
    parser.add_option('-u', '--url', action='store', dest='url',
                      help = "override server location with another (faster) mirror")
    parser.add_option('-d', '--depth', action='store', type='int', dest='depth',
                      default=3,
                      help = ("passed to git fetch to speed up the download. "
                              "Defaults to 3; set to 0 to disable. Requires a tag." )
                      )
    parser.add_option('--new-files-only', action='store_true', dest='new_files_only',
                      help = "Stops immediately after extracting brand new files " +
                      "from the patch(es)"
                      )
    parser.add_option('-w', '--work', action='store', dest='workdir', default = 'work',
                      help = "Sets the name of working directory"
                      )


    (opts, _) = parser.parse_args()
    return opts

def run_command(command):
    print('Running: ' + command)
    subprocess.check_call(command, shell=True)


def extract_newfiles(patches):

    for p in patches:

        dname = "new-files-from-patch-" + p[:-len('.patch')]
        print (dname)
        os.mkdir(dname)

        run_command('cd %s && >/dev/null patch --force -p1 < ../%s; test $? -lt 2' %
                    ( dname, p ) )

def main():

    opts = get_options()

    config = ConfigParser()
    if os.path.isfile(opts.config):
        config.read(opts.config)
    else:
        print('Config file doesnt exist: %s' % opts.config)
        sys.exit(-1)

    patches = [ p for p in os.listdir('.') if p.endswith('.patch') ]
    # Python orders characters based on Unicode codepoints, see
    # Reference->Expressions->Comparisons.
    patches.sort()

    extract_newfiles(patches)

    if opts.new_files_only:
        return

    packagename = config.get('upstream', 'NAME')
    workdir = opts.workdir
    url = opts.url if opts.url else config.get('upstream', 'URL')
    tag = get_or_none(config, 'upstream', 'TAG')
    sha = get_or_none(config, 'upstream', 'SHA')
    gitref = "tags/" + tag if tag else sha

    if os.path.isdir(workdir):
        print('ERROR: Directory already exists: %s' % workdir)
        sys.exit(-1)

    if tag:
        os.mkdir(workdir)
        os.chdir(workdir)
        run_command('git init')
        run_command('git remote add origin %s' % url)
        fetchspec = "%s:%s" % (gitref, gitref)
        run_command('git fetch origin --depth=%s %s' % (opts.depth, fetchspec))
    else:
        # We cannot use --depth here. Part of the explanation:
        # http://thread.gmane.org/gmane.comp.version-control.git/73368/focus=73994
        run_command('git clone %s %s' % (url, workdir))
        os.chdir(workdir)

    run_command('git checkout -b clanton %s' % gitref)

    if sha and tag and not opts.depth:
        git_rparse_cmd = 'test "$(git rev-parse %s~0)" = "%s"' % ( gitref, sha.strip() )
        run_command(git_rparse_cmd)

if __name__ == '__main__':
    main()
