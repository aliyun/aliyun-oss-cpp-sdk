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
#include "src/utils/FileSystemUtils.h"
#include "src/utils/Utils.h"
#include <fstream>
namespace AlibabaCloud {
namespace OSS {

class MultipartUploadTest : public ::testing::Test {
protected:
    MultipartUploadTest()
    {
    }

    ~MultipartUploadTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-multipartupload");
        Client->CreateBucket(CreateBucketRequest(BucketName));
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

    // Sets up the test fixture.
    void SetUp() override
    {
    }

    // Tears down the test fixture.
    void TearDown() override
    {
    }

    static int CalculatePartCount(int64_t totalSize, int singleSize);
    static int64_t GetFileLength(const std::string file);

public:
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;
    static std::string TestFile;
    static std::string TestFileMd5;

};

std::shared_ptr<OssClient> MultipartUploadTest::Client = nullptr;
std::string MultipartUploadTest::BucketName = "";
std::string MultipartUploadTest::TestFile = "";
std::string MultipartUploadTest::TestFileMd5 = "";


int64_t MultipartUploadTest::GetFileLength(const std::string file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

int MultipartUploadTest::CalculatePartCount(int64_t totalSize, int singleSize)
{
    // Calculate the part count
    auto partCount = (int)(totalSize / singleSize);
    if (totalSize % singleSize != 0)
    {
        partCount++;
    }
    return partCount;
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadBasicTest)
{
    auto key = TestUtils::GetObjectKey("InitiateMultipartUploadBasicTest");
    InitiateMultipartUploadRequest request(BucketName, key);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadWithEncodingTypeTest)
{
    auto key = TestUtils::GetObjectKey("InitiateMultipartUploadWithEncodingTypeTest");
    key.push_back(0x1c); key.push_back(0x1a); key.append(".1.t");
    InitiateMultipartUploadRequest request(BucketName, key);
    request.setEncodingType("url");
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadWithMetaDataTest)
{
    auto key = TestUtils::GetObjectKey("InitiateMultipartUploadWithMetaTest");
    ObjectMetaData metaData;
    metaData.UserMetaData()["test"] = "test";
    metaData.UserMetaData()["data"] = "data";
    InitiateMultipartUploadRequest request(BucketName, key, metaData);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadWithFullSettingTest)
{
    auto key = TestUtils::GetObjectKey("InitiateMultipartUploadWithFullSettingTest");
    ObjectMetaData metaData;
    metaData.setCacheControl("No-Cache");
    metaData.setContentType("user/test");
    metaData.setContentEncoding("myzip");
    metaData.setContentDisposition("test.zip");
    metaData.UserMetaData()["test"] = "test";
    metaData.UserMetaData()["data"] = "data";
    InitiateMultipartUploadRequest request(BucketName, key, metaData);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);

    auto content = TestUtils::GetRandomStream(100);
    auto uploadPartOutcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, initOutcome.result().UploadId(), content));
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    auto lOutcome = Client->ListParts(ListPartsRequest(BucketName, key, initOutcome.result().UploadId()));
    EXPECT_EQ(lOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest cRequest(BucketName, key,
        lOutcome.result().PartList(), initOutcome.result().UploadId());

    auto outcome = Client->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().ContentLength(), 100);
    EXPECT_EQ(hOutcome.result().CacheControl(), "No-Cache");
    EXPECT_EQ(hOutcome.result().ContentType(), "user/test");
    EXPECT_EQ(hOutcome.result().ContentDisposition(), "test.zip");
    EXPECT_EQ(hOutcome.result().ContentEncoding(), "myzip");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "test");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("data"), "data");
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadWithFullSetting2Test)
{
    auto key = TestUtils::GetObjectKey("InitiateMultipartUploadWithFullSetting2Test");
    InitiateMultipartUploadRequest request(BucketName, key);
    request.setCacheControl("No-Cache");
    request.setContentEncoding("myzip");
    request.setContentDisposition("test.zip");
    request.MetaData().setContentType("user/test");
    request.MetaData().UserMetaData()["test"] = "test";
    request.MetaData().UserMetaData()["data"] = "data";
    request.setExpires(TestUtils::GetGMTString(3600));

    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);

    auto content = TestUtils::GetRandomStream(100);
    auto uploadPartOutcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, initOutcome.result().UploadId(), content));
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    auto lOutcome = Client->ListParts(ListPartsRequest(BucketName, key, initOutcome.result().UploadId()));
    EXPECT_EQ(lOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest cRequest(BucketName, key,
        lOutcome.result().PartList(), initOutcome.result().UploadId());

    auto outcome = Client->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().ContentLength(), 100);
    EXPECT_EQ(hOutcome.result().CacheControl(), "No-Cache");
    EXPECT_EQ(hOutcome.result().ContentType(), "user/test");
    EXPECT_EQ(hOutcome.result().ContentDisposition(), "test.zip");
    EXPECT_EQ(hOutcome.result().ContentEncoding(), "myzip");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "test");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("data"), "data");
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <InitiateMultipartUploadResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <Bucket>multipart_upload</Bucket>
                            <Key>multipart.data</Key>
                            <UploadId>0004B9894A22E5B1888A1E29F8236E2D</UploadId>
                        </InitiateMultipartUploadResult>)";
    InitiateMultipartUploadResult result(xml);
    EXPECT_EQ(result.Bucket(), "multipart_upload");
    EXPECT_EQ(result.Key(), "multipart.data");
    EXPECT_EQ(result.UploadId(), "0004B9894A22E5B1888A1E29F8236E2D");
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadResultEncodingType)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <InitiateMultipartUploadResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <EncodingType>url</EncodingType>
                            <Bucket>multipart_upload</Bucket>
                            <Key>multipart%20%2Fdata</Key>
                            <UploadId>0004B9894A22E5B1888A1E29F8236E2D</UploadId>
                        </InitiateMultipartUploadResult>)";
    InitiateMultipartUploadResult result(xml);
    EXPECT_EQ(result.Bucket(), "multipart_upload");
    EXPECT_EQ(result.Key(), "multipart /data");
    EXPECT_EQ(result.UploadId(), "0004B9894A22E5B1888A1E29F8236E2D");
}

TEST_F(MultipartUploadTest, InitiateMultipartUploadResultEmptyNodeTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <InitiateMultipartUploadResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <EncodingType></EncodingType>
                            <Bucket></Bucket>
                            <Key></Key>
                            <UploadId></UploadId>
                        </InitiateMultipartUploadResult>)";
    InitiateMultipartUploadResult result(xml);
    EXPECT_EQ(result.Bucket(), "");
    EXPECT_EQ(result.Key(), "");
    EXPECT_EQ(result.UploadId(), "");
}

