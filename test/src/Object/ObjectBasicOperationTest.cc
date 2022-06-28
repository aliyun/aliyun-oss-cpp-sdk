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
#include <src/utils/Utils.h>
#include <src/utils/FileSystemUtils.h>

namespace AlibabaCloud {
namespace OSS {

class ObjectBasicOperationTest : public ::testing::Test {
protected:
    ObjectBasicOperationTest()
    {
    }

    ~ObjectBasicOperationTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectbasicoperation");
        Client->CreateBucket(CreateBucketRequest(BucketName));
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase() 
    {
        TestUtils::CleanBucketsByPrefix(*Client, BucketName);
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

std::shared_ptr<OssClient> ObjectBasicOperationTest::Client = nullptr;
std::string ObjectBasicOperationTest::BucketName = "";


TEST_F(ObjectBasicOperationTest, InvalidBucketNameTest)
{
    auto content = TestUtils::GetRandomStream(100);
    for (auto const& invalidBucketName : TestUtils::InvalidBucketNamesList()) {
        auto outcome = Client->PutObject(invalidBucketName, "InvalidBucketNameTest", content);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(ObjectBasicOperationTest, InvalidObjectKeyTest)
{
    auto content = TestUtils::GetRandomStream(100);
    for (auto const& invalidKeyName : TestUtils::InvalidObjectKeyNamesList()) {
        auto outcome = Client->PutObject(BucketName, invalidKeyName, content);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(ObjectBasicOperationTest, UserMetaDataTest)
{
    ObjectMetaData meta;
    meta.addHeader("x-oss-copy-source", "11111");
    meta.addHeader("x-oss-copy-source-if-match", "22222");
    meta.addHeader("x", "aaaaaa");
    meta.addHeader("x1", "bbb");
    meta.addHeader("aa", "aaaaa");
    meta.addHeader("AA", "bbbbb");
    meta.addHeader("ab", "aaaa");
    meta.addHeader("Ab", "bbbb");
    EXPECT_EQ(meta.HttpMetaData().size(), 6UL);

    static const char *keys[] = { "aa", "ab", "x", "x-oss-copy-source", "x-oss-copy-source-if-match", "x1" };
    static const char *values[] = { "bbbbb", "bbbb", "aaaaaa", "11111", "22222", "bbb" };

    int i = 0;
    for (auto const &header : meta.HttpMetaData()) {
        EXPECT_STREQ(header.first.c_str(), keys[i]);
        EXPECT_STREQ(header.second.c_str(), values[i]);
        i++;
    }
}

TEST_F(ObjectBasicOperationTest, ListAllObjectsTest)
{
    //create test file
    for (int i = 0; i < 20; i++) {
        std::string key = TestUtils::GetObjectKey("ListAllObjectsTest");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    //list object use default
    auto listOutcome = Client->ListObjects(BucketName);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().ObjectSummarys().size(), 20UL);
    EXPECT_EQ(listOutcome.result().IsTruncated(), false);

    int i = 0;
    for (auto const &obj : listOutcome.result().ObjectSummarys()) {
        EXPECT_EQ(obj.Size(), 100LL);
        EXPECT_EQ(obj.StorageClass(), "Standard");
        i++;
    }
    EXPECT_EQ(i, 20);

}

TEST_F(ObjectBasicOperationTest, ListObjectsWithPrefixTest)
{
    //create test file
    for (int i = 0; i < 30; i++) {
        std::string key = TestUtils::GetObjectKey("ListObjectsWithPrefixTest");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    //list object by prefix
    ListObjectsRequest request(BucketName);
    request.setMaxKeys(2);
    request.setPrefix("ListObjectsWithPrefixTest");

    bool IsTruncated = false;
    size_t total = 0;
    do {
        auto outcome = Client->ListObjects(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
        total += outcome.result().ObjectSummarys().size();
    } while (IsTruncated);

    EXPECT_EQ(30UL, total);

    auto lOutcome = Client->ListObjects(BucketName, "ListObjectsWithPrefixTest");
    EXPECT_EQ(lOutcome.isSuccess(), true);
    EXPECT_EQ(lOutcome.result().ObjectSummarys().size(), 30UL);
}

TEST_F(ObjectBasicOperationTest, ListObjectsWithIllegalMaxKeys)
{
    ListObjectsRequest request(BucketName);
    request.setMaxKeys(1000 + 1);
    auto outcome = Client->ListObjects(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidArgument");
}

TEST_F(ObjectBasicOperationTest, ListObjectsWithDelimiterTest)
{
    std::string folder = TestUtils::GetObjectKey("ListObjectsWithDelimiterTest").append("folder/");
    for (int i = 0; i < 5; i++) {
        std::string key = folder;
        key.append(std::to_string(i)).append("-empty.txt");
        std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>();
        auto outcome = Client->PutObject(BucketName, key, ss);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    std::string folder2 = TestUtils::GetObjectKey("ListObjectsWithDelimiterTest").append("folder/");
    for (int i = 0; i < 5; i++) {
        std::string key = folder2;
        std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>();
        key.append(std::to_string(i)).append("-empty.txt");
        auto outcome = Client->PutObject(BucketName, key, ss);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    //list object by prefix
    ListObjectsRequest request(BucketName);
    request.setPrefix("ListObjectsWithDelimiterTest");
    request.setDelimiter("/");
    auto outcome = Client->ListObjects(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CommonPrefixes().size(), 2UL);
}

TEST_F(ObjectBasicOperationTest, ListAllObjectsCallableTest)
{
    //create test file
    for (int i = 0; i < 20; i++) {
        std::string key = TestUtils::GetObjectKey("ListAllObjectsCallableTest");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    //list object use default
    ListObjectsRequest request(BucketName);
    request.setPrefix("ListAllObjectsCallableTest");
    auto listOutcomeCallable = Client->ListObjectsCallable(request);
    auto listOutcome = listOutcomeCallable.get();
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().ObjectSummarys().size(), 20UL);
    EXPECT_EQ(listOutcome.result().IsTruncated(), false);

    int i = 0;
    for (auto const &obj : listOutcome.result().ObjectSummarys()) {
        EXPECT_EQ(obj.Size(), 100LL);
        i++;
    }
    EXPECT_EQ(i, 20);

}

TEST_F(ObjectBasicOperationTest, ListObjectsNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-listobject");
    auto outcome = Client->ListObjects(name);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

/*Get Object Test*/
TEST_F(ObjectBasicOperationTest, GetAndDeleteNonExistObjectTest)
{
    std::string key = TestUtils::GetObjectKey("GetAndDeleteNonExistObjectTest");
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), false);
    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), false);
    EXPECT_EQ(outome.error().Code(), "NoSuchKey");

    auto dOutcome = Client->DeleteObject(BucketName, key);
    EXPECT_EQ(dOutcome.isSuccess(), true);
}

TEST_F(ObjectBasicOperationTest, GetObjectBasicTest)
{
    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outome   = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    auto fOutcome = Client->GetObject(BucketName, key, tmpFile);
    EXPECT_EQ(fOutcome.isSuccess(), true);
    fOutcome = dummy;

    std::shared_ptr<std::iostream> fileContent = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);

    std::string oriMd5  = ComputeContentMD5(*content.get());
    std::string memMd5  = ComputeContentMD5(*outome.result().Content().get());
    std::string fileMd5 = ComputeContentMD5(*fileContent.get());

    EXPECT_STREQ(oriMd5.c_str(), memMd5.c_str());
    EXPECT_STREQ(oriMd5.c_str(), fileMd5.c_str());
    fileContent = nullptr;
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectBasicOperationTest, GetObjectToFileTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto fOutcome = Client->GetObject(BucketName, key, tmpFile);
    EXPECT_EQ(fOutcome.isSuccess(), true);
    //EXPECT_EQ(RemoveFile(tmpFile), false);

    auto fileETag = TestUtils::GetFileETag(tmpFile);

    EXPECT_EQ(fileETag, pOutcome.result().ETag());

    fOutcome.result().setContent(std::shared_ptr<std::iostream>());

    EXPECT_EQ(RemoveFile(tmpFile), true);
}


TEST_F(ObjectBasicOperationTest, GetObjectToNullContentTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToNullContentTest");
    std::shared_ptr<std::iostream> content = nullptr;
    auto outome = Client->GetObject(BucketName, key, content);
    EXPECT_EQ(outome.isSuccess(), false);
}

TEST_F(ObjectBasicOperationTest, GetObjectToFailContentTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToFailContentTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicNegativeTest").append(".tmp");

    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(tmpFile, std::ios::in| std::ios::binary);
    auto outome = Client->GetObject(BucketName, key, content);
    EXPECT_EQ(outome.isSuccess(), false);
}

TEST_F(ObjectBasicOperationTest, GetObjectToBadContentTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectToBadContentTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectBasicNegativeTest").append(".tmp");

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    content->setstate(content->badbit);
    auto outome = Client->GetObject(BucketName, key, content);
    EXPECT_EQ(outome.isSuccess(), false);
}

TEST_F(ObjectBasicOperationTest, GetObjectToBadKeyTest)
{
    std::string key = "/InvalidObjectName";
    auto outcome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outcome.isSuccess(), false);
}

TEST_F(ObjectBasicOperationTest, GetObjectUsingRangeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectUsingRangeTest");
    auto content = TestUtils::GetRandomStream(1024);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest request(BucketName, key);
    request.setRange(10, 19);
    auto outome = Client->GetObject(request);
    EXPECT_EQ(outome.isSuccess(), true);

    char buffer[10];
    content->clear();
    content->seekg(10, content->beg);
    EXPECT_EQ(content->good(), true);
    content->read(buffer, 10);

    std::string oriMd5 = ComputeContentMD5(buffer, 10);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content().get());

    EXPECT_STREQ(oriMd5.c_str(), memMd5.c_str());
}

TEST_F(ObjectBasicOperationTest, GetObjectUsingRangeNegativeTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectMatchingETagPositiveTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectMatchingETagsPositiveTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectMatchingETagNegativeTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectModifiedSincePositiveTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectModifiedSinceNegativeTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectNonMatchingETagPositiveTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectNonMatchingETagsPositiveTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectNonMatchingETagNegativeTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectUnmodifiedSincePositiveTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectUnmodifiedSinceNegativeTest)
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

TEST_F(ObjectBasicOperationTest, GetObjectWithResponseHeadersSettingTest)
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
    GetObjectAsyncContex():ready(false) {}
    ~GetObjectAsyncContex() {}
    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    mutable std::string md5;
    mutable bool ready;
};

void GetObjectHandler(const AlibabaCloud::OSS::OssClient* client, 
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

TEST_F(ObjectBasicOperationTest, GetObjectAsyncBasicTest)
{
    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectAsyncBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(102400);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectAsyncHandler handler = GetObjectHandler;
    GetObjectRequest request(BucketName, key);
    std::shared_ptr<GetObjectAsyncContex> memContext = std::make_shared<GetObjectAsyncContex> ();

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

TEST_F(ObjectBasicOperationTest, GetObjectCallableBasicTest)
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

    auto memOutcomeCallable  = Client->GetObjectCallable(request);
    auto fileOutcomeCallable = Client->GetObjectCallable(fileRequest);


    std::cout << "Client[" << Client << "]" << "Issue GetObjectCallable done." << std::endl;

    auto fileOutcome = fileOutcomeCallable.get();
    auto memOutcome  = memOutcomeCallable.get();
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

TEST_F(ObjectBasicOperationTest, PutObjectBasicTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectBasicTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() == nullptr);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(ObjectBasicOperationTest, PutObjectWithExpiresTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithExpiresTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    request.MetaData().setContentType("application/x-test");
    request.setExpires(TestUtils::GetGMTString(120));
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(ObjectBasicOperationTest, PutObjectUsingContentLengthTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUsingContentLengthTest");
    std::shared_ptr<std::stringstream> content = std::make_shared<std::stringstream>();
    *content << "123456789";

    PutObjectRequest request(BucketName, key, content);
    request.MetaData().setContentLength(2);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);
 
    std::string oriMd5 = ComputeContentMD5("12", 2);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content().get());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(ObjectBasicOperationTest, PutObjectFullSettingsTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectFullSettingsTest");
    auto content = TestUtils::GetRandomStream(1024);
    key.append("/attachement_test.data");

    std::string saveAs = "abc����123.zip";
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
    request.MetaData().UserMetaData()["MyKey3"] = "��������";

    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto metaOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(metaOutcome.isSuccess(), true);

    EXPECT_EQ(metaOutcome.result().CacheControl(), "no-cache");
    EXPECT_EQ(metaOutcome.result().ContentDisposition(), contentDisposition);
    EXPECT_EQ(metaOutcome.result().ContentEncoding(), "gzip");

    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey1"), "MyValue1");
    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey2"), "MyValue2");
    EXPECT_EQ(metaOutcome.result().UserMetaData().at("MyKey3"), "��������");
}

TEST_F(ObjectBasicOperationTest, PutObjectDefaultMetadataTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectDefaultMetadataTest");
    auto content = TestUtils::GetRandomStream(1024);

    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);
}

TEST_F(ObjectBasicOperationTest, PutObjectFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);

    auto pOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    std::fstream file(tmpFile, std::ios::in | std::ios::binary);
    std::string oriMd5 = ComputeContentMD5(file);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
    file.close();
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectBasicOperationTest, PutObjectUsingContentLengthFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUsingContentLengthFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectUsingContentLengthFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream> (tmpFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(file->good(), true);

    file->seekg(0, file->end);
    auto content_length = file->tellg();
    EXPECT_EQ(content_length, 1024LL);

    file->seekg(content_length / 2, file->beg);

    PutObjectRequest request(BucketName, key, file);
    request.MetaData().setContentLength(content_length / 2);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    char buff[2048];
    file->clear();
    file->seekg(content_length / 2, file->beg);
    file->read(buff, 2048);
    size_t readSize = static_cast<size_t>(file->gcount());

    std::string oriMd5 = ComputeContentMD5(buff, readSize);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content().get());
    EXPECT_EQ(oriMd5, memMd5);
    file->close();
    RemoveFile(tmpFile);
}

TEST_F(ObjectBasicOperationTest, PutObjectFullSettingsFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectFullSettingsFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFullSettingsFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(file->good(), true);
    key.append("/attachement_test.data");

    std::string saveAs = "abc����123.zip";
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

TEST_F(ObjectBasicOperationTest, PutObjectDefaultMetadataFromFileTest)
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

TEST_F(ObjectBasicOperationTest, PutObjectUserMetadataFromFileContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUserMetadataFromFileContentTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectUserMetadataFromFileContentTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
    EXPECT_EQ(file->good(), true);

    ObjectMetaData metaData;
    metaData.UserMetaData()["test"] = "testvalue";
    auto outcome = Client->PutObject(BucketName, key, file, metaData);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "testvalue");
    file->close();
    RemoveFile(tmpFile);
}

TEST_F(ObjectBasicOperationTest, PutObjectUserMetadataFromFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectUserMetadataFromFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectUserMetadataFromFileTest").append("-put.tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);

    ObjectMetaData metaData;
    metaData.UserMetaData()["test"] = "testvalue";
    auto outcome = Client->PutObject(BucketName, key, tmpFile, metaData);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "testvalue");
    EXPECT_EQ(hOutcome.result().ContentLength(), 1024LL);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectBasicOperationTest, PutObjectWithNotExistFileTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithNotExistFileTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFileTest").append("-put.tmp");

    auto pOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(pOutcome.error().Message(), "Request body is in fail state. Logical error on i/o operation.");
}

TEST_F(ObjectBasicOperationTest, PutObjectWithNullContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithNullContentTest");
    std::shared_ptr<std::iostream> content = nullptr;

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(pOutcome.error().Message(), "Request body is null.");
}

TEST_F(ObjectBasicOperationTest, PutObjectWithBadContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithNullContentTest");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    content->setstate(content->badbit);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "ValidateError");
    EXPECT_EQ(pOutcome.error().Message(), "Request body is in bad state. Read/writing error on i/o operation.");
}

TEST_F(ObjectBasicOperationTest, PutObjectWithSameContentTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithSameContentTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content());
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

void PutObjectHandler(const AlibabaCloud::OSS::OssClient* client,
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

TEST_F(ObjectBasicOperationTest, PutObjectAsyncBasicTest)
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

TEST_F(ObjectBasicOperationTest, PutObjectCallableBasicTest)
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
    //EXPECT_EQ(RemoveFile(tmpFile), true);
    RemoveFile(tmpFile);
}

TEST_F(ObjectBasicOperationTest, ListObjectsResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                        <Name>oss-example</Name>
                        <Prefix></Prefix>
                        <Marker></Marker>
                        <MaxKeys>100</MaxKeys>
                        <Delimiter></Delimiter>
                        <IsTruncated>false</IsTruncated>
                        <Contents>
                            <Key>fun/movie/001.avi</Key>
                            <LastModified>2012-02-24T08:43:07.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>5368709120</Size>
                            <StorageClass>Standard</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key>fun/movie/007.avi</Key>
                            <LastModified>2012-02-24T08:43:27.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>344606</Size>
                            <StorageClass>IA</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key>fun/test.jpg</Key>
                            <LastModified>2012-02-24T08:42:32.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>344606</Size>
                            <StorageClass>Standard</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key>oss.jpg</Key>
                            <LastModified>2012-02-24T06:07:48.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>344606</Size>
                            <StorageClass>Standard</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                    </ListBucketResult>)";
    ListObjectsResult result(xml);
    EXPECT_EQ(result.ObjectSummarys().size(), 4UL);
    EXPECT_EQ(result.ObjectSummarys()[0].ETag(), "5B3C1A2E053D763E1B002CC607C5A0FE");
    EXPECT_EQ(result.ObjectSummarys()[0].Size(), 5368709120LL);
    EXPECT_EQ(result.ObjectSummarys()[0].StorageClass(), "Standard");
    EXPECT_EQ(result.ObjectSummarys()[1].StorageClass(), "IA");
}

TEST_F(ObjectBasicOperationTest, ListObjectsResultWithEncodingType)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                        <Name>oss-example</Name>
                        <Prefix>hello%20world%21</Prefix>
                        <Marker>hello%20</Marker>
                        <MaxKeys>100</MaxKeys>
                        <Delimiter>hello%20%21world</Delimiter>
                        <IsTruncated>false</IsTruncated>
                        <EncodingType>url</EncodingType>
                        <Contents>
                            <Key>fun/movie/001.avi</Key>
                            <LastModified>2012-02-24T08:43:07.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>344606</Size>
                            <StorageClass>Standard</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key>fun/movie/007.avi</Key>
                            <LastModified>2012-02-24T08:43:27.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>344606</Size>
                            <StorageClass>Standard</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key>fun/test.jpg</Key>
                            <LastModified>2012-02-24T08:42:32.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>344606</Size>
                            <StorageClass>Standard</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key>oss.jpg</Key>
                            <LastModified>2012-02-24T06:07:48.000Z</LastModified>
                            <ETag>&quot;5B3C1A2E053D763E1B002CC607C5A0FE&quot;</ETag>
                            <Type>Normal</Type>
                            <Size>344606</Size>
                            <StorageClass>Standard</StorageClass>
                            <Owner>
                                <ID>00220120222</ID>
                                <DisplayName>user-example</DisplayName>
                            </Owner>
                        </Contents>
                    </ListBucketResult>)";
    ListObjectsResult result(xml);
    EXPECT_EQ(result.ObjectSummarys().size(), 4UL);
    EXPECT_EQ(result.ObjectSummarys()[0].ETag(), "5B3C1A2E053D763E1B002CC607C5A0FE");
    EXPECT_EQ(result.Marker(), "hello ");
    EXPECT_EQ(result.Prefix(), "hello world!");
    EXPECT_EQ(result.Delimiter(), "hello !world");
}


TEST_F(ObjectBasicOperationTest, DeleteObjectsResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                        <Deleted>
                           <Key>multipart.data</Key>
                        </Deleted>
                        <Deleted>
                           <Key>test.jpg</Key>
                        </Deleted>
                        <Deleted>
                           <Key>demo.jpg</Key>
                        </Deleted>
                    </DeleteResult>)";
    DeleteObjectsResult result(xml);
    EXPECT_EQ(result.keyList().size(), 3UL);
    EXPECT_EQ(result.Quiet(), false);
}

