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
#include <src/utils/Crc64.h>
#include "src/utils/FileSystemUtils.h"
#include "src/utils/Utils.h"
#include "src/auth/SimpleCredentialsProvider.h"
#include "../Config.h"
#include "../Utils.h"
#include <alibabacloud/oss/OssEncryptionClient.h>
#include <fstream>

namespace AlibabaCloud {
namespace OSS {

class CryptoObjectTest : public ::testing::Test {
protected:
    CryptoObjectTest()
    {
    }

    ~CryptoObjectTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        const std::string publicKey =
            "-----BEGIN RSA PUBLIC KEY-----\n"
            "MIGJAoGBALpUiB+w+r3v2Fgw0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGl\n"
            "s5YGoqD4lObG/sCQyaWd0B/XzOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMK\n"
            "zeyVFFFvl9prlr6XpuJQlY0F/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAE=\n"
            "-----END RSA PUBLIC KEY-----";

        const std::string privateKey =
            "-----BEGIN RSA PRIVATE KEY-----\n"
            "MIICXgIBAAKBgQC6VIgfsPq979hYMNEoDG1pfG581FXN7d2GwPPR9d5a8O7+kVGy\n"
            "/PhxpbOWBqKg+JTmxv7AkMmlndAf18zoY5UnNW+d58mYZrPODBiepxdjUD/tYI2N\n"
            "QcgzCs3slRRRb5faa5a+l6biUJWNBf1uKW4y7JPD8eEIseQKCW0oRJVLtQIDAQAB\n"
            "AoGBAJrzWRAhuSLipeMRFZ5cV1B1rdwZKBHMUYCSTTC5amPuIJGKf4p9XI4F4kZM\n"
            "1klO72TK72dsAIS9rCoO59QJnCpG4CvLYlJ37wA2UbhQ1rBH5dpBD/tv3CUyfdtI\n"
            "9CLUsZR3DGBWXYwGG0KGMYPExe5Hq3PUH9+QmuO+lXqJO4IBAkEA6iLee6oBzu6v\n"
            "90zrr4YA9NNr+JvtplpISOiL/XzsU6WmdXjzsFLSsZCeaJKsfdzijYEceXY7zUNa\n"
            "0/qQh2BKoQJBAMu61rQ5wKtql2oR4ePTSm00/iHoIfdFnBNU+b8uuPXlfwU80OwJ\n"
            "Gbs0xBHe+dt4uT53QLci4KgnNkHS5lu4XJUCQQCisCvrvcuX4B6BNf+mbPSJKcci\n"
            "biaJqr4DeyKatoz36mhpw+uAH2yrWRPZEeGtayg4rvf8Jf2TuTOJi9eVWYFBAkEA\n"
            "uIPzyS81TQsxL6QajpjjI52HPXZcrPOis++Wco0Cf9LnA/tczSpA38iefAETEq94\n"
            "NxcSycsQ5br97QfyEsgbMQJANTZ/HyMowmDPIC+n9ExdLSrf4JydARSfntFbPsy1\n"
            "4oC6ciKpRdtAtAtiU8s9eAUSWi7xoaPJzjAHWbmGSHHckg==\n"
            "-----END RSA PRIVATE KEY-----";

        Endpoint = Config::Endpoint;
        Client = std::make_shared<OssEncryptionClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
            ClientConfiguration(), std::make_shared<SimpleRSAEncryptionMaterials>(publicKey, privateKey),
            CryptoConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-crypto-object");
        Client->CreateBucket(BucketName);
        TestFile = TestUtils::GetTargetFileName("cpp-sdk-multipartupload");
        TestUtils::WriteRandomDatatoFile(TestFile, 1 * 1024 * 1024 + 100);
        TestFileMd5 = TestUtils::GetFileMd5(TestFile);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        RemoveFile(TestFile);
        TestUtils::CleanBucket(*Client, BucketName);
        Client = nullptr;
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
public:
    static std::shared_ptr<OssEncryptionClient> Client;
    static std::string BucketName;
    static std::string TestFile;
    static std::string TestFileMd5;
    static std::string Endpoint;
};

std::shared_ptr<OssEncryptionClient> CryptoObjectTest::Client = nullptr;
std::string CryptoObjectTest::BucketName = "";
std::string CryptoObjectTest::TestFile = "";
std::string CryptoObjectTest::TestFileMd5 = "";
std::string CryptoObjectTest::Endpoint = "";

const int DownloadPartFailedFlag = 1 << 30;


static int64_t GetFileLength(const std::string& file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

static int CalculatePartCount(int64_t totalSize, int singleSize)
{
    // Calculate the part count
    auto partCount = (int)(totalSize / singleSize);
    if (totalSize % singleSize != 0)
    {
        partCount++;
    }
    return partCount;
}

TEST_F(CryptoObjectTest, PutAndGetObjectTest)
{
    std::string key = TestUtils::GetObjectKey("PutAndGetObjectTest");
    auto content = std::make_shared<std::stringstream>();
    *content << "123";

    PutObjectRequest pRequest(BucketName, key, content);
    auto pOutcome = Client->PutObject(pRequest);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() == nullptr);

    GetObjectRequest gRequest(BucketName, key);
    auto outcome = Client->GetObject(gRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
    
    auto oriMd5 = ComputeContentMD5(*content);
    auto getMd5 = ComputeContentMD5(*outcome.result().Content());

    auto ss = std::make_shared<std::stringstream>();
    outcome = Client->GetObject(BucketName, key, ss);
    EXPECT_EQ(outcome.isSuccess(), true);
    getMd5 = ComputeContentMD5(*ss);

    EXPECT_EQ(oriMd5, getMd5);
}

TEST_F(CryptoObjectTest, GetObjectBasicTest)
{
    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), true);

    auto fOutcome = Client->GetObject(BucketName, key, tmpFile);
    EXPECT_EQ(fOutcome.isSuccess(), true);
    fOutcome = dummy;

    std::shared_ptr<std::iostream> fileContent = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);

    std::string oriMd5 = ComputeContentMD5(*content.get());
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content().get());
    std::string fileMd5 = ComputeContentMD5(*fileContent.get());

    EXPECT_STREQ(oriMd5.c_str(), memMd5.c_str());
    EXPECT_STREQ(oriMd5.c_str(), fileMd5.c_str());
    fileContent = nullptr;
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoObjectTest, GetObjectToFileTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto fOutcome = Client->GetObject(BucketName, key, tmpFile);
    EXPECT_EQ(fOutcome.isSuccess(), true);

    auto fileETag = TestUtils::GetFileETag(tmpFile);

    EXPECT_NE(fileETag, pOutcome.result().ETag());

    fOutcome.result().setContent(std::shared_ptr<std::iostream>());

    EXPECT_EQ(RemoveFile(tmpFile), true);
}


TEST_F(CryptoObjectTest, GetObjectToNullContentTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToNullContentTest");
    std::shared_ptr<std::iostream> content = nullptr;
    auto outcome = Client->GetObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(CryptoObjectTest, GetObjectToFailContentTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToFailContentTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicNegativeTest").append(".tmp");

    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    auto outcome = Client->GetObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(CryptoObjectTest, GetObjectToBadContentTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToBadContentTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicNegativeTest").append(".tmp");

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    content->setstate(content->badbit);
    auto outcome = Client->GetObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(CryptoObjectTest, GetObjectToBadKeyTest)
{
    std::string key = "/InvalidObjectName";
    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(CryptoObjectTest, GetObjectUsingRangeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectUsingRangeTest");
    auto content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest request(BucketName, key);
    request.setRange(10, 19);
    auto outcome = Client->GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    char buffer[10];
    content->clear();
    content->seekg(10, content->beg);
    EXPECT_EQ(content->good(), true);
    content->read(buffer, 10);

    std::string oriMd5 = ComputeContentMD5(buffer, 10);
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content().get());

    EXPECT_STREQ(oriMd5.c_str(), memMd5.c_str());
}

TEST_F(CryptoObjectTest, GetObjectUsingRangeNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectUsingRangeNegativeTest");

    GetObjectRequest request(BucketName, key);
    request.setRange(10, 9);
    auto outcome = Client->GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "The range is invalid.") != nullptr);

    request.setRange(10, -2);
    outcome = Client->GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "The range is invalid.") != nullptr);

    request.setRange(-1, 9);
    outcome = Client->GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "The range is invalid.") != nullptr);
}

