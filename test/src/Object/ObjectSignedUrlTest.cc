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
#include "src/utils/Utils.h"
#include "src/utils/FileSystemUtils.h"

namespace AlibabaCloud {
namespace OSS {

class ObjectSignedUrlTest : public ::testing::Test {
protected:
    ObjectSignedUrlTest()
    {
    }

    ~ObjectSignedUrlTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectsignedurl");
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
    static int64_t GetExpiresDelayS(int64_t value)
    {
        auto tp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
        return tp.time_since_epoch().count() + value;
    }
public:
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;
};

std::shared_ptr<OssClient> ObjectSignedUrlTest::Client = nullptr;
std::string ObjectSignedUrlTest::BucketName = "";


TEST_F(ObjectSignedUrlTest, GetObjectWithPreSignedAndAclParameter)
{
    std::string key = TestUtils::GetObjectKey("GetObjectWithPreSignedAndAclParameter");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    request.setExpires(GetExpiresDelayS(120));
    request.addParameter("acl", "");
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);

    if (gOutcome.isSuccess()) {
        std::istreambuf_iterator<char> isb(*gOutcome.result().Content().get()), end;
        std::string data(isb, end);
        EXPECT_TRUE(strstr(data.c_str(), "default") != nullptr);
    }
    else {
        EXPECT_TRUE(false);
    }
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriDefaultExpireDatePositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriDefaultExpireDatePositiveTest");
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

TEST_F(ObjectSignedUrlTest, GetPreSignedUriDefaultNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriDefaultNegativeTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    request.setExpires(GetExpiresDelayS(-10));
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "AccessDenied");
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriDefaultPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriDefaultPositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), true);
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriWithExpiresTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriWithExpiresTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    std::time_t t = std::time(nullptr) + 60;
    auto urlOutcome = Client->GeneratePresignedUrl(BucketName, key, t);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), true);
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriFullSettingsPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriFullSettingsPositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);
    std::string md5 = ComputeContentMD5(*content.get());

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    request.setContentMd5(md5);
    request.setContentType("application/zip");
    request.setExpires(GetExpiresDelayS(120));
    request.addResponseHeaders(RequestResponseHeader::CacheControl, "No-cache");
    request.addResponseHeaders(RequestResponseHeader::ContentType, "application/zip");
    request.addResponseHeaders(RequestResponseHeader::ContentDisposition, "myDownload.zip");
    request.UserMetaData()["name1"] = "value1";
    request.UserMetaData()["name2"] = "value2";
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    ObjectMetaData meta;
    meta.setContentMd5(md5);
    meta.setContentType("application/zip");
    meta.UserMetaData()["name1"] = "value1";
    meta.UserMetaData()["name2"] = "value2";

    auto gOutcome = Client->GetObjectByUrl(GetObjectByUrlRequest(urlOutcome.result(), meta));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_STREQ(gOutcome.result().Metadata().ContentDisposition().c_str(), "myDownload.zip");
    EXPECT_STREQ(gOutcome.result().Metadata().CacheControl().c_str(), "No-cache");
    EXPECT_STREQ(gOutcome.result().Metadata().ContentType().c_str(), "application/zip");
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriWithContentTypeAndMd5NegativeTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriWithContentTypeAndMd5NegativeTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);
    std::string md5 = ComputeContentMD5(*content.get());

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    ObjectMetaData meta;
    meta.setContentMd5(md5);

    auto gOutcome = Client->GetObjectByUrl(GetObjectByUrlRequest(urlOutcome.result(), meta));
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "SignatureDoesNotMatch");
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriWithContentTypeAndMd5PositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriWithContentTypeAndMd5PositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);
    std::string md5 = ComputeContentMD5(*content.get());

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    request.setContentMd5(md5);
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    ObjectMetaData meta;
    meta.setContentMd5(md5);

    auto gOutcome = Client->GetObjectByUrl(GetObjectByUrlRequest(urlOutcome.result(), meta));
    EXPECT_EQ(gOutcome.isSuccess(), true);
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriWithResponseHeaderPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriWithResponseHeaderPositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    request.addResponseHeaders(RequestResponseHeader::CacheControl, "No-cache");
    request.addResponseHeaders(RequestResponseHeader::ContentType, "application/zip");
    request.addResponseHeaders(RequestResponseHeader::ContentDisposition, "myDownload.zip");
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(GetObjectByUrlRequest(urlOutcome.result()));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_STREQ(gOutcome.result().Metadata().ContentDisposition().c_str(), "myDownload.zip");
    EXPECT_STREQ(gOutcome.result().Metadata().CacheControl().c_str(), "No-cache");
    EXPECT_STREQ(gOutcome.result().Metadata().ContentType().c_str(), "application/zip");
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriWithUserMetaPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriWithUserMetaPositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Get);
    request.setExpires(GetExpiresDelayS(120));
    request.UserMetaData()["name1"] = "value1";
    request.UserMetaData()["name2"] = "value2";
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    ObjectMetaData meta;
    meta.UserMetaData()["name1"] = "value1";
    meta.UserMetaData()["name2"] = "value2";

    auto gOutcome = Client->GetObjectByUrl(GetObjectByUrlRequest(urlOutcome.result(), meta));
    EXPECT_EQ(gOutcome.isSuccess(), true);
}


