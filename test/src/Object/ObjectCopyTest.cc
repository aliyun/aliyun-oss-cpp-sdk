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
class ObjectCopyTest : public ::testing::Test
{
protected:
    ObjectCopyTest()
    {
    }

    ~ObjectCopyTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
		conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());

        ListBucketsRequest request;
		request.setPrefix("cpp-sdk-objectcopy");
		request.setMaxKeys(200);
		auto outcome = Client->ListBuckets(request);
		EXPECT_EQ(outcome.isSuccess(), true);

        BucketName1 = TestUtils::GetBucketName("cpp-sdk-objectcopy1");
        CreateBucketOutcome outCome = Client->CreateBucket(CreateBucketRequest(BucketName1));
        EXPECT_EQ(outCome.isSuccess(), true);

        BucketName2 = TestUtils::GetBucketName("cpp-sdk-objectcopy2");
        outCome = Client->CreateBucket(CreateBucketRequest(BucketName2));
        EXPECT_EQ(outCome.isSuccess(), true);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
		TestUtils::CleanBucket(*Client, BucketName1);
		TestUtils::CleanBucket(*Client, BucketName2);
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
    static std::string BucketName1;
    static std::string BucketName2;
};

std::shared_ptr<OssClient> ObjectCopyTest::Client = nullptr;
std::string ObjectCopyTest::BucketName1 = "";
std::string ObjectCopyTest::BucketName2 = "";

TEST_F(ObjectCopyTest, ObjectCopyNewObject)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);

	//begin copy object
	CopyObjectOutcome copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
}

TEST_F(ObjectCopyTest, ObjectCopyNewObjectWithSrcMeta)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

    ObjectMetaData MetaInfo;
	MetaInfo.UserMetaData()["author"] = "chanju";
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);

	//begin copy object
	CopyObjectOutcome copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    std::string metaValue = getOutcome.result().Metadata().UserMetaData().at("author");
    EXPECT_EQ(metaValue, "chanju");
}


TEST_F(ObjectCopyTest, ObjectCopyNewObjectNotCreateNewMetaCopy)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

    ObjectMetaData MetaInfo;
	MetaInfo.UserMetaData()["author"] = "chanju";
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

    ObjectMetaData MetaInfoCopy;
	MetaInfoCopy.UserMetaData()["author1"] = "chanju1";
    
	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	CopyObjectRequest copyRequest(BucketName2, objName2,MetaInfoCopy);
	copyRequest.setCopySource(BucketName1, objName1);
    copyRequest.setMetadataDirective(CopyActionList::Copy);

	//begin copy object
	CopyObjectOutcome copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author"), "chanju");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author1"),
              getOutcome.result().Metadata().UserMetaData().end());
}

TEST_F(ObjectCopyTest, ObjectCopyNewObjectNotCreateNewMetaReplace)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

    ObjectMetaData MetaInfo;
	MetaInfo.UserMetaData()["author"] = "chanju";
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

    ObjectMetaData MetaInfoCopy;
	MetaInfoCopy.UserMetaData()["author1"] = "chanju1";
    MetaInfoCopy.UserMetaData()["author2"] = "chanju2";
    
	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	CopyObjectRequest copyRequest(BucketName2, objName2,MetaInfoCopy);
	copyRequest.setCopySource(BucketName1, objName1);
    copyRequest.setMetadataDirective(CopyActionList::Replace);

	//begin copy object
	CopyObjectOutcome copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author"),
              getOutcome.result().Metadata().UserMetaData().end());
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"),"chanju1");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author2"),"chanju2");
}

TEST_F(ObjectCopyTest, ObjectCopyExistObject)
{
    std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
    std::string text1 = "hellowworld";

    std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
    std::string text2 = text1 + text1;

    PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1)));
    EXPECT_EQ(putOutcome.isSuccess(), true);
    std::string etag1 = putOutcome.result().ETag();

    putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2)));
    EXPECT_EQ(putOutcome.isSuccess(), true);
    std::string etag2 = putOutcome.result().ETag();

    EXPECT_EQ(etag1!=etag2, true);

    CopyObjectRequest copyRequest(BucketName2,objName2);
    copyRequest.setCopySource(BucketName1,objName1);

    //begin copy object
    CopyObjectOutcome copyOutCome = Client->CopyObject(copyRequest);
    EXPECT_EQ(copyOutCome.isSuccess(), true);

    // get object
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData ;
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, text1);  
}

TEST_F(ObjectCopyTest, ObjectCopyETagMatch)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	std::string text2 = text1 + text1;

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag2 = putOutcome.result().ETag();

	EXPECT_EQ((etag1 != etag2), true);

    // set etag invalid
	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);
	copyRequest.setSourceIfMatchETag("aaa");
	
	//begin copy object
	CopyObjectOutcome copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), false);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text2);

    // set valid etag
    copyRequest.setSourceIfMatchETag(etag1);
    copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
}


