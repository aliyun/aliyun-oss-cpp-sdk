#pragma once

#include "SignGenerator.h"
#include <alibabacloud/oss/OssRequest.h>

namespace AlibabaCloud
{
  namespace OSS
  {
    class SignGeneratorV4 : public SignGenerator
    {
    public:
      SignGeneratorV4(const std::string &algo) : SignGenerator("4.0", algo) {}

      virtual void signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const SignParam &signParam) const override;
      virtual std::string presign(const SignParam &signParam) const override;
      virtual std::string signRTMP(const SignParam &signParam) const override;
    };
  }
}