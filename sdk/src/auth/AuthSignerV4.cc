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

#include "AuthSignerV4.h"


using namespace AlibabaCloud::OSS;

namespace
{
    const char *TAG = "AuthSignerV4";
}

const char* AuthSignerV4::X_OSS_CONTENT_SHA256 = "X-Oss-Content-Sha256";
const char* AuthSignerV4::X_OSS_DATE = "X-Oss-Date";

AuthSignerV4::AuthSignerV4(const std::string &region, const std::string &product) : region_(region), product_(product) {
  signAlgo_ = std::make_shared<HmacSha256Signer>();
}

bool AuthSignerV4::mustToSignHeader(const std::string &headerKey) const {
    return headerKey == "content-type" 
        || headerKey == "content-md5" 
        || headerKey.compare(0, 6, "x-oss-") == 0;
}

std::string AuthSignerV4::genCanonicalReuqest(const std::string &method,
                                                 const std::string &resource,
                                                 const HeaderCollection &headers,
                                                 const ParameterCollection &parameters,
                                                 const HeaderSet &additionalHeaders,
                                                 const std::string &addiHeadersStr) const
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
    ss << method << "\n"; 
    // UriEncode(<Resource>) + "\n"
    ss << UrlEncodeIgnoreSlash(resource) << "\n"; 

    // Canonical Query String + "\n"
    // UriEncode(<QueryParam1>) + "=" + UriEncode(<Value>) + "&" + UriEncode(<QueryParam2>) + "\n"
    std::map<std::string, std::string> encodeParams;
    for (auto const &param : parameters) {
        encodeParams[UrlEncode(param.first)] = UrlEncode(param.second);
    }

    char separator = '&';
    bool isFirstParam = true;
    for (auto const &param : encodeParams)
    {
        if (!isFirstParam)
        {
            ss << separator;
        }
        else
        {
            isFirstParam = false;
        }

        ss << param.first;
        if (!param.second.empty())
        {
            ss << "=" << param.second;
        }
    }
    ss << "\n";

    // Lowercase(<HeaderName1>) + ":" + Trim(<value>) + "\n" + Lowercase(<HeaderName2>) + ":" + Trim(<value>) + "\n" + "\n"
    for (const auto &header : headers)
    {
        std::string lowerKey = Trim(ToLower(header.first.c_str()).c_str());
        std::string value = Trim(header.second.c_str());
        if (mustToSignHeader(lowerKey) ||
            additionalHeaders.find(lowerKey) != additionalHeaders.end()) {
            ss << lowerKey << ":" << value << "\n";
        }
    }
    ss << "\n";

    // Lowercase(<AdditionalHeaderName1>) + ";" + Lowercase(<AdditionalHeaderName2>) + "\n" +
    ss << addiHeadersStr << "\n"
       << Trim(headers.at(X_OSS_CONTENT_SHA256).c_str());
    return ss.str();
}

std::string AuthSignerV4::genStringToSign(const std::string &canonical, const std::string &date, const std::string &scope, const std::string &algoName) const
{
    // Hex(SHA256Hash(Canonical Reuqest))
    ByteBuffer hash(SHA256_DIGEST_LENGTH);

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, canonical.c_str(), canonical.size());
    SHA256_Final(hash.data(), &sha256);

    std::string hashedCalRequest = LowerHexToString(hash);

    // "OSS4-HMAC-SHA256" + "\n" +
    // TimeStamp + "\n" +
    // Scope + "\n" +
    // Hex(SHA256Hash(Canonical Reuqest))
    std::stringstream stringToSign;
    stringToSign << "OSS4-" << algoName << "\n"
                 << date << "\n"
                 << scope << "\n"
                 << hashedCalRequest;

    return stringToSign.str();
}

