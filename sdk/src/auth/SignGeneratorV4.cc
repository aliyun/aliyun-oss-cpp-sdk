#include "SignGeneratorV4.h"
#include <openssl/sha.h>
#include <time.h>
#include <openssl/hmac.h>
#ifdef OPENSSL_IS_BORINGSSL
#include <openssl/base64.h>
#endif

using namespace AlibabaCloud::OSS;

namespace
{
    const char *TAG = "SignGeneratorV4";
}

std::string SignGeneratorV4::genCanonicalReuqest(const std::string &method,
                                                 const std::string &resource,
                                                 const HeaderCollection &headers,
                                                 const ParameterCollection &parameters,
                                                 const HeaderSet &additionalHeaders) const
{
    /*Version 4*/
    // HTTP Verb + "\n" +
    // Canonical URI + "\n" +
    // Canonical Query String + "\n" +
    // Canonical Headers + "\n" +
    // Additional Headers + "\n" +
    // Hashed PayLoad

    std::stringstream ss;

    ss << method << "\n"; // "GET" | "PUT" | "POST" | ... + "\n"
    // UrlEncode
    ss << UrlEncode(resource, true) << "\n"; // UriEncode(<Resource>) + "\n"

    // Canonical Query String + "\n"
    // UriEncode(<QueryParam1>) + "=" + UriEncode(<Value>) + "&" + UriEncode(<QueryParam2>) + "\n"
    char separator = '&';
    bool isFirstParam = true;
    for (auto const &param : parameters)
    {
        if (ParamtersToSign.find(param.first) == ParamtersToSign.end())
        {
            continue;
        }

        if (!isFirstParam)
        {
            ss << separator;
        }
        else
        {
            isFirstParam = false;
        }

        ss << UrlEncode(param.first);
        if (!param.second.empty())
        {
            ss << "=" << UrlEncode(param.second);
        }
    }
    ss << "\n";

    // Lowercase(<HeaderName1>) + ":" + Trim(<value>) + "\n" + Lowercase(<HeaderName2>) + ":" + Trim(<value>) + "\n" + "\n"
    std::string playload;
    for (const auto &header : headers)
    {
        std::string lowerKey = Trim(ToLower(header.first.c_str()).c_str());
        std::string value = Trim(header.second.c_str());
        ss << lowerKey << ":" << value << "\n";
        if (lowerKey == "x-oss-content-sha256")
        {
            // hashed payload
            playload = value;
        }
    }

    // Lowercase(<AdditionalHeaderName1>) + ";" + Lowercase(<AdditionalHeaderName2>) + "\n" +
    std::stringstream additionalSS;
    bool isFirstHeader = true;
    for (const auto &addHeader : additionalHeaders)
    {
        if (headers.find(addHeader) == headers.end())
        {
            continue;
        }

        if (isFirstHeader)
        {
            additionalSS << ";";
        }
        else
        {
            isFirstHeader = false;
        }
        additionalSS << addHeader.c_str();
    }

    ss << "\n"
       << additionalSS.str() << "\n"
       << playload;

    return ss.str();
}

std::string SignGeneratorV4::genStringToSign(const std::string &canonical, const std::string &date, const std::string &scope, const std::string &algoName) const
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

std::string SignGeneratorV4::genSignature(const std::string &accessKeySecret, const std::shared_ptr<Signer> &signAlgo,
                                          const std::string &day, const std::string &region, const std::string &product,
                                          const std::string &stringToSign, const std::string &canonical) const
{
    // HMACSHA256(HMACSHA256(HMACSHA256(HMACSHA256("aliyun_v4"+SK,Date),Region),oss),"aliyun_v4_request");
    std::string toKey = "aliyun_v4" + accessKeySecret;
    ByteBuffer signingKey = signAlgo->calculate(signAlgo->calculate(signAlgo->calculate(signAlgo->calculate(ByteBuffer{toKey.begin(), toKey.end()}, day), region), product), "aliyun_v4_request");
    // HEX(HMAC-SHA256(SigningKey,StringToSign))
    ByteBuffer signSrc = signAlgo->calculate(signingKey, stringToSign);

    ByteBuffer hash(SHA256_DIGEST_LENGTH);
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, canonical.c_str(), canonical.size());
    SHA256_Final(hash.data(), &sha256);

    return LowerHexToString(hash);
}

std::string genAuthStr(const std::string &accessKeyId, const std::string &scope,
                        const HeaderSet &additionalHeaders, const std::string &signature) {
    std::stringstream authValue;
    authValue
        << "OSS4-HMAC-SHA256 Credential=" << accessKeyId << "/" << scope;

    if (!additionalHeaders.empty())
    {
        authValue << ",AdditionalHeaders=";
        bool isFirstHeader = true;
        for (const auto &addHeader : additionalHeaders)
        {
            if (!isFirstHeader)
            {
                authValue << ";";
            }
            else
            {
                isFirstHeader = false;
            }
            authValue << addHeader.c_str();
        }
    }

    authValue << ",Signature=" << signature;
    return authValue.str();
}

void SignGeneratorV4::signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const SignParam &signParam) const
{
    if (!httpRequest->hasHeader("x-oss-date")) {
        std::time_t t = std::time(nullptr);
        t += signParam.dateOffset_;
        httpRequest->addHeader("x-oss-date", ToUtcTimeWithoutMill(t));
    }

    if (!signParam.credentials_.SessionToken().empty())
    {
        httpRequest->addHeader("x-oss-security-token", signParam.credentials_.SessionToken());
    }
    httpRequest->addHeader("x-oss-content-sha256", "UNSIGNED-PAYLOAD");

    // Sort the parameters
    ParameterCollection parameters;
    for (auto const &param : signParam.params_)
    {
        parameters[param.first] = param.second;
    }

    std::string method = Http::MethodToString(httpRequest->method());

    std::string date = httpRequest->Header("x-oss-date");

    std::stringstream scope;
    // convert to "20060102" time format
    std::string day(date.begin(), date.begin() + 8);

    std::string region;
    std::string product;

    if (signParam.cloudBoxId_.empty())
    {
        region = signParam.region_;
    }
    else
    {
        region = signParam.cloudBoxId_;
    }
    scope << day
          << "/" << region
          << "/" << signParam.product_
          << "/aliyun_v4_request";

    std::string canonical = genCanonicalReuqest(method, signParam.resource_, httpRequest->Headers(), parameters, signParam.additionalHeaders_);
    std::string stringToSign = genStringToSign(canonical, date, scope.str(), signAlgo_->name());
    std::string signature = genSignature(signParam.credentials_.AccessKeySecret(), signAlgo_, day, region, product, stringToSign, canonical);
    std::string authValue = genAuthStr(signParam.credentials_.AccessKeyId(), scope.str(), signParam.additionalHeaders_, signature);

    httpRequest->addHeader(Http::AUTHORIZATION, authValue);

    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) CanonicalString:%s", this, httpRequest.get(), canonical.c_str());
    OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) Authorization:%s", this, httpRequest.get(), authValue.c_str());
}

std::string SignGeneratorV4::presign(const SignParam &signParam) const
{
    (void)(signParam);
    OSS_LOG(LogLevel::LogError, TAG, "V4 version url signature is not supported");
    return "";
}

std::string SignGeneratorV4::signRTMP(const SignParam &signParam) const
{
    (void)(signParam);
    OSS_LOG(LogLevel::LogError, TAG, "V4 version RTMP signature is not supported");
    return "";
}