TEST_F(CryptoObjectTest, GetObjectMatchingETagPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectMatchingETagPositiveTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string eTag = outcome.result().ETag();
    GetObjectRequest reqeust(BucketName, key);
    reqeust.addMatchingETagConstraint(eTag);

    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_STREQ(gOutcome.result().Metadata().ETag().c_str(), eTag.c_str());
}

TEST_F(CryptoObjectTest, GetObjectMatchingETagsPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectMatchingETagsPositiveTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string eTag = outcome.result().ETag();
    GetObjectRequest reqeust(BucketName, key);
    std::vector<std::string> eTagList;
    eTagList.push_back(eTag);
    reqeust.addMatchingETagConstraint("invalidETag");
    reqeust.setMatchingETagConstraints(eTagList);

    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_STREQ(gOutcome.result().Metadata().ETag().c_str(), eTag.c_str());
}

TEST_F(CryptoObjectTest, GetObjectMatchingETagNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectMatchingETagNegativeTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetObjectRequest reqeust(BucketName, key);
    reqeust.addMatchingETagConstraint("Dummy1");
    reqeust.addMatchingETagConstraint("Dummy2");
    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "PreconditionFailed");
}

TEST_F(CryptoObjectTest, GetObjectModifiedSincePositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectModifiedSincePositiveTest");
    auto content = TestUtils::GetRandomStream(100);
    std::string timeStr = TestUtils::GetGMTString(-10);

    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetObjectRequest reqeust(BucketName, key);
    reqeust.setModifiedSinceConstraint(timeStr);
    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
}

TEST_F(CryptoObjectTest, GetObjectModifiedSinceNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectModifiedSinceNegativeTest");
    auto content = TestUtils::GetRandomStream(100);
    std::string timeStr = TestUtils::GetGMTString(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetObjectRequest reqeust(BucketName, key);
    reqeust.setModifiedSinceConstraint(timeStr);
    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "ServerError:304");
}

TEST_F(CryptoObjectTest, GetObjectNonMatchingETagPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectNonMatchingETagPositiveTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string eTag = "Dummy";
    GetObjectRequest reqeust(BucketName, key);
    reqeust.addNonmatchingETagConstraint(eTag);

    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
}

TEST_F(CryptoObjectTest, GetObjectNonMatchingETagsPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectNonMatchingETagsPositiveTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetObjectRequest reqeust(BucketName, key);
    std::vector<std::string> eTagList;
    eTagList.push_back("Dummy1");
    eTagList.push_back("Dummy2");
    reqeust.setNonmatchingETagConstraints(eTagList);

    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
}

TEST_F(CryptoObjectTest, GetObjectNonMatchingETagNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectNonMatchingETagNegativeTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string eTag = outcome.result().ETag();
    GetObjectRequest reqeust(BucketName, key);
    reqeust.addNonmatchingETagConstraint(eTag);

    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "ServerError:304");
}

TEST_F(CryptoObjectTest, GetObjectUnmodifiedSincePositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectUnmodifiedSincePositiveTest");
    auto content = TestUtils::GetRandomStream(100);
    std::string timeStr = TestUtils::GetGMTString(100);

    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetObjectRequest reqeust(BucketName, key);
    reqeust.setUnmodifiedSinceConstraint(timeStr);
    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
}

TEST_F(CryptoObjectTest, GetObjectUnmodifiedSinceNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectUnmodifiedSinceNegativeTest");
    auto content = TestUtils::GetRandomStream(100);
    std::string timeStr = TestUtils::GetGMTString(-10);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetObjectRequest reqeust(BucketName, key);
    reqeust.setUnmodifiedSinceConstraint(timeStr);
    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "PreconditionFailed");
}

TEST_F(CryptoObjectTest, GetObjectWithResponseHeadersSettingTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectWithResponseHeadersSettingTest");
    auto content = TestUtils::GetRandomStream(100);
    std::string timeStr = TestUtils::GetGMTString(100);

    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetObjectRequest reqeust(BucketName, key);
    auto gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentType(), "application/octet-stream");

    reqeust.addResponseHeaders(RequestResponseHeader::ContentType, "test/haah");
    gOutcome = Client->GetObject(reqeust);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentType(), "test/haah");
}

class GetObjectAsyncContex : public AsyncCallerContext
{
public:
    GetObjectAsyncContex() :ready(false) {}
    ~GetObjectAsyncContex() {}
    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    mutable std::string md5;
    mutable bool ready;
};

static void GetObjectHandler(const AlibabaCloud::OSS::OssClient* client,
    const GetObjectRequest& request,
    const GetObjectOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    std::cout << "Client[" << client << "]" << "GetObjectHandler" << ", key:" << request.Key() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const GetObjectAsyncContex *>(context.get());
        EXPECT_EQ(outcome.isSuccess(), true);
        std::string memMd5 = ComputeContentMD5(*outcome.result().Content().get());
        ctx->md5 = memMd5;
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}

TEST_F(CryptoObjectTest, GetObjectAsyncBasicTest)
{
    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectAsyncBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(102400);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectAsyncHandler handler = GetObjectHandler;
    GetObjectRequest request(BucketName, key);
    std::shared_ptr<GetObjectAsyncContex> memContext = std::make_shared<GetObjectAsyncContex>();

    GetObjectRequest fileRequest(BucketName, key);
    fileRequest.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios::binary); });
    std::shared_ptr<GetObjectAsyncContex> fileContext = std::make_shared<GetObjectAsyncContex>();

    Client->GetObjectAsync(request, handler, memContext);
    Client->GetObjectAsync(fileRequest, handler, fileContext);
    std::cout << "Client[" << Client << "]" << "Issue GetObjectAsync done." << std::endl;

    {
        std::unique_lock<std::mutex> lck(fileContext->mtx);
        if (!fileContext->ready) fileContext->cv.wait(lck);
    }

    {
        std::unique_lock<std::mutex> lck(memContext->mtx);
        if (!memContext->ready) memContext->cv.wait(lck);
    }

    std::string oriMd5 = ComputeContentMD5(*content.get());

    EXPECT_EQ(oriMd5, memContext->md5);
    EXPECT_EQ(oriMd5, fileContext->md5);
    memContext = nullptr;
    fileContext = nullptr;
    TestUtils::WaitForCacheExpire(1);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoObjectTest, GetObjectCallableBasicTest)
{
    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectCallableBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectCallableBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(102400);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest request(BucketName, key);
    std::shared_ptr<GetObjectAsyncContex> memContext = std::make_shared<GetObjectAsyncContex>();

    GetObjectRequest fileRequest(BucketName, key);
    fileRequest.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios::binary); });

    auto memOutcomeCallable = Client->GetObjectCallable(request);
    auto fileOutcomeCallable = Client->GetObjectCallable(fileRequest);


    std::cout << "Client[" << Client << "]" << "Issue GetObjectCallable done." << std::endl;

    auto fileOutcome = fileOutcomeCallable.get();
    auto memOutcome = memOutcomeCallable.get();
    EXPECT_EQ(fileOutcome.isSuccess(), true);
    EXPECT_EQ(memOutcome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content.get());
    std::string memMd5 = ComputeContentMD5(*memOutcome.result().Content());
    std::string fileMd5 = ComputeContentMD5(*fileOutcome.result().Content());

    EXPECT_EQ(oriMd5, fileMd5);
    EXPECT_EQ(oriMd5, fileMd5);
    memOutcome = dummy;
    fileOutcome = dummy;
    //EXPECT_EQ(RemoveFile(tmpFile), true);
    RemoveFile(tmpFile);
}

