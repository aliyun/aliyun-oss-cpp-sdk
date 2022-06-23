#include "SignGenerator.h"

using namespace AlibabaCloud::OSS;

SignGenerator::SignGenerator(const std::string& version, const std::string& algoType) : version_(version) {
  // 先根据算法判断，如果没有指定算法则按照版本默认算法
  if (algoType == "HMAC-SHA1") {
    signAlgo_ = std::make_shared<HmacSha1Signer>();
  } else if (algoType == "HMAC-SHA256") {
    signAlgo_ = std::make_shared<HmacSha256Signer>();
  } else if (version == "4.0") {
    signAlgo_ = std::make_shared<HmacSha256Signer>();
  } else {
    signAlgo_ = std::make_shared<HmacSha1Signer>();
  }
}