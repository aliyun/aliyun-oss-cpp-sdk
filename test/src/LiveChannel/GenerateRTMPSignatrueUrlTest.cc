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

class GenerateRTMPSignatureUrlTest : public ::testing::Test
{
protected:
    GenerateRTMPSignatureUrlTest()
    {
    }

    ~GenerateRTMPSignatureUrlTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-delete-live-channel");
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
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

std::shared_ptr<OssClient> GenerateRTMPSignatureUrlTest::Client = nullptr;
std::string GenerateRTMPSignatureUrlTest::BucketName = "";

TEST_F(GenerateRTMPSignatureUrlTest, GenerateRTMPSignatureUrlUT)
{
    std::string channelName = "not_exist";
    GenerateRTMPSignedUrlRequest request(BucketName, channelName, "", 0);
    
    EXPECT_EQ(request.PlayList(), "");
    EXPECT_EQ(request.Expires(), 0U);
    
	request.setPlayList("test.m3u8");
    EXPECT_EQ(request.PlayList(), "test.m3u8");

	time_t tExpire = time(nullptr) + 15 * 60;
	request.setExpires(tExpire);
    EXPECT_EQ(request.Expires(), (uint64_t)tExpire);

    auto generateOutcome = Client->GenerateRTMPSignedUrl(request);
    EXPECT_EQ(generateOutcome.isSuccess(), true);
    EXPECT_EQ(generateOutcome.result().empty(), false);

	std::string signedURL = generateOutcome.result();
	EXPECT_TRUE(signedURL.find("playlistName") != std::string::npos);
}

TEST_F(GenerateRTMPSignatureUrlTest, GenerateRTMPSignatureUrlInvalidBucketTest)
{
    GenerateRTMPSignedUrlRequest request("Invalid-bucket-test", "channel-name", "playlist.m3u8", 1000);
    auto generateOutcome = Client->GenerateRTMPSignedUrl(request);
    EXPECT_EQ(generateOutcome.isSuccess(), false);
    EXPECT_EQ(generateOutcome.error().Code(), "ValidateError");

    //channel invalid
    request.setBucket(BucketName);
    request.setChannelName("");
    request.setPlayList("playlist.m3u8");
    request.setExpires(1000);
    generateOutcome = Client->GenerateRTMPSignedUrl(request);
    EXPECT_EQ(generateOutcome.isSuccess(), false);
    EXPECT_EQ(generateOutcome.error().Code(), "ValidateError");

    request.setBucket(BucketName);
    request.setChannelName("chanelname");
    request.setPlayList("");
    request.setExpires(1000);
    generateOutcome = Client->GenerateRTMPSignedUrl(request);
    EXPECT_EQ(generateOutcome.isSuccess(), false);
    EXPECT_EQ(generateOutcome.error().Code(), "ValidateError");

    request.setBucket(BucketName);
    request.setChannelName("chanelname");
    request.setPlayList("playlist.m3u8");
    request.setExpires(0);
    generateOutcome = Client->GenerateRTMPSignedUrl(request);
    EXPECT_EQ(generateOutcome.isSuccess(), false);
    EXPECT_EQ(generateOutcome.error().Code(), "ValidateError");
}

TEST_F(GenerateRTMPSignatureUrlTest, GenerateRTMPSignedUrlRequestBranchTest)
{
    std::string str;
    GenerateRTMPSignedUrlRequest request("INVALIDNAME", "test", str, 0);
    Client->GenerateRTMPSignedUrl(request);
}
}
}
