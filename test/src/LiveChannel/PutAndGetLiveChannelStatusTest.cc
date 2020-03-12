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

class PutAndGetLiveChannelStatusTest : public ::testing::Test
{
protected:
    PutAndGetLiveChannelStatusTest()
    {
    }

    ~PutAndGetLiveChannelStatusTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-delete-live-channel");
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

std::shared_ptr<OssClient> PutAndGetLiveChannelStatusTest::Client = nullptr;
std::string PutAndGetLiveChannelStatusTest::BucketName = "";

TEST_F(PutAndGetLiveChannelStatusTest, PutLiveChannelStatusAndGetTest)
{
    std::string channelName = "not_exist";
    PutLiveChannelStatusRequest request(BucketName, channelName, LiveChannelStatus::EnabledStatus);
    auto putOutcome = Client->PutLiveChannelStatus(request);
    EXPECT_EQ(putOutcome.isSuccess(), false);

    channelName = "new_channel";
    PutLiveChannelRequest request2(BucketName, channelName, "HLS");
    auto putOutcome2 = Client->PutLiveChannel(request2);
    EXPECT_EQ(putOutcome2.isSuccess(), true);

    PutLiveChannelStatusRequest request3(BucketName, channelName, LiveChannelStatus::DisabledStatus);
    auto putOutcome3 = Client->PutLiveChannelStatus(request3);
    EXPECT_EQ(putOutcome3.isSuccess(), true);

    GetLiveChannelStatRequest request4(BucketName, channelName);
    auto getStatOutcome = Client->GetLiveChannelStat(request4);
    EXPECT_EQ(getStatOutcome.isSuccess(), true);
    EXPECT_EQ(getStatOutcome.result().Status(), LiveChannelStatus::DisabledStatus);
    EXPECT_EQ(getStatOutcome.result().ConnectedTime(), "");
    EXPECT_EQ(getStatOutcome.result().RemoteAddr(), "");
    EXPECT_EQ(getStatOutcome.result().VideoBandWidth(), 0u);
    EXPECT_EQ(getStatOutcome.result().AudioBandWidth(), 0u);

	PutLiveChannelStatusRequest request5(BucketName, channelName);
	request5.setStatus(LiveChannelStatus::IdleStatus);
	auto putOutcome5 = Client->PutLiveChannelStatus(request5);
	EXPECT_EQ(putOutcome5.isSuccess(), false);

	PutLiveChannelStatusRequest request6(BucketName, channelName);
	request6.setStatus(LiveChannelStatus::EnabledStatus);
	auto putOutcome6 = Client->PutLiveChannelStatus(request6);
	EXPECT_EQ(putOutcome6.isSuccess(), true);

}

TEST_F(PutAndGetLiveChannelStatusTest, GetLiveChannelStatusWithInvalidResponseBodyTest)
{
    // case 1 default
    std::string channelName = "test-channel1";

    PutLiveChannelRequest request2(BucketName, channelName, "HLS");
    auto putOutcome2 = Client->PutLiveChannel(request2);
    EXPECT_EQ(putOutcome2.isSuccess(), true);

    PutLiveChannelStatusRequest request3(BucketName, channelName, LiveChannelStatus::DisabledStatus);
    auto putOutcome3 = Client->PutLiveChannelStatus(request3);
    EXPECT_EQ(putOutcome3.isSuccess(), true);

    GetLiveChannelStatRequest request4(BucketName, channelName);
    request4.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto getStatOutcome = Client->GetLiveChannelStat(request4);
    EXPECT_EQ(getStatOutcome.isSuccess(), false);
    EXPECT_EQ(getStatOutcome.error().Code(), "GetLiveChannelStatError");
}

TEST_F(PutAndGetLiveChannelStatusTest, GetLiveChannelResultTest)
{
    std::string xml1 = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <LiveChannelStat>
                            <Status>Idle</Status>
                        </LiveChannelStat>)";
    GetLiveChannelStatResult result(xml1);
    EXPECT_EQ(result.Status(), LiveChannelStatus::IdleStatus);
    EXPECT_EQ(result.ConnectedTime(), "");
    EXPECT_EQ(result.RemoteAddr(), "");

