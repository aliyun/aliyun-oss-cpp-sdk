#include "SignGeneratorV1.h"

using namespace AlibabaCloud::OSS;

namespace
{
  const char *TAG = "SignGeneratorV1";
}

void SignGeneratorV1::signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const ServiceRequest &request, const Credentials &credentials, const std::string &resource, __attribute__((unused)) const ClientConfiguration &config) const
{
  if (!credentials.SessionToken().empty())
  {
    httpRequest->addHeader("x-oss-security-token", credentials.SessionToken());
  }

  // Sort the parameters
  ParameterCollection parameters;
  for (auto const &param : request.Parameters())
  {
    parameters[param.first] = param.second;
  }

  std::string method = Http::MethodToString(httpRequest->method());

  std::string date = httpRequest->Header(Http::DATE);

  SignUtils signUtils(version_);
  HeaderSet additionalHeaders;
  signUtils.genAdditionalHeader(httpRequest->Headers(), additionalHeaders);
  signUtils.build(method, resource, date, httpRequest->Headers(), parameters, additionalHeaders);

  std::string signature = signAlgo_->generate(signUtils.CanonicalString(), credentials.AccessKeySecret());
  std::stringstream authValue;
  authValue
      << "OSS "
      << credentials.AccessKeyId()
      << ":"
      << signature;

  httpRequest->addHeader(Http::AUTHORIZATION, authValue.str());

  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) CanonicalString:%s", this, httpRequest.get(), signUtils.CanonicalString().c_str());
  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) Authorization:%s", this, httpRequest.get(), authValue.str().c_str());
}

// std::string signUrl(const GeneratePresignedUrlRequest &request) const {
//   ;
// }
std::string SignGeneratorV1::signUrl(const std::string &method, const std::string &resource, const HeaderCollection &headers, const ParameterCollection &parameters, const std::string &secret) const
{
  SignUtils signUtils(version_);
  HeaderSet additionalHeaders;
  signUtils.genAdditionalHeader(headers, additionalHeaders);
  signUtils.build(method, resource, headers.at(Http::EXPIRES), headers, parameters, additionalHeaders);
  return signAlgo_->generate(signUtils.CanonicalString(), secret);
}

std::string SignGeneratorV1::signRTMP(const std::string &expires, const std::string &resource, const ParameterCollection &parameters, const std::string &secret) const
{
  SignUtils signUtils(version_);
  signUtils.build(expires, resource, parameters);
  return signAlgo_->generate(signUtils.CanonicalString(), secret);
}