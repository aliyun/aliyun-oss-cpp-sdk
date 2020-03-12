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

class ListLiveChannelTest : public ::testing::Test
{
protected:
    ListLiveChannelTest()
    {
    }

    ~ListLiveChannelTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-list-live-channel");
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

std::shared_ptr<OssClient> ListLiveChannelTest::Client = nullptr;
std::string ListLiveChannelTest::BucketName = "";

TEST_F(ListLiveChannelTest, ListLiveChannelListTest)
{
    ListLiveChannelRequest request(BucketName);
    auto listOutcome = Client->ListLiveChannel(request);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().LiveChannelList().size(), 0u);

    std::string channelName = "test_channel";
    PutLiveChannelRequest request2(BucketName, channelName, "HLS");
    auto putOutcome = Client->PutLiveChannel(request2);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    listOutcome = Client->ListLiveChannel(request);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().LiveChannelList().size(), 1u);
    std::vector<LiveChannelInfo> vec = listOutcome.result().LiveChannelList();
    EXPECT_EQ(vec[0].name, "test_channel");
    EXPECT_EQ(vec[0].description, "");
    EXPECT_EQ(vec[0].status, "enabled");
    EXPECT_EQ(vec[0].lastModified.empty(), false);
    EXPECT_EQ(vec[0].publishUrl.empty(), false);
    EXPECT_EQ(vec[0].playUrl.empty(), false);


    //
    ListLiveChannelRequest request3(BucketName);
    request3.setMaxKeys(1001u);
    listOutcome = Client->ListLiveChannel(request3);
    EXPECT_EQ(listOutcome.isSuccess(), false);
    EXPECT_EQ(listOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(listOutcome.error().Message(), "The Max Key param is invalid, it's default valus is 100, and smaller than 1000");

	//
	ListLiveChannelRequest request4(BucketName);
	request4.setMarker("/");
	request4.setPrefix("test_test");
	request4.setMaxKeys(999);
	listOutcome = Client->ListLiveChannel(request4);
	EXPECT_EQ(listOutcome.isSuccess(), true);
	vec = listOutcome.result().LiveChannelList();
	EXPECT_EQ(vec.size(), 0U);
}

TEST_F(ListLiveChannelTest, ListLiveChannelWithInvalidResponseBodyTest)
{
    ListLiveChannelRequest request(BucketName);
    request.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto listOutcome = Client->ListLiveChannel(request);
    EXPECT_EQ(listOutcome.isSuccess(), false);
    EXPECT_EQ(listOutcome.error().Code(), "GetLiveChannelStatError");
}

TEST_F(ListLiveChannelTest, ListLiveChannelResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListLiveChannelResult>
                            <Prefix></Prefix>
                            <Marker></Marker>
                            <MaxKeys>1</MaxKeys>
                            <IsTruncated>true</IsTruncated>
                            <NextMarker>channel-0</NextMarker>
                            <LiveChannel>
                                <Name>channel-0</Name>
                                <Description>test</Description>
                                <Status>disabled</Status>
                                <LastModified>2016-07-30T01:54:21.000Z</LastModified>
                                <PublishUrls>
                                    <Url>rtmp://test-bucket.oss-cn-hangzhou.aliyuncs.com/live/channel-0</Url>
                                </PublishUrls>
                                <PlayUrls>
                                    <Url>http://test-bucket.oss-cn-hangzhou.aliyuncs.com/channel-0/playlist.m3u8</Url>
                                </PlayUrls>
                            </LiveChannel>
                            <LiveChannel>
                                <Name>channel-0</Name>
                                <Description></Description>
                                <Status>disabled</Status>
                                <LastModified>2016-07-30T01:54:21.000Z</LastModified>
                                <PublishUrls>
                                    <Url>rtmp://test-bucket.oss-cn-hangzhou.aliyuncs.com/live/channel-0</Url>
                                </PublishUrls>
                                <PlayUrls>
                                    <Url>http://test-bucket.oss-cn-hangzhou.aliyuncs.com/channel-0/playlist.m3u8</Url>
                                </PlayUrls>
                            </LiveChannel>
                        </ListLiveChannelResult>)";

    ListLiveChannelResult result(xml);
    EXPECT_EQ(result.Prefix(), "");
    EXPECT_EQ(result.Marker(), "");
    EXPECT_EQ(result.MaxKeys(), 1u);
    EXPECT_EQ(result.IsTruncated(), true);
    EXPECT_EQ(result.NextMarker(), "channel-0");
    std::vector<LiveChannelInfo> vec = result.LiveChannelList();
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0].description, "test");
    EXPECT_EQ(vec[0].status, "disabled");
    EXPECT_EQ(vec[0].name, "channel-0");
    EXPECT_EQ(vec[0].lastModified, "2016-07-30T01:54:21.000Z");
    EXPECT_EQ(vec[0].publishUrl, "rtmp://test-bucket.oss-cn-hangzhou.aliyuncs.com/live/channel-0");
    EXPECT_EQ(vec[0].playUrl, "http://test-bucket.oss-cn-hangzhou.aliyuncs.com/channel-0/playlist.m3u8");
}

TEST_F(ListLiveChannelTest, ListLiveChannelResultBranchTest)
{
    ListLiveChannelResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListLiveChannel>
                        </ListLiveChannel>)";

    ListLiveChannelResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListLiveChannelResult>
                            
                        </ListLiveChannelResult>)";

    ListLiveChannelResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListLiveChannelResult>

                            <LiveChannel>
                                
                            </LiveChannel>
                            <LiveChannel>
                                
                            </LiveChannel>
                        </ListLiveChannelResult>)";

    ListLiveChannelResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListLiveChannelResult>
                            <Prefix></Prefix>
                            <Marker></Marker>
                            <MaxKeys></MaxKeys>
                            <IsTruncated></IsTruncated>
                            <NextMarker></NextMarker>
                            <LiveChannel>
                                <Name></Name>
                                <Description></Description>
                                <Status></Status>
                                <LastModified></LastModified>
                                <PublishUrls>
                                <Url></Url>
                                </PublishUrls>
                                <PlayUrls>
                                    <Url></Url>
                                </PlayUrls>
                            </LiveChannel>
                            <LiveChannel>
                                <Name></Name>
                                <Description></Description>
                                <Status></Status>
                                <LastModified></LastModified>
                                <PublishUrls>
                                    <Url></Url>
                                </PublishUrls>
                                <PlayUrls>
                                <Url></Url>
                                </PlayUrls>
                            </LiveChannel>
                        </ListLiveChannelResult>)";

    ListLiveChannelResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    ListLiveChannelResult result5(xml);

}
}
}
