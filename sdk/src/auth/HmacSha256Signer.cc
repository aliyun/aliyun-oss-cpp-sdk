/*
 * Copyright 2009-2017 Alibaba Cloud All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "HmacSha256Signer.h"
#if 0//def _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <openssl/hmac.h>
#ifdef OPENSSL_IS_BORINGSSL 
#include <openssl/base64.h>
#endif
#endif

using namespace AlibabaCloud::OSS;

HmacSha256Signer::HmacSha256Signer() :
    Signer(HmacSha256, "HMAC-SHA256", "2.0")
{
}

HmacSha256Signer::~HmacSha256Signer()
{
}

std::string HmacSha256Signer::generate(const std::string & src, const std::string & secret) const {
    OSS_LOG(LogLevel::LogError, "HmacSha256Signer", "no implemented, src = %s, secret = %s", src.c_str(), secret.c_str());
    return "";
}

ByteBuffer HmacSha256Signer::calculate(const ByteBuffer &src, const std::string & secret) const
{
    if (src.size() == 0)
        return ByteBuffer{};

#if 0//def _WIN32
    typedef struct _my_blob {
        BLOBHEADER hdr;
        DWORD dwKeySize;
        BYTE rgbKeyData[];
    }my_blob;

    DWORD kbLen = sizeof(my_blob) + secret.size();
    my_blob * kb = (my_blob *)LocalAlloc(LPTR, kbLen);
    kb->hdr.bType = PLAINTEXTKEYBLOB;
    kb->hdr.bVersion = CUR_BLOB_VERSION;
    kb->hdr.reserved = 0;
    kb->hdr.aiKeyAlg = CALG_RC2;
    kb->dwKeySize = secret.size();
    memcpy(&kb->rgbKeyData, secret.c_str(), secret.size());

    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHmacHash = 0;
    BYTE pbHash[32];
    DWORD dwDataLen = 32;
    HMAC_INFO HmacInfo;
    ZeroMemory(&HmacInfo, sizeof(HmacInfo));
    HmacInfo.HashAlgid = CALG_SHA256;

    CryptAcquireContext(&hProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET);
    CryptImportKey(hProv, (BYTE*)kb, kbLen, 0, CRYPT_IPSEC_HMAC_KEY, &hKey);
    CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmacHash);
    CryptSetHashParam(hHmacHash, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0);
    CryptHashData(hHmacHash, (BYTE*)(src.c_str()), src.size(), 0);
    CryptGetHashParam(hHmacHash, HP_HASHVAL, pbHash, &dwDataLen, 0);

    LocalFree(kb);
    CryptDestroyHash(hHmacHash);
    CryptDestroyKey(hKey);
    CryptReleaseContext(hProv, 0);

    DWORD dlen = 0;
    CryptBinaryToString(pbHash, dwDataLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &dlen);
    char* dest = new char[dlen];
    CryptBinaryToString(pbHash, dwDataLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, dest, &dlen);

    std::string ret = std::string(dest, dlen);
    delete dest;
    return ret;
#else
    ByteBuffer md(32);
    unsigned int mdLen = 32;

    if (HMAC(EVP_sha256(), secret.c_str(), static_cast<int>(secret.size()),
        src.data(), src.size(),
        md.data(), &mdLen) == nullptr)
        return ByteBuffer{};

    md.resize(mdLen);

    return md;
#endif
}