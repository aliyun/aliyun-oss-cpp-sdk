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
#include <alibabacloud/oss/Types.h>
#include "../Config.h"
#include "../Utils.h"
#include "src/utils/FileSystemUtils.h"
#include "src/utils/Utils.h"
#include <fstream>

namespace AlibabaCloud
{ 
namespace OSS 
{

class BucketRequestPaymentTest : public ::testing::Test
{
protected:
    BucketRequestPaymentTest()
    {
    }

    ~BucketRequestPaymentTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        ClientConfiguration conf;
        conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());

        BucketName1 = TestUtils::GetBucketName("cpp-sdk-objectcopy1");
        CreateBucketOutcome outCome = Client->CreateBucket(CreateBucketRequest(BucketName1));
        EXPECT_EQ(outCome.isSuccess(), true);

        PayerClient = std::make_shared<OssClient>(Config::Endpoint, Config::PayerAccessKeyId, Config::PayerAccessKeySecret, ClientConfiguration());

    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName1);
        Client = nullptr;
        PayerClient = nullptr;
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
    static std::shared_ptr<OssClient> PayerClient;
    static std::string BucketName1;
};

std::shared_ptr<OssClient> BucketRequestPaymentTest::Client = nullptr;
std::shared_ptr<OssClient> BucketRequestPaymentTest::PayerClient = nullptr;
std::string BucketRequestPaymentTest::BucketName1 = "";

TEST_F(BucketRequestPaymentTest, PutAndGetBucketRequestPayment)
{
    GetBucketRequestPaymentRequest getRequest(BucketName1);
    GetBucketPaymentOutcome getOutcome = Client->GetBucketRequestPayment(getRequest);
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().Payer(), RequestPayer::BucketOwner);

    SetBucketRequestPaymentRequest setRequest(BucketName1);
    setRequest.setRequestPayer(RequestPayer::NotSet);
    VoidOutcome setOutcome = Client->SetBucketRequestPayment(setRequest);
    EXPECT_EQ(setOutcome.isSuccess(), false);

    setRequest.setRequestPayer(RequestPayer::BucketOwner);
    setOutcome = Client->SetBucketRequestPayment(setRequest);
    EXPECT_EQ(setOutcome.isSuccess(), true);

    setRequest.setRequestPayer(RequestPayer::Requester);
    setOutcome = Client->SetBucketRequestPayment(setRequest);
    EXPECT_EQ(setOutcome.isSuccess(), true);

    getOutcome = Client->GetBucketRequestPayment(getRequest);
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().Payer(), RequestPayer::Requester);
}

TEST_F(BucketRequestPaymentTest, BucketRequestPaymentWithInvalidResponseBodyTest)
{
    auto gbrpRequest = GetBucketRequestPaymentRequest(BucketName1);
    gbrpRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gbrpOutcome = Client->GetBucketRequestPayment(gbrpRequest);
    EXPECT_EQ(gbrpOutcome.isSuccess(), false);
    EXPECT_EQ(gbrpOutcome.error().Code(), "ParseXMLError");
}

TEST_F(BucketRequestPaymentTest, BucketPaymentResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <RequestPaymentConfiguration>
                        <Payer>BucketOwner</Payer>
                        </RequestPaymentConfiguration>)";
    GetBucketPaymentResult result(xml);
    EXPECT_EQ(result.Payer(), RequestPayer::BucketOwner);
}

TEST_F(BucketRequestPaymentTest, GetBucketRequestPaymentInvalidValidateTest)
{
    auto getoutcome = Client->GetBucketRequestPayment(GetBucketRequestPaymentRequest("Invalid-bucket-test"));

    EXPECT_EQ(getoutcome.isSuccess(), false);
    EXPECT_EQ(getoutcome.error().Code(), "ValidateError");
}

TEST_F(BucketRequestPaymentTest, BucketPaymentResultBranchTest)
{
    GetBucketPaymentResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <RequestPayment>
                        <Payer>BucketOwner</Payer>
                        </RequestPayment>)";
    GetBucketPaymentResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <RequestPaymentConfiguration>
                        </RequestPaymentConfiguration>)";
    GetBucketPaymentResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <RequestPaymentConfiguration>
                        <Payer></Payer>
                        </RequestPaymentConfiguration>)";
    GetBucketPaymentResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketPaymentResult result4(xml);
}
}
}
