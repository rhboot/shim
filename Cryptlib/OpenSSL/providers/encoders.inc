/*
 * Copyright 2020-2025 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * Shim size reduction: all encoder entries removed.
 *
 * Shim only verifies signatures and never exports or encodes keys, so no
 * provider encoder implementations are needed.  Removing these entries
 * prevents encode_key2any.o, encode_key2text.o, encode_key2ms.o, and
 * encode_key2blob.o from being pulled into the final binary.
 */