TEST_F(ObjectBasicOperationTest, DeleteObjectsResultWithEmtpy)
{
    std::string xml = "";
    DeleteObjectsResult result(xml);
    EXPECT_EQ(result.keyList().size(), 0UL);
    EXPECT_EQ(result.Quiet(), true);
}

TEST_F(ObjectBasicOperationTest, DeleteObjectsBasicTest)
{
    const size_t TestKeysCnt = 10;
    auto keyPrefix = TestUtils::GetObjectKey("DeleteObjectsBasicTest");

    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        auto content = TestUtils::GetRandomStream(100);
        key.append(std::to_string(i)).append(".txt");
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    DeleteObjectsRequest delRequest(BucketName);
    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    auto delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), TestKeysCnt);
    EXPECT_EQ(delOutcome.result().Quiet(), false);
}

TEST_F(ObjectBasicOperationTest, DeleteObjectsQuietTest)
{
    const size_t TestKeysCnt = 10;
    auto keyPrefix = TestUtils::GetObjectKey("DeleteObjectsQuietTest");

    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        auto content = TestUtils::GetRandomStream(100);
        key.append(std::to_string(i)).append(".txt");
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    DeleteObjectsRequest delRequest(BucketName);
    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    delRequest.setQuiet(true);
    EXPECT_EQ(delRequest.Quiet(), true);
    auto delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), 0UL);
    EXPECT_EQ(delOutcome.result().Quiet(), true);
}


