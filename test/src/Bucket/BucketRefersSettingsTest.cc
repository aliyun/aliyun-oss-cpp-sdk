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

class BucketRefersSettingsTest : public ::testing::Test {
protected:
    BucketRefersSettingsTest()
    {
    }

    ~BucketRefersSettingsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketreferssettings");
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

std::shared_ptr<OssClient> BucketRefersSettingsTest::Client = nullptr;
std::string BucketRefersSettingsTest::BucketName = "";

TEST_F(BucketRefersSettingsTest, GetBucketDefaultRefersTest)
{
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-bucketreferssettings");

    auto cOutcome = Client->CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(cOutcome.isSuccess(), true);

    auto refOutcome = Client->GetBucketReferer(bucketName);
    EXPECT_EQ(refOutcome.isSuccess(), true);
    EXPECT_EQ(refOutcome.result().RefererList().size(), 0UL);
    EXPECT_EQ(refOutcome.result().AllowEmptyReferer(), true);

    Client->DeleteBucket(DeleteBucketRequest(bucketName));
}

TEST_F(BucketRefersSettingsTest, SetBucketRefersPositiveTest)
{
    //initialize refer list
    RefererList referList = {
        "http://*.aliyun.com", 
        "http://wwww.alibaba.com" 
    };
    //use construct which pass in 3 parameters
    auto outcome = Client->SetBucketReferer(SetBucketRefererRequest(BucketName, referList, false));
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    auto refOutcome = Client->GetBucketReferer(GetBucketRefererRequest(BucketName));
    EXPECT_EQ(refOutcome.isSuccess(), true);
    EXPECT_EQ(refOutcome.result().RefererList().size(), 2UL);
    EXPECT_EQ(refOutcome.result().AllowEmptyReferer(), false);

    referList.push_back("http://www.taobao?.com");
    //use construct which pass in 2 parameters, and allowEmptyRefer set to true
    //outcome = Client->SetBucketReferer(SetBucketRefererRequest(BucketName, referList));
    outcome = Client->SetBucketReferer(BucketName, referList, true);
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    refOutcome = Client->GetBucketReferer(GetBucketRefererRequest(BucketName));
    EXPECT_EQ(refOutcome.isSuccess(), true);
    EXPECT_EQ(refOutcome.result().RefererList().size(), 3UL);
    //it is true this time
    EXPECT_EQ(refOutcome.result().AllowEmptyReferer(), true);

    //use construct which pass in 1 parameter, which means set it back to init status
    outcome = Client->SetBucketReferer(SetBucketRefererRequest(BucketName));
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    refOutcome = Client->GetBucketReferer(GetBucketRefererRequest(BucketName));
    EXPECT_EQ(refOutcome.isSuccess(), true);
    EXPECT_EQ(refOutcome.result().RefererList().empty(), true);
    EXPECT_EQ(refOutcome.result().AllowEmptyReferer(), true);
}

TEST_F(BucketRefersSettingsTest, SetBucketRefersEmptyListTest)
{
    auto outcome = Client->SetBucketReferer(SetBucketRefererRequest(BucketName, RefererList(), false));
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    auto refOutcome = Client->GetBucketReferer(GetBucketRefererRequest(BucketName));
    EXPECT_EQ(refOutcome.isSuccess(), true);
    EXPECT_EQ(refOutcome.result().RefererList().empty(), true);
    EXPECT_EQ(refOutcome.result().AllowEmptyReferer(), false);
}

TEST_F(BucketRefersSettingsTest, GetBucketRefersNegativeTest)
{
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-bucketreferssettings");
    auto outcome = Client->GetBucketReferer(bucketName);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketRefersSettingsTest, SetBucketRefersNegativeTest)
{
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-bucketreferssettings");
    auto outcome = Client->SetBucketReferer(SetBucketRefererRequest(bucketName, RefererList(), false));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}


TEST_F(BucketRefersSettingsTest, SetBucketRefersEmptyElementTest)
{
    RefererList referList = {
        "", ""
    };

    auto outcome = Client->SetBucketReferer(SetBucketRefererRequest(BucketName, referList));
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    auto refOutcome = Client->GetBucketReferer(GetBucketRefererRequest(BucketName));
    EXPECT_EQ(refOutcome.isSuccess(), true);
    EXPECT_EQ(refOutcome.result().RefererList().empty(), true);
    EXPECT_EQ(refOutcome.result().AllowEmptyReferer(), true);
}

TEST_F(BucketRefersSettingsTest, SetBucketRefererNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-bucket-refer");
    RefererList referList;
    referList.push_back("http://www.taobao?.com");
    auto outcome = Client->SetBucketReferer(name, referList, true);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketRefersSettingsTest, GetBucketRefererResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <RefererConfiguration>
                    <AllowEmptyReferer>true</AllowEmptyReferer >
                        <RefererList>
                            <Referer> http://www.aliyun.com</Referer>
                            <Referer> https://www.aliyun.com</Referer>
                            <Referer> http://www.*.com</Referer>
                            <Referer> https://www.?.aliyuncs.com</Referer>
                        </RefererList>
                    </RefererConfiguration>)";
    GetBucketRefererResult result(xml);
    EXPECT_EQ(result.AllowEmptyReferer(), true);
    EXPECT_EQ(result.RefererList().size(), 4UL);

}

TEST_F(BucketRefersSettingsTest, GetBucketRefererResultBranchTest)
{
    GetBucketRefererResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <Referer>
                    <AllowEmptyReferer>true</AllowEmptyReferer >
                        <RefererList>
                            <Referer> http://www.aliyun.com</Referer>
                            <Referer> https://www.aliyun.com</Referer>
                            <Referer> http://www.*.com</Referer>
                            <Referer> https://www.?.aliyuncs.com</Referer>
                        </RefererList>
                    </Referer>)";
    GetBucketRefererResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <RefererConfiguration>
                    </RefererConfiguration>)";
    GetBucketRefererResult result2(xml);


    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <RefererConfiguration>
                    <AllowEmptyReferer>true</AllowEmptyReferer >
                        <RefererList>
                        </RefererList>
                    </RefererConfiguration>)";
    GetBucketRefererResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <RefererConfiguration>
                    <AllowEmptyReferer></AllowEmptyReferer >
                        <RefererList>
                            <Referer> </Referer>
                            <Referer> </Referer>
                        </RefererList>
                    </RefererConfiguration>)";
    GetBucketRefererResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketRefererResult result5(xml);
}
}
}