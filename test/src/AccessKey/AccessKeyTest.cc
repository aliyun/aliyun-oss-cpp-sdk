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
#include <alibabacloud/oss/OssClient.h>
#include "../Config.h"

namespace AlibabaCloud {
namespace OSS {

class AccessKeyTest : public ::testing::Test {
protected:
    AccessKeyTest()
    {
    }

    ~AccessKeyTest() override
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(AccessKeyTest, InvalidAccessKeyIdTest)
{
    ClientConfiguration conf;
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, "invalidAccessKeyId", Config::AccessKeySecret, conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "InvalidAccessKeyId");
}

TEST_F(AccessKeyTest, InvalidAccessKeySecretTest)
{
    ClientConfiguration conf;
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, 
        Config::AccessKeyId, "invalidAccessKeySecret", 
        conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "SignatureDoesNotMatch");
}

TEST_F(AccessKeyTest, InvalidSecurityTokenTest)
{
    ClientConfiguration conf;
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint,
        Config::AccessKeyId, Config::AccessKeySecret, "InvalidSecurityToken",
        conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "InvalidAccessKeyId");
}

TEST_F(AccessKeyTest, ValidAccessKeyTest)
{
    ClientConfiguration conf;
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint,
        Config::AccessKeyId, Config::AccessKeySecret,
        conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), true);
}


class MyCredentialsProvider : public CredentialsProvider
{
public:
    MyCredentialsProvider(const Credentials &credentials):
        CredentialsProvider(),
        credentials_(credentials)
    {}

    MyCredentialsProvider(const std::string &accessKeyId,
        const std::string &accessKeySecret, const std::string &securityToken = ""):
        CredentialsProvider(),
        credentials_(accessKeyId, accessKeySecret, securityToken)
    {}
    ~MyCredentialsProvider() {};

    virtual Credentials getCredentials() override { return credentials_; }
private:
    Credentials credentials_;
};

TEST_F(AccessKeyTest, InValidCredentialsProviderTest)
{
    ClientConfiguration conf;
    auto credentialsProvider = std::make_shared<MyCredentialsProvider>("", "", "");
    credentialsProvider->getCredentials().setAccessKeyId(Config::AccessKeyId);
    credentialsProvider->getCredentials().setAccessKeySecret(Config::AccessKeySecret);
    credentialsProvider->getCredentials().setSessionToken("haha");
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, credentialsProvider, conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(AccessKeyTest, ValidCredentialsProviderTest)
{
    ClientConfiguration conf;
    auto credentialsProvider = std::make_shared<MyCredentialsProvider>(Config::AccessKeyId, Config::AccessKeySecret, "");
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, credentialsProvider, conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(AccessKeyTest, InValidCredentialsTest)
{
    ClientConfiguration conf;
    auto credentialsProvider = std::make_shared<MyCredentialsProvider>("", "", "");
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, credentialsProvider->getCredentials(), conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(AccessKeyTest, ValidCredentialsTest)
{
    ClientConfiguration conf;
    auto credentialsProvider = std::make_shared<MyCredentialsProvider>(Config::AccessKeyId, Config::AccessKeySecret, "");
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, credentialsProvider->getCredentials(), conf);
    auto outcome = client->ListBuckets(ListBucketsRequest());
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(AccessKeyTest, GeneratePresignedUrlRequestCredentialsProviderTest)
{
    ClientConfiguration conf;
    auto credentialsProvider = std::make_shared<MyCredentialsProvider>(Config::AccessKeyId, Config::AccessKeySecret, "haha");
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, credentialsProvider, conf);
    auto outcome = client->GeneratePresignedUrl(GeneratePresignedUrlRequest("bucket","key"));
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(AccessKeyTest, GenerateRTMPSignatureUrlCredentialsProviderTest)
{
    ClientConfiguration conf;
    auto credentialsProvider = std::make_shared<MyCredentialsProvider>(Config::AccessKeyId, Config::AccessKeySecret, "haha");
    std::shared_ptr<OssClient> client = std::make_shared<OssClient>(Config::Endpoint, credentialsProvider, conf);
    std::string channelName = "channel";
    GenerateRTMPSignedUrlRequest request("bucket", channelName, "", 0);
    request.setPlayList("test.m3u8");

    time_t tExpire = time(nullptr) + 15 * 60;
    request.setExpires(tExpire);
    auto generateOutcome = client->GenerateRTMPSignedUrl(request);

    EXPECT_EQ(generateOutcome.isSuccess(), true);

    auto outcome = generateOutcome;
    outcome = outcome;
    EXPECT_EQ(outcome.isSuccess(), generateOutcome.isSuccess());
    EXPECT_EQ(outcome.result(), generateOutcome.result());

#ifndef __clang__
    outcome = std::move(outcome);
    EXPECT_EQ(outcome.isSuccess(), generateOutcome.isSuccess());
    EXPECT_EQ(outcome.result(), generateOutcome.result());
#endif
}

TEST_F(AccessKeyTest, EndpointTest)
{
    ClientConfiguration conf;
    auto request = ListBucketsRequest();
    request.setMaxKeys(1);

    auto endpoint = Config::Endpoint;
    auto client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto outcome = client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    endpoint = "http://oss-cn-hangzhou.aliyuncs.com";
    client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    outcome = client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    endpoint = "http://oss-cn-hangzhou.aliyuncs.com:80";
    client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    outcome = client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    endpoint = "http://oss-cn-hangzhou.aliyuncs.com:80/?test=123";
    client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    outcome = client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    endpoint = "www.test-inc.com\\oss-cn-hangzhou.aliyuncs.com";
    client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    outcome = client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "The endpoint is invalid.");

    endpoint = "www.test-inc*test.com";
    client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    outcome = client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "The endpoint is invalid.");

    endpoint = "";
    client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    outcome = client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "The endpoint is invalid.");
}

}
}