TEST_F(ObjectBasicOperationTest, DeleteObjectsByStepTest)
{
    const size_t TestKeysCnt = 10;
    EXPECT_TRUE(TestKeysCnt > 8);
    auto keyPrefix = TestUtils::GetObjectKey("DeleteObjectsByStepTest");

    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        auto content = TestUtils::GetRandomStream(100);
        key.append(std::to_string(i)).append(".txt");
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    ListObjectsRequest lRequest(BucketName);
    lRequest.setPrefix(keyPrefix);
    auto lOutcome = Client->ListObjects(lRequest);
    EXPECT_EQ(lOutcome.isSuccess(), true);
    EXPECT_EQ(lOutcome.result().ObjectSummarys().size(), TestKeysCnt);

    //Delete 0, 1 objects
    DeleteObjectsRequest delRequest(BucketName);
    for (size_t i = 0; i < 2; i++) {
        auto key = keyPrefix;
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    EXPECT_EQ(delRequest.KeyList().size(), 2UL);

    auto delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), 2UL);

    //delete  2, 3, 4
    delRequest.clearKeyList();
    for (int i = 2; i < 5; i++) {
        auto key = keyPrefix;
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), 3UL);

    DeletedKeyList keyList;
    for (size_t i = 5; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        key.append(std::to_string(i)).append(".txt");
        keyList.push_back(key);
    }
    delRequest.setKeyList(keyList);
    delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), (TestKeysCnt-5));

    lOutcome = Client->ListObjects(lRequest);
    EXPECT_EQ(lOutcome.result().ObjectSummarys().size(), 0UL);
}

