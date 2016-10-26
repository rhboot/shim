/** @file

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

Module Name:

  PlatformData.h

Abstract:

  Defines for Quark Platform Data Flash Area.

--*/

#ifndef __PLATFORM_DATA_H__
#define __PLATFORM_DATA_H__

///
/// Unique identifier to allow quick sanity checking on our Platform Data Area.
///
#define PDAT_IDENTIFIER                 SIGNATURE_32 ('P', 'D', 'A', 'T')

//
// Length constants used for sanity checks.
//

#define PDAT_ITEM_DESC_LENGTH           10

//
// Platform data Item id definitions.
//

#define PDAT_ITEM_ID_INVALID            0x0000
#define PDAT_ITEM_ID_PLATFORM_ID        0x0001
#define PDAT_ITEM_ID_SERIAL_NUM         0x0002
#define PDAT_ITEM_ID_MAC0               0x0003
#define PDAT_ITEM_ID_MAC1               0x0004
#define PDAT_ITEM_ID_MEM_CFG            0x0005
#define PDAT_ITEM_ID_MRC_VARS           0x0006

//
// Public certificate for 'pk' variable.
//
#define PDAT_ITEM_ID_PK                 0x0007

//
// SecureBoot records to be provisioned into 'kek', 'db' or 'dbx' variable.
//
#define PDAT_ITEM_ID_SB_RECORD          0x0008

#pragma pack(1)

///
/// Platform Data item.
///
typedef struct {
  UINT16       Identifier;              ///< One of PDAT_ITEM_ID_XXXX constants above.
  UINT16       Length;                  ///< Length in bytes of data part of item entry.
  CHAR8        Description[PDAT_ITEM_DESC_LENGTH];
  UINT16       Version;
} PDAT_ITEM_HEADER;

///
/// Platform Data item.
///
typedef struct {
  PDAT_ITEM_HEADER Header;
  ///
  /// In reality following field has byte length of
  /// PDAT_ITEM_HEADER.Length.
  ///
  UINT8                     Data[1];
} PDAT_ITEM;

///
/// Platform Data Flash Area.
///
typedef struct {
  UINT32       Identifier;              ///< Should == PDAT_IDENTIFIER above.
  UINT32       Length;                  ///< Length of body field.
  UINT32       Crc;                     ///< Crc of Body field.
} PDAT_AREA_HEADER;

///
/// Platform Data Flash Area.
///
typedef struct {
  PDAT_AREA_HEADER Header;
  PDAT_ITEM        Body[1];    ///< In reality multiple PDAT_ITEM entries.
} PDAT_AREA;

//
// Minimun MRC Platform Data version required for this release
//
#define PDAT_MRC_MIN_VERSION            0x01

#define PDAT_MRC_FLAG_ECC_EN            BIT0
#define PDAT_MRC_FLAG_SCRAMBLE_EN       BIT1
#define PDAT_MRC_FLAG_MEMTEST_EN        BIT2
#define PDAT_MRC_FLAG_TOP_TREE_EN       BIT3  ///< 0b DDR "fly-by" topology else 1b DDR "tree" topology.
#define PDAT_MRC_FLAG_WR_ODT_EN         BIT4  ///< If set ODR signal is asserted to DRAM devices on writes.

///
/// MRC Params Platform Data.
///

typedef struct {
  UINT16       PlatformId;              ///< Sanity check should equal data part of PDAT_ITEM_ID_PLATFORM_ID record.
  UINT32       Flags;                   ///< Bitmap of PDAT_MRC_FLAG_XXX defs above.
  UINT8        DramWidth;               ///< 0=x8, 1=x16, others=RESERVED.
  UINT8        DramSpeed;               ///< 0=DDRFREQ_800, 1=DDRFREQ_1066, others=RESERVED. Only 533MHz SKU support 1066 memory.
  UINT8        DramType;                ///< 0=DDR3,1=DDR3L, others=RESERVED.
  UINT8        RankMask;                ///< bit[0] RANK0_EN, bit[1] RANK1_EN, others=RESERVED.
  UINT8        ChanMask;                ///< bit[0] CHAN0_EN, others=RESERVED.
  UINT8        ChanWidth;               ///< 1=x16, others=RESERVED.
  UINT8        AddrMode;                ///< 0, 1, 2 (mode 2 forced if ecc enabled), others=RESERVED.
  UINT8        SrInt;                   ///< 1=1.95us, 2=3.9us, 3=7.8us, others=RESERVED. REFRESH_RATE.
  UINT8        SrTemp;                  ///< 0=normal, 1=extended, others=RESERVED.
  UINT8        DramRonVal;              ///< 0=34ohm, 1=40ohm, others=RESERVED. RON_VALUE Select MRS1.DIC driver impedance control.
  UINT8        DramRttNomVal;           ///< 0=40ohm, 1=60ohm, 2=120ohm, others=RESERVED.
  UINT8        DramRttWrVal;            ///< 0=off others=RESERVED.
  UINT8        SocRdOdtVal;             ///< 0=off, 1=60ohm, 2=120ohm, 3=180ohm, others=RESERVED.
  UINT8        SocWrRonVal;             ///< 0=27ohm, 1=32ohm, 2=40ohm, others=RESERVED.
  UINT8        SocWrSlewRate;           ///< 0=2.5V/ns, 1=4V/ns, others=RESERVED.
  UINT8        DramDensity;             ///< 0=512Mb, 1=1Gb, 2=2Gb, 3=4Gb, others=RESERVED.
  UINT32       tRAS;                    ///< ACT to PRE command period in picoseconds.
  UINT32       tWTR;                    ///< Delay from start of internal write transaction to internal read command in picoseconds.
  UINT32       tRRD;                    ///< ACT to ACT command period (JESD79 specific to page size 1K/2K) in picoseconds.
  UINT32       tFAW;                    ///< Four activate window (JESD79 specific to page size 1K/2K) in picoseconds.
  UINT8        tCL;                     ///< DRAM CAS Latency in clocks.
} PDAT_MRC_ITEM;

//
// Secure boot record defs for PDAT_ITEM_ID_SB_RECORD items.
// Secure boot record is made up of three sequentional data blobs.
// 1) PDAT_SB_PAYLOAD_HEADER (required)
// 2) GUID (optional) to identify record in secure boot data bases, depends on PDAT_SB_FLAG_HAVE_GUID.
// 3) Certificate (required) data for one of PDAT_SB_CERT_TYPE_XXXX types.
//

#define PDAT_SB_CERT_TYPE_X509            0x00  // X509 certificate format using ASN.1 DER encoding.
#define PDAT_SB_CERT_TYPE_SHA256          0x01  // 32byte SHA256 Image digest.
#define PDAT_SB_CERT_TYPE_RSA2048         0x02  // Rsa2048 storing file (*.pbk).

#define PDAT_SB_FLAG_HAVE_GUID            BIT0

typedef struct {
  //
  // Payload Secure Boot Store Specifier (UINT8 version of SECUREBOOT_STORE_TYPE)
  //
  UINT8        StoreType;

  //
  // One of PDAT_SB_CERT_TYPE_XXXX defs above.
  // For KEKStore can be PDAT_SB_CERT_TYPE_X509 or PDAT_SB_CERT_TYPE_RSA2048.
  // For DBStore/DBXStore can be PDAT_SB_CERT_TYPE_X509 or PDAT_SB_CERT_TYPE_SHA256.
  //
  UINT8        CertType;

  UINT16       Flags;       ///< Bitmap of PDAT_SB_FLAG_XXX defs above.
} PDAT_SB_PAYLOAD_HEADER;

#pragma pack()

#endif // #ifndef __PLATFORM_DATA_H__
