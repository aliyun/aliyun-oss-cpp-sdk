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

#include <sstream>
#include "Signer.h"
#include "../utils/SignUtils.h"
#include "../utils/Utils.h"
#include "../utils/LogUtils.h"
#include <openssl/hmac.h>
#ifdef OPENSSL_IS_BORINGSSL 
#include <openssl/base64.h>
#endif


using namespace AlibabaCloud::OSS;
namespace
{
    const char *TAG = "SignerV4";
}

class Sha256Helper
{
public:
    Sha256Helper()
    {
        ctx_ = EVP_MD_CTX_create();
    #if !defined(OPENSSL_IS_BORINGSSL)
        EVP_MD_CTX_set_flags(ctx_, EVP_MD_CTX_FLAG_NON_FIPS_ALLOW);
    #endif
        EVP_DigestInit_ex(ctx_, EVP_sha256(), nullptr); 
    }

    ~Sha256Helper()
    {
        EVP_MD_CTX_destroy(ctx_);
        ctx_ = nullptr;
    }

    ByteBuffer Calculate(const ByteBuffer &data)
    {
        return Calculate((const char *)data.data(), data.size());
    }

    ByteBuffer Calculate(const char *data, size_t cnt)
    {
        ByteBuffer hash(EVP_MD_size(EVP_sha256()));
        EVP_DigestInit_ex(ctx_, EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx_, (const void *)data, cnt);
        EVP_DigestFinal(ctx_, hash.data(), nullptr);
        return hash;
    }

    ByteBuffer CalculateHMAC(const std::string& src, const ByteBuffer& secret)
    {
        ByteBuffer md(EVP_MAX_MD_SIZE);
        unsigned int mdLen = EVP_MAX_MD_SIZE;

        if (HMAC(EVP_sha256(), 
                secret.data(), 
                static_cast<int>(secret.size()),
                reinterpret_cast<unsigned char const*>(src.data()), 
                static_cast<int>(src.size()),
                md.data(), &mdLen) == nullptr) {
            return ByteBuffer();
        }

        md.resize(mdLen);
        return md;
    }
private:
    EVP_MD_CTX *ctx_;     
};


SignerV4::SignerV4() :
    Signer(HmacSha256, "HMAC-SHA256", "4.0")
{
}

SignerV4::~SignerV4()
{
}

std::string SignerV4::generate(const std::string & src, const std::string & secret) const
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(secret);
    return "";
}

static bool isDefaultSignedHeader(const std::string& lowerKey)
{
    if (lowerKey == "content-type" ||
        lowerKey == "content-md5" ||
        lowerKey.compare(0, 6, "x-oss-") == 0) {
            return true;
    }
    return false;
}

static HeaderSet getCommonAdditionalHeaders(const HeaderCollection& headers, const HeaderSet &additionalHeaders)
{
    HeaderSet result;
    for (auto const &key : additionalHeaders) {
        std::string lowerKey = ToLower(key.c_str());
        if (isDefaultSignedHeader(lowerKey)) {
                //default signed header, skip
                continue;
        } else if (headers.find(lowerKey) != headers.end()) {
            result.emplace(lowerKey);
        }
    }

    return result;
}

static std::string toHeaderSetString(const HeaderSet &headers)
{
    std::stringstream ss;
    bool isFirstParam = true;
    for (auto const &key : headers) {
        std::string lowerKey = ToLower(key.c_str());
        if (isFirstParam) {
            ss << lowerKey;
        } else {
            ss << ";" << lowerKey;
        }
        isFirstParam = false;
    }
    return ss.str();
}