TEST_F(ObjectSignedUrlTest, PutPreSignedUriDefaultPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedUriDefaultPositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    std::string md5 = ComputeContentMD5(*content.get());

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(120));
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

TEST_F(ObjectSignedUrlTest, PutObjectWithPreSignedUriAndCrc)
{
    ClientConfiguration conf;
    conf.enableCrc64 = true;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    std::string key = TestUtils::GetObjectKey("PutObjectWithPreSignedUriAndCrc");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(120));
    auto urlOutcome = client.GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = client.PutObjectByUrl(urlOutcome.result(), content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), true);

}

TEST_F(ObjectSignedUrlTest, PutObjectWithPreSignedUriWithoutCrc)
{
    ClientConfiguration conf;
    conf.enableCrc64 = false;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    std::string key = TestUtils::GetObjectKey("PutObjectWithPreSignedUriWithoutCrc");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(120));
    auto urlOutcome = client.GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = client.PutObjectByUrl(urlOutcome.result(), content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), true);

}

TEST_F(ObjectSignedUrlTest, PutPreSignedWithExpiresTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedWithExpiresTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    auto urlOutcome = Client->GeneratePresignedUrl(BucketName, key, GetExpiresDelayS(60), Http::Put);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(PutObjectByUrlRequest(urlOutcome.result(), content));
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().ContentLength(), 1024LL);
}

TEST_F(ObjectSignedUrlTest, PutPreSignedByFileNameTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedByFileNameTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutPreSignedByFileNameTest");
    TestUtils::WriteRandomDatatoFile(tmpFile, 2048);
    auto fileETag = TestUtils::GetFileETag(tmpFile);

    auto urlOutcome = Client->GeneratePresignedUrl(BucketName, key, GetExpiresDelayS(60), Http::Put);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), tmpFile);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().ETag(), fileETag);
    RemoveFile(tmpFile);
}

TEST_F(ObjectSignedUrlTest, PutPreSignedByFileNameWithMetaDataTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedByFileNameWithMetaDataTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutPreSignedByFileNameWithMetaDataTest");
    TestUtils::WriteRandomDatatoFile(tmpFile, 2048);
    auto fileETag = TestUtils::GetFileETag(tmpFile);

    ObjectMetaData metaData;
    metaData.UserMetaData()["test0"] = "value0";
    metaData.UserMetaData()["test1"] = "value1";

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.UserMetaData()["test0"] = "value0";
    request.UserMetaData()["test1"] = "value1";

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), tmpFile, metaData);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().ETag(), fileETag);

    RemoveFile(tmpFile);
}

TEST_F(ObjectSignedUrlTest, PutPreSignedUriDefaultNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedUriDefaultNegativeTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(-5));
    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), content);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_STREQ(pOutcome.error().Code().c_str(), "AccessDenied");
}

TEST_F(ObjectSignedUrlTest, PutObjectWithPreSignedUriWithParameter)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithPreSignedUriWithParameter");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    std::string md5 = ComputeContentMD5(*content.get());

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(120));
    request.setContentMd5(md5);
    request.setContentType("text/rtf");
    request.UserMetaData()["Author"] = "oss";
    request.UserMetaData()["Test"] = "test";
    request.addParameter("x-param-null", "");
    request.addParameter("x-param-space0", " ");
    request.addParameter("x-param-value", "value");
    request.addParameter("x-param-space1", " ");

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    ObjectMetaData meta;
    meta.setContentMd5(md5);
    meta.setContentType("text/rtf");
    meta.UserMetaData()["Author"] = "oss";
    meta.UserMetaData()["Test"] = "test";

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), content, meta);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), true);

    auto metaOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_STREQ(metaOutcome.result().ContentType().c_str(), "text/rtf");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("Author").c_str(), "oss");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("author").c_str(), "oss");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("Test").c_str(), "test");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("tesT").c_str(), "test");
}

TEST_F(ObjectSignedUrlTest, PutObjectWithPreSignedUriWithParameterNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithPreSignedUriWithParameterNegativeTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    std::string md5 = ComputeContentMD5(*content.get());

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(120));
    request.setContentMd5(md5);
    request.setContentType("text/rtf");
    request.UserMetaData()["Author"] = "oss";
    request.UserMetaData()["Test"] = "test";
    request.addParameter("x-param-null", "");
    request.addParameter("x-param-space0", " ");
    request.addParameter("x-param-value", "value");
    request.addParameter("x-param-space1", " ");

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    ObjectMetaData meta;
    meta.setContentMd5(md5);
    meta.setContentType("text/rtf");
    meta.UserMetaData()["Author"] = "oss";

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), content, meta);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_STREQ(pOutcome.error().Code().c_str(), "SignatureDoesNotMatch");
}

