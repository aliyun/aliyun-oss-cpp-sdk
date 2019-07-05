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
#include <alibabacloud/oss/Types.h>
#include "../Config.h"
#include "../Utils.h"
#include "src/utils/FileSystemUtils.h"
#include "src/utils/Utils.h"
#include <fstream>


namespace AlibabaCloud
{ 
namespace OSS 
{

class ObjectRequestPaymentTest : public ::testing::Test
{
protected:
    ObjectRequestPaymentTest()
    {
    }

    ~ObjectRequestPaymentTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        ClientConfiguration conf;
        conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());

        BucketName1 = TestUtils::GetBucketName("cpp-sdk-objectcopy1");
        CreateBucketOutcome outCome = Client->CreateBucket(CreateBucketRequest(BucketName1));
        EXPECT_EQ(outCome.isSuccess(), true);

        SetBucketPolicyRequest request(BucketName1);
        std::string policy = std::string("{\"Version\":\"1\",\"Statement\":[{\"Action\":[\"oss:*\"],\"Effect\": \"Allow\",")
        .append("\"Principal\":[\"").append(Config::PayerUID).append("\"],")
        .append("\"Resource\": [\"acs:oss:*:*:").append(BucketName1).append("\",\"acs:oss:*:*:").append(BucketName1).append("/*\"]}]}");
        request.setPolicy(policy);
        auto policyOutcome1 = Client->SetBucketPolicy(request);
        EXPECT_EQ(policyOutcome1.isSuccess(), true);

        SetBucketRequestPaymentRequest setRequest(BucketName1);
        setRequest.setRequestPayer(RequestPayer::Requester);
        VoidOutcome setOutcome = Client->SetBucketRequestPayment(setRequest);
        EXPECT_EQ(setOutcome.isSuccess(), true);

        GetBucketRequestPaymentRequest getRequest(BucketName1);
        GetBucketPaymentOutcome getOutcome = Client->GetBucketRequestPayment(getRequest);
        EXPECT_EQ(getOutcome.isSuccess(), true);
        EXPECT_EQ(getOutcome.result().Payer(), RequestPayer::Requester);

        TestFile = TestUtils::GetTargetFileName("cpp-sdk-multipartupload");
        TestUtils::WriteRandomDatatoFile(TestFile, 500*1024);

        PayerClient = std::make_shared<OssClient>(Config::Endpoint, Config::PayerAccessKeyId, Config::PayerAccessKeySecret, ClientConfiguration());

    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName1);
        Client = nullptr;
        PayerClient = nullptr;
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
    static std::shared_ptr<OssClient> PayerClient;
    static std::string BucketName1;
    static std::string TestFile;
};

std::shared_ptr<OssClient> ObjectRequestPaymentTest::Client = nullptr;
std::shared_ptr<OssClient> ObjectRequestPaymentTest::PayerClient = nullptr;
std::string ObjectRequestPaymentTest::BucketName1 = "";
std::string ObjectRequestPaymentTest::TestFile = "";


