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

class CallableTest : public ::testing::Test {
protected:
	CallableTest()
	{
	}
	~CallableTest()
	{
	}

	// Sets up the stuff shared by all tests in this test case.
	static void SetUpTestCase()
	{
		Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
		BucketName = TestUtils::GetBucketName("cpp-sdk-callable");
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

std::shared_ptr<OssClient> CallableTest::Client = nullptr;
std::string CallableTest::BucketName = "";

TEST_F(CallableTest, MultipartUploadCallableBasicTest)
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

    auto memOutcomeCallable = Client->UploadPartCallable(UploadPartRequest(BucketName, memKey,
        1, memInitOutcome.result().UploadId(), memContent));
    auto fileOutcomeCallable = Client->UploadPartCallable(UploadPartRequest(BucketName, fileKey,
        1, fileInitOutcome.result().UploadId(), fileContent));
    
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

TEST_F(CallableTest, MultipartUploadCopyCallableBasicTest)
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
    auto memOutcomeCallable = Client->UploadPartCopyCallable(UploadPartCopyRequest(BucketName, memKey,
        BucketName, memObjKey, memInitObjOutcome.result().UploadId(), 1));
    auto fileOutcomeCallable = Client->UploadPartCopyCallable(UploadPartCopyRequest(BucketName, fileKey,
        BucketName, fileObjKey, fileInitObjOutcome.result().UploadId(), 1));

    std::cout << "Client[" << Client << "]" << "Issue UploadPartCopyCallable done." << std::endl;

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

}
}