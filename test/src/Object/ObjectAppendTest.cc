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
class ObjectAppendTest : public ::testing::Test
{
protected:
    ObjectAppendTest()
    {
    }

    ~ObjectAppendTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        ClientConfiguration conf;
        conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectappend");
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

std::shared_ptr<OssClient> ObjectAppendTest::Client = nullptr;
std::string ObjectAppendTest::BucketName = "";

TEST_F(ObjectAppendTest, appendDataNormalTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectappend");

    // put object
    std::string text = "hellowworld";
    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text));
    appendRequest.setExpires(TestUtils::GetGMTString(100));
    auto appendOutcome = Client->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size());

    auto testOutcome = Client->GetObject(BucketName, objName);

    AppendObjectRequest  requestOther(BucketName, objName, std::make_shared<std::stringstream>(text));
    requestOther.setPosition(text.size());
    appendOutcome = Client->AppendObject(requestOther);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size()*2);
    
    // read object
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName, objName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData ;
    (*getOutcome.result().Content().get())>>strData;
    EXPECT_EQ(strData, text+text);
}

TEST_F(ObjectAppendTest, appendDataPositionErrorTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectappend");

    // put object
    std::string text = "hellowworld";
    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text));
    auto appendOutcome = Client->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size());

    AppendObjectRequest  requestOther(BucketName, objName, std::make_shared<std::stringstream>(text));
    appendOutcome = Client->AppendObject(requestOther);
    EXPECT_EQ(appendOutcome.isSuccess(), false);
    
    // read object
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName, objName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData;
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, text);
}

TEST_F(ObjectAppendTest, appendDataNormalMeta1Test)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectappend");

    // put object
    std::string text = "helloworld";
    ObjectMetaData MetaInfo;
    MetaInfo.UserMetaData()["author1"] = "chanju1-src";

    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text), MetaInfo);
    appendRequest.setCacheControl("max-age=3");
    appendRequest.setContentDisposition("append-object-disposition");
    appendRequest.setContentEncoding("myzip");
    auto appendOutcome = Client->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size());

    // read object
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName, objName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData;
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, text);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"), "chanju1-src");
    EXPECT_EQ(getOutcome.result().Metadata().HttpMetaData().at("Cache-Control"), "max-age=3");
    EXPECT_EQ(getOutcome.result().Metadata().HttpMetaData().at("Content-Disposition"), "append-object-disposition");
    EXPECT_EQ(getOutcome.result().Metadata().HttpMetaData().at("Content-Encoding"), "myzip");
}

TEST_F(ObjectAppendTest, appendDataNormalMeta2Test)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectappend");

    // put object
    std::string text = "hellowworld";
    ObjectMetaData MetaInfo;
    MetaInfo.UserMetaData()["author1"] = "chanju1-src";
    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text), MetaInfo);
    auto appendOutcome = Client->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size());

    // read object
    GetObjectOutcome getOutcome = Client->GetObject(GetObjectRequest(BucketName, objName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    std::string strData;
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, text);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"), "chanju1-src");

    MetaInfo.UserMetaData()["author1"] = "chanju1-diffrent";
    MetaInfo.UserMetaData()["author2"] = "chanju2";
    AppendObjectRequest  requestOther(BucketName, objName, std::make_shared<std::stringstream>(text), MetaInfo);
    requestOther.setPosition(text.size());
    appendOutcome = Client->AppendObject(requestOther);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size() * 2);

    // read object
    getOutcome = Client->GetObject(GetObjectRequest(BucketName, objName));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    (*getOutcome.result().Content().get()) >> strData;
    EXPECT_EQ(strData, text + text);
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().at("author1"), "chanju1-src");
    EXPECT_EQ(getOutcome.result().Metadata().UserMetaData().find("author2"),
        getOutcome.result().Metadata().UserMetaData().end());
}


TEST_F(ObjectAppendTest, appendDataAclTest)
{
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectappend");

    // put object
    std::string text = "hellowworld";
    ObjectMetaData MetaInfo;
    MetaInfo.UserMetaData()["author1"] = "chanju1-src";
    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text), MetaInfo);
    appendRequest.setAcl(CannedAccessControlList::PublicReadWrite);
    
    auto appendOutcome = Client->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), true);
    EXPECT_EQ(appendOutcome.result().Length(), text.size());

    // get acl
    auto aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, objName));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);
}


TEST_F(ObjectAppendTest, appendNormalObjectTest)
{
    // put object
    std::string objName = TestUtils::GetObjectKey("test-cpp-sdk-objectappend");
    std::string text = "hellowworld";
    PutObjectOutcome putOutcome = Client->PutObject(PutObjectRequest(BucketName, objName, std::make_shared<std::stringstream>(text)));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // append failure
    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text));
    auto appendOutcome = Client->AppendObject(appendRequest);
    EXPECT_EQ(appendOutcome.isSuccess(), false);
}

TEST_F(ObjectAppendTest, AppendObjectResultTest)
{
    HeaderCollection header;
    AppendObjectResult result(header);
    EXPECT_EQ(result.CRC64(), 0UL);
    EXPECT_EQ(result.Length(), 0UL);
}

TEST_F(ObjectAppendTest, AppendObjectFuntionTest)
{
    std::string objName = std::string("test-cpp-sdk-objectappend");
    std::string text = "hellowworld";
    AppendObjectRequest appendRequest(BucketName, objName, std::make_shared<std::stringstream>(text));
    appendRequest.setContentMd5("test");
    appendRequest.setExpires("1");
    appendRequest.setExpires(1);

    ObjectMetaData meta;
    meta.setContentType("test");
    AppendObjectRequest appendRequest1(BucketName, objName, std::make_shared<std::stringstream>(text), meta);
    Client->AppendObject(appendRequest1);

}

}
}
