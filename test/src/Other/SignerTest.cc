/*
 * Copyright 2009-2017 Alibaba Cloud All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <src/signer/Signer.h>
#include "../Config.h"
#include "../Utils.h"

namespace AlibabaCloud {
namespace OSS {

class SignerTest : public ::testing::Test {
protected:
    SignerTest()
    {
    }

    ~SignerTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {

    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
public:

};

TEST_F(SignerTest, SignerV4)
{
    auto signer = Signer::createSigner(SignatureVersionType::V4);

    auto credentialsProvider = std::make_shared<SimpleCredentialsProvider>("ak", "sk", "");

    auto credentials = credentialsProvider->getCredentials();
    auto bucket = "bucket";
    auto key = "1234+-/123/1.txt";
    auto region = "cn-hangzhou";
    auto product = "oss";
    auto t = 1702743657LL;

    HeaderCollection headers;
    ParameterCollection parameters;

    headers["x-oss-head1"] = "value";
    headers["abc"] = "value";
    headers["ZAbc"] = "value";
    headers["XYZ"] = "value";
    headers["XYZ"] = "value";
    headers["content-type"] = "text/plain";
    headers["x-oss-content-sha256"] = "UNSIGNED-PAYLOAD";

    parameters["param1"] = "value1";
    parameters["|param1"] = "value2";
    parameters["+param1"] = "value3";
    parameters["|param1"] = "value4";
    parameters["+param2"] = "";
    parameters["|param2"] = "";
    parameters["param2"] = "";

    auto httpRequest = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest->addHeader(header.first, header.second);
    }

    SignerParam signerParam(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signer->sign(httpRequest, parameters, signerParam);

    std::string authPat = "OSS4-HMAC-SHA256 Credential=ak/20231216/cn-hangzhou/oss/aliyun_v4_request,Signature=e21d18daa82167720f9b1047ae7e7f1ce7cb77a31e8203a7d5f4624fa0284afe";

    EXPECT_EQ(authPat, httpRequest->Header(Http::AUTHORIZATION));
}

TEST_F(SignerTest, SignerV4Token)
{
    auto signer = Signer::createSigner(SignatureVersionType::V4);

    auto credentialsProvider = std::make_shared<SimpleCredentialsProvider>("ak", "sk", "token");

    auto credentials = credentialsProvider->getCredentials();
    auto bucket = "bucket";
    auto key = "1234+-/123/1.txt";
    auto region = "cn-hangzhou";
    auto product = "oss";
    auto t = 1702784856LL;

    HeaderCollection headers;
    ParameterCollection parameters;

    headers["x-oss-head1"] = "value";
    headers["abc"] = "value";
    headers["ZAbc"] = "value";
    headers["XYZ"] = "value";
    headers["XYZ"] = "value";
    headers["content-type"] = "text/plain";
    headers["x-oss-content-sha256"] = "UNSIGNED-PAYLOAD";

    parameters["param1"] = "value1";
    parameters["|param1"] = "value2";
    parameters["+param1"] = "value3";
    parameters["|param1"] = "value4";
    parameters["+param2"] = "";
    parameters["|param2"] = "";
    parameters["param2"] = "";

    auto httpRequest = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest->addHeader(header.first, header.second);
    }

    SignerParam signerParam(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signer->sign(httpRequest, parameters, signerParam);

    std::string authPat = "OSS4-HMAC-SHA256 Credential=ak/20231217/cn-hangzhou/oss/aliyun_v4_request,Signature=b94a3f999cf85bcdc00d332fbd3734ba03e48382c36fa4d5af5df817395bd9ea";
    EXPECT_EQ(authPat, httpRequest->Header(Http::AUTHORIZATION));
    EXPECT_EQ("token", httpRequest->Header("x-oss-security-token"));
}

TEST_F(SignerTest, SignerV4WithAdditionalHeaders)
{
    auto signer = Signer::createSigner(SignatureVersionType::V4);

    auto credentialsProvider = std::make_shared<SimpleCredentialsProvider>("ak", "sk", "");

    auto credentials = credentialsProvider->getCredentials();
    auto bucket = "bucket";
    auto key = "1234+-/123/1.txt";
    auto region = "cn-hangzhou";
    auto product = "oss";
    auto t = 1702747512LL;

    HeaderCollection headers;
    ParameterCollection parameters;
    HeaderSet signHeaders;

    headers["x-oss-head1"] = "value";
    headers["abc"] = "value";
    headers["ZAbc"] = "value";
    headers["XYZ"] = "value";
    headers["XYZ"] = "value";
    headers["content-type"] = "text/plain";
    headers["x-oss-content-sha256"] = "UNSIGNED-PAYLOAD";

    signHeaders.emplace("abc");
    signHeaders.emplace("ZAbc");

    parameters["param1"] = "value1";
    parameters["|param1"] = "value2";
    parameters["+param1"] = "value3";
    parameters["|param1"] = "value4";
    parameters["+param2"] = "";
    parameters["|param2"] = "";
    parameters["param2"] = "";

    // 1
    auto httpRequest = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest->addHeader(header.first, header.second);
    }

    SignerParam signerParam(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signerParam.setAdditionalHeaders(signHeaders);

    signer->sign(httpRequest, parameters, signerParam);

    std::string authPat = "OSS4-HMAC-SHA256 Credential=ak/20231216/cn-hangzhou/oss/aliyun_v4_request,AdditionalHeaders=abc;zabc,Signature=4a4183c187c07c8947db7620deb0a6b38d9fbdd34187b6dbaccb316fa251212f";

    // 2
    HeaderSet signHeaders2;
    signHeaders2.emplace("ZAbc");
    signHeaders2.emplace("abc");
    signHeaders2.emplace("x-oss-head1");
    signHeaders2.emplace("x-oss-no-exist");

    auto httpRequest2  = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest2->addHeader(header.first, header.second);
    }

    SignerParam signerParam2(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signerParam2.setAdditionalHeaders(signHeaders2);

    signer->sign(httpRequest2, parameters, signerParam2);

    EXPECT_EQ(authPat, httpRequest2->Header(Http::AUTHORIZATION));
}

TEST_F(SignerTest, SignerV4Presign)
{
    auto signer = Signer::createSigner(SignatureVersionType::V4);

    auto credentialsProvider = std::make_shared<SimpleCredentialsProvider>("ak", "sk", "");

    auto credentials = credentialsProvider->getCredentials();
    auto bucket = "bucket";
    auto key = "1234+-/123/1.txt";
    auto region = "cn-hangzhou";
    auto product = "oss";
    auto t = 1702781677LL;
    auto expires = 1702782276LL;

    HeaderCollection headers;
    ParameterCollection parameters;
    HeaderSet signHeaders;

    headers["x-oss-head1"] = "value";
    headers["abc"] = "value";
    headers["ZAbc"] = "value";
    headers["XYZ"] = "value";
    headers["XYZ"] = "value";
    headers["content-type"] = "application/octet-stream";

    parameters["param1"] = "value1";
    parameters["|param1"] = "value2";
    parameters["+param1"] = "value3";
    parameters["|param1"] = "value4";
    parameters["+param2"] = "";
    parameters["|param2"] = "";
    parameters["param2"] = "";

    auto httpRequest = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest->addHeader(header.first, header.second);
    }

    SignerParam signerParam(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signerParam.setExpires(expires);
    signer->presign(httpRequest, parameters, signerParam);

    EXPECT_EQ("OSS4-HMAC-SHA256", parameters["x-oss-signature-version"]);
    EXPECT_EQ("20231217T025437Z", parameters["x-oss-date"]);
    EXPECT_EQ("599", parameters["x-oss-expires"]);
    EXPECT_EQ("ak/20231217/cn-hangzhou/oss/aliyun_v4_request", parameters["x-oss-credential"]);
    EXPECT_EQ("a39966c61718be0d5b14e668088b3fa07601033f6518ac7b523100014269c0fe", parameters["x-oss-signature"]);
    EXPECT_EQ(true, parameters.find("x-oss-additional-headers") == parameters.end());
}

TEST_F(SignerTest, SignerV4PresignToken)
{
    auto signer = Signer::createSigner(SignatureVersionType::V4);

    auto credentialsProvider = std::make_shared<SimpleCredentialsProvider>("ak", "sk", "token");

    auto credentials = credentialsProvider->getCredentials();
    auto bucket = "bucket";
    auto key = "1234+-/123/1.txt";
    auto region = "cn-hangzhou";
    auto product = "oss";
    auto t = 1702785388LL;
    auto expires = 1702785987LL;

    HeaderCollection headers;
    ParameterCollection parameters;
    HeaderSet signHeaders;

    headers["x-oss-head1"] = "value";
    headers["abc"] = "value";
    headers["ZAbc"] = "value";
    headers["XYZ"] = "value";
    headers["XYZ"] = "value";
    headers["content-type"] = "application/octet-stream";

    parameters["param1"] = "value1";
    parameters["|param1"] = "value2";
    parameters["+param1"] = "value3";
    parameters["|param1"] = "value4";
    parameters["+param2"] = "";
    parameters["|param2"] = "";
    parameters["param2"] = "";

    auto httpRequest = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest->addHeader(header.first, header.second);
    }

    SignerParam signerParam(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signerParam.setExpires(expires);
    signer->presign(httpRequest, parameters, signerParam);

    EXPECT_EQ("OSS4-HMAC-SHA256", parameters["x-oss-signature-version"]);
    EXPECT_EQ("20231217T035628Z", parameters["x-oss-date"]);
    EXPECT_EQ("599", parameters["x-oss-expires"]);
    EXPECT_EQ("token", parameters["x-oss-security-token"]);
    EXPECT_EQ("ak/20231217/cn-hangzhou/oss/aliyun_v4_request", parameters["x-oss-credential"]);
    EXPECT_EQ("ak/20231217/cn-hangzhou/oss/aliyun_v4_request", parameters["x-oss-credential"]);
    EXPECT_EQ("3817ac9d206cd6dfc90f1c09c00be45005602e55898f26f5ddb06d7892e1f8b5", parameters["x-oss-signature"]);
    EXPECT_EQ(true, parameters.find("x-oss-additional-headers") == parameters.end());
}

TEST_F(SignerTest, SignerV4PresignWithAdditionalHeaders)
{
    auto signer = Signer::createSigner(SignatureVersionType::V4);

    auto credentialsProvider = std::make_shared<SimpleCredentialsProvider>("ak", "sk", "");

    auto credentials = credentialsProvider->getCredentials();
    auto bucket = "bucket";
    auto key = "1234+-/123/1.txt";
    auto region = "cn-hangzhou";
    auto product = "oss";
    auto t = 1702783809LL;
    auto expires = 1702784408LL;

    HeaderCollection headers;
    ParameterCollection parameters;
    HeaderSet signHeaders;

    headers["x-oss-head1"] = "value";
    headers["abc"] = "value";
    headers["ZAbc"] = "value";
    headers["XYZ"] = "value";
    headers["XYZ"] = "value";
    headers["content-type"] = "application/octet-stream";

    parameters["param1"] = "value1";
    parameters["|param1"] = "value2";
    parameters["+param1"] = "value3";
    parameters["|param1"] = "value4";
    parameters["+param2"] = "";
    parameters["|param2"] = "";
    parameters["param2"] = "";

    //1
    signHeaders.emplace("abc");
    signHeaders.emplace("ZAbc");

    auto httpRequest = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest->addHeader(header.first, header.second);
    }

    SignerParam signerParam(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signerParam.setExpires(expires);
    signerParam.setAdditionalHeaders(signHeaders);
    signer->presign(httpRequest, parameters, signerParam);

    EXPECT_EQ("OSS4-HMAC-SHA256", parameters["x-oss-signature-version"]);
    EXPECT_EQ("20231217T033009Z", parameters["x-oss-date"]);
    EXPECT_EQ("599", parameters["x-oss-expires"]);
    EXPECT_EQ("ak/20231217/cn-hangzhou/oss/aliyun_v4_request", parameters["x-oss-credential"]);
    EXPECT_EQ("6bd984bfe531afb6db1f7550983a741b103a8c58e5e14f83ea474c2322dfa2b7", parameters["x-oss-signature"]);
    EXPECT_EQ("abc;zabc", parameters["x-oss-additional-headers"]);

    //2
    ParameterCollection parameters2;
    parameters2["param1"] = "value1";
    parameters2["|param1"] = "value2";
    parameters2["+param1"] = "value3";
    parameters2["|param1"] = "value4";
    parameters2["+param2"] = "";
    parameters2["|param2"] = "";
    parameters2["param2"] = "";

    auto httpRequest2 = std::make_shared<HttpRequest>(Http::Method::Put);
    for (auto const& header : headers) {
        httpRequest2->addHeader(header.first, header.second);
    }
    HeaderSet signHeaders2; 
    signHeaders2.emplace("abc");
    signHeaders2.emplace("ZAbc");
    signHeaders2.emplace("x-oss-head1");

    SignerParam signerParam2(std::move(region), std::move(product), 
        std::move(bucket), std::move(key), std::move(credentials), t);
    signerParam2.setExpires(expires);
    signerParam2.setAdditionalHeaders(signHeaders);
    EXPECT_EQ(true, parameters2.find("x-oss-additional-headers") == parameters2.end());

    signer->presign(httpRequest2, parameters2, signerParam2);

    EXPECT_EQ("OSS4-HMAC-SHA256", parameters2["x-oss-signature-version"]);
    EXPECT_EQ("20231217T033009Z", parameters2["x-oss-date"]);
    EXPECT_EQ("599", parameters2["x-oss-expires"]);
    EXPECT_EQ("ak/20231217/cn-hangzhou/oss/aliyun_v4_request", parameters2["x-oss-credential"]);
    EXPECT_EQ("6bd984bfe531afb6db1f7550983a741b103a8c58e5e14f83ea474c2322dfa2b7", parameters2["x-oss-signature"]);
    EXPECT_EQ("abc;zabc", parameters2["x-oss-additional-headers"]);
}

}
}
