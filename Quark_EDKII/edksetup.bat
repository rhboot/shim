@REM @file
@REM   Windows batch file to setup a WORKSPACE environment
@REM
@REM Copyright (c) 2013 Intel Corporation.
@REM
@REM Redistribution and use in source and binary forms, with or without
@REM modification, are permitted provided that the following conditions
@REM are met:
@REM
@REM * Redistributions of source code must retain the above copyright
@REM notice, this list of conditions and the following disclaimer.
@REM * Redistributions in binary form must reproduce the above copyright
@REM notice, this list of conditions and the following disclaimer in
@REM the documentation and/or other materials provided with the
@REM distribution.
@REM * Neither the name of Intel Corporation nor the names of its
@REM contributors may be used to endorse or promote products derived
@REM from this software without specific prior written permission.
@REM
@REM THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
@REM "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
@REM LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
@REM A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
@REM OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
@REM SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
@REM LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
@REM DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
@REM THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
@REM (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
@REM OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM

@REM set CYGWIN_HOME=C:\cygwin

@REM usage: 
@REM   edksetup.bat [--nt32] [AntBuild] [Rebuild] [ForceRebuild] [Reconfig]
@REM if the argument, skip is present, only the paths and the
@REM test and set of environment settings are performed. 

@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #

@echo off

@REM
@REM Set the WORKSPACE to the current working directory
@REM
pushd .
cd %~dp0

if not defined WORKSPACE (
  @goto SetWorkSpace
)

if %WORKSPACE% == %CD% (
  @REM Workspace is not changed.
  @goto ParseArgs
)

:SetWorkSpace
@REM set new workspace
@REM set EFI_SOURCE, ECP_SOURCE and EDK_SOURCE for the new workspace
set WORKSPACE=%CD%
set EFI_SOURCE=%CD%
set EDK_SOURCE=%CD%
set ECP_SOURCE=%CD%

:ParseArgs

if not defined VS_VERSION (
  set VS_VERSION=
  set VSCOMNTOOLS=
    
  if defined VS90COMNTOOLS (
    set VS_VERSION=VS2008
    set VSCOMNTOOLS="%VS90COMNTOOLS%"
    call "%VS90COMNTOOLS%\vsvars32.bat"
  ) else (
    if defined VS80COMNTOOLS (
      set VS_VERSION=VS2005
      set VSCOMNTOOLS="%VS80COMNTOOLS%"
      call "%VS80COMNTOOLS%\vsvars32.bat"
    ) else (
      if defined VS71COMNTOOLS (
        set VS_VERSION=VS2003
        set VSCOMNTOOLS="%VS71COMNTOOLS%"
        call "%VS71COMNTOOLS%\vsvars32.bat"
      ) else (
        echo.
        echo !!! WARNING !!! Cannot find Visual Studio !!!
        echo.
      )
    )
  )
)

set VS_X86=
set VSCOMNTOOLS_TMP=%VSCOMNTOOLS:(x86)=%
if  %VSCOMNTOOLS_TMP% == %VSCOMNTOOLS% goto ParseContinue
set VS_X86=x86

:ParseContinue
@if /I "%1"=="-h" goto Usage
@if /I "%1"=="-help" goto Usage
@if /I "%1"=="--help" goto Usage
@if /I "%1"=="/h" goto Usage
@if /I "%1"=="/?" goto Usage
@if /I "%1"=="/help" goto Usage
@if /I not "%1"=="--nt32" goto no_nt32

@REM Flag, --nt32 is set
@REM The Nt32 Emluation Platform requires Microsoft Libraries
@REM and headers to interface with Windows.

shift

:no_nt32
@if /I "%1"=="NewBuild" shift
@if not defined EDK_TOOLS_PATH set EDK_TOOLS_PATH=%WORKSPACE%\BaseTools
@IF NOT EXIST "%EDK_TOOLS_PATH%\toolsetup.bat" goto BadBaseTools
@call %EDK_TOOLS_PATH%\toolsetup.bat %*
@if /I "%1"=="Reconfig" shift
@goto check_cygwin

:BadBaseTools
  @REM
  @REM Need the BaseTools Package in order to build
  @REM
  echo.
  echo !!! ERROR !!! The BaseTools Package was not found !!!
  echo.
  echo Set the system environment variable, EDK_TOOLS_PATH to the BaseTools,
  echo For example,
  echo   set EDK_TOOLS_PATH=C:\MyTools\BaseTools
  echo The setup script, toolsetup.bat must reside in this folder.
  echo.
  @goto end

:check_cygwin
  @if exist c:\cygwin (
    @set CYGWIN_HOME=c:\cygwin
  ) else (
    @echo.
    @echo !!! WARNING !!! No CYGWIN_HOME set, gcc build may not be used !!!
    @echo.
  )

@if NOT "%1"=="" goto Usage
@goto end

:Usage
  @echo.
  @echo  Usage: "%0 [-h | -help | --help | /h | /help | /?] [--nt32] [Reconfig]"
  @echo         --nt32         Call vsvars32.bat for NT32 platform build.
  @echo.
  @echo         Reconfig       Reinstall target.txt, tools_def.txt and build_rule.txt.
  @echo.
  @echo  Note that target.template, tools_def.template and build_rules.template
  @echo  will be only copied to target.txt, tools_def.txt and build_rule.txt
  @echo  respectively if they do not exist. Using option [Reconfig] to force the copy. 
  @echo.
  @goto end

:end
  @popd
  @echo on

