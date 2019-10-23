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
#include <fstream>

namespace AlibabaCloud {
namespace OSS {

class ObjectEncodingTypeTest : public ::testing::Test {
protected:
    ObjectEncodingTypeTest()
    {
    }

    ~ObjectEncodingTypeTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectencodingtype");
        Client->CreateBucket(CreateBucketRequest(BucketName));
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

std::shared_ptr<OssClient> ObjectEncodingTypeTest::Client = nullptr;
std::string ObjectEncodingTypeTest::BucketName = "";


TEST_F(ObjectEncodingTypeTest, DeleteObjectsWithHiddenCharacters)
{
    std::string key = TestUtils::GetObjectKey("DeleteObjectsWithHiddenCharacters");

    std::string newKey1 = key;
    newKey1.push_back(0x1c); newKey1.push_back(0x1a); newKey1.append(".1.cd");

    std::string newKey2 = key;
    newKey2.push_back(0x1c); newKey2.push_back(0x1a); newKey2.append(".2.cd");

    std::string newKey3 = key;
    newKey3.append(".3.cd");

    std::string src = TestUtils::GetRandomString(1024);

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>(src);
    auto outcome = Client->PutObject(BucketName, newKey1, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    content = std::make_shared<std::stringstream>(src);
    outcome = Client->PutObject(BucketName, newKey2, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    content = std::make_shared<std::stringstream>(src);
    outcome = Client->PutObject(BucketName, newKey3, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    DeletedKeyList keyList;
    keyList.push_back(newKey1);
    keyList.push_back(newKey2);
    keyList.push_back(newKey3);
    auto dOutcome = Client->DeleteObjects(BucketName, keyList);
    EXPECT_EQ(dOutcome.isSuccess(), true);
}

TEST_F(ObjectEncodingTypeTest, DeleteObjectsWithHiddenCharactersUseUrlEncoding)
{
    std::string key = TestUtils::GetObjectKey("DeleteObjectsWithHiddenCharactersUseUrlEncoding");

    std::string newKey1 = key;
    newKey1.push_back(0x1c); newKey1.push_back(0x1a); newKey1.append(".1.cd");

    std::string newKey2 = key;
    newKey2.push_back(0x1c); newKey2.push_back(0x1a); newKey2.append(".2.cd");

    std::string newKey3 = key;
    newKey3.append(".3.cd");

    std::string src = TestUtils::GetRandomString(1024);

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>(src);
    auto outcome = Client->PutObject(BucketName, newKey1, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    content = std::make_shared<std::stringstream>(src);
    outcome = Client->PutObject(BucketName, newKey2, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    content = std::make_shared<std::stringstream>(src);
    outcome = Client->PutObject(BucketName, newKey3, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto lsOutcome = Client->ListObjects(BucketName, key);
    EXPECT_EQ(lsOutcome.isSuccess(), true);
    EXPECT_EQ(lsOutcome.result().ObjectSummarys().size(), 3UL);

    DeletedKeyList keyList;
    keyList.push_back(newKey1);
    keyList.push_back(newKey2);
    keyList.push_back(newKey3);
    DeleteObjectsRequest request(BucketName);
    request.setEncodingType("url");
    request.setKeyList(keyList);
    auto dOutcome = Client->DeleteObjects(request);
    EXPECT_EQ(dOutcome.isSuccess(), true);

    std::list<std::string> patList;
    patList.push_back(newKey1);
    patList.push_back(newKey2);
    patList.push_back(newKey3);
    EXPECT_EQ(dOutcome.result().keyList(), patList);

    lsOutcome = Client->ListObjects(BucketName, key);
    EXPECT_EQ(lsOutcome.isSuccess(), true);
    EXPECT_EQ(lsOutcome.result().ObjectSummarys().size(), 0UL);
}

TEST_F(ObjectEncodingTypeTest, DeleteObjectsWithEscapeCharacters)
{
    std::string keyPrefix = TestUtils::GetObjectKey("DeleteObjectsWithEscapeCharacters");
    char entities[] = { '\"', '&', '\'', '<', '>' };

    std::vector<std::string> keys;

    for (size_t i = 0; i < sizeof(entities) / sizeof(entities[0]); i++) {
        std::string key = keyPrefix;
        key.append("-").append(std::to_string(i));
        key.push_back(entities[i]);
        key.append(".dat");
        keys.push_back(key);
        auto outcome = Client->PutObject(BucketName, key, std::make_shared<std::stringstream>("just for test."));
        EXPECT_EQ(outcome.isSuccess(), true);
    }
    {
    std::string key = keyPrefix;
    key.append("-").append(std::to_string(9));
    key.append("\"&\'<>-10.dat");
    keys.push_back(key);
    auto outcome = Client->PutObject(BucketName, key, std::make_shared<std::stringstream>("just for test."));
    EXPECT_EQ(outcome.isSuccess(), true);
    }

    EXPECT_EQ(keys.size(), 6UL);

    auto lsOutcome = Client->ListObjects(BucketName, keyPrefix);
    EXPECT_EQ(lsOutcome.isSuccess(), true);
    EXPECT_EQ(lsOutcome.result().ObjectSummarys().size(), keys.size());

    DeletedKeyList keyList;
    std::list<std::string> patList;
    for (const auto& key : keys) {
        keyList.push_back(key);
        patList.push_back(key);
    }
    auto dOutcome = Client->DeleteObjects(BucketName, keyList);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().keyList(), patList);

    lsOutcome = Client->ListObjects(BucketName, keyPrefix);
    EXPECT_EQ(lsOutcome.isSuccess(), true);
    EXPECT_EQ(lsOutcome.result().ObjectSummarys().size(), 0UL);
}

}
}