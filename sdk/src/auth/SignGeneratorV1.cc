#include "SignGeneratorV1.h"

using namespace AlibabaCloud::OSS;

namespace
{
  const char *TAG = "SignGeneratorV1";
}

void SignGeneratorV1::signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const SignParam &signParam) const
{
  if (!signParam.credentials_.SessionToken().empty())
  {
    httpRequest->addHeader("x-oss-security-token", signParam.credentials_.SessionToken());
  }

  // Sort the parameters
  ParameterCollection parameters;
  for (auto const &param : signParam.params_)
  {
    parameters[param.first] = param.second;
  }

  std::string method = Http::MethodToString(httpRequest->method());

  std::string date = httpRequest->Header(Http::DATE);

  SignUtils signUtils(version_);
  signUtils.build(method, signParam.resource_, date, httpRequest->Headers(), parameters, signParam.additionalHeaders_);
  
  byteArray signature = signAlgo_->generate(byteArray{signUtils.CanonicalString()}, signParam.credentials_.AccessKeySecret());
  std::stringstream authValue;
  authValue
      << "OSS "
      << signParam.credentials_.AccessKeyId()
      << ":"
      << signature.str_;
  httpRequest->addHeader(Http::AUTHORIZATION, authValue.str());

  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) CanonicalString:%s", this, httpRequest.get(), signUtils.CanonicalString().c_str());
  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) Authorization:%s", this, httpRequest.get(), authValue.str().c_str());
}

std::string SignGeneratorV1::presign(const SignParam &signParam) const
{
  SignUtils signUtils(version_);
  signUtils.build(signParam.method_, signParam.resource_, signParam.headers_.at(Http::EXPIRES), signParam.headers_, signParam.params_, signParam.additionalHeaders_);
  return signAlgo_->generate(byteArray{signUtils.CanonicalString()}, signParam.credentials_.AccessKeySecret());
}

std::string SignGeneratorV1::signRTMP(const SignParam &signParam) const
{
  SignUtils signUtils(version_);
  signUtils.build(signParam.expires_, signParam.resource_, signParam.params_);
  return signAlgo_->generate(byteArray{signUtils.CanonicalString()}, signParam.credentials_.AccessKeySecret());
}