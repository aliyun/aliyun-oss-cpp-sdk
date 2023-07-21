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
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        PayerClient = std::make_shared<OssClient>(Config::Endpoint, Config::PayerAccessKeyId, Config::PayerAccessKeySecret, ClientConfiguration());

        BucketName = TestUtils::GetBucketName("cpp-sdk-objectcopy1");
        CreateBucketOutcome outCome = Client->CreateBucket(CreateBucketRequest(BucketName));
        EXPECT_EQ(outCome.isSuccess(), true);

        SetBucketPolicyRequest request(BucketName);
        std::string policy = std::string("{\"Version\":\"1\",\"Statement\":[{\"Action\":[\"oss:*\"],\"Effect\": \"Allow\",")
            .append("\"Principal\":[\"").append(Config::PayerUID).append("\"],")
            .append("\"Resource\": [\"acs:oss:*:*:").append(BucketName).append("\",\"acs:oss:*:*:").append(BucketName).append("/*\"]}]}");
        request.setPolicy(policy);
        auto policyOutcome1 = Client->SetBucketPolicy(request);
        EXPECT_EQ(policyOutcome1.isSuccess(), true);

        SetBucketRequestPaymentRequest setRequest(BucketName);
        setRequest.setRequestPayer(RequestPayer::Requester);
        VoidOutcome setOutcome = Client->SetBucketRequestPayment(setRequest);
        EXPECT_EQ(setOutcome.isSuccess(), true);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName);
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
    
    static int64_t GetFileLength(const std::string file)
    {
        std::fstream f(file, std::ios::in | std::ios::binary);
        f.seekg(0, f.end);
        int64_t size = f.tellg();
        f.close();
        return size;
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
    static std::shared_ptr<OssClient> PayerClient;
    static std::string BucketName;
};

std::shared_ptr<OssClient> ObjectRequestPaymentTest::Client = nullptr;
std::shared_ptr<OssClient> ObjectRequestPaymentTest::PayerClient = nullptr;
std::string ObjectRequestPaymentTest::BucketName = "";