int64_t ObjectRequestPaymentTest::GetFileLength(const std::string file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

int ObjectRequestPaymentTest::CalculatePartCount(int64_t totalSize, int singleSize)
{
    // Calculate the part count
    auto partCount = (int)(totalSize / singleSize);
    if (totalSize % singleSize != 0)
    {
        partCount++;
    }
    return partCount;
}

TEST_F(ObjectRequestPaymentTest, PutObjectAndCopyObject)
{
    std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
    std::string text1 = "hellowworld";
    PutObjectRequest putRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1));
    PutObjectOutcome putOutcome = PayerClient->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), false);
    EXPECT_EQ(putOutcome.error().Code(), "AccessDenied");
    putRequest.setRequestPayer(RequestPayer::Requester);
    putOutcome = PayerClient->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
    CopyObjectRequest copyRequest(BucketName1, objName2);
    copyRequest.setCopySource(BucketName1, objName1);
    CopyObjectOutcome copyOutCome = PayerClient->CopyObject(copyRequest);
    EXPECT_EQ(copyOutCome.isSuccess(), false);
    EXPECT_EQ(copyOutCome.error().Code(), "AccessDenied");
    copyRequest.setRequestPayer(RequestPayer::Requester);
    //begin copy object
    copyOutCome = PayerClient->CopyObject(copyRequest);
    EXPECT_EQ(copyOutCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, ListObjects)
{
    ListObjectsRequest request(BucketName1);
    ListObjectOutcome outCome = PayerClient->ListObjects(request);
    EXPECT_EQ(outCome.isSuccess(), false);
    EXPECT_EQ(outCome.error().Code(), "AccessDenied");
    request.setRequestPayer(RequestPayer::Requester);
    outCome = PayerClient->ListObjects(request);
    EXPECT_EQ(outCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, MultipartUpload)
{
    auto key = TestUtils::GetObjectKey("InitiateMultipartUploadWithFullSettingTest");
    ObjectMetaData metaData;
    metaData.setCacheControl("No-Cache");
    metaData.setContentType("user/test");
    metaData.setContentEncoding("myzip");
    metaData.setContentDisposition("test.zip");
    metaData.UserMetaData()["test"] = "test";
    metaData.UserMetaData()["data"] = "data";
    InitiateMultipartUploadRequest request(BucketName1, key, metaData);
    auto initOutcome = PayerClient->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), false);
    EXPECT_EQ(initOutcome.error().Code(), "AccessDenied");
    request.setRequestPayer(RequestPayer::Requester);
    initOutcome = PayerClient->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);

    auto content = TestUtils::GetRandomStream(100);
    UploadPartRequest upRequest(BucketName1, key, 1, initOutcome.result().UploadId(), content);
    auto uploadPartOutcome = PayerClient->UploadPart(upRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), false);
    EXPECT_EQ(uploadPartOutcome.error().Code(), "AccessDenied");
    upRequest.setRequestPayer(RequestPayer::Requester);
    uploadPartOutcome = PayerClient->UploadPart(upRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    ListPartsRequest listRequest(BucketName1, key, initOutcome.result().UploadId());
    auto lOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(lOutcome.isSuccess(), false);
    EXPECT_EQ(lOutcome.error().Code(), "AccessDenied");
    listRequest.setRequestPayer(RequestPayer::Requester);
    lOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(lOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest cRequest(BucketName1, key,
        lOutcome.result().PartList(), initOutcome.result().UploadId());
    auto outcome = PayerClient->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "AccessDenied");
    cRequest.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    HeadObjectRequest headRequest(BucketName1, key);
    auto hOutcome = PayerClient->HeadObject(headRequest);
    EXPECT_EQ(hOutcome.isSuccess(), false);
    EXPECT_EQ(hOutcome.error().Code(), "ServerError:403");
    headRequest.setRequestPayer(RequestPayer::Requester);
    hOutcome = PayerClient->HeadObject(headRequest);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().ContentLength(), 100);
    EXPECT_EQ(hOutcome.result().CacheControl(), "No-Cache");
    EXPECT_EQ(hOutcome.result().ContentType(), "user/test");
    EXPECT_EQ(hOutcome.result().ContentDisposition(), "test.zip");
    EXPECT_EQ(hOutcome.result().ContentEncoding(), "myzip");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "test");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("data"), "data");
}

TEST_F(ObjectRequestPaymentTest, MultipartUploadPartCopy)
{

    auto testKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest-TestKey");
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(TestFile, std::ios::in | std::ios::binary);
    PutObjectRequest putrequest(BucketName1, testKey, content);
    putrequest.setRequestPayer(RequestPayer::Requester);
    auto putoutcome = PayerClient->PutObject(putrequest);

    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest");

    InitiateMultipartUploadRequest request(BucketName1, targetObjectKey);
    request.setRequestPayer(RequestPayer::Requester);
    auto initOutcome = PayerClient->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    // Set the part size 
    const int partSize = 200*1024;
    const int64_t fileLength = GetFileLength(sourceFile);

    auto partCount = CalculatePartCount(fileLength, partSize);
    for (auto i = 0; i < partCount; i++)
    {
        // Skip to the start position
        int64_t skipBytes = partSize * i;
        int64_t position = skipBytes;

        // calculate the part size
        auto size = partSize < (fileLength - skipBytes) ? partSize : fileLength - skipBytes;

        UploadPartCopyRequest copyRequest(BucketName1, targetObjectKey, BucketName1, testKey);
        copyRequest.setPartNumber(i + 1);
        copyRequest.setUploadId(initOutcome.result().UploadId());
        copyRequest.setCopySourceRange(position, position + size - 1);
        auto uploadPartOutcome = PayerClient->UploadPartCopy(copyRequest);
        EXPECT_EQ(uploadPartOutcome.isSuccess(), false);
        EXPECT_EQ(uploadPartOutcome.error().Code(), "AccessDenied");
        copyRequest.setRequestPayer(RequestPayer::Requester);
        uploadPartOutcome = PayerClient->UploadPartCopy(copyRequest);
        EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
        EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
    }

    ListMultipartUploadsRequest listRequest1(BucketName1);
    auto lmuOutcome = PayerClient->ListMultipartUploads(ListMultipartUploadsRequest(listRequest1));
    EXPECT_EQ(lmuOutcome.isSuccess(), false);
    EXPECT_EQ(lmuOutcome.error().Code(), "AccessDenied");
    listRequest1.setRequestPayer(RequestPayer::Requester);
    lmuOutcome = PayerClient->ListMultipartUploads(ListMultipartUploadsRequest(listRequest1));
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

    ListPartsRequest listRequest(BucketName1, targetObjectKey);
    listRequest.setUploadId(uploadId);
    auto listOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), false);
    EXPECT_EQ(listOutcome.error().Code(), "AccessDenied");
    listRequest.setRequestPayer(RequestPayer::Requester);
    listOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest completeRequest(BucketName1, targetObjectKey, listOutcome.result().PartList());
    completeRequest.setUploadId(uploadId);
    completeRequest.setUploadId(uploadId);
    completeRequest.setRequestPayer(RequestPayer::Requester);
    auto cOutcome = PayerClient->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);

    GetObjectRequest getRequest(BucketName1, targetObjectKey);
    auto gOutcome = PayerClient->GetObject(getRequest);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "AccessDenied");
    getRequest.setRequestPayer(RequestPayer::Requester);
    gOutcome = PayerClient->GetObject(getRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);
}

