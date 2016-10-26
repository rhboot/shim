#
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
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# In *inux environment, the build tools's source is required and need to be compiled
# firstly, please reference https://edk2.tianocore.org/unix-getting-started.html to 
# to get how to setup build tool.
#
# After build tool is downloaded and compiled, a soft symbol linker need to be created
# at <workspace>/Conf. For example: ln -s /work/BaseTools /work/edk2/Conf/BaseToolsSource.
#
# Setup the environment for unix-like systems running a bash-like shell.
# This file must be "sourced" not merely executed. For example: ". edksetup.sh"
#
# CYGWIN users: Your path and filename related environment variables should be
# set up in the unix style.  This script will make the necessary conversions to
# windows style.
#
# Please reference edk2 user manual for more detail descriptions at https://edk2.tianocore.org/files/documents/64/494/EDKII_UserManual.pdf
#

if [ \
     "$1" = "-?" -o \
     "$1" = "-h" -o \
     "$1" = "--help" \
   ]
then
  echo BaseTools Usage: \'. edksetup.sh\'
  echo
  echo Please note: This script must be \'sourced\' so the environment can be changed.
  echo \(Either \'. edksetup.sh\' or \'source edksetup.sh\'\)
  return
fi

if [ -z "$WORKSPACE" ]
then
  . BaseTools/BuildEnv $*
else
  . $WORKSPACE/BaseTools/BuildEnv $*
fi