TEST_F(ObjectRequestPaymentTest, PutObjectTest)
{
    std::string key = TestUtils::GetObjectKey("put-object");
    
    PutObjectRequest putRequest(BucketName, key, std::make_shared<std::stringstream>("hello world"));
    auto putOutcome = PayerClient->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), false);
    EXPECT_EQ(putOutcome.error().Code(), "AccessDenied");

    putRequest.setRequestPayer(RequestPayer::BucketOwner);
    putOutcome = PayerClient->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), false);
    EXPECT_EQ(putOutcome.error().Code(), "AccessDenied");

    putRequest.setRequestPayer(RequestPayer::Requester);
    putOutcome = PayerClient->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, GetObjectTest)
{
    std::string key = TestUtils::GetObjectKey("put-object");

    PutObjectRequest putRequest(BucketName, key, std::make_shared<std::stringstream>("hello world"));
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GetObjectRequest getRequest(BucketName, key);
    auto getOutcome = PayerClient->GetObject(getRequest);
    EXPECT_EQ(getOutcome.isSuccess(), false);
    EXPECT_EQ(getOutcome.error().Code(), "AccessDenied");

    getRequest.setRequestPayer(RequestPayer::BucketOwner);
    getOutcome = PayerClient->GetObject(getRequest);
    EXPECT_EQ(getOutcome.isSuccess(), false);
    EXPECT_EQ(getOutcome.error().Code(), "AccessDenied");

    getRequest.setRequestPayer(RequestPayer::Requester);
    getOutcome = PayerClient->GetObject(getRequest);
    EXPECT_EQ(getOutcome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, DeleteObjectTest)
{
    std::string key = TestUtils::GetObjectKey("delete-object");

    PutObjectRequest putRequest(BucketName, key, std::make_shared<std::stringstream>("hello world"));
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    DeleteObjectRequest delRequest(BucketName, key);
    auto delOutcome = PayerClient->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), false);
    EXPECT_EQ(delOutcome.error().Code(), "AccessDenied");

    delRequest.setRequestPayer(RequestPayer::Requester);
    delOutcome = PayerClient->DeleteObject(delRequest);
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, GetObjectMetaTest)
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
    PutObjectRequest putrequest(BucketName, "GetObjectMeta", content);
    auto putOutcome = Client->PutObject(putrequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GetObjectMetaRequest request(BucketName, "GetObjectMeta");
    auto outcome = PayerClient->GetObjectMeta(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:403");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->GetObjectMeta(request);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, HeadObjectTest)
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
    PutObjectRequest putrequest(BucketName, "HeadObject", content);
    auto putOutcome = Client->PutObject(putrequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    HeadObjectRequest request(BucketName, "HeadObject");
    auto outcome = PayerClient->HeadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:403");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->HeadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, SetAndGetObjectAclTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectacl");

    PutObjectRequest putRequest(BucketName, objName, std::make_shared<std::stringstream>("hello world"));
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GetObjectAclRequest getAclrequest(BucketName, objName);
    auto aclOutcome = PayerClient->GetObjectAcl(getAclrequest);
    EXPECT_EQ(aclOutcome.isSuccess(), false);
    EXPECT_EQ(aclOutcome.error().Code(), "AccessDenied");

    getAclrequest.setRequestPayer(RequestPayer::Requester);
    aclOutcome = PayerClient->GetObjectAcl(getAclrequest);
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Default);

    SetObjectAclRequest setAclRequest(BucketName, objName, CannedAccessControlList::PublicRead);
    auto setOutCome = PayerClient->SetObjectAcl(setAclRequest);
    EXPECT_EQ(setOutCome.isSuccess(), false);
    EXPECT_EQ(setOutCome.error().Code(), "AccessDenied");

    setAclRequest.setRequestPayer(RequestPayer::Requester);
    setOutCome = PayerClient->SetObjectAcl(setAclRequest);
    EXPECT_EQ(setOutCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, CopyObjectTest)
{
    std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
    PutObjectRequest putRequest(BucketName, objName1, std::make_shared<std::stringstream>("hello world"));
    PutObjectOutcome putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
    CopyObjectRequest copyRequest(BucketName, objName2);
    copyRequest.setCopySource(BucketName, objName1);
    CopyObjectOutcome copyOutCome = PayerClient->CopyObject(copyRequest);
    EXPECT_EQ(copyOutCome.isSuccess(), false);
    EXPECT_EQ(copyOutCome.error().Code(), "AccessDenied");

    copyRequest.setRequestPayer(RequestPayer::Requester);
    copyOutCome = PayerClient->CopyObject(copyRequest);
    EXPECT_EQ(copyOutCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, AppendObjectTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectappend");

    // append object
    std::string text = "helloworld";
    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text));
    auto appendOutcome = PayerClient->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), false);
    EXPECT_EQ(appendOutcome.error().Code(), "AccessDenied");

    appendRequest.setRequestPayer(RequestPayer::Requester);
    appendOutcome = PayerClient->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size());

    // append object again
    AppendObjectRequest  requestOther(BucketName, objName, std::make_shared<std::stringstream>(text));
    requestOther.setPosition(text.size());
    appendOutcome = PayerClient->AppendObject(requestOther);
    EXPECT_EQ(appendOutcome.isSuccess(), false);
    EXPECT_EQ(appendOutcome.error().Code(), "AccessDenied");

    requestOther.setRequestPayer(RequestPayer::Requester);
    appendOutcome = PayerClient->AppendObject(requestOther);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size()*2);

    // read object
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName, objName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData;
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, text + text);
}

