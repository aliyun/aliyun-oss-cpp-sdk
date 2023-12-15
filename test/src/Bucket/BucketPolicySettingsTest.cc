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
#include "../Utils.h"

namespace AlibabaCloud {
namespace OSS {

class BucketPolicySettingsTest : public ::testing::Test {
protected:
	BucketPolicySettingsTest()
    {
    }

    ~BucketPolicySettingsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketpolicysettings");
        Client->CreateBucket(CreateBucketRequest(BucketName));
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase() 
    {
        Client->DeleteBucket(DeleteBucketRequest(BucketName));
        Client = nullptr;
    }

    // Sets up the test fixture.
    void SetUp() override
    {
    }

    // Tears down the test fixture.
    void TearDown() override
    {
    }
public:
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;
};
std::shared_ptr<OssClient> BucketPolicySettingsTest::Client = nullptr;
std::string BucketPolicySettingsTest::BucketName = "";

TEST_F(BucketPolicySettingsTest, InvalidBucketNameTest)
{
    std::string policy = "invalidpolicy";
    for (auto const& invalidBucketName : TestUtils::InvalidBucketNamesList()) {
        SetBucketPolicyRequest  request(invalidBucketName);
        request.setPolicy(policy);
        auto outcome = Client->SetBucketPolicy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketPolicySettingsTest, SetBucketPolicyInvalidInputTest)
{
    SetBucketPolicyRequest request(BucketName);
    request.setPolicy("policy");
    auto outcome = Client->SetBucketPolicy(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidPolicyDocument");
    EXPECT_EQ(outcome.error().Message(), "unknown char p");
}

TEST_F(BucketPolicySettingsTest, SetBucketPolicyTest)
{
    SetBucketPolicyRequest request(BucketName);
    std::string policy = "{\"Version\":\"1\",\"Statement\":[{\"Action\":[\"oss:PutObject\",\"oss:GetObject\"],\"Resource\": \"acs:oss:*:*:*\",\"Effect\": \"Deny\"}]}\n";
    request.setPolicy(policy);
    auto outcome = Client->SetBucketPolicy(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    GetBucketPolicyRequest request1(BucketName);
    auto outcome1 = Client->GetBucketPolicy(request1);
    EXPECT_EQ(outcome1.isSuccess(), true);
    EXPECT_EQ(outcome1.result().Policy(), policy);
    DeleteBucketPolicyRequest request2(BucketName);
    auto outcome2 = Client->DeleteBucketPolicy(request2);
    EXPECT_EQ(outcome2.isSuccess(), true);
}

TEST_F(BucketPolicySettingsTest, GetBucketPolicyResult)
{
    std::string policy = "policy";
    GetBucketPolicyResult result(policy);
    EXPECT_EQ(result.Policy(), "policy");
}

TEST_F(BucketPolicySettingsTest, DeleteBucketPolicyInvalidValidateTest)
{
    auto deloutcome = Client->DeleteBucketPolicy(DeleteBucketPolicyRequest("Invalid-bucket-test"));

    EXPECT_EQ(deloutcome.isSuccess(), false);
    EXPECT_EQ(deloutcome.error().Code(), "ValidateError");
}

TEST_F(BucketPolicySettingsTest, GetBucketPolicyInvalidValidateTest)
{
    auto getoutcome = Client->GetBucketPolicy(GetBucketPolicyRequest("Invalid-bucket-test"));

    EXPECT_EQ(getoutcome.isSuccess(), false);
    EXPECT_EQ(getoutcome.error().Code(), "ValidateError");
}

}
}