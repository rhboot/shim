#!/usr/bin/env python
'''generator.py: Creates a Flash.bin from a layout.conf'''

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

# John Toomey / Marc Herbert

from __future__ import print_function
from __future__ import with_statement
import os
import sys
import struct
import re
from binascii import a2b_hex
from optparse import OptionParser
from ConfigParser import NoOptionError

def die(message, status = -1):
    print('ERROR: ' + message + '\n', file=sys.stderr)
    sys.exit(status)

try:
    from collections import OrderedDict
except ImportError as e:
    print (e, file=sys.stderr)
    die ('Python 2.7 or above is required')

try:
    import configparser
except ImportError:
    import ConfigParser as configparser

VERSION = '0.1'
FLASH_SIZE = 0
MFH_INFO = ''
MFH_VERSION = 0
MFH_FLAGS = 0
IMAGE_VERSION = 0
MFH_ENTRY_STRUCT = '<IIII'

MFH_ITEM_TYPE = {
  'host_fw_stage1'              : 0x00000000,
  'host_fw_stage1_signed'       : 0x00000001,
  #Reserved                     : 0x00000002,
  'host_fw_stage2'              : 0x00000003,
  'host_fw_stage2_signed'       : 0x00000004,
  'host_fw_stage2_conf'         : 0x00000005,
  'host_fw_stage2_conf_signed'  : 0x00000006,
  'host_fw_parameters'          : 0x00000007,
  'host_recovery_fw'            : 0x00000008,
  'host_recovery_fw_signed'     : 0x00000009,
  #Reserved                     : 0x0000000A,
  'bootloader'                  : 0x0000000B,
  'bootloader_signed'           : 0x0000000C,
  'bootloader_conf'             : 0x0000000D,
  'bootloader_conf_signed'      : 0x0000000E,
  #Reserved                     : 0x0000000F,
  'kernel'                      : 0x00000010,
  'kernel_signed'               : 0x00000011,
  'ramdisk'                     : 0x00000012,
  'ramdisk_signed'              : 0x00000013,
  #Reserved                     : 0x00000014,
  'loadable_program'            : 0x00000015,
  'loadable_program_signed'     : 0x00000016,
  #Reserved                     : 0x00000017,
  'build_information'           : 0x00000018,
  'flash_image_version'         : 0x00000019,
}

def cfg_get_or_none(cfg, secname, propname):
    '''Converts an NoOptionError exception into None'''
    try:
         return cfg.get(secname, propname)
    except NoOptionError:
         return None


def address_print(addr):
    '''Split an address in half so its easier to read'''

    return '0x {0:04x} {1:04x}'.format(addr / 0x10000, addr % 0x10000)


def strip_dotdir_prefixes_like_make_does(d):
    '''This strips any of the following prefixes:
    "." "./" ".//" etc. Careful not to touch '..' and '../xx'!
    This function may return an empty string.
    TODO: find in Make's documentation (or source code?) what it does EXACTLY.
    '''
    if d == '.':
        return ''

    if not d.startswith('./'):
        return d

    d = d[1:]
    return d.lstrip(os.path.sep)


class Section(object):
    '''Class to represent sections of the Flash.bin file'''

    def __init__(self, name, address, stype):

        # Note: wherever a default value makes sense it should be
        # defined here. On the other hand, when no default value makes
        # sense this should be defined (or... not defined) to fail
        # early and loudly.
        self.name = name
        self.address = address
        self.stype = stype
        self.size = None
        self.data = None
        self.meta = ''
        self.item_file = ''
        self.svn_index = None
        self.sign = False
        self.key_file = './config/key.pem'
        self.boot_index = None
        self.fvwrap = False
        self.guid = None
        self.allocated = None
        self.in_capsule = True

    def rname(self):
        '''Relative name of the item_file'''

        if self.item_file.startswith('/'):
            return os.path.basename(self.item_file)
        else:
            return strip_dotdir_prefixes_like_make_does(self.item_file)

    def final_name(self):
        '''Finale name after wrapping and signing'''

        ext = '.fv' if self.fvwrap else ''
        ext += '.signed' if self.sign else ''

        if self.item_file.startswith('/'):
            return os.path.basename(self.item_file)+ext
        else:
            return strip_dotdir_prefixes_like_make_does(self.item_file)+ext

    def src_sign(self):
        return self.rname() + '.fv' if self.fvwrap else self.rname()

    def svnindex_var(self):
        return self.src_sign() + '.SVNINDEX'

    def keyfile_var(self):
        return self.src_sign() + '.KEYFILE'


    def zero_addr(self):
        '''Return the 0-based address'''

        return self.address - 0xFFFFFFFF + FLASH_SIZE - 1

    def mfh_entry(self):
        '''Struct to use in the MFH when it is being generated'''

        # Check that the address is in the correct range
        if self.address > 0xffffffff or self.address < (0xffffffff - FLASH_SIZE):
            print('ERROR: Address {:#x} not valid for {}'.format(self.address, self.name),
                  file=sys.stderr)
            die('Valid range is {:#x} - 0xffffffff'.format(0xffffffff - FLASH_SIZE))

        mfh_type = MFH_ITEM_TYPE[self.stype.replace('mfh.', '')]
        return struct.pack(MFH_ENTRY_STRUCT, mfh_type, self.address,
                           self.size, 0xf3f3f3f3)

