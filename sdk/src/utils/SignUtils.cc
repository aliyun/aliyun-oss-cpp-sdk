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

#include "SignUtils.h"
#include "Utils.h"
#include <sstream>
#include <map>
#include <set>
#include <alibabacloud/oss/Const.h>
#include <alibabacloud/oss/Types.h>
#include <alibabacloud/oss/http/HttpType.h>

using namespace AlibabaCloud::OSS;

const static std::set<std::string> ParamtersToSign =
    {
        "acl", "location", "bucketInfo", "stat", "referer", "cors", "website", "restore",
        "logging", "symlink", "qos", "uploadId", "uploads", "partNumber",
        "response-content-type", "response-content-language", "response-expires",
        "response-cache-control", "response-content-disposition", "response-content-encoding",
        "append", "position", "lifecycle", "delete", "live", "status", "comp", "vod",
        "startTime", "endTime", "x-oss-process", "security-token", "objectMeta",
        "callback", "callback-var", "tagging", "policy", "requestPayment", "x-oss-traffic-limit",
        "encryption", "qosInfo", "versioning", "versionId", "versions",
        "x-oss-request-payer", "sequential", "inventory", "inventoryId", "continuation-token",
        "worm", "wormId", "wormExtend"};

SignUtils::SignUtils(const std::string &version) : signVersion_(version),
                                                   canonicalString_()
{
}

SignUtils::~SignUtils()
{
}

const std::string &SignUtils::CanonicalString() const
{
    return canonicalString_;
}

void SignUtils::build(const std::string &method,
                      const std::string &resource,
                      const std::string &date,
                      const HeaderCollection &headers,
                      const ParameterCollection &parameters,
                      const HeaderSet &additionalHeaders)
{
    if (signVersion_ == "V4")
    {
        buildV4(method, resource, headers, parameters, additionalHeaders);
    }
    else
    {
        buildV1V2(method, resource, date, headers, parameters, additionalHeaders);
    }
}

void SignUtils::build(const std::string &expires,
                      const std::string &resource,
                      const ParameterCollection &parameters)
{
    std::stringstream ss;
    ss << expires << '\n';
    for (auto const &param : parameters)
    {
        ss << param.first << ":" << param.second << '\n';
    }
    ss << resource;
    canonicalString_ = ss.str();
}

void SignUtils::buildV1V2(const std::string &method,
                          const std::string &resource,
                          const std::string &date,
                          const HeaderCollection &headers,
                          const ParameterCollection &parameters,
                          const HeaderSet &additionalHeaders)
{
    std::stringstream ss;

    /*Version 1*/
    // VERB + "\n" +
    // Content-MD5 + "\n"  +
    // Content-Type + "\n"
    // Date + "\n"  +
    // CanonicalizedOSSHeaders +
    // CanonicalizedResource) +

    // common headers
    ss << method << "\n";
    if (headers.find(Http::CONTENT_MD5) != headers.end())
    {
        ss << headers.at(Http::CONTENT_MD5);
    }
    ss << "\n";
    if (headers.find(Http::CONTENT_TYPE) != headers.end())
    {
        ss << headers.at(Http::CONTENT_TYPE);
    }
    ss << "\n";
    // Date or EXPIRES
    ss << date << "\n";

    // CanonicalizedOSSHeaders, start with x-oss-
    for (const auto &header : headers)
    {
        std::string lowerKey = Trim(ToLower(header.first.c_str()).c_str());
        std::string value = Trim(header.second.c_str());
        if (lowerKey.compare(0, 6, "x-oss-", 6) == 0)
        {
            ss << lowerKey << ":" << value << "\n";
        }
    }

    // for v2 version: AdditionalHeadersNormalizedVal + "\n"
    if (signVersion_ == "V2")
    {
        std::stringstream additionalSS;
        bool isFirstHeader = true;
        for (const auto &addHeader : additionalHeaders)
        {
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
        ss << additionalSS.str() << "\n";
    }

    // CanonicalizedResource, the sub resouce in
    ss << resource;
    char separator = '?';
    for (auto const &param : parameters)
    {
        if (ParamtersToSign.find(param.first) == ParamtersToSign.end())
        {
            continue;
        }

        ss << separator;
        ss << param.first;
        if (!param.second.empty())
        {
            ss << "=" << param.second;
        }
        separator = '&';
    }

    canonicalString_ = ss.str();
}

void SignUtils::buildV4(const std::string &method,
                        const std::string &resource,
                        const HeaderCollection &headers,
                        const ParameterCollection &parameters,
                        const HeaderSet &additionalHeaders)
{
    /*Version 4*/
    // HTTP Verb + "\n" +
    // Canonical URI + "\n" +
    // Canonical Query String + "\n" +
    // Canonical Headers + "\n" +
    // Additional Headers + "\n" +
    // Hashed PayLoad

    std::stringstream ss;

    ss << method << "\n";   // HTTP Verb + "\n"
    ss << resource << "\n"; // Canonical URI + "\n"

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

        ss << param.first;
        if (!param.second.empty())
        {
            ss << "=" << param.second;
        }
    }
    ss << "\n";

    // Canonical Headers + "\n" +
    // Additional Headers + "\n" +
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

    std::stringstream additionalSS;
    bool isFirstHeader = true;
    for (const auto &addHeader : additionalHeaders)
    {
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

    canonicalString_ = ss.str();
}

void SignUtils::genAdditionalHeader(const HeaderCollection &headers, HeaderSet &additionalHeaders)
{
    // osshead: include addtional head、x-oss-、content-md5、content-type
    for (const auto &header : headers)
    {
        std::string lowerKey = Trim(ToLower(header.first.c_str()).c_str());
        std::string value = Trim(header.second.c_str());
        if (lowerKey.compare(0, 6, "x-oss-", 6) != 0 && lowerKey != "user-agent")
        {
            // V4 addtional head不包括content-md5、content-type
            if (signVersion_ == "4.0" && lowerKey.compare(0, 8, "content-", 8) == 0)
            {
                continue;
            }
            additionalHeaders.insert(lowerKey);
        }
    }
}