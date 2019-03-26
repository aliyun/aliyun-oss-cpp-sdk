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
#include <src/utils/Utils.h>
#include <src/utils/FileSystemUtils.h>
#include "src/external/json/json.h"

namespace AlibabaCloud
{ 
namespace OSS 
{
class ObjectCallbackTest : public ::testing::Test
{
protected:
    ObjectCallbackTest()
    {
    }

    ~ObjectCallbackTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
		BucketName = TestUtils::GetBucketName("cpp-sdk-objectcallback");
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

std::shared_ptr<OssClient> ObjectCallbackTest::Client = nullptr;
std::string ObjectCallbackTest::BucketName = "";

TEST_F(ObjectCallbackTest, PutObjectCallbackTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectCallbackTest");
    auto content = TestUtils::GetRandomStream(1024);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();

    PutObjectRequest request(BucketName, key, content);
    request.setCallback(value);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() != nullptr);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(ObjectCallbackTest, PutObjectCallbackVarTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectCallbackVarTest");
    auto content = TestUtils::GetRandomStream(1024);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody, "", ObjectCallbackBuilder::URL);
    std::string value = builder.build();

    ObjectCallbackVariableBuilder varBuilder;
    varBuilder.addCallbackVariable("x:var1", "value1");
    varBuilder.addCallbackVariable("x:var2", "value2");
    std::string varValue = varBuilder.build();

    PutObjectRequest request(BucketName, key, content);
    request.setCallback(value, varValue);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() != nullptr);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

TEST_F(ObjectCallbackTest, MultipartUploadCallbackTest)
{
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadCallbackTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());

    PartList partETags;
    auto content = TestUtils::GetRandomStream(1024);
    UploadPartRequest uRequest(BucketName, targetObjectKey, content);
    uRequest.setPartNumber(1);
    uRequest.setUploadId(initOutcome.result().UploadId());
    auto uploadPartOutcome = Client->UploadPart(uRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
    Part part(1, uploadPartOutcome.result().ETag());
    partETags.push_back(part);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, partETags);
    completeRequest.setUploadId(initOutcome.result().UploadId());
    completeRequest.setCallback(value);
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_TRUE(cOutcome.result().Content() != nullptr);

}

TEST_F(ObjectCallbackTest, MultipartUploadCallbackVarTest)
{
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadCallbackTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());

    PartList partETags;
    auto content = TestUtils::GetRandomStream(1024);
    UploadPartRequest uRequest(BucketName, targetObjectKey, content);
    uRequest.setPartNumber(1);
    uRequest.setUploadId(initOutcome.result().UploadId());
    auto uploadPartOutcome = Client->UploadPart(uRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
    Part part(1, uploadPartOutcome.result().ETag());
    partETags.push_back(part);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody, "", ObjectCallbackBuilder::URL);
    std::string value = builder.build();

    ObjectCallbackVariableBuilder varBuilder;
    varBuilder.addCallbackVariable("x:var1", "value1");
    varBuilder.addCallbackVariable("x:var2", "value2");
    std::string varValue = varBuilder.build();

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, partETags);
    completeRequest.setUploadId(initOutcome.result().UploadId());
    completeRequest.setCallback(value, varValue);
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_TRUE(cOutcome.result().Content() != nullptr);

}

TEST_F(ObjectCallbackTest, ResumableUploadCallbackTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadCallbackTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadCallbackTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();
    //simple mode 
    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(309600);
    request.setCallback(value);
    auto pOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() != nullptr);
    std::string fileETag = TestUtils::GetFileETag(file);
    EXPECT_EQ(fileETag, pOutcome.result().ETag());

    //multiprat mode
    request.setPartSize(102400);
    auto pOutcome2 = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome2.isSuccess(), true);
    EXPECT_TRUE(pOutcome2.result().Content() != nullptr);

    EXPECT_EQ(pOutcome2.result().CRC64(), pOutcome.result().CRC64());
    EXPECT_NE(pOutcome2.result().ETag(), pOutcome.result().ETag());

    EXPECT_EQ(RemoveFile(file), true);
}

TEST_F(ObjectCallbackTest, ResumableUploadCallbackVarTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadCallbackVarTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadCallbackVarTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();
    ObjectCallbackVariableBuilder varBuilder;
    varBuilder.addCallbackVariable("x:var1", "value1");
    varBuilder.addCallbackVariable("x:var2", "value2");
    std::string varValue = varBuilder.build();

    //simple mode 
    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(309600);
    request.setCallback(value, varValue);
    auto pOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() != nullptr);
    std::string fileETag = TestUtils::GetFileETag(file);
    EXPECT_EQ(fileETag, pOutcome.result().ETag());

    //multiprat mode
    request.setPartSize(102400);
    auto pOutcome2 = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome2.isSuccess(), true);
    EXPECT_TRUE(pOutcome2.result().Content() != nullptr);

    EXPECT_EQ(pOutcome2.result().CRC64(), pOutcome.result().CRC64());
    EXPECT_NE(pOutcome2.result().ETag(), pOutcome.result().ETag());

    EXPECT_EQ(RemoveFile(file), true);
}

