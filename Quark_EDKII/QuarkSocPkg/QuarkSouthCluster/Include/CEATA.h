/** @file

  Header file for chipset CE-AT spec.

Copyright (c) 2013 Intel Corporation.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _CE_ATA_H
#define _CE_ATA_H

#pragma pack(1)


#define  DATA_UNIT_SIZE       512          


#define  CMD60                60
#define  CMD61                61


#define RW_MULTIPLE_REGISTER  CMD60
#define RW_MULTIPLE_BLOCK     CMD61


#define CE_ATA_SIG_CE         0xCE  
#define CE_ATA_SIG_AA         0xAA  


#define Reg_Features_Exp      01
#define Reg_SectorCount_Exp   02
#define Reg_LBALow_Exp        03
#define Reg_LBAMid_Exp        04
#define Reg_LBAHigh_Exp       05
#define Reg_Control           06
#define Reg_Features_Error    09
#define Reg_SectorCount       10
#define Reg_LBALow            11
#define Reg_LBAMid            12
#define Reg_LBAHigh           13
#define Reg_Device_Head       14
#define Reg_Command_Status    15

#define Reg_scrTempC          0x80
#define Reg_scrTempMaxP       0x84
#define Reg_scrTempMinP       0x88
#define Reg_scrStatus         0x8C
#define Reg_scrReallocsA      0x90
#define Reg_scrERetractsA     0x94
#define Reg_scrCapabilities   0x98
#define Reg_scrControl        0xC0



typedef struct {
  UINT8  Reserved0;
  UINT8  Features_Exp;
  UINT8  SectorCount_Exp;
  UINT8  LBALow_Exp;
  UINT8  LBAMid_Exp;
  UINT8  LBAHigh_Exp;
  UINT8  Control;
  UINT8  Reserved1[2];
  UINT8  Features_Error;
  UINT8  SectorCount;
  UINT8  LBALow;
  UINT8  LBAMid;
  UINT8  LBAHigh;
  UINT8  Device_Head;
  UINT8  Command_Status;
}TASK_FILE;


//
//Reduced ATA command set
//
#define IDENTIFY_DEVICE       0xEC 
#define READ_DMA_EXT          0x25
#define WRITE_DMA_EXT         0x35 
#define STANDBY_IMMEDIATE     0xE0
#define FLUSH_CACHE_EXT       0xEA



typedef struct {
  UINT16  Reserved0[10];
  UINT16  SerialNumber[10];
  UINT16  Reserved1[3];
  UINT16  FirmwareRevision[4];
  UINT16  ModelNumber[20];
  UINT16  Reserved2[33];
  UINT16  MajorVersion;
  UINT16  Reserved3[19];
  UINT16  MaximumLBA[4];
  UINT16  Reserved4[2];
  UINT16  Sectorsize;
  UINT16  Reserved5;
  UINT16  DeviceGUID[4];
  UINT16  Reserved6[94];
  UINT16  Features;
  UINT16  MaxWritesPerAddress;
  UINT16  Reserved7[47];
  UINT16  IntegrityWord;
}IDENTIFY_DEVICE_DATA;





#pragma pack()

#endif