TEST_F(ObjectRequestPaymentTest, MultipartUploadAbort)
{
    auto sourceFile = TestFile;
    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadAbortInMiddleTest");

    InitiateMultipartUploadRequest request(BucketName1, targetObjectKey);
    request.setRequestPayer(RequestPayer::Requester);
    auto initOutcome = PayerClient->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    // Set the part size 
    const int partSize = 100;;
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
            UploadPartRequest request(BucketName1, targetObjectKey, content);
            request.setRequestPayer(RequestPayer::Requester);
            request.setPartNumber(i + 1);
            request.setUploadId(initOutcome.result().UploadId());
            request.setContentLength(partSize);
            auto uploadPartOutcome = PayerClient->UploadPart(request);
            EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

        }
    }

    ListMultipartUploadsRequest listRequest1(BucketName1);
    listRequest1.setRequestPayer(RequestPayer::Requester);
    auto lmuOutcome = PayerClient->ListMultipartUploads(listRequest1);
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

    AbortMultipartUploadRequest abortRequest(BucketName1, targetObjectKey, uploadId);
    auto abortOutcome = PayerClient->AbortMultipartUpload(abortRequest);
    EXPECT_EQ(abortOutcome.isSuccess(), false);
    EXPECT_EQ(abortOutcome.error().Code(), "AccessDenied");
    abortRequest.setRequestPayer(RequestPayer::Requester);
    abortOutcome = PayerClient->AbortMultipartUpload(abortRequest);
    EXPECT_EQ(abortOutcome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, ObjectAcl)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectacl");

    std::string text = "hellowworld";
    PutObjectRequest putrequest(BucketName1, objName, std::make_shared<std::stringstream>(text));
    putrequest.setRequestPayer(RequestPayer::Requester);

    auto putOutcome = PayerClient->PutObject(putrequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GetObjectAclRequest getAclrequest(BucketName1, objName);
    auto aclOutcome = PayerClient->GetObjectAcl(getAclrequest);
    EXPECT_EQ(aclOutcome.isSuccess(), false);
    EXPECT_EQ(aclOutcome.error().Code(), "AccessDenied");
    getAclrequest.setRequestPayer(RequestPayer::Requester);
    aclOutcome = PayerClient->GetObjectAcl(getAclrequest);
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Default);

    SetObjectAclRequest setAclRequest(BucketName1, objName, CannedAccessControlList::PublicRead);
    auto setOutCome = PayerClient->SetObjectAcl(setAclRequest);
    EXPECT_EQ(setOutCome.isSuccess(), false);
    EXPECT_EQ(setOutCome.error().Code(), "AccessDenied");
    setAclRequest.setRequestPayer(RequestPayer::Requester);
    setOutCome = PayerClient->SetObjectAcl(setAclRequest);
    EXPECT_EQ(setOutCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, GetObjectMeta)
{
    auto meta = ObjectMetaData();
    // sets the content type.
    meta.setContentType("text/plain");
    // sets the cache control
    meta.setCacheControl("max-age=3");
    // sets the custom metadata.
    meta.UserMetaData()["meta"] = "meta-value";

    // uploads the file
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "Thank you for using Aliyun Object Storage Service!";
    PutObjectRequest putrequest(BucketName1, "GetObjectMeta", content);
    putrequest.setRequestPayer(RequestPayer::Requester);
    auto putOutcome = PayerClient->PutObject(putrequest);
    GetObjectMetaRequest request(BucketName1, "GetObjectMeta");
    request.setRequestPayer(RequestPayer::Requester);

    // get the object meta information
    auto outcome = PayerClient->GetObjectMeta(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    // delete the object
    DeleteObjectRequest delRequest(BucketName1, "GetObjectMeta");
    auto delOutcome = PayerClient->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), false);
    EXPECT_EQ(delOutcome.error().Code(), "AccessDenied");
    delRequest.setRequestPayer(RequestPayer::Requester);
    delOutcome = PayerClient->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);

}

TEST_F(ObjectRequestPaymentTest, DeleteObjects)
{
    const size_t TestKeysCnt = 10;
    auto keyPrefix = TestUtils::GetObjectKey("DeleteObjectsBasicTest");

    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        auto content = TestUtils::GetRandomStream(100);
        key.append(std::to_string(i)).append(".txt");
        PutObjectRequest putrequest(BucketName1, key, content);
        putrequest.setRequestPayer(RequestPayer::Requester);
        auto outcome = PayerClient->PutObject(putrequest);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    DeleteObjectsRequest delRequest(BucketName1);
    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    auto delOutcome = PayerClient->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), false);
    EXPECT_EQ(delOutcome.error().Code(), "AccessDenied");

    delRequest.setRequestPayer(RequestPayer::Requester);

    delOutcome = PayerClient->DeleteObjects(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
    EXPECT_EQ(delOutcome.result().keyList().size(), TestKeysCnt);
    EXPECT_EQ(delOutcome.result().Quiet(), false);
}