TEST_F(CryptoObjectTest, PutObjectBasicTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectBasicTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() == nullptr);

    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(CryptoObjectTest, PutObjectWithExpiresTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithExpiresTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    request.MetaData().setContentType("application/x-test");
    request.setExpires(TestUtils::GetGMTString(120));
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(CryptoObjectTest, PutObjectUsingContentLengthTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUsingContentLengthTest");
    std::shared_ptr<std::stringstream> content = std::make_shared<std::stringstream>();
    *content << "123456789";

    PutObjectRequest request(BucketName, key, content);
    request.MetaData().setContentLength(2);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5("12", 2);
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content().get());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(CryptoObjectTest, PutObjectFullSettingsTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectFullSettingsTest");
    auto content = TestUtils::GetRandomStream(1024);
    key.append("/attachement_test.data");

    std::string saveAs = "abc测试123.zip";
    std::string contentDisposition = "attachment;filename*=utf-8''";
    contentDisposition.append(UrlEncode(saveAs));

    PutObjectRequest request(BucketName, key, content);
    request.setCacheControl("no-cache");
    request.setContentDisposition(contentDisposition);
    request.setContentEncoding("gzip");

    std::string contentMd5 = ComputeContentMD5(*content);
    request.setContentMd5(contentMd5);

    //user metadata
    request.MetaData().UserMetaData()["MyKey1"] = "MyValue1";
    request.MetaData().UserMetaData()["MyKey2"] = "MyValue2";
    request.MetaData().UserMetaData()["MyKey3"] = "中文内容";

    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto metaOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(metaOutcome.isSuccess(), true);

    EXPECT_EQ(metaOutcome.result().CacheControl(), "no-cache");
    EXPECT_EQ(metaOutcome.result().ContentDisposition(), contentDisposition);
    EXPECT_EQ(metaOutcome.result().ContentEncoding(), "gzip");

    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey1"), "MyValue1");
    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey2"), "MyValue2");
    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey3"), "中文内容");
}

TEST_F(CryptoObjectTest, PutObjectDefaultMetadataTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectDefaultMetadataTest");
    auto content = TestUtils::GetRandomStream(1024);

    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);
}

TEST_F(CryptoObjectTest, PutObjectFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);

    auto pOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), true);

    std::fstream file(tmpFile, std::ios::in | std::ios::binary);
    std::string oriMd5 = ComputeContentMD5(file);
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
    file.close();
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoObjectTest, PutObjectUsingContentLengthFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUsingContentLengthFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectUsingContentLengthFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(file->good(), true);

    file->seekg(0, file->end);
    auto content_length = file->tellg();
    EXPECT_EQ(content_length, 1024LL);

    file->seekg(content_length / 2, file->beg);

    PutObjectRequest request(BucketName, key, file);
    request.MetaData().setContentLength(content_length / 2);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), true);

    char buff[2048];
    file->clear();
    file->seekg(content_length / 2, file->beg);
    file->read(buff, 2048);
    size_t readSize = static_cast<size_t>(file->gcount());

    std::string oriMd5 = ComputeContentMD5(buff, readSize);
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content().get());
    EXPECT_EQ(oriMd5, memMd5);
    file->close();
    RemoveFile(tmpFile);
}

TEST_F(CryptoObjectTest, PutObjectFullSettingsFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectFullSettingsFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFullSettingsFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(file->good(), true);
    key.append("/attachement_test.data");

    std::string saveAs = "abc测试123.zip";
    std::string contentDisposition = "attachment;filename*=utf-8''";
    contentDisposition.append(UrlEncode(saveAs));

    PutObjectRequest request(BucketName, key, file);
    request.setCacheControl("no-cache");
    request.setContentDisposition(contentDisposition);
    request.setContentEncoding("gzip");

    std::string contentMd5 = ComputeContentMD5(*file);
    request.setContentMd5(contentMd5);

    //user metadata
    request.MetaData().UserMetaData()["MyKey1"] = "MyValue1";
    request.MetaData().UserMetaData()["MyKey2"] = "MyValue2";

    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto metaOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(metaOutcome.isSuccess(), true);

    EXPECT_EQ(metaOutcome.result().CacheControl(), "no-cache");
    EXPECT_EQ(metaOutcome.result().ContentDisposition(), contentDisposition);
    EXPECT_EQ(metaOutcome.result().ContentEncoding(), "gzip");

    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey1"), "MyValue1");
    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey2"), "MyValue2");
    file->close();
    RemoveFile(tmpFile);
}

TEST_F(CryptoObjectTest, PutObjectDefaultMetadataFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectDefaultMetadataFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectDefaultMetadataFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(file->good(), true);

    auto outcome = Client->PutObject(BucketName, key, file);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);
    file->close();
    RemoveFile(tmpFile);
}

TEST_F(CryptoObjectTest, PutObjectUserMetadataFromFileContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUserMetadataFromFileContentTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectUserMetadataFromFileContentTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(file->good(), true);

    PutObjectRequest request(BucketName, key, file);
    request.MetaData().UserMetaData()["test"] = "testvalue";
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "testvalue");
    file->close();
    file = nullptr;
    RemoveFile(tmpFile);
}

TEST_F(CryptoObjectTest, PutObjectUserMetadataFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUserMetadataFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectUserMetadataFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);

    ObjectMetaData metaData;
    PutObjectRequest request(BucketName, key, file);
    request.MetaData().UserMetaData()["test"] = "testvalue";
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "testvalue");
    EXPECT_EQ(hOutcome.result().ContentLength(), 1024LL);
    file->close();
    file = nullptr;
    RemoveFile(tmpFile);
}

TEST_F(CryptoObjectTest, PutObjectWithNotExistFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithNotExistFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFileTest").append("-put.tmp");

    auto pOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(pOutcome.error().Message(), "Request body is in fail state. Logical error on i/o operation.");
}

TEST_F(CryptoObjectTest, PutObjectWithNullContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithNullContentTest");
    std::shared_ptr<std::iostream> content = nullptr;

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(pOutcome.error().Message(), "Request body is null.");
}

TEST_F(CryptoObjectTest, PutObjectWithBadContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithNullContentTest");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    content->setstate(content->badbit);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(pOutcome.error().Message(), "Request body is in bad state. Read/writing error on i/o operation.");
}

