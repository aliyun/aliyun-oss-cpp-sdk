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
#include <string.h>

namespace AlibabaCloud {
namespace OSS {

class ObjectProcessTest : public ::testing::Test {
protected:
    ObjectProcessTest()
    {
    }

    ~ObjectProcessTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectprocess");
        ImageFilePath = Config::GetDataPath();
        ImageFilePath.append("example.jpg");
        Process   = "image/resize,m_fixed,w_100,h_100";
        ImageInfo = "{\n    \"FileSize\": {\"value\": \"3267\"},\n    \"Format\": {\"value\": \"jpg\"},\n    \"ImageHeight\": {\"value\": \"100\"},\n    \"ImageWidth\": {\"value\": \"100\"},\n    \"ResolutionUnit\": {\"value\": \"1\"},\n    \"XResolution\": {\"value\": \"1/1\"},\n    \"YResolution\": {\"value\": \"1/1\"}}";

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

    static std::string GetOssImageObjectInfo(const std::string &bucket, const std::string &key)
    {
        auto outcome = Client->GetObject(GetObjectRequest(bucket, key, "image/info"));

        if (outcome.isSuccess()) {
            std::istreambuf_iterator<char> isb(*outcome.result().Content().get()), end;
            return std::string(isb, end);
        }
        return "";
    }


public:
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;
    static std::string ImageFilePath;
    static std::string Process;
    static std::string ImageInfo;

};

std::shared_ptr<OssClient> ObjectProcessTest::Client = nullptr;
std::string ObjectProcessTest::BucketName = "";
std::string ObjectProcessTest::ImageFilePath = "";
std::string ObjectProcessTest::Process = "";
std::string ObjectProcessTest::ImageInfo = "";

static bool CompareImageInfo(const std::string &src, const std::string &dst)
{
    if (src == dst) {
        return true;
    }
    //the filesize maybe not equal 
    const char * srcPtr = strstr(src.c_str(), "Format");
    const char * dstPtr = strstr(dst.c_str(), "Format");
    return strcmp(srcPtr, dstPtr) == 0 ? true : false;
}

TEST_F(ObjectProcessTest, ImageProcessTest)
{
    std::string key = TestUtils::GetObjectKey("cpp-sdk-objectprocess");
    key.append("-noraml.jpg");
    auto pOutcome = Client->PutObject(BucketName, key, ImageFilePath);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest request(BucketName, key, Process);
    auto gOutcome = Client->GetObject(request);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    if (gOutcome.isSuccess()) {
        std::string key1 = TestUtils::GetObjectKey("cpp-sdk-objectprocess");
        key1.append("-precessed.jpg");
        auto outcome = Client->PutObject(BucketName, key1, gOutcome.result().Content());
        EXPECT_EQ(outcome.isSuccess(), true);
        std::string imageInfo    = GetOssImageObjectInfo(BucketName, key1);
        //EXPECT_STREQ(imageInfo.c_str(), ImageInfo.c_str());
        EXPECT_TRUE(CompareImageInfo(imageInfo, ImageInfo));
    }
    else {
        EXPECT_TRUE(false);
    }
}

TEST_F(ObjectProcessTest, ImageProcessBysetProcessTest)
{
    std::string key = TestUtils::GetObjectKey("ImageProcessBysetProcessTest");
    key.append("-noraml.jpg");
    auto pOutcome = Client->PutObject(BucketName, key, ImageFilePath);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest request(BucketName, key);
    request.setProcess(Process);
    auto gOutcome = Client->GetObject(request);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    if (gOutcome.isSuccess()) {
        std::string key1 = TestUtils::GetObjectKey("ImageProcessBysetProcessTest");
        key1.append("-precessed.jpg");
        auto outcome = Client->PutObject(BucketName, key1, gOutcome.result().Content());
        EXPECT_EQ(outcome.isSuccess(), true);
        std::string imageInfo = GetOssImageObjectInfo(BucketName, key1);
        EXPECT_TRUE(CompareImageInfo(imageInfo, ImageInfo));
    }
    else {
        EXPECT_TRUE(false);
    }
}


TEST_F(ObjectProcessTest, GenerateUriWithProcessTest)
{
    std::string key = TestUtils::GetObjectKey("cpp-sdk-objectprocess");
    key.append("-url-noraml.jpg");
    auto pOutcome = Client->PutObject(BucketName, key, ImageFilePath);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    request.setProcess(Process);
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);

    if (gOutcome.isSuccess()) {
        std::string key1 = TestUtils::GetObjectKey("cpp-sdk-objectprocess");
        key1.append("-url-precessed.jpg");
        auto outcome = Client->PutObject(BucketName, key1, gOutcome.result().Content());
        EXPECT_EQ(outcome.isSuccess(), true);
        std::string imageInfo = GetOssImageObjectInfo(BucketName, key1);
        //EXPECT_STREQ(imageInfo.c_str(), ImageInfo.c_str());
        EXPECT_TRUE(CompareImageInfo(imageInfo, ImageInfo));
    }
    else {
        EXPECT_TRUE(false);
    }
}

TEST_F(ObjectProcessTest, ProcessObjectRequestTest)
{
    std::string key = TestUtils::GetObjectKey("ImageProcessBysetProcessAndSavetoTest");
    std::string key1 = key;
    std::string key2 = key;
    key.append("-noraml.jpg");
    key1.append("-saveas.jpg");
    key2.append("-saveas2.jpg");
    auto pOutcome = Client->PutObject(BucketName, key, ImageFilePath);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    std::stringstream ss;
    ss  << Process 
        <<"|sys/saveas"
        << ",o_" << Base64EncodeUrlSafe(key1)
        << ",b_" << Base64EncodeUrlSafe(BucketName);

    ProcessObjectRequest request(BucketName, key, ss.str());
    auto gOutcome = Client->ProcessObject(request);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    std::istreambuf_iterator<char> isb(*gOutcome.result().Content()), end;
    std::string json_str = std::string(isb, end);
    std::cout << json_str << std::endl;
    EXPECT_TRUE(json_str.find(key1) != std::string::npos);

    std::string imageInfo = GetOssImageObjectInfo(BucketName, key1);
    EXPECT_TRUE(CompareImageInfo(imageInfo, ImageInfo));

    //Use default bucketName
    ss.str("");
    ss << Process
        << "|sys/saveas"
        << ",o_" << Base64EncodeUrlSafe(key2);
    request.setProcess(ss.str());
    gOutcome = Client->ProcessObject(request);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    std::istreambuf_iterator<char> isb1(*gOutcome.result().Content()), end1;
    json_str = std::string(isb1, end1);
    std::cout << json_str << std::endl;
    EXPECT_TRUE(json_str.find(key2) != std::string::npos);
    imageInfo = GetOssImageObjectInfo(BucketName, key2);
    EXPECT_TRUE(CompareImageInfo(imageInfo, ImageInfo));
}

TEST_F(ObjectProcessTest, ProcessObjectRequestNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("ProcessObjectRequestNegativeTest");
    key.append("-noraml.jpg");
    auto pOutcome = Client->PutObject(BucketName, key, ImageFilePath);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    ProcessObjectRequest request(BucketName, key);
    auto gOutcome = Client->ProcessObject(request);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "InvalidRequest");
}

}
}