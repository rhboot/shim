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

'''
PlatformDataPatch.py: tool for patching Flash.bin
with Platform Data Binary
Krzysztof.M.Sywula@intel.com
'''
VERSION = '0.1'
OFFSET_FROM_TOP = 0xF0000
PLATFORM_DATA_MAGIC = b'PDAT'

import sys
import struct
import binascii
import os
from shutil import copy
from optparse import OptionParser

try:
    from collections import OrderedDict
    DICTIONARY = OrderedDict
except ImportError:
    DICTIONARY = dict

try:
    import configparser
except ImportError:
    import ConfigParser as configparser


def parse_input():
    '''
    Function to parse all of command line params
    '''
    parser = OptionParser(
        usage="%prog",
        version="%prog " + VERSION)
    parser.add_option("-i", "--original-image", dest="original_image",
        help="input flash image [default: %default]", default="Flash-missingPDAT.bin",
        action="store", type="string")
    parser.add_option("-p", "--platform-config", dest="input_file",
        help="configuration (INI) file [default: %default]", 
        action="store", type="string", default="platform-data.ini")
    parser.add_option("-n", "--name", dest="modified_image",
        help="output flash image [default: %default]", 
        action="store", type="string", default="Flash+PlatformData.bin")
    parser.add_option("-u", "--undefined-order", dest="undefined_order",
        help="By default, items are put in the same order as they come in the " +
             "config file. However ordering requires python 2.7 or above.",
        action="store_true", default=False)

    opts = parser.parse_args()[0]

    mandatory = ["input_file", "original_image"]
    if DICTIONARY == dict:
        mandatory.append("undefined_order")

    for opt in mandatory:
        if not opts.__dict__[opt]:
            print("missing:", opt)
            parser.print_help()
            sys.exit(-1)

    return opts

def get_int_size(length):
    '''
    Function to return matching letter 
    for struct.pack module following:
    - B is unsigned char (1 byte)
    - H is unsigned short (2 bytes)
    - L is unsigned long (4 bytes)
    - Q is unsigned long long (8 bytes)
    '''
    if length == 1:
        return 'B'
    elif length == 2:
        return 'H'
    elif length == 4:
        return 'L'
    elif length == 8:
        return 'Q'
    else:
        raise ValueError("wrong integer size")

def get_data(data, data_type):
    '''
    Function to fetch the data from different sources:
    - file
    - text passed in ini file
    - hex value passed in ini file
    It returns in every case the data as a binary blob.
    '''
    prefix = {'hex':16, 'dec':10}
    
    if data_type == "file":
        with open(data, 'rb') as data_file:
            return data_file.read()
    elif data_type == "utf8.string":
        return data.encode('UTF-8')
    elif data_type == "hex.string":
        return binascii.unhexlify(data)
    #accepting [dec/hex].uint[8/16/32/64]
    elif "uint" in data_type:
        int_size = get_int_size(int(data_type[8:])/8)        
        return struct.pack('<' + int_size, int(data, prefix[data_type[:3]]))
    else:
        raise ValueError("allowed values: hex.uint[8/16/32/64] / hex.string / utf8.string / file")

def create_header(data):
    '''
    Function to create Platform Data header
    '''
    return struct.pack('<4sII', 
        PLATFORM_DATA_MAGIC, 
        len(data),
        binascii.crc32(data) & 0xFFFFFFFF)

def parse_ini(ini_file):
    '''
    Function to parse ini file. It returns binary blob.
    '''
    config = configparser.ConfigParser(dict_type=DICTIONARY)
    config.readfp(open(ini_file))

    platform_data = bytes()
    for section in config.sections():
        data = get_data(
            config.get(section, "data.value"), 
            config.get(section, "data.type"))
        desc = config.get(section, "desc").encode('UTF-8')
        length = int(len(data))
        id = int(config.get(section, "id"))
        try:
            ver = int(config.get(section, "ver"))
        except configparser.NoOptionError:
            ver = 0

        if len(desc) > 10:
            raise ValueError("desc field too long, only 10 characters allowed")

        # 2byte id, 2byte length, 10byte ASCII string, 2byte version, Xbyte data
        platform_data += struct.pack('<H H 10s H' + str(len(data)) + 's' , 
            id, length, desc, ver, data)
    
    #appending header of platform data
    return create_header(platform_data) + platform_data

def create_platform_data_binary(file_name, data):
    '''
    Function to create platform data binary in current directory.
    '''
    with open(os.path.basename(file_name), 'wb') as data_file:
        data_file.write(data)

def patch_output_file(file_name, data, offset_from_top):
    '''
    Function to patch binary file (Flash+PlatformData.bin)
    and save it in current directory.
    '''
    with open(file_name, 'rb+') as data_file:
        data_file.seek(0, 2)
        file_size = data_file.tell()
        offset = file_size - offset_from_top
        data_file.seek(offset, 0)
        data_file.write(data)

def main():
    '''
    Main function of the module
    '''
    opts = parse_input()
    platform_data = parse_ini(opts.input_file)
    create_platform_data_binary(
        os.path.splitext(opts.input_file)[0] + ".bin", platform_data)
    #don't touch original binary file; copy it to current dir and then patch
    copy(opts.original_image, opts.modified_image)
    patch_output_file(opts.modified_image, platform_data, OFFSET_FROM_TOP)

main()