TEST_F(ObjectRequestPaymentTest, SetAndGetObjectSymlink)
{

    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectsymlink");
    // put object
    std::string text = "hellowworld";
    PutObjectRequest putrequest(BucketName1,objName,std::make_shared<std::stringstream>(text));
    putrequest.setRequestPayer(RequestPayer::Requester);
    auto putOutcome = PayerClient->PutObject(putrequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // create symlink success
    std::string linkName = objName + "-link";
    CreateSymlinkRequest setRequest(BucketName1, linkName);
    setRequest.SetSymlinkTarget(objName);
    CreateSymlinkOutcome  linkOutcom = PayerClient->CreateSymlink(setRequest);
    EXPECT_EQ(linkOutcom.isSuccess(), false);
    EXPECT_EQ(linkOutcom.error().Code(), "AccessDenied");

    setRequest.setRequestPayer(RequestPayer::Requester);
    linkOutcom = PayerClient->CreateSymlink(setRequest);
    EXPECT_EQ(linkOutcom.isSuccess(), true);

    GetSymlinkRequest getRequest(BucketName1, linkName);
    GetSymlinkOutcome getLinkOutcome = PayerClient->GetSymlink(getRequest);
    EXPECT_EQ(getLinkOutcome.isSuccess(), false);
    EXPECT_EQ(getLinkOutcome.error().Code(), "AccessDenied");

    getRequest.setRequestPayer(RequestPayer::Requester);
    getLinkOutcome = PayerClient->GetSymlink(getRequest);
    EXPECT_EQ(getLinkOutcome.isSuccess(), true);
    EXPECT_EQ(getLinkOutcome.result().SymlinkTarget(), objName);
}

TEST_F(ObjectRequestPaymentTest, SetAndGetObjectRestore)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectrestore");

    // first: put object
    std::string text = "hellowworld";
    ObjectMetaData meta;
    meta.addHeader("x-oss-storage-class", "Archive");
    PutObjectRequest putrequest(BucketName1, objName, std::make_shared<std::stringstream>(text), meta);
    putrequest.setRequestPayer(RequestPayer::Requester);
    auto putOutcome = PayerClient->PutObject(putrequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(2);

    //second:restore object
    RestoreObjectRequest request(BucketName1, objName);
    VoidOutcome restoreOutCome = PayerClient->RestoreObject(request);
    EXPECT_EQ(restoreOutCome.isSuccess(), false);
    EXPECT_EQ(restoreOutCome.error().Code(), "AccessDenied");

    request.setRequestPayer(RequestPayer::Requester);
    restoreOutCome = PayerClient->RestoreObject(request);
    EXPECT_EQ(restoreOutCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, SelectObject)
{
    std::string sqlMessage = std::string("name,school,company,age\r\n")
        .append("Lora Francis,School A,Staples Inc,27\r\n")
        .append("Eleanor Little,School B,\"Conectiv, Inc\",43\r\n")
        .append("Rosie Hughes,School C,Western Gas Resources Inc,44\r\n")
        .append("Lawrence Ross,School D,MetLife Inc.,24");
    std::string key = TestUtils::GetObjectKey("SqlObjectWithCsvData");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName1, key, content);
    putRequest.setRequestPayer(RequestPayer::Requester);
    // put object
    auto putOutcome = PayerClient->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName1, key);
    selectRequest.setExpression("select * from ossobject");

    CSVInputFormat csvInputFormat;
    csvInputFormat.setHeaderInfo(CSVHeader::Use);
    csvInputFormat.setRecordDelimiter("\r\n");
    csvInputFormat.setFieldDelimiter(",");
    csvInputFormat.setQuoteChar("\"");
    csvInputFormat.setCommentChar("#");
    selectRequest.setInputFormat(csvInputFormat);

    CSVOutputFormat csvOutputFormat;
    selectRequest.setOutputFormat(csvOutputFormat);
    selectRequest.setRequestPayer(RequestPayer::Requester);

    auto outcome = PayerClient->SelectObject(selectRequest);

    // createSelectObjectMeta
    CreateSelectObjectMetaRequest metaRequest(BucketName1, key);
    metaRequest.setInputFormat(csvInputFormat);
    metaRequest.setRequestPayer(RequestPayer::Requester);
    auto metaOutcome = PayerClient->CreateSelectObjectMeta(metaRequest);
}

class ListObjectsAsyncContext : public AsyncCallerContext
{
public:
    ListObjectsAsyncContext() :ready(false) {}
    virtual ~ListObjectsAsyncContext()
    {
    }

    const ObjectSummaryList& ObjectSummarys() const { return objectSummarys_; }
    void setObjectSummaryList(const AlibabaCloud::OSS::ObjectSummaryList& objectSummarys) const
    {
        objectSummarys_ = objectSummarys;
    }

    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    mutable bool ready;
    mutable AlibabaCloud::OSS::ObjectSummaryList objectSummarys_;
};