def parse_args():
    '''Function to parse all of command line params'''

    parser = OptionParser(usage='%prog layout.conf', version='%prog ' + VERSION)
    parser.add_option('-o', '--output', dest='output',
                      default='Flash-missingPDAT.bin', action='store',
                      help='Name of the output file [%default]')
    parser.add_option('-m', '-M', action='store_true', dest='layoutmk',
                      help='Generate a layout.mk file and exit')

    (opts, args) = parser.parse_args()

    if len(args) == 0:
        args.append('./layout.conf')

    # Check that we get a valid layout.conf
    if len(args) != 1 or not os.path.isfile(args[0]):
        print('\nERROR: Please provide the path to a valid layout.conf\n',
              file=sys.stderr)
        parser.print_help()
        sys.exit(-1)

    return (opts, args)


def search_boot_index_zero_dir(config, layout_sec):
    """This is currently used to find the path to the CapsuleCreate
    tool and uncompressed/recovery stage2 which are not specified in
    the layout.conf"""

    boot_index = cfg_get_or_none(config, layout_sec, 'boot_index')
    if not boot_index or 0 != int(boot_index):
        return None # not this entry; pass

    print("Found boot_index=0 in section " + layout_sec)
    item_file = config.get(layout_sec, 'item_file')

    # We don't throw an exception here because we want to still
    # support making "raw" images even when capsule creation can't
    # work.
    dir1 = os.path.dirname(item_file)
    if not os.path.basename(dir1) == 'FlashModules':
        return  'ERROR_no_FlashModules/_directory_found_in_path_' + item_file + '_xxx_'
            
    dir2 = os.path.dirname(dir1)
    if not os.path.basename(dir2) == 'FV':
        return  'ERROR_no_FV/_directory_found_in_path_' + item_file + '_xxx_'

    dir3 = os.path.dirname(dir2)

    # At this point dir3 can be almost anything including all these: '' '.' './x/y' './//x/y' 'x/y'
    # print ('dir3 = _%s_' % dir3)
    dir3 = strip_dotdir_prefixes_like_make_does(dir3)

    # Append trailing slash unless dir3 is empty (empty = current directory)
    if len(dir3):
        dir3 += os.path.sep

    return dir3

def find_diplicate_sections_and_options(ini_file):
    '''Function to check for duplicate sections and options within a section
    This function works around an issue with the Python 2.x version of
    ConfigParser and should be replaced when moving to Python 3.x'''

    section_re = re.compile('^\[(.*)\]')
    option_re = re.compile('(.*)\s*[=:]')
    sections = []
    current_section_options = []

    with open(ini_file,'r') as file:
        for line in file:

            line = line.strip()

            if line.startswith('#') or line == '':
                pass

            elif line.startswith('['):
                section = section_re.match(line).group(1)
                if section in sections:
                    die('Duplicate section detected: {0}'.format(section))
                sections.append(section)
                current_section_options = []

            else:
                option = option_re.match(line).group(1)
                if option in current_section_options:
                    die('Duplicate option detected: {0}  in section: {1}'.format(option, section))
                current_section_options.append(option)

