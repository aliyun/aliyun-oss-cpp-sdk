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
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace AlibabaCloud {
namespace OSS {
class ObjectAsyncTest : public ::testing::Test {
protected:
    ObjectAsyncTest()
	{
	}
	~ObjectAsyncTest() override
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

public:
	static std::shared_ptr<OssClient> Client;
	static std::string BucketName;
	static std::string TestFile;
	static std::string TestFileMd5;

};

std::shared_ptr<OssClient> ObjectAsyncTest::Client = nullptr;
std::string ObjectAsyncTest::BucketName = "";
std::string ObjectAsyncTest::TestFile = "";
std::string ObjectAsyncTest::TestFileMd5 = "";

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

void UploadPartInvalidHandler(const AlibabaCloud::OSS::OssClient* client,
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

void UploadPartHandler(const AlibabaCloud::OSS::OssClient* client,
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

TEST_F(ObjectAsyncTest, UploadPartAsyncBasicTest)
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
	std::shared_ptr<UploadPartAsyncContext> memContext = std::make_shared<UploadPartAsyncContext>();
	memContext->setUuid("UploadPartAsyncFromMem");

	UploadPartRequest fileRequest(BucketName, fileKey, 1, fileInitOutcome.result().UploadId(), fileContent);
	std::shared_ptr<UploadPartAsyncContext> fileContext = std::make_shared<UploadPartAsyncContext>();
	fileContext->setUuid("UploadPartAsyncFromFile");

	Client->UploadPartAsync(memRequest, handler, memContext);
	Client->UploadPartAsync(fileRequest, handler, fileContext);
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

	EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
	EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

	fileContext = nullptr;
	TestUtils::WaitForCacheExpire(1);
	EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(ObjectAsyncTest, UploadPartAsyncInvalidContentTest)
{
	std::string key = TestUtils::GetObjectKey("UploadPartCopyAsyncInvalidContentTest");
	UploadPartAsyncHandler handler = UploadPartInvalidHandler;
	std::shared_ptr<std::iostream> content = nullptr;
	UploadPartRequest request(BucketName, key, 1, "id", content);
	std::shared_ptr<UploadPartAsyncContext> context = std::make_shared<UploadPartAsyncContext>();
	context->setUuid("UploadPartCopyAsyncInvalidContent");
	Client->UploadPartAsync(request, handler, context);

#ifdef _WIN32
	Sleep(1 * 1000);
#else
	sleep(1);
#endif
	content = TestUtils::GetRandomStream(100);
	request.setConetent(content);
	Client->UploadPartAsync(request, handler, context);

#ifdef _WIN32
	Sleep(1 * 1000);
#else
	sleep(1);
#endif
	request.setContentLength(5LL * 1024LL * 1024LL * 1024LL + 1LL);
	Client->UploadPartAsync(request, handler, context);
	request.setContentLength(100);
#ifdef _WIN32
	Sleep(1 * 1000);
#else
	sleep(1);
#endif
	request.setBucket("non-existent-bucket");
	Client->UploadPartAsync(request, handler, context);
	request.setBucket(BucketName);
}

void UploadPartCopyHandler(const AlibabaCloud::OSS::OssClient* client,
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
		EXPECT_EQ(outcome.isSuccess(), true);
		std::unique_lock<std::mutex> lck(ctx->mtx);
		ctx->ready = true;
		ctx->cv.notify_all();
	}
}


TEST_F(ObjectAsyncTest, UploadPartCopyAsyncBasicTest)
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
	std::shared_ptr<UploadPartAsyncContext> memContext = std::make_shared<UploadPartAsyncContext>();
	memContext->setUuid("UploadPartCopyAsyncFromMem");
	UploadPartCopyRequest fileRequest(BucketName, fileKey, BucketName, fileObjKey, fileInitObjOutcome.result().UploadId(), 1);
	std::shared_ptr<UploadPartAsyncContext> fileContext = std::make_shared<UploadPartAsyncContext>();
	fileContext->setUuid("UploadPartCopyAsyncFromFile");

	Client->UploadPartCopyAsync(memRequest, handler, memContext);
	Client->UploadPartCopyAsync(fileRequest, handler, fileContext);
	std::cout << "Client[" << Client << "]" << "Issue UploadPartCopyAsync done." << std::endl;
	{
		std::unique_lock<std::mutex> lck(fileContext->mtx);
		if(!fileContext->ready) fileContext->cv.wait(lck);
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

	EXPECT_EQ(Client->DoesObjectExist(BucketName, memKey), true);
	EXPECT_EQ(Client->DoesObjectExist(BucketName, fileKey), true);

	fileObjContent = nullptr;
	TestUtils::WaitForCacheExpire(1);
	EXPECT_EQ(RemoveFile(tmpFile), true);
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

void ListObjectsHandler(const AlibabaCloud::OSS::OssClient* client,
    const ListObjectsRequest& request,
    const ListObjectOutcome& outcome,
    const std::shared_ptr<const AsyncCallerContext>& context)
{
	UNUSED_PARAM(request);
    std::cout << "Client[" << client << "]" << "ListObjectsHandler" << std::endl;
    if (context != nullptr) {
        auto ctx = static_cast<const ListObjectsAsyncContext *>(context.get());
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

TEST_F(ObjectAsyncTest, ListObjectsAsyncBasicTest)
{
    auto ListObjectBucketName = TestUtils::GetBucketName("list-object-async-bucket");
    auto bucketOutcome = Client->CreateBucket(CreateBucketRequest(ListObjectBucketName));
    EXPECT_EQ(bucketOutcome.isSuccess(), true);

    // create test file
    for (int i = 0; i < 20; i++) {
        std::string key = TestUtils::GetObjectKey("ListAllObjectsAsync");
        auto content = TestUtils::GetRandomStream(100);
        auto outcome = Client->PutObject(ListObjectBucketName, key, content);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    // list objects async
    ListObjectsRequest request(ListObjectBucketName);
    ListObjectAsyncHandler handler = ListObjectsHandler;
    std::shared_ptr<ListObjectsAsyncContext> content = std::make_shared<ListObjectsAsyncContext>();
    Client->ListObjectsAsync(request, handler, content);
    std::cout << "Client[" << Client << "]" << "Issue ListObjectsAsync done." << std::endl;

    {
        std::unique_lock<std::mutex> lck(content->mtx);
        if (!content->ready) content->cv.wait(lck);
    }
    int i = 0;
    for (auto const &obj : content->ObjectSummarys()) {
        EXPECT_EQ(obj.Size(), 100LL);
        i++;
    }
    EXPECT_EQ(i, 20);

    TestUtils::CleanBucket(*Client, ListObjectBucketName);
    EXPECT_EQ(Client->DoesBucketExist(ListObjectBucketName), false);
}

TEST_F(ObjectAsyncTest, AsyncCallerContextClassTest)
{
    AsyncCallerContext context("AsyncCallerContextClassTest");
}
}
}