TEST_F(CryptoObjectTest, PutObjectWithSameContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithSameContentTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outcome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

class PutObjectAsyncContex : public AsyncCallerContext
{
public:
    PutObjectAsyncContex() :ready(false) {}
    virtual ~PutObjectAsyncContex()
    {
    }

    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    mutable bool ready;
};

static void PutObjectHandler(const AlibabaCloud::OSS::OssClient* client,
    const PutObjectRequest& request,
    const PutObjectOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    std::cout << "Client[" << client << "]" << "PutObjectHandler, tag:" << context->Uuid() <<
        ", key:" << request.Key() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const PutObjectAsyncContex *>(context.get());
        EXPECT_EQ(outcome.isSuccess(), true);
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}

TEST_F(CryptoObjectTest, PutObjectAsyncBasicTest)
{
    std::string memKey = TestUtils::GetObjectKey("PutObjectAsyncBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);

    std::string fileKey = TestUtils::GetObjectKey("PutObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectAsyncBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);

    PutObjectAsyncHandler handler = PutObjectHandler;
    PutObjectRequest memRequest(BucketName, memKey, memContent);
    std::shared_ptr<PutObjectAsyncContex> memContext = std::make_shared<PutObjectAsyncContex>();
    memContext->setUuid("PutobjectasyncFromMem");

    PutObjectRequest fileRequest(BucketName, fileKey, fileContent);
    std::shared_ptr<PutObjectAsyncContex> fileContext = std::make_shared<PutObjectAsyncContex>();
    fileContext->setUuid("PutobjectasyncFromFile");

    Client->PutObjectAsync(memRequest, handler, memContext);
    Client->PutObjectAsync(fileRequest, handler, fileContext);
    std::cout << "Client[" << Client << "]" << "Issue PutObjectAsync done." << std::endl;

    {
        std::unique_lock<std::mutex> lck(fileContext->mtx);
        if (!fileContext->ready) fileContext->cv.wait(lck);
    }

    {
        std::unique_lock<std::mutex> lck(memContext->mtx);
        if (!memContext->ready) memContext->cv.wait(lck);
    }

    fileContent->close();

    EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

    fileContext = nullptr;

    TestUtils::WaitForCacheExpire(1);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoObjectTest, PutObjectCallableBasicTest)
{
    std::string memKey = TestUtils::GetObjectKey("PutObjectCallableBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);

    std::string fileKey = TestUtils::GetObjectKey("PutObjectCallableBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectCallableBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);



    auto memOutcomeCallable = Client->PutObjectCallable(PutObjectRequest(BucketName, memKey, memContent));
    auto fileOutcomeCallable = Client->PutObjectCallable(PutObjectRequest(BucketName, fileKey, fileContent));


    std::cout << "Client[" << Client << "]" << "Issue PutObjectCallable done." << std::endl;

    auto fileOutcome = fileOutcomeCallable.get();
    auto memOutcome = memOutcomeCallable.get();
    EXPECT_EQ(fileOutcome.isSuccess(), true);
    EXPECT_EQ(memOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

    memContent = nullptr;
    fileContent = nullptr;
    RemoveFile(tmpFile);
}

TEST_F(CryptoObjectTest, PutAndGetObjectFailTest)
{
    std::string key = TestUtils::GetObjectKey("PutAndGetObjectFailTest");
    auto content = std::make_shared<std::stringstream>();
    *content << "123";

    auto pOutcome = Client->PutObject(BucketName, key, "invalid path");
    EXPECT_EQ(pOutcome.isSuccess(), false);

    auto outcome = Client->GetObject(BucketName, key, "invalid path");
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(CryptoObjectTest, PutObjectWithContentMd5Test)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithContentMd5Test");
    auto content = std::make_shared<std::stringstream>();
    *content << "123";
    auto oriMd5 = ComputeContentMD5(*content);

    PutObjectRequest pRequest(BucketName, key, content);
    pRequest.setContentMd5(oriMd5);
    auto pOutcome = Client->PutObject(pRequest);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() == nullptr);

    GetObjectRequest gRequest(BucketName, key);
    auto outcome = Client->GetObject(gRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
    auto getMd5 = ComputeContentMD5(*outcome.result().Content());
    auto unencryptMd5 = outcome.result().Metadata().UserMetaData().at("client-side-encryption-unencrypted-content-md5");

    EXPECT_EQ(unencryptMd5, getMd5);
}


TEST_F(CryptoObjectTest, GetObjectWithRangeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectWithRangeTest");
    auto content = std::make_shared<std::stringstream>();
    std::string contentStr = "1234567890abcdefjhijklmnopqrstuvwxzyQAZWSXEDC";
    *content << contentStr;
    int64_t contentLen = contentStr.size();

    PutObjectRequest pRequest(BucketName, key, content);
    auto pOutcome = Client->PutObject(pRequest);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() == nullptr);

    //from offset to end
    for (int64_t i = 0; i < contentLen - 1; i++) {
        GetObjectRequest gRequest(BucketName, key);
        gRequest.setRange(i, -1);
        auto outcome = Client->GetObject(gRequest);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().Metadata().ContentLength(), contentLen - i);
        std::string getStr;
        *outcome.result().Content() >> getStr;
        EXPECT_EQ(getStr, contentStr.substr(i));
    }

    //from offset to contentLen - 3
    int64_t last_remain = 2;
    for (int64_t i = 0; i < contentLen - 1 - last_remain; i++) {
        GetObjectRequest gRequest(BucketName, key);
        gRequest.setRange(i, contentLen - last_remain);
        auto outcome = Client->GetObject(gRequest);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().Metadata().ContentLength(), (contentLen - last_remain - i + 1) );
        std::string getStr;
        *outcome.result().Content() >> getStr;
        EXPECT_EQ(getStr, contentStr.substr(i, (contentLen - last_remain - i + 1)));
    }
}

//
static void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData)
{
    UNUSED_PARAM(increment);
    UNUSED_PARAM(transfered);
    UNUSED_PARAM(total);
    UNUSED_PARAM(userData);
}

TEST_F(CryptoObjectTest, PutObjectProgressTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectProgressTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    TransferProgress arg;
    arg.Handler = ProgressCallback;
    arg.UserData = this;
    request.setTransferProgress(arg);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
}

TEST_F(CryptoObjectTest, GetObjectProgressTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectProgressTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest grequest(BucketName, key);
    TransferProgress arg;
    arg.Handler = ProgressCallback;
    arg.UserData = this;
    grequest.setTransferProgress(arg);
    auto gOutcome = Client->GetObject(grequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
}

TEST_F(CryptoObjectTest, MultipartUploadComplexStepTest)
{
    const int partSize = 100 * 1024;
    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadComplexStepTest");
    const int64_t fileLength = GetFileLength(sourceFile);

    MultipartUploadCryptoContext cryptoCTX;
    cryptoCTX.setPartSize(partSize);
    cryptoCTX.setDataSize(fileLength);

    InitiateMultipartUploadRequest imuRequest(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(imuRequest, cryptoCTX);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());
    EXPECT_EQ(cryptoCTX.ContentMaterial().ContentIV().size(), static_cast<size_t>(16));
    EXPECT_EQ(cryptoCTX.ContentMaterial().ContentKey().size(), static_cast<size_t>(32));
    EXPECT_EQ(cryptoCTX.UploadId(), initOutcome.result().UploadId());

    // Set the part size, must be 16 alignment
    auto partCount = CalculatePartCount(fileLength, partSize);

    // Create a list to save result 
    PartList partETags;
    //upload the file
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(sourceFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(content->good(), true);
    if (content->good())
    {
        for (auto i = 0; i < partCount; i++)
        {
            // Skip to the start position
            int64_t skipBytes = partSize * i;
            int64_t position = skipBytes;

            // Create a UploadPartRequest, uploading parts
            content->clear();
            content->seekg(position, content->beg);
            UploadPartRequest request(BucketName, targetObjectKey, content);
            request.setPartNumber(i + 1);
            request.setUploadId(initOutcome.result().UploadId());
            request.setContentLength(partSize);
            auto uploadPartOutcome = Client->UploadPart(request, cryptoCTX);
            EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
            EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());

            // Save the result
            Part part(i + 1, uploadPartOutcome.result().ETag());
            partETags.push_back(part);
        }
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_FALSE(lmuOutcome.result().RequestId().empty());

    std::string uploadId;

    for (auto const &upload : lmuOutcome.result().MultipartUploadList())
    {
        if (upload.UploadId == initOutcome.result().UploadId())
        {
            uploadId = upload.UploadId;
        }
    }

    EXPECT_EQ(uploadId.empty(), false);

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, partETags);
    completeRequest.setUploadId(uploadId);
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest, cryptoCTX);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_FALSE(cOutcome.result().RequestId().empty());

    auto gOutcome = Client->GetObject(GetObjectRequest(BucketName, targetObjectKey));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);
    auto calcMd5 = ComputeContentMD5(*gOutcome.result().Content());
    EXPECT_EQ(calcMd5, TestFileMd5);
}

TEST_F(CryptoObjectTest, InitiateMultipartUploadWithoutSpecialHeadersTest)
{
    const int partSize = 100 * 1024;
    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadComplexStepTest");
    const int64_t fileLength = GetFileLength(sourceFile);

    MultipartUploadCryptoContext cryptoCTX;

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request, cryptoCTX);
    EXPECT_EQ(initOutcome.isSuccess(), false);
    EXPECT_EQ(initOutcome.error().Code(), "EncryptionClientError");

    cryptoCTX.setPartSize(partSize);
    initOutcome = Client->InitiateMultipartUpload(request, cryptoCTX);
    EXPECT_EQ(initOutcome.isSuccess(), false);
    EXPECT_EQ(initOutcome.error().Code(), "InvalidEncryptionRequest");

    cryptoCTX.setPartSize(partSize);
    cryptoCTX.setDataSize(fileLength);
    initOutcome = Client->InitiateMultipartUpload(request, cryptoCTX);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());
    EXPECT_EQ(cryptoCTX.ContentMaterial().ContentIV().size(), static_cast<size_t>(16));
    EXPECT_EQ(cryptoCTX.ContentMaterial().ContentKey().size(), static_cast<size_t>(32));
    EXPECT_EQ(cryptoCTX.UploadId(), initOutcome.result().UploadId());
}


