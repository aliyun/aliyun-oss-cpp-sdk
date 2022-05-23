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
#include <memory>
#include <iostream>
#include <alibabacloud/oss/OssResult.h>

namespace AlibabaCloud
{
namespace OSS
{
    class ALIBABACLOUD_OSS_EXPORT GetBucketStatResult : public OssResult
    {
    public:
        GetBucketStatResult();
        GetBucketStatResult(const std::string& data);
        GetBucketStatResult(const std::shared_ptr<std::iostream>& data);
        GetBucketStatResult& operator=(const std::string& data);
        uint64_t Storage() const { return storage_; }
        uint64_t ObjectCount() const { return objectCount_; }
        uint64_t MultipartUploadCount() const { return multipartUploadCount_; }
        uint64_t LiveChannelCount() const { return liveChannelCount_; }
        uint64_t LastModifiedTime() const { return lastModifiedTime_; }
        uint64_t StandardStorage() const { return standardStorage_; }
        uint64_t StandardObjectCount() const { return standardObjectCount_; }
        uint64_t InfrequentAccessStorage() const { return infrequentAccessStorage_; }
        uint64_t InfrequentAccessObjectCount() const { return infrequentAccessObjectCount_; }
        uint64_t ArchiveStorage() const { return archiveStorage_; }
        uint64_t ArchiveObjectCount() const { return archiveObjectCount_; }
        uint64_t ColdArchiveStorage() const { return coldArchiveStorage_; }
        uint64_t ColdArchiveObjectCount() const { return coldArchiveObjectCount_; }

    private:
        uint64_t storage_;
        uint64_t objectCount_;
        uint64_t multipartUploadCount_;
        uint64_t liveChannelCount_;
        uint64_t lastModifiedTime_;
        uint64_t standardStorage_;
        uint64_t standardObjectCount_;
        uint64_t infrequentAccessStorage_;
        uint64_t infrequentAccessObjectCount_;
        uint64_t archiveStorage_;
        uint64_t archiveObjectCount_;
        uint64_t coldArchiveStorage_;
        uint64_t coldArchiveObjectCount_;
    };
}
}
