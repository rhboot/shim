#!/bin/bash

DIR=$1

cp $DIR/CryptoPkg/Library/BaseCryptLib/InternalCryptLib.h InternalCryptLib.h
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptMd4.c Hash/CryptMd4.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptMd5.c Hash/CryptMd5.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha1.c Hash/CryptSha1.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha256.c Hash/CryptSha256.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacMd5.c Hmac/CryptHmacMd5.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacSha1.c Hmac/CryptHmacSha1.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptAes.c Cipher/CryptAes.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptTdes.c Cipher/CryptTdes.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptArc4.c Cipher/CryptArc4.c
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

cp $DIR/CryptoPkg/Include/openssl/* Include/openssl/

patch -p2 <Cryptlib.diff
