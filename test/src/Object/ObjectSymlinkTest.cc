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

namespace AlibabaCloud{ namespace OSS {
class ObjectSymlinkTest : public ::testing::Test
{
protected:
    ObjectSymlinkTest()
    {
    }

    ~ObjectSymlinkTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
		BucketName = TestUtils::GetBucketName("cpp-sdk-objectsymlink");
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

std::shared_ptr<OssClient> ObjectSymlinkTest::Client = nullptr;
std::string ObjectSymlinkTest::BucketName = "";

TEST_F(ObjectSymlinkTest, SetAndGetObjectSymlinkSuccessTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectsymlink");

    // put object
    std::string text = "hellowworld";
    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, objName, std::make_shared<std::stringstream>(text)));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // create symlink success
    std::string linkName = objName+"-link";
    CreateSymlinkRequest setRequest(BucketName, linkName);
    setRequest.SetSymlinkTarget(objName);
    CreateSymlinkOutcome  linkOutcom = Client->CreateSymlink(setRequest);
    EXPECT_EQ(linkOutcom.isSuccess(), true);

    // read symlink success
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName, linkName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData ;
    (*getOutcome.result().Content().get())>>strData;
    EXPECT_EQ(strData, text);

    GetSymlinkOutcome getLinkOutcome = Client->GetSymlink(GetSymlinkRequest(BucketName, linkName));
    EXPECT_EQ(getLinkOutcome.isSuccess(), true);
    EXPECT_EQ(getLinkOutcome.result().SymlinkTarget(), objName);
    
    TestUtils::WaitForCacheExpire(5);

    // create symlink to symlink success 
    std::string linkNameRepeat = linkName +"-link";
    CreateSymlinkRequest setRequestRepeat(BucketName, linkNameRepeat);
    setRequestRepeat.SetSymlinkTarget(linkName);
	linkOutcom = Client->CreateSymlink(setRequestRepeat);
    EXPECT_EQ(linkOutcom.isSuccess(), true);

    getLinkOutcome = Client->GetSymlink(GetSymlinkRequest(BucketName, linkNameRepeat));
    EXPECT_EQ(getLinkOutcome.isSuccess(), true);
    EXPECT_EQ(getLinkOutcome.result().SymlinkTarget(), linkName);

	//but read symlink to symlink error
	getOutcome = Client->GetObject(GetObjectRequest(BucketName, linkNameRepeat));
	EXPECT_EQ(getOutcome.isSuccess(), false);
    
	// create symlink to not exist object success, but read error
	std::string linkNameNotExist = "test-link";
	CreateSymlinkRequest setRequestNotExist(BucketName, linkNameNotExist);
	setRequestNotExist.SetSymlinkTarget("not-exist-object-name");
	linkOutcom = Client->CreateSymlink(setRequestNotExist);
	EXPECT_EQ(linkOutcom.isSuccess(), true);
    
    getLinkOutcome = Client->GetSymlink(GetSymlinkRequest(BucketName, linkNameNotExist));
    EXPECT_EQ(getLinkOutcome.isSuccess(), true);
    EXPECT_EQ(getLinkOutcome.result().SymlinkTarget(), "not-exist-object-name");

	getOutcome = Client->GetObject(GetObjectRequest(BucketName, linkNameNotExist));
	EXPECT_EQ(getOutcome.isSuccess(), false);

	//change symlink to another exist object success and read success
	std::string objNameOther = TestUtils::GetObjectKey("test-cpp-sdk-objectsymlink");
	std::string textOther = "hellowworldhellowworld";
	putOutcome = Client->PutObject(PutObjectRequest(BucketName, objNameOther, std::make_shared<std::stringstream>(textOther)));
	EXPECT_EQ(putOutcome.isSuccess(), true);

	CreateSymlinkRequest setRequestOther(BucketName, linkName);
	setRequestOther.SetSymlinkTarget(objNameOther);
	linkOutcom = Client->CreateSymlink(setRequestOther);
	EXPECT_EQ(linkOutcom.isSuccess(), true);

    getLinkOutcome = Client->GetSymlink(GetSymlinkRequest(BucketName, linkName));
    EXPECT_EQ(getLinkOutcome.isSuccess(), true);
    EXPECT_EQ(getLinkOutcome.result().SymlinkTarget(), objNameOther);

