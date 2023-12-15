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

#include "AuthSignerV1.h"
#include "../utils/Utils.h"
#include "../utils/SignUtils.h"
#include "HmacSha1Signer.h"
#include <sstream>

using namespace AlibabaCloud::OSS;

AuthSignerV1::AuthSignerV1() {
    signAlgo_ = std::make_shared<HmacSha1Signer>();
}

bool AuthSignerV1::signRequest(HttpRequest& request, const AuthSignerParam& param) const
{
    if (!request.hasHeader(Http::DATE)) {
        request.addHeader(Http::DATE, ToGmtTime(param.RequestTime()));
    }

    std::string method = Http::MethodToString(request.method());

    std::string resource;
    resource.append("/");
    if (!param.Bucket().empty()) {
        resource.append(param.Bucket());
        resource.append("/");
    }
    if (!param.Key().empty()) {
        resource.append(param.Key());
    }

    std::string date = request.Header(Http::DATE);

    SignUtils signUtils("");
    signUtils.build(method, resource, date, request.Headers(), param.Parameters());

    auto signature = signAlgo_->generate(signUtils.CanonicalString(), param.Cred().AccessKeySecret());

    std::stringstream authValue;
    authValue
        << "OSS "
        << param.Cred().AccessKeyId()
        << ":"
        << signature;

    request.addHeader(Http::AUTHORIZATION, authValue.str());
    return true;
} 