    std::string xml2 = R"(<?xml version="1.0" encoding="UTF-8"?>
                            <LiveChannelStat>
                                <Status>Live</Status>
                                <ConnectedTime>2016-08-25T06:25:15.000Z</ConnectedTime>
                                <RemoteAddr>10.1.2.3:47745</RemoteAddr>
                            <Video>
                                <Width>1280</Width>
                                <Height>536</Height>
                                <FrameRate>24</FrameRate>
                                <Bandwidth>0</Bandwidth>
                                <Codec>H264</Codec>
                            </Video>
                            <Audio>
                                <Bandwidth>0</Bandwidth>
                                <SampleRate>44100</SampleRate>
                                <Codec>ADPCM</Codec>
                            </Audio>
                            </LiveChannelStat>)";
    GetLiveChannelStatResult result2(xml2);
    EXPECT_EQ(result2.Status(), LiveChannelStatus::LiveStatus);
    EXPECT_EQ(result2.ConnectedTime(), "2016-08-25T06:25:15.000Z");
    EXPECT_EQ(result2.RemoteAddr(), "10.1.2.3:47745");
    EXPECT_EQ(result2.Width(), 1280u);
    EXPECT_EQ(result2.Height(), 536u);
    EXPECT_EQ(result2.VideoBandWidth(), 0u);
    EXPECT_EQ(result2.VideoCodec(), "H264");
    EXPECT_EQ(result2.AudioBandWidth(), 0u);
    EXPECT_EQ(result2.AudioCodec(), "ADPCM");
}

TEST_F(PutAndGetLiveChannelStatusTest, GetLiveChannelStatusInvalidBucketTest)
{
    std::string channelName = "not_exist";

    GetLiveChannelStatRequest request4("Invalid-bucket-test", channelName);
    auto getStatOutcome = Client->GetLiveChannelStat(request4);
    EXPECT_EQ(getStatOutcome.isSuccess(), false);
    EXPECT_EQ(getStatOutcome.error().Code(), "ValidateError");
}

TEST_F(PutAndGetLiveChannelStatusTest, GetLiveChannelStatResultFunctionTest)
{
    GetLiveChannelStatResult result("test");
    result.FrameRate();
    result.SampleRate();
}
TEST_F(PutAndGetLiveChannelStatusTest, PutLiveChannelStatusRequestValidateFailFunctionTest)
{
    PutLiveChannelStatusRequest request("INVLAIDNAME", "test2");
    auto putOutcome = Client->PutLiveChannelStatus(request);
}
TEST_F(PutAndGetLiveChannelStatusTest, GetLiveChannelResultBranchTest)
{
    GetLiveChannelStatResult result("test");

    std::string xml2 = R"(<?xml version="1.0" encoding="UTF-8"?>
                            <LiveChannel>
                                <Status>Live</Status>
                                <ConnectedTime>2016-08-25T06:25:15.000Z</ConnectedTime>
                                <RemoteAddr>10.1.2.3:47745</RemoteAddr>
                            <Video>
                                <Width>1280</Width>
                                <Height>536</Height>
                                <FrameRate>24</FrameRate>
                                <Bandwidth>0</Bandwidth>
                                <Codec>H264</Codec>
                            </Video>
                            <Audio>
                                <Bandwidth>0</Bandwidth>
                                <SampleRate>44100</SampleRate>
                                <Codec>ADPCM</Codec>
                            </Audio>
                            </LiveChannel>)";
    GetLiveChannelStatResult result2(xml2);

    xml2 = R"(<?xml version="1.0" encoding="UTF-8"?>
                            <LiveChannelStat>

                                <Width>1280</Width>
                                <Height>536</Height>
                                <FrameRate>24</FrameRate>
                                <Bandwidth>0</Bandwidth>
                                <Codec>H264</Codec>
                                <Bandwidth>0</Bandwidth>
                                <SampleRate>44100</SampleRate>
                                <Codec>ADPCM</Codec>
                            </LiveChannelStat>)";
    GetLiveChannelStatResult result3(xml2);

    xml2 = R"(<?xml version="1.0" encoding="UTF-8"?>
                            <LiveChannelStat>
                            <Video>

                            </Video>
                            <Audio>

                            </Audio>
                            </LiveChannelStat>)";
    GetLiveChannelStatResult result4(xml2);

    xml2 = R"(<?xml version="1.0" encoding="UTF-8"?>
                            <LiveChannelStat>
                                <Status></Status>
                                <ConnectedTime></ConnectedTime>
                                <RemoteAddr></RemoteAddr>
                            <Video>
                                <Width></Width>
                                <Height></Height>
                                <FrameRate></FrameRate>
                                <Bandwidth></Bandwidth>
                                <Codec></Codec>
                            </Video>
                            <Audio>
                                <Bandwidth></Bandwidth>
                                <SampleRate></SampleRate>
                                <Codec></Codec>
                            </Audio>
                            </LiveChannelStat>)";
    GetLiveChannelStatResult result5(xml2);

    xml2 = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetLiveChannelStatResult result6(xml2);

}

}
}
