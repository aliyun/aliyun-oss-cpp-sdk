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

class ObjectTrafficLimitTest : public ::testing::Test {
protected:
    ObjectTrafficLimitTest()
    {
    }

    ~ObjectTrafficLimitTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-object-traffic-limit");
        auto outcome1 = Client->CreateBucket(CreateBucketRequest(BucketName));
        BucketName1 = TestUtils::GetBucketName("cpp-sdk-object-copy-object");
        Client->CreateBucket(CreateBucketRequest(BucketName1));
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase() 
    {
        TestUtils::CleanBucket(*Client, BucketName);
        TestUtils::CleanBucket(*Client, BucketName1);
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
    static std::string BucketName1;
    class Timer
    {
    public:
        Timer() : begin_(std::chrono::high_resolution_clock::now()) {}
        void reset() { begin_ = std::chrono::high_resolution_clock::now(); }
        int64_t elapsed() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - begin_).count();
        }
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> begin_;
    };
};

std::shared_ptr<OssClient> ObjectTrafficLimitTest::Client = nullptr;
std::string ObjectTrafficLimitTest::BucketName = "";
std::string ObjectTrafficLimitTest::BucketName1 = "";


TEST_F(ObjectTrafficLimitTest, PutAndGetObject)
{
    Timer timer;
    std::string key = TestUtils::GetObjectKey("PutAndGetObject");
    /*set content 800 KB*/
    auto content = TestUtils::GetRandomStream(800*1024);

    PutObjectRequest putrequest(BucketName, key, content);
    /* set upload traffic limit 100KB/s*/
    putrequest.setTrafficLimit(819200*2);
    auto theory_time = (800 * 1024 * 8) / (819200*2);
    timer.reset();
    auto pOutcome = Client->PutObject(putrequest);
    auto diff_put = timer.elapsed();
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_NEAR(diff_put, theory_time, 1.0);

    GetObjectRequest getrequest(BucketName, key);
    getrequest.setTrafficLimit(8192000);
    auto outome = Client->GetObject(getrequest);
    EXPECT_EQ(outome.isSuccess(), true);
    auto responseHeader = outome.result().Metadata().HttpMetaData();
    EXPECT_EQ(responseHeader.find("x-oss-qos-delay-time") != responseHeader.end(), true);
    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, AppendObjectTest)
{
    Timer timer;
    std::string key = TestUtils::GetObjectKey("AppendObjectTest");
    /*set content 4800 KB*/
    auto content = TestUtils::GetRandomStream(800 * 1024);

    AppendObjectRequest apprequest(BucketName, key, content);
    /* set append traffic limit 100KB/s*/
    apprequest.setTrafficLimit(819200);
    auto theory_time = (800 * 1024 * 8) / (819200);
    timer.reset();
    auto pOutcome = Client->AppendObject(apprequest);
    auto time = timer.elapsed();
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_NEAR(time, theory_time, 1.0);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
}
       
