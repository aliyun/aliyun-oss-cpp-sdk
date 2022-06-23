#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "Signer.h"
#include "HmacSha1Signer.h"
#include "HmacSha256Signer.h"
// #include <alibabacloud/oss/OssRequest.h>
#include "../utils/SignUtils.h"
#include "../utils/LogUtils.h"

namespace AlibabaCloud
{
  namespace OSS
  {
    struct SignParam
    {
      // expireStr, resource, parameters, credentials
      SignParam(const std::string &expire, const std::string &resource, const ParameterCollection &params, const Credentials &credentials)
      : resource_(resource), credentials_(credentials), params_(params), expires_(expire) {}
      // method, resource, headers, parameters, credentials
      SignParam(const std::string &method, const std::string &resource, const HeaderCollection &headers, const ParameterCollection &params, const Credentials &credentials)
          : resource_(resource), credentials_(credentials), params_(params), method_(method), headers_(headers), expires_("") {}

      // request, credentialsProvider_->getCredentials(), resource, configuration()
      SignParam(const ClientConfiguration &config, const std::string &resource, const ParameterCollection &params, const Credentials &credentials)
          : config_(config), resource_(resource), credentials_(credentials), params_(params) {}

      ClientConfiguration config_;
      std::string resource_;
      Credentials credentials_;
      ParameterCollection params_;
      std::string method_;
      HeaderCollection headers_;
      std::string expires_;
    };

    class SignGenerator
    {
    public:
      SignGenerator(const std::string &version, const std::string &algoType);

      virtual void signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const SignParam &signParam) const = 0;
      virtual std::string presign(const SignParam &signParam) const = 0;
      virtual std::string signRTMP(const SignParam &signParam) const = 0;

    protected:
      std::shared_ptr<Signer> signAlgo_;
      std::string version_;
    };
  }
}