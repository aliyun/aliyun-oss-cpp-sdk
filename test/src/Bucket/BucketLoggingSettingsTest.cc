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

class BucketLoggingSettingsTest : public ::testing::Test {
protected:
    BucketLoggingSettingsTest()
    {
    }

    ~BucketLoggingSettingsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketloggingsettings");
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

std::shared_ptr<OssClient> BucketLoggingSettingsTest::Client = nullptr;
std::string BucketLoggingSettingsTest::BucketName = "";


TEST_F(BucketLoggingSettingsTest, InvalidBucketNameTest)
{
    for (auto const& invalidBucketName : TestUtils::InvalidBucketNamesList()) {
        auto outcome = Client->SetBucketLogging(invalidBucketName, "bucket", "LogPrefix");
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketLoggingSettingsTest, GetBucketNotSetLoggingTest)
{
    auto dOutcome = Client->DeleteBucketLogging(DeleteBucketLoggingRequest(BucketName));
    EXPECT_EQ(dOutcome.isSuccess(), true);

    auto gOutcome = Client->GetBucketLogging(GetBucketLoggingRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().TargetBucket().empty(), true);
    EXPECT_EQ(gOutcome.result().TargetPrefix().empty(), true);
}

TEST_F(BucketLoggingSettingsTest, GetBucketLoggingNegativeTest)
{
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-bucketloggingsettings");
    auto outcome = Client->GetBucketLogging(bucketName);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketLoggingSettingsTest, DeleteBucketLoggingNegativeTest)
{
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-bucketloggingsettings");
    auto outcome = Client->DeleteBucketLogging(bucketName);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}


TEST_F(BucketLoggingSettingsTest, EnableLoggingTest)
{
    auto sOutcome = Client->SetBucketLogging(SetBucketLoggingRequest(BucketName, BucketName, "LoggingTest"));
    EXPECT_EQ(sOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    auto gOutcome = Client->GetBucketLogging(GetBucketLoggingRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().TargetPrefix(), "LoggingTest");

    auto dOutcome = Client->DeleteBucketLogging(DeleteBucketLoggingRequest(BucketName));
    EXPECT_EQ(dOutcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    gOutcome = Client->GetBucketLogging(GetBucketLoggingRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().TargetBucket().empty(), true);
    EXPECT_EQ(gOutcome.result().TargetPrefix().empty(), true);

}

TEST_F(BucketLoggingSettingsTest, EnableLoggingInvalidTargetBucketNameTest)
{
    for (auto invalidBucketName : TestUtils::InvalidBucketNamesList())
    {
        auto outcome = Client->SetBucketLogging(SetBucketLoggingRequest(BucketName, invalidBucketName, "LogPrefix"));
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketLoggingSettingsTest, EnableLoggingNonExistTargetBucketNameTest)
{
    auto targetBucketName = TestUtils::GetBucketName("cpp-sdk-bucketloggingsettings");
    EXPECT_EQ(TestUtils::BucketExists(*Client, targetBucketName), false);

    auto outcome = Client->SetBucketLogging(SetBucketLoggingRequest(BucketName, targetBucketName, "LogPrefix"));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "InvalidTargetBucketForLogging");
}

TEST_F(BucketLoggingSettingsTest, EnableLoggingInvalidPrefixNameTest)
{
    for (auto const &invalidPrefix : TestUtils::InvalidLoggingPrefixNamesList()) {
        auto outcome = Client->SetBucketLogging(SetBucketLoggingRequest(BucketName, BucketName, invalidPrefix));
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketLoggingSettingsTest, EnableLoggingWithEmtpyPrefixNameTest)
{
    auto outcome = Client->SetBucketLogging(SetBucketLoggingRequest(BucketName, BucketName, ""));
    std::cout << "SetBucketLogging, requestid:" << outcome.result().RequestId() << std::endl;
    TestUtils::WaitForCacheExpire(5);
    auto blOutcome = Client->GetBucketLogging(BucketName);
    int i = 0;
    while (blOutcome.result().TargetBucket() != BucketName) {
        std::cout << "GetBucketLogging, cnt:" << i++ << ", requestid:" << blOutcome.result().RequestId() << std::endl;
        TestUtils::WaitForCacheExpire(5);
        blOutcome = Client->GetBucketLogging(BucketName);
    }
    EXPECT_EQ(blOutcome.isSuccess(), true);
    EXPECT_STREQ(blOutcome.result().TargetBucket().c_str(), BucketName.c_str());
    EXPECT_STREQ(blOutcome.result().TargetPrefix().c_str(), "");
}

TEST_F(BucketLoggingSettingsTest, BucketLoggingWithInvalidResponseBodyTest)
{
    auto gblRequest = GetBucketLoggingRequest(BucketName);
    gblRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gblOutcome = Client->GetBucketLogging(gblRequest);
    EXPECT_EQ(gblOutcome.isSuccess(), false);
    EXPECT_EQ(gblOutcome.error().Code(), "ParseXMLError");
}

TEST_F(BucketLoggingSettingsTest, GetBucketLoggingResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketLoggingStatus xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <LoggingEnabled>
                                <TargetBucket>mybucketlogs</TargetBucket>
                                <TargetPrefix>mybucket-access_log/</TargetPrefix>
                            </LoggingEnabled>
                        </BucketLoggingStatus>)";
    GetBucketLoggingResult result(xml);
    EXPECT_EQ(result.TargetBucket(), "mybucketlogs");
    EXPECT_EQ(result.TargetPrefix(), "mybucket-access_log/");
}

TEST_F(BucketLoggingSettingsTest, SetBucketLoggingRequestConstructionFunctiontest)
{ 
    SetBucketLoggingRequest request("test");
}

TEST_F(BucketLoggingSettingsTest, GetBucketLoggingResultBranchtest)
{
    GetBucketLoggingResult result("test");
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketLogging xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <LoggingEnabled>
                                <TargetBucket>mybucketlogs</TargetBucket>
                                <TargetPrefix>mybucket-access_log/</TargetPrefix>
                            </LoggingEnabled>
                        </BucketLogging>)";
    GetBucketLoggingResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketLoggingStatus xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <LoggingEnabled>
                            </LoggingEnabled>
                        </BucketLoggingStatus>)";
    GetBucketLoggingResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketLoggingStatus xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                                <TargetBucket>mybucketlogs</TargetBucket>
                                <TargetPrefix>mybucket-access_log/</TargetPrefix>
                        </BucketLoggingStatus>)";
    GetBucketLoggingResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketLoggingResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketLogging xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <LoggingEnabled>
                                <TargetBucket></TargetBucket>
                                <TargetPrefix></TargetPrefix>
                            </LoggingEnabled>
                        </BucketLogging>)";
    GetBucketLoggingResult result5(xml);
}

}
}