TEST_F(ObjectRequestPaymentTest, DeleteObjectsTest)
{
    const size_t TestKeysCnt = 10;
    auto keyPrefix = TestUtils::GetObjectKey("DeleteObjectsBasicTest");

    for (size_t i = 0; i < TestKeysCnt; i++) {
        auto key = keyPrefix;
        auto content = TestUtils::GetRandomStream(100);
        key.append(std::to_string(i)).append(".txt");
        PutObjectRequest putrequest(BucketName, key, content);
        auto outcome = Client->PutObject(putrequest);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    DeleteObjectsRequest delRequest(BucketName);
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

TEST_F(ObjectRequestPaymentTest, ListObjectsTest)
{
    ListObjectsRequest request(BucketName);
    ListObjectOutcome outCome = PayerClient->ListObjects(request);
    EXPECT_EQ(outCome.isSuccess(), false);
    EXPECT_EQ(outCome.error().Code(), "AccessDenied");

    request.setRequestPayer(RequestPayer::Requester);
    outCome = PayerClient->ListObjects(request);
    EXPECT_EQ(outCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, SetAndGetObjectSymlinkTest)
{

    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectsymlink");
    // put object
    PutObjectRequest putrequest(BucketName, objName, std::make_shared<std::stringstream>("hello world"));
    putrequest.setRequestPayer(RequestPayer::Requester);
    auto putOutcome = Client->PutObject(putrequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // create symlink success
    std::string linkName = objName + "-link";
    CreateSymlinkRequest setRequest(BucketName, linkName);
    setRequest.SetSymlinkTarget(objName);
    auto  linkOutcom = PayerClient->CreateSymlink(setRequest);
    EXPECT_EQ(linkOutcom.isSuccess(), false);
    EXPECT_EQ(linkOutcom.error().Code(), "AccessDenied");

    setRequest.setRequestPayer(RequestPayer::Requester);
    linkOutcom = PayerClient->CreateSymlink(setRequest);
    EXPECT_EQ(linkOutcom.isSuccess(), true);

    GetSymlinkRequest getRequest(BucketName, linkName);
    auto getLinkOutcome = PayerClient->GetSymlink(getRequest);
    EXPECT_EQ(getLinkOutcome.isSuccess(), false);
    EXPECT_EQ(getLinkOutcome.error().Code(), "AccessDenied");

    getRequest.setRequestPayer(RequestPayer::Requester);
    getLinkOutcome = PayerClient->GetSymlink(getRequest);
    EXPECT_EQ(getLinkOutcome.isSuccess(), true);
    EXPECT_EQ(getLinkOutcome.result().SymlinkTarget(), objName);
}

TEST_F(ObjectRequestPaymentTest, RestoreObjectTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectrestore");

    // first: put object
    ObjectMetaData meta;
    meta.addHeader("x-oss-storage-class", "Archive");
    PutObjectRequest putrequest(BucketName, objName, std::make_shared<std::stringstream>("hello world"), meta);
    auto putOutcome = Client->PutObject(putrequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(2);

    //second:restore object
    RestoreObjectRequest request(BucketName, objName);
    auto restoreOutCome = PayerClient->RestoreObject(request);
    EXPECT_EQ(restoreOutCome.isSuccess(), false);
    EXPECT_EQ(restoreOutCome.error().Code(), "AccessDenied");

    request.setRequestPayer(RequestPayer::Requester);
    restoreOutCome = PayerClient->RestoreObject(request);
    EXPECT_EQ(restoreOutCome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, MultipartUploadTest)
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
    auto initOutcome = PayerClient->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), false);
    EXPECT_EQ(initOutcome.error().Code(), "AccessDenied");
    request.setRequestPayer(RequestPayer::Requester);
    initOutcome = PayerClient->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().Key(), key);

    auto content = TestUtils::GetRandomStream(100);
    UploadPartRequest upRequest(BucketName, key, 1, initOutcome.result().UploadId(), content);
    auto uploadPartOutcome = PayerClient->UploadPart(upRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), false);
    EXPECT_EQ(uploadPartOutcome.error().Code(), "AccessDenied");
    upRequest.setRequestPayer(RequestPayer::Requester);
    uploadPartOutcome = PayerClient->UploadPart(upRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);

    ListPartsRequest listRequest(BucketName, key, initOutcome.result().UploadId());
    auto lOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(lOutcome.isSuccess(), false);
    EXPECT_EQ(lOutcome.error().Code(), "AccessDenied");
    listRequest.setRequestPayer(RequestPayer::Requester);
    lOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(lOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest cRequest(BucketName, key,
        lOutcome.result().PartList(), initOutcome.result().UploadId());
    auto outcome = PayerClient->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "AccessDenied");
    cRequest.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->CompleteMultipartUpload(cRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    HeadObjectRequest headRequest(BucketName, key);
    auto hOutcome = Client->HeadObject(headRequest);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().ContentLength(), 100);
    EXPECT_EQ(hOutcome.result().CacheControl(), "No-Cache");
    EXPECT_EQ(hOutcome.result().ContentType(), "user/test");
    EXPECT_EQ(hOutcome.result().ContentDisposition(), "test.zip");
    EXPECT_EQ(hOutcome.result().ContentEncoding(), "myzip");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "test");
    EXPECT_EQ(hOutcome.result().UserMetaData().at("data"), "data");
}

TEST_F(ObjectRequestPaymentTest, MultipartUploadPartCopyTest)
{
    auto sourceFile = TestUtils::GetTargetFileName("cpp-sdk-multipartupload");
    TestUtils::WriteRandomDatatoFile(sourceFile, 500 * 1024);

    auto testKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest-TestKey");
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(sourceFile, std::ios::in | std::ios::binary);
    PutObjectRequest putrequest(BucketName, testKey, content);
    auto putoutcome = Client->PutObject(putrequest);

    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadPartCopyComplexStepTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
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

        UploadPartCopyRequest copyRequest(BucketName, targetObjectKey, BucketName, testKey);
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

    ListMultipartUploadsRequest listRequest1(BucketName);
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

    ListPartsRequest listRequest(BucketName, targetObjectKey);
    listRequest.setUploadId(uploadId);
    auto listOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), false);
    EXPECT_EQ(listOutcome.error().Code(), "AccessDenied");
    listRequest.setRequestPayer(RequestPayer::Requester);
    listOutcome = PayerClient->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest completeRequest(BucketName, targetObjectKey, listOutcome.result().PartList());
    completeRequest.setUploadId(uploadId);
    completeRequest.setUploadId(uploadId);
    completeRequest.setRequestPayer(RequestPayer::Requester);
    auto cOutcome = PayerClient->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);

    GetObjectRequest getRequest(BucketName, targetObjectKey);
    auto gOutcome = Client->GetObject(getRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().Metadata().ContentLength(), fileLength);

    RemoveFile(sourceFile);
}

