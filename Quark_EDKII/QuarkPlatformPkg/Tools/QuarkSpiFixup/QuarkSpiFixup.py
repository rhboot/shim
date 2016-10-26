#!/usr/bin/env python
## @file
#
#  Copyright (c) 2013 Intel Corporation.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  * Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in
#  the documentation and/or other materials provided with the
#  distribution.
#  * Neither the name of Intel Corporation nor the names of its
#  contributors may be used to endorse or promote products derived
#  from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#


import glob
import os
import subprocess
import sys
import struct
import re
import shutil

#
# Input arguments:
#	platform name, build type
#
platformName = sys.argv[1]
buildType = sys.argv[2]
buildTools = sys.argv[3]

# Build Name Options
buildOptionDebug = "DEBUG"
buildOptionRelease = "RELEASE"

# BootROM definitions
QuarkRomBaseAddress = 4294836224

#NVRAM definitions
nvramLocation = 0x730000
nvramSize = 0x20000

# Root Directory
curr_dir = os.getcwd()

buildType = buildType.upper()
	
#check if valid build option supplied
if re.match(buildOptionDebug, buildType) or re.match(buildOptionRelease, buildType):
	capsule_app_file = curr_dir+"/"+platformName+"Pkg"+"/Applications/CapsuleApp.efi"
	build_dir = curr_dir+"/Build/" + platformName + "/"+ buildType + "_" + buildTools + "/FV"
	output_dir = curr_dir+"/Build/" + platformName + "/"+ buildType +"_" + buildTools + "/FV/FlashModules"
	if not os.path.exists(output_dir):     
		os.makedirs(output_dir)	

	#copy RMU binaries to the output_dir
	rmu_binary_file = curr_dir+"/QuarkSocPkg/QuarkNorthCluster/Binary/QuarkMicrocode/RMU.bin"
	shutil.copyfileobj(open(rmu_binary_file,'rb'), open(output_dir+"/RMU.bin",'wb'))
	rmu_binary_file = curr_dir+"/QuarkSocPkg/QuarkNorthCluster/Binary/Quark2Microcode/RMU.bin"
	shutil.copyfileobj(open(rmu_binary_file,'rb'), open(output_dir+"/RMU2.bin",'wb'))	
	
	# Check build directory really exists
	if os.path.exists(build_dir):
		os.chdir(build_dir)
		currdir = os.getcwd()
		listing = sorted(os.listdir(currdir))
		
		# Stage1 images use Index 0x00 - 0x0F
		# Recovery images use Index 0x10 - 0x1F
		stage1_index = 0
		recovery_index = 16
			
		#check files for fvs that are stage1 or recovery only
		for infile in listing:
			if infile.endswith(".Fv"):
				if re.search("EDKII_BOOT_STAGE1_", infile) or re.search("EDKII_RECOVERY_STAGE1_", infile):
						
					# open current file, then update the offset and index
					currentBin = open(infile, 'rb+')
					currentBinLength = len(currentBin.read())
						
					#
					# Search for Spi Entry Offset:
					#
					currentBin.seek(0, os.SEEK_SET)
					currCount = 0
					for currCount in range(currentBinLength):
						currByte = currentBin.read(1)
						currCount = currCount+1
						if currByte == 'S':
							currByte = currentBin.read(1)
							currCount = currCount+1
							if currByte == 'P':
								currByte = currentBin.read(1)
								currCount = currCount+1
								if currByte == 'I':
									currByte = currentBin.read(1)
									currCount = currCount+1
									if currByte == ' ':
										currByte = currentBin.read(1)
										currCount = currCount+1
										if currByte == 'E':
											currByte = currentBin.read(1)
											currCount = currCount+1
											if currByte == 'n':
												currByte = currentBin.read(1)
												currCount = currCount+1
												if currByte == 't':
													currByte = currentBin.read(1)
													currCount = currCount+1
													if currByte == 'r':
														currByte = currentBin.read(1)
														currCount = currCount+1
														if currByte == 'y':
															currByte = currentBin.read(1)
															currCount = currCount+1
															if currByte == ' ':
																currByte = currentBin.read(1)
																currCount = currCount+1
																if currByte == 'P':
																	currByte = currentBin.read(1)
																	currCount = currCount+1
																	if currByte == 'o':
																		currByte = currentBin.read(1)
																		currCount = currCount+1
																		if currByte == 'i':
																			currByte = currentBin.read(1)
																			currCount = currCount+1
																			if currByte == 'n':
																				currByte = currentBin.read(1)
																				currCount = currCount+1
																				if currByte == 't':
																					currByte = currentBin.read(1)
																					currCount = currCount+1
																					if currByte == ' ':
																						break 
					mySpiLocation = currentBin.tell()
					currentBin.seek(0, os.SEEK_SET)
					SpiOffSet = mySpiLocation
						
					# Add spi offset to first 4 bytes of images
					currentBin.write(struct.pack('I', SpiOffSet))
						
					# Add index as unique identifier for the image to byte 5 offset
					if re.search("EDKII_BOOT_STAGE1_", infile):
						currentBin.write(struct.pack('I', stage1_index))
							
						# Print out Software Module fixup information
						print '%(File_Name)s (Entry point:%(Entry_Offset)s Index:%(Module_Index)02d) \n' %{"File_Name": infile, "Entry_Offset": hex(mySpiLocation), "Module_Index": stage1_index}
							
						stage1_index += 1
							
					else:
						currentBin.write(struct.pack('I', recovery_index))
							
						# Print out Software Module fixup information
						print '%(File_Name)s (Entry point:%(Entry_Offset)s Index:%(Module_Index)02d) \n' %{"File_Name": infile, "Entry_Offset": hex(mySpiLocation), "Module_Index": recovery_index}
							
						recovery_index += 1
														
					# Fixup the Firmware Volume Header Checksum
					currentBin.seek(50, os.SEEK_SET)
					currentBin.write(struct.pack('H', 0x0000))
						
					currentBin.seek(48, os.SEEK_SET)
					low_byte = ord(currentBin.read(1))
					high_byte = ord(currentBin.read(1))
					headerLength = ((high_byte << 8) | low_byte) / 2
						
					currentBin.seek(0, os.SEEK_SET)
					headerChecksum = 0
					for index in range(0,headerLength):
						low_byte = ord(currentBin.read(1))
						high_byte = ord(currentBin.read(1))
						temp = (high_byte << 8) | low_byte
						headerChecksum = (headerChecksum + temp) & 0xFFFF
							
					headerChecksum = (0x10000 - headerChecksum) & 0xFFFF
					currentBin.seek(50, os.SEEK_SET)
					currentBin.write(struct.pack('B', (headerChecksum & 0x00FF)))
					currentBin.write(struct.pack('B', (headerChecksum >> 8) & 0x00FF))
						
					currentBin.close()
						
				elif (re.search("EDKII_BOOTROM_OVERRIDE", infile)):
					# open current file, then update the offset and index
					currentBin = open(infile, 'rb+')
					currentBinLength = len(currentBin.read())
						
					#
					# Search for 'ValidateModuleCallBack':
					#
					currentBin.seek(0, os.SEEK_SET)
					currCount = 0
					for currCount in range(currentBinLength):
						currByte = currentBin.read(1)
						currCount = currCount+1
						if currByte == 'V':
							currByte = currentBin.read(1)
							currCount = currCount+1
							if currByte == 'a':
								currByte = currentBin.read(1)
								currCount = currCount+1
								if currByte == 'l':
									currByte = currentBin.read(1)
									currCount = currCount+1
									if currByte == 'i':
										currByte = currentBin.read(1)
										currCount = currCount+1
										if currByte == 'd':
											currByte = currentBin.read(1)
											currCount = currCount+1
											if currByte == 'a':
												currByte = currentBin.read(1)
												currCount = currCount+1
												if currByte == 't':
													currByte = currentBin.read(1)
													currCount = currCount+1
													if currByte == 'e':
														currByte = currentBin.read(1)
														currCount = currCount+1
														if currByte == 'M':
															currByte = currentBin.read(1)
															currCount = currCount+1
															if currByte == 'o':
																currByte = currentBin.read(1)
																currCount = currCount+1
																if currByte == 'd':
																	currByte = currentBin.read(1)
																	currCount = currCount+1
																	if currByte == 'u':
																		currByte = currentBin.read(1)
																		currCount = currCount+1
																		if currByte == 'l':
																			currByte = currentBin.read(1)
																			currCount = currCount+1
																			if currByte == 'e':
																				currByte = currentBin.read(1)
																				currCount = currCount+1
																				if currByte == 'C':
																					currByte = currentBin.read(1)
																					currCount = currCount+1
																					if currByte == 'a':
																						currByte = currentBin.read(1)
																						currCount = currCount+1
																						if currByte == 'l':
																							currByte = currentBin.read(1)
																							currCount = currCount+1
																							if currByte == 'l':
																								currByte = currentBin.read(1)
																								currCount = currCount+1
																								if currByte == 'B':
																									currByte = currentBin.read(1)
																									currCount = currCount+1
																									if currByte == 'a':
																										currByte = currentBin.read(1)
																										currCount = currCount+1
																										if currByte == 'c':
																											currByte = currentBin.read(1)
																											currCount = currCount+1
																											if currByte == 'k':																						
																												break 
					mySpiLocation = currentBin.tell()
					mySpiLocation = mySpiLocation + QuarkRomBaseAddress

					#
					# Patch the validate callback address address of the 32-bit code entry point
					#
					currentBin.seek(-32, os.SEEK_END)
					currentBin.write(struct.pack('L', mySpiLocation))
					
					currentBin.seek(0, os.SEEK_SET)
					currCount = 0
					for currCount in range(currentBinLength):
						currByte = currentBin.read(1)
						currCount = currCount+1
						if currByte == 'V':
							currByte = currentBin.read(1)
							currCount = currCount+1
							if currByte == 'a':
								currByte = currentBin.read(1)
								currCount = currCount+1
								if currByte == 'l':
									currByte = currentBin.read(1)
									currCount = currCount+1
									if currByte == 'i':
										currByte = currentBin.read(1)
										currCount = currCount+1
										if currByte == 'd':
											currByte = currentBin.read(1)
											currCount = currCount+1
											if currByte == 'a':
												currByte = currentBin.read(1)
												currCount = currCount+1
												if currByte == 't':
													currByte = currentBin.read(1)
													currCount = currCount+1
													if currByte == 'e':
														currByte = currentBin.read(1)
														currCount = currCount+1
														if currByte == 'K':
															currByte = currentBin.read(1)
															currCount = currCount+1
															if currByte == 'e':
																currByte = currentBin.read(1)
																currCount = currCount+1
																if currByte == 'y':
																	currByte = currentBin.read(1)
																	currCount = currCount+1
																	if currByte == 'M':
																		currByte = currentBin.read(1)
																		currCount = currCount+1
																		if currByte == 'o':
																			currByte = currentBin.read(1)
																			currCount = currCount+1
																			if currByte == 'd':
																				currByte = currentBin.read(1)
																				currCount = currCount+1
																				if currByte == 'C':
																					currByte = currentBin.read(1)
																					currCount = currCount+1
																					if currByte == 'a':
																						currByte = currentBin.read(1)
																						currCount = currCount+1
																						if currByte == 'l':
																							currByte = currentBin.read(1)
																							currCount = currCount+1
																							if currByte == 'l':
																								currByte = currentBin.read(1)
																								currCount = currCount+1
																								if currByte == 'B':
																									currByte = currentBin.read(1)
																									currCount = currCount+1
																									if currByte == 'a':
																										currByte = currentBin.read(1)
																										currCount = currCount+1
																										if currByte == 'c':
																											currByte = currentBin.read(1)
																											currCount = currCount+1
																											if currByte == 'k':																						
																												break 
					mySpiLocation = currentBin.tell()
					mySpiLocation = mySpiLocation + QuarkRomBaseAddress

					#
					# Patch the validate key callback address address of the 32-bit code entry point
					#
					currentBin.seek(-123590, os.SEEK_END)
					currentBin.write(struct.pack('L', mySpiLocation))

					currentBin.close()						
						
		index1 = 1
		index2 = 1
		#check files for fvs that are recovery only combine stage1 and stage2 to create 1 recovery image, plus any additional images
		for infile in listing:
			if infile.endswith(".Fv"):
				pattern1 = "EDKII_RECOVERY_STAGE1_IMAGE"+ str(index1)					
				if re.search(pattern1, infile):
					file1 = infile
					for infile in listing:
						pattern2 = "EDKII_RECOVERY_STAGE2_IMAGE"+ str(index1)	
						if infile.endswith(".Fv") and re.search(pattern2, infile):
							file2 = infile
							outfile_name = "EDKII_RECOVERY_IMAGE"+str(index1)+".Fv"
							destination = open(output_dir+"/"+outfile_name,'wb') 
							shutil.copyfileobj(open(file1,'rb'), destination) 
							shutil.copyfileobj(open(file2,'rb'), destination) 
							destination.close() 
					index1 += 1
				elif (re.search("EDKII_BOOT_STAGE1_IMAGE"+ str(index2), infile)):
					file1 = infile
					outfile_name = "EDKII_BOOT_STAGE1_IMAGE"+str(index2)+".Fv"
					destination = open(output_dir+"/"+outfile_name,'wb')
					shutil.copyfileobj(open(file1,'rb'), destination)
					destination.close()
					index2 += 1
				else:
					if (re.search("EDKII_BOOT_STAGE2", infile)):
						shutil.copy(infile, output_dir)
					if (re.search("EDKII_BOOT_STAGE2_RECOVERY", infile)):
						shutil.copy(infile, output_dir)							
					if (re.search("EDKII_BOOT_STAGE2_COMPACT", infile)):
						shutil.copy(infile, output_dir)							
					if (re.search("EDKII_BOOTROM_OVERRIDE", infile)):
						shutil.copy(infile, output_dir)

							
		#check files for fd and extract the nvram area and create new binary file with it
		for infile in listing:
			if infile.endswith(".fd"):	
				# open current file, then update the offset and index
				currentBin = open(infile, 'rb+')
				currentBinLength = len(currentBin.read())
					
				#
				# Search for Spi Entry Offset:
				#
				currentBin.seek(nvramLocation, os.SEEK_SET)
				mySpiLocation = currentBin.tell()
				outfile_name = "EDKII_NVRAM.bin"  
				destination = open(output_dir+"/"+outfile_name,'wb')
				bytes_read = currentBin.read(nvramSize) 
				destination.write(bytes_read)
				currentBin.close()					
	else:
		print '\t ERROR: Build directory does not exist for platform: \t', platformName, '\n'
else:
	print '\t ERROR: Incorrect build type specified: \t', buildType, '\n'
	print '\t Options are: \n'
	print '\t 1:\t', buildOptionDebug, ' or ', buildOptionDebug.lower(), '\n'
	print '\t 2:\t', buildOptionRelease, ' or ', buildOptionRelease.lower(),'\n'
	print '\t Please specify correct build option\t\n'