TEST_F(CryptoObjectTest, CompleteMultipartUploadNegativeTest)
{
    const int partSize = 100 * 1024;
    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadComplexStepTest");
    const int64_t fileLength = GetFileLength(sourceFile);

    MultipartUploadCryptoContext cryptoCTX;
    cryptoCTX.setPartSize(partSize);
    cryptoCTX.setDataSize(fileLength);

    InitiateMultipartUploadRequest imuRequest(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(imuRequest, cryptoCTX);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());
    EXPECT_EQ(cryptoCTX.ContentMaterial().ContentIV().size(), static_cast<size_t>(16));
    EXPECT_EQ(cryptoCTX.ContentMaterial().ContentKey().size(), static_cast<size_t>(32));
    EXPECT_EQ(cryptoCTX.UploadId(), initOutcome.result().UploadId());


    // Create a list to save result 
    PartList partETags;
    //upload the file
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(sourceFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(content->good(), true);
    if (content->good())
    {
        for (auto i = 0; i < 2; i++)
        {
            // Skip to the start position
            int64_t skipBytes = partSize * i;
            int64_t position = skipBytes;

            // Create a UploadPartRequest, uploading parts
            content->clear();
            content->seekg(position, content->beg);
            UploadPartRequest request(BucketName, targetObjectKey, content);
            request.setPartNumber(i + 1);
            request.setUploadId(initOutcome.result().UploadId());
            request.setContentLength(partSize);
            auto uploadPartOutcome = Client->UploadPart(request, cryptoCTX);
            EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
            EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());

            // Save the result
            Part part(i + 1, uploadPartOutcome.result().ETag());
            partETags.push_back(part);
        }
    }

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, partETags);
    completeRequest.setUploadId(cryptoCTX.UploadId());
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest, cryptoCTX);
    EXPECT_EQ(cOutcome.isSuccess(), false);
    EXPECT_EQ(cOutcome.error().Code(), "InvalidEncryptionRequest");
    EXPECT_EQ(cOutcome.error().Message(), "The client encryption part list is unexpected with init_multipart setted.");
}

TEST_F(CryptoObjectTest, MultipartUploadCryptoContextTest)
{
    auto key = TestUtils::GetObjectKey("MultipartUploadCryptoContextTest");

    MultipartUploadCryptoContext cryptoCtx;
    cryptoCtx.setPartSize(102401);
    cryptoCtx.setDataSize(11);
    auto initOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, key), cryptoCtx);
    EXPECT_EQ(initOutcome.isSuccess(), false);
    EXPECT_EQ(initOutcome.error().Code(), "EncryptionClientError");

    auto content = std::make_shared<std::stringstream>("hello world");
    UploadPartRequest request(BucketName, key, content);
    request.setPartNumber(1);
    request.setUploadId("uploadid");
    request.setContentLength(11);
    auto uploadPartOutcome = Client->UploadPart(request, cryptoCtx);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), false);
    EXPECT_EQ(uploadPartOutcome.error().Code(), "EncryptionClientError");
}


TEST_F(CryptoObjectTest, MultipartUploadCallableBasicTest)
{
    auto memKey = TestUtils::GetObjectKey("MultipartUploadCallable-MemObject");
    auto memContent = TestUtils::GetRandomStream(102400);
    MultipartUploadCryptoContext memCryptoCtx;
    memCryptoCtx.setPartSize(102400);
    memCryptoCtx.setDataSize(102400);
    auto memInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, memKey), memCryptoCtx);
    EXPECT_EQ(memInitOutcome.isSuccess(), true);

    auto fileKey = TestUtils::GetObjectKey("MultipartUploadCallable-FileObject");
    auto tmpFile = TestUtils::GetObjectKey("MultipartUploadCallable-FileObject").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    {
        MultipartUploadCryptoContext fileCryptoCtx;
        fileCryptoCtx.setPartSize(102400);
        fileCryptoCtx.setDataSize(1024);
        auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);
        auto fileInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, fileKey), fileCryptoCtx);
        EXPECT_EQ(fileInitOutcome.isSuccess(), true);

        auto memOutcomeCallable = Client->UploadPartCallable(UploadPartRequest(BucketName, memKey,
            1, memInitOutcome.result().UploadId(), memContent), memCryptoCtx);
        auto fileOutcomeCallable = Client->UploadPartCallable(UploadPartRequest(BucketName, fileKey,
            1, fileInitOutcome.result().UploadId(), fileContent), fileCryptoCtx);

        std::cout << "Client[" << Client << "]" << "Issue MultipartUploadCallable done." << std::endl;

        auto fileOutcome = fileOutcomeCallable.get();
        auto menOutcome = memOutcomeCallable.get();
        EXPECT_EQ(fileOutcome.isSuccess(), true);
        EXPECT_EQ(menOutcome.isSuccess(), true);

        // list part
        auto memListPartOutcome = Client->ListParts(ListPartsRequest(BucketName, memKey, memInitOutcome.result().UploadId()));
        auto fileListPartOutcome = Client->ListParts(ListPartsRequest(BucketName, fileKey, fileInitOutcome.result().UploadId()));
        EXPECT_EQ(memListPartOutcome.isSuccess(), true);
        EXPECT_EQ(fileListPartOutcome.isSuccess(), true);

        auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, memKey,
            memListPartOutcome.result().PartList(), memInitOutcome.result().UploadId()), memCryptoCtx);
        auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, fileKey,
            fileListPartOutcome.result().PartList(), fileInitOutcome.result().UploadId()), fileCryptoCtx);
        EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
        EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

        EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

        memContent = nullptr;
        fileContent = nullptr;
    }
    RemoveFile(tmpFile);
}

class UploadPartAsyncContext : public AsyncCallerContext
{
public:
    UploadPartAsyncContext() :ready(false) {}
    virtual ~UploadPartAsyncContext()
    {
    }

    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    mutable bool ready;
};

static void UploadPartInvalidHandler(const AlibabaCloud::OSS::OssClient* client,
    const UploadPartRequest& request,
    const PutObjectOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    UNUSED_PARAM(request);
    std::cout << "Client[" << client << "]" << "UploadPartInvalidHandler, tag:" << context->Uuid() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const UploadPartAsyncContext *>(context.get());
        EXPECT_EQ(outcome.isSuccess(), false);
        std::cout << __FUNCTION__ << " InvalidHandler, code:" << outcome.error().Code()
            << ", message:" << outcome.error().Message() << std::endl;
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}

static void UploadPartHandler(const AlibabaCloud::OSS::OssClient* client,
    const UploadPartRequest& request,
    const PutObjectOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    UNUSED_PARAM(request);
    std::cout << "Client[" << client << "]" << "UploadPartHandler,tag:" << context->Uuid() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const UploadPartAsyncContext *>(context.get());
        if (!outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " failed, Code:" << outcome.error().Code()
                << ", message:" << outcome.error().Message() << std::endl;
        }
        EXPECT_EQ(outcome.isSuccess(), true);
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}

