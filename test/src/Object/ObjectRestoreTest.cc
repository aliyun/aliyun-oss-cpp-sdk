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

namespace AlibabaCloud{ namespace OSS{
class ObjectRestoreTest : public ::testing::Test
{
protected:
    ObjectRestoreTest()
    {
    }

    ~ObjectRestoreTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        ClientConfiguration conf;
        conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        
		BucketName = TestUtils::GetBucketName("cpp-sdk-objectrestore-standard");
        CreateBucketOutcome outCome = Client->CreateBucket(CreateBucketRequest(BucketName));
        EXPECT_EQ(outCome.isSuccess(), true);

		BucketNameRestore = TestUtils::GetBucketName("cpp-sdk-objectrestore-archive");
		outCome = Client->CreateBucket(CreateBucketRequest(BucketNameRestore, Archive));
        EXPECT_EQ(outCome.isSuccess(), true);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName);
        TestUtils::CleanBucket(*Client, BucketNameRestore);
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
    static std::string BucketNameRestore;
};

std::shared_ptr<OssClient> ObjectRestoreTest::Client = nullptr;
std::string ObjectRestoreTest::BucketName = "";
std::string ObjectRestoreTest::BucketNameRestore = "";


TEST_F(ObjectRestoreTest, SetAndGetObjectRestoreSuccessTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectrestore");

    // first: put object
    std::string text = "hellowworld";
    auto putOutcome = Client->PutObject(PutObjectRequest(BucketNameRestore, objName, std::make_shared<std::stringstream>(text)));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    //second:restore object
    VoidOutcome restoreOutCome =  Client->RestoreObject(BucketNameRestore, objName);
    EXPECT_EQ(restoreOutCome.isSuccess(), true);

    // third:wait restore success
    bool result = false;
    do {
        TestUtils::WaitForCacheExpire(5);
        restoreOutCome =  Client->RestoreObject(RestoreObjectRequest(BucketNameRestore, objName));
        if(restoreOutCome.isSuccess()) {
            result = true;
        } else {
            std::string errorCode = restoreOutCome.error().Code();
			printf("errorcode:%s.\n",errorCode.c_str());
            EXPECT_EQ((errorCode == "RestoreAlreadyInProgress") || (errorCode == "ValidateError"), true);
        }
    } while (!result);

    //repeated success
    restoreOutCome =  Client->RestoreObject(RestoreObjectRequest(BucketNameRestore, objName));
    EXPECT_EQ(restoreOutCome.isSuccess(), true);

	TestUtils::WaitForCacheExpire(2);

    // read data
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketNameRestore, objName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData ;
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, text);

    // restore not exist object
    restoreOutCome =  Client->RestoreObject(RestoreObjectRequest(BucketNameRestore, "aaa"));
    EXPECT_EQ(restoreOutCome.isSuccess(), false);

    //restore  standard object
    std::string objStandard = TestUtils::GetObjectKey("test-cpp-sdk-objectrestore");
    putOutcome = Client->PutObject(PutObjectRequest(BucketName, objStandard, std::make_shared<std::stringstream>(text)));
    EXPECT_EQ(putOutcome.isSuccess(), true);
    restoreOutCome =  Client->RestoreObject(RestoreObjectRequest(BucketName, objStandard));
    EXPECT_EQ(restoreOutCome.isSuccess(), false);
    EXPECT_EQ(restoreOutCome.error().Code(), "OperationNotSupported");
}
}}