TEST_F(MultipartUploadTest, UploadPartBasicTest)
{
    auto key = TestUtils::GetObjectKey("UploadPartBasicTest");
    InitiateMultipartUploadRequest request(BucketName, key);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);

    auto content = TestUtils::GetRandomStream(100);
    auto uploadPartOutcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, initOutcome.result().UploadId(), content));
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    auto calcETag = ComputeContentETag(*content);
    EXPECT_EQ(uploadPartOutcome.result().ETag(), calcETag);
    EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
}

TEST_F(MultipartUploadTest, UploadPartNegativeTest)
{
    auto key = TestUtils::GetObjectKey("UploadPartNegativeTest");
    auto content = TestUtils::GetRandomStream(100);
    auto outcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, "invaliduploadid", content));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchUpload");
}

TEST_F(MultipartUploadTest, UploadPartInvalidContentTest)
{
    auto key = TestUtils::GetObjectKey("UploadPartInvalidContentTest");

    std::shared_ptr<std::iostream> content = nullptr;
    auto outcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, "id", content));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "Request body is null.");

    content = TestUtils::GetRandomStream(100);
    content->setstate(content->badbit);
    outcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, "id", content));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "Request body is in bad state. Read/writing error on i/o operation.");

    content = TestUtils::GetRandomStream(100);
    content->setstate(content->failbit);
    outcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, "id", content));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "Request body is in fail state. Logical error on i/o operation.");
}

TEST_F(MultipartUploadTest, UploadPartInvalidContentLengthTest)
{
    auto key = TestUtils::GetObjectKey("UploadPartInvalidContentLengthTest");

    auto content = TestUtils::GetRandomStream(100);
    UploadPartRequest request(BucketName, key, 1, "id", content);
    request.setContentLength(5LL * 1024LL * 1024LL * 1024LL + 1LL);
    auto outcome = Client->UploadPart(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "PartSize should not be less than 100*1024 or greater than 5*1024*1024*1024.");
}

TEST_F(MultipartUploadTest, UploadPartInvalidPartNumberTest)
{
    auto key = TestUtils::GetObjectKey("UploadPartInvalidPartNumberTest");

    auto content = TestUtils::GetRandomStream(100);
    UploadPartRequest request(BucketName, key, 10001, "id", content);
    auto outcome = Client->UploadPart(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "PartNumber should not be less than 1 or greater than 10000.");

    request.setPartNumber(0);
    outcome = Client->UploadPart(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "PartNumber should not be less than 1 or greater than 10000.");
}

TEST_F(MultipartUploadTest, UploadPartInvalidBucketObjectTest)
{
    auto content = TestUtils::GetRandomStream(100);
    UploadPartRequest request("InvalidBucket", "key", 10001, "id", content);
    auto outcome = Client->UploadPart(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");

    request.setBucket("bucket");
    request.setKey("");
    outcome = Client->UploadPart(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadNegativeTest)
{
    auto key = TestUtils::GetObjectKey("CompleteMultipartUploadNegativeTest");
    PartList partList;
    partList.push_back(Part(1, "invalidetag"));

    CompleteMultipartUploadRequest cRequest(BucketName, key, partList, "invaliduploadid");
    auto outcome = Client->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchUpload");
    EXPECT_FALSE(outcome.error().RequestId().empty());
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadWithEmptyPartListTest)
{
    auto key = TestUtils::GetObjectKey("CompleteMultipartUploadWithEmptyPartListTest");
    CompleteMultipartUploadRequest cRequest(BucketName, key);
    auto outcome = Client->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "PartList is empty.");
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadInvalidBucketNameTest)
{
    auto key = TestUtils::GetObjectKey("CompleteMultipartUploadInvalidBucketNameTest");
    CompleteMultipartUploadRequest cRequest("InavlidBucketName", key);
    auto outcome = Client->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "The bucket name is invalid") != nullptr);
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadWithEncodingTypeTest)
{
    auto key = TestUtils::GetObjectKey("CompleteMultipartUploadWithEncodingTypeTest  ");
    InitiateMultipartUploadRequest request(BucketName, key);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);

    auto content = TestUtils::GetRandomStream(100);
    auto uploadPartOutcome = Client->UploadPart(UploadPartRequest(BucketName, key, 1, initOutcome.result().UploadId(), content));
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    PartList partList;
    Part part(1, uploadPartOutcome.result().ETag());
    partList.push_back(part);
    
    CompleteMultipartUploadRequest completeRequest(BucketName, key);
    completeRequest.setPartList(partList);
    completeRequest.setUploadId(initOutcome.result().UploadId());
    completeRequest.setEncodingType("url");
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().Key(), key);
    EXPECT_TRUE(strstr(cOutcome.result().Location().c_str(), key.c_str()) != nullptr);
    EXPECT_EQ(cOutcome.result().EncodingType(), "url");
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CompleteMultipartUploadResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <Location>http://oss-example.oss-cn-hangzhou.aliyuncs.com/multipart.data</Location>
                            <Bucket>oss-example</Bucket>
                            <Key>multipart.data</Key>
                            <ETag>B864DB6A936D376F9F8D3ED3BBE540DD-3</ETag>
                        </CompleteMultipartUploadResult>)";
    CompleteMultipartUploadResult result(xml);
    EXPECT_EQ(result.Bucket(), "oss-example");
    EXPECT_EQ(result.Location(), "http://oss-example.oss-cn-hangzhou.aliyuncs.com/multipart.data");
    EXPECT_EQ(result.Key(), "multipart.data");
    EXPECT_EQ(result.ETag(), "B864DB6A936D376F9F8D3ED3BBE540DD-3");
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadResultEncodingTypeTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CompleteMultipartUploadResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <Location>oss-example.oss-cn-hangzhou.aliyuncs.com</Location>
                            <Bucket>oss-example</Bucket>
                            <Key>multipart%2F%20.data</Key>
                            <ETag>B864DB6A936D376F9F8D3ED3BBE540DD-3</ETag>
                            <EncodingType>url</EncodingType>
                        </CompleteMultipartUploadResult>)";
    CompleteMultipartUploadResult result(xml);
    EXPECT_EQ(result.Bucket(), "oss-example");
    EXPECT_EQ(result.Location(), "oss-example.oss-cn-hangzhou.aliyuncs.com");
    EXPECT_EQ(result.Key(), "multipart/ .data");
    EXPECT_EQ(result.ETag(), "B864DB6A936D376F9F8D3ED3BBE540DD-3");
    EXPECT_EQ(result.EncodingType(), "url");
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadResultEmptyNodeTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CompleteMultipartUploadResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <Location></Location>
                            <Bucket></Bucket>
                            <Key></Key>
                            <ETag></ETag>
                            <EncodingType></EncodingType>
                        </CompleteMultipartUploadResult>)";
    CompleteMultipartUploadResult result(xml);
    EXPECT_EQ(result.Bucket(), "");
    EXPECT_EQ(result.Location(), "");
    EXPECT_EQ(result.Key(), "");
    EXPECT_EQ(result.ETag(), "");
}