TEST_F(ObjectCopyTest, ObjectCopyETagNotMatch)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	std::string text2 = text1 + text1;

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag2 = putOutcome.result().ETag();

	EXPECT_EQ((etag1 != etag2), true);

    CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);

    // set valid etag
	copyRequest.setSourceIfNotMatchETag(etag1);
    CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), false);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text2);

    // set etag invalid
	copyRequest.setSourceIfNotMatchETag("aaa");
	
	//begin copy object
	copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);  
}

TEST_F(ObjectCopyTest, ObjectCopyUnModifiedSince)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	std::string text2 = text1 + text1;

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag2 = putOutcome.result().ETag();

	EXPECT_EQ((etag1 != etag2), true);

    TestUtils::WaitForCacheExpire(2);

    // set early time,not copy
    CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);

    std::string earlyDate = TestUtils::GetGMTString(-100);
	copyRequest.setSourceIfUnModifiedSince(earlyDate);
    
    CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), false);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text2);

    // set later date,copy
    std::string laterDate = TestUtils::GetGMTString(100);
	copyRequest.setSourceIfUnModifiedSince(laterDate);
	
	//begin copy object
	copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);  
}

TEST_F(ObjectCopyTest, ObjectCopyModifiedSince)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	std::string text2 = text1 + text1;

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag2 = putOutcome.result().ETag();

	EXPECT_EQ((etag1 != etag2), true);

	TestUtils::WaitForCacheExpire(2);

	// set later time,not copy
	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);

	std::string laterDate = TestUtils::GetGMTString(100);
	copyRequest.setSourceIfModifiedSince(laterDate);

	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), false);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text2);

	// set early date,copy
	std::string earlyDate = TestUtils::GetGMTString(-100);
	copyRequest.setSourceIfModifiedSince(earlyDate);

	//begin copy object
	copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
}


TEST_F(ObjectCopyTest, ObjectCopyAclPrivate)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text = "hellowworld";

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	TestUtils::WaitForCacheExpire(2);

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");

	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);
	copyRequest.setAcl(CannedAccessControlList::Private);

	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text);

	// get acl
	auto aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName2, objName2));
	EXPECT_EQ(aclOutcome.isSuccess(), true);
	EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Private);
}

TEST_F(ObjectCopyTest, ObjectCopyAclPublicRead)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text = "hellowworld";

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	TestUtils::WaitForCacheExpire(2);

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");

	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);
	copyRequest.setAcl(CannedAccessControlList::PublicRead);

	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text);

	// get acl
	auto aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName2, objName2));
	EXPECT_EQ(aclOutcome.isSuccess(), true);
	EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicRead);
}

TEST_F(ObjectCopyTest, ObjectCopyAclPublicReadWrite)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text = "hellowworld";

	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text)));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	TestUtils::WaitForCacheExpire(2);

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");

	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);
	copyRequest.setAcl(CannedAccessControlList::PublicReadWrite);

	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text);

	// get acl
	auto aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName2, objName2));
	EXPECT_EQ(aclOutcome.isSuccess(), true);
	EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);
}

TEST_F(ObjectCopyTest, ObjectCopyExistObjectMeta)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	std::string text2 = text1 + text1;

    ObjectMetaData MetaInfo1;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-src";
    MetaInfo1.UserMetaData()["author2"] = "chanju2-src";
    
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo1));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

    ObjectMetaData MetaInfo2;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-dest";
    MetaInfo1.UserMetaData()["author3"] = "chanju3-dest";
	putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2),MetaInfo2));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag2 = putOutcome.result().ETag();

	EXPECT_EQ((etag1 != etag2), true);

	TestUtils::WaitForCacheExpire(2);

	//ObjectMetaData MetaInfoCopy;
    //MetaInfoCopy.UserMetaData()["author1"] = "chanju1-dest";
    //MetaInfoCopy.UserMetaData()["author2"] = "chanju2-dest";
    //MetaInfoCopy.UserMetaData()["author3"] = "chanju3-dest";
    
	CopyObjectRequest copyRequest(BucketName2, objName2);
	copyRequest.setCopySource(BucketName1, objName1);
	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"),"chanju1-src");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author2"),"chanju2-src");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author3"),
              getOutcome.result().Metadata().UserMetaData().end());

}

