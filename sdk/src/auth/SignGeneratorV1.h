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

      virtual void signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const SignParam &signParam) const override;
      virtual std::string presign(const SignParam &signParam) const override;
      virtual std::string signRTMP(const SignParam &signParam) const override;
    };
  }
}