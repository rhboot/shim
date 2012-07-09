#!/bin/bash

DIR=$1

cp $DIR/InternalCryptLib.h InternalCryptLib.h
cp $DIR/Hash/CryptMd4.c Hash/CryptMd4.c
cp $DIR/Hash/CryptMd5.c Hash/CryptMd5.c
cp $DIR/Hash/CryptSha1.c Hash/CryptSha1.c
cp $DIR/Hash/CryptSha256.c Hash/CryptSha256.c
cp $DIR/Hmac/CryptHmacMd5.c Hmac/CryptHmacMd5.c
cp $DIR/Hmac/CryptHmacSha1.c Hmac/CryptHmacSha1.c
cp $DIR/Cipher/CryptAes.c Cipher/CryptAes.c
cp $DIR/Cipher/CryptTdes.c Cipher/CryptTdes.c
cp $DIR/Cipher/CryptArc4.c Cipher/CryptArc4.c
cp $DIR/Rand/CryptRand.c Rand/CryptRand.c
cp $DIR/Pk/CryptRsa.c Pk/CryptRsa.c
cp $DIR/Pk/CryptPkcs7.c Pk/CryptPkcs7.c
cp $DIR/Pk/CryptDh.c Pk/CryptDh.c
cp $DIR/Pk/CryptX509.c Pk/CryptX509.c
cp $DIR/Pk/CryptAuthenticode.c Pk/CryptAuthenticode.c
cp $DIR/Pem/CryptPem.c Pem/CryptPem.c
cp $DIR/SysCall/CrtWrapper.c SysCall/CrtWrapper.c
cp $DIR/SysCall/TimerWrapper.c SysCall/TimerWrapper.c
cp $DIR/SysCall/BaseMemAllocation.c SysCall/BaseMemAllocation.c

patch -p2 <Cryptlib.diff