TEST_F(ObjectRequestPaymentTest, MultipartUploadAbortTest)
{
    auto targetObjectKey = TestUtils::GetObjectKey("MultipartUploadAbortInMiddleTest");

    InitiateMultipartUploadRequest request(BucketName, targetObjectKey);
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);

    AbortMultipartUploadRequest abortRequest(BucketName, targetObjectKey, initOutcome.result().UploadId());
    auto abortOutcome = PayerClient->AbortMultipartUpload(abortRequest);
    EXPECT_EQ(abortOutcome.isSuccess(), false);
    EXPECT_EQ(abortOutcome.error().Code(), "AccessDenied");
    
    abortRequest.setRequestPayer(RequestPayer::Requester);
    abortOutcome = PayerClient->AbortMultipartUpload(abortRequest);
    EXPECT_EQ(abortOutcome.isSuccess(), true);
}

TEST_F(ObjectRequestPaymentTest, SelectObjectTest)
{
    std::string sqlMessage = std::string("name,school,company,age\r\n")
        .append("Lora Francis,School A,Staples Inc,27\r\n")
        .append("Eleanor Little,School B,\"Conectiv, Inc\",43\r\n")
        .append("Rosie Hughes,School C,Western Gas Resources Inc,44\r\n")
        .append("Lawrence Ross,School D,MetLife Inc.,24");
    std::string key = TestUtils::GetObjectKey("SqlObjectWithCsvData");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    // put object
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
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

    auto outcome = PayerClient->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "AccessDenied");

    selectRequest.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "AccessDenied");
    EXPECT_EQ(outcome.error().Message(), "Select API does not support requester pay now.");

    // createSelectObjectMeta
    CreateSelectObjectMetaRequest metaRequest(BucketName, key);
    metaRequest.setInputFormat(csvInputFormat);
    auto metaOutcome = PayerClient->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(metaOutcome.isSuccess(), false);
    EXPECT_EQ(metaOutcome.error().Code(), "AccessDenied");

    metaRequest.setRequestPayer(RequestPayer::Requester);
    metaOutcome = PayerClient->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(metaOutcome.isSuccess(), false);
    EXPECT_EQ(metaOutcome.error().Code(), "AccessDenied");
    EXPECT_EQ(metaOutcome.error().Message(), "Select API does not support requester pay now.");
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

