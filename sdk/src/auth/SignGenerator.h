#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "Signer.h"
#include "HmacSha1Signer.h"
#include "HmacSha256Signer.h"
#include "../utils/SignUtils.h"
#include "../utils/LogUtils.h"

namespace AlibabaCloud
{
    namespace OSS
    {
        std::string ALIBABACLOUD_OSS_EXPORT UtcV4ToDay(const std::string &t);
        struct SignParam
        {
            // expireStr, resource, parameters, credentials
            SignParam(const std::string &expire, const std::string &resource, const ParameterCollection &params, const Credentials &credentials)
                : resource_(resource), credentials_(credentials), params_(params), expires_(expire) {}
            // method, resource, headers, parameters, credentials
            SignParam(const std::string &method, const std::string &resource, const HeaderCollection &headers, const ParameterCollection &params, const Credentials &credentials)
                : resource_(resource), credentials_(credentials), params_(params), method_(method), headers_(headers) {}

            // request, credentialsProvider_->getCredentials(), resource, configuration()
            SignParam(const ClientConfiguration &config, const std::string &resource, const ParameterCollection &params, const Credentials &credentials)
                : config_(config), resource_(resource), credentials_(credentials), params_(params) {}

            inline void setCloudBoxId(const std::string &cloudBoxId)
            {
                cloudBoxId_ = cloudBoxId;
            }

            inline void setRegion(const std::string &region)
            {
                region_ = region;
            }

            inline void setAdditionalHeaders(const HeaderSet &additionalHeaders)
            {
                additionalHeaders_ = additionalHeaders;
            }

            ClientConfiguration config_;
            std::string resource_;
            Credentials credentials_;
            ParameterCollection params_;
            std::string method_;
            HeaderCollection headers_;
            std::string expires_;
            std::string cloudBoxId_;
            std::string region_;
            HeaderSet additionalHeaders_;
        };

        class SignGenerator
        {
        public:
            SignGenerator(const std::string &version, const std::string &algoType);

            virtual void signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const SignParam &signParam) const = 0;
            virtual std::string presign(const SignParam &signParam) const = 0;
            virtual std::string signRTMP(const SignParam &signParam) const = 0;

            inline std::string version() {
                return version_;
            }

        protected:
            std::shared_ptr<Signer> signAlgo_;
            std::string version_;
        };
    }
}