static void ListObjectsHandler(const AlibabaCloud::OSS::OssClient* client,
    const ListObjectsRequest& request,
    const ListObjectOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    UNUSED_PARAM(request);
    std::cout << "PayerClient[" << client << "]" << "ListObjectsHandler" << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const ListObjectsAsyncContext*>(context.get());
        if (!outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " failed, Code:" << outcome.error().Code()
                << ", message:" << outcome.error().Message() << std::endl;
        }
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().ObjectSummarys().size(), 20U);
        EXPECT_EQ(outcome.result().IsTruncated(), false);

        ctx->setObjectSummaryList(outcome.result().ObjectSummarys());
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}

TEST_F(ObjectRequestPaymentTest, ListObjectsAsync)
{
    // create test file
    for (int i = 0; i < 20; i++) {
        std::string key = TestUtils::GetObjectKey("ListAllObjectsAsync");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(BucketName1, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    // list objects async
    ListObjectsRequest request(BucketName1);
    request.setRequestPayer(RequestPayer::Requester);
    ListObjectAsyncHandler handler = ListObjectsHandler;
    std::shared_ptr<ListObjectsAsyncContext> content = std::make_shared<ListObjectsAsyncContext>();
    PayerClient->ListObjectsAsync(request, handler, content);
    std::cout << "PayerClient[" << PayerClient << "]" << "Issue ListObjectsAsync done." << std::endl;

    {
        std::unique_lock<std::mutex> lck(content->mtx);
        if (!content->ready) content->cv.wait(lck);
    }
    int i = 0;
    for (auto const& obj : content->ObjectSummarys()) {
        EXPECT_EQ(obj.Size(), 100LL);
        i++;
    }
    EXPECT_EQ(i, 20);
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
    std::cout << "PayerClient[" << client << "]" << "GetObjectHandler" << ", key:" << request.Key() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const GetObjectAsyncContex*>(context.get());
        EXPECT_EQ(outcome.isSuccess(), true);
        std::string memMd5 = ComputeContentMD5(*outcome.result().Content().get());
        ctx->md5 = memMd5;
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}
TEST_F(ObjectRequestPaymentTest, GetObjectAsync)
{

    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectAsyncBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(102400);

    auto pOutcome = Client->PutObject(BucketName1, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectAsyncHandler handler = GetObjectHandler;
    GetObjectRequest request(BucketName1, key);
    request.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<GetObjectAsyncContex> memContext = std::make_shared<GetObjectAsyncContex>();

    GetObjectRequest fileRequest(BucketName1, key);
    fileRequest.setRequestPayer(RequestPayer::Requester);
    fileRequest.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios::binary); });
    std::shared_ptr<GetObjectAsyncContex> fileContext = std::make_shared<GetObjectAsyncContex>();

    PayerClient->GetObjectAsync(request, handler, memContext);
    PayerClient->GetObjectAsync(fileRequest, handler, fileContext);
    std::cout << "PayerClient[" << PayerClient << "]" << "Issue GetObjectAsync done." << std::endl;

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
    std::cout << "PayerClient[" << client << "]" << "PutObjectHandler, tag:" << context->Uuid() <<
        ", key:" << request.Key() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const PutObjectAsyncContex*>(context.get());
        EXPECT_EQ(outcome.isSuccess(), true);
        std::unique_lock<std::mutex> lck(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_all();
    }
}