TEST_F(CryptoObjectTest, UploadPartAsyncBasicTest)
{
    std::string memKey = TestUtils::GetObjectKey("UploadPartMemObjectAsyncBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);
    MultipartUploadCryptoContext memCryptoCtx;
    memCryptoCtx.setPartSize(102400);
    memCryptoCtx.setDataSize(102400);
    InitiateMultipartUploadRequest memInitRequest(BucketName, memKey);
    auto memInitOutcome = Client->InitiateMultipartUpload(memInitRequest, memCryptoCtx);
    EXPECT_EQ(memInitOutcome.isSuccess(), true);
    EXPECT_EQ(memInitOutcome.result().Key(), memKey);

    std::string fileKey = TestUtils::GetObjectKey("UploadPartFileObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("UploadPartFileObjectAsyncBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);
    MultipartUploadCryptoContext fileCryptoCtx;
    fileCryptoCtx.setPartSize(102400);
    fileCryptoCtx.setDataSize(1024);
    InitiateMultipartUploadRequest fileInitRequest(BucketName, fileKey);
    auto fileInitOutcome = Client->InitiateMultipartUpload(fileInitRequest, fileCryptoCtx);
    EXPECT_EQ(fileInitOutcome.isSuccess(), true);
    EXPECT_EQ(fileInitOutcome.result().Key(), fileKey);

    UploadPartAsyncHandler handler = UploadPartHandler;

    UploadPartRequest memRequest(BucketName, memKey, 1, memInitOutcome.result().UploadId(), memContent);
    std::shared_ptr<UploadPartAsyncContext> memContext = std::make_shared<UploadPartAsyncContext>();
    memContext->setUuid("UploadPartAsyncFromMem");

    UploadPartRequest fileRequest(BucketName, fileKey, 1, fileInitOutcome.result().UploadId(), fileContent);
    std::shared_ptr<UploadPartAsyncContext> fileContext = std::make_shared<UploadPartAsyncContext>();
    fileContext->setUuid("UploadPartAsyncFromFile");

    Client->UploadPartAsync(memRequest, handler, memCryptoCtx, memContext);
    Client->UploadPartAsync(fileRequest, handler, fileCryptoCtx, fileContext);
    std::cout << "Client[" << Client << "]" << "Issue UploadPartAsync done." << std::endl;
    {
        std::unique_lock<std::mutex> lck(fileContext->mtx);
        if (!fileContext->ready) fileContext->cv.wait(lck);
    }

    {
        std::unique_lock<std::mutex> lck(memContext->mtx);
        if (!memContext->ready) memContext->cv.wait(lck);
    }

    fileContent->close();

    auto memListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName, memKey, memInitOutcome.result().UploadId()));
    EXPECT_EQ(memListPartsOutcome.isSuccess(), true);
    auto fileListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName, fileKey, fileInitOutcome.result().UploadId()));
    EXPECT_EQ(fileListPartsOutcome.isSuccess(), true);

    auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, memKey,
        memListPartsOutcome.result().PartList(), memInitOutcome.result().UploadId()), memCryptoCtx);
    EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
    auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, fileKey,
        fileListPartsOutcome.result().PartList(), fileInitOutcome.result().UploadId()), fileCryptoCtx);
    EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);
        
    EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

    fileContext = nullptr;
    TestUtils::WaitForCacheExpire(1);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoObjectTest, UploadPartAsyncInvalidContentTest)
{
    std::string key = TestUtils::GetObjectKey("UploadPartCopyAsyncInvalidContentTest");
    UploadPartAsyncHandler handler = UploadPartInvalidHandler;
    std::shared_ptr<std::iostream> content = nullptr;
    MultipartUploadCryptoContext cryptoCtx;
    cryptoCtx.setPartSize(102400);
    cryptoCtx.setDataSize(1024);
    UploadPartRequest request(BucketName, key, 1, "id", content);
    std::shared_ptr<UploadPartAsyncContext> context = std::make_shared<UploadPartAsyncContext>();
    context->setUuid("UploadPartCopyAsyncInvalidContent");
    Client->UploadPartAsync(request, handler, cryptoCtx, context);

    TestUtils::WaitForCacheExpire(1);

    content = TestUtils::GetRandomStream(100);
    request.setConetent(content);
    Client->UploadPartAsync(request, handler, cryptoCtx, context);

    TestUtils::WaitForCacheExpire(1);

    request.setContentLength(5LL * 1024LL * 1024LL * 1024LL + 1LL);
    Client->UploadPartAsync(request, handler, cryptoCtx, context);

    TestUtils::WaitForCacheExpire(1);

    ContentCryptoMaterial contentMaterial;
    contentMaterial.setContentIV(ByteBuffer(16));
    contentMaterial.setContentKey(ByteBuffer(32));
    cryptoCtx.setContentMaterial(contentMaterial);
    request.setContentLength(100);
    Client->UploadPartAsync(request, handler, cryptoCtx, context);

    TestUtils::WaitForCacheExpire(1);

    request.setBucket("non-existent-bucket");
    request.setContentLength(100);
    Client->UploadPartAsync(request, handler, cryptoCtx, context);
    request.setBucket(BucketName);
}


TEST_F(CryptoObjectTest, NormalResumableDownloadRetryWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("NormalDownloadObjectRetryWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalDownloadObjectRetryWithCheckpoint").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    // upload object
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    auto uploadOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);
    content = nullptr;

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, 1);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // retry
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);

    std::string uploadFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadFileMd5, downloadFileMd5);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoObjectTest, MultiResumableDownloadRetryWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("MultiDownloadObjectRetryWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiDownloadObjectRetryWithCheckpoint").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    // upload object
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    auto uploadOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    content = nullptr;
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    int threadNum = 1 + rand() % 100;
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, threadNum);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // retry
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);

    std::string uploadFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadFileMd5, downloadFileMd5);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoObjectTest, EncryptionMaterialsFunctionFailTest)
{
    std::string publicKey = "invalid";
    std::string privateKey = "invalid";
    auto client = std::make_shared<OssEncryptionClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
        ClientConfiguration(), std::make_shared<SimpleRSAEncryptionMaterials>(publicKey, privateKey),
        CryptoConfiguration());

    //put
    auto content = std::make_shared<std::stringstream>("just for test");
    auto pOutcome = client->PutObject(BucketName, "test-key", content);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "EncryptionClientError");
    EXPECT_EQ(pOutcome.error().Message(), "EncryptCEK fail, return value:-1");

    //get
    std::string key = TestUtils::GetObjectKey("test-key");
    pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    auto gOutcome = client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "EncryptionClientError");
    EXPECT_EQ(gOutcome.error().Message(), "DecryptCEK fail, return value:-1");

    //init
    MultipartUploadCryptoContext cryptoCTX;
    cryptoCTX.setPartSize(1001024);
    InitiateMultipartUploadRequest request(BucketName, "test-key");
    auto initOutcome = client->InitiateMultipartUpload(request, cryptoCTX);
    EXPECT_EQ(initOutcome.isSuccess(), false);
    EXPECT_EQ(initOutcome.error().Code(), "EncryptionClientError");
    EXPECT_EQ(initOutcome.error().Message(), "EncryptCEK fail, return value:-1");
}