TEST_F(ObjectRequestPaymentTest, ListObjectsAsyncTest)
{
    // create test file
    for (int i = 0; i < 20; i++) {
        std::string key = TestUtils::GetObjectKey("ListAllObjectsAsync");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    // list objects async
    ListObjectsRequest request(BucketName);
    request.setPrefix("ListAllObjectsAsync");
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
TEST_F(ObjectRequestPaymentTest, GetObjectAsyncTest)
{

    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectAsyncBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(102400);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectAsyncHandler handler = GetObjectHandler;
    GetObjectRequest request(BucketName, key);
    request.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<GetObjectAsyncContex> memContext = std::make_shared<GetObjectAsyncContex>();

    GetObjectRequest fileRequest(BucketName, key);
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

TEST_F(ObjectRequestPaymentTest, PutObjectAsyncTest)
{

    std::string memKey = TestUtils::GetObjectKey("PutObjectAsyncBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);

    std::string fileKey = TestUtils::GetObjectKey("PutObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectAsyncBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);

    PutObjectAsyncHandler handler = PutObjectHandler;
    PutObjectRequest memRequest(BucketName, memKey, memContent);
    memRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<PutObjectAsyncContex> memContext = std::make_shared<PutObjectAsyncContex>();
    memContext->setUuid("PutobjectasyncFromMem");

    PutObjectRequest fileRequest(BucketName, fileKey, fileContent);
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

TEST_F(ObjectRequestPaymentTest, UploadPartAsyncTest)
{

    std::string memKey = TestUtils::GetObjectKey("UploadPartMemObjectAsyncBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);
    InitiateMultipartUploadRequest memInitRequest(BucketName, memKey);
    auto memInitOutcome = Client->InitiateMultipartUpload(memInitRequest);
    EXPECT_EQ(memInitOutcome.isSuccess(), true);
    EXPECT_EQ(memInitOutcome.result().Key(), memKey);

    std::string fileKey = TestUtils::GetObjectKey("UploadPartFileObjectAsyncBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("UploadPartFileObjectAsyncBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);
    InitiateMultipartUploadRequest fileInitRequest(BucketName, fileKey);
    auto fileInitOutcome = Client->InitiateMultipartUpload(fileInitRequest);
    EXPECT_EQ(fileInitOutcome.isSuccess(), true);
    EXPECT_EQ(fileInitOutcome.result().Key(), fileKey);

    UploadPartAsyncHandler handler = UploadPartHandler;

    UploadPartRequest memRequest(BucketName, memKey, 1, memInitOutcome.result().UploadId(), memContent);
    memRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<UploadPartAsyncContext> memContext = std::make_shared<UploadPartAsyncContext>();
    memContext->setUuid("UploadPartAsyncFromMem");

    UploadPartRequest fileRequest(BucketName, fileKey, 1, fileInitOutcome.result().UploadId(), fileContent);
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

    auto memListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName, memKey, memInitOutcome.result().UploadId()));
    EXPECT_EQ(memListPartsOutcome.isSuccess(), true);
    auto fileListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName, fileKey, fileInitOutcome.result().UploadId()));
    EXPECT_EQ(fileListPartsOutcome.isSuccess(), true);

    auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, memKey,
        memListPartsOutcome.result().PartList(), memInitOutcome.result().UploadId()));
    EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
    auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, fileKey,
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

TEST_F(ObjectRequestPaymentTest, UploadPartCopyAsyncTest)
{
    // put object from buffer
    std::string memObjKey = TestUtils::GetObjectKey("PutObjectFromBuffer");
    auto memObjContent = TestUtils::GetRandomStream(102400);
    auto putMemObjOutcome = Client->PutObject(PutObjectRequest(BucketName, memObjKey, memObjContent));
    EXPECT_EQ(putMemObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, memObjKey), true);

    // put object from local file
    std::string fileObjKey = TestUtils::GetObjectKey("PutObjectFromFile");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFile").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileObjContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::out | std::ios::binary);
    auto putFileObjOutcome = Client->PutObject(PutObjectRequest(BucketName, fileObjKey, fileObjContent));
    EXPECT_EQ(putFileObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, fileObjKey), true);

    // close the file
    fileObjContent->close();

    // apply the upload id
    std::string memKey = TestUtils::GetObjectKey("UploadPartCopyMemObjectAsyncBasicTest");
    auto memInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, memKey));
    EXPECT_EQ(memInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(memInitObjOutcome.result().Key(), memKey);

    std::string fileKey = TestUtils::GetObjectKey("UploadPartCopyFileObjectAsyncBasicTest");
    auto fileInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, fileKey));
    EXPECT_EQ(fileInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(fileInitObjOutcome.result().Key(), fileKey);

    // construct parameter
    UploadPartCopyAsyncHandler handler = UploadPartCopyHandler;
    UploadPartCopyRequest memRequest(BucketName, memKey, BucketName, memObjKey, memInitObjOutcome.result().UploadId(), 1);
    memRequest.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<UploadPartAsyncContext> memContext = std::make_shared<UploadPartAsyncContext>();
    memContext->setUuid("UploadPartCopyAsyncFromMem");
    UploadPartCopyRequest fileRequest(BucketName, fileKey, BucketName, fileObjKey, fileInitObjOutcome.result().UploadId(), 1);
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
    auto memListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName, memKey, memInitObjOutcome.result().UploadId()));
    EXPECT_EQ(memListPartsOutcome.isSuccess(), true);
    auto fileListPartsOutcome = Client->ListParts(ListPartsRequest(BucketName, fileKey, fileInitObjOutcome.result().UploadId()));
    EXPECT_EQ(fileListPartsOutcome.isSuccess(), true);

    // complete
    CompleteMultipartUploadRequest memCompleteRequest(BucketName, memKey, memListPartsOutcome.result().PartList());
    memCompleteRequest.setUploadId(memInitObjOutcome.result().UploadId());
    auto memCompleteOutcome = Client->CompleteMultipartUpload(memCompleteRequest);
    EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
    CompleteMultipartUploadRequest fileCompleteRequest(BucketName, fileKey, fileListPartsOutcome.result().PartList());
    fileCompleteRequest.setUploadId(fileInitObjOutcome.result().UploadId());
    auto fileCompleteOutcome = Client->CompleteMultipartUpload(fileCompleteRequest);
    EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

    fileObjContent = nullptr;
    TestUtils::WaitForCacheExpire(1);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, ListObjectsCallableTest)
{
    for (int i = 0; i < 20; i++) {
        std::string key = TestUtils::GetObjectKey("ListAllObjectsCallableTest");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(BucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    //list object use default
    ListObjectsRequest request(BucketName);
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

TEST_F(ObjectRequestPaymentTest, GetObjectCallableTest)
{
    GetObjectOutcome dummy;
    std::string key = TestUtils::GetObjectKey("GetObjectCallableBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("GetObjectCallableBasicTest").append(".tmp");
    auto content = TestUtils::GetRandomStream(102400);

    auto pOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    GetObjectRequest request(BucketName, key);
    request.setRequestPayer(RequestPayer::Requester);
    std::shared_ptr<GetObjectAsyncContex> memContext = std::make_shared<GetObjectAsyncContex>();

    GetObjectRequest fileRequest(BucketName, key);
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

TEST_F(ObjectRequestPaymentTest, PutObjectCallableTest)
{
    std::string memKey = TestUtils::GetObjectKey("PutObjectCallableBasicTest");
    auto memContent = TestUtils::GetRandomStream(102400);

    std::string fileKey = TestUtils::GetObjectKey("PutObjectCallableBasicTest");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectCallableBasicTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);

    PutObjectRequest putrequest1(BucketName, memKey, memContent);
    putrequest1.setRequestPayer(RequestPayer::Requester);
    PutObjectRequest putrequest2(BucketName, fileKey, fileContent);
    putrequest2.setRequestPayer(RequestPayer::Requester);
    auto memOutcomeCallable = PayerClient->PutObjectCallable(putrequest1);
    auto fileOutcomeCallable = PayerClient->PutObjectCallable(putrequest2);

    std::cout << "PayerClient[" << PayerClient << "]" << "Issue PutObjectCallable done." << std::endl;

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

TEST_F(ObjectRequestPaymentTest, UploadPartCallableTest)
{
    auto memKey = TestUtils::GetObjectKey("MultipartUploadCallable-MemObject");
    auto memContent = TestUtils::GetRandomStream(102400);
    auto memInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, memKey));
    EXPECT_EQ(memInitOutcome.isSuccess(), true);

    auto fileKey = TestUtils::GetObjectKey("MultipartUploadCallable-FileObject");
    auto tmpFile = TestUtils::GetObjectKey("MultipartUploadCallable-FileObject").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    {
        auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);
        auto fileInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, fileKey));
        EXPECT_EQ(fileInitOutcome.isSuccess(), true);

        UploadPartRequest uprequest1(BucketName, memKey, 1, memInitOutcome.result().UploadId(), memContent);
        UploadPartRequest uprequest2(BucketName, fileKey, 1, fileInitOutcome.result().UploadId(), fileContent);
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
        auto memListPartOutcome = Client->ListParts(ListPartsRequest(BucketName, memKey, memInitOutcome.result().UploadId()));
        auto fileListPartOutcome = Client->ListParts(ListPartsRequest(BucketName, fileKey, fileInitOutcome.result().UploadId()));
        EXPECT_EQ(memListPartOutcome.isSuccess(), true);
        EXPECT_EQ(fileListPartOutcome.isSuccess(), true);

        auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, memKey,
            memListPartOutcome.result().PartList(), memInitOutcome.result().UploadId()));
        auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, fileKey,
            fileListPartOutcome.result().PartList(), fileInitOutcome.result().UploadId()));
        EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
        EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

        EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

        memContent = nullptr;
        fileContent = nullptr;
    }
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, UploadPartCopyCallableTest)
{
    // put object from buffer
    std::string memObjKey = TestUtils::GetObjectKey("PutObjectFromBuffer");
    auto memObjContent = TestUtils::GetRandomStream(102400);
    auto putMemObjOutcome = Client->PutObject(PutObjectRequest(BucketName, memObjKey, memObjContent));
    EXPECT_EQ(putMemObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, memObjKey), true);

    // put object from local file
    std::string fileObjKey = TestUtils::GetObjectKey("PutObjectFromFile");
    std::string tmpFile = TestUtils::GetTargetFileName("PutObjectFromFile").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    auto fileObjContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);
    auto putFileObjOutcome = Client->PutObject(PutObjectRequest(BucketName, fileObjKey, fileObjContent));
    EXPECT_EQ(putFileObjOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, fileObjKey), true);

    // close file
    fileObjContent->close();

    // apply upload id
    std::string memKey = TestUtils::GetObjectKey("UploadPartCopyCallableMemObjectBasicTest");
    auto memInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, memKey));
    EXPECT_EQ(memInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(memInitObjOutcome.result().Key(), memKey);

    std::string fileKey = TestUtils::GetObjectKey("UploadPartCopyCallableFileObjectBasicTest");
    auto fileInitObjOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, fileKey));
    EXPECT_EQ(fileInitObjOutcome.isSuccess(), true);
    EXPECT_EQ(fileInitObjOutcome.result().Key(), fileKey);

    // upload part copy
    UploadPartCopyRequest request1(BucketName, memKey, BucketName, memObjKey, memInitObjOutcome.result().UploadId(), 1);
    UploadPartCopyRequest request2(BucketName, fileKey, BucketName, fileObjKey, fileInitObjOutcome.result().UploadId(), 1);
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
    auto memListPartOutcome = Client->ListParts(ListPartsRequest(BucketName, memKey, memInitObjOutcome.result().UploadId()));
    auto fileListPartOutcome = Client->ListParts(ListPartsRequest(BucketName, fileKey, fileInitObjOutcome.result().UploadId()));
    EXPECT_EQ(memListPartOutcome.isSuccess(), true);
    EXPECT_EQ(fileListPartOutcome.isSuccess(), true);

    // complete the part
    auto memCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, memKey,
        memListPartOutcome.result().PartList(), memInitObjOutcome.result().UploadId()));
    auto fileCompleteOutcome = Client->CompleteMultipartUpload(CompleteMultipartUploadRequest(BucketName, fileKey,
        fileListPartOutcome.result().PartList(), fileInitObjOutcome.result().UploadId()));
    EXPECT_EQ(memCompleteOutcome.isSuccess(), true);
    EXPECT_EQ(fileCompleteOutcome.isSuccess(), true);

    EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

    memObjContent = nullptr;
    fileObjContent = nullptr;
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

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
TEST_F(ObjectRequestPaymentTest, ProcessObjectRequestTest)
{
    std::string ImageFilePath = Config::GetDataPath();
    ImageFilePath.append("example.jpg");
    std::string Process = "image/resize,m_fixed,w_100,h_100";
    std::string ImageInfo = "{\n    \"FileSize\": {\"value\": \"3267\"},\n    \"Format\": {\"value\": \"jpg\"},\n    \"FrameCount\": {\"value\": \"1\"},\n    \"ImageHeight\": {\"value\": \"100\"},\n    \"ImageWidth\": {\"value\": \"100\"},\n    \"ResolutionUnit\": {\"value\": \"1\"},\n    \"XResolution\": {\"value\": \"1/1\"},\n    \"YResolution\": {\"value\": \"1/1\"}}";

    std::string key = TestUtils::GetObjectKey("ImageProcessBysetProcessAndSavetoTest");
    std::string key1 = key;
    std::string key2 = key;
    key.append("-noraml.jpg");
    key1.append("-saveas.jpg");
    key2.append("-saveas2.jpg");
    auto pOutcome = Client->PutObject(BucketName, key, ImageFilePath);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    std::stringstream ss;
    ss << Process
        << "|sys/saveas"
        << ",o_" << Base64EncodeUrlSafe(key1)
        << ",b_" << Base64EncodeUrlSafe(BucketName);

    ProcessObjectRequest request(BucketName, key, ss.str());
    auto gOutcome = PayerClient->ProcessObject(request);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "AccessDenied");

    request.setRequestPayer(RequestPayer::Requester);
    gOutcome = PayerClient->ProcessObject(request);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    std::istreambuf_iterator<char> isb(*gOutcome.result().Content()), end;
    std::string json_str = std::string(isb, end);
    std::cout << json_str << std::endl;
    EXPECT_TRUE(json_str.find(key1) != std::string::npos);

    std::string imageInfo = GetOssImageObjectInfo(BucketName, key1);
    EXPECT_TRUE(CompareImageInfo(imageInfo, ImageInfo));
}