std::string AuthSignerV4::genSignature(const std::string &accessKeySecret, const std::shared_ptr<Signer> &signAlgo,
                                          const std::string &day, const std::string &region, const std::string &product,
                                          const std::string &stringToSign) const
{
    // HMACSHA256(HMACSHA256(HMACSHA256(HMACSHA256("aliyun_v4"+SK,Date),Region),oss),"aliyun_v4_request");
    std::string toKey = "aliyun_v4" + accessKeySecret;
    ByteBuffer signingSecret = ByteBuffer{toKey.begin(), toKey.end()};
    ByteBuffer signingDate = signAlgo->calculate(day, signingSecret);
    ByteBuffer signingRegion = signAlgo->calculate(region, signingDate);
    ByteBuffer signingService = signAlgo->calculate(product, signingRegion);
    ByteBuffer signingKey = signAlgo->calculate("aliyun_v4_request", signingService);
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) signingSecret:\n%s\n", this, LowerHexToString(signingSecret).c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) signingDate:\n%s\ndate:%s", this, LowerHexToString(signingDate).c_str(), day.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) signingRegion:\n%s\nregion:%s", this, LowerHexToString(signingRegion).c_str(), region.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) signingService:\n%s\nproduct:%s", this, LowerHexToString(signingService).c_str(), product.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) signingKey:\n%s", this, LowerHexToString(signingKey).c_str());
    ByteBuffer signSrc = signAlgo->calculate(stringToSign, signingKey);

    return LowerHexToString(signSrc);
}

std::string AuthSignerV4::addiHeaderToStr(const HeaderSet &additionalHeaders, const HeaderCollection &headers) const {
    if (additionalHeaders.empty()) {
        return std::string();
    }
    
    std::stringstream additionalSS;
    bool isFirstHeader = true;

    for (const auto &addHeader : additionalHeaders)
    {
        std::string lowerKey = Trim(ToLower(addHeader.c_str()).c_str());
        if (headers.find(lowerKey) == headers.end() || mustToSignHeader(lowerKey))
        {
            continue;
        }

        if (!isFirstHeader)
        {
            additionalSS << ";";
        }
        else
        {
            isFirstHeader = false;
        }

        additionalSS << lowerKey;
    }

    return additionalSS.str();
}

std::string AuthSignerV4::genAuthStr(const std::string &accessKeyId, const std::string &scope,
                        const std::string &addiHeadersStr, const std::string &signature) const {
    std::stringstream authValue;
    authValue
        << "OSS4-HMAC-SHA256 Credential=" << accessKeyId << "/" << scope;

    if (!addiHeadersStr.empty())
    {
        authValue << ",AdditionalHeaders=" << addiHeadersStr;
    }

    authValue << ",Signature=" << signature;
    return authValue.str();
}

void AuthSignerV4::addHeaders(HttpRequest& request, const AuthSignerParam& param) const {
    // Date
    if (!request.hasHeader(X_OSS_DATE)) {
        request.addHeader(X_OSS_DATE, ToUtcTimeWithoutMill(param.RequestTime()));
    }

    // Sha256
    request.addHeader(X_OSS_CONTENT_SHA256, "UNSIGNED-PAYLOAD");

    // host
    if (param.AddiHeaders().find(Http::HOST) != param.AddiHeaders().end() &&
        !request.hasHeader(Http::HOST)) {
            request.addHeader(Http::HOST, request.url().host());
    }
}

bool AuthSignerV4::signRequest(HttpRequest& request, const AuthSignerParam& param) const {
    std::string method = Http::MethodToString(request.method());

    std::string resource = GenResource(param.Bucket(), param.Key());

    addHeaders(request, param);
    std::string addiHeadersStr = addiHeaderToStr(param.AddiHeaders(), request.Headers());
    std::string canonical = genCanonicalReuqest(method, resource, request.Headers(), param.Parameters(), param.AddiHeaders(), addiHeadersStr);

    std::string date = request.Header(X_OSS_DATE);
    // convert to "20060102" time format
    std::string day(date.begin(), date.begin() + 8);

    std::string scope = GenScope(day, region_, product_, "aliyun_v4_request");
    std::string stringToSign = genStringToSign(canonical, date, scope, signAlgo_->name());
    std::string signature = genSignature(param.Cred().AccessKeySecret(), signAlgo_, day, region_, product_, stringToSign);
    std::string authValue = genAuthStr(param.Cred().AccessKeyId(), scope, addiHeadersStr, signature);

    request.addHeader(Http::AUTHORIZATION, authValue);

    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) CanonicalString:\n%s", this, canonical.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) stringToSign:\n%s", this, stringToSign.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) signature:\n%s", this, signature.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) Authorization:\n%s", this, authValue.c_str());
    return true;
}