TEST_F(MultipartUploadTest, UploadPartCopyTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CopyPartResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <LastModified>2014-07-17T06:27:54.000Z</LastModified>
                            <ETag>"5B3C1A2E053D763E1B002CC607C5A0FE"</ETag>
                        </CopyPartResult>)";
    UploadPartCopyResult result(xml);
    EXPECT_EQ(result.LastModified(), "2014-07-17T06:27:54.000Z");
    EXPECT_EQ(result.ETag(), "5B3C1A2E053D763E1B002CC607C5A0FE");
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadResultEmptyTest)
{
    CompleteMultipartUploadResult result("");
    EXPECT_EQ(result.Bucket(), "");
    EXPECT_EQ(result.Location(), "");
    EXPECT_EQ(result.Key(), "");
    EXPECT_EQ(result.ETag(), "");
}

TEST_F(MultipartUploadTest, ListMultipartUploadsResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListMultipartUploadsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <Bucket>oss-example</Bucket>
                            <KeyMarker>keyMarker</KeyMarker>
                            <UploadIdMarker>1104B99B8E707874FC2D692FA5D77D3F</UploadIdMarker>
                            <NextKeyMarker>oss.avi</NextKeyMarker>
                            <NextUploadIdMarker>0004B99B8E707874FC2D692FA5D77D3F</NextUploadIdMarker>
                            <Delimiter>/</Delimiter>
                            <Prefix>prefix</Prefix>
                            <MaxUploads>1000</MaxUploads>
                            <IsTruncated>false</IsTruncated>
                            <Upload>
                                <Key>multipart.data</Key>
                                <UploadId>0004B999EF518A1FE585B0C9360DC4C8</UploadId>
                                <Initiated>2012-02-23T04:18:23.000Z</Initiated>
                            </Upload>
                            <Upload>
                                <Key>multipart.data</Key>
                                <UploadId>0004B999EF5A239BB9138C6227D69F95</UploadId>
                                <Initiated>2012-02-23T04:18:23.000Z</Initiated>
                            </Upload>
                            <Upload>
                                <Key>oss.avi</Key>
                                <UploadId>0004B99B8E707874FC2D692FA5D77D3F</UploadId>
                                <Initiated>2012-02-23T06:14:27.000Z</Initiated>
                            </Upload>
                        </ListMultipartUploadsResult>)";
    ListMultipartUploadsResult result(xml);
    EXPECT_EQ(result.Bucket(), "oss-example");
    EXPECT_EQ(result.KeyMarker(), "keyMarker");
    EXPECT_EQ(result.UploadIdMarker(), "1104B99B8E707874FC2D692FA5D77D3F");
    EXPECT_EQ(result.NextKeyMarker(), "oss.avi");
    EXPECT_EQ(result.NextUploadIdMarker(), "0004B99B8E707874FC2D692FA5D77D3F");
    EXPECT_EQ(result.MaxUploads(), 1000U);
    EXPECT_EQ(result.IsTruncated(), false);
    EXPECT_EQ(result.MultipartUploadList().size(), 3U);
    EXPECT_EQ(result.MultipartUploadList().begin()->Key, "multipart.data");
    EXPECT_EQ(result.MultipartUploadList().begin()->UploadId, "0004B999EF518A1FE585B0C9360DC4C8");
    EXPECT_EQ(result.MultipartUploadList().begin()->Initiated, "2012-02-23T04:18:23.000Z");
    EXPECT_EQ(result.MultipartUploadList().rbegin()->Key, "oss.avi");
    EXPECT_EQ(result.MultipartUploadList().rbegin()->UploadId, "0004B99B8E707874FC2D692FA5D77D3F");
    EXPECT_EQ(result.MultipartUploadList().rbegin()->Initiated, "2012-02-23T06:14:27.000Z");
}

TEST_F(MultipartUploadTest, ListMultipartUploadsResultEmptyNodeTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListMultipartUploadsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                            <Bucket></Bucket>
                            <KeyMarker></KeyMarker>
                            <UploadIdMarker></UploadIdMarker>
                            <NextKeyMarker></NextKeyMarker>
                            <NextUploadIdMarker></NextUploadIdMarker>
                            <Delimiter></Delimiter>
                            <Prefix></Prefix>
                            <MaxUploads></MaxUploads>
                            <IsTruncated></IsTruncated>
                            <Upload>
                                <Key></Key>
                                <UploadId></UploadId>
                                <Initiated></Initiated>
                            </Upload>
                            <Upload>
                                <Key></Key>
                                <UploadId></UploadId>
                                <Initiated></Initiated>
                            </Upload>
                        </ListMultipartUploadsResult>)";
    ListMultipartUploadsResult result(xml);
    EXPECT_EQ(result.Bucket(), "");
    EXPECT_EQ(result.KeyMarker(), "");
    EXPECT_EQ(result.UploadIdMarker(), "");
    EXPECT_EQ(result.NextKeyMarker(), "");
    EXPECT_EQ(result.NextUploadIdMarker(), "");
    EXPECT_EQ(result.MaxUploads(), 0UL);
    EXPECT_EQ(result.IsTruncated(), false);
    EXPECT_EQ(result.MultipartUploadList().size(), 2U);
    EXPECT_EQ(result.MultipartUploadList().begin()->Key, "");
    EXPECT_EQ(result.MultipartUploadList().begin()->UploadId, "");
    EXPECT_EQ(result.MultipartUploadList().begin()->Initiated, "");
    EXPECT_EQ(result.MultipartUploadList().rbegin()->Key, "");
    EXPECT_EQ(result.MultipartUploadList().rbegin()->UploadId, "");
    EXPECT_EQ(result.MultipartUploadList().rbegin()->Initiated, "");
}