TEST_F(ObjectBasicOperationTest, DeleteObjectsWithEncodingTypeTest)
{
    const size_t TestKeysCnt = 10;
    auto keyPrefix = TestUtils::GetObjectKey("DeleteObjectsWithEncodingTypeTest");

    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        auto content = TestUtils::GetRandomStream(100);
        key.append(std::to_string(i)).append(".txt");
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    DeleteObjectsRequest delRequest(BucketName);
    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    delRequest.setEncodingType("url");
    EXPECT_EQ(delRequest.EncodingType(), "url");

    auto delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), TestKeysCnt);
    EXPECT_EQ(delOutcome.result().Quiet(), false);
}

TEST_F(ObjectBasicOperationTest, DeleteObjectsInvalidBucketNameTest)
{
    DeletedKeyList keyList;
    keyList.push_back("key");
    auto outcome = Client->DeleteObjects("InvalidBucketName", keyList);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(ObjectBasicOperationTest, DeleteObjectInvalidBucketNameTest)
{
    auto outcome = Client->DeleteObject("InvalidBucketName", "key");
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(ObjectBasicOperationTest, DeleteObjectInvalidKeyTest)
{
    auto outcome = Client->DeleteObject("bucketname", "");
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(ObjectBasicOperationTest, PutObjectMultiTimesUseTheSameContentTest)
{
    const size_t TestKeysCnt = 5;
    auto keyPrefix = TestUtils::GetObjectKey("PutObjectMultiTimesUseTheSameContentTest");

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "123456789";

    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        content->clear();
        content->seekg(0, content->beg);
        key.append(std::to_string(i)).append(".txt");
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }
}

TEST_F(ObjectBasicOperationTest, HeadObjectsNegativeTest)
{
    auto outcome = Client->HeadObject("no-exist-bucket", "object");
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:404");

    outcome = Client->HeadObject(BucketName, "object");
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:404");
    EXPECT_EQ(outcome.error().RequestId().empty(), false);
}

TEST_F(ObjectBasicOperationTest, ObjectMetaDataDefaultTest)
{
    ObjectMetaData meta;
    EXPECT_EQ(meta.LastModified(), "");
    EXPECT_EQ(meta.ExpirationTime(), "");
    EXPECT_EQ(meta.ContentLength(), -1LL);
    EXPECT_EQ(meta.ContentType(), "");
    EXPECT_EQ(meta.ContentEncoding(), "");
    EXPECT_EQ(meta.CacheControl(), "");
    EXPECT_EQ(meta.ContentDisposition(), "");
    EXPECT_EQ(meta.ETag(), "");
    EXPECT_EQ(meta.ContentMd5(), "");
    EXPECT_EQ(meta.CRC64(), 0ULL);
    EXPECT_EQ(meta.ObjectType(), "");
}

TEST_F(ObjectBasicOperationTest, ObjectMetaDataSetTest)
{
    ObjectMetaData meta;

    meta.setCacheControl("No-Cache");
    meta.setExpirationTime("Fri, 09 Nov 2018 05:57:16 GMT");
    meta.setContentLength(10000LL);
    meta.setContentType("application/xml");
    meta.setContentEncoding("url");
    meta.setContentDisposition(".zip");
    meta.setETag("ETAG");
    meta.setContentMd5("MD5");
    meta.setCrc64(1000ULL);
    EXPECT_EQ(meta.CacheControl(), "No-Cache");
    EXPECT_EQ(meta.ExpirationTime(), "Fri, 09 Nov 2018 05:57:16 GMT");
    EXPECT_EQ(meta.ContentLength(), 10000LL);
    EXPECT_EQ(meta.ContentType(), "application/xml");
    EXPECT_EQ(meta.ContentEncoding(), "url");
    EXPECT_EQ(meta.ContentDisposition(), ".zip");
    EXPECT_EQ(meta.ETag(), "ETAG");
    EXPECT_EQ(meta.ContentMd5(), "MD5");
    EXPECT_EQ(meta.CRC64(), 1000ULL);
}

TEST_F(ObjectBasicOperationTest, GetObjectResultTest)
{
    std::string bucketName = TestUtils::GetBucketName("get-object-result-test");
    std::string key = TestUtils::GetObjectKey("GetObjectResultTestKey");
    ObjectMetaData meta;

    GetObjectResult result(bucketName, key, meta);
    EXPECT_EQ(result.RequestId(), "");
}

TEST_F(ObjectBasicOperationTest, UtilsfunctionTest)
{
    auto md5 = ComputeContentMD5("test");
    std::string invalidKey;
    IsValidTagKey(invalidKey);
    ToRequestPayer(invalidKey.c_str());

    DeleteObjectsResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <Delete xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">

                    </Delete>)";
    DeleteObjectsResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                        <EncodingType></EncodingType>
                        <Deleted>
                        </Deleted>
                        <Deleted>
                           <Key></Key>
                        </Deleted>
                    </DeleteResult>)";
    DeleteObjectsResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                       
                    </DeleteResult>)";
    DeleteObjectsResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    DeleteObjectsResult result4(xml);

}