TEST_F(ObjectCopyTest, ObjectCopyExistObjectMetaCopy)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	std::string text2 = text1 + text1;

    ObjectMetaData MetaInfo1;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-src";
    MetaInfo1.UserMetaData()["author2"] = "chanju2-src";
    
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo1));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

    ObjectMetaData MetaInfo2;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-dest";
    MetaInfo1.UserMetaData()["author3"] = "chanju3-dest";
	putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2),MetaInfo2));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag2 = putOutcome.result().ETag();

	EXPECT_EQ((etag1 != etag2), true);

	TestUtils::WaitForCacheExpire(2);

	ObjectMetaData MetaInfoCopy;
    MetaInfoCopy.UserMetaData()["author1"] = "chanju1-copy";
    MetaInfoCopy.UserMetaData()["author2"] = "chanju2-copy";
    MetaInfoCopy.UserMetaData()["author3"] = "chanju3-copy";
    
	CopyObjectRequest copyRequest(BucketName2, objName2,MetaInfoCopy);
	copyRequest.setCopySource(BucketName1, objName1);
    copyRequest.setMetadataDirective(CopyActionList::Copy);
	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"),"chanju1-src");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author2"),"chanju2-src");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author3"),
                  getOutcome.result().Metadata().UserMetaData().end());
}


TEST_F(ObjectCopyTest, ObjectCopyExistObjectMetaReplace)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy1");
	std::string text1 = "hellowworld";

	std::string objName2 = TestUtils::GetObjectKey("test-cpp-sdk-objectcopy2");
	std::string text2 = text1 + text1;

    ObjectMetaData MetaInfo1;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-src";
    MetaInfo1.UserMetaData()["author2"] = "chanju2-src";
    
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo1));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

    ObjectMetaData MetaInfo2;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-dest";
    MetaInfo1.UserMetaData()["author3"] = "chanju3-dest";
	putOutcome = Client->PutObject(PutObjectRequest(BucketName2, objName2, std::make_shared<std::stringstream>(text2),MetaInfo2));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag2 = putOutcome.result().ETag();

	EXPECT_EQ((etag1 != etag2), true);

	TestUtils::WaitForCacheExpire(2);

	ObjectMetaData MetaInfoCopy;
    MetaInfoCopy.UserMetaData()["author1"] = "chanju1-copy";
    MetaInfoCopy.UserMetaData()["author2"] = "chanju2-copy";
    MetaInfoCopy.UserMetaData()["author3"] = "chanju3-copy";
    
	CopyObjectRequest copyRequest(BucketName2, objName2,MetaInfoCopy);
	copyRequest.setCopySource(BucketName1, objName1);
    copyRequest.setMetadataDirective(CopyActionList::Replace);
	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName2, objName2));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"),"chanju1-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author2"),"chanju2-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author3"),"chanju3-copy");
}


TEST_F(ObjectCopyTest, ObjectCopySameObjectMetaCopy)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-ObjectCopySameObjectMetaCopy");
	std::string text1 = "hellowworld";

    ObjectMetaData MetaInfo1;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-src";
    MetaInfo1.UserMetaData()["author2"] = "chanju2-src";
    
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo1));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	TestUtils::WaitForCacheExpire(2);

	ObjectMetaData MetaInfoCopy;
    MetaInfoCopy.UserMetaData()["author1"] = "chanju1-copy";
    MetaInfoCopy.UserMetaData()["author2"] = "chanju2-copy";
    MetaInfoCopy.UserMetaData()["author3"] = "chanju3-copy";
    
	CopyObjectRequest copyRequest(BucketName1, objName1,MetaInfoCopy);
	copyRequest.setCopySource(BucketName1, objName1);
    copyRequest.setMetadataDirective(CopyActionList::Copy);
	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName1, objName1));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"),"chanju1-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author2"),"chanju2-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author3"),"chanju3-copy");
}

TEST_F(ObjectCopyTest, ObjectCopySameObjectMetaReplace)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-ObjectCopySameObjectMetaReplace");
	std::string text1 = "hellowworld";

    ObjectMetaData MetaInfo1;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-src";
    MetaInfo1.UserMetaData()["author2"] = "chanju2-src";
    
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo1));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	TestUtils::WaitForCacheExpire(2);

	ObjectMetaData MetaInfoCopy;
    MetaInfoCopy.UserMetaData()["author1"] = "chanju1-copy";
    MetaInfoCopy.UserMetaData()["author2"] = "chanju2-copy";
    MetaInfoCopy.UserMetaData()["author3"] = "chanju3-copy";
    
	CopyObjectRequest copyRequest(BucketName1, objName1,MetaInfoCopy);
	copyRequest.setCopySource(BucketName1, objName1);
    copyRequest.setMetadataDirective(CopyActionList::Replace);
	CopyObjectOutcome  copyOutCome = Client->CopyObject(copyRequest);
	EXPECT_EQ(copyOutCome.isSuccess(), true);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName1, objName1));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"),"chanju1-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author2"),"chanju2-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author3"),"chanju3-copy");
}

