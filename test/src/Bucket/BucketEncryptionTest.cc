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

    class BucketEncryptionTest : public ::testing::Test {
    protected:
        BucketEncryptionTest()
        {
        }

        ~BucketEncryptionTest() override
        {
        }

        // Sets up the stuff shared by all tests in this test case.
        static void SetUpTestCase()
        {
            Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
            BucketName = TestUtils::GetBucketName("cpp-sdk-bucketEncryption");
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

    std::shared_ptr<OssClient> BucketEncryptionTest::Client = nullptr;
    std::string BucketEncryptionTest::BucketName = "";

    TEST_F(BucketEncryptionTest, SetAndDeleteBucketEncryptionTest)
    {
        SetBucketEncryptionRequest setrequest(BucketName);
        setrequest.setSSEAlgorithm(SSEAlgorithm::KMS);
        auto setoutcome = Client->SetBucketEncryption(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        DeleteBucketEncryptionRequest delrequest(BucketName);
        auto deloutcome = Client->DeleteBucketEncryption(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketEncryptionTest, GetBucketEncryptionTest)
    {
        SetBucketEncryptionRequest setrequest(BucketName);
        setrequest.setSSEAlgorithm(SSEAlgorithm::KMS);
        setrequest.setKMSMasterKeyID("1234");
        auto setoutcome = Client->SetBucketEncryption(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        GetBucketEncryptionRequest getrequest(BucketName);
        auto getoutcome = Client->GetBucketEncryption(getrequest);
        EXPECT_EQ(getoutcome.isSuccess(),true);
        EXPECT_EQ(getoutcome.result().SSEAlgorithm(), SSEAlgorithm::KMS);
        EXPECT_EQ(getoutcome.result().KMSMasterKeyID(), "1234");

        setrequest.setSSEAlgorithm(SSEAlgorithm::AES256);
        setoutcome = Client->SetBucketEncryption(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        getoutcome = Client->GetBucketEncryption(getrequest);
        EXPECT_EQ(getoutcome.isSuccess(), true);
        EXPECT_EQ(getoutcome.result().SSEAlgorithm(), SSEAlgorithm::AES256);
    }

    TEST_F(BucketEncryptionTest, GetBucketEncryptionResult)
    {
        std::string xml = R"(<?xml version="1.0" ?>
                        <ServerSideEncryptionRule>
                            <ApplyServerSideEncryptionByDefault>
                                <SSEAlgorithm>KMS</SSEAlgorithm>
                                <KMSMasterKeyID>1234</KMSMasterKeyID>
                            </ApplyServerSideEncryptionByDefault>
                        </ServerSideEncryptionRule>)";
        GetBucketEncryptionResult result(xml);
        EXPECT_EQ(result.SSEAlgorithm(), SSEAlgorithm::KMS);
        EXPECT_EQ(result.KMSMasterKeyID(), "1234");
    }
}
}