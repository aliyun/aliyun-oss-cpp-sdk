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

#include "AuthSigner.h"
#include "AuthSignerV1.h"
#include "AuthSignerV4.h"
#include "HmacSha1Signer.h"
#include "HmacSha256Signer.h"

using namespace AlibabaCloud::OSS;

std::shared_ptr<AuthSigner> AuthSigner::CreateSigner(const std::string &region, const std::string &version, const std::string &product)
{
    
    if (version == "4.0") {
        return std::make_shared<AuthSignerV4>(region, product);
    }
    return std::make_shared<AuthSignerV1>();
}