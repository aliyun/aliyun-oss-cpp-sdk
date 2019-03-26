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
#include "src/utils/Utils.h"

namespace AlibabaCloud {
namespace OSS {

class HttpsTest : public ::testing::Test {
protected:
    HttpsTest()
    {
    }

    ~HttpsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        std::string endpoint = TestUtils::GetHTTPSEndpoint(Config::Endpoint);
        Client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-httpstest");
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

std::shared_ptr<OssClient> HttpsTest::Client = nullptr;
std::string HttpsTest::BucketName = "";


TEST_F(HttpsTest, CreateAndDeleteBucketTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-httpstest");

    //assert bucket does not exist
    EXPECT_EQ(Client->DoesBucketExist(bucketName), false);

    //create a new bucket
    Client->CreateBucket(CreateBucketRequest(bucketName));
    //TestUtils::WaitForCacheExpire(5);
    EXPECT_EQ(Client->DoesBucketExist(bucketName), true);

    //delete the bucket
    Client->DeleteBucket(DeleteBucketRequest(bucketName));
    TestUtils::WaitForCacheExpire(5);
    Client->DoesBucketExist(bucketName);
    TestUtils::WaitForCacheExpire(5);
    EXPECT_EQ(Client->DoesBucketExist(bucketName), false);
}

TEST_F(HttpsTest, GetBucketAclTest)
{
    auto outcome = Client->GetBucketAcl(BucketName);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Acl(), CannedAccessControlList::Private);

    Client->SetBucketAcl(BucketName, CannedAccessControlList::PublicRead);
    TestUtils::WaitForCacheExpire(8);
    outcome = Client->GetBucketAcl(BucketName);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Acl(), CannedAccessControlList::PublicRead);
}


TEST_F(HttpsTest, GetBucketInfoTest)
{
    auto outcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto aclOutcome = Client->GetBucketAcl(BucketName);
    EXPECT_EQ(aclOutcome.isSuccess(), true);

    EXPECT_EQ(outcome.result().Acl(), aclOutcome.result().Acl());
}

TEST_F(HttpsTest, GetBucketLocationTest)
{
    auto locOutcome = Client->GetBucketLocation(BucketName);
    EXPECT_EQ(locOutcome.isSuccess(), true);
    EXPECT_EQ(locOutcome.result().Location().compare(0,4,"oss-", 4), 0);
}

TEST_F(HttpsTest, GetBucketStatTest)
{
    //put object
    auto objectName = BucketName;
    objectName.append("firstobject");
    Client->PutObject(BucketName, objectName, std::make_shared<std::stringstream>("1234"));

    auto bsOutcome = Client->GetBucketStat(BucketName);
    EXPECT_EQ(bsOutcome.isSuccess(), true);
    EXPECT_EQ(bsOutcome.result().Storage(), 4ULL);
    EXPECT_EQ(bsOutcome.result().ObjectCount(), 1ULL);
    EXPECT_EQ(bsOutcome.result().MultipartUploadCount(), 0ULL);
}


TEST_F(HttpsTest, ListBucketspagingTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-httpstest");

    for (int i = 0; i < 5; i++)
    {
        auto name = bucketName;
        name.append("-").append(std::to_string(i));
        Client->CreateBucket(name);
    }

    //list all
    ListBucketsRequest request;
    request.setPrefix(bucketName);
    request.setMaxKeys(100);
    auto outcome = Client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Buckets().size(), 5UL);


    //list by step
    request.setMaxKeys(2);
    bool IsTruncated = false;
    size_t total = 0;
    do {
        outcome = Client->ListBuckets(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_LT(outcome.result().Buckets().size(), 3UL);
        total += outcome.result().Buckets().size();
        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
    } while (IsTruncated);
    EXPECT_EQ(total, 5UL);

    //delete all
    TestUtils::CleanBucketsByPrefix(*Client, bucketName);
}

TEST_F(HttpsTest, PutObjectTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectTest");
    auto content = TestUtils::GetRandomStream(1024);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);
}

TEST_F(HttpsTest, GetObjectTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectTest");
    auto content = TestUtils::GetRandomStream(1024);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto gOutcome = Client->GetObject(BucketName, key);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content.get());
    std::string memMd5 = ComputeContentMD5(*gOutcome.result().Content().get());

    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(HttpsTest, HeadObjectTest)
{
    std::string key = TestUtils::GetObjectKey("HeadObjectTest");
    auto content = TestUtils::GetRandomStream(1024);
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);

    EXPECT_EQ(outcome.result().ETag(), hOutcome.result().ETag());
}

TEST_F(HttpsTest, GetObjectMetaTest)
{
    std::string key = TestUtils::GetObjectKey("GetObjectMetaTest");
    auto content = TestUtils::GetRandomStream(1024);
    ObjectMetaData meta;
    meta.UserMetaData()["user"] = "test";
    auto outcome = Client->PutObject(BucketName, key, content, meta);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectMeta(BucketName, key);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().UserMetaData().find("user"), gOutcome.result().UserMetaData().end());

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().UserMetaData().at("user"), "test");
}