TEST_F(ObjectCopyTest, ObjectCopyUpdateUserMeta1)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-ObjectCopyUpdateUserMeta1");
	std::string text1 = "hellowworld";

    ObjectMetaData MetaInfo1;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-src";
    MetaInfo1.UserMetaData()["author2"] = "chanju2-src";
    
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo1));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	TestUtils::WaitForCacheExpire(2);

	ObjectMetaData MetaInfoCopy;
    MetaInfoCopy.UserMetaData()["author1"] = "chanju1-copy";
    MetaInfoCopy.UserMetaData()["author2"] = "chanju2-copy";
    MetaInfoCopy.UserMetaData()["author3"] = "chanju3-copy";

	CopyObjectOutcome  copyOutCome = Client->ModifyObjectMeta(BucketName1, objName1, MetaInfoCopy);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName1, objName1));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"),"chanju1-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author2"),"chanju2-copy");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author3"),"chanju3-copy");
}

TEST_F(ObjectCopyTest, ObjectCopyUpdateUserMeta2)
{
	std::string objName1 = TestUtils::GetObjectKey("test-cpp-sdk-ObjectCopyUpdateUserMeta2");
	std::string text1 = "hellowworld";

    ObjectMetaData MetaInfo1;
    MetaInfo1.UserMetaData()["author1"] = "chanju1-src";
    MetaInfo1.UserMetaData()["author2"] = "chanju2-src";
    
	PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName1, objName1, std::make_shared<std::stringstream>(text1),MetaInfo1));
	EXPECT_EQ(putOutcome.isSuccess(), true);
	std::string etag1 = putOutcome.result().ETag();

	TestUtils::WaitForCacheExpire(2);

	ObjectMetaData MetaInfoCopy;  
	CopyObjectOutcome  copyOutCome = Client->ModifyObjectMeta(BucketName1, objName1, MetaInfoCopy);

	// get object
	GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName1, objName1));
	EXPECT_EQ(getOutcome.isSuccess(), true);
	std::string strData;
	(*getOutcome.result().Content().get()) >> strData;
	EXPECT_EQ(strData, text1);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author1"),
                  getOutcome.result().Metadata().UserMetaData().end());    
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author2"),
                  getOutcome.result().Metadata().UserMetaData().end());  
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author3"),
                  getOutcome.result().Metadata().UserMetaData().end());  
}



TEST_F(ObjectCopyTest, CopyObjectResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <CopyObjectResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                     <LastModified>Fri, 24 Feb 2012 07:18:48 GMT</LastModified>
                     <ETag>"5B3C1A2E053D763E1B002CC607C5A0FE"</ETag>
                    </CopyObjectResult>)";
    CopyObjectResult result(std::make_shared<std::stringstream>(xml));
    EXPECT_EQ(result.LastModified(), "Fri, 24 Feb 2012 07:18:48 GMT");
    EXPECT_EQ(result.ETag(), "5B3C1A2E053D763E1B002CC607C5A0FE");
}

TEST_F(ObjectCopyTest, CopyObjectWithSpecialKeyNameTest)
{
    //put test file
    //u8"中文名字+"
    unsigned char buff[] = { 0xE4, 0XB8, 0XAD, 0XE6, 0X96, 0X87, 0XE5, 0X90, 0X8D, 0XE5, 0XAD, 0X97, 0X2B, 0X0 };
    std::string u8_str((char *)buff);//= u8"中文名字+";
    auto testKey = u8_str;
    auto content = TestUtils::GetRandomStream(100);
    Client->PutObject(BucketName1, testKey, content);
    EXPECT_EQ(Client->DoesObjectExist(BucketName1, testKey), true);

    //get target object name
    auto targetObjectKey = TestUtils::GetObjectKey("CopyObjectWithSpecialKeyNameTest");

    CopyObjectRequest request(BucketName1, targetObjectKey);
    request.setCopySource(BucketName1, testKey);
    auto outcome = Client->CopyObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(ObjectCopyTest, CopyObjectRequestBranchTest)
{
    ObjectMetaData meta;
    meta.setContentType("test");

    CopyObjectRequest request(BucketName1,"test", meta);
    Client->CopyObject(request);

    CopyObjectResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <CopyObject xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                     <LastModified>Fri, 24 Feb 2012 07:18:48 GMT</LastModified>
                     <ETag>"5B3C1A2E053D763E1B002CC607C5A0FE"</ETag>
                    </CopyObject>)";
    CopyObjectResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <CopyObjectResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                    </CopyObjectResult>)";
    CopyObjectResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <CopyObjectResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
                     <LastModified></LastModified>
                     <ETag></ETag>
                    </CopyObjectResult>)";
    CopyObjectResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    CopyObjectResult result4(xml);
}
}
}
