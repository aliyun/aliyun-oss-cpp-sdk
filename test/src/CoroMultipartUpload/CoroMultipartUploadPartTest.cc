/*
 * Copyright 2009-2023 Alibaba Cloud All rights reserved.
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
#include <src/http/CoroHttpClient.hpp>

namespace AlibabaCloud {
namespace OSS {

class CoroTest : public ::testing::Test {
protected:
	CoroTest()
	{
	}
	~CoroTest()
	{
	}

	// Sets up the stuff shared by all tests in this test case.
	static void SetUpTestCase()
	{
        ClientConfiguration config{};
        config.httpClient = std::make_shared<CoroHttpClient>();
		Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, config);

        BucketName = "cpp-sdk-coro";
		auto outcome = Client->CreateBucket(CreateBucketRequest(BucketName));
        if(outcome.isSuccess()){
            std::cout << "Create Bucket "<<BucketName<<" Successfully" << std::endl;
        }
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

std::shared_ptr<OssClient> CoroTest::Client = nullptr;
std::string CoroTest::BucketName = "";

TEST_F(CoroTest, MultipartUploadCoroBasicTest)
{
    auto memKey = TestUtils::GetObjectKey("MultipartUploadCoro-MemObject");
    auto memContent = TestUtils::GetRandomStream(102400);
    auto memInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, memKey));
    EXPECT_EQ(memInitOutcome.isSuccess(), true);

    auto fileKey = TestUtils::GetObjectKey("MultipartUploadCoro-FileObject");
    auto tmpFile = TestUtils::GetObjectKey("MultipartUploadCoro-FileObject").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    {
    auto fileContent = std::make_shared<std::fstream>(tmpFile, std::ios_base::in | std::ios::binary);
    auto fileInitOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, fileKey));
    EXPECT_EQ(fileInitOutcome.isSuccess(), true);

    auto mem_request = UploadPartRequest(BucketName, memKey, 1, memInitOutcome.result().UploadId(), memContent);
    auto file_request = UploadPartRequest(BucketName, fileKey, 1, fileInitOutcome.result().UploadId(), tmpFile);
    
    auto memOutcomeCallable = Client->UploadPartCoro(mem_request);
    auto fileOutcomeCallable = Client->UploadPartCoro(file_request);

    std::cout << "Client[" << Client << "]" << "Issue MultipartUploadCoro done." << std::endl;

    auto fileOutcome = async_simple::coro::syncAwait(fileOutcomeCallable);
    auto menOutcome = async_simple::coro::syncAwait(memOutcomeCallable);

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

}
}