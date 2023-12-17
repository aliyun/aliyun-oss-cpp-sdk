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
const char *TAG = "SignerV1";
}

SignerV1::SignerV1() :
    Signer(HmacSha1, "HMAC-SHA1", "1.0")
{
}

SignerV1::~SignerV1()
{
}

static std::string buildResource(const std::string &bucket, const std::string &key) 
{
    std::string resource;
    resource.append("/");
    if (!bucket.empty()) {
        resource.append(bucket);
        resource.append("/");
    }
    if (!key.empty()) {
        resource.append(key);
    }
    return resource;
}

std::string SignerV1::generate(const std::string & src, const std::string & secret) const
{
    if (src.empty())
        return std::string();

    unsigned char md[32];
    unsigned int mdLen = 32;

    if (HMAC(EVP_sha1(), secret.c_str(), static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(src.c_str()), src.size(),
        md, &mdLen) == nullptr)
        return std::string();

    char encodedData[100];
    EVP_EncodeBlock(reinterpret_cast<unsigned char*>(encodedData), md, mdLen);
    return encodedData;
}

void SignerV1::sign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameters,
    SignerParam &signerParam)const
{
    if (!signerParam.Cred().SessionToken().empty()) {
        httpRequest->addHeader("x-oss-security-token", signerParam.Cred().SessionToken());
    }

    if (httpRequest->hasHeader("x-oss-date")) {
        httpRequest->addHeader(Http::DATE, httpRequest->Header("x-oss-date"));
    } else {
        auto requestTime = signerParam.RequestTime();
        httpRequest->addHeader(Http::DATE, ToGmtTime(requestTime));
    }

    auto method = Http::MethodToString(httpRequest->method());
    auto resource = buildResource(signerParam.Bucket(), signerParam.Key());
    auto date = httpRequest->Header(Http::DATE);

    SignUtils signUtils("");
    signUtils.build(method, resource, date, httpRequest->Headers(), parameters);
    auto signature = generate(signUtils.CanonicalString(), signerParam.Cred().AccessKeySecret());

    std::stringstream authValue;
    authValue
        << "OSS "
        << signerParam.Cred().AccessKeyId()
        << ":"
        << signature;

    httpRequest->addHeader(Http::AUTHORIZATION, authValue.str());

    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) CanonicalString:%s", httpRequest.get(), signUtils.CanonicalString().c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "request(%p) Authorization:%s", httpRequest.get(), authValue.str().c_str());
}

void SignerV1::presign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameters,
    SignerParam &signerParam)const
{
    if (!signerParam.Cred().SessionToken().empty()) {
        parameters["security-token"] = signerParam.Cred().SessionToken();
    }

    auto method = Http::MethodToString(httpRequest->method());
    auto resource = buildResource(signerParam.Bucket(), signerParam.Key());
    auto date = std::to_string(signerParam.Expires());

    SignUtils signUtils("");
    signUtils.build(method, resource, date, httpRequest->Headers(), parameters);
    auto signature = generate(signUtils.CanonicalString(), signerParam.Cred().AccessKeySecret());

    OSS_LOG(LogLevel::LogDebug, TAG, "CanonicalString:%s", signUtils.CanonicalString().c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "signature:%s", signature.c_str());

    parameters["Expires"] = date;
    parameters["OSSAccessKeyId"] = signerParam.Cred().AccessKeyId();
    parameters["Signature"] = signature;
}