TEST_F(ObjectTrafficLimitTest, CopyObjectTest)
{
    std::string key = TestUtils::GetObjectKey("CopyObjectTest");
    /*set content 4800 KB*/
    auto content = TestUtils::GetRandomStream(4800 * 1024);

    PutObjectRequest putrequest(BucketName, key, content);
    auto pOutcome = Client->PutObject(putrequest);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy");
    CopyObjectRequest copyRequest(BucketName1, objName2);
    copyRequest.setCopySource(BucketName, key);
    /* set copy traffic limit 800KB/s*/
    copyRequest.setTrafficLimit(819200*8);
    //auto theory_time = (4800 * 1024 * 8) / (819200 * 8);
    Timer timer;
    timer.reset();
    /*begin copy object*/
    CopyObjectOutcome copyOutCome = Client->CopyObject(copyRequest);
    //auto time = timer.elapsed();
    EXPECT_EQ(copyOutCome.isSuccess(), true);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);

    DeleteObjectRequest delRequest1(BucketName1, objName2);
    auto delOutcome1 = Client->DeleteObject(delRequest1);
    EXPECT_EQ(delOutcome1.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, UploadPartTest)
{
    auto key = TestUtils::GetObjectKey("InitiateMultipartUploadTest");
    InitiateMultipartUploadRequest request(BucketName, key);

    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);

    /*set content 4800 KB*/
    auto content = TestUtils::GetRandomStream(4800 * 1024);
    UploadPartRequest uprequest(BucketName, key, 1, initOutcome.result().UploadId(), content);
    /* set UploadPart traffic limit 800KB/s*/
    uprequest.setTrafficLimit(819200*8);
    auto theory_time = (4800 * 1024 * 8) / (819200 * 8);
    Timer timer;
    timer.reset();
    auto uploadPartOutcome = Client->UploadPart(uprequest);
    auto time = timer.elapsed();
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    EXPECT_NEAR(time, theory_time, 1.0);

    auto lOutcome = Client->ListParts(ListPartsRequest(BucketName, key, initOutcome.result().UploadId()));
    EXPECT_EQ(lOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest cRequest(BucketName, key,
        lOutcome.result().PartList(), initOutcome.result().UploadId());

    auto outcome = Client->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, UploadPartCopyTest)
{
    std::string key = TestUtils::GetObjectKey("UploadPartCopyTest");
    /*set content 1200 KB*/
    auto content = TestUtils::GetRandomStream(1200 * 1024);

    PutObjectRequest putrequest(BucketName, key, content);
    auto pOutcome = Client->PutObject(putrequest);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    //get target object name*/
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest");

    InitiateMultipartUploadRequest imuRequest(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(imuRequest);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    //Set the part size 
    const int partSize = 600 * 1024;
    const int64_t fileLength = 1200 * 1024;

    auto partCount = 2;
    for (auto i = 0; i < partCount; i++)
    {
        // Skip to the start position
        int64_t skipBytes = partSize * i;
        int64_t position = skipBytes;

        // calculate the part size
        auto size = partSize < (fileLength - skipBytes) ? partSize : fileLength - skipBytes;

        UploadPartCopyRequest request(BucketName, targetObjectKey, BucketName, key);
        request.setPartNumber(i + 1);
        request.setUploadId(initOutcome.result().UploadId());
        request.setCopySourceRange(position, position + size - 1);
        request.setTrafficLimit(819200*6);
        //auto theory_time = (600 * 1024 * 8) / (819200 * 6);
        Timer timer;
        timer.reset();
        auto uploadPartOutcome = Client->UploadPartCopy(request);
        //auto time = timer.elapsed();
        //EXPECT_EQ(time, theory_time);
        EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
        EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
    }

    auto lmuOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
    EXPECT_EQ(lmuOutcome.isSuccess(), true);

    std::string uploadId;

    for (auto const& upload : lmuOutcome.result().MultipartUploadList())
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

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);

    DeleteObjectRequest delRequest1(BucketName, targetObjectKey);
    auto delOutcome1 = Client->DeleteObject(delRequest1);
    EXPECT_EQ(delOutcome1.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, ResumableUploadObjectTest)
{

    std::string key = TestUtils::GetObjectKey("ResumableUploadObjectUnderPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadObjectUnderPartSize").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1200 * 1024);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(1200 * 1024);
    request.setThreadNum(1);
    request.setTrafficLimit(819200 * 6);
    auto theory_time = (1200 * 1024 * 8) / (819200 * 6);
    Timer timer;
    timer.reset();
    auto outcome = Client->ResumableUploadObject(request);
    auto time = timer.elapsed();
    EXPECT_NEAR(time, theory_time, 1.0);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(BucketName, key);
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, ResumableDownloadObjectMultipartTest)
{
    // upload object
    std::string key = TestUtils::GetObjectKey("UnnormalResumableDownloadObjectWithDisableRequest");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalResumableDownloadObjectWithDisableRequest").append(".tmp");
    std::string targetKey = TestUtils::GetObjectKey("UnnormalResumableDownloadTargetObject");
    TestUtils::WriteRandomDatatoFile(tmpFile, 4800 * 1024);
    auto putObjectOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);

    DownloadObjectRequest request(BucketName, key, targetKey);
    request.setPartSize(1200 * 1024);
    request.setThreadNum(1);
    request.setTrafficLimit(819200 * 6);
    auto theory_time = (4800 * 1024 * 8) / (819200 * 6);
    Timer timer;
    timer.reset();
    auto outcome = Client->ResumableDownloadObject(request);
    auto time = timer.elapsed();
    EXPECT_NEAR(time, theory_time,1.0);
    EXPECT_EQ(outcome.isSuccess(), true);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);

}

