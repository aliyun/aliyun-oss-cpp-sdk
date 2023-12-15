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

#include "AuthSigner.h"
#include "HmacSha256Signer.h"
#include "../utils/LogUtils.h"
#include "../utils/Utils.h"
#include "../utils/SignUtils.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <alibabacloud/oss/http/HttpType.h>

namespace AlibabaCloud
{
namespace OSS
{
    class  AuthSignerV4 : public AuthSigner
    {
    public:
        static const char* X_OSS_CONTENT_SHA256;
        static const char* X_OSS_DATE;
        
        AuthSignerV4(const std::string &region, const std::string &product);
        virtual ~AuthSignerV4() = default;
        virtual bool signRequest(HttpRequest &request, const AuthSignerParam& param) const override;
    private:
        std::string genCanonicalReuqest(const std::string &method,
                                        const std::string &resource,
                                        const HeaderCollection &headers,
                                        const ParameterCollection &parameters,
                                        const HeaderSet &additionalHeaders,
                                        const std::string &addiHeadersStr) const;

        std::string genStringToSign(const std::string &canonical, const std::string &date, const std::string &scope, const std::string &algoName) const;

        std::string genSignature(const std::string &accessKeySecret, const std::shared_ptr<Signer> &signAlgo,
                                    const std::string &day, const std::string &region, const std::string &product,
                                    const std::string &stringToSign) const;

        std::string genAuthStr(const std::string &accessKeyId, const std::string &scope,
                        const std::string &addiHeadersStr, const std::string &signature) const;

        void addHeaders(HttpRequest& request, const AuthSignerParam& param) const;

        bool mustToSignHeader(const std::string &headerKey) const;

        std::string addiHeaderToStr(const HeaderSet &additionalHeaders, const HeaderCollection &headers) const;
    private:
        std::string region_;
        std::string product_;
    };
}
}