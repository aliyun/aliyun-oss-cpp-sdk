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
    class SignGenerator
    {
    public:
      SignGenerator(const std::string &version, const std::string &algoType);

      virtual void signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const ServiceRequest &request, const Credentials &credentials, const std::string &resource, const ClientConfiguration &config) const = 0;
      virtual std::string signUrl(const std::string &method, const std::string &resource, const HeaderCollection &headers, const ParameterCollection &parameters, const std::string &secret) const = 0;
      virtual std::string signRTMP(const std::string &expires, const std::string &resource, const ParameterCollection &parameters, const std::string &secret) const = 0;

    protected:
      std::shared_ptr<Signer> signAlgo_;
      std::string version_;
    };
  }
}