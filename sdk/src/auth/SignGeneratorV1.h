#pragma once

#include "SignGenerator.h"
#include <alibabacloud/oss/OssRequest.h>

namespace AlibabaCloud
{
  namespace OSS
  {
    class SignGeneratorV1 : public SignGenerator
    {
    public:
      SignGeneratorV1(const std::string &algo) : SignGenerator("1.0", algo) {}

      virtual void signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const ServiceRequest &request, const Credentials &credentials, const std::string &resource, const ClientConfiguration &config) const override;
      virtual std::string signUrl(const std::string &method, const std::string &resource, const HeaderCollection &headers, const ParameterCollection &parameters, const std::string &secret) const override;
      virtual std::string signRTMP(const std::string &expires, const std::string &resource, const ParameterCollection &parameters, const std::string &secret) const override;
    };
  }
}