def parse_layout(ini_file):
    '''Function to parse ini file and return binary blobs'''

    config = configparser.ConfigParser(dict_type=OrderedDict)
    with open(ini_file) as config_file:
        config.readfp(config_file)

    options = ['svn_index', 'boot_index',  'guid', 'item_file',
               'meta', 'key_file']

    boolean_options = [ 'sign', 'fvwrap', 'in_capsule' ]

    layout_data = []
    global BOOT_INDEX_ZERO_DIR
    BOOT_INDEX_ZERO_DIR = None

    # Loop over the sections again and capture everything else
    for layout_sec in config.sections():
        #TODO handle M and B for sizes here
        if config.has_option(layout_sec, 'type'):
            if config.get(layout_sec, 'type') == 'global':
                global FLASH_SIZE
                FLASH_SIZE = int(config.get(layout_sec, 'size'))
                continue

        #FIXME print an error if MFH is missing
        if config.has_option(layout_sec, 'type'):
            if config.get(layout_sec, 'type') == 'mfh':
                mfh_address = int(config.get(layout_sec, 'address'), 0)
                mfh_section = Section('MFH', mfh_address, 'mfh')
                setattr(mfh_section, 'item_file', 'mfh.bin')
                layout_data.append(mfh_section)
                global MFH_VERSION
                MFH_VERSION = int(config.get(layout_sec, 'version'), 0)
                global MFH_FLAGS
                MFH_FLAGS = int(config.get(layout_sec, 'flags'), 0)
                continue

        if config.has_option(layout_sec, 'meta'):
            if config.get(layout_sec, 'meta') == 'version':
                global IMAGE_VERSION
                IMAGE_VERSION = int(config.get(layout_sec, 'value'), 0)
                continue

        if None == BOOT_INDEX_ZERO_DIR:
            BOOT_INDEX_ZERO_DIR = search_boot_index_zero_dir(config, layout_sec)

        address = int(config.get(layout_sec, 'address'), 0)
        sec_type = config.get(layout_sec, 'type')
        section = Section(layout_sec, address, sec_type)

        for option in options:
            if config.has_option(layout_sec, option):
                setattr(section, option, config.get(layout_sec, option))


        for option in boolean_options:
            if not config.has_option(layout_sec, option):
                continue # keep the default value defined in the class

            value = config.get(layout_sec, option)

            if value.lower() not in [ 'yes', 'no', 'true', 'false' ]:
                print('ERROR: Invalid setting {}={} in {}'.format(option, value, layout_sec),
                      file=sys.stderr)
                die('Valid options are "yes" and "no"')

            bool_value = value.lower() in [ 'yes', 'true' ]

            setattr(section, option, bool_value)


        layout_data.append(section)


    if None == BOOT_INDEX_ZERO_DIR:
        die ('No item found with boot_index=0')

    # Get the allocated space for each item
    section_s = sorted(layout_data, key = lambda sec: sec.address)
    for i in range(len(section_s)-1):
        section_s[i].allocated = section_s[i+1].address - section_s[i].address
    section_s[-1].allocated = 4294967296 - section_s[-1].address

    return layout_data

def validate_layout(layout):
    '''Validate the input file'''

    for sec in layout:

        # Check for overlapping sections
        if sec.size > sec.allocated:
            die('Overlap detected in {}'.format(sec.name))

        # Check that the boot index is valid
        if sec.boot_index != None:
            if not int(sec.boot_index) in range(32):
                die('Invalid boot_index {} in {}\nERROR: Valid options '
                    'are 0, 1, 2, ...'.format(sec.boot_index, sec.name))

        # Check that the svn index is valid
        if sec.svn_index != None:
            if not int(sec.svn_index) in range(16):
                die('Invalid svn_index {} in {}\nERROR: Valid options '
                    'are 0, 1, 2, ...'.format(sec.svn_index, sec.name))

        # Check for blank type
        if sec.stype == '':
            die('Type field empty for {}'.format(sec.name))

    # Make sure we got a valid MFH and flash versions
    if not MFH_VERSION > 0:
        print('WARNING: MFH version missing or not valid')
        #sys.exit(-1)

    if not IMAGE_VERSION > 0:
        print('WARNING: Flash version missing or not valid')
        #sys.exit(-1)

    item_files = [x.item_file for x in layout]
    for file in item_files:
        if item_files.count(file) > 1:
            die('Multiple uses of the same input file must have unique '
                'names and "{}" is used more than once. If this is required '
                'to do this it is reccomended to create a symlink for the'
                'each instance of the file after the first.'.format(file))

def read_data(layout, layout_conf):
    '''Read in the data files'''

    for i, sec in enumerate(layout):
        if sec.meta == 'layout':
            with open(layout_conf) as config_file:
                sec.data = config_file.read()
            sec.size = os.path.getsize(layout_conf)
            setattr(sec, 'item_file', 'layout.conf')
        elif sec.stype == 'mfh':
            mfh_index = i
        else:
            with open(sec.final_name(), 'rb') as ifile:
                sec.data = ifile.read()
            sec.size = os.path.getsize(sec.final_name())

    #FIXME better way to do this with 'else'?
    (layout[mfh_index].size, layout[mfh_index].data) = generate_mfh(layout)