TEST_F(ObjectRequestPaymentTest, ObjectTaggingBasicTest)
{
    auto key = TestUtils::GetObjectKey("ObjectTaggingBasicTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    SetObjectTaggingRequest request(BucketName, key, tagging);
    auto putTaggingOutcome = PayerClient->SetObjectTagging(request);
    EXPECT_EQ(putTaggingOutcome.isSuccess(), false);
    EXPECT_EQ(putTaggingOutcome.error().Code(), "AccessDenied");

    request.setRequestPayer(RequestPayer::Requester);
    putTaggingOutcome = PayerClient->SetObjectTagging(request);
    EXPECT_EQ(putTaggingOutcome.isSuccess(), true);
    EXPECT_TRUE(putTaggingOutcome.result().RequestId().size() > 0U);

    GetObjectTaggingRequest gRequest(BucketName, key);
    auto getTaggingOutcome = PayerClient->GetObjectTagging(gRequest);
    EXPECT_EQ(getTaggingOutcome.isSuccess(), false);
    EXPECT_EQ(getTaggingOutcome.error().Code(), "AccessDenied");

    gRequest.setRequestPayer(RequestPayer::Requester);
    getTaggingOutcome = PayerClient->GetObjectTagging(gRequest);
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);
    EXPECT_TRUE(getTaggingOutcome.result().RequestId().size() > 0U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }

    DeleteObjectTaggingRequest dRequest(BucketName, key);
    auto delTaggingOutcome = PayerClient->DeleteObjectTagging(dRequest);
    EXPECT_EQ(delTaggingOutcome.isSuccess(), false);
    EXPECT_EQ(delTaggingOutcome.error().Code(), "AccessDenied");

    dRequest.setRequestPayer(RequestPayer::Requester);
    delTaggingOutcome = PayerClient->DeleteObjectTagging(dRequest);
    EXPECT_EQ(delTaggingOutcome.isSuccess(), true);
    EXPECT_TRUE(delTaggingOutcome.result().RequestId().size() > 0U);
    
    getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 0U);
}

