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
#include <string>
#include <alibabacloud/oss/http/HttpRequest.h>
#include <alibabacloud/oss/auth/Credentials.h>
#include "HmacSha1Signer.h"


namespace AlibabaCloud
{
namespace OSS
{
    class AuthSignerParam
    {
    public:
        AuthSignerParam(std::string&& bucket, std::string&& key, Credentials&& credentials, std::time_t &&requestTime):
            bucket_(bucket),
            key_(key),
            credentials_(credentials),
            requestTime_(requestTime)
        {}

        AuthSignerParam(const std::string& bucket, const std::string& key, const Credentials& credentials, const std::time_t &requestTime) :
            bucket_(bucket),
            key_(key),
            credentials_(credentials),
            requestTime_(requestTime)
        {}

        inline const std::string& Bucket() const { return bucket_; }
        inline const std::string& Key() const { return key_; }
        inline const ParameterCollection& Parameters() const { return parameters_; }
        inline const Credentials& Cred() const { return credentials_; }
        inline const HeaderSet& AddiHeaders() const { return additionalHeaders_; }
        inline const std::time_t& RequestTime() const { return requestTime_;}
        inline void setParameters(const ParameterCollection& parameters) { parameters_ = parameters; }
        inline void setAdditionalHeaders(const HeaderSet& additionalHeaders) { additionalHeaders_ = additionalHeaders; }

    private:
        AuthSignerParam() = delete;

    private:
        std::string bucket_;
        std::string key_;
        ParameterCollection parameters_;
        Credentials credentials_;
        HeaderSet additionalHeaders_;
        std::time_t requestTime_;
    };

    class  AuthSigner
    {
    public:
        virtual bool signRequest(HttpRequest &request, const AuthSignerParam & param) const = 0;
    public:
        virtual ~AuthSigner() = default;
        static std::shared_ptr<AuthSigner> CreateSigner(const std::string &region = "", const std::string &version = "1.0", const std::string &product = "oss");
    protected:
        std::shared_ptr<Signer> signAlgo_;
    };
}
}