def generate_mfh(layout):
    '''Generate the master flash header'''

    mfh_body = ''
    format_str = ''
    mfh_count = 0

    # Create boot index list
    boot_index_dict = {}
    boot_items_count = 0
    mfh_items = [x for x in layout if x.stype.startswith('mfh.')] 
    for mfh_position, item in enumerate(mfh_items):
        if item.boot_index != None:
            boot_items_count += 1
            boot_index_dict[int(item.boot_index)] = mfh_position

    # Now convert this dict to an actual list
    try:
        boot_index_list = [ boot_index_dict[i] for i in range(boot_items_count) ]
    except KeyError:
        print ("ERROR: boot indexes don't make a complete list from 0 "
               "to {0:d}".format(boot_items_count-1), file=sys.stderr)
        raise

    for bi in boot_index_list:
        mfh_body += struct.pack('<I', bi)
        format_str += 'I'

    # Insert MFH item entries
    for item in layout:
        if item.stype.startswith('mfh.'):
            mfh_body += item.mfh_entry()
            format_str += 'IIII'
            mfh_count += 1

    # Image version extra entry hack
    if IMAGE_VERSION > 0:
        flash_ver_args = (MFH_ITEM_TYPE['flash_image_version'],
                          0x00000000, 0x00000000, IMAGE_VERSION)
        mfh_body += struct.pack(MFH_ENTRY_STRUCT, *flash_ver_args)
        format_str += 'IIII'
        mfh_count += 1

    # MFH Identifier, Version of MFH, Flags, Next header block,
    # Flash item count, Boot priority list count
    mfh_args = (0x5F4D4648,
                MFH_VERSION,
                MFH_FLAGS,
                0x00000000,
                mfh_count,
                len([x for x in layout if x.boot_index != None]),
               )

    mfh_head = struct.pack('<IIIIII', *mfh_args)

    padding = bytearray([243] * (512 - struct.calcsize(format_str)))

    mfh_struct = mfh_head + mfh_body + padding

    # Generate the MFH info for the log file
    global MFH_INFO
    summary = (' ---- Master Flash Header summary ----\n\n'
               'MFH Identifier     = {:#010x}\nVersion            = {:#010x}\n'
               'Flags              = {:#010x}\nNext Header Block  = {:#010x}\n'
               'Flash Item Count   = {:#010x}\nBoot List Count    = {:#010x}\n\n')
    MFH_INFO = summary.format(mfh_args[0], mfh_args[1], mfh_args[2],
                              mfh_args[3], mfh_args[4], mfh_args[5])

    MFH_INFO += 'Boot index list:\n  '
    for j in boot_index_list:
        MFH_INFO += ('{0:d} '.format(j))
        MFH_INFO += ('[{0:s}], '.format(mfh_items[j].name))

    MFH_INFO += ('\n\nType Address    Size     Reserved\n')
    for item in mfh_items:
        item_type = MFH_ITEM_TYPE[item.stype.replace('mfh.', '')]
        line = '{0:#04x} {1:#10x} {2:#8x} {3:#10x} [{4:s}]\n'
        MFH_INFO += line.format(item_type, item.address, item.size, 0xf3f3f3f3, item.name)

    with open('mfh.bin','wb') as mfh_bin:
        mfh_bin.write(mfh_struct)

    return(struct.calcsize(format_str), mfh_struct)

def gen_image_info(layout):
    '''Write data to the image info file'''

    layout_str = '\n ---- Flash content [{} items] ----\n'.format(len(layout))
    layout_str += 'Address       Size    Free    Type  BI  Name'
    total_free, total_size = 0, 0
    for item in sorted(layout, key = lambda sec: sec.address):
        try:
            mfh_type_s = MFH_ITEM_TYPE[item.stype.replace('mfh.', '')]
            type_s = '{:#04x}'.format(mfh_type_s)
        except KeyError:
            type_s = ''

        bindex = item.boot_index if item.boot_index else ''

        free_after = item.allocated - item.size
        total_free += free_after
        total_size += item.size

        rep = ('\n{0:s}  {1:7d} {2:7d} {3:<5s} {4:>2s} {5}')
        layout_str += rep.format(address_print(item.address), item.size,
                                 free_after, type_s,
                                 bindex, item.name)

    layout_str += rep.format('Total       ', total_size, total_free, '', '', '')

    print(layout_str)

    with open('image_info.txt','w') as info_file:
        info_file.write(MFH_INFO)
        info_file.write(layout_str)

