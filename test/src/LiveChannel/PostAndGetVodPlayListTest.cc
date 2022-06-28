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
#include <time.h>
#include <gtest/gtest.h>
#include <alibabacloud/oss/OssClient.h>
#include "../Config.h"
#include "../Utils.h"

namespace AlibabaCloud
{
namespace OSS 
{

class PostAndGetVodPlayListTest : public ::testing::Test
{
protected:
    PostAndGetVodPlayListTest()
    {
    }

    ~PostAndGetVodPlayListTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-post-and-get-vod-play-list");
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

std::shared_ptr<OssClient> PostAndGetVodPlayListTest::Client = nullptr;
std::string PostAndGetVodPlayListTest::BucketName = "";

TEST_F(PostAndGetVodPlayListTest, PostVodPlayListTest)
{
    std::string channelName = "test-channel";
    std::string playList = "my_play_list.m3u8";
    std::string channelType = "HLS";

    uint64_t startTime = time(NULL);
    uint64_t endTime = startTime + 3600;

    PostVodPlaylistRequest request(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome = Client->PostVodPlaylist(request);
    EXPECT_EQ(postOutcome.isSuccess(), false);

    auto putOutcome = Client->PutLiveChannel(PutLiveChannelRequest(BucketName, channelName, channelType));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    PostVodPlaylistRequest request2(BucketName, channelName,
         playList, startTime, endTime);

    postOutcome = Client->PostVodPlaylist(request2);
    EXPECT_EQ(postOutcome.isSuccess(), false);

	PostVodPlaylistRequest request3(BucketName, channelName,
		playList, startTime, endTime);
	request3.setPlayList("another_play_list.m3u8");
	request3.setStartTime(startTime);
	request3.setEndTime(endTime);

	postOutcome = Client->PostVodPlaylist(request3);
	EXPECT_EQ(postOutcome.isSuccess(), false);
	EXPECT_TRUE(postOutcome.error().Code() != "ValidateError");

    auto getOutcome = Client->GetVodPlaylist(GetVodPlaylistRequest(BucketName, channelName, startTime, endTime));
    EXPECT_EQ(getOutcome.isSuccess(), false);

}

TEST_F(PostAndGetVodPlayListTest, PostVodPlayListErrorParamTest)
{
    std::string channelName = "test-channel";
    std::string playList = "my_play_list/.m3u8";

    uint64_t startTime = 1543978300;
    uint64_t endTime = 1543989000;

    PostVodPlaylistRequest request(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome = Client->PostVodPlaylist(request);
    EXPECT_EQ(postOutcome.isSuccess(), false);

    playList = "my_play_list.m3u8";
    channelName = "/test/channel/name";
    PostVodPlaylistRequest request2(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome2 = Client->PostVodPlaylist(request2);
    EXPECT_EQ(postOutcome2.isSuccess(), false);

    //
    channelName = "test_channel_name测试";
    PostVodPlaylistRequest request3(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome3 = Client->PostVodPlaylist(request3);
    EXPECT_EQ(postOutcome3.isSuccess(), false);

    //
    startTime = 0;
    PostVodPlaylistRequest request4(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome4 = Client->PostVodPlaylist(request4);
    EXPECT_EQ(postOutcome4.isSuccess(), false);

    //
    startTime = 1543978300;
    endTime = 1543978299;
    PostVodPlaylistRequest request5(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome5 = Client->PostVodPlaylist(request5);
    EXPECT_EQ(postOutcome5.isSuccess(), false);

    //
    startTime = 1543978300;
    endTime = 1544064701;
    PostVodPlaylistRequest request6(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome6 = Client->PostVodPlaylist(request6);
    EXPECT_EQ(postOutcome6.isSuccess(), false);

    //
    startTime = time(NULL);
    endTime = 0;
    PostVodPlaylistRequest request7(BucketName, channelName,
         playList, startTime, endTime);

    auto postOutcome7 = Client->PostVodPlaylist(request7);
    EXPECT_EQ(postOutcome7.isSuccess(), false);

    GetVodPlaylistRequest getRequest1(BucketName, channelName);
    getRequest1.setStartTime(0);
    getRequest1.setEndTime(0);

    auto getVodOutcome = Client->GetVodPlaylist(getRequest1);
    EXPECT_EQ(getVodOutcome.isSuccess(), false);

    GetVodPlaylistRequest getRequest2(BucketName, channelName);
    getRequest2.setStartTime(1000000);
    getRequest2.setEndTime(99999);

    getVodOutcome = Client->GetVodPlaylist(getRequest2);
    EXPECT_EQ(getVodOutcome.isSuccess(), false);

    GetVodPlaylistRequest getRequest3(BucketName, channelName);
	time_t tNow = time(nullptr);
	time_t tNext = tNow + 60 * 60 * 24 + 1;
    getRequest3.setStartTime(tNow);
    getRequest3.setEndTime(tNext);

    getVodOutcome = Client->GetVodPlaylist(getRequest3);
    EXPECT_EQ(getVodOutcome.isSuccess(), false);
	EXPECT_STREQ(getVodOutcome.error().Code().c_str(), "ValidateError");
}

TEST_F(PostAndGetVodPlayListTest, GetVodPlaylistResultTest)
{
    std::string xml = R"(#EXTM3U
                    #EXT-X-VERSION:3
                    #EXT-X-MEDIA-SEQUENCE:0
                    #EXT-X-TARGETDURATION:13
                    #EXTINF:7.120,
                    1543895706266.ts
                    #EXTINF:5.840,
                    1543895706323.ts
                    #EXTINF:6.400,
                    1543895706356.ts
                    #EXTINF:5.520,
                    1543895706389.ts
                    #EXTINF:5.240,
                    1543895706428.ts
                    #EXTINF:13.320,
                    1543895706468.ts
                    #EXTINF:5.960,
                    1543895706538.ts
                    #EXTINF:6.520,
                    1543895706561.ts
                    #EXT-X-ENDLIST)";
    GetVodPlaylistResult result(xml);
    EXPECT_EQ(result.PlaylistContent().empty(), false);

	std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>(xml);
    GetVodPlaylistResult result2(content);
    EXPECT_EQ(result2.PlaylistContent().empty(), false);
}

TEST_F(PostAndGetVodPlayListTest, GetVodPlaylistRequestFunctionTest)
{
    auto getOutcome = Client->GetVodPlaylist(GetVodPlaylistRequest("INVLAIDNAME", "test-channel", 1, 2));
}
}
}