TEST_F(ObjectSignedUrlTest, GetPreSignedUriSetBucketObjectPositiveTest)
{
    std::string key = TestUtils::GetObjectKey("GetPreSignedUriSetBucketObjectPositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    std::string actualETag = pOutcome.result().ETag();

    GeneratePresignedUrlRequest request("", "");
    request.setBucket(BucketName);
    request.setKey(key);
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

TEST_F(ObjectSignedUrlTest, GetObjectToFilePositiveTest)
{
    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectToFilePositiveTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    std::string actualETag = pOutcome.result().ETag();

    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectToFilePositiveTest");

    auto urlOutcome = Client->GeneratePresignedUrl(BucketName, key);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result(), tmpFile);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    auto fileContent = gOutcome.result().Content();
    fileContent->seekg(0, fileContent->beg);
    
    auto fileETag0 = ComputeContentETag(*fileContent);

    gOutcome = dummy;
    fileContent = nullptr;
    auto fileETag = TestUtils::GetFileETag(tmpFile);

    EXPECT_EQ(actualETag, fileETag0);
    EXPECT_EQ(actualETag, fileETag);
    RemoveFile(tmpFile);
}

TEST_F(ObjectSignedUrlTest, GeneratePresignedUrlInvalidBucketNameTest)
{
    auto outcome = Client->GeneratePresignedUrl("InvalidBucketName", "key");
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(ObjectSignedUrlTest, GeneratePresignedUrlInvalidKeyTest)
{
    auto outcome = Client->GeneratePresignedUrl("bucket", "");
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(ObjectSignedUrlTest, GetObjectByUrlRequestFunctionTest)
{
    GetObjectByUrlRequest request("http://demo.test/test");
    auto paramters = request.Parameters();
    EXPECT_TRUE(paramters.empty());
}

TEST_F(ObjectSignedUrlTest, PutObjectByUrlRequestFunctionTest)
{
    auto content = TestUtils::GetRandomStream(0);
    PutObjectByUrlRequest request("http://demo.test/test", content);
    auto paramters = request.Parameters();
    EXPECT_TRUE(paramters.empty());
}

TEST_F(ObjectSignedUrlTest, UnencodedSlashTest)
{
    std::string key = TestUtils::GetObjectKey("UnencodedSlashTest/123/456%2F/123");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(2048);

    std::string md5 = ComputeContentMD5(*content.get());

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(120));
    request.setContentMd5(md5);
    request.setContentType("text/rtf");
    request.UserMetaData()["Author"] = "oss";
    request.UserMetaData()["Test"] = "test";
    request.addParameter("x-param-null", "");
    request.addParameter("x-param-space0", " ");
    request.addParameter("x-param-value", "value");
    request.addParameter("x-param-space1", " ");

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    EXPECT_TRUE(urlOutcome.result().find("UnencodedSlashTest%2F123%2F456%252F%2F123") != std::string::npos);

    ObjectMetaData meta;
    meta.setContentMd5(md5);
    meta.setContentType("text/rtf");
    meta.UserMetaData()["Author"] = "oss";
    meta.UserMetaData()["Test"] = "test";

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), content, meta);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), true);

    auto metaOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_STREQ(metaOutcome.result().ContentType().c_str(), "text/rtf");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("Author").c_str(), "oss");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("author").c_str(), "oss");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("Test").c_str(), "test");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("tesT").c_str(), "test");

    //
    request = GeneratePresignedUrlRequest(BucketName, key, Http::Put);
    request.setExpires(GetExpiresDelayS(120));
    request.setContentMd5(md5);
    request.setContentType("text/rtf");
    request.UserMetaData()["Author"] = "oss1";
    request.UserMetaData()["Test"] = "test1";
    request.addParameter("x-param-null", "");
    request.addParameter("x-param-space0", " ");
    request.addParameter("x-param-value", "value");
    request.addParameter("x-param-space1", " ");
    request.setUnencodedSlash(true);

    urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    EXPECT_TRUE(urlOutcome.result().find("UnencodedSlashTest/123/456%252F/123") != std::string::npos);

    meta = ObjectMetaData();
    meta.setContentMd5(md5);
    meta.setContentType("text/rtf");
    meta.UserMetaData()["Author"] = "oss1";
    meta.UserMetaData()["Test"] = "test1";

    pOutcome = Client->PutObjectByUrl(urlOutcome.result(), content, meta);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::ObjectExists(*Client, BucketName, key), true);

    metaOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_STREQ(metaOutcome.result().ContentType().c_str(), "text/rtf");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("Author").c_str(), "oss1");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("author").c_str(), "oss1");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("Test").c_str(), "test1");
    EXPECT_STREQ(metaOutcome.result().UserMetaData().at("tesT").c_str(), "test1");
}



}
}