TEST_F(MultipartUploadTest, ListPartsResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
            <ListPartsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                <Bucket>multipart_upload</Bucket>
                <Key>multipart.data</Key>
                <UploadId>0004B999EF5A239BB9138C6227D69F95</UploadId>
                <NextPartNumberMarker>5</NextPartNumberMarker>
                <MaxParts>1000</MaxParts>
                <IsTruncated>false</IsTruncated>
                <Part>
                    <PartNumber>1</PartNumber>
                    <LastModified>2012-02-23T07:01:34.000Z</LastModified>
                    <ETag>&quot;3349DC700140D7F86A078484278075A9&quot;</ETag>
                    <Size>6291456</Size>
                </Part>
                <Part>
                    <PartNumber>2</PartNumber>
                    <LastModified>2012-02-23T07:01:12.000Z</LastModified>
                    <ETag>&quot;3349DC700140D7F86A078484278075A9&quot;</ETag>
                    <Size>6291456</Size>
                </Part>
                <Part>
                    <PartNumber>5</PartNumber>
                    <LastModified>2012-02-23T07:02:03.000Z</LastModified>
                    <ETag>&quot;7265F4D211B56873A381D321F586E4A9&quot;</ETag>
                    <Size>1024</Size>
                </Part>
            </ListPartsResult>)";
    ListPartsResult result(xml);
    EXPECT_EQ(result.Bucket(), "multipart_upload");
    EXPECT_EQ(result.Key(), "multipart.data");
    EXPECT_EQ(result.UploadId(), "0004B999EF5A239BB9138C6227D69F95");
    EXPECT_EQ(result.EncodingType(), "");
    EXPECT_EQ(result.NextPartNumberMarker(), 5U);
    EXPECT_EQ(result.MaxParts(), 1000U);
    EXPECT_EQ(result.IsTruncated(), false);
    EXPECT_EQ(result.PartList().size(), 3U);
    EXPECT_EQ(result.PartList().begin()->ETag(), "3349DC700140D7F86A078484278075A9");
    EXPECT_EQ(result.PartList().begin()->PartNumber(), 1);
    EXPECT_EQ(result.PartList().begin()->LastModified(), "2012-02-23T07:01:34.000Z");
    EXPECT_EQ(result.PartList().begin()->Size(), 6291456LL);

    EXPECT_EQ(result.PartList().rbegin()->ETag(), "7265F4D211B56873A381D321F586E4A9");
    EXPECT_EQ(result.PartList().rbegin()->PartNumber(), 5);
    EXPECT_EQ(result.PartList().rbegin()->LastModified(), "2012-02-23T07:02:03.000Z");
    EXPECT_EQ(result.PartList().rbegin()->Size(), 1024LL);
}

TEST_F(MultipartUploadTest, ListPartsResultEmptyTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
            <ListPartsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                <Bucket></Bucket>
                <Key></Key>
                <UploadId></UploadId>
                <NextPartNumberMarker></NextPartNumberMarker>
                <MaxParts></MaxParts>
                <IsTruncated></IsTruncated>
                <Part>
                    <PartNumber></PartNumber>
                    <LastModified></LastModified>
                    <ETag></ETag>
                    <Size></Size>
                </Part>
                <Part>
                    <PartNumber></PartNumber>
                    <LastModified></LastModified>
                    <ETag></ETag>
                    <Size></Size>
                </Part>
            </ListPartsResult>)";
    ListPartsResult result(xml);
    EXPECT_EQ(result.Bucket(), "");
    EXPECT_EQ(result.Key(), "");
    EXPECT_EQ(result.UploadId(), "");
    EXPECT_EQ(result.NextPartNumberMarker(), 0UL);
    EXPECT_EQ(result.MaxParts(), 0U);
    EXPECT_EQ(result.IsTruncated(), false);
    EXPECT_EQ(result.PartList().size(), 2U);
    EXPECT_EQ(result.PartList().begin()->ETag(), "");
    EXPECT_EQ(result.PartList().begin()->PartNumber(), 0);
    EXPECT_EQ(result.PartList().begin()->LastModified(), "");
    EXPECT_EQ(result.PartList().begin()->Size(), 0);

    EXPECT_EQ(result.PartList().rbegin()->ETag(), "");
    EXPECT_EQ(result.PartList().rbegin()->PartNumber(), 0);
    EXPECT_EQ(result.PartList().rbegin()->LastModified(), "");
    EXPECT_EQ(result.PartList().rbegin()->Size(), 0);
}

TEST_F(MultipartUploadTest, MultipartUploadComplexStepTest)
{
    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadComplexStepTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());

    // Set the part size 
    const int partSize = 100 * 1024;
    const int64_t fileLength = GetFileLength(sourceFile);

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
            auto uploadPartOutcome = Client->UploadPart(request);
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
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_FALSE(cOutcome.result().RequestId().empty());

    auto gOutcome = Client->GetObject(BucketName, targetObjectKey);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);
    auto calcMd5 = ComputeContentMD5(*gOutcome.result().Content());
    EXPECT_EQ(calcMd5, TestFileMd5);
}

TEST_F(MultipartUploadTest, MultipartUploadComplexStepTestWithoutCrc)
{
    ClientConfiguration conf;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    OssClient *XClient = &client;

    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadComplexStepTestWithoutCrc");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = XClient->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    // Set the part size 
    const int partSize = 100 * 1024;
    const int64_t fileLength = GetFileLength(sourceFile);

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
            auto uploadPartOutcome = XClient->UploadPart(request);
            EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

            // Save the result
            Part part(i + 1, uploadPartOutcome.result().ETag());
            partETags.push_back(part);
        }
    }

    auto lmuOutcome = XClient->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);

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
    auto cOutcome = XClient->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);


    auto gOutcome = XClient->GetObject(BucketName, targetObjectKey);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);
    auto calcMd5 = ComputeContentMD5(*gOutcome.result().Content());
    EXPECT_EQ(calcMd5, TestFileMd5);
}

TEST_F(MultipartUploadTest, CompleteMultipartUploadWithListParts)
{
    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("CompleteMultipartUploadWithListParts");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    // Set the part size 
    const int partSize = 100 * 1024;
    const int64_t fileLength = GetFileLength(sourceFile);

    auto partCount = CalculatePartCount(fileLength, partSize);

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
            auto uploadPartOutcome = Client->UploadPart(request);
            EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

            // Save the result
            //Part part(i+1, uploadPartOutcome.result().ETag());
        }
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);

    std::string uploadId;

    for (auto const &upload : lmuOutcome.result().MultipartUploadList())
    {
        if (upload.UploadId == initOutcome.result().UploadId())
        {
            uploadId = upload.UploadId;
        }
    }
    EXPECT_EQ(uploadId.empty(), false);

    ListPartsRequest listRequest(BucketName, targetObjectKey);
    listRequest.setUploadId(uploadId);
    auto listOutcome = Client->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, listOutcome.result().PartList());
    completeRequest.setUploadId(uploadId);
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObject(BucketName, targetObjectKey);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);
    auto calcMd5 = ComputeContentMD5(*gOutcome.result().Content());
    EXPECT_EQ(calcMd5, TestFileMd5);
}

