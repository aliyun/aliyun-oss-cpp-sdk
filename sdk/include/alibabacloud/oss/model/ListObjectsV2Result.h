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
#include <alibabacloud/oss/model/Bucket.h>
#include <alibabacloud/oss/model/ListObjectsResult.h>
#include <vector>
#include <memory>
#include <iostream>
#include <alibabacloud/oss/OssResult.h>
#include <alibabacloud/oss/model/Owner.h>

namespace AlibabaCloud
{
namespace OSS
{
    class ALIBABACLOUD_OSS_EXPORT ListObjectsV2Result : public OssResult
    {
    public:
        ListObjectsV2Result();
        ListObjectsV2Result(const std::string& data);
        ListObjectsV2Result(const std::shared_ptr<std::iostream>& data);
        ListObjectsV2Result& operator=(const std::string& data);
        const std::string& Name() const { return name_; }
        const std::string& Prefix() const { return prefix_; }
        const std::string& StartAfter() const { return startAfter_; }
        const std::string& ContinuationToken() const { return continuationToken_; }
        const std::string& NextContinuationToken() const { return nextContinuationToken_; }
        const std::string& Delimiter() const { return delimiter_; }
        const std::string& EncodingType() const { return encodingType_; }
        int MaxKeys() const { return maxKeys_; }
        int KeyCount() const { return keyCount_; }
        bool IsTruncated() const { return isTruncated_; }
        const CommonPrefixeList& CommonPrefixes() const { return commonPrefixes_; }
        const ObjectSummaryList& ObjectSummarys() const { return objectSummarys_; }
    private:
        std::string name_;
        std::string prefix_;
        std::string startAfter_;
        std::string continuationToken_;
        std::string nextContinuationToken_;
        std::string delimiter_;
        std::string encodingType_;
        int         maxKeys_;
        int         keyCount_;
        bool        isTruncated_;
        CommonPrefixeList commonPrefixes_;
        ObjectSummaryList objectSummarys_;
    };
} 
}
