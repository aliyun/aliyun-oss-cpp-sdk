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
#include <fstream>

namespace AlibabaCloud {
namespace OSS {

class ObjectProgressTest : public ::testing::Test {
protected:
    ObjectProgressTest()
    {
    }

    ~ObjectProgressTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectprogress");
        Client->CreateBucket(CreateBucketRequest(BucketName));
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

std::shared_ptr<OssClient> ObjectProgressTest::Client = nullptr;
std::string ObjectProgressTest::BucketName = "";

static void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData)
{
    std::cout << "ProgressCallback[" << userData << "] => " <<
        increment << " ," << transfered << "," << total << std::endl;
}

TEST_F(ObjectProgressTest, PutObjectProgressTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectProgressTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    TransferProgress arg;
    arg.Handler = ProgressCallback;
    arg.UserData = this;
    request.setTransferProgress(arg);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
}

TEST_F(ObjectProgressTest, GetObjectProgressTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectProgressTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest grequest(BucketName, key);
    TransferProgress arg;
    arg.Handler = ProgressCallback;
    arg.UserData = this;
    grequest.setTransferProgress(arg);
    auto gOutcome = Client->GetObject(grequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
}


}
}