TEST_F(CryptoObjectTest, DifferentEncryptionMaterialsTest)
{
    const std::string publicKey =
        "-----BEGIN RSA PUBLIC KEY-----\n"
        "MIGJAoGBALpUiB+w+r3v2Fgw0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGl\n"
        "s5YGoqD4lObG/sCQyaWd0B/XzOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMK\n"
        "zeyVFFFvl9prlr6XpuJQlY0F/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAE=\n"
        "-----END RSA PUBLIC KEY-----";
    const std::string privateKey =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIICXgIBAAKBgQC6VIgfsPq979hYMNEoDG1pfG581FXN7d2GwPPR9d5a8O7+kVGy\n"
        "/PhxpbOWBqKg+JTmxv7AkMmlndAf18zoY5UnNW+d58mYZrPODBiepxdjUD/tYI2N\n"
        "QcgzCs3slRRRb5faa5a+l6biUJWNBf1uKW4y7JPD8eEIseQKCW0oRJVLtQIDAQAB\n"
        "AoGBAJrzWRAhuSLipeMRFZ5cV1B1rdwZKBHMUYCSTTC5amPuIJGKf4p9XI4F4kZM\n"
        "1klO72TK72dsAIS9rCoO59QJnCpG4CvLYlJ37wA2UbhQ1rBH5dpBD/tv3CUyfdtI\n"
        "9CLUsZR3DGBWXYwGG0KGMYPExe5Hq3PUH9+QmuO+lXqJO4IBAkEA6iLee6oBzu6v\n"
        "90zrr4YA9NNr+JvtplpISOiL/XzsU6WmdXjzsFLSsZCeaJKsfdzijYEceXY7zUNa\n"
        "0/qQh2BKoQJBAMu61rQ5wKtql2oR4ePTSm00/iHoIfdFnBNU+b8uuPXlfwU80OwJ\n"
        "Gbs0xBHe+dt4uT53QLci4KgnNkHS5lu4XJUCQQCisCvrvcuX4B6BNf+mbPSJKcci\n"
        "biaJqr4DeyKatoz36mhpw+uAH2yrWRPZEeGtayg4rvf8Jf2TuTOJi9eVWYFBAkEA\n"
        "uIPzyS81TQsxL6QajpjjI52HPXZcrPOis++Wco0Cf9LnA/tczSpA38iefAETEq94\n"
        "NxcSycsQ5br97QfyEsgbMQJANTZ/HyMowmDPIC+n9ExdLSrf4JydARSfntFbPsy1\n"
        "4oC6ciKpRdtAtAtiU8s9eAUSWi7xoaPJzjAHWbmGSHHckg==\n"
        "-----END RSA PRIVATE KEY-----";
    std::map<std::string, std::string> description;
    description["provider"] = "aliclould";

    auto otherClient = std::make_shared<OssEncryptionClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
        ClientConfiguration(), std::make_shared<SimpleRSAEncryptionMaterials>(publicKey, privateKey, description),
        CryptoConfiguration());

    std::string key = TestUtils::GetObjectKey("DifferentEncryptionMaterialsTest");
    auto content = std::make_shared<std::stringstream>("just for test");
    auto pOutcome = otherClient->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto gOutcome = otherClient->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(gOutcome.isSuccess(), true);

    //
    gOutcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "EncryptionClientError");
    EXPECT_EQ(gOutcome.error().Message(), "DecryptCEK fail, return value:-1");

    auto otherClient2 = std::make_shared<OssEncryptionClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
        ClientConfiguration(), std::make_shared<SimpleRSAEncryptionMaterials>(publicKey, privateKey, description),
        CryptoConfiguration());

    gOutcome = otherClient2->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(gOutcome.isSuccess(), true);

    auto oriMd5 = ComputeContentMD5(*content);
    auto getMd5 = ComputeContentMD5(*gOutcome.result().Content());
    EXPECT_EQ(oriMd5, getMd5);
}


TEST_F(CryptoObjectTest, DownloadUnencryptedObjectTest)
{
    auto otherClient = std::make_shared<OssClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
        ClientConfiguration());

    std::string key = TestUtils::GetObjectKey("DownloadUnencryptedObjectTest");
    auto content = std::make_shared<std::stringstream>("just for test");
    auto pOutcome = otherClient->PutObject(BucketName, key, content);

    GetObjectRequest gRequest(BucketName, key);
    auto outcome = Client->GetObject(gRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto oriMd5 = ComputeContentMD5(*content);
    auto getMd5 = ComputeContentMD5(*outcome.result().Content());
    EXPECT_EQ(oriMd5, getMd5);
}

TEST_F(CryptoObjectTest, UnsupportCEKAlgoTest)
{
    auto otherClient = std::make_shared<OssClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
        ClientConfiguration());

    std::string key = TestUtils::GetObjectKey("UnsupportCEKAlgoTest");
    auto content = std::make_shared<std::stringstream>("just for test");
    PutObjectRequest pRequest(BucketName, key, content);
    pRequest.MetaData().addUserHeader("client-side-encryption-key", "1234");
    pRequest.MetaData().addUserHeader("client-side-encryption-start", "1234");
    pRequest.MetaData().addUserHeader("client-side-encryption-cek-alg", "AES/ECB/NoPadding");
    pRequest.MetaData().addUserHeader("client-side-encryption-wrap-alg", "RSA/NONE/PKCS1Padding");
    auto pOutcome = otherClient->PutObject(pRequest);

    GetObjectRequest gRequest(BucketName, key);
    auto outcome = Client->GetObject(gRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "EncryptionClientError");
    EXPECT_EQ(outcome.error().Message(), "Cipher name is not support, expect AES/CTR/NoPadding, got AES/ECB/NoPadding.");
}