def gen_layout_mk(layout):
    '''Generate the layout.mk file'''

    srcs_fvwrap = [x.rname() for x in layout if x.fvwrap]
    signed_objects = [x for x in layout if x.sign] 
    srcs_sign = [x.src_sign() for x in signed_objects]
    guid = [x.rname()+'.GUID = '+x.guid for x in layout if x.guid]
    svn_index = [x.svnindex_var()+' = '+x.svn_index for x in signed_objects]
    key_files = [x.keyfile_var() + ' := ' + x.key_file for x in signed_objects]
    key_files_unique = set([x.key_file for x in signed_objects])
    scrs = [x.item_file for x in layout if x.item_file.startswith('/')]
    slinks = [x.rname()+' : '+x.item_file for x in layout if x.item_file.startswith('/')]
    unmodified = [x.rname() for x in layout if not x.fvwrap and not x.sign]
    unmodified = [x for x in unmodified if not x in ('mfh.bin', '')]

    with open('layout.mk', 'w') as layout_mk:
        layout_mk.write('#\n# File generated by {0}\n#\n\n'.format(sys.argv[0]))
        layout_mk.write('SRCS_FVWRAP = {0}\n\n'.format(' '.join(srcs_fvwrap)))
        layout_mk.write('SRCS_SIGN = {0}\n\n'.format(' '.join(srcs_sign)))
        if guid:
            layout_mk.write('\n'.join(guid) + '\n')
        layout_mk.write('\n'.join(svn_index) + '\n\n')
        layout_mk.write('\n'.join(key_files) + '\n\n')
        layout_mk.write('KEY_FILES := {0}\n\n'.format(' '.join(key_files_unique)))
        layout_mk.write('SRCS_ALL_REMOTE := {0}'.format(' '.join(scrs))+'\n\n')
        layout_mk.write('SYMLINKS := $(notdir ${SRCS_ALL_REMOTE})\n\n')
        layout_mk.write('\n'.join(slinks))
        layout_mk.write('\n\nSRCS_UNMODIFIED := {}'.format(' '.join(unmodified)))
        layout_mk.write('\n\nBOOT_INDEX_ZERO_DIR := {0}'.format(BOOT_INDEX_ZERO_DIR))
        layout_mk.write('\n')

def gen_capsule_comp_ini(layout):
    '''Generate the capsule creation file'''

    with open('CapsuleComponents.ini','w') as cc_ini:
        cc_ini.write('[Components]\n\n')
        for item in layout:
            cc_ini.write('\n### [{0}] ###\n'.format(item.name))
            cc_ini.write('# {0} \n'.format(item.item_file))
            comment = '' if item.in_capsule else '# '
            item_str = '{0}BEGIN\n{0}  FILE_NAME={1}\n{0}  START_ADDRESS={2:#07x}\n{0}'
            cc_ini.write(item_str.format(comment, item.final_name(), item.address))
            cc_ini.write('# Size: {0} ({0:#08x})\n'.format(item.size))
            cc_ini.write(comment + 'END\n')

def gen_flash_binary(layout, output):
    '''Create the final Flash.bin file'''

    with open(output, 'wb') as output_file:

        # padding =''
        # This is where most of the time is spent...
        padding = a2b_hex('C0') * FLASH_SIZE

        output_file.write(padding)
        for item in sorted(layout, key = lambda sec: sec.address):
            output_file.seek(item.zero_addr())
            # ... together with this other place
            output_file.write(item.data)

def main():
    '''Main function'''

    (opts, args) = parse_args()
    layout_file = args[0]
    layout = parse_layout(layout_file)
    find_diplicate_sections_and_options(layout_file)

    if opts.layoutmk:
        gen_layout_mk(layout)
    else:
        read_data(layout, layout_file)
        gen_image_info(layout)
        validate_layout(layout)
        gen_capsule_comp_ini(layout)
        gen_flash_binary(layout, opts.output)

if __name__ == '__main__':
    main()

    #import timeit
    #tx = timeit.Timer('main()', setup='from __main__ import main')
    #print ( min(tx.repeat(7, 1))/1.0, file=sys.stderr )