static std::string buildCanonicalReuqest(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameters,
    SignerParam &signerParam, const HeaderSet &additionalHeaders) 
{
    /*Version 4*/
    // HTTP Verb + "\n" +
    // Canonical URI + "\n" +
    // Canonical Query String + "\n" +
    // Canonical Headers + "\n" +
    // Additional Headers + "\n" +
    // Hashed PayLoad

    std::stringstream ss;
    // "GET" | "PUT" | "POST" | ... + "\n"
    ss << Http::MethodToString(httpRequest->method()) << "\n"; 
    
    // UriEncode(<Resource>) + "\n"
    std::string resource;
    resource.append("/");
    if (!signerParam.Bucket().empty()) {
        resource.append(signerParam.Bucket());
        resource.append("/");
    }
    if (!signerParam.Key().empty()) {
        resource.append(signerParam.Key());
    }    
    ss << UrlEncodePath(resource, true) << "\n"; 

    // Canonical Query String + "\n"
    // UriEncode(<QueryParam1>) + "=" + UriEncode(<Value>) + "&" + UriEncode(<QueryParam2>) + "\n"
    ParameterCollection signedParameters;
    for (auto const &param : parameters) {
        signedParameters[UrlEncode(param.first)] = UrlEncode(param.second);
    }
    char separator = '&';
    bool isFirstParam = true;
    for (auto const &param : signedParameters) {
        if (!isFirstParam) {
            ss << separator;
        } else {
            isFirstParam = false;
        }

        ss << param.first;
        if (!param.second.empty()) {
            ss << "=" << param.second;
        }
    }
    ss << "\n";

    // Lowercase(<HeaderName1>) + ":" + Trim(<value>) + "\n" + Lowercase(<HeaderName2>) + ":" + Trim(<value>) + "\n" + "\n"
    for (const auto &header : httpRequest->Headers()) {
        std::string lowerKey = ToLower(header.first.c_str());
        std::string value = Trim(header.second.c_str());
        if (value.empty()) {
            continue;
        }
        if (lowerKey == "content-type" ||
            lowerKey == "content-md5" ||
            lowerKey.compare(0, 6, "x-oss-") == 0) {
            ss << lowerKey << ":" << value << "\n";
        } else if (additionalHeaders.find(lowerKey) != additionalHeaders.end()) {
            ss << lowerKey << ":" << value << "\n";
        }
    }
    ss << "\n";

    // Lowercase(<AdditionalHeaderName1>) + ";" + Lowercase(<AdditionalHeaderName2>) + "\n" +
    ss << toHeaderSetString(additionalHeaders);
    ss << "\n";
   
    // Hashed PayLoad
    std::string hash = "UNSIGNED-PAYLOAD";
    if (httpRequest->hasHeader("x-oss-content-sha256")) {
        hash = httpRequest->Header("x-oss-content-sha256");
    }
    ss << hash;

    return ss.str();
}

static std::string LowerHexToString(const unsigned char *data, size_t size)
{ 
    static char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    std::stringstream ss;
    for (size_t i = 0; i < size; i++)
        ss << hex[(data[i] >> 4)] << hex[(data[i] & 0x0F)];
    return ss.str();
}

std::string buildStringToSign(const std::string &datetime, const std::string &scope, const std::string &canonical)
{
    // "OSS4-HMAC-SHA256" + "\n" +
    // TimeStamp + "\n" +
    // Scope + "\n" +
    // Hex(SHA256Hash(Canonical Reuqest))
    Sha256Helper sha265;
    auto hash = sha265.Calculate(canonical.data(), canonical.size());
    auto hashedCalRequest = LowerHexToString(hash.data(), hash.size());

    std::stringstream stringToSign;
    stringToSign << "OSS4-HMAC-SHA256" << "\n"
                 << datetime << "\n"
                 << scope << "\n"
                 << hashedCalRequest;

    return stringToSign.str();
}

std::string buildSignature(const std::string &keySecrect, const std::string &date,
     const std::string &region, const std::string &product, const std::string &stringToSign)
{
    // SigningKey
    Sha256Helper sha265;

    std::string key = "aliyun_v4" + keySecrect;
    auto signingSecret = ByteBuffer{key.begin(), key.end()};
    auto signingDate =  sha265.CalculateHMAC(date, signingSecret);
    auto signingRegion =  sha265.CalculateHMAC(region, signingDate);
    auto signingProduct =  sha265.CalculateHMAC(product, signingRegion);
    auto signingKey =  sha265.CalculateHMAC("aliyun_v4_request", signingProduct);

    // Signature
    auto signature =  sha265.CalculateHMAC(stringToSign, signingKey);

    //std::cout << "signingSecret:" << LowerHexToString(signingSecret.data(), signingSecret.size()) << std::endl;
    //std::cout << "signingDate:" << LowerHexToString(signingDate.data(), signingDate.size()) << std::endl;
    //std::cout << "signingRegion:" << LowerHexToString(signingRegion.data(), signingRegion.size()) << std::endl;
    //std::cout << "signingProduct:" << LowerHexToString(signingProduct.data(), signingProduct.size()) << std::endl;
    //std::cout << "signingKey:" << LowerHexToString(signingKey.data(), signingKey.size()) << std::endl;
    //std::cout << "signature:" << LowerHexToString(signature.data(), signature.size()) << std::endl;

    return LowerHexToString(signature.data(), signature.size());
}

