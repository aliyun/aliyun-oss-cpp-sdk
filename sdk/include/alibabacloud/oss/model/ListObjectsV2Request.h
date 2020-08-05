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
#include <alibabacloud/oss/Export.h>
#include <alibabacloud/oss/OssRequest.h>

namespace AlibabaCloud
{
namespace OSS
{
    class ALIBABACLOUD_OSS_EXPORT ListObjectsV2Request: public OssBucketRequest
    {
    public:
        ListObjectsV2Request(const std::string& bucket):
            OssBucketRequest(bucket),
            delimiterIsSet_(false),
            startAfterIsSet_(false),
            continuationTokenIsSet_(false),
            maxKeysIsSet_(false),
            prefixIsSet_(false),
            encodingTypeIsSet_(false),
            fetchOwnerIsSet_(false),
            requestPayer_(RequestPayer::NotSet)
        {
        }
        void setDelimiter(const std::string& delimiter) { delimiter_ = delimiter; delimiterIsSet_ = true; }
        void setStartAfter(const std::string& value) { startAfter_ = value; startAfterIsSet_ = true;}
        void setContinuationToken(const std::string& value) { continuationToken_ = value; continuationTokenIsSet_ = true; }
        void setMaxKeys(int maxKeys) {maxKeys_ = maxKeys; maxKeysIsSet_ = true;}
        void setPrefix(const std::string& prefix) { prefix_ = prefix; prefixIsSet_ = true; }
        void setEncodingType(const std::string& type) { encodingType_ = type; encodingTypeIsSet_ = true; }
        void setFetchOwner(bool value) { fetchOwner_ = value; fetchOwnerIsSet_ = true; }
        void setRequestPayer(RequestPayer value) { requestPayer_ = value; }

    protected:
        virtual ParameterCollection specialParameters() const;
        virtual HeaderCollection specialHeaders() const;
    private:
        std::string delimiter_;
        bool delimiterIsSet_;
        std::string startAfter_;
        bool startAfterIsSet_;
        std::string continuationToken_;
        bool continuationTokenIsSet_;
        int maxKeys_;
        bool maxKeysIsSet_;
        std::string prefix_;
        bool prefixIsSet_;
        std::string encodingType_;
        bool encodingTypeIsSet_;
        bool fetchOwner_;
        bool fetchOwnerIsSet_;
        RequestPayer requestPayer_;
    };
} 
}
