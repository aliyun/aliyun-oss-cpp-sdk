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

#include <stdlib.h>
#include <sstream>
#include <gtest/gtest.h>
#include <alibabacloud/oss/OssClient.h>
#include "../Config.h"
#include "../Utils.h"

namespace AlibabaCloud
{
namespace OSS 
{

class ObjectAclTest : public ::testing::Test
{
protected:
    ObjectAclTest()
    {
    }

    ~ObjectAclTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectacl");
        CreateBucketOutcome outCome = Client->CreateBucket(CreateBucketRequest(BucketName));
        EXPECT_EQ(outCome.isSuccess(), true);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
       TestUtils::CleanBucket(*Client, BucketName);
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

std::shared_ptr<OssClient> ObjectAclTest::Client = nullptr;
std::string ObjectAclTest::BucketName = "";

TEST_F(ObjectAclTest, SetAndGetObjectAclSuccessTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectacl");

    std::string text = "hellowworld";
    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, objName, std::make_shared<std::stringstream>(text)));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    auto aclOutcome =  Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Default);

    auto setOutCome = Client->SetObjectAcl(SetObjectAclRequest(BucketName, objName, CannedAccessControlList::PublicRead));
	EXPECT_EQ(aclOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(2);
    aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicRead);

    //set to readwrite
    Client->SetObjectAcl(SetObjectAclRequest(BucketName, objName, CannedAccessControlList::PublicReadWrite));
    TestUtils::WaitForCacheExpire(2);
    aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);

    //set to private
    Client->SetObjectAcl(SetObjectAclRequest(BucketName, objName, CannedAccessControlList::Private));
    TestUtils::WaitForCacheExpire(2);
    aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Private);

    // set to default
    Client->SetObjectAcl(SetObjectAclRequest(BucketName, objName, CannedAccessControlList::Default));
    TestUtils::WaitForCacheExpire(2);
    aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Default);

    // set to private
	TestUtils::WaitForCacheExpire(2);
    SetObjectAclRequest aclRequest(BucketName, objName);
    aclRequest.setAcl(CannedAccessControlList::Private);
    Client->SetObjectAcl(aclRequest);

    aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Private);

    // set to void
	TestUtils::WaitForCacheExpire(2);
    SetObjectAclRequest aclRequest1(BucketName, objName);
	auto setOutcom = Client->SetObjectAcl(aclRequest1);
	EXPECT_EQ(setOutcom.isSuccess(), false);
    TestUtils::WaitForCacheExpire(5);

    aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Private);
}


TEST_F(ObjectAclTest, SetAndGetObjectAclErrorTest)
{
    auto aclOutcome1 = Client->SetObjectAcl(SetObjectAclRequest(BucketName, "test-void", CannedAccessControlList::PublicRead));
    TestUtils::WaitForCacheExpire(5);
    EXPECT_EQ(aclOutcome1.isSuccess(), false);
    EXPECT_EQ(aclOutcome1.error().Code().size()>0, true);

    auto aclOutcome2 = Client->GetObjectAcl(GetObjectAclRequest(BucketName, "test-void"));
    TestUtils::WaitForCacheExpire(5);
	EXPECT_EQ(aclOutcome2.isSuccess(), false);
    EXPECT_EQ(aclOutcome2.error().Code().size()>0, true);
}

TEST_F(ObjectAclTest, GetObjectAclWithInvalidResponseBodyTest)
{
    std::string objName = TestUtils::GetObjectKey("GetObjectAclWithInvalidResponseBodyTest");
    std::string text = "hellowworld";
    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, objName, std::make_shared<std::stringstream>(text)));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GetObjectAclRequest gaclRequest(BucketName, objName);
    gaclRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gaclOutcome = Client->GetObjectAcl(gaclRequest);
    EXPECT_EQ(gaclOutcome.isSuccess(), false);
    EXPECT_EQ(gaclOutcome.error().Code(), "ParseXMLError");
}

TEST_F(ObjectAclTest, GetObjectAclResultTest)
{
    std::string xml = R"(<?xml version="1.0" ?>
                    <AccessControlPolicy>
                        <Owner>
                            <ID>00220120222</ID>
                            <DisplayName>00220120222</DisplayName>
                        </Owner>
                        <AccessControlList>
                            <Grant>public-read</Grant>
                        </AccessControlList>
                    </AccessControlPolicy>)";
    GetObjectAclResult result(xml);
    EXPECT_EQ(result.Owner().DisplayName(), "00220120222");
    EXPECT_EQ(result.Owner().Id(), "00220120222");
    EXPECT_EQ(result.Acl(), CannedAccessControlList::PublicRead);
}

TEST_F(ObjectAclTest, GetObjectAclResultBranchTest)
{
    GetObjectAclResult result("test");

    std::string xml = R"(<?xml version="1.0" ?>
                    <AccessControl>
                    </AccessControl>)";
    GetObjectAclResult result1(xml);

    xml = R"(<?xml version="1.0" ?>
                    <AccessControlPolicy>
                            <ID>00220120222</ID>
                            <DisplayName>00220120222</DisplayName>
                            <Grant>public-read</Grant>
                    </AccessControlPolicy>)";
    GetObjectAclResult result2(xml);

    xml = R"(<?xml version="1.0" ?>
                    <AccessControlPolicy>
                        <Owner>

                        </Owner>
                        <AccessControlList>

                        </AccessControlList>
                    </AccessControlPolicy>)";
    GetObjectAclResult result3(xml);

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
    GetObjectAclResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetObjectAclResult result5(xml);
}
}
}
