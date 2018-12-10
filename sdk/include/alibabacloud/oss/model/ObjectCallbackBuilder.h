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
#include <alibabacloud/oss/Export.h>
#include <alibabacloud/oss/Types.h>

namespace AlibabaCloud
{
namespace OSS
{
    enum CallbackBodyType
    {
        Url = 0,
        Json
    };

    class ALIBABACLOUD_OSS_EXPORT ObjectCallbackBuilder
    {
    public:
        ObjectCallbackBuilder(const std::string& url, const std::string& body);
        ObjectCallbackBuilder(const std::string& url, const std::string& body,
            const std::string& host, AlibabaCloud::OSS::CallbackBodyType type);
        const std::string& CallbackUrl() const { return callbackUrl_; }
        const std::string& CallbackHost() const { return callbackHost_; }
        const std::string& CallbackBody() const { return callbackBody_; }
        AlibabaCloud::OSS::CallbackBodyType CallbackBodyType() const { return callbackBodyType_; }
        void setCallbackUrl(const std::string& url) { callbackUrl_ = url; }
        void setCallbackHost(const std::string& host) { callbackHost_ = host; }
        void setCallbackBody(const std::string& body) { callbackBody_ = body; }
        void setCallbackBodyType(AlibabaCloud::OSS::CallbackBodyType type) { callbackBodyType_ = type; }
        std::string build();
    private:
        std::string callbackUrl_;
        std::string callbackHost_;
        std::string callbackBody_;
        AlibabaCloud::OSS::CallbackBodyType callbackBodyType_;
    };

    class ALIBABACLOUD_OSS_EXPORT ObjectCallbackVariableBuilder
    {
    public:
        ObjectCallbackVariableBuilder() = default;
        const HeaderCollection& CallbackVariable() const { return callbackVariable_; }
        bool addCallbackVariable(const std::string &key, const std::string& value);
        std::string build();
    private:
        HeaderCollection callbackVariable_;
    };
}
}