TEST_F(ObjectRequestPaymentTest, NormalResumableUploadWithSizeOverPartSizeTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadObjectOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadObjectOverPartSize").append(".tmp");
    int num = 4;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    auto outcome = PayerClient->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "AccessDenied");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(BucketName, key);
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);

    std::fstream file(tmpFile, std::ios::in | std::ios::binary);
    std::string oriMd5 = ComputeContentMD5(file);
    std::string memMd5 = ComputeContentMD5(*getObjectOutcome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);

    file.close();
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, NormalResumableUploadWithSizeUnderPartSizeTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadObjectUnderPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadObjectUnderPartSize").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 10240);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    auto outcome = PayerClient->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "AccessDenied");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(BucketName, key);
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, NormalResumableDownloadWithSizeOverPartSizeTest)
{
    // upload object
    std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectOverPartSize").append(".tmp");
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    int num = 4;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    auto uploadOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    auto outcome = PayerClient->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:403");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, NormalResumableDownloadWithSizeUnderPartSizeTest)
{
    // upload object
    std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectUnderPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectUnderPartSize").append(".tmp");
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    int num = 4;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    auto uploadOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setThreadNum(1);
    auto outcome = PayerClient->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:403");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectRequestPaymentTest, NormalResumableCopyWithSizeOverPartSizeTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectOverPartSize");
    std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectOverPartSize");
    // put object into bucket
    int num = 1 + rand() % 10;
    auto putObjectContent = TestUtils::GetRandomStream(102400 * num);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    // Copy Object
    MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    auto outcome = PayerClient->ResumableCopyObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:403");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->ResumableCopyObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
}

