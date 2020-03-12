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

class GetLiveChannelHistoryTest : public ::testing::Test
{
protected:
    GetLiveChannelHistoryTest()
    {
    }

    ~GetLiveChannelHistoryTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-get-live-channel-history");
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

std::shared_ptr<OssClient> GetLiveChannelHistoryTest::Client = nullptr;
std::string GetLiveChannelHistoryTest::BucketName = "";

TEST_F(GetLiveChannelHistoryTest, GetLiveChannelHistoryGetTest)
{
    std::string channelName = "test-channel";
    PutLiveChannelRequest request(BucketName, channelName, "HLS");
    auto putOutcome = Client->PutLiveChannel(request);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GetLiveChannelHistoryRequest request2(BucketName, channelName);
    auto getOutcome = Client->GetLiveChannelHistory(request2);
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().LiveRecordList().size(), 0u);

    GetLiveChannelHistoryRequest request3(BucketName, "not_exist");
    auto getOutcome2 = Client->GetLiveChannelHistory(request3);
    EXPECT_EQ(getOutcome2.isSuccess(), false);
    EXPECT_EQ(getOutcome2.result().LiveRecordList().size(), 0u);
}

TEST_F(GetLiveChannelHistoryTest, GetLiveChannelHistoryWithInvalidResponseBodyTest)
{
    std::string channelName = "test-channel1";
    PutLiveChannelRequest request(BucketName, channelName, "HLS");
    auto putOutcome = Client->PutLiveChannel(request);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GetLiveChannelHistoryRequest request2(BucketName, channelName);
    request2.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto getOutcome = Client->GetLiveChannelHistory(request2);
    EXPECT_EQ(getOutcome.isSuccess(), false);
    EXPECT_EQ(getOutcome.error().Code(), "GetLiveChannelStatError");
}

TEST_F(GetLiveChannelHistoryTest, GetLiveChannelHistoryResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
							          <LiveChannelHistory>
							            <LiveRecord>
								          <StartTime>2016-07-30T01:53:21.000Z</StartTime>
								          <EndTime>2016-07-30T01:53:31.000Z</EndTime>
								          <RemoteAddr>10.101.194.148:56861</RemoteAddr>
							          </LiveRecord>
							          <LiveRecord>
								          <StartTime>2016-07-30T01:53:35.000Z</StartTime>
								          <EndTime>2016-07-30T01:53:45.000Z</EndTime>
								          <RemoteAddr>10.101.194.148:57126</RemoteAddr>
							          </LiveRecord>
							          <LiveRecord>
								          <StartTime>2016-07-30T01:53:49.000Z</StartTime>
								          <EndTime>2016-07-30T01:53:59.000Z</EndTime>
								          <RemoteAddr>10.101.194.148:57577</RemoteAddr>
							          </LiveRecord>
							          <LiveRecord>
								          <StartTime>2016-07-30T01:54:04.000Z</StartTime>
								          <EndTime>2016-07-30T01:54:14.000Z</EndTime>
								          <RemoteAddr>10.101.194.148:57632</RemoteAddr>
							          </LiveRecord>
							        </LiveChannelHistory>)";
    GetLiveChannelHistoryResult result(xml);
    std::vector<LiveRecord>vec = result.LiveRecordList();
    EXPECT_EQ(vec.size(), 4u);
    EXPECT_EQ(vec[0].startTime, "2016-07-30T01:53:21.000Z");
    EXPECT_EQ(vec[0].endTime, "2016-07-30T01:53:31.000Z");
    EXPECT_EQ(vec[0].remoteAddr, "10.101.194.148:56861");

    EXPECT_EQ(vec[1].startTime, "2016-07-30T01:53:35.000Z");
    EXPECT_EQ(vec[1].endTime, "2016-07-30T01:53:45.000Z");
    EXPECT_EQ(vec[1].remoteAddr, "10.101.194.148:57126");

    EXPECT_EQ(vec[2].startTime, "2016-07-30T01:53:49.000Z");
    EXPECT_EQ(vec[2].endTime, "2016-07-30T01:53:59.000Z");
    EXPECT_EQ(vec[2].remoteAddr, "10.101.194.148:57577");

    EXPECT_EQ(vec[3].startTime, "2016-07-30T01:54:04.000Z");
    EXPECT_EQ(vec[3].endTime, "2016-07-30T01:54:14.000Z");
    EXPECT_EQ(vec[3].remoteAddr, "10.101.194.148:57632");

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
							          <LiveChannelHistory>
							        </LiveChannelHistory>)";
    GetLiveChannelHistoryResult result2(xml);
    EXPECT_EQ(result2.LiveRecordList().empty(), true);
}

TEST_F(GetLiveChannelHistoryTest, GetLiveChannelHistoryResultBranchTest)
{
    GetLiveChannelHistoryResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
							          <LiveChannel>
							            <LiveRecord>
								          <StartTime>2016-07-30T01:53:21.000Z</StartTime>
								          <EndTime>2016-07-30T01:53:31.000Z</EndTime>
								          <RemoteAddr>10.101.194.148:56861</RemoteAddr>
							          </LiveRecord>
							        </LiveChannel>)";
    GetLiveChannelHistoryResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
							          <LiveChannelHistory>
							        </LiveChannelHistory>)";
    GetLiveChannelHistoryResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
							          <LiveChannelHistory>
							            <LiveRecord>
							          </LiveRecord>
							        </LiveChannelHistory>)";
    GetLiveChannelHistoryResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
							          <LiveChannelHistory>
							            <LiveRecord>
								          <StartTime></StartTime>
								          <EndTime></EndTime>
								          <RemoteAddr></RemoteAddr>
							          </LiveRecord>
							        </LiveChannelHistory>)";
    GetLiveChannelHistoryResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetLiveChannelHistoryResult result5(xml);

}
}
}