TEST_F(ObjectBasicOperationTest, ListObjectsResultBranchTest)
{
    ListObjectsResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListBucket xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">

                    </ListBucket>)";
    ListObjectsResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                        
                    </ListBucketResult>)";
    ListObjectsResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">

                        <Contents>

                        </Contents>
                    </ListBucketResult>)";
    ListObjectsResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                        <Name></Name>
                        <Prefix></Prefix>
                        <Marker></Marker>
                        <MaxKeys></MaxKeys>
                        <Delimiter></Delimiter>
                        <IsTruncated></IsTruncated>
                        <NextMarker></NextMarker>
                        <EncodingType></EncodingType>
                        <CommonPrefixes>
                        <Prefix></Prefix>
                        </CommonPrefixes>
                        <Contents>
                            <Key></Key>
                            <LastModified></LastModified>
                            <ETag></ETag>
                            <Type></Type>
                            <Size></Size>
                            <StorageClass></StorageClass>
                            <Owner>
                                <ID></ID>
                                <DisplayName></DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key></Key>
                            <LastModified></LastModified>
                            <ETag></ETag>
                            <Type></Type>
                            <Size></Size>
                            <StorageClass></StorageClass>
                            <Owner>
                                <ID></ID>
                                <DisplayName></DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key></Key>
                            <LastModified></LastModified>
                            <ETag></ETag>
                            <Type></Type>
                            <Size></Size>
                            <StorageClass></StorageClass>
                            <Owner>
                                <ID></ID>
                                <DisplayName></DisplayName>
                            </Owner>
                        </Contents>
                        <Contents>
                            <Key></Key>
                            <LastModified></LastModified>
                            <ETag></ETag>
                            <Type></Type>
                            <Size></Size>
                            <StorageClass></StorageClass>
                            <Owner>
                                <ID></ID>
                                <DisplayName></DisplayName>
                            </Owner>
                        </Contents>
                    </ListBucketResult>)";
    ListObjectsResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    ListObjectsResult result5(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                        <CommonPrefixes>
                        </CommonPrefixes>
                        <Contents>
                            <Key></Key>
                            <LastModified></LastModified>
                            <ETag></ETag>
                            <Type></Type>
                            <Size></Size>
                            <StorageClass></StorageClass>
                            <Owner>
                            </Owner>
                        </Contents>
                    </ListBucketResult>)";
    ListObjectsResult result6(xml);

}