TEST_F(ObjectRequestPaymentTest, PutObjectAsync)
{

    std::string memKey = TestUtils::GetObjectKey("PutObjectAsyncBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);

    std::string fileKey = TestUtils::GetObjectKey("PutObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectAsyncBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);

    PutObjectAsyncHandler handler = PutObjectHandler;
    PutObjectRequest memRequest(BucketName1, memKey, memContent);
    memRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<PutObjectAsyncContex> memContext = std::make_shared<PutObjectAsyncContex>();
    memContext->setUuid("PutobjectasyncFromMem");

    PutObjectRequest fileRequest(BucketName1, fileKey, fileContent);
    fileRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<PutObjectAsyncContex> fileContext = std::make_shared<PutObjectAsyncContex>();
    fileContext->setUuid("PutobjectasyncFromFile");

    PayerClient->PutObjectAsync(memRequest, handler, memContext);
    PayerClient->PutObjectAsync(fileRequest, handler, fileContext);
    std::cout << "PayerClient[" << PayerClient << "]" << "Issue PutObjectAsync done." << std::endl;

    {
        std::unique_lock<std::mutex> lck(fileContext->mtx);
        if (!fileContext->ready) fileContext->cv.wait(lck);
    }

    {
        std::unique_lock<std::mutex> lck(memContext->mtx);
        if (!memContext->ready) memContext->cv.wait(lck);
    }

    fileContent->close();

    fileContext = nullptr;

    TestUtils::WaitForCacheExpire(1);
    EXPECT_EQ(RemoveFile(tmpFile), true);
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

static void UploadPartHandler(const AlibabaCloud::OSS::OssClient* client,
    const UploadPartRequest& request,
    const PutObjectOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    UNUSED_PARAM(request);
    std::cout << "PayerClient[" << client << "]" << "UploadPartHandler,tag:" << context->Uuid() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const UploadPartAsyncContext*>(context.get());
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

TEST_F(ObjectRequestPaymentTest, UploadPartAsync)
{

    std::string memKey = TestUtils::GetObjectKey("UploadPartMemObjectAsyncBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);
    InitiateMultipartUploadRequest memInitRequest(BucketName1, memKey);
    auto memInitOutcome = Client->InitiateMultipartUpload(memInitRequest);
    EXPECT_EQ(memInitOutcome.isSuccess(), true);
    EXPECT_EQ(memInitOutcome.result().Key(), memKey);

    std::string fileKey = TestUtils::GetObjectKey("UploadPartFileObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("UploadPartFileObjectAsyncBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);
    InitiateMultipartUploadRequest fileInitRequest(BucketName1, fileKey);
    auto fileInitOutcome = Client->InitiateMultipartUpload(fileInitRequest);
    EXPECT_EQ(fileInitOutcome.isSuccess(), true);
    EXPECT_EQ(fileInitOutcome.result().Key(), fileKey);

    UploadPartAsyncHandler handler = UploadPartHandler;

    UploadPartRequest memRequest(BucketName1, memKey, 1, memInitOutcome.result().UploadId(), memContent);
    memRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<UploadPartAsyncContext> memContext = std::make_shared<UploadPartAsyncContext>();
    memContext->setUuid("UploadPartAsyncFromMem");

    UploadPartRequest fileRequest(BucketName1, fileKey, 1, fileInitOutcome.result().UploadId(), fileContent);
    fileRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<UploadPartAsyncContext> fileContext = std::make_shared<UploadPartAsyncContext>();
    fileContext->setUuid("UploadPartAsyncFromFile");

    PayerClient->UploadPartAsync(memRequest, handler, memContext);
    PayerClient->UploadPartAsync(fileRequest, handler, fileContext);
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

    auto memListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName1, memKey, memInitOutcome.result().UploadId()));
    EXPECT_EQ(memListPartsOutcome.isSuccess(), true);
    auto fileListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName1, fileKey, fileInitOutcome.result().UploadId()));
    EXPECT_EQ(fileListPartsOutcome.isSuccess(), true);

    auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName1, memKey,
        memListPartsOutcome.result().PartList(), memInitOutcome.result().UploadId()));
    EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
    auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName1, fileKey,
        fileListPartsOutcome.result().PartList(), fileInitOutcome.result().UploadId()));
    EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

    fileContext = nullptr;
    TestUtils::WaitForCacheExpire(1);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