TEST_F(MultipartUploadTest, MultipartUploadAbortInMiddleTest)
{
    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadAbortInMiddleTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    // Set the part size 
    const int partSize = 100 * 1024;;
    const int64_t fileLength = GetFileLength(sourceFile);

    auto partCount = CalculatePartCount(fileLength, partSize);

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
            auto uploadPartOutcome = Client->UploadPart(request);
            EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

        }
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);

    std::string uploadId;
    for (auto const &upload : lmuOutcome.result().MultipartUploadList())
    {
        if (upload.UploadId == initOutcome.result().UploadId())
        {
            uploadId = upload.UploadId;
        }
    }
    EXPECT_EQ(uploadId.empty(), false);

    auto abortOutcome = Client->AbortMultipartUpload(AbortMultipartUploadRequest(BucketName, targetObjectKey, uploadId));
    EXPECT_EQ(abortOutcome.isSuccess(), true);

    abortOutcome = Client->AbortMultipartUpload(AbortMultipartUploadRequest(BucketName, targetObjectKey, uploadId));
    EXPECT_EQ(abortOutcome.isSuccess(), false);
    EXPECT_EQ(abortOutcome.error().Code(), "NoSuchUpload");
}

TEST_F(MultipartUploadTest, MultipartUploadPartCopyComplexStepTest)
{
    //put test file
    auto testKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest-TestKey");
    Client->PutObject(BucketName, testKey, TestFile);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, testKey), true);

    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    // Set the part size 
    const int partSize = 100 * 1024;
    const int64_t fileLength = GetFileLength(sourceFile);

    auto partCount = CalculatePartCount(fileLength, partSize);
    for (auto i = 0; i < partCount; i++)
    {
        // Skip to the start position
        int64_t skipBytes = partSize * i;
        int64_t position = skipBytes;

        // calculate the part size
        auto size = partSize < (fileLength - skipBytes) ? partSize : fileLength - skipBytes;

        UploadPartCopyRequest request(BucketName, targetObjectKey, BucketName, testKey);
        request.setPartNumber(i + 1);
        request.setUploadId(initOutcome.result().UploadId());
        request.setCopySourceRange(position, position + size - 1);
        auto uploadPartOutcome = Client->UploadPartCopy(request);
        EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
        EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);

    std::string uploadId;

    for (auto const &upload : lmuOutcome.result().MultipartUploadList())
    {
        if (upload.UploadId == initOutcome.result().UploadId())
        {
            uploadId = upload.UploadId;
        }
    }
    EXPECT_EQ(uploadId.empty(), false);

    ListPartsRequest listRequest(BucketName, targetObjectKey);
    listRequest.setUploadId(uploadId);
    auto listOutcome = Client->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, listOutcome.result().PartList());
    completeRequest.setUploadId(uploadId);
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObject(BucketName, targetObjectKey);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);
    auto calcMd5 = ComputeContentMD5(*gOutcome.result().Content());
    EXPECT_EQ(calcMd5, TestFileMd5);
}

TEST_F(MultipartUploadTest, MultipartUploadPartCopyWithSpecialKeyNameTest)
{
    //put test file
    //u8"中文名字+"
    unsigned char buff[] = { 0xE4, 0XB8, 0XAD, 0XE6, 0X96, 0X87, 0XE5, 0X90, 0X8D, 0XE5, 0XAD, 0X97, 0X2B, 0X0 };
    std::string u8_str((char *)buff);//= u8"中文名字+";
    auto testKey = u8_str;
    Client->PutObject(BucketName, testKey, TestFile);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, testKey), true);

    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    // Set the part size 
    const int partSize = 100 * 1024;
    const int64_t fileLength = GetFileLength(sourceFile);

    auto partCount = CalculatePartCount(fileLength, partSize);

    //upload the file
    //std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(sourceFile, std::ios::in | std::ios::binary);
    //EXPECT_EQ(content->good(), true);
    //if (content->good())
    {
        for (auto i = 0; i < partCount; i++)
        {
            // Skip to the start position
            int64_t skipBytes = partSize * i;
            int64_t position = skipBytes;

            // calculate the part size
            auto size = partSize < (fileLength - skipBytes) ? partSize : fileLength - skipBytes;

            // Create a UploadPartRequest, uploading parts
            //content->clear();
            //content->seekg(position, content->beg);
            UploadPartCopyRequest request(BucketName, targetObjectKey, BucketName, testKey);
            request.setPartNumber(i + 1);
            request.setUploadId(initOutcome.result().UploadId());
            request.setCopySourceRange(position, position + size - 1);
            auto uploadPartOutcome = Client->UploadPartCopy(request);
            EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
        }
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);

    std::string uploadId;

    for (auto const &upload : lmuOutcome.result().MultipartUploadList())
    {
        if (upload.UploadId == initOutcome.result().UploadId())
        {
            uploadId = upload.UploadId;
        }
    }
    EXPECT_EQ(uploadId.empty(), false);

    ListPartsRequest listRequest(BucketName, targetObjectKey);
    listRequest.setUploadId(uploadId);
    auto listOutcome = Client->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, listOutcome.result().PartList());
    completeRequest.setUploadId(uploadId);
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObject(BucketName, targetObjectKey);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);
    auto calcMd5 = ComputeContentMD5(*gOutcome.result().Content());
    EXPECT_EQ(calcMd5, TestFileMd5);
}