void SignerV4::sign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameters,
    SignerParam &signerParam)const
{
    if (!signerParam.Cred().SessionToken().empty()) {
        httpRequest->addHeader("x-oss-security-token", signerParam.Cred().SessionToken());
    }

    auto requestTime = signerParam.RequestTime();
    auto datetime = FormatUnixTime(requestTime, "%Y%m%dT%H%M%SZ");
    auto date = FormatUnixTime(requestTime, "%Y%m%d");

    httpRequest->addHeader(Http::DATE, ToGmtTime(requestTime));
    httpRequest->addHeader("x-oss-date", datetime);

    if (!httpRequest->hasHeader("x-oss-content-sha256")) {
        httpRequest->addHeader("x-oss-content-sha256", "UNSIGNED-PAYLOAD");
    }

    auto additionalHeaders = getCommonAdditionalHeaders(httpRequest->Headers(), signerParam.AdditionalHeaders());

    auto canonicalReuqest = buildCanonicalReuqest(httpRequest, parameters, signerParam, additionalHeaders);
    auto scope = date + "/" + signerParam.Region() + "/" + signerParam.Product() + "/aliyun_v4_request";
    auto stringToSign = buildStringToSign(datetime, scope, canonicalReuqest);
    auto signature = buildSignature(signerParam.Cred().AccessKeySecret(), date, signerParam.Region(), signerParam.Product(), stringToSign);

    std::stringstream authValue;
    authValue
        << "OSS4-HMAC-SHA256"
        << " Credential="
        << signerParam.Cred().AccessKeyId() << "/"
        << scope;
    if (!additionalHeaders.empty()) {
        authValue    
            << ",AdditionalHeaders="
            << toHeaderSetString(additionalHeaders);
    }
    authValue    
        << ",Signature="
        << signature;

    //std::cout << "canonicalReuqest:" << canonicalReuqest << std::endl;
    //std::cout << "scope:" << scope << std::endl;
    //std::cout << "stringToSign:" << stringToSign << std::endl;
    //std::cout << "signature:" << signature << std::endl;
    //std::cout << "AUTHORIZATION:" << authValue.str() << std::endl;

    httpRequest->addHeader(Http::AUTHORIZATION, authValue.str());

    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) CanonicalReuqest:%s", httpRequest.get(), canonicalReuqest.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) StringToSign:%s", httpRequest.get(), stringToSign.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) Authorization:%s", httpRequest.get(), authValue.str().c_str());
 }

void SignerV4::presign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameters,
    SignerParam &signerParam)const
{
    if (!signerParam.Cred().SessionToken().empty()) {
        parameters["x-oss-security-token"] = signerParam.Cred().SessionToken();
    }

    auto datetime = FormatUnixTime(signerParam.RequestTime(), "%Y%m%dT%H%M%SZ");
    auto date = FormatUnixTime(signerParam.RequestTime(), "%Y%m%d");

    // x-oss-signature-version
    parameters["x-oss-signature-version"] = "OSS4-HMAC-SHA256";

    // x-oss-credential
    auto credential = signerParam.Cred().AccessKeyId() + "/" + date + "/" + signerParam.Region() + "/" + signerParam.Product() + "/aliyun_v4_request";
    parameters["x-oss-credential"] = credential;

    // x-oss-date
    parameters["x-oss-date"] = datetime;

    // x-oss-expires
    auto expires_duration = signerParam.Expires() - signerParam.RequestTime();
    parameters["x-oss-expires"] = std::to_string(expires_duration);

    // x-oss-additional-headers
    auto additionalHeaders = getCommonAdditionalHeaders(httpRequest->Headers(), signerParam.AdditionalHeaders());
    if (!additionalHeaders.empty()) {
        parameters["x-oss-additional-headers"] = toHeaderSetString(additionalHeaders);
    }

    auto canonicalReuqest = buildCanonicalReuqest(httpRequest, parameters, signerParam, additionalHeaders);
    auto scope = date + "/" + signerParam.Region() + "/" + signerParam.Product() + "/aliyun_v4_request";
    auto stringToSign = buildStringToSign(datetime, scope, canonicalReuqest);
    auto signature = buildSignature(signerParam.Cred().AccessKeySecret(), date, signerParam.Region(), signerParam.Product(), stringToSign);

    // "x-oss-signature"
    parameters["x-oss-signature"] = signature;

    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) CanonicalReuqest:%s", httpRequest.get(), canonicalReuqest.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) StringToSign:%s", httpRequest.get(), stringToSign.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) Signature:%s", httpRequest.get(), signature.c_str());
}