TEST_F(ObjectRequestPaymentTest, NormalResumableCopyWithSizeUnderPartSizeTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectUnderPartSize");
    std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectUnderPartSize");
    // put Object into bucket
    auto putObjectContent = TestUtils::GetRandomStream(1024 * (rand() % 100));
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    // copy object
    MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
    request.setPartSize(100 * 1024 + 1);
    auto outcome = PayerClient->ResumableCopyObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ServerError:403");

    request.setRequestPayer(RequestPayer::Requester);
    outcome = PayerClient->ResumableCopyObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
}

TEST_F(ObjectRequestPaymentTest, SignUrlTest)
{
    std::string key = TestUtils::GetObjectKey("SignUrlTest");

    PutObjectRequest putRequest(BucketName, key, std::make_shared<std::stringstream>("hello world"));
    putRequest.setRequestPayer(RequestPayer::Requester);
    auto putOutcome = PayerClient->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    GeneratePresignedUrlRequest gRequest(BucketName, key, Http::Method::Get);
    auto gOutcome = PayerClient->GeneratePresignedUrl(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    auto gurlOutcome = PayerClient->GetObjectByUrl(gOutcome.result());
    EXPECT_EQ(gurlOutcome.isSuccess(), false);
    EXPECT_EQ(gurlOutcome.error().Code(), "AccessDenied");

    gRequest.setRequestPayer(RequestPayer::Requester);
    gOutcome = PayerClient->GeneratePresignedUrl(gRequest);
    gurlOutcome = PayerClient->GetObjectByUrl(gOutcome.result());
    EXPECT_EQ(gurlOutcome.isSuccess(), true);
    std::istreambuf_iterator<char> isb(*gurlOutcome.result().Content().get()), end;
    std::string str(isb, end);
    EXPECT_EQ(str, "hello world");
}

}
}
