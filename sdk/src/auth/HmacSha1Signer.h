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

#include "Signer.h"


namespace AlibabaCloud
{
namespace OSS
{

    class  HmacSha1Signer : public Signer
    {
    public:
        HmacSha1Signer();
        ~HmacSha1Signer();
        
        virtual std::string generate(const std::string &src, const std::string &secret)const override;
        virtual ByteBuffer calculate(const std::string &src, const ByteBuffer &secret) const override;
    };
}
}
