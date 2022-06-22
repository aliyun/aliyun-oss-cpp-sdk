#include "SignGeneratorV4.h"

using namespace AlibabaCloud::OSS;

namespace
{
  const char *TAG = "SignGeneratorV4";
}

void SignGeneratorV4::signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const ServiceRequest &request, const Credentials &credentials, const std::string &resource, const ClientConfiguration &config) const
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

  std::stringstream credential;
  // 生成"20060102"格式的时间
  int y, M, d, h, m;
  float s;
  sscanf(date.c_str(), "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s);
  credential << y << M << d;
  std::string day = credential.str();

  std::string region;
  std::string product;

  if (config.cloudBoxId.empty())
  {
    region = config.region;
    product = "oss";
  }
  else
  {
    region = config.cloudBoxId;
    product = "oss-cloundbox";
  }
  credential << "/" << region
             << "/" << product
             << "/aliyun_v4_request";

  std::string hashedCalRequest = signAlgo_->generate(credentials.AccessKeyId(), credentials.AccessKeySecret());

  // string to sign
  // "OSS4-HMAC-SHA256" + "\n" +
  // TimeStamp + "\n" +
  // Scope + "\n" +
  // Hex(SHA256Hash(Canonical Reuqest))
  std::stringstream stringToSign;
  stringToSign << "OSS4-HMAC-SHA256"
               << "\n"
               << date << "\n"
               << credential << "\n"
               << hashedCalRequest;

  // HMACSHA256(HMACSHA256(HMACSHA256(HMACSHA256("aliyun_v4"+SK,Date),Region),oss),"aliyun_v4_request");
  std::string signature = signAlgo_->generate(signAlgo_->generate(signAlgo_->generate(signAlgo_->generate("aliyun_v4" + credentials.AccessKeySecret(), day), region), product), "aliyun_v4_request");

  SignUtils signUtils(version_);
  HeaderSet additionalHeaders;
  signUtils.genAdditionalHeader(httpRequest->Headers(), additionalHeaders);
  signUtils.build(method, resource, date, httpRequest->Headers(), parameters, additionalHeaders);

  std::stringstream authValue;
  authValue
      << "OSS4-HMAC-SHA256 Credential=" << credentials.AccessKeyId() << "/" << credential.str();

  if (!additionalHeaders.empty())
  {
    authValue << ",AdditionalHeaders=";
    bool isFirstHeader = true;
    for (const auto &addHeader : additionalHeaders)
    {
      if (isFirstHeader)
      {
        authValue << ";";
      }
      else
      {
        isFirstHeader = false;
      }
      authValue << addHeader.c_str();
    }
  }

  authValue << ",Signature=" << signature;

  httpRequest->addHeader(Http::AUTHORIZATION, authValue.str());

  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) CanonicalString:%s", this, httpRequest.get(), signUtils.CanonicalString().c_str());
  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) Authorization:%s", this, httpRequest.get(), authValue.str().c_str());
}

std::string SignGeneratorV4::signUrl(const std::string &method, const std::string &resource, const HeaderCollection &headers, const ParameterCollection &parameters, const std::string &secret) const
{
  SignUtils signUtils(version_);
  HeaderSet addtionalHeaders;
  signUtils.genAdditionalHeader(headers, addtionalHeaders);
  signUtils.build(method, resource, headers.at(Http::EXPIRES), headers, parameters, addtionalHeaders);
  return signAlgo_->generate(signUtils.CanonicalString(), secret);
}

std::string SignGeneratorV4::signRTMP(const std::string &expires, const std::string &resource, const ParameterCollection &parameters, const std::string &secret) const
{
  SignUtils signUtils(version_);
  signUtils.build(expires, resource, parameters);
  return signAlgo_->generate(signUtils.CanonicalString(), secret);
}