	getOutcome = Client->GetObject(GetObjectRequest(BucketName, linkName));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, textOther);

	// create symlink with meta
	std::string linkNameWithMeta = objName +"-test-link-meta";
	ObjectMetaData MetaInfo;
	MetaInfo.UserMetaData()["author"] = "chanju";

	CreateSymlinkRequest setRequestWithMeta(BucketName, linkNameWithMeta, MetaInfo);
	setRequestWithMeta.SetSymlinkTarget(objNameOther);
	linkOutcom = Client->CreateSymlink(setRequestWithMeta);
	EXPECT_EQ(linkOutcom.isSuccess(), true);

    getLinkOutcome = Client->GetSymlink(GetSymlinkRequest(BucketName, linkNameWithMeta));
    EXPECT_EQ(getLinkOutcome.isSuccess(), true);
    EXPECT_EQ(getLinkOutcome.result().SymlinkTarget(), objNameOther);

    // read meta
	getOutcome = Client->GetObject(GetObjectRequest(BucketName, linkNameWithMeta));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, textOther);
	std::string metaValue = getOutcome.result().Metadata().UserMetaData().at("author");
    EXPECT_EQ(metaValue, "chanju");

	// test Archive bucket
	std::string  archiveBucket = TestUtils::GetBucketName("cpp-sdk-arvhive-objectsymlink");
	CreateBucketOutcome bucketOutcome =  Client->CreateBucket(CreateBucketRequest(archiveBucket, Archive));
	EXPECT_EQ(bucketOutcome.isSuccess(), true);

	// put object in archiveBucket
	std::string archiveObjName = TestUtils::GetObjectKey("test-cpp-sdk-archive");
	putOutcome = Client->PutObject(PutObjectRequest(archiveBucket, archiveObjName, TestUtils::GetRandomStream(1024 * 1024)));
	EXPECT_EQ(putOutcome.isSuccess(), true);

    std::string linkNameArchive = objName + "-test-link-archive";
	CreateSymlinkRequest setArchiveRequest(archiveBucket, linkNameArchive);
	setArchiveRequest.SetSymlinkTarget(archiveObjName);
	linkOutcom = Client->CreateSymlink(setArchiveRequest);
	EXPECT_EQ(linkOutcom.isSuccess(), true);
	TestUtils::CleanBucket(*Client, archiveBucket);
}


TEST_F(ObjectSymlinkTest, SetAndGetObjectSymlinkErrorTest)
{
    GetSymlinkOutcome getLinkOutcome = Client->GetSymlink(GetSymlinkRequest(BucketName, "aaa"));
    EXPECT_EQ(getLinkOutcome.isSuccess(), false);
    EXPECT_EQ(getLinkOutcome.error().Code().size()>0, true);
}

TEST_F(ObjectSymlinkTest, CreateSymlinkResultTest)
{
    CreateSymlinkResult reuslt;
    EXPECT_EQ(reuslt.ETag(), "");

    CreateSymlinkResult reuslt1("ETag");
    reuslt = reuslt1;
    EXPECT_EQ(reuslt.ETag(), "ETag");

    GetSymlinkResult result2("symlink", "etag");
    EXPECT_EQ(result2.ETag(), "etag");
    EXPECT_EQ(result2.SymlinkTarget(), "symlink");

    auto headers = HeaderCollection();
    GetSymlinkResult result3(headers);
    EXPECT_EQ(result3.ETag(), "");
}

TEST_F(ObjectSymlinkTest, CreateSymlinkNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-symlink");
    CreateSymlinkRequest setRequestRepeat(name, "test-key");
    setRequestRepeat.SetSymlinkTarget("test-key-link");
    auto outcome = Client->CreateSymlink(setRequestRepeat);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}
/*
TEST_F(ObjectSymlinkTest, CreateSymlinkNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-symlink");
    CreateSymlinkRequest setRequestRepeat(name, "test-key");
    setRequestRepeat.SetSymlinkTarget("test-key-link");
    auto outcome = Client->CreateSymlink(setRequestRepeat);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().code(), "NoSuchBucket");
}
*/
}
}