TEST_F(ObjectBasicOperationTest, PutObjectResultBranchTest)
{
    HeaderCollection header;
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "test";
    PutObjectResult result(header, content);
}

TEST_F(ObjectBasicOperationTest, GetObjectWithOssDateHeaderTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectWithOssDateHeaderTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    std::time_t t = std::time(nullptr);
    request.MetaData().addHeader("x-oss-date", ToGmtTime(t));
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
}

TEST_F(ObjectBasicOperationTest, DeleteObjectsWithSpecialCharsTest)
{
    std::string key = "bswodnvsttpqvnzwsgifetwe\n\n\t@#$!\t<!@#$%^&*()-=";
    for (int i = 1; i < 0x1F; i++)
        key.push_back((char)i);
    key.append("--");

    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    DeleteObjectsRequest delRequest(BucketName);
    delRequest.addKey(key);
    auto delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), 1U);
    EXPECT_EQ(delOutcome.result().Quiet(), false);

    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), false);
}

TEST_F(ObjectBasicOperationTest, DeleteObjectsWithInvalidResponseBodyTest)
{
    std::string key = "bswodnvsttpqvnzwsgifetwe\n\n\t@#$!\t<!@#$%^&*()-=";
    for (int i = 1; i < 0x1F; i++)
        key.push_back((char)i);
    key.append("--");

    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    DeleteObjectsRequest delRequest(BucketName);
    delRequest.addKey(key);
    delRequest.setResponseStreamFactory([=]() {
        auto data = std::make_shared<std::stringstream>();
        data->write("invlid data", 11);
        return data;
    });    
    auto delOutcome = Client->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), false);
    EXPECT_EQ(delOutcome.error().Code(), "ParseXMLError");
}