TEST_F(ObjectCallbackTest, PutPreSignedCallbackTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedCallbackTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";
    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.addParameter("callback", value);

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(PutObjectByUrlRequest(urlOutcome.result(), content));
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() != nullptr);

    auto outcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().ContentLength(), 1024LL);
}

TEST_F(ObjectCallbackTest, PutPreSignedCallbackVarTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedCallbackVarTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";
    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();
    ObjectCallbackVariableBuilder varBuilder;
    varBuilder.addCallbackVariable("x:var1", "value1");
    varBuilder.addCallbackVariable("x:var2", "value2");
    std::string varValue = varBuilder.build();

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.addParameter("callback", value);
    request.addParameter("callback-var", varValue);

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(PutObjectByUrlRequest(urlOutcome.result(), content));
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() != nullptr);

    auto outcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().ContentLength(), 1024LL);
}

TEST_F(ObjectCallbackTest, ResumableUploadCallbackUrlNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadCallbackUrlNegativeTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadCallbackUrlNegativeTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);

    std::string callbackUrl = "http://192.168.1.1";
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();
    //simple mode 
    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(309600);
    request.setCallback(value);
    auto pOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "InvalidArgument");
    EXPECT_EQ(pOutcome.error().Message(), "Private address is forbidden to callback.");
    EXPECT_TRUE(pOutcome.error().RequestId().size() > 0);

    //multiprat mode
    request.setPartSize(102400);
    auto pOutcome2 = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome2.isSuccess(), false);
    EXPECT_EQ(pOutcome2.error().Code(), "InvalidArgument");
    EXPECT_EQ(pOutcome.error().Message(), "Private address is forbidden to callback.");
    EXPECT_TRUE(pOutcome.error().RequestId().size() > 0);

    //simple mode 
    builder.setCallbackUrl("http://oss-cn-hangzhou.aliyuncs.com");
    value = builder.build();
    request.setPartSize(309600);
    request.setCallback(value);
    pOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "CallbackFailed");
    EXPECT_TRUE(pOutcome.error().RequestId().size() > 0);

    request.setPartSize(102400);
    pOutcome2 = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome2.isSuccess(), false);
    EXPECT_EQ(pOutcome2.error().Code(), "CallbackFailed");
    EXPECT_TRUE(pOutcome.error().RequestId().size() > 0);

    EXPECT_EQ(RemoveFile(file), true);
}

TEST_F(ObjectCallbackTest, PutObjectCallbackNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectCallbackBodyNegativeTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    request.setCallback("error argument");
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "InvalidArgument");
    EXPECT_EQ(pOutcome.error().Message(), "The callback configuration is not base64 encoded.");

    request.setCallback(Base64Encode("error argument"));
    pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "InvalidArgument");
    EXPECT_EQ(pOutcome.error().Message(), "The callback configuration is not json format.");
}

TEST_F(ObjectCallbackTest, PutPreSignedCallbackNegativeTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedCallbackNegativeTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);
    request.addParameter("callback", Base64Encode("error argument"));

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(PutObjectByUrlRequest(urlOutcome.result(), content));
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "InvalidArgument");
    EXPECT_EQ(pOutcome.error().Message(), "The callback configuration is not json format.");

    //invalid callback server
    std::string callbackUrl = "http://oss-cn-hangzhou.aliyuncs.com";
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody);
    std::string value = builder.build();
    request.addParameter("callback", value);

    urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    pOutcome = Client->PutObjectByUrl(PutObjectByUrlRequest(urlOutcome.result(), content));
    EXPECT_EQ(pOutcome.isSuccess(), false);
    EXPECT_EQ(pOutcome.error().Code(), "CallbackFailed");
    EXPECT_TRUE(pOutcome.error().RequestId().size() > 0);
}

