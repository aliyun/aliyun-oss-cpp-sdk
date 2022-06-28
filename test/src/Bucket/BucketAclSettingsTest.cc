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

class BucketAclSettingsTest : public ::testing::Test {
protected:
    BucketAclSettingsTest()
    {
    }

    ~BucketAclSettingsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketaclsettings");
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

std::shared_ptr<OssClient> BucketAclSettingsTest::Client = nullptr;
std::string BucketAclSettingsTest::BucketName = "";

TEST_F(BucketAclSettingsTest, SetBucketAclUseRequestTest)
{
    Client->SetBucketAcl(SetBucketAclRequest(BucketName, CannedAccessControlList::PublicRead));
    TestUtils::WaitForCacheExpire(5);
    auto aclOutcome = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicRead);

    //set to readwrite
    Client->SetBucketAcl(SetBucketAclRequest(BucketName, CannedAccessControlList::PublicReadWrite));
    TestUtils::WaitForCacheExpire(5);
    aclOutcome = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);

    //set to private
    Client->SetBucketAcl(SetBucketAclRequest(BucketName, CannedAccessControlList::Private));
    TestUtils::WaitForCacheExpire(5);
    aclOutcome = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Private);
}

TEST_F(BucketAclSettingsTest, SetBucketAclUseDefaultTest)
{
    auto aclOutcome = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);

    //set to Default
    Client->SetBucketAcl(SetBucketAclRequest(BucketName, CannedAccessControlList::Default));
    TestUtils::WaitForCacheExpire(5);
    auto aclOutcome1 = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    EXPECT_EQ(aclOutcome1.isSuccess(), true);
   
    EXPECT_EQ(aclOutcome1.result().Acl(), aclOutcome.result().Acl());
}

TEST_F(BucketAclSettingsTest, SetBucketAclPublicReadTest)
{
    auto aclOutcome = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);

    //set to Default
    SetBucketAclRequest request(BucketName, CannedAccessControlList::Default);
    request.setAcl(CannedAccessControlList::PublicRead);
    Client->SetBucketAcl(request);
    TestUtils::WaitForCacheExpire(5);
    auto aclOutcome1 = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    TestUtils::WaitForCacheExpire(5);
    aclOutcome1 = Client->GetBucketAcl(GetBucketAclRequest(BucketName));
    EXPECT_EQ(aclOutcome1.isSuccess(), true);
    EXPECT_EQ(aclOutcome1.result().Acl(), CannedAccessControlList::PublicRead);
}

TEST_F(BucketAclSettingsTest, GetBucketAclNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-bucket-acl");
    auto aclOutcome = Client->GetBucketAcl(name);
    EXPECT_EQ(aclOutcome.isSuccess(), false);
    EXPECT_EQ(aclOutcome.error().Code(), "NoSuchBucket");
}

#if 0
//NG
TEST_F(BucketAclSettingsTest, SetBucketAclNegativeTest)
{
    auto aclOutcome = Client->SetBucketAcl("no-exist-bucket", CannedAccessControlList::PublicRead);
    EXPECT_EQ(aclOutcome.isSuccess(), false);
    EXPECT_EQ(aclOutcome.error().Code(), "NoSuchBucket");
}
#endif

#if 0
//NG
TEST_F(BucketAclSettingsTest, SetBucketAclNegativeTest)
{
    auto aclOutcome = Client->SetBucketAcl("no-exist-bucket", CannedAccessControlList::PublicRead);
    EXPECT_EQ(aclOutcome.isSuccess(), false);
    EXPECT_EQ(aclOutcome.error().code(), "NoSuchBucket");
}
#endif

TEST_F(BucketAclSettingsTest, GetBucketAclResult)
{
    std::string xml = R"(<?xml version="1.0" ?>
                        <AccessControlPolicy>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user_example</DisplayName>
                            </Owner>
                            <AccessControlList>
                                <Grant>public-read</Grant>
                            </AccessControlList>
                        </AccessControlPolicy>)";
    GetBucketAclResult result(xml);
    EXPECT_EQ(result.Acl(), CannedAccessControlList::PublicRead);
    EXPECT_EQ(result.Owner().DisplayName(), "user_example");
}

TEST_F(BucketAclSettingsTest, SetBucketAclInvalidValidateTest)
{
    auto aclOutcome = Client->SetBucketAcl("Invalid-bucket-test", CannedAccessControlList::PublicRead);
    EXPECT_EQ(aclOutcome.isSuccess(), false);
    EXPECT_EQ(aclOutcome.error().Code(), "ValidateError");
}

TEST_F(BucketAclSettingsTest, OssBucketRequestSetBucketTest)
{
    GetBucketAclRequest request(BucketName);
    request.setBucket(BucketName);
    auto aclOutcome = Client->GetBucketAcl(request);
    EXPECT_EQ(aclOutcome.isSuccess(), true);
}

TEST_F(BucketAclSettingsTest, BucketAclWithInvalidResponseBodyTest)
{
    auto gbaRequest = GetBucketAclRequest(BucketName);
    gbaRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gbaOutcome = Client->GetBucketAcl(gbaRequest);
    EXPECT_EQ(gbaOutcome.isSuccess(), false);
    EXPECT_EQ(gbaOutcome.error().Code(), "ParseXMLError");
}

TEST_F(BucketAclSettingsTest, GetBucketAclResultBranchTest)
{
    GetBucketAclResult result("test");

    std::string xml = R"(<?xml version="1.0" ?>
                        <AccessControl>
                            <Owner>
                            </Owner>
                            <AccessControlList>
                            </AccessControlList>
                        </AccessControl>)";
    GetBucketAclResult result1(std::make_shared<std::stringstream>(xml));

    xml = R"(<?xml version="1.0" ?>
                        <AccessControlPolicy>
                                <ID>00220120222</ID>
                                <DisplayName>user_example</DisplayName>
                                <Grant>public-read</Grant>
                        </AccessControlPolicy>)";
    GetBucketAclResult result2(xml);

    xml = R"(<?xml version="1.0" ?>
                        <AccessControlPolicy>
                            <Owner>
                            </Owner>
                            <AccessControlList>
                            </AccessControlList>
                        </AccessControlPolicy>)";
    GetBucketAclResult result4(xml);

    xml = R"(<?xml version="1.0" ?>
                        <AccessControlPolicy>
                            <Owner>
                                <ID></ID>
                                <DisplayName></DisplayName>
                            </Owner>
                            <AccessControlList>
                                <Grant></Grant>
                            </AccessControlList>
                        </AccessControlPolicy>)";
    GetBucketAclResult result5(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketAclResult result6(xml);

}
}
}