TEST_F(CryptoObjectTest, CompatibilityTest)
{
    #define ArraySize(a) sizeof(a)/sizeof(a[0])
    std::string fileNames[] = { "cpp-enc-example.jpg", "go-enc-example.jpg" };
    std::map<std::string, std::string> userMetas[] = { 
        //cpp-enc-example.jpg
        { 
            {"client-side-encryption-key", "nyXOp7delQ/MQLjKQMhHLaT0w7u2yQoDLkSnK8MFg/MwYdh4na4/LS8LLbLcM18m8I/ObWUHU775I50sJCpdv+f4e0jLeVRRiDFWe+uo7Puc9j4xHj8YB3QlcIOFQiTxHIB6q+C+RA6lGwqqYVa+n3aV5uWhygyv1MWmESurppg="},
            {"client-side-encryption-start", "De/S3T8wFjx7QPxAAFl7h7TeI2EsZlfCwox4WhLGng5DK2vNXxULmulMUUpYkdc9umqmDilgSy5Z3Foafw+v4JJThfw68T/9G2gxZLrQTbAlvFPFfPM9Ehk6cY4+8WpY32uN8w5vrHyoSZGr343NxCUGIp6fQ9sSuOLMoJg7hNw="},
            {"client-side-encryption-cek-alg", "AES/CTR/NoPadding"},
            {"client-side-encryption-wrap-alg", "RSA/NONE/PKCS1Padding"},
        },
        //go-enc-example.jpg
        {
            {"client-side-encryption-key", "F2L5QjyA2s85tPvaGdQ5EKnU/XN5dUWqZfgwcM4gfzPMcDWR93AZGSpeB9VSJBYPdIqhy1cevKEJv+Dv2ckDuDJ7nzijwcBnO5tPl5jXYlWxgzj6t1gMqQr/LENbB5iC8hzGkkoVWjWtSPDB+uE3+qf4V1A0308OqSM3OKxV0VI="},
            {"client-side-encryption-start", "D+3z6ftLp500eVnvsat5awYdYI/jTeSRlGlmHNrhTm3l1bonYP1v72vGqZhvOpT++9ZXOhdePu82gjhqVfh8Qv2HZsVGeJLzQJRU8kIKc7PRI4SoqpHZh2VYsASvnDtxVy2MQmpJzvG8xr4j3I29EgsEha7NV+2hGq/dolxLHNc="},
            {"client-side-encryption-cek-alg", "AES/CTR/NoPadding"},
            {"client-side-encryption-wrap-alg", "RSA/NONE/PKCS1Padding"},
        },
    };

    std::string privateKey =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIICWwIBAAKBgQCokfiAVXXf5ImFzKDw+XO/UByW6mse2QsIgz3ZwBtMNu59fR5z\n"
        "ttSx+8fB7vR4CN3bTztrP9A6bjoN0FFnhlQ3vNJC5MFO1PByrE/MNd5AAfSVba93\n"
        "I6sx8NSk5MzUCA4NJzAUqYOEWGtGBcom6kEF6MmR1EKib1Id8hpooY5xaQIDAQAB\n"
        "AoGAOPUZgkNeEMinrw31U3b2JS5sepG6oDG2CKpPu8OtdZMaAkzEfVTJiVoJpP2Y\n"
        "nPZiADhFW3e0ZAnak9BPsSsySRaSNmR465cG9tbqpXFKh9Rp/sCPo4Jq2n65yood\n"
        "JBrnGr6/xhYvNa14sQ6xjjfSgRNBSXD1XXNF4kALwgZyCAECQQDV7t4bTx9FbEs5\n"
        "36nAxPsPM6aACXaOkv6d9LXI7A0J8Zf42FeBV6RK0q7QG5iNNd1WJHSXIITUizVF\n"
        "6aX5NnvFAkEAybeXNOwUvYtkgxF4s28s6gn11c5HZw4/a8vZm2tXXK/QfTQrJVXp\n"
        "VwxmSr0FAajWAlcYN/fGkX1pWA041CKFVQJAG08ozzekeEpAuByTIOaEXgZr5MBQ\n"
        "gBbHpgZNBl8Lsw9CJSQI15wGfv6yDiLXsH8FyC9TKs+d5Tv4Cvquk0efOQJAd9OC\n"
        "lCKFs48hdyaiz9yEDsc57PdrvRFepVdj/gpGzD14mVerJbOiOF6aSV19ot27u4on\n"
        "Td/3aifYs0CveHzFPQJAWb4LCDwqLctfzziG7/S7Z74gyq5qZF4FUElOAZkz718E\n"
        "yZvADwuz/4aK0od0lX9c4Jp7Mo5vQ4TvdoBnPuGoyw==\n"
        "-----END RSA PRIVATE KEY-----";
    
    EXPECT_EQ(ArraySize(fileNames), ArraySize(userMetas));

    ClientConfiguration conf;
    auto unencryptedClient = std::make_shared<OssClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto encryptedClient = std::make_shared<OssEncryptionClient>(Endpoint,
        std::make_shared<SimpleCredentialsProvider>(Config::AccessKeyId, Config::AccessKeySecret), conf,
        std::make_shared<SimpleRSAEncryptionMaterials>("", privateKey), CryptoConfiguration());

    size_t index = 0;
    for (const auto& file : fileNames) {
        std::string encryptedFile = Config::GetDataPath();
        encryptedFile.append(file);
        
        std::string oriFile = Config::GetDataPath();
        oriFile.append("example.jpg");

        std::string key = file;
        ObjectMetaData meta;
        for (const auto& it : userMetas[index]) {
            meta.addUserHeader(it.first, it.second);
        }
        
        auto pOutcome = unencryptedClient->PutObject(BucketName, key, encryptedFile, meta);
        EXPECT_EQ(pOutcome.isSuccess(), true);

        auto gOutcome = encryptedClient->GetObject(GetObjectRequest(BucketName, key));
        EXPECT_EQ(gOutcome.isSuccess(), true);

        for (const auto& it : userMetas[index]) {
            EXPECT_EQ(gOutcome.result().Metadata().UserMetaData().at(it.first), it.second);
        }

        auto getMd5 = ComputeContentMD5(*gOutcome.result().Content());
        auto oriMd5 = TestUtils::GetFileMd5(oriFile);

        EXPECT_EQ(oriMd5, getMd5);
        index++;
    }

    EXPECT_EQ(index, ArraySize(fileNames));
}

class EncryptionDisableFuncClient : public OssEncryptionClient {

public:
    EncryptionDisableFuncClient(const std::string& endpoint, const std::string& accessKeyId, const std::string& accessKeySecret) :
        OssEncryptionClient(endpoint, accessKeyId, accessKeySecret, ClientConfiguration(),
            std::make_shared<SimpleRSAEncryptionMaterials>("", ""), CryptoConfiguration())
    {}

public:
    AppendObjectOutcome AppendObject(const AppendObjectRequest& request) const
    {
        return OssEncryptionClient::AppendObject(request);
    }
    
    UploadPartCopyOutcome UploadPartCopy(const UploadPartCopyRequest& request, const MultipartUploadCryptoContext& ctx) const
    {
        return OssEncryptionClient::UploadPartCopy(request, ctx);
    }
    void UploadPartCopyAsync(const UploadPartCopyRequest& request, const UploadPartCopyAsyncHandler& handler, const MultipartUploadCryptoContext& cryptoCtx, const std::shared_ptr<const AsyncCallerContext>& context = nullptr) const
    {
        OssEncryptionClient::UploadPartCopyAsync(request, handler, cryptoCtx, context);
    }
    UploadPartCopyOutcomeCallable UploadPartCopyCallable(const UploadPartCopyRequest& request, const MultipartUploadCryptoContext& cryptoCtx) const
    {
        return OssEncryptionClient::UploadPartCopyCallable(request, cryptoCtx);
    }
    CopyObjectOutcome ResumableCopyObject(const MultiCopyObjectRequest& request) const
    {
        return OssEncryptionClient::ResumableCopyObject(request);
    }
    GetObjectOutcome GetObjectByUrl(const GetObjectByUrlRequest& request) const
    {
        return OssEncryptionClient::GetObjectByUrl(request);
    }
    PutObjectOutcome PutObjectByUrl(const PutObjectByUrlRequest& request) const
    {
        return OssEncryptionClient::PutObjectByUrl(request);
    }
};

static void UploadPartCopyDisableFuncHandler(const AlibabaCloud::OSS::OssClient* client,
    const UploadPartCopyRequest& request,
    const UploadPartCopyOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    UNUSED_PARAM(request);
    std::cout << "Client[" << client << "]" << "UploadPartCopyHandler, tag:" << context->Uuid() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const UploadPartAsyncContext *>(context.get());
        if (!outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " failed, Code:" << outcome.error().Code()
                << ", message:" << outcome.error().Message() << std::endl;
        }
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "EncryptionClientError");
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}

TEST_F(CryptoObjectTest, TestDisableFunctionTest)
{
    EncryptionDisableFuncClient disableClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret);

    auto content = std::make_shared<std::stringstream>();

    auto aOutcome = disableClient.AppendObject(AppendObjectRequest(BucketName, "key", content));
    EXPECT_EQ(aOutcome.isSuccess(), false);
    EXPECT_EQ(aOutcome.error().Code(), "EncryptionClientError");

    MultipartUploadCryptoContext ctx;
    auto uOutcome = disableClient.UploadPartCopy(UploadPartCopyRequest(BucketName, "key"), ctx);
    EXPECT_EQ(uOutcome.isSuccess(), false);
    EXPECT_EQ(uOutcome.error().Code(), "EncryptionClientError");

    auto uOutcomeCallable = disableClient.UploadPartCopyCallable(UploadPartCopyRequest(BucketName, "key"), ctx);
    uOutcome = uOutcomeCallable.get();
    EXPECT_EQ(uOutcome.isSuccess(), false);
    EXPECT_EQ(uOutcome.error().Code(), "EncryptionClientError");

    auto rOutcome = disableClient.ResumableCopyObject(MultiCopyObjectRequest(BucketName, "key", "", ""));
    EXPECT_EQ(rOutcome.isSuccess(), false);
    EXPECT_EQ(rOutcome.error().Code(), "EncryptionClientError");

    auto gOutcome = disableClient.GetObjectByUrl(GetObjectByUrlRequest("url"));
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "EncryptionClientError");

    auto pOutcome = disableClient.PutObjectByUrl(PutObjectByUrlRequest("key", content));
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "EncryptionClientError");


    UploadPartCopyAsyncHandler handler = UploadPartCopyDisableFuncHandler;
    UploadPartCopyRequest cRequest(BucketName, "key");
    std::shared_ptr<UploadPartAsyncContext> cContext = std::make_shared<UploadPartAsyncContext>();
    cContext->setUuid("UploadPartCopyAsync");
    disableClient.UploadPartCopyAsync(cRequest, handler, ctx, cContext);
    {
        std::unique_lock<std::mutex> lck(cContext->mtx);
        if (!cContext->ready) cContext->cv.wait(lck);
    }
}

}
}