static void UploadPartCopyHandler(const AlibabaCloud::OSS::OssClient* client,
    const UploadPartCopyRequest& request,
    const UploadPartCopyOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
    UNUSED_PARAM(request);
    std::cout << "PayerClient[" << client << "]" << "UploadPartCopyHandler, tag:" << context->Uuid() << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const UploadPartAsyncContext*>(context.get());
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

TEST_F(ObjectRequestPaymentTest, UploadPartCopyAsync)
{
    // put object from buffer
    std::string memObjKey = TestUtils::GetObjectKey("PutObjectFromBuffer");
    auto memObjContent = TestUtils::GetRandomStream(102400);
    auto putMemObjOutcome = Client->PutObject(PutObjectRequest(BucketName1, memObjKey, memObjContent));
    EXPECT_EQ(putMemObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, memObjKey), true);

    // put object from local file
    std::string fileObjKey = TestUtils::GetObjectKey("PutObjectFromFile");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFile").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileObjContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);
    auto putFileObjOutcome = Client->PutObject(PutObjectRequest(BucketName1, fileObjKey, fileObjContent));
    EXPECT_EQ(putFileObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, fileObjKey), true);

    // close the file
    fileObjContent->close();

    // apply the upload id
    std::string memKey = TestUtils::GetObjectKey("UploadPartCopyMemObjectAsyncBasicTest");
    auto memInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName1, memKey));
    EXPECT_EQ(memInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(memInitObjOutcome.result().Key(), memKey);

    std::string fileKey = TestUtils::GetObjectKey("UploadPartCopyFileObjectAsyncBasicTest");
    auto fileInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName1, fileKey));
    EXPECT_EQ(fileInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(fileInitObjOutcome.result().Key(), fileKey);

    // construct parameter
    UploadPartCopyAsyncHandler handler = UploadPartCopyHandler;
    UploadPartCopyRequest memRequest(BucketName1, memKey, BucketName1, memObjKey, memInitObjOutcome.result().UploadId(), 1);
    memRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<UploadPartAsyncContext> memContext = std::make_shared<UploadPartAsyncContext>();
    memContext->setUuid("UploadPartCopyAsyncFromMem");
    UploadPartCopyRequest fileRequest(BucketName1, fileKey, BucketName1, fileObjKey, fileInitObjOutcome.result().UploadId(), 1);
    fileRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<UploadPartAsyncContext> fileContext = std::make_shared<UploadPartAsyncContext>();
    fileContext->setUuid("UploadPartCopyAsyncFromFile");

    PayerClient->UploadPartCopyAsync(memRequest, handler, memContext);
    PayerClient->UploadPartCopyAsync(fileRequest, handler, fileContext);
    std::cout << "PayerClient[" << Client << "]" << "Issue UploadPartCopyAsync done." << std::endl;
    {
        std::unique_lock<std::mutex> lck(fileContext->mtx);
        if (!fileContext->ready) fileContext->cv.wait(lck);
    }
    {
        std::unique_lock<std::mutex> lck(memContext->mtx);
        if (!memContext->ready) memContext->cv.wait(lck);
    }

    // list parts
    auto memListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName1, memKey, memInitObjOutcome.result().UploadId()));
    EXPECT_EQ(memListPartsOutcome.isSuccess(), true);
    auto fileListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName1, fileKey, fileInitObjOutcome.result().UploadId()));
    EXPECT_EQ(fileListPartsOutcome.isSuccess(), true);

    // complete
    CompleteMultipartUploadRequest memCompleteRequest(BucketName1, memKey, memListPartsOutcome.result().PartList());
    memCompleteRequest.setUploadId(memInitObjOutcome.result().UploadId());
    auto memCompleteOutcome = Client->CompleteMultipartUpload(memCompleteRequest);
    EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
    CompleteMultipartUploadRequest fileCompleteRequest(BucketName1, fileKey, fileListPartsOutcome.result().PartList());
    fileCompleteRequest.setUploadId(fileInitObjOutcome.result().UploadId());
    auto fileCompleteOutcome = Client->CompleteMultipartUpload(fileCompleteRequest);
    EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

    fileObjContent = nullptr;
    TestUtils::WaitForCacheExpire(1);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, ListObjectsCallable)
{
    //create test file
    for (int i = 0; i < 20; i++) {
        std::string key = TestUtils::GetObjectKey("ListAllObjectsCallableTest");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(BucketName1, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    //list object use default
    ListObjectsRequest request(BucketName1);
    request.setPrefix("ListAllObjectsCallableTest");
    request.setRequestPayer(RequestPayer::Requester);
    auto listOutcomeCallable = PayerClient->ListObjectsCallable(request);
    auto listOutcome = listOutcomeCallable.get();
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().ObjectSummarys().size(), 20UL);
    EXPECT_EQ(listOutcome.result().IsTruncated(), false);

    int i = 0;
    for (auto const& obj : listOutcome.result().ObjectSummarys()) {
        EXPECT_EQ(obj.Size(), 100LL);
        i++;
    }
    EXPECT_EQ(i, 20);
}

TEST_F(ObjectRequestPaymentTest, GetObjectCallable)
{

    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectCallableBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectCallableBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(102400);

    auto pOutcome = Client->PutObject(BucketName1, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest request(BucketName1, key);
    request.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<GetObjectAsyncContex> memContext = std::make_shared<GetObjectAsyncContex>();

    GetObjectRequest fileRequest(BucketName1, key);
    fileRequest.setRequestPayer(RequestPayer::Requester);
    fileRequest.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios::binary); });

    auto memOutcomeCallable = PayerClient->GetObjectCallable(request);
    auto fileOutcomeCallable = PayerClient->GetObjectCallable(fileRequest);


    std::cout << "PayerClient[" << PayerClient << "]" << "Issue GetObjectCallable done." << std::endl;

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

TEST_F(ObjectRequestPaymentTest, PutObjectCallable)
{
    std::string memKey = TestUtils::GetObjectKey("PutObjectCallableBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);

    std::string fileKey = TestUtils::GetObjectKey("PutObjectCallableBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectCallableBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);

    PutObjectRequest putrequest1(BucketName1, memKey, memContent);
    putrequest1.setRequestPayer(RequestPayer::Requester);
    PutObjectRequest putrequest2(BucketName1, fileKey, fileContent);
    putrequest2.setRequestPayer(RequestPayer::Requester);
    auto memOutcomeCallable = PayerClient->PutObjectCallable(putrequest1);
    auto fileOutcomeCallable = PayerClient->PutObjectCallable(putrequest2);

    std::cout << "PayerClient[" << PayerClient << "]" << "Issue PutObjectCallable done." << std::endl;

    auto fileOutcome = fileOutcomeCallable.get();
    auto memOutcome = memOutcomeCallable.get();
    EXPECT_EQ(fileOutcome.isSuccess(), true);
    EXPECT_EQ(memOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, memKey), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, fileKey), true);

    memContent = nullptr;
    fileContent = nullptr;
    //EXPECT_EQ(RemoveFile(tmpFile), true);
    RemoveFile(tmpFile);
}

