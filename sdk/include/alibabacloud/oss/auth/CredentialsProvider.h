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
#include "Credentials.h"

namespace AlibabaCloud
{
namespace OSS
{
    class ALIBABACLOUD_OSS_EXPORT CredentialsProvider
    {
    public:
        CredentialsProvider() = default;
        virtual ~CredentialsProvider();
        virtual Credentials getCredentials() = 0;
    private:

    };

    class ALIBABACLOUD_OSS_EXPORT SimpleCredentialsProvider : public CredentialsProvider
    {
    public:
        SimpleCredentialsProvider(const Credentials& credentials);
        SimpleCredentialsProvider(const std::string& accessKeyId,
            const std::string& accessKeySecret, const std::string& securityToken = "");
        ~SimpleCredentialsProvider();

        virtual Credentials getCredentials() override;
    private:
        Credentials credentials_;
    };

    class ALIBABACLOUD_OSS_EXPORT EnvironmentVariableCredentialsProvider : public CredentialsProvider
    {
    public:
        Credentials getCredentials() override;
    };
}
}
