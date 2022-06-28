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

    class BucketVersioningTest : public ::testing::Test {
    protected:
        BucketVersioningTest()
        {
        }

        ~BucketVersioningTest() override
        {
        }

        // Sets up the stuff shared by all tests in this test case.
        static void SetUpTestCase()
        {
            Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
            BucketName = TestUtils::GetBucketName("cpp-sdk-versioning");
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

    std::shared_ptr<OssClient> BucketVersioningTest::Client = nullptr;
    std::string BucketVersioningTest::BucketName = "";

    TEST_F(BucketVersioningTest, SetAndGetBucketVersioningTest)
    {
        //default
        auto gOutcome = Client->GetBucketVersioning(GetBucketVersioningRequest(BucketName));
        EXPECT_EQ(gOutcome.isSuccess(), true);
        EXPECT_EQ(gOutcome.result().RequestId().size(), 24UL);
        EXPECT_EQ(gOutcome.result().Status(), VersioningStatus::NotSet);

        auto bfOutcome = Client->GetBucketInfo(BucketName);
        EXPECT_EQ(bfOutcome.isSuccess(), true);
        EXPECT_EQ(bfOutcome.result().RequestId().size(), 24UL);
        EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::NotSet);

        //set Enabled
        SetBucketVersioningRequest request(BucketName, VersioningStatus::Enabled);
        auto sOutcome = Client->SetBucketVersioning(request);
        EXPECT_EQ(sOutcome.isSuccess(), true);
        EXPECT_EQ(sOutcome.result().RequestId().size(), 24UL);

        gOutcome = Client->GetBucketVersioning(GetBucketVersioningRequest(BucketName));
        EXPECT_EQ(gOutcome.isSuccess(), true);
        EXPECT_EQ(gOutcome.result().Status(), VersioningStatus::Enabled);

        bfOutcome = Client->GetBucketInfo(BucketName);
        EXPECT_EQ(bfOutcome.isSuccess(), true);
        EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

        //set Suspended
        request.setStatus(VersioningStatus::Suspended);
        sOutcome = Client->SetBucketVersioning(request);
        EXPECT_EQ(sOutcome.isSuccess(), true);
        EXPECT_EQ(sOutcome.result().RequestId().size(), 24UL);

        gOutcome = Client->GetBucketVersioning(GetBucketVersioningRequest(BucketName));
        EXPECT_EQ(gOutcome.isSuccess(), true);
        EXPECT_EQ(gOutcome.result().Status(), VersioningStatus::Suspended);

        bfOutcome = Client->GetBucketInfo(BucketName);
        EXPECT_EQ(bfOutcome.isSuccess(), true);
        EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Suspended);

        //set Enabled again
        sOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
        EXPECT_EQ(sOutcome.isSuccess(), true);
        EXPECT_EQ(sOutcome.result().RequestId().size(), 24UL);

        gOutcome = Client->GetBucketVersioning(GetBucketVersioningRequest(BucketName));
        EXPECT_EQ(gOutcome.isSuccess(), true);
        EXPECT_EQ(gOutcome.result().Status(), VersioningStatus::Enabled);

        bfOutcome = Client->GetBucketInfo(BucketName);
        EXPECT_EQ(bfOutcome.isSuccess(), true);
        EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);
    }
    
    TEST_F(BucketVersioningTest, SetAndGetBucketVersioningNGTest)
    {
        SetBucketVersioningRequest sRequest("Invalid-Bucket", VersioningStatus::Enabled);
        auto sOutcome = Client->SetBucketVersioning(sRequest);
        EXPECT_EQ(sOutcome.isSuccess(), false);

        GetBucketVersioningRequest gRequest("Invalid-Bucket");
        auto gOutcome = Client->GetBucketVersioning(gRequest);
        EXPECT_EQ(gOutcome.isSuccess(), false);
    }

    TEST_F(BucketVersioningTest, GetBucketVersioningWithInvalidResponseBodyTest)
    {
        SetBucketVersioningRequest request(BucketName, VersioningStatus::Enabled);
        auto sOutcome = Client->SetBucketVersioning(request);
        EXPECT_EQ(sOutcome.isSuccess(), true);
        EXPECT_EQ(sOutcome.result().RequestId().size(), 24UL);

        auto gbvRequest = GetBucketVersioningRequest(BucketName);
        gbvRequest.setResponseStreamFactory([=]() {
            auto content = std::make_shared<std::stringstream>();
            content->write("invlid data", 11);
            return content;
        });
        auto gbvOutcome = Client->GetBucketVersioning(gbvRequest);
        EXPECT_EQ(gbvOutcome.isSuccess(), false);
        EXPECT_EQ(gbvOutcome.error().Code(), "ParseXMLError");
    }

    TEST_F(BucketVersioningTest, GetBucketVersioningResult)
    {
        std::string xml;
        GetBucketVersioningResult result;

        xml = R"(<?xml version="1.0" ?>
                <VersioningConfiguration>
                    <Status>Enabled</Status>
                </VersioningConfiguration>)";

        result = GetBucketVersioningResult(xml);
        EXPECT_EQ(result.Status(), VersioningStatus::Enabled);

        xml = R"(<?xml version="1.0" ?>
                <VersioningConfiguration>
                    <Status>Suspended</Status>
                </VersioningConfiguration>)";
        result = GetBucketVersioningResult(xml);
        EXPECT_EQ(result.Status(), VersioningStatus::Suspended);

        xml = R"(<?xml version="1.0" ?>
                <VersioningConfiguration>
                </VersioningConfiguration>)";
        result = GetBucketVersioningResult(xml);
        EXPECT_EQ(result.Status(), VersioningStatus::NotSet);
    }
}
}