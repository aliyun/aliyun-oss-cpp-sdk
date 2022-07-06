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


namespace AlibabaCloud
{
namespace OSS
{
    class AuthSignerParam
    {
    public:
        AuthSignerParam(std::string&& bucket, std::string&& key, ParameterCollection&& parameters, Credentials&& credentials):
            bucket_(bucket),
            key_(key),
            parameters_(parameters),
            credentials_(credentials)
        {}

        AuthSignerParam(std::string& bucket, std::string& key, ParameterCollection& parameters, Credentials& credentials) :
            bucket_(bucket),
            key_(key),
            parameters_(parameters),
            credentials_(credentials)
        {}

        const std::string& Bucket() const { return bucket_; }
        const std::string& Key() const { return key_; }
        const ParameterCollection& Parameters() const { return parameters_; }
        const Credentials& Cred() const { return credentials_; }


    private:
        AuthSignerParam() = delete;

    private:
        std::string bucket_;
        std::string key_;
        ParameterCollection parameters_;
        Credentials credentials_;
    };

    class  AuthSigner
    {
    public:
        virtual bool signRequest(HttpRequest &request, const AuthSignerParam & param) const = 0;
    public:
        virtual ~AuthSigner() = default;
        static std::shared_ptr<AuthSigner> CreateSigner(const std::string &version = "1.0");
    };
}
}
