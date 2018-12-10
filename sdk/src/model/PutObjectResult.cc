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


#include <alibabacloud/oss/model/PutObjectResult.h>
#include "../utils/Utils.h"
#include <alibabacloud/oss/http/HttpType.h>
using namespace AlibabaCloud::OSS;

PutObjectResult::PutObjectResult():
    OssResult()
{
}

PutObjectResult::PutObjectResult(const HeaderCollection & header) :
    OssResult()
{
    if (header.find(Http::ETAG) != header.end())
    {
        eTag_ = TrimQuotes(header.at(Http::ETAG).c_str());
    }

    if (header.find("x-oss-hash-crc64ecma") != header.end()) {
        crc64_ = std::strtoull(header.at("x-oss-hash-crc64ecma").c_str(), nullptr, 10);
    }

    if (header.find("x-oss-request-id") != header.end()) {
        requestId_ = header.at("x-oss-request-id");
    }
}

const std::string& PutObjectResult::ETag() const
{
    return eTag_;
}

uint64_t PutObjectResult::CRC64() 
{
    return crc64_;
}
