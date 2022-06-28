#include "SignGeneratorV4.h"
#include <openssl/sha.h>
#include <time.h>
#include <openssl/hmac.h>
#ifdef OPENSSL_IS_BORINGSSL
#include <openssl/base64.h>
#endif

using namespace AlibabaCloud::OSS;

namespace
{
  const char *TAG = "SignGeneratorV4";
}

void SignGeneratorV4::sha256Hex(const std::string &src, char output[65]) const
{
  byte hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, src.c_str(), src.size());
  SHA256_Final(hash, &sha256);
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
  {
    sprintf(output + (i * 2), "%02x", hash[i]);
  }
  output[64] = '\0';
}

std::string SignGeneratorV4::getCurrentGmtDate() const
{
  char buf[10] = {0};
  time_t t = time(NULL);
  tm rs;
  gmtime_r(&t, &rs);
  strftime(buf, sizeof(buf), "%Y%m%d", &rs);
  return buf;
}

void SignGeneratorV4::signHeader(const std::shared_ptr<HttpRequest> &httpRequest, const SignParam &signParam) const
{
  if (!signParam.credentials_.SessionToken().empty())
  {
    httpRequest->addHeader("x-oss-security-token", signParam.credentials_.SessionToken());
  }
  httpRequest->addHeader("x-oss-content-sha256", "UNSIGNED-PAYLOAD");

  // Sort the parameters
  ParameterCollection parameters;
  for (auto const &param : signParam.params_)
  {
    parameters[param.first] = param.second;
  }

  std::string method = Http::MethodToString(httpRequest->method());

  std::string date = httpRequest->Header("x-oss-date");

  std::stringstream scope;
  // convert to "20060102" time format
  std::string day = UtcV4ToDay(date);
  scope << day;

  std::string region;
  std::string product;

  if (signParam.cloudBoxId_.empty())
  {
    region = signParam.region_;
    product = "oss";
  }
  else
  {
    region = signParam.cloudBoxId_;
    product = "oss-cloundbox";
  }
  scope << "/" << region
        << "/" << product
        << "/aliyun_v4_request";

  // Canonical Reuqest
  SignUtils signUtils(version_);
  signUtils.build(method, signParam.resource_, date, httpRequest->Headers(), parameters, signParam.additionalHeaders_);
  std::string canonical = signUtils.CanonicalString();

  // Hex(SHA256Hash(Canonical Reuqest))
  char hashedCalRequest[65];
  sha256Hex(canonical, hashedCalRequest);

  // string to sign
  // "OSS4-HMAC-SHA256" + "\n" +
  // TimeStamp + "\n" +
  // Scope + "\n" +
  // Hex(SHA256Hash(Canonical Reuqest))
  std::stringstream stringToSign;
  stringToSign << "OSS4-" << signAlgo_->name() << "\n"
               << date << "\n"
               << scope.str() << "\n"
               << hashedCalRequest;

  // HMACSHA256(HMACSHA256(HMACSHA256(HMACSHA256("aliyun_v4"+SK,Date),Region),oss),"aliyun_v4_request");
  byteArray signingKey = signAlgo_->generate(signAlgo_->generate(signAlgo_->generate(signAlgo_->generate(byteArray{"aliyun_v4" + signParam.credentials_.AccessKeySecret()}, day), region), product), "aliyun_v4_request");
  // HEX(HMAC-SHA256(SigningKey,StringToSign))
  byteArray signSrc = signAlgo_->generate(signingKey, stringToSign.str());
  char signature[65];
  sha256Hex(signSrc, signature);

  std::stringstream authValue;
  authValue
      << "OSS4-HMAC-SHA256 Credential=" << signParam.credentials_.AccessKeyId() << "/" << scope.str();

  if (!signParam.additionalHeaders_.empty())
  {
    authValue << ",AdditionalHeaders=";
    bool isFirstHeader = true;
    for (const auto &addHeader : signParam.additionalHeaders_)
    {
      if (!isFirstHeader)
      {
        authValue << ";";
      }
      else
      {
        isFirstHeader = false;
      }
      authValue << addHeader.c_str();
      // if (addHeader == "host") {
      //   httpRequest->addHeader("")
      // }
    }
  }

  authValue << ",Signature=" << signature;

  httpRequest->addHeader(Http::AUTHORIZATION, authValue.str());

  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) CanonicalString:%s", this, httpRequest.get(), signUtils.CanonicalString().c_str());
  OSS_LOG(LogLevel::LogDebug, TAG, "client(%p) request(%p) Authorization:%s", this, httpRequest.get(), authValue.str().c_str());
}

std::string SignGeneratorV4::presign(const SignParam &signParam) const
{
  SignUtils signUtils(version_);
  signUtils.build(signParam.method_, signParam.resource_, signParam.headers_.at(Http::EXPIRES), signParam.headers_, signParam.params_, signParam.additionalHeaders_);
  return signAlgo_->generate(byteArray{signUtils.CanonicalString()}, signParam.credentials_.AccessKeySecret());
}

std::string SignGeneratorV4::signRTMP(const SignParam &signParam) const
{
  SignUtils signUtils(version_);
  signUtils.build(signParam.expires_, signParam.resource_, signParam.params_);
  return signAlgo_->generate(byteArray{signUtils.CanonicalString()}, signParam.credentials_.AccessKeySecret());
}