/*
 * Copyright 2009-2018 Alibaba Cloud All rights reserved.
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

#include <alibabacloud/oss/model/UploadPartRequest.h>
#include <alibabacloud/oss/http/HttpType.h>
#include "../utils/Utils.h"
#include "ModelError.h"
#include <alibabacloud/oss/Const.h>
using namespace AlibabaCloud::OSS;

UploadPartRequest::UploadPartRequest(const std::string &bucket, const std::string &key,
    const std::shared_ptr<std::iostream> &content) :
    UploadPartRequest(bucket, key, 0, "", content)
{
}

UploadPartRequest::UploadPartRequest(const std::string &bucket, const std::string &key,
    int partNumber, const std::string &uploadId,
    const std::shared_ptr<std::iostream> &content) :
    OssObjectRequest(bucket, key),
    partNumber_(partNumber),
    uploadId_(uploadId),
    content_(content),
    contentLengthIsSet_(false),
    trafficLimit_(0),
    userAgent_(),
    contentMd5IsSet_(false)
{
    setFlags(Flags() | REQUEST_FLAG_CHECK_CRC64);
}

#ifdef USE_CORO
UploadPartRequest::UploadPartRequest(const std::string &bucket, const std::string& key,
            int partNumber, const std::string& uploadId,
            std::string filename) : UploadPartRequest(bucket, key, partNumber, uploadId, std::make_shared<std::stringstream>())
{
    multipartFilename_ = std::move(filename);
}

std::string_view UploadPartRequest::filename() const {
    return multipartFilename_;
}

void UploadPartRequest::setMultipartFilename(std::string filename) {
    multipartFilename_ = std::move(filename);
}

const std::string & UploadPartRequest::getMultipartFilename() {
    return multipartFilename_;
}
#endif

void UploadPartRequest::setPartNumber(int partNumber)
{
    partNumber_ = partNumber;
}

void UploadPartRequest::setUploadId(const std::string &uploadId)
{
    uploadId_ = uploadId;
}

void UploadPartRequest::setContent(const std::shared_ptr<std::iostream> &content)
{
    content_ = content;
}

void UploadPartRequest::setConetent(const std::shared_ptr<std::iostream> &content)
{
    content_ = content;
}

void UploadPartRequest::setContentLength(uint64_t length)
{
    contentLength_ = length;
    contentLengthIsSet_ = true;
}

void UploadPartRequest::setTrafficLimit(uint64_t value)
{
    trafficLimit_ = value;
}

void UploadPartRequest::setUserAgent(const std::string& ua)
{
    userAgent_ = ua;
}

void UploadPartRequest::setContentMd5(const std::string& value)
{
    contentMd5_ = value;
    contentMd5IsSet_ = true;
}

int UploadPartRequest::PartNumber() const
{
    return partNumber_;
}

std::shared_ptr<std::iostream> UploadPartRequest::Body() const
{
    return content_;
}

HeaderCollection UploadPartRequest::specialHeaders() const
{
    auto headers = OssObjectRequest::specialHeaders();
    headers[Http::CONTENT_TYPE] = "";
    if (contentLengthIsSet_) {
        headers[Http::CONTENT_LENGTH] = std::to_string(contentLength_);
    }
    if (trafficLimit_ != 0) {
        headers["x-oss-traffic-limit"] = std::to_string(trafficLimit_);
    }
    if (!userAgent_.empty()) {
        headers[Http::USER_AGENT] = userAgent_;
    }
    if (contentMd5IsSet_) {
        headers[Http::CONTENT_MD5] = contentMd5_;
    }
    return headers;
}

ParameterCollection UploadPartRequest::specialParameters() const
{
    ParameterCollection parameters;
    parameters["partNumber"] = std::to_string(partNumber_);
    parameters["uploadId"] = uploadId_;
    return parameters;
}

int UploadPartRequest::validate() const
{
    int ret = OssObjectRequest::validate();
    if (ret)
    {
        return ret;
    }
#ifdef USE_CORO
    if(!multipartFilename_.empty()){
        return 0;
    }
#endif    

    if (content_ == nullptr) {
        return ARG_ERROR_REQUEST_BODY_NULLPTR;
    }

    if (content_->bad()) {
        return ARG_ERROR_REQUEST_BODY_BAD_STATE;
    }

    if (content_->fail()) {
        return ARG_ERROR_REQUEST_BODY_FAIL_STATE;
    }

    if (!(partNumber_ > 0 && partNumber_ < PartNumberUpperLimit)) {
        return ARG_ERROR_MULTIPARTUPLOAD_PARTNUMBER_RANGE;
    }

    uint64_t partSize;

    if (contentLengthIsSet_) {
        partSize = contentLength_;
    }
    else {
        partSize = GetIOStreamLength(*content_);
    }

    if (partSize > MaxFileSize) {
        return ARG_ERROR_MULTIPARTUPLOAD_PARTSIZE_RANGE;
    }

    return 0;
}

