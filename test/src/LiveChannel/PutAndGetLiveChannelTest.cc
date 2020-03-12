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

class PutAndGetLiveChannelTest : public ::testing::Test
{
protected:
    PutAndGetLiveChannelTest()
    {
    }

    ~PutAndGetLiveChannelTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-put-and-get-live-channel");
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

std::shared_ptr<OssClient> PutAndGetLiveChannelTest::Client = nullptr;
std::string PutAndGetLiveChannelTest::BucketName = "";

TEST_F(PutAndGetLiveChannelTest, PutAndGetLiveChannelInfoTest)
{
    // case 1 default
    std::string channelName = "test-channel";
    std::string channelType = "HLS";

    auto putOutcome = Client->PutLiveChannel(PutLiveChannelRequest(BucketName, channelName, channelType));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    auto getOutcome = Client->GetLiveChannelInfo(GetLiveChannelInfoRequest(BucketName, channelName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().Type(), "HLS");
    EXPECT_EQ(getOutcome.result().FragDuration(), 5u);
    EXPECT_EQ(getOutcome.result().FragCount(), 3u);
    EXPECT_EQ(getOutcome.result().Status(), LiveChannelStatus::EnabledStatus);
    EXPECT_EQ(getOutcome.result().Description(), "");
    EXPECT_EQ(getOutcome.result().PlaylistName(), "playlist.m3u8");

    // case 2 no snapshot
    std::string channelName2 = channelName;
    PutLiveChannelRequest request(BucketName, channelName2, "HLS");
    request.setChannelName(channelName2+"-test");
    request.setDescripition("just_a_test");
    request.setFragDuration(25u);
    request.setFragCount(30u);
    request.setStatus(LiveChannelStatus::DisabledStatus);
    request.setPlayListName("test_play-list.m3u8");
    auto putOutcome2 = Client->PutLiveChannel(request);
    EXPECT_EQ(putOutcome2.isSuccess(), true);

    auto getOutcome2 = Client->GetLiveChannelInfo(GetLiveChannelInfoRequest(BucketName, channelName2+"-test"));
    EXPECT_EQ(getOutcome2.isSuccess(), true);
    EXPECT_EQ(getOutcome2.result().Type(), "HLS");
    EXPECT_EQ(getOutcome2.result().FragDuration(), 25u);
    EXPECT_EQ(getOutcome2.result().FragCount(), 30u);
    EXPECT_EQ(getOutcome2.result().Status(), LiveChannelStatus::DisabledStatus);
    EXPECT_EQ(getOutcome2.result().Description(), "just_a_test");
    EXPECT_EQ(getOutcome2.result().PlaylistName(), "test_play-list.m3u8");

    std::string channelName3 = "test_channel_name测试";
    PutLiveChannelRequest request2(BucketName, channelName2, "HLS");
    auto putOutcome3 = Client->PutLiveChannel(request2);
    EXPECT_EQ(putOutcome3.isSuccess(), true);
    EXPECT_EQ(putOutcome3.result().PublishUrl().empty(), false);
    EXPECT_EQ(putOutcome3.result().PlayUrl().empty(), false);

    // case 3 snapshot partial
	PutLiveChannelRequest request3(BucketName, channelName2+"-test2", "HLS");
	request3.setChannelType("HLS");
	request3.setDestBucket("not_exist_bucket");
	putOutcome3 = Client->PutLiveChannel(request3);
	EXPECT_EQ(putOutcome3.isSuccess(), false);
	EXPECT_STREQ(putOutcome3.error().Code().c_str(), "ValidateError");

	// case 4 snapshot full
	PutLiveChannelRequest request4(BucketName, channelName2 + "-test3", "HLS");
	request4.setChannelType("HLS");
	request4.setDestBucket("not_exist_bucket");
	request4.setInterval(6);
	request4.setNotifyTopic("not_exist_topic");
	request4.setRoleName("not_exist_role_name");
	putOutcome3 = Client->PutLiveChannel(request4);
	EXPECT_EQ(putOutcome3.isSuccess(), false);

    // clear env
    auto deleteOutcome1 = Client->DeleteLiveChannel(DeleteLiveChannelRequest(BucketName, channelName));
    EXPECT_EQ(deleteOutcome1.isSuccess(), true);

    auto deleteOutcome2 = Client->DeleteLiveChannel(DeleteLiveChannelRequest(BucketName, channelName2+"-test"));
    EXPECT_EQ(deleteOutcome2.isSuccess(), true);
}

TEST_F(PutAndGetLiveChannelTest, PutLiveChannelErrorParamTest)
{
    std::string channelName;
    channelName.assign(1024, 'a');
    EXPECT_EQ(channelName.size(), 1024u);

    auto putOutcome = Client->PutLiveChannel(PutLiveChannelRequest(BucketName, channelName, "HLS"));
    EXPECT_EQ(putOutcome.isSuccess(), false);
    EXPECT_EQ(putOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(putOutcome.error().Message(), "The channelName param is invalid, it shouldn't contain '/' and length < 1023");

    //
    channelName.clear();
    channelName = "/test";
    EXPECT_EQ(channelName, "/test");

    auto putOutcome2 = Client->PutLiveChannel(PutLiveChannelRequest(BucketName, channelName, "HLS"));
    EXPECT_EQ(putOutcome2.isSuccess(), false);
    EXPECT_EQ(putOutcome2.error().Code(), "ValidateError");
    EXPECT_EQ(putOutcome2.error().Message(), "The channelName param is invalid, it shouldn't contain '/' and length < 1023");

    //
    channelName.clear();
    channelName = "/test/test/test/test/aaa";
    EXPECT_EQ(channelName, "/test/test/test/test/aaa");

    auto putOutcome3 = Client->PutLiveChannel(PutLiveChannelRequest(BucketName, channelName, "HLS"));
    EXPECT_EQ(putOutcome3.isSuccess(), false);
    EXPECT_EQ(putOutcome3.error().Code(), "ValidateError");
    EXPECT_EQ(putOutcome3.error().Message(), "The channelName param is invalid, it shouldn't contain '/' and length < 1023");


    //
    channelName.clear();
    channelName = "测试通过";
    EXPECT_EQ(channelName, "测试通过");

    auto putOutcome4 = Client->PutLiveChannel(PutLiveChannelRequest(BucketName,
         channelName, "HLS"));
    EXPECT_EQ(putOutcome4.isSuccess(), true);

    auto getOutcome = Client->GetLiveChannelInfo(GetLiveChannelInfoRequest(
        BucketName, channelName));
    EXPECT_EQ(getOutcome.isSuccess(), true);

    auto deleteOutcome = Client->DeleteLiveChannel(DeleteLiveChannelRequest(
        BucketName, channelName));
    EXPECT_EQ(deleteOutcome.isSuccess(), true);

    //
    channelName.clear();
    channelName = "test-channel";
    EXPECT_EQ(channelName, "test-channel");

    putOutcome = Client->PutLiveChannel(PutLiveChannelRequest(BucketName,
         channelName, "HLs"));
    EXPECT_EQ(putOutcome.isSuccess(), false);

    putOutcome = Client->PutLiveChannel(PutLiveChannelRequest(BucketName,
         channelName, "test"));
    EXPECT_EQ(putOutcome.isSuccess(), false);

    //
    PutLiveChannelRequest request1(BucketName, channelName, "HLS");
    request1.setFragDuration(101u);
    putOutcome = Client->PutLiveChannel(request1);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    PutLiveChannelRequest request2(BucketName, channelName, "HLS");
    request2.setFragDuration(0u);
    putOutcome = Client->PutLiveChannel(request2);
    EXPECT_EQ(putOutcome.isSuccess(), false);
    EXPECT_EQ(putOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(putOutcome.error().Message(), "The live channel frag duration param is invalid, it should be [1,100]");

    //
    PutLiveChannelRequest request3(BucketName, channelName, "HLS");
    request3.setFragCount(1011111u);
    putOutcome = Client->PutLiveChannel(request1);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    PutLiveChannelRequest request4(BucketName, channelName, "HLS");
    request4.setFragCount(0u);
    putOutcome = Client->PutLiveChannel(request4);
    EXPECT_EQ(putOutcome.isSuccess(), false);
    
    //
    PutLiveChannelRequest request5(BucketName, channelName, "HLS");
    request5.setPlayListName("myplay.m3");
    putOutcome = Client->PutLiveChannel(request5);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    PutLiveChannelRequest request6(BucketName, channelName, "HLS");
    request6.setPlayListName("myplay.M3U8");
    putOutcome = Client->PutLiveChannel(request6);
    EXPECT_EQ(putOutcome.isSuccess(), true);

	PutLiveChannelRequest request7(BucketName, channelName, "HLS");
	request7.setStatus(LiveChannelStatus::LiveStatus);
	putOutcome = Client->PutLiveChannel(request7);
	EXPECT_EQ(putOutcome.isSuccess(), false);

	PutLiveChannelRequest request8(BucketName, channelName, "HLS");
	std::string too_long;
	too_long.assign(129, 'a');
	request8.setDescripition(too_long);
	putOutcome = Client->PutLiveChannel(request8);
	EXPECT_EQ(putOutcome.isSuccess(), false);

    getOutcome = Client->GetLiveChannelInfo(GetLiveChannelInfoRequest(
        BucketName, channelName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().PlaylistName(), "myplay.m3u8");

    auto deleteOutcome2 = Client->DeleteLiveChannel(DeleteLiveChannelRequest(
        BucketName, channelName));
    EXPECT_EQ(deleteOutcome.isSuccess(), true);

    request6.setPlayListName("myplay..M3U8");
    putOutcome = Client->PutLiveChannel(request6);
    EXPECT_EQ(putOutcome.isSuccess(), false);
    EXPECT_EQ(putOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(putOutcome.error().Message(), "The live channel play list param is invalid, it should end with '.m3u8' & length in [6,128]");

    request6.setPlayListName("..M3U8");
    putOutcome = Client->PutLiveChannel(request6);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    request6.setPlayListName("..m3u8");
    putOutcome = Client->PutLiveChannel(request6);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    request6.setPlayListName("a.b..m3u8");
    putOutcome = Client->PutLiveChannel(request6);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    request6.setPlayListName("a.b..m3u");
    putOutcome = Client->PutLiveChannel(request6);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    request6.setPlayListName("a.b.m3u8");
    putOutcome = Client->PutLiveChannel(request6);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    getOutcome = Client->GetLiveChannelInfo(GetLiveChannelInfoRequest(
        BucketName, channelName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().PlaylistName(), "a.b.m3u8");

    deleteOutcome2 = Client->DeleteLiveChannel(DeleteLiveChannelRequest(
        BucketName, channelName));
    EXPECT_EQ(deleteOutcome2.isSuccess(), true);

    std::string longStr(1028, 'a');
    longStr.append(".m3u8");
    PutLiveChannelRequest request9(BucketName, channelName, "HLS");
    request9.setPlayListName(longStr);
    putOutcome = Client->PutLiveChannel(request9);
    EXPECT_EQ(putOutcome.isSuccess(), false);
}

TEST_F(PutAndGetLiveChannelTest, PutAndGetLiveChannelResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CreateLiveChannelResult>
                            <PublishUrls>
                                <Url>rtmp://test-bucket.oss-cn-hangzhou.aliyuncs.com/live/test-channel</Url>
                            </PublishUrls>
                            <PlayUrls>
                                <Url>http://test-bucket.oss-cn-hangzhou.aliyuncs.com/test-channel/playlist.m3u8</Url>
                            </PlayUrls>
                        </CreateLiveChannelResult>)";
    PutLiveChannelResult result(xml);
    EXPECT_EQ(result.PublishUrl(), "rtmp://test-bucket.oss-cn-hangzhou.aliyuncs.com/live/test-channel");
    EXPECT_EQ(result.PlayUrl(), "http://test-bucket.oss-cn-hangzhou.aliyuncs.com/test-channel/playlist.m3u8");
}

TEST_F(PutAndGetLiveChannelTest, GetLiveChannelInfoInvalidBucketTest)
{
    std::string channelName;
    channelName.assign(1024, 'a');

    auto getOutcome = Client->GetLiveChannelInfo(GetLiveChannelInfoRequest(
        "Invalid-bucket-test", channelName));
    EXPECT_EQ(getOutcome.isSuccess(), false);
    EXPECT_EQ(getOutcome.error().Code(), "ValidateError");
}

TEST_F(PutAndGetLiveChannelTest, LiveChannelRequestTest)
{
    std::string channelName;
    channelName.assign(1024, 'a');

    GetLiveChannelInfoRequest request("bucket", channelName);
    request.setBucket("bucket");
    auto getOutcome = Client->GetLiveChannelInfo(request);
}

TEST_F(PutAndGetLiveChannelTest, PutLiveChannelWithInvalidResponseBodyTest)
{
    // case 1 default
    std::string channelName = "test-channel-1";
    std::string channelType = "HLS";

    PutLiveChannelRequest plcRequest(BucketName, channelName, channelType);
    plcRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto putOutcome = Client->PutLiveChannel(plcRequest);
    EXPECT_EQ(putOutcome.isSuccess(), false);
    EXPECT_EQ(putOutcome.error().Code(), "PutLiveChannelError");

    GetLiveChannelInfoRequest glcRequest(BucketName, channelName);
    glcRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto glcOutcome = Client->GetLiveChannelInfo(glcRequest);
    EXPECT_EQ(glcOutcome.isSuccess(), false);
    EXPECT_EQ(glcOutcome.error().Code(), "GetLiveChannelStatError");
}

TEST_F(PutAndGetLiveChannelTest, PutLiveChannelResultFunctionTest)
{

    PutLiveChannelResult result("test");
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CreateLiveChannel>
                            <PublishUrls>
                                <Url>rtmp://test-bucket.oss-cn-hangzhou.aliyuncs.com/live/test-channel</Url>
                            </PublishUrls>
                            <PlayUrls>
                                <Url>http://test-bucket.oss-cn-hangzhou.aliyuncs.com/test-channel/playlist.m3u8</Url>
                            </PlayUrls>
                        </CreateLiveChannel>)";
    PutLiveChannelResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CreateLiveChannelResult>
                                <Url>rtmp://test-bucket.oss-cn-hangzhou.aliyuncs.com/live/test-channel</Url>
                                <Url>http://test-bucket.oss-cn-hangzhou.aliyuncs.com/test-channel/playlist.m3u8</Url>
                        </CreateLiveChannelResult>)";
    PutLiveChannelResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CreateLiveChannelResult>
                            <PublishUrls>
                            </PublishUrls>
                            <PlayUrls>
                            </PlayUrls>
                        </CreateLiveChannelResult>)";
    PutLiveChannelResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CreateLiveChannel>
                            <PublishUrls>
                                <Url></Url>
                            </PublishUrls>
                            <PlayUrls>
                                <Url></Url>
                            </PlayUrls>
                        </CreateLiveChannel>)";
    PutLiveChannelResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    PutLiveChannelResult result5(xml);
}

TEST_F(PutAndGetLiveChannelTest, GetLiveChannelInfoResultBranchTest)
{
    GetLiveChannelInfoResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <LiveChannel>

                        </LiveChannel>)";
    GetLiveChannelInfoResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <LiveChannelConfiguration>

                        </LiveChannelConfiguration>)";
    GetLiveChannelInfoResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <LiveChannelConfiguration>

                            <Target>

                            </Target>

                        </LiveChannelConfiguration>)";
    GetLiveChannelInfoResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <LiveChannelConfiguration>
                                <Description></Description>
                                <Status></Status>
                            <Target>
                                <Type></Type>
                                <FragDuration></FragDuration>
                                <FragCount></FragCount>
                                <PlaylistName></PlaylistName>
                            </Target>
                        </LiveChannelConfiguration>)";
    GetLiveChannelInfoResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetLiveChannelInfoResult result5(xml);
}
}
}
