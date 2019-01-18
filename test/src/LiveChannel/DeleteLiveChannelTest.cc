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

class DeleteLiveChannelTest : public ::testing::Test
{
protected:
    DeleteLiveChannelTest()
    {
    }

    ~DeleteLiveChannelTest() override
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

std::shared_ptr<OssClient> DeleteLiveChannelTest::Client = nullptr;
std::string DeleteLiveChannelTest::BucketName = "";

TEST_F(DeleteLiveChannelTest, DeleteLiveChannelErrorParam)
{
    std::string channelName = "not_exist";
    DeleteLiveChannelRequest request(BucketName, channelName);
    auto deleteOutcome = Client->DeleteLiveChannel(request);
    // with http code 204 No Content
    EXPECT_EQ(deleteOutcome.isSuccess(), true);

    DeleteLiveChannelRequest request2(BucketName, channelName+"/test");
    deleteOutcome = Client->DeleteLiveChannel(request2);
    EXPECT_EQ(deleteOutcome.isSuccess(), false);
}

}
}