TEST_F(MultipartUploadTest, UploadPartCopyWithSourceIfMatchTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("upload-part-copy-object-source");
    std::string targetKey = TestUtils::GetObjectKey("upload-part-copy-object-target");

    // put the source obj
    auto putObjectContent = TestUtils::GetRandomStream(102400);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    std::string eTag = putObjectOutcome.result().ETag();

    // apply upload id
    auto uploadPartCopyInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, targetKey));
    EXPECT_EQ(uploadPartCopyInitOutcome.isSuccess(), true);
    auto uploadId = uploadPartCopyInitOutcome.result().UploadId();

    {
        UploadPartCopyRequest request(BucketName, targetKey);
        request.SetCopySource(BucketName, sourceKey);
        request.setUploadId(uploadId);
        request.setPartNumber(1);
        request.SetSourceIfMatchETag("ErrorETag");
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        // http status code : 412
        EXPECT_EQ(outcome.error().Code(), "PreconditionFailed");
    }

    // success upload part copy with the real ETag
    UploadPartCopyRequest request(BucketName, targetKey);
    request.SetCopySource(BucketName, sourceKey);
    request.setUploadId(uploadId);
    request.setPartNumber(1);
    request.SetSourceIfMatchETag(eTag);
    auto outcome = Client->UploadPartCopy(request);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(MultipartUploadTest, UploadPartCopyWithSourceIfNoneMatchTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("upload-part-copy-object-source");
    std::string targetKey = TestUtils::GetObjectKey("upload-part-copy-object-target");

    // put the source obj
    auto putObjectContent = TestUtils::GetRandomStream(102400);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    std::string eTag = putObjectOutcome.result().ETag();

    // apply upload id
    auto uploadPartCopyInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, targetKey));
    EXPECT_EQ(uploadPartCopyInitOutcome.isSuccess(), true);
    auto uploadId = uploadPartCopyInitOutcome.result().UploadId();

    {
        UploadPartCopyRequest request(BucketName, targetKey);
        request.SetCopySource(BucketName, sourceKey);
        request.setUploadId(uploadId);
        request.setPartNumber(1);
        request.SetSourceIfNotMatchETag(eTag);
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ServerError:304");
    }

    // success upload part copy with the real ETag
    UploadPartCopyRequest request(BucketName, targetKey);
    request.SetCopySource(BucketName, sourceKey);
    request.setUploadId(uploadId);
    request.setPartNumber(1);
    request.SetSourceIfNotMatchETag("ErrorETag");
    auto outcome = Client->UploadPartCopy(request);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(MultipartUploadTest, UploadPartCopyWithIfUnmodifiedSinceTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("upload-part-copy-object-source");
    std::string targetKey = TestUtils::GetObjectKey("upload-part-copy-object-target");

    // put the source obj
    auto putObjectContent = TestUtils::GetRandomStream(102400);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    std::string eTag = putObjectOutcome.result().ETag();

    // apply upload id
    auto uploadPartCopyInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, targetKey));
    EXPECT_EQ(uploadPartCopyInitOutcome.isSuccess(), true);
    auto uploadId = uploadPartCopyInitOutcome.result().UploadId();

    std::string beforeChangeTime = TestUtils::GetGMTString(-100);
    std::string afterChangeTime = TestUtils::GetGMTString(100);

    // the target time before the last modified time of sourceObj
    {
        UploadPartCopyRequest request(BucketName, targetKey);
        request.SetCopySource(BucketName, sourceKey);
        request.setUploadId(uploadId);
        request.setPartNumber(1);
        request.SetSourceIfUnModifiedSince(beforeChangeTime);
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        // http status code : 412
        EXPECT_EQ(outcome.error().Code(), "PreconditionFailed");
    }

    // the target time equals the last modified time of sourceObj
    {
        auto objectMetaOutcome = Client->GetObjectMeta(BucketName, sourceKey);
        EXPECT_EQ(objectMetaOutcome.isSuccess(), true);

        UploadPartCopyRequest request(BucketName, targetKey);
        request.SetCopySource(BucketName, sourceKey);
        request.setUploadId(uploadId);
        request.setPartNumber(1);
        request.SetSourceIfUnModifiedSince(objectMetaOutcome.result().LastModified());
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    // the target time after the last modified time of sourceObj
    UploadPartCopyRequest request(BucketName, targetKey);
    request.SetCopySource(BucketName, sourceKey);
    request.setUploadId(uploadId);
    request.setPartNumber(1);
    request.SetSourceIfUnModifiedSince(afterChangeTime);
    auto outcome = Client->UploadPartCopy(request);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(MultipartUploadTest, UploadPartCopyWithIfModifiedSinceTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("upload-part-copy-object-source");
    std::string targetKey = TestUtils::GetObjectKey("upload-part-copy-object-target");

    // put the source obj
    auto putObjectContent = TestUtils::GetRandomStream(102400);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);

    // apply upload id
    auto uploadPartCopyInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, targetKey));
    EXPECT_EQ(uploadPartCopyInitOutcome.isSuccess(), true);
    auto uploadId = uploadPartCopyInitOutcome.result().UploadId();

    // time
    std::string beforeChangeTime = TestUtils::GetGMTString(-100);
    std::string afterChangeTime = TestUtils::GetGMTString(100);

    {
        UploadPartCopyRequest request(BucketName, targetKey);
        request.SetCopySource(BucketName, sourceKey);
        request.setUploadId(uploadId);
        request.setPartNumber(1);
        request.SetSourceIfModifiedSince(beforeChangeTime);
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    {
        UploadPartCopyRequest request(BucketName, targetKey);
        request.SetCopySource(BucketName, sourceKey);
        request.setUploadId(uploadId);
        request.setPartNumber(1);
        request.SetSourceIfModifiedSince(afterChangeTime);
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ServerError:304");
    }
}

TEST_F(MultipartUploadTest, UnormalUploadPartCopyTest)
{
    std::string sourceBucket = TestUtils::GetBucketName("unormal-upload-part-copy-bucket-source");
    std::string targetBucket = TestUtils::GetBucketName("unormal-upload-part-copy-bucket-target");
    std::string sourceKey = TestUtils::GetObjectKey("unormal-upload-part-copy-object-source");
    std::string targetKey = TestUtils::GetObjectKey("unormal-upload-part-copy-object-target");
    // Set length of parts less than minimum limit(100KB)

    EXPECT_EQ(Client->CreateBucket(sourceBucket).isSuccess(), true);
    EXPECT_EQ(Client->CreateBucket(targetBucket).isSuccess(), true);

    // put object into source bucket
    auto putObjectContent = TestUtils::GetRandomStream(102400);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(sourceBucket, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    std::string eTag = putObjectOutcome.result().ETag();

    // apply upload id for target bucket
    auto uploadPartCopyInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(targetBucket, targetKey));
    EXPECT_EQ(uploadPartCopyInitOutcome.isSuccess(), true);
    auto uploadId = uploadPartCopyInitOutcome.result().UploadId();

    // Copy part to non-existent target bucket
    {
        std::string nonexistentTargetBucket = TestUtils::GetBucketName("nonexistent-target-key");
        UploadPartCopyRequest request(nonexistentTargetBucket, targetKey);
        request.SetCopySource(sourceBucket, sourceKey);
        request.setUploadId(uploadId);
        request.setPartNumber(1);
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
    }

    // Copy part from non-existent source bucket
    {
        std::string nonexistentSourceBucket = TestUtils::GetBucketName("nonexistent-source-bucket");
        UploadPartCopyRequest request(targetBucket, targetKey, nonexistentSourceBucket, sourceKey, uploadId, 1);
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
    }

    // Copy part with non-existent source key
    {
        std::string nonexistentSourceKey = "nonexistent-source-key";
        UploadPartCopyRequest request(targetBucket, targetKey, sourceBucket, nonexistentSourceKey, uploadId, 1);
        auto outcome = Client->UploadPartCopy(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "NoSuchKey");
    }

    // Copy part with non-existent upload id
    {
        auto outcome = Client->UploadPartCopy(UploadPartCopyRequest(targetBucket, targetKey,
            sourceBucket, sourceKey, "id", 1));
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "NoSuchUpload");
    }

    // Upload part copy
    PartList partETags;
    auto firstUploadOutcome = Client->UploadPartCopy(UploadPartCopyRequest(targetBucket, targetKey,
        sourceBucket, sourceKey, uploadId, 1));
    EXPECT_EQ(firstUploadOutcome.isSuccess(), true);
    // EXPECT_EQ(firstUploadOutcome.result().ETag(), eTag);
    // partETags.push_back(Part(1, firstUploadOutcome.result().ETag()));

    auto secondUploadOutcome = Client->UploadPartCopy(UploadPartCopyRequest(targetBucket, targetKey,
        sourceBucket, sourceKey, uploadId, 2));
    EXPECT_EQ(secondUploadOutcome.isSuccess(), true);
    // EXPECT_EQ(secondUploadOutcome.result().ETag(), eTag);
    // partETags.push_back(Part(2, secondUploadOutcome.result().ETag()));

    auto partListOutcome = Client->ListParts(ListPartsRequest(targetBucket, targetKey, uploadId));
    EXPECT_EQ(partListOutcome.isSuccess(), true);
    EXPECT_EQ(partListOutcome.result().PartList().size(), 2U);

    // Try to complete multipart upload with all uploaded parts
    CompleteMultipartUploadRequest request(targetBucket, targetKey, partListOutcome.result().PartList(), uploadId);
    auto completeOutcome = Client->CompleteMultipartUpload(request);
    EXPECT_EQ(completeOutcome.isSuccess(), true);

    // get the source object
    EXPECT_EQ(Client->DoesObjectExist(targetBucket, targetKey), true);

    // delete source bucket and target bucket
    EXPECT_EQ(Client->DeleteObject(sourceBucket, sourceKey).isSuccess(), true);
    EXPECT_EQ(Client->DeleteObject(targetBucket, targetKey).isSuccess(), true);
    EXPECT_EQ(Client->DeleteBucket(sourceBucket).isSuccess(), true);
    EXPECT_EQ(Client->DeleteBucket(targetBucket).isSuccess(), true);
}


