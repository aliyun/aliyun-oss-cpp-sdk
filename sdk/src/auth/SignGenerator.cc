#include "SignGenerator.h"

using namespace AlibabaCloud::OSS;

SignGenerator::SignGenerator(const std::string& version, const std::string& algoType) : version_(version) {
  // First decided by algorithm, if not 
  // then use version's default algorithm
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