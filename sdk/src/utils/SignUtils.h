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

#pragma once

#include <string>
#include <map>
#include <ctime>
#include <iostream>
#include <alibabacloud/oss/Types.h>

namespace AlibabaCloud
{
    namespace OSS
    {

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

        class SignUtils
        {
        public:
            SignUtils(const std::string &version);
            ~SignUtils();

            void build(const std::string &method,
                       const std::string &resource,
                       const std::string &date,
                       const HeaderCollection &headers,
                       const ParameterCollection &parameters,
                       const HeaderSet &additionalHeaders);

            void build(const std::string &expires,
                       const std::string &resource,
                       const ParameterCollection &parameters);

            const std::string &CanonicalString() const;

        private:
            std::string signVersion_;
            std::string canonicalString_;
        };
    }
}