TEST_F(ObjectRequestPaymentTest, UploadPartCallable)
{
    auto memKey = TestUtils::GetObjectKey("MultipartUploadCallable-MemObject");
    auto memContent = TestUtils::GetRandomStream(102400);
    auto memInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName1, memKey));
    EXPECT_EQ(memInitOutcome.isSuccess(), true);

    auto fileKey = TestUtils::GetObjectKey("MultipartUploadCallable-FileObject");
    auto tmpFile = TestUtils::GetObjectKey("MultipartUploadCallable-FileObject").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    {
        auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);
        auto fileInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName1, fileKey));
        EXPECT_EQ(fileInitOutcome.isSuccess(), true);

        UploadPartRequest uprequest1(BucketName1, memKey, 1, memInitOutcome.result().UploadId(), memContent);
        UploadPartRequest uprequest2(BucketName1, fileKey, 1, fileInitOutcome.result().UploadId(), fileContent);
        uprequest1.setRequestPayer(RequestPayer::Requester);
        uprequest2.setRequestPayer(RequestPayer::Requester);

        auto memOutcomeCallable = PayerClient->UploadPartCallable(uprequest1);
        auto fileOutcomeCallable = PayerClient->UploadPartCallable(uprequest2);

        std::cout << "PayerClient[" << PayerClient << "]" << "Issue MultipartUploadCallable done." << std::endl;

        auto fileOutcome = fileOutcomeCallable.get();
        auto menOutcome = memOutcomeCallable.get();
        EXPECT_EQ(fileOutcome.isSuccess(), true);
        EXPECT_EQ(menOutcome.isSuccess(), true);

        // list part
        auto memListPartOutcome = Client->ListParts(ListPartsRequest(BucketName1, memKey, memInitOutcome.result().UploadId()));
        auto fileListPartOutcome = Client->ListParts(ListPartsRequest(BucketName1, fileKey, fileInitOutcome.result().UploadId()));
        EXPECT_EQ(memListPartOutcome.isSuccess(), true);
        EXPECT_EQ(fileListPartOutcome.isSuccess(), true);

        auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName1, memKey,
            memListPartOutcome.result().PartList(), memInitOutcome.result().UploadId()));
        auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName1, fileKey,
            fileListPartOutcome.result().PartList(), fileInitOutcome.result().UploadId()));
        EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
        EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

        EXPECT_EQ(Client->DoesObjectExist(BucketName1, memKey), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName1, fileKey), true);

        memContent = nullptr;
        fileContent = nullptr;
    }
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, UploadPartCopyCallable)
{
    // put object from buffer
    std::string memObjKey = TestUtils::GetObjectKey("PutObjectFromBuffer");
    auto memObjContent = TestUtils::GetRandomStream(102400);
    auto putMemObjOutcome = Client->PutObject(PutObjectRequest(BucketName1, memObjKey, memObjContent));
    EXPECT_EQ(putMemObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, memObjKey), true);

    // put object from local file
    std::string fileObjKey = TestUtils::GetObjectKey("PutObjectFromFile");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFile").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileObjContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);
    auto putFileObjOutcome = Client->PutObject(PutObjectRequest(BucketName1, fileObjKey, fileObjContent));
    EXPECT_EQ(putFileObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, fileObjKey), true);

    // close file
    fileObjContent->close();

    // apply upload id
    std::string memKey = TestUtils::GetObjectKey("UploadPartCopyCallableMemObjectBasicTest");
    auto memInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName1, memKey));
    EXPECT_EQ(memInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(memInitObjOutcome.result().Key(), memKey);

    std::string fileKey = TestUtils::GetObjectKey("UploadPartCopyCallableFileObjectBasicTest");
    auto fileInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName1, fileKey));
    EXPECT_EQ(fileInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(fileInitObjOutcome.result().Key(), fileKey);

    // upload part copy
    UploadPartCopyRequest request1(BucketName1, memKey, BucketName1, memObjKey, memInitObjOutcome.result().UploadId(), 1);
    UploadPartCopyRequest request2(BucketName1, fileKey, BucketName1, fileObjKey, fileInitObjOutcome.result().UploadId(), 1);
    request1.setRequestPayer(RequestPayer::Requester);
    request2.setRequestPayer(RequestPayer::Requester);
    auto memOutcomeCallable = PayerClient->UploadPartCopyCallable(request1);
    auto fileOutcomeCallable = PayerClient->UploadPartCopyCallable(request2);

    std::cout << "PayerClient[" << PayerClient << "]" << "Issue UploadPartCopyCallable done." << std::endl;

    auto fileOutcome = fileOutcomeCallable.get();
    auto memOutcome = memOutcomeCallable.get();
    EXPECT_EQ(fileOutcome.isSuccess(), true);
    EXPECT_EQ(memOutcome.isSuccess(), true);

    // list part
    auto memListPartOutcome = Client->ListParts(ListPartsRequest(BucketName1, memKey, memInitObjOutcome.result().UploadId()));
    auto fileListPartOutcome = Client->ListParts(ListPartsRequest(BucketName1, fileKey, fileInitObjOutcome.result().UploadId()));
    EXPECT_EQ(memListPartOutcome.isSuccess(), true);
    EXPECT_EQ(fileListPartOutcome.isSuccess(), true);

    // complete the part
    auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName1, memKey,
        memListPartOutcome.result().PartList(), memInitObjOutcome.result().UploadId()));
    auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName1, fileKey,
        fileListPartOutcome.result().PartList(), fileInitObjOutcome.result().UploadId()));
    EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
    EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

    EXPECT_EQ(Client->DoesObjectExist(BucketName1, memKey), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, fileKey), true);

    memObjContent = nullptr;
    fileObjContent = nullptr;
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

}
}