TEST_F(ObjectBasicOperationTest, ListObjectsWithInvalidResponseBodyTest)
{
    std::string key = TestUtils::GetObjectKey("ListObjectsWithInvalidResponseBodyTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);

    ListObjectsRequest lsRequest(BucketName);
    lsRequest.setResponseStreamFactory([=]() {
        auto data = std::make_shared<std::stringstream>();
        data->write("invlid data", 11);
        return data;
    });
    auto listOutcome = Client->ListObjects(lsRequest);
    EXPECT_EQ(listOutcome.isSuccess(), false);
    EXPECT_EQ(listOutcome.error().Code(), "ParseXMLError");
}

TEST_F(ObjectBasicOperationTest, GetObjectRequestStandardModeTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectRequestStandardModeTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->PutObject(BucketName, key, content);

    auto getRequet = GetObjectRequest(BucketName, key);
    getRequet.setRange(10, 200);
    auto getOutcome = Client->GetObject(getRequet);
    EXPECT_EQ(getOutcome.result().Metadata().ContentLength(), 100);

    getRequet = GetObjectRequest(BucketName, key);
    getRequet.setRange(10, 200, true);
    getOutcome = Client->GetObject(getRequet);
    EXPECT_EQ(getOutcome.result().Metadata().ContentLength(), 90);

    std::vector<std::string> etags;
    std::map<std::string, std::string> maps;
    getRequet = GetObjectRequest(BucketName, key, "", "", etags, etags, maps);
    getRequet.setRange(10, 200);
    getOutcome = Client->GetObject(getRequet);
    EXPECT_EQ(getOutcome.result().Metadata().ContentLength(), 100);

    getRequet = GetObjectRequest(BucketName, key, "", "", etags, etags, maps);
    getRequet.setRange(10, 200, true);
    getOutcome = Client->GetObject(getRequet);
    EXPECT_EQ(getOutcome.result().Metadata().ContentLength(), 90);
}

//listobject2V2
TEST_F(ObjectBasicOperationTest, ListObjectsV2Test)
{
    auto bucketName = BucketName + "-v2";
    Client->CreateBucket(CreateBucketRequest(bucketName));

    std::vector<std::string> keyArray;
    //create test file
    for (int i = 0; i < 10; i++) {
        std::string key = TestUtils::GetObjectKey("folder/ListAllObjectsV2Test");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(bucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
        keyArray.push_back(key);
    }

    for (int i = 0; i < 8; i++) {
        std::string key = TestUtils::GetObjectKey("sub/ListAllObjectsV2Test");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(bucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    for (int i = 0; i < 5; i++) {
        std::string key = TestUtils::GetObjectKey("list/ListAllObjectsV2Test");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(bucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    //list object use default
    auto request = ListObjectsV2Request(bucketName);
    auto listOutcome = Client->ListObjectsV2(request);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().ObjectSummarys().size(), 23UL);
    EXPECT_EQ(listOutcome.result().KeyCount(), 23L);
    EXPECT_EQ(listOutcome.result().MaxKeys(), 100L);
    EXPECT_EQ(listOutcome.result().Prefix(), "");
    EXPECT_EQ(listOutcome.result().Delimiter(), "");
    EXPECT_EQ(listOutcome.result().IsTruncated(), false);

    int i = 0;
    for (auto const &obj : listOutcome.result().ObjectSummarys()) {
        EXPECT_EQ(obj.Size(), 100LL);
        i++;
    }
    EXPECT_EQ(i, 23L);

    //list object with prefix
    request = ListObjectsV2Request(bucketName);
    request.setMaxKeys(2);
    request.setPrefix("folder");

    bool IsTruncated = false;
    size_t total = 0;
    do {
        auto outcome = Client->ListObjectsV2(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        request.setContinuationToken(outcome.result().NextContinuationToken());
        IsTruncated = outcome.result().IsTruncated();
        total += outcome.result().ObjectSummarys().size();
        EXPECT_EQ(outcome.result().KeyCount(), 2L);
        EXPECT_EQ(outcome.result().MaxKeys(), 2L);
        EXPECT_EQ(outcome.result().Prefix(), "folder");
    } while (IsTruncated);

    EXPECT_EQ(total, 10UL);

    //with Delimiter
    request = ListObjectsV2Request(bucketName);
    request.setDelimiter("/");
    listOutcome = Client->ListObjectsV2(request);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().CommonPrefixes().size(), 3UL);
    EXPECT_EQ(listOutcome.result().KeyCount(), 3L);

    //start-After
    request = ListObjectsV2Request(bucketName);
    request.setStartAfter(keyArray.at(3));
    request.setPrefix("folder");
    listOutcome = Client->ListObjectsV2(request);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().KeyCount(), 6L);
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Key(), keyArray.at(4));
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Owner().DisplayName().empty(), true);
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Owner().Id().empty(), true);

    //Fetch Owner
    request = ListObjectsV2Request(bucketName);
    request.setStartAfter(keyArray.at(4));
    request.setPrefix("folder");
    request.setFetchOwner(true);
    listOutcome = Client->ListObjectsV2(request);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().KeyCount(), 5L);
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Key(), keyArray.at(5));
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Owner().DisplayName().empty(), false);
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Owner().Id().empty(), false);

    //encoding type
    request = ListObjectsV2Request(bucketName);
    request.setStartAfter(keyArray.at(5));
    request.setPrefix("folder/ListAllObjectsV2Test");
    request.setEncodingType("url");
    listOutcome = Client->ListObjectsV2(request);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().EncodingType(), "url");
    EXPECT_EQ(listOutcome.result().KeyCount(), 4L);
    EXPECT_EQ(listOutcome.result().Prefix(), "folder/ListAllObjectsV2Test");
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Key(), keyArray.at(6));
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Owner().DisplayName().empty(), true);
    EXPECT_EQ(listOutcome.result().ObjectSummarys()[0].Owner().Id().empty(), true);
}