TEST_F(ObjectCallbackTest, ObjectCallbackBuilderTest)
{
    ObjectCallbackBuilder builder("192.168.1.100", "11111");

    std::stringstream ss;
    std::string value;
    Json::CharReaderBuilder rbuilder;
    Json::Value readRoot;
    std::string errMessage;

    builder.setCallbackUrl("192.168.1.1");
    builder.setCallbackBody("123456");
    value = TestUtils::Base64Decode(builder.build());
    ss << value;
    if (Json::parseFromStream(rbuilder, ss, &readRoot, &errMessage)) {
        EXPECT_EQ(readRoot["callbackUrl"].asString(), "192.168.1.1");
        EXPECT_EQ(readRoot["callbackHost"].asString(), "");
        EXPECT_EQ(readRoot["callbackBody"].asString(), "123456");
        EXPECT_EQ(readRoot["callbackBodyType"].asString(), "");
    }
    else {
        EXPECT_EQ(errMessage, "");
    }

    builder.setCallbackHost("demo.com");
    ss.str("");
    value = TestUtils::Base64Decode(builder.build());
    ss << value;
    if (Json::parseFromStream(rbuilder, ss, &readRoot, &errMessage)) {
        EXPECT_EQ(readRoot["callbackUrl"].asString(), "192.168.1.1");
        EXPECT_EQ(readRoot["callbackHost"].asString(), "demo.com");
        EXPECT_EQ(readRoot["callbackBody"].asString(), "123456");
        EXPECT_EQ(readRoot["callbackBodyType"].asString(), "");
    }
    else {
        EXPECT_EQ(errMessage, "");
    }

    builder.setCallbackBodyType(ObjectCallbackBuilder::JSON);
    ss.str("");
    value = TestUtils::Base64Decode(builder.build());
    ss << value;
    if (Json::parseFromStream(rbuilder, ss, &readRoot, &errMessage)) {
        EXPECT_EQ(readRoot["callbackUrl"].asString(), "192.168.1.1");
        EXPECT_EQ(readRoot["callbackHost"].asString(), "demo.com");
        EXPECT_EQ(readRoot["callbackBody"].asString(), "123456");
        EXPECT_EQ(readRoot["callbackBodyType"].asString(), "application/json");
    }
    else {
        EXPECT_EQ(errMessage, "");
    }
}

TEST_F(ObjectCallbackTest, ObjectCallbackVariableBuilderTest)
{
    ObjectCallbackVariableBuilder builder;
    builder.addCallbackVariable("x:var1", "value1");
    builder.addCallbackVariable("x:var2", "value2");

    std::stringstream ss;
    std::string value;
    Json::CharReaderBuilder rbuilder;
    Json::Value readRoot;
    std::string errMessage;

    value = TestUtils::Base64Decode(builder.build());
    ss << value;
    if (Json::parseFromStream(rbuilder, ss, &readRoot, &errMessage)) {
        EXPECT_EQ(readRoot["x:var1"].asString(), "value1");
        EXPECT_EQ(readRoot["x:var2"].asString(), "value2");
    }
    else {
        EXPECT_EQ(errMessage, "");
    }
}

TEST_F(ObjectCallbackTest, PutObjectWithoutCallbackTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithoutCallbackTest");
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

TEST_F(ObjectCallbackTest, MultipartUploadWitoutCallbackTest)
{
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadWitoutCallbackTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());

    PartList partETags;
    auto content = TestUtils::GetRandomStream(1024);
    UploadPartRequest uRequest(BucketName, targetObjectKey, content);
    uRequest.setPartNumber(1);
    uRequest.setUploadId(initOutcome.result().UploadId());
    auto uploadPartOutcome = Client->UploadPart(uRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
    Part part(1, uploadPartOutcome.result().ETag());
    partETags.push_back(part);

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, partETags);
    completeRequest.setUploadId(initOutcome.result().UploadId());
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_TRUE(cOutcome.result().Content() == nullptr);
}

TEST_F(ObjectCallbackTest, ResumableUploadWithoutCallbackTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadWithoutCallbackTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadWithoutCallbackTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);

    //simple mode 
    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(309600);
    auto pOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() == nullptr);
    std::string fileETag = TestUtils::GetFileETag(file);
    EXPECT_EQ(fileETag, pOutcome.result().ETag());

    //multiprat mode
    request.setPartSize(102400);
    auto pOutcome2 = Client->ResumableUploadObject(request);
    EXPECT_EQ(pOutcome2.isSuccess(), true);
    EXPECT_TRUE(pOutcome2.result().Content() == nullptr);

    EXPECT_EQ(pOutcome2.result().CRC64(), pOutcome.result().CRC64());
    EXPECT_NE(pOutcome2.result().ETag(), pOutcome.result().ETag());

    EXPECT_EQ(RemoveFile(file), true);
}

TEST_F(ObjectCallbackTest, PutPreSignedWithoutCallbackTest)
{
    std::string key = TestUtils::GetObjectKey("PutPreSignedWithoutCallbackTest");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    GeneratePresignedUrlRequest request(BucketName, key, Http::Put);

    auto urlOutcome = Client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto pOutcome = Client->PutObjectByUrl(PutObjectByUrlRequest(urlOutcome.result(), content));
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_TRUE(pOutcome.result().Content() == nullptr);

    auto outcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().ContentLength(), 1024LL);
}

}
}
