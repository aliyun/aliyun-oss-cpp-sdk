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

}
}