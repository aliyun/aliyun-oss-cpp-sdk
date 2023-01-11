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

class BucketStorageCapacityTest : public ::testing::Test {
protected:
    BucketStorageCapacityTest()
    {
    }

    ~BucketStorageCapacityTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketstoragecapacity");
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
std::shared_ptr<OssClient> BucketStorageCapacityTest::Client = nullptr;
std::string BucketStorageCapacityTest::BucketName = "";

TEST_F(BucketStorageCapacityTest, InvalidBucketNameTest)
{
    for (auto const& invalidBucketName : TestUtils::InvalidBucketNamesList()) {
        auto outcome = Client->SetBucketStorageCapacity(invalidBucketName, 10240);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketStorageCapacityTest, SetBucketStorageCapacityTest)
{
    auto sOutcome = Client->SetBucketStorageCapacity(SetBucketStorageCapacityRequest(BucketName, 10240));
    EXPECT_EQ(sOutcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);

    auto outcome = Client->GetBucketStorageCapacity(GetBucketStorageCapacityRequest(BucketName));
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().StorageCapacity(), 10240LL);
}

TEST_F(BucketStorageCapacityTest, GetBucketStorageCapacityNegativeTest)
{
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-bucketstoragecapacity");
    auto outcome = Client->GetBucketStorageCapacity(bucketName);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketStorageCapacityTest, BucketStorageCapacityWithInvalidResponseBodyTest)
{
    auto gbscRequest = GetBucketStorageCapacityRequest(BucketName);
    gbscRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gbscOutcome = Client->GetBucketStorageCapacity(gbscRequest);
    EXPECT_EQ(gbscOutcome.isSuccess(), false);
    EXPECT_EQ(gbscOutcome.error().Code(), "ParseXMLError");
}

TEST_F(BucketStorageCapacityTest, SetBucketStorageCapacityInvalidInputTest)
{
    auto outcome = Client->SetBucketStorageCapacity(SetBucketStorageCapacityRequest(BucketName, -2));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
}

TEST_F(BucketStorageCapacityTest, GetBucketStorageCapacityResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketUserQos>
                          <StorageCapacity>10240</StorageCapacity>
                        </BucketUserQos>)";
    GetBucketStorageCapacityResult result(xml);
    EXPECT_EQ(result.StorageCapacity(), 10240LL);
}

TEST_F(BucketStorageCapacityTest, GetBucketStorageCapacityResultBranchTest)
{
    GetBucketStorageCapacityResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketUser>
                          <StorageCapacity>10240</StorageCapacity>
                        </BucketUser>)";
    GetBucketStorageCapacityResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketUserQos>
                        </BucketUserQos>)";
    GetBucketStorageCapacityResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketUserQos>
                          <StorageCapacity></StorageCapacity>
                        </BucketUserQos>)";
    GetBucketStorageCapacityResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketStorageCapacityResult result4(xml);
}
}
}