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
#include <ctime>
#include <alibabacloud/oss/Types.h>
#include <alibabacloud/oss/auth/Credentials.h>
#include <alibabacloud/oss/http/HttpRequest.h>

namespace AlibabaCloud
{
namespace OSS
{
    class SignerParam
    {
    public:
        SignerParam(std::string&& region, std::string&& product, 
            std::string&& bucket, std::string&& key, 
            Credentials&& credentials, std::time_t requestTime):
                region_(region),
                product_(product),
                bucket_(bucket),
                key_(key),
                credentials_(credentials),
                requestTime_(requestTime)
        {}

    const std::string& Region() const { return region_; }
    const std::string& Product() const { return product_; }
    const std::string& Bucket() const { return bucket_; }
    const std::string& Key() const { return key_; }
    const Credentials& Cred() const { return credentials_; }
    std::time_t RequestTime() const { return requestTime_; }
    const HeaderSet& AdditionalHeaders() const { return additionalHeaders_; }
    void setAdditionalHeaders(const HeaderSet& headers) { additionalHeaders_ = headers; }

    int64_t Expires() const { return expires_; }
    void setExpires(int64_t expires) { expires_ = expires; }

    private:
        SignerParam() = delete;

    private:
        std::string region_;
        std::string product_;
        std::string bucket_;
        std::string key_;
        Credentials credentials_;
        std::time_t requestTime_;
        HeaderSet additionalHeaders_;
        std::time_t expires_;
    };

    class  Signer
    {
    public:

        enum Type
        {
            HmacSha1,
            HmacSha256,
        };
        virtual ~Signer();

        virtual void sign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameter, 
            SignerParam &signerParam)const = 0;
        virtual void presign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameter,
            SignerParam &signerParam)const = 0;

        virtual std::string generate(const std::string &src, const std::string &secret)const = 0;


        virtual std::string name()const;
        virtual Type type() const;
        virtual std::string version()const;

    public:
         static std::shared_ptr<Signer> createSigner(SignatureVersionType version);
    protected:
        Signer(Type type, const std::string &name, const std::string &version = "1.0");
    private:
        Type type_;
        std::string name_;
        std::string version_;
    };

    class  SignerV1 : public Signer
    {
    public:
        SignerV1();
        ~SignerV1();
        
        virtual void sign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameter,
            SignerParam &signerParam)const override;
        virtual void presign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameter,
            SignerParam &signerParam)const override;
        virtual std::string generate(const std::string &src, const std::string &secret)const override;
    };

    class  SignerV4 : public Signer
    {
    public:
        SignerV4();
        ~SignerV4();
        
        virtual void sign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameter,
            SignerParam &signerParam)const override;
        virtual void presign(const std::shared_ptr<HttpRequest> &httpRequest, ParameterCollection &parameter,
            SignerParam &signerParam)const override;
        virtual std::string generate(const std::string &src, const std::string &secret)const override;
    };
}
}