TEST_F(HttpsTest, ListObjectsTest)
{
    auto outcome = Client->ListObjects(BucketName);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ((outcome.result().ObjectSummarys().size() > 0UL), true);
}

TEST_F(HttpsTest, DeleteObjectsTest)
{
    std::string key = TestUtils::GetObjectKey("DeleteObjectsTest");
    auto content = TestUtils::GetRandomStream(100);
    Client->PutObject(BucketName, key, content);

    auto outcome = Client->ListObjects(BucketName);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ((outcome.result().ObjectSummarys().size() > 0UL), true);
    for (auto const &obj : outcome.result().ObjectSummarys()) {
        Client->DeleteObject(BucketName, obj.Key());
    }

    outcome = Client->ListObjects(BucketName);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().ObjectSummarys().size(), 0UL);
}

TEST_F(HttpsTest, GetPreSignedUriTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    std::string actualETag = pOutcome.result().ETag();

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);

    if (gOutcome.isSuccess()) {
        EXPECT_STREQ(actualETag.c_str(), gOutcome.result().Metadata().ETag().c_str());
    }
    else {
        EXPECT_TRUE(false);
    }
}

TEST_F(HttpsTest, PutPreSignedUriTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedUriTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    std::string md5 = ComputeContentMD5(*content.get());

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setContentMd5(md5);
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    ObjectMetaData meta;
    meta.setContentMd5(md5);

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), content, meta);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    auto metaOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
}

TEST_F(HttpsTest, MultipartUploadTest)
{
    auto key = TestUtils::GetObjectKey("MultipartUploadTest");
    InitiateMultipartUploadRequest request(BucketName, key);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    auto content = TestUtils::GetRandomStream(100*1024);
    UploadPartRequest uRequest(BucketName, key, content);
    uRequest.setPartNumber(1);
    uRequest.setUploadId(initOutcome.result().UploadId());
    auto uploadPartOutcome = Client->UploadPart(uRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    content = TestUtils::GetRandomStream(100);
    uRequest.setPartNumber(2);
    uRequest.setConetent(content);
    uRequest.setUploadId(initOutcome.result().UploadId());
    uploadPartOutcome = Client->UploadPart(uRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    ListMultipartUploadsRequest lpRequest(BucketName);
    lpRequest.setKeyMarker("MultipartUploadTest");
    auto lmuOutcome = Client->ListMultipartUploads(lpRequest);
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_EQ(lmuOutcome.result().MultipartUploadList().size(), 1U);
    EXPECT_EQ(lmuOutcome.result().MultipartUploadList().begin()->Key, key);

    auto lOutcome = Client->ListParts(ListPartsRequest(BucketName, key, initOutcome.result().UploadId()));
    EXPECT_EQ(lOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest cRequest(BucketName, key,
        lOutcome.result().PartList(), initOutcome.result().UploadId());

    auto outcome = Client->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto hOutcome = Client->GetObjectMeta(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().ContentLength(), ((100LL * 1024LL) + 100LL));
}

TEST_F(HttpsTest, CacertPositiveTest)
{
    ClientConfiguration conf;
    std::string caFile = Config::GetDataPath();
    caFile.append("ca-certificates.crt");
    conf.caFile = caFile;
    conf.caPath = Config::GetDataPath();
    conf.verifySSL = true;
    std::string endpoint = TestUtils::GetHTTPSEndpoint(Config::Endpoint);
    OssClient client(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    SetLogLevel(LogLevel::LogAll);
    SetLogCallback(TestUtils::LogPrintCallback);

    std::string key = TestUtils::GetObjectKey("CacertPositiveTest");
    auto content = TestUtils::GetRandomStream(1024);
    auto outcome = client.PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    auto gOutcome = client.GetObject(BucketName, key);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content.get());
    std::string memMd5 = ComputeContentMD5(*gOutcome.result().Content().get());

    EXPECT_EQ(oriMd5, memMd5);
    SetLogLevel(LogLevel::LogOff);
    SetLogCallback(nullptr);
}

#if _WIN32
TEST_F(HttpsTest, CacertNegativeTest)
{
    ClientConfiguration conf;
    conf.verifySSL = true;
    std::string endpoint = TestUtils::GetHTTPSEndpoint(Config::Endpoint);
    OssClient client(endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    SetLogLevel(LogLevel::LogAll);
    SetLogCallback(TestUtils::LogPrintCallback);

    std::string key = TestUtils::GetObjectKey("CacertNegativeTest");
    auto content = TestUtils::GetRandomStream(1024);
    auto outcome = client.PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:200060");
    EXPECT_EQ(outcome.error().Message(), "Peer certificate cannot be authenticated with given CA certificates");

    SetLogLevel(LogLevel::LogOff);
    SetLogCallback(nullptr);
}
#endif



}
}