TEST_F(ObjectTrafficLimitTest, ResumableUploadObjectMultipartTest)
{

    std::string key = TestUtils::GetObjectKey("ResumableUploadObjectUnderPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadObjectUnderPartSize").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1800 * 1024);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(1200 * 1024);
    request.setThreadNum(1);
    request.setTrafficLimit(819200 * 2);
    auto theory_time = (1800 * 1024 * 8) / (819200 * 2);
    Timer timer;
    timer.reset();
    auto outcome = Client->ResumableUploadObject(request);
    auto time = timer.elapsed();
    EXPECT_NEAR(time, theory_time, 2.0);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(BucketName, key);
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, ResumableDownloadObjectTest)
{
    // upload object
    std::string key = TestUtils::GetObjectKey("UnnormalResumableDownloadObjectWithDisableRequest");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalResumableDownloadObjectWithDisableRequest").append(".tmp");
    std::string targetKey = TestUtils::GetObjectKey("UnnormalResumableDownloadTargetObject");
    TestUtils::WriteRandomDatatoFile(tmpFile, 600 * 1024);
    auto putObjectOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);

    DownloadObjectRequest request(BucketName, key, targetKey);
    request.setPartSize(1200 * 1024);
    request.setThreadNum(1);
    request.setTrafficLimit(819200);
    auto theory_time = (600 * 1024 * 8) / (819200);
    Timer timer;
    timer.reset();
    auto outcome = Client->ResumableDownloadObject(request);
    auto time = timer.elapsed();
    EXPECT_NEAR(time, theory_time, 1.0);
    EXPECT_EQ(outcome.isSuccess(), true);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = Client->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, PutAndGetObjectWithPreSignedUriTest)
{
    std::string key = TestUtils::GetObjectKey("PutObjectWithPreSignedUriAndCrc");
    std::shared_ptr<std::iostream> content = TestUtils::GetRandomStream(1024);

    GeneratePresignedUrlRequest putrequest(BucketName, key, Http::Put);
    putrequest.setExpires(GetExpiresDelayS(120));
    putrequest.setTrafficLimit(819200);
    auto urlOutcome = Client->GeneratePresignedUrl(putrequest);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    EXPECT_TRUE(urlOutcome.result().find("x-oss-traffic-limit=819200") != std::string::npos);

    auto pOutcome = Client->PutObjectByUrl(urlOutcome.result(), content);
    //EXPECT_EQ(pOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest getrequest(BucketName, key, Http::Get);
    getrequest.setExpires(GetExpiresDelayS(120));
    getrequest.setTrafficLimit(819201);
    urlOutcome = Client->GeneratePresignedUrl(getrequest);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    EXPECT_TRUE(urlOutcome.result().find("x-oss-traffic-limit=819201") != std::string::npos);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    //EXPECT_EQ(gOutcome.isSuccess(), true);
}

TEST_F(ObjectTrafficLimitTest, NormalResumableCopyWithSizelessPartSizeTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectOverPartSize");
    std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectOverPartSize");
    // put object into bucket
    auto putObjectContent = TestUtils::GetRandomStream(102400 - 2);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    // Copy Object
    MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    request.setTrafficLimit(819201);
    auto outcome = Client->ResumableCopyObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
}
}
}