TEST_F(MultipartUploadTest, ListMultipartUploadsTest)
{
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsTest");

    for (size_t i = 0; i < 20; i++) {
        auto key = targetObjectKey;
        key.append("-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_TRUE(lmuOutcome.result().MultipartUploadList().size() >= 20UL);
}

TEST_F(MultipartUploadTest, ListMultipartUploadsByPrefixTest)
{
    const size_t TestLoop = 10;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsByPrefixTest");

    for (size_t i = 0; i < TestLoop; i++) {
        auto key = targetObjectKey;
        key.append("-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_TRUE(lmuOutcome.result().MultipartUploadList().size() >= TestLoop);

    ListMultipartUploadsRequest request(BucketName);
    request.setPrefix("ListMultipartUploadsByPrefixTest");
    request.setMaxUploads(1U);
    size_t cnt = 0;
    do {
        lmuOutcome = Client->ListMultipartUploads(request);
        EXPECT_EQ(lmuOutcome.isSuccess(), true);
        EXPECT_EQ(lmuOutcome.result().MultipartUploadList().size(), 1U);
        request.setKeyMarker(lmuOutcome.result().NextKeyMarker());
        request.setUploadIdMarker(lmuOutcome.result().NextUploadIdMarker());
        cnt++;
    } while (lmuOutcome.result().IsTruncated());

    EXPECT_EQ(cnt, TestLoop);
}

TEST_F(MultipartUploadTest, ListMultipartUploadsByPrefixAndKeyMarkerTest)
{
    const size_t TestLoop = 10;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsByPrefixAndKeyMarkerTest");

    for (size_t i = 0; i < TestLoop; i++) {
        auto key = targetObjectKey;
        key.append("-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }

    auto index = TestLoop / 2;
    auto keyMarker = targetObjectKey.append("-").append(std::to_string(index));
    index = TestLoop - index - 1;
    ListMultipartUploadsRequest request(BucketName);
    request.setPrefix("ListMultipartUploadsByPrefixAndKeyMarkerTest");
    request.setMaxUploads(TestLoop);
    request.setKeyMarker(keyMarker);
    auto lmuOutcome = Client->ListMultipartUploads(request);
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_EQ(lmuOutcome.result().MultipartUploadList().size(), index);
}

TEST_F(MultipartUploadTest, ListMultipartUploadsByPrefixWithEncodingTypeTest)
{
    const size_t TestLoop = 5;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsWithEncodingTypeTest");
    targetObjectKey.push_back(0x1c); targetObjectKey.push_back(0x1a);

    for (size_t i = 0; i < TestLoop; i++) {
        auto key = targetObjectKey;
        key.append("-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }

    ListMultipartUploadsRequest request(BucketName);
    request.setPrefix(targetObjectKey);
    request.setEncodingType("url");
    request.setMaxUploads(1U);
    auto lmuOutcome = Client->ListMultipartUploads(request);
    size_t cnt = 0;
    do {
        lmuOutcome = Client->ListMultipartUploads(request);
        EXPECT_EQ(lmuOutcome.isSuccess(), true);
        EXPECT_EQ(lmuOutcome.result().MultipartUploadList().size(), 1U);
        EXPECT_EQ(lmuOutcome.result().NextKeyMarker().compare(0, targetObjectKey.size(), targetObjectKey), 0);
        EXPECT_EQ(lmuOutcome.result().MultipartUploadList().begin()->Key.compare(0, targetObjectKey.size(), targetObjectKey), 0);
        request.setKeyMarker(lmuOutcome.result().NextKeyMarker());
        request.setUploadIdMarker(lmuOutcome.result().NextUploadIdMarker());
        cnt++;
    } while (lmuOutcome.result().IsTruncated());

    EXPECT_EQ(cnt, TestLoop);

}

TEST_F(MultipartUploadTest, ListMultipartUploadsWithDelimiterTest)
{
    const size_t TestLoop = 10;
    std::vector<std::string> commonPrefixs;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsWithDelimiterTest");

    for (size_t i = 0; i < TestLoop; i++) {
        auto key = targetObjectKey;
        key.append("/").append("-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }
    targetObjectKey.append("/");
    commonPrefixs.push_back(targetObjectKey);

    targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsWithDelimiterTest");
    for (size_t i = 0; i < TestLoop; i++) {
        auto key = targetObjectKey;
        key.append("/").append("-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }
    targetObjectKey.append("/");
    commonPrefixs.push_back(targetObjectKey);

    ListMultipartUploadsRequest request(BucketName);
    request.setPrefix("ListMultipartUploadsWithDelimiterTest");
    request.setDelimiter("/");
    auto lmuOutcome = Client->ListMultipartUploads(request);
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_EQ(lmuOutcome.result().CommonPrefixes().size(), 2U);
    size_t index = 0;
    auto first = commonPrefixs.begin();
    for (auto const &prefix : lmuOutcome.result().CommonPrefixes()) {
        EXPECT_EQ(prefix, *first);
        first++;
        index++;
    }
    EXPECT_EQ(index, 2U);
}

TEST_F(MultipartUploadTest, ListMultipartUploadsWithDelimiterAndEncodingTypeTest)
{
    const size_t TestLoop = 5;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsWithDelimiterAndEncodingTypeTest");
    targetObjectKey.push_back(0x1c); targetObjectKey.push_back(0x1a);

    for (size_t i = 0; i < TestLoop; i++) {
        auto key = targetObjectKey;
        key.append("/-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }

    auto commonKey = targetObjectKey;
    commonKey.append("/");
    ListMultipartUploadsRequest request(BucketName);
    request.setPrefix(targetObjectKey);
    request.setEncodingType("url");
    request.setDelimiter("/");
    auto lmuOutcome = Client->ListMultipartUploads(request);
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_EQ(lmuOutcome.result().CommonPrefixes().size(), 1U);
    EXPECT_EQ(*lmuOutcome.result().CommonPrefixes().begin(), commonKey);
}

TEST_F(MultipartUploadTest, ListMultipartUploadsWithMuiltiUploadForSameKeyTest)
{
    const size_t TestLoop = 10;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("ListMultipartUploadsWithMuiltiUploadForSameKeyTest");

    for (size_t i = 0; i < TestLoop; i++) {
        auto key = targetObjectKey;
        key.append("-").append(std::to_string(i));
        InitiateMultipartUploadRequest request(BucketName, key);
        auto initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);

        initOutcome = Client->InitiateMultipartUpload(request);
        EXPECT_EQ(initOutcome.isSuccess(), true);
    }
    ListMultipartUploadsRequest request(BucketName);
    request.setPrefix("ListMultipartUploadsWithMuiltiUploadForSameKeyTest");
    auto lmuOutcome = Client->ListMultipartUploads(request);
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_EQ(lmuOutcome.result().MultipartUploadList().size(), 2 * TestLoop);

    auto prefixKey = targetObjectKey;
    prefixKey.append("-0");
    request.setPrefix(prefixKey);
    lmuOutcome = Client->ListMultipartUploads(request);
    EXPECT_EQ(lmuOutcome.isSuccess(), true);
    EXPECT_EQ(lmuOutcome.result().MultipartUploadList().size(), 2UL);
    for (auto const &upload : lmuOutcome.result().MultipartUploadList()) {
        EXPECT_EQ(upload.Key, prefixKey);
    }
}

TEST_F(MultipartUploadTest, ListMultipartUploadsNegativeTest)
{
    //get target object name
    auto bucket = TestUtils::GetBucketName("cpp-sdk-multipartuploadtest");

    ListMultipartUploadsRequest request(bucket);
    auto outcome = Client->ListMultipartUploads(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(MultipartUploadTest, ListPartsTest)
{
    const size_t TestLoop = 10;
    //get target object name
    auto key = TestUtils::GetObjectKey("ListPartsTest");
    InitiateMultipartUploadRequest request(BucketName, key);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);


    for (int i = 0; i < static_cast<int>(TestLoop); i++) {
        std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(100 * 1024 + i);
        UploadPartRequest request(BucketName, key, content);
        request.setPartNumber(i + 1);
        request.setUploadId(initOutcome.result().UploadId());
        auto uploadPartOutcome = Client->UploadPart(request);
        EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    }

    ListPartsRequest listRequest(BucketName, key, initOutcome.result().UploadId());
    auto outcome = Client->ListParts(listRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().PartList().size(), TestLoop);
}

TEST_F(MultipartUploadTest, ListPartsSetpTest)
{
    const size_t TestLoop = 10;
    //get target object name
    auto key = TestUtils::GetObjectKey("ListPartsSetpTest");
    InitiateMultipartUploadRequest request(BucketName, key);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);


    for (int i = 0; i < static_cast<int>(TestLoop); i++) {
        std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(100 * 1024 + i);
        UploadPartRequest request(BucketName, key, content);
        request.setPartNumber(i + 1);
        request.setUploadId(initOutcome.result().UploadId());
        auto uploadPartOutcome = Client->UploadPart(request);
        EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    }

    ListPartsRequest listRequest(BucketName, key, initOutcome.result().UploadId());
    listRequest.setMaxParts(1);
    auto outcome = Client->ListParts(listRequest);
    size_t cnt = 0;
    do {
        outcome = Client->ListParts(listRequest);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().PartList().size(), 1U);
        EXPECT_EQ(outcome.result().PartNumberMarker(), cnt);
        listRequest.setPartNumberMarker(outcome.result().NextPartNumberMarker());
        cnt++;
    } while (outcome.result().IsTruncated());

    EXPECT_EQ(cnt, TestLoop);
}

TEST_F(MultipartUploadTest, ListPartsSetpUseEncodingTypeTest)
{
    const size_t TestLoop = 5;
    //get target object name
    auto key = TestUtils::GetObjectKey("ListPartsSetpUseEncodingTypeTest");
    key.push_back(0x1c); key.push_back(0x1a); key.append(".1.cd");

    InitiateMultipartUploadRequest request(BucketName, key);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    for (int i = 0; i < static_cast<int>(TestLoop); i++) {
        std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(100 * 1024 + i);
        UploadPartRequest request(BucketName, key, content);
        request.setPartNumber(i + 1);
        request.setUploadId(initOutcome.result().UploadId());
        auto uploadPartOutcome = Client->UploadPart(request);
        EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    }

    ListPartsRequest listRequest(BucketName, key, initOutcome.result().UploadId());
    listRequest.setMaxParts(1U);
    listRequest.setEncodingType("url");
    auto outcome = Client->ListParts(listRequest);
    size_t cnt = 0;
    do {
        outcome = Client->ListParts(listRequest);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().PartList().size(), 1U);
        EXPECT_EQ(outcome.result().Key(), key);
        EXPECT_EQ(outcome.result().MaxParts(), 1U);
        listRequest.setPartNumberMarker(outcome.result().NextPartNumberMarker());
        cnt++;
    } while (outcome.result().IsTruncated());

    EXPECT_EQ(cnt, TestLoop);
}

TEST_F(MultipartUploadTest, ListPartsNegativeTest)
{
    //get target object name
    auto key = TestUtils::GetObjectKey("ListPartsNegativeTest");

    ListPartsRequest listRequest(BucketName, key, "asdadf");
    listRequest.setMaxParts(1U);
    auto outcome = Client->ListParts(listRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchUpload");
}

}
}