TEST_F(ObjectBasicOperationTest, ListObjectsV2ResultTest)
{
    ListObjectsV2Result result("test");

    std::string xml = R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <ListBucket xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            </ListBucket>
            )";
    ListObjectsV2Result result1(xml);

    xml = R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            </ListBucketResult>
            )";
    ListObjectsV2Result result2(xml);

    xml = R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Contents>
            </Contents>
            </ListBucketResult>
            )";
    ListObjectsV2Result result3(xml);

    xml = R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Name></Name>
            <Prefix></Prefix>
            <StartAfter></StartAfter>
            <MaxKeys></MaxKeys>
            <Delimiter></Delimiter>
            <IsTruncated></IsTruncated>
            <NextContinuationToken></NextContinuationToken>
            <EncodingType></EncodingType>
            <CommonPrefixes>
            <Prefix></Prefix>
            </CommonPrefixes>
            <Contents>
                <Key></Key>
                <LastModified></LastModified>
                <ETag></ETag>
                <Type></Type>
                <Size></Size>
                <StorageClass></StorageClass>
                <Owner>
                    <ID></ID>
                    <DisplayName></DisplayName>
                </Owner>
            </Contents>
            <Contents>
                <Key></Key>
                <LastModified></LastModified>
                <ETag></ETag>
                <Type></Type>
                <Size></Size>
                <StorageClass></StorageClass>
                <Owner>
                    <ID></ID>
                    <DisplayName></DisplayName>
                </Owner>
            </Contents>
        </ListBucketResult>
        )";
    ListObjectsV2Result result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    ListObjectsV2Result result5(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <CommonPrefixes>
            </CommonPrefixes>
            <Contents>
                <Key></Key>
                <LastModified></LastModified>
                <ETag></ETag>
                <Type></Type>
                <Size></Size>
                <StorageClass></StorageClass>
                <Owner>
                </Owner>
            </Contents>
        </ListBucketResult>
        )";
    ListObjectsV2Result result6(xml);

    xml = R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Name>name</Name>
            <Prefix>prefix</Prefix>
            <StartAfter>start</StartAfter>
            <MaxKeys>20</MaxKeys>
            <Delimiter>/</Delimiter>
            <IsTruncated>true</IsTruncated>
            <NextContinuationToken>next</NextContinuationToken>
            <ContinuationToken>current</ContinuationToken>
            <EncodingType>type</EncodingType>
            <CommonPrefixes>
                <Prefix>com-prefix</Prefix>
            </CommonPrefixes>
            <Contents>
                <Key>key</Key>
                <LastModified>last</LastModified>
                <ETag>tag</ETag>
                <Type>type</Type>
                <Size>100</Size>
                <StorageClass>class</StorageClass>
                <Owner>
                    <ID>id</ID>
                    <DisplayName>display</DisplayName>
                </Owner>
            </Contents>
            <KeyCount>3</KeyCount>
        </ListBucketResult>
        )";
    ListObjectsV2Result result7(xml);
    EXPECT_EQ(result7.Name(), "name");
    EXPECT_EQ(result7.Prefix(), "prefix");
    EXPECT_EQ(result7.MaxKeys(), 20L);
    EXPECT_EQ(result7.Delimiter(), "/");
    EXPECT_EQ(result7.IsTruncated(), true);
    EXPECT_EQ(result7.NextContinuationToken(), "next");
    EXPECT_EQ(result7.ContinuationToken(), "current");
    EXPECT_EQ(result7.EncodingType(), "type");
    EXPECT_EQ(result7.CommonPrefixes().size(), 1UL);
    EXPECT_EQ(result7.CommonPrefixes()[0], "com-prefix");
    EXPECT_EQ(result7.ObjectSummarys().size(), 1UL);
    EXPECT_EQ(result7.ObjectSummarys()[0].Key(), "key");
    EXPECT_EQ(result7.ObjectSummarys()[0].LastModified(), "last");
    EXPECT_EQ(result7.ObjectSummarys()[0].ETag(), "tag");
    EXPECT_EQ(result7.ObjectSummarys()[0].Type(), "type");
    EXPECT_EQ(result7.ObjectSummarys()[0].Size(), 100LL);
    EXPECT_EQ(result7.ObjectSummarys()[0].StorageClass(), "class");
    EXPECT_EQ(result7.ObjectSummarys()[0].Owner().DisplayName(), "display");
    EXPECT_EQ(result7.ObjectSummarys()[0].Owner().Id(), "id");
}

}
}