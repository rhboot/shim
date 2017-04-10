#!/bin/bash

DIR=$1
OPENSSL_VERSION="1.0.2k"

cp $DIR/CryptoPkg/Library/BaseCryptLib/InternalCryptLib.h InternalCryptLib.h
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptMd4Null.c Hash/CryptMd4Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptMd5.c Hash/CryptMd5.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha1.c Hash/CryptSha1.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha256.c Hash/CryptSha256.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha512.c Hash/CryptSha512.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacMd5Null.c Hmac/CryptHmacMd5Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacSha1Null.c Hmac/CryptHmacSha1Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacSha256Null.c Hmac/CryptHmacSha256Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptAesNull.c Cipher/CryptAesNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptTdesNull.c Cipher/CryptTdesNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptArc4Null.c Cipher/CryptArc4Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Rand/CryptRand.c Rand/CryptRand.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptRsaBasic.c Pk/CryptRsaBasic.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptRsaExtNull.c Pk/CryptRsaExtNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptPkcs7SignNull.c Pk/CryptPkcs7SignNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptPkcs7Verify.c Pk/CryptPkcs7Verify.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptDhNull.c Pk/CryptDhNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptTs.c Pk/CryptTs.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptX509.c Pk/CryptX509.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptAuthenticode.c Pk/CryptAuthenticode.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pem/CryptPem.c Pem/CryptPem.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/SysCall/CrtWrapper.c SysCall/CrtWrapper.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/SysCall/TimerWrapper.c SysCall/TimerWrapper.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/SysCall/BaseMemAllocation.c SysCall/BaseMemAllocation.c

cp $DIR/CryptoPkg/Library/OpensslLib/openssl-${OPENSSL_VERSION}/include/openssl/* Include/openssl/

patch -p2 <Cryptlib.diff
patch -p2 <opensslconf-diff.patch
