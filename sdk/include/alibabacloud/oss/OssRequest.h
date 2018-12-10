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
#include <alibabacloud/oss/ServiceRequest.h>

namespace AlibabaCloud
{
namespace OSS
{
    class OssClientImpl;
    class  ALIBABACLOUD_OSS_EXPORT OssRequest : public ServiceRequest
    {
    public:
        OssRequest();
        virtual ~ OssRequest() = default;
        virtual HeaderCollection Headers() const;
        virtual ParameterCollection Parameters() const;
        virtual std::shared_ptr<std::iostream> Body() const;
    protected:
        OssRequest(const std::string& bucket, const std::string& key);
        friend class OssClientImpl;

        virtual int validate() const;
        const char *validateMessage(int code) const;
        
        virtual std::string payload() const;
        virtual HeaderCollection specialHeaders() const;
        virtual ParameterCollection specialParameters() const;
        
        const std::string& bucket() const;
        const std::string& key()  const;
    protected:
        std::string bucket_;
        std::string key_;
    };

    class ALIBABACLOUD_OSS_EXPORT OssBucketRequest : public OssRequest
    {
    public:
        OssBucketRequest(const std::string &bucket):
            OssRequest(bucket, "")
        {}
        void setBucket(const std::string &bucket);
        const std::string& Bucket() const;
    protected:
        virtual int validate() const;
    };

    class ALIBABACLOUD_OSS_EXPORT OssObjectRequest : public OssRequest
    {
    public:
        OssObjectRequest(const std::string &bucket, const std::string &key) :
            OssRequest(bucket, key)
        {}
        void setBucket(const std::string &bucket);
        const std::string& Bucket() const;

        void setKey(const std::string &key);
        const std::string& Key() const;
    protected:
        virtual int validate() const;
    };
}
}
