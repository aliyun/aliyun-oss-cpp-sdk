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

namespace AlibabaCloud
{
namespace OSS 
{

class ObjectTaggingTest : public ::testing::Test
{
protected:
    ObjectTaggingTest()
    {
    }

    ~ObjectTaggingTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
		ClientConfiguration conf;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objecttagging");
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

std::shared_ptr<OssClient> ObjectTaggingTest::Client = nullptr;
std::string ObjectTaggingTest::BucketName = "";

TEST_F(ObjectTaggingTest, TagClassTest)
{
    Tag tag1;
    EXPECT_EQ("", tag1.Key());
    EXPECT_EQ("", tag1.Value());

    tag1.setKey("key1");
    tag1.setValue("value1");
    EXPECT_EQ("key1", tag1.Key());
    EXPECT_EQ("value1", tag1.Value());

    Tag tag2("key2", "value2");
    EXPECT_EQ("key2", tag2.Key());
    EXPECT_EQ("value2", tag2.Value());
}

TEST_F(ObjectTaggingTest, TaggingClassTest)
{
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    Tag tag("key3", "value3");
    tagging.addTag(tag);
    EXPECT_EQ(tagging.Tags().size(), 3U);
    EXPECT_EQ(tagging.Tags()[1].Key(), "key2");
    EXPECT_EQ(tagging.Tags()[1].Value(), "value2");

    TagSet tagset;
    tagset.push_back(Tag("key4", "value4"));
    tagset.push_back(Tag("key5", "value5"));
    Tagging tagging2(tagset);
    EXPECT_EQ(tagging2.Tags().size(), 2U);
    EXPECT_EQ(tagging2.Tags()[1].Key(), "key5");
    EXPECT_EQ(tagging2.Tags()[1].Value(), "value5");
    EXPECT_EQ(tagging2.toQueryParameters(), "key4=value4&key5=value5");


    tagset.push_back(Tag("key6", "value6"));
    tagset.push_back(Tag("key7", "value7"));
    tagging2.setTags(tagset);
    EXPECT_EQ(tagging2.Tags().size(), 4U);
    EXPECT_EQ(tagging2.Tags()[2].Key(), "key6");
    EXPECT_EQ(tagging2.Tags()[2].Value(), "value6");
    EXPECT_EQ(tagging2.toQueryParameters(), "key4=value4&key5=value5&key6=value6&key7=value7");

    tagset.push_back(Tag("key8", ""));
    tagset.push_back(Tag("key9", "value9"));
    tagging2.setTags(tagset);
    EXPECT_EQ(tagging2.toQueryParameters(), "key4=value4&key5=value5&key6=value6&key7=value7&key8&key9=value9");

    tagging2.clear();
    EXPECT_EQ(tagging2.Tags().size(), 0U);
    tagset.clear();
    tagset.push_back(Tag("key10=", "value 10+"));
    tagset.push_back(Tag("key11&", "value11."));
    tagging2.setTags(tagset);
    EXPECT_EQ(tagging2.Tags().size(), 2U);
    EXPECT_EQ(tagging2.toQueryParameters(), "key10%3D=value%2010%2B&key11%26=value11.");

    tagging2.addTag(Tag("", "value12."));
    EXPECT_EQ(tagging2.Tags().size(), 3U);
    EXPECT_EQ(tagging2.toQueryParameters(), "key10%3D=value%2010%2B&key11%26=value11.");
}

TEST_F(ObjectTaggingTest, GetObjectTaggingResultTest)
{

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <Tagging>
                          <TagSet/>
                          </TagSet>
                        </Tagging>)";

    GetObjectTaggingResult result(xml);
    EXPECT_EQ(result.Tagging().Tags().size(), 0U);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
            <Tagging>
                <TagSet>
                <Tag>
                    <Key>a</Key>
                    <Value>1</Value>
                </Tag>
                <Tag>
                    <Key>b</Key>
                    <Value>2</Value>
                </Tag>
                </TagSet>
            </Tagging>)";

    GetObjectTaggingResult result1(xml);
    EXPECT_EQ(result1.Tagging().Tags().size(), 2U);
    EXPECT_EQ(result1.Tagging().Tags()[0].Key(), "a");
    EXPECT_EQ(result1.Tagging().Tags()[0].Value(), "1");
    EXPECT_EQ(result1.Tagging().Tags()[1].Key(), "b");
    EXPECT_EQ(result1.Tagging().Tags()[1].Value(), "2");

    GetObjectTaggingResult result2;
    result2 = xml;
    EXPECT_EQ(result2.Tagging().Tags().size(), 2U);
    EXPECT_EQ(result2.Tagging().Tags()[0].Key(), "a");
    EXPECT_EQ(result2.Tagging().Tags()[0].Value(), "1");
    EXPECT_EQ(result2.Tagging().Tags()[1].Key(), "b");
    EXPECT_EQ(result2.Tagging().Tags()[1].Value(), "2");
}

TEST_F(ObjectTaggingTest, LifecycleRuleClassTest)
{
    LifecycleRule rule1;
    LifecycleRule rule2;
    TagSet tagset;
    tagset.push_back(Tag("key1", "value1"));
    tagset.push_back(Tag("key2", "value2"));

    rule1.setTags(tagset);
    EXPECT_EQ(rule1.Tags().size(), 2U);
    rule1.addTag(Tag("key3", "value3"));
    EXPECT_EQ(rule1.Tags().size(), 3U);
    EXPECT_EQ(rule1.Tags()[0].Key(), "key1");
    EXPECT_EQ(rule1.Tags()[1].Key(), "key2");
    EXPECT_EQ(rule1.Tags()[2].Key(), "key3");

    EXPECT_FALSE(rule1 == rule2);

    rule2.addTag(Tag("key1", "value1"));
    rule2.addTag(Tag("key2", "value2"));
    rule2.addTag(Tag("key3", "value3"));
    EXPECT_TRUE(rule1 == rule2);

    rule2.setTags(tagset);
    rule2.addTag(Tag("key4", "value3"));
    EXPECT_FALSE(rule1 == rule2);

    rule2.setTags(tagset);
    rule2.addTag(Tag("key3", "value4"));
    EXPECT_FALSE(rule1 == rule2);
}

TEST_F(ObjectTaggingTest, ObjectTaggingBasicTest)
{
    auto key = TestUtils::GetObjectKey("ObjectTaggingBasicTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    auto putTaggingOutcome = Client->SetObjectTagging(SetObjectTaggingRequest(BucketName, key, tagging));
    EXPECT_EQ(putTaggingOutcome.isSuccess(), true);
    EXPECT_TRUE(putTaggingOutcome.result().RequestId().size() > 0U);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);
    EXPECT_TRUE(getTaggingOutcome.result().RequestId().size() > 0U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }

    auto delTaggingOutcome = Client->DeleteObjectTagging(DeleteObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(delTaggingOutcome.isSuccess(), true);
    EXPECT_TRUE(delTaggingOutcome.result().RequestId().size() > 0U);


    getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 0U);
}

TEST_F(ObjectTaggingTest, ObjectTaggingNegativeTest)
{
    std::string key = "invalid-key";
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));

    auto putTaggingOutcome = Client->SetObjectTagging(SetObjectTaggingRequest("cpp-sdk-test-invalid-bucket", key, tagging));
    EXPECT_EQ(putTaggingOutcome.isSuccess(), false);
    EXPECT_TRUE(putTaggingOutcome.error().RequestId().size() > 0U);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), false);
    EXPECT_TRUE(getTaggingOutcome.error().RequestId().size() > 0U);

    auto delTaggingOutcome = Client->DeleteObjectTagging(DeleteObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(delTaggingOutcome.isSuccess(), false);
    EXPECT_TRUE(delTaggingOutcome.error().RequestId().size() > 0U);
}

TEST_F(ObjectTaggingTest, SetObjectWithSpecialCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("SetObjectWithSpecialCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1=", "value 1+"));
    tagging.addTag(Tag("key2-", "value2."));
    tagging.addTag(Tag("key3:/", ""));

    auto putTaggingOutcome = Client->SetObjectTagging(SetObjectTaggingRequest(BucketName, key, tagging));
    EXPECT_EQ(putTaggingOutcome.isSuccess(), true);
    EXPECT_TRUE(putTaggingOutcome.result().RequestId().size() > 0);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, SetObjectWithGreaterThan10TagsTest)
{
    auto key = TestUtils::GetObjectKey("SetObjectWithGreaterThan10TagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    tagging.addTag(Tag("key3", "value3"));
    tagging.addTag(Tag("key4", "value4"));
    tagging.addTag(Tag("key5", "value5"));
    tagging.addTag(Tag("key6", "value6"));
    tagging.addTag(Tag("key7", "value7"));
    tagging.addTag(Tag("key8", "value8"));
    tagging.addTag(Tag("key9", "value9"));
    tagging.addTag(Tag("key10", "value10"));
    tagging.addTag(Tag("key11", "value11"));

    auto putTaggingOutcome = Client->SetObjectTagging(SetObjectTaggingRequest(BucketName, key, tagging));
    EXPECT_EQ(putTaggingOutcome.isSuccess(), false);
    EXPECT_EQ(putTaggingOutcome.error().Code(), "ValidateError");
    EXPECT_TRUE(putTaggingOutcome.error().Message().find("Object tags cannot be greater than 10") != std::string::npos);
}

TEST_F(ObjectTaggingTest, SetObjectWithDuplicatedTagsTest)
{
    auto key = TestUtils::GetObjectKey("SetObjectWithDuplicatedTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key1", "value2"));

    auto putTaggingOutcome = Client->SetObjectTagging(SetObjectTaggingRequest(BucketName, key, tagging));
    EXPECT_EQ(putTaggingOutcome.isSuccess(), false);
    EXPECT_EQ(putTaggingOutcome.error().Code(), "InvalidTag");
    EXPECT_TRUE(putTaggingOutcome.error().Message().find("Cannot provide multiple Tags with the same key") != std::string::npos);
}

TEST_F(ObjectTaggingTest, SetObjectWithUnsupportTagsTest)
{
    auto key = TestUtils::GetObjectKey("SetObjectWithUnsupportTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    Tagging tagging;
    tagging.addTag(Tag("key1&", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    auto putTaggingOutcome = Client->SetObjectTagging(SetObjectTaggingRequest(BucketName, key, tagging));
    EXPECT_EQ(putTaggingOutcome.isSuccess(), false);
    EXPECT_EQ(putTaggingOutcome.error().Code(), "InvalidTag");
    EXPECT_TRUE(putTaggingOutcome.error().Message().find("The TagKey you have provided is invalid") != std::string::npos);
}


TEST_F(ObjectTaggingTest, PutObjectWithNormalCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("PutObjectWithNormalCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, PutObjectWithSpecialCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("PutObjectWithSpecialCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1=", "value 1+"));
    tagging.addTag(Tag("key2-", "value2."));
    tagging.addTag(Tag("key3:/", ""));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, PutObjectWithGreaterThan10TagsTest)
{
    auto key = TestUtils::GetObjectKey("PutObjectWithGreaterThan10TagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    tagging.addTag(Tag("key3", "value3"));
    tagging.addTag(Tag("key4", "value4"));
    tagging.addTag(Tag("key5", "value5"));
    tagging.addTag(Tag("key6", "value6"));
    tagging.addTag(Tag("key7", "value7"));
    tagging.addTag(Tag("key8", "value8"));
    tagging.addTag(Tag("key9", "value9"));
    tagging.addTag(Tag("key10", "value10"));
    tagging.addTag(Tag("key11", "value11"));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "BadRequest");
    EXPECT_TRUE(outcome.error().Message().find("Object tags cannot be greater than 10") != std::string::npos);
}

TEST_F(ObjectTaggingTest, PutObjectWithDuplicatedTagsTest)
{
    auto key = TestUtils::GetObjectKey("PutObjectWithDuplicatedTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key1", "value2"));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidArgument");
    EXPECT_TRUE(outcome.error().Message().find("query parameters without tag name duplicates") != std::string::npos);
}

TEST_F(ObjectTaggingTest, PutObjectWithUnsupportTagsTest)
{
    auto key = TestUtils::GetObjectKey("PutObjectWithUnsupportTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1&", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidTag");
    EXPECT_TRUE(outcome.error().Message().find("The TagKey you have provided is invalid") != std::string::npos);
}

TEST_F(ObjectTaggingTest, MultipartUploadWithNormalCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("MultipartUploadWithNormalCharTagsTest");
    Tagging tagging;
    tagging.addTag(Tag("key1=", "value 1+"));
    tagging.addTag(Tag("key2-", "value2."));
    tagging.addTag(Tag("key3:/", ""));

    InitiateMultipartUploadRequest request(BucketName, key);
    request.setTagging(tagging.toQueryParameters());
    auto initOutcome = Client->InitiateMultipartUpload(request);
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_FALSE(initOutcome.result().RequestId().empty());

    PartList partETags;
    auto content = TestUtils::GetRandomStream(1024);
    UploadPartRequest uRequest(BucketName, key, content);
    uRequest.setPartNumber(1);
    uRequest.setUploadId(initOutcome.result().UploadId());
    auto uploadPartOutcome = Client->UploadPart(uRequest);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    EXPECT_FALSE(uploadPartOutcome.result().RequestId().empty());
    Part part(1, uploadPartOutcome.result().ETag());
    partETags.push_back(part);

    CompleteMultipartUploadRequest completeRequest(BucketName, key, partETags);
    completeRequest.setUploadId(initOutcome.result().UploadId());
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, ResumableUploadWithNormalCharTagsByPutObjectTest)
{
    auto key = TestUtils::GetObjectKey("ResumableUploadWithNormalCharTagsByPutObjectTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadWithNormalCharTagsByPutObjectTest");
    TestUtils::WriteRandomDatatoFile(file, 102400);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(204800U);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, ResumableUploadWithNormalCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("ResumableUploadWithNormalCharTagsTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadWithNormalCharTagsTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(102400U);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, ResumableUploadWithSpecialCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("ResumableUploadWithSpecialCharTagsTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadWithSpecialCharTagsTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);

    Tagging tagging;
    tagging.addTag(Tag("key1=", "value 1+"));
    tagging.addTag(Tag("key2-", "value2."));
    tagging.addTag(Tag("key3:/", ""));

    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(102400U);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, ResumableUploadWithGreaterThan10TagsTest)
{
    auto key = TestUtils::GetObjectKey("ResumableUploadWithGreaterThan10TagsTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadWithSpecialCharTagsTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    tagging.addTag(Tag("key3", "value3"));
    tagging.addTag(Tag("key4", "value4"));
    tagging.addTag(Tag("key5", "value5"));
    tagging.addTag(Tag("key6", "value6"));
    tagging.addTag(Tag("key7", "value7"));
    tagging.addTag(Tag("key8", "value8"));
    tagging.addTag(Tag("key9", "value9"));
    tagging.addTag(Tag("key10", "value10"));
    tagging.addTag(Tag("key11", "value11"));

    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(102400U);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "BadRequest");
    EXPECT_TRUE(outcome.error().Message().find("Object tags cannot be greater than 10") != std::string::npos);
}

TEST_F(ObjectTaggingTest, ResumableUploadWithDuplicatedTagsTest)
{
    auto key = TestUtils::GetObjectKey("ResumableUploadWithDuplicatedTagsTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadWithDuplicatedTagsTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key1", "value2"));

    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(102400U);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidArgument");
    EXPECT_TRUE(outcome.error().Message().find("query parameters without tag name duplicates") != std::string::npos);
}

TEST_F(ObjectTaggingTest, ResumableUploadWithUnsupportTagsTest)
{
    auto key = TestUtils::GetObjectKey("ResumableUploadWithUnsupportTagsTest");
    std::string file = TestUtils::GetTargetFileName("ResumableUploadWithUnsupportTagsTest");
    TestUtils::WriteRandomDatatoFile(file, 204800);
    Tagging tagging;
    tagging.addTag(Tag("key1&", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    UploadObjectRequest request(BucketName, key, file);
    request.setPartSize(102400U);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidTag");
    EXPECT_TRUE(outcome.error().Message().find("The TagKey you have provided is invalid") != std::string::npos);
}

TEST_F(ObjectTaggingTest, AppendObjectWithNormalCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("AppendObjectWithNormalCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    AppendObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->AppendObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, AppendObjectWithSpecialCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("AppendObjectWithSpecialCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1=", "value 1+"));
    tagging.addTag(Tag("key2-", "value2."));
    tagging.addTag(Tag("key3:/", ""));

    AppendObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->AppendObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
}

TEST_F(ObjectTaggingTest, AppendObjectWithGreaterThan10TagsTest)
{
    auto key = TestUtils::GetObjectKey("AppendObjectWithGreaterThan10TagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    tagging.addTag(Tag("key3", "value3"));
    tagging.addTag(Tag("key4", "value4"));
    tagging.addTag(Tag("key5", "value5"));
    tagging.addTag(Tag("key6", "value6"));
    tagging.addTag(Tag("key7", "value7"));
    tagging.addTag(Tag("key8", "value8"));
    tagging.addTag(Tag("key9", "value9"));
    tagging.addTag(Tag("key10", "value10"));
    tagging.addTag(Tag("key11", "value11"));

    AppendObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->AppendObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "BadRequest");
    EXPECT_TRUE(outcome.error().Message().find("Object tags cannot be greater than 10") != std::string::npos);
}

TEST_F(ObjectTaggingTest, AppendObjectWithDuplicatedTagsTest)
{
    auto key = TestUtils::GetObjectKey("AppendObjectWithDuplicatedTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key1", "value2"));

    AppendObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->AppendObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidArgument");
    EXPECT_TRUE(outcome.error().Message().find("query parameters without tag name duplicates") != std::string::npos);
}

TEST_F(ObjectTaggingTest, AppendObjectWithUnsupportTagsTest)
{
    auto key = TestUtils::GetObjectKey("AppendObjectWithUnsupportTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1&", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    AppendObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->AppendObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidTag");
    EXPECT_TRUE(outcome.error().Message().find("The TagKey you have provided is invalid") != std::string::npos);
}

TEST_F(ObjectTaggingTest, CopyObjectWithDefaultTest)
{
    auto key = TestUtils::GetObjectKey("CopyObjectWithDefaultTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto key_cp = TestUtils::GetObjectKey("CopyObjectWithDefaultTest-COPY");
    CopyObjectRequest cpRequest(BucketName, key_cp);
    cpRequest.setCopySource(BucketName, key);
    auto cpOutcome = Client->CopyObject(cpRequest);
    EXPECT_EQ(cpOutcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key_cp));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }

    EXPECT_EQ(outcome.result().ETag(), Client->HeadObject(BucketName, key_cp).result().ETag());
}

TEST_F(ObjectTaggingTest, CopyObjectWithNormalCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("CopyObjectWithNormalCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    Tagging tagging2;
    tagging2.addTag(Tag("key1-2", "value1-2"));
    tagging2.addTag(Tag("key2-2", "value2-2"));
    tagging2.addTag(Tag("key3-2", "value3-2"));

    //replace
    auto key_cp = TestUtils::GetObjectKey("CopyObjectWithNormalCharTagsTest-COPY");
    CopyObjectRequest cpRequest(BucketName, key_cp);
    cpRequest.setCopySource(BucketName, key);
    cpRequest.setTagging(tagging2.toQueryParameters());
    cpRequest.setTaggingDirective(CopyActionList::Replace);
    auto cpOutcome = Client->CopyObject(cpRequest);
    EXPECT_EQ(cpOutcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key_cp));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging2.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging2.Tags()[i].Value(), tag.Value());
        i++;
    }
    EXPECT_EQ(outcome.result().ETag(), Client->HeadObject(BucketName, key_cp).result().ETag());

    //copy
    cpRequest.setCopySource(BucketName, key);
    cpRequest.setTagging(tagging2.toQueryParameters());
    cpRequest.setTaggingDirective(CopyActionList::Copy);
    cpOutcome = Client->CopyObject(cpRequest);
    EXPECT_EQ(cpOutcome.isSuccess(), true);

    getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key_cp));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);

    i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
    EXPECT_EQ(outcome.result().ETag(), Client->HeadObject(BucketName, key_cp).result().ETag());
}

TEST_F(ObjectTaggingTest, CopyObjectWithSpecialCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("CopyObjectWithSpecialCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto outcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(outcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1=", "value 1+"));
    tagging.addTag(Tag("key2-", "value2."));
    tagging.addTag(Tag("key3:/", ""));

    //replace
    auto key_cp = TestUtils::GetObjectKey("CopyObjectWithSpecialCharTagsTest-COPY");
    CopyObjectRequest cpRequest(BucketName, key_cp);
    cpRequest.setCopySource(BucketName, key);
    cpRequest.setTagging(tagging.toQueryParameters());
    cpRequest.setTaggingDirective(CopyActionList::Replace);
    auto cpOutcome = Client->CopyObject(cpRequest);
    EXPECT_EQ(cpOutcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key_cp));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
    EXPECT_EQ(outcome.result().ETag(), Client->HeadObject(BucketName, key_cp).result().ETag());
}

TEST_F(ObjectTaggingTest, CopyObjectWithGreaterThan10TagsTest)
{
    auto key = TestUtils::GetObjectKey("CopyObjectWithGreaterThan10TagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    tagging.addTag(Tag("key3", "value3"));
    tagging.addTag(Tag("key4", "value4"));
    tagging.addTag(Tag("key5", "value5"));
    tagging.addTag(Tag("key6", "value6"));
    tagging.addTag(Tag("key7", "value7"));
    tagging.addTag(Tag("key8", "value8"));
    tagging.addTag(Tag("key9", "value9"));
    tagging.addTag(Tag("key10", "value10"));
    tagging.addTag(Tag("key11", "value11"));

    //replace
    auto key_cp = TestUtils::GetObjectKey("CopyObjectWithGreaterThan10TagsTest-COPY");
    CopyObjectRequest cpRequest(BucketName, key_cp);
    cpRequest.setCopySource(BucketName, key);
    cpRequest.setTagging(tagging.toQueryParameters());
    cpRequest.setTaggingDirective(CopyActionList::Replace);
    auto outcome = Client->CopyObject(cpRequest);    
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "BadRequest");
    EXPECT_TRUE(outcome.error().Message().find("Object tags cannot be greater than 10") != std::string::npos);
}

TEST_F(ObjectTaggingTest, CopyObjectWithDuplicatedTagsTest)
{
    auto key = TestUtils::GetObjectKey("CopyObjectWithDuplicatedTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key1", "value2"));

    //replace
    auto key_cp = TestUtils::GetObjectKey("CopyObjectWithDuplicatedTagsTest-COPY");
    CopyObjectRequest cpRequest(BucketName, key_cp);
    cpRequest.setCopySource(BucketName, key);
    cpRequest.setTagging(tagging.toQueryParameters());
    cpRequest.setTaggingDirective(CopyActionList::Replace);
    auto outcome = Client->CopyObject(cpRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidArgument");
    EXPECT_TRUE(outcome.error().Message().find("query parameters without tag name duplicates") != std::string::npos);
}

TEST_F(ObjectTaggingTest, CopyObjectWithUnsupportTagsTest)
{
    auto key = TestUtils::GetObjectKey("CopyObjectWithUnsupportTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1&", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    //replace
    auto key_cp = TestUtils::GetObjectKey("CopyObjectWithUnsupportTagsTest-COPY");
    CopyObjectRequest cpRequest(BucketName, key_cp);
    cpRequest.setCopySource(BucketName, key);
    cpRequest.setTagging(tagging.toQueryParameters());
    cpRequest.setTaggingDirective(CopyActionList::Replace);
    auto outcome = Client->CopyObject(cpRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidTag");
    EXPECT_TRUE(outcome.error().Message().find("The TagKey you have provided is invalid") != std::string::npos);
}

TEST_F(ObjectTaggingTest, CreateSymlinkWithDefaultTest)
{
    auto key = TestUtils::GetObjectKey("CreateSymlinkWithDefaultTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    PutObjectRequest request(BucketName, key, content);
    request.setTagging(tagging.toQueryParameters());
    auto outcome = Client->PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto key_ln = TestUtils::GetObjectKey("CreateSymlinkWithDefaultTest-LINK");
    CreateSymlinkRequest csRequest(BucketName, key_ln);
    csRequest.SetSymlinkTarget(key);
    auto csOutcome = Client->CreateSymlink(csRequest);
    EXPECT_EQ(csOutcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key_ln));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 0U);

    EXPECT_EQ(outcome.result().ETag(), Client->HeadObject(BucketName, key_ln).result().ETag());
}

TEST_F(ObjectTaggingTest, CreateSymlinkWithNormalCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("CreateSymlinkWithNormalCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto outcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(outcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    auto key_ln = TestUtils::GetObjectKey("CreateSymlinkWithNormalCharTagsTest-LINK");
    CreateSymlinkRequest csRequest(BucketName, key_ln);
    csRequest.SetSymlinkTarget(key);
    csRequest.setTagging(tagging.toQueryParameters());
    auto csOutcome = Client->CreateSymlink(csRequest);
    EXPECT_EQ(csOutcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key_ln));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 2U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }
    EXPECT_EQ(outcome.result().ETag(), Client->HeadObject(BucketName, key_ln).result().ETag());
}

TEST_F(ObjectTaggingTest, CreateSymlinkWithSpecialCharTagsTest)
{
    auto key = TestUtils::GetObjectKey("CreateSymlinkWithSpecialCharTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto outcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(outcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1=", "value 1+"));
    tagging.addTag(Tag("key2-", "value2."));
    tagging.addTag(Tag("key3:/", ""));

    auto key_ln = TestUtils::GetObjectKey("CreateSymlinkWithSpecialCharTagsTest-LINK");
    CreateSymlinkRequest csRequest(BucketName, key_ln);
    csRequest.SetSymlinkTarget(key);
    csRequest.setTagging(tagging.toQueryParameters());
    auto csOutcome = Client->CreateSymlink(csRequest);
    EXPECT_EQ(csOutcome.isSuccess(), true);

    auto getTaggingOutcome = Client->GetObjectTagging(GetObjectTaggingRequest(BucketName, key_ln));
    EXPECT_EQ(getTaggingOutcome.isSuccess(), true);
    EXPECT_EQ(getTaggingOutcome.result().Tagging().Tags().size(), 3U);

    size_t i = 0;
    for (const auto& tag : getTaggingOutcome.result().Tagging().Tags()) {
        EXPECT_EQ(tagging.Tags()[i].Key(), tag.Key());
        EXPECT_EQ(tagging.Tags()[i].Value(), tag.Value());
        i++;
    }

    EXPECT_EQ(outcome.result().ETag(), Client->HeadObject(BucketName, key_ln).result().ETag());
}

TEST_F(ObjectTaggingTest, CreateSymlinkWithGreaterThan10TagsTest)
{
    auto key = TestUtils::GetObjectKey("CreateSymlinkWithGreaterThan10TagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));
    tagging.addTag(Tag("key3", "value3"));
    tagging.addTag(Tag("key4", "value4"));
    tagging.addTag(Tag("key5", "value5"));
    tagging.addTag(Tag("key6", "value6"));
    tagging.addTag(Tag("key7", "value7"));
    tagging.addTag(Tag("key8", "value8"));
    tagging.addTag(Tag("key9", "value9"));
    tagging.addTag(Tag("key10", "value10"));
    tagging.addTag(Tag("key11", "value11"));

    auto key_ln = TestUtils::GetObjectKey("CreateSymlinkWithGreaterThan10TagsTest-LINK");
    CreateSymlinkRequest csRequest(BucketName, key_ln);
    csRequest.SetSymlinkTarget(key);
    csRequest.setTagging(tagging.toQueryParameters());
    auto outcome = Client->CreateSymlink(csRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "BadRequest");
    EXPECT_TRUE(outcome.error().Message().find("Object tags cannot be greater than 10") != std::string::npos);
}

TEST_F(ObjectTaggingTest, CreateSymlinkWithDuplicatedTagsTest)
{
    auto key = TestUtils::GetObjectKey("CreateSymlinkWithDuplicatedTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key1", "value2"));

    auto key_ln = TestUtils::GetObjectKey("CreateSymlinkWithDuplicatedTagsTest-LINK");
    CreateSymlinkRequest csRequest(BucketName, key_ln);
    csRequest.SetSymlinkTarget(key);
    csRequest.setTagging(tagging.toQueryParameters());
    auto outcome = Client->CreateSymlink(csRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidArgument");
    EXPECT_TRUE(outcome.error().Message().find("query parameters without tag name duplicates") != std::string::npos);
}

TEST_F(ObjectTaggingTest, CreateSymlinkWithUnsupportTagsTest)
{
    auto key = TestUtils::GetObjectKey("CreateSymlinkWithDuplicatedTagsTest");
    auto content = std::make_shared<std::stringstream>("Tagging Test");

    auto putOutcome = Client->PutObject(PutObjectRequest(BucketName, key, content));
    EXPECT_EQ(putOutcome.isSuccess(), true);

    Tagging tagging;
    tagging.addTag(Tag("key1&", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    auto key_ln = TestUtils::GetObjectKey("CreateSymlinkWithDuplicatedTagsTest-LINK");
    CreateSymlinkRequest csRequest(BucketName, key_ln);
    csRequest.SetSymlinkTarget(key);
    csRequest.setTagging(tagging.toQueryParameters());
    auto outcome = Client->CreateSymlink(csRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidTag");
    EXPECT_TRUE(outcome.error().Message().find("The TagKey you have provided is invalid") != std::string::npos);
}

TEST_F(ObjectTaggingTest, LifecycleNormalCharTagsTest)
{
    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-001");
    rule.setPrefix("test");
    rule.addTag(Tag("key1", "value1"));
    rule.addTag(Tag("key2", "value2"));
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);

    SetBucketLifecycleRequest request(BucketName);
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    auto gOutcome = Client->GetBucketLifecycle(BucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_TRUE(*(gOutcome.result().LifecycleRules().begin()) == rule);
}


TEST_F(ObjectTaggingTest, LifecycleWithSpecialCharTagsTest)
{
    TagSet tagSet;
    tagSet.push_back(Tag("key1=", "value 1+"));
    tagSet.push_back(Tag("key2-", "value2."));
    tagSet.push_back(Tag("key3:/", ""));

    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-001");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);
    rule.setTags(tagSet);

    SetBucketLifecycleRequest request(BucketName);
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    auto gOutcome = Client->GetBucketLifecycle(BucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_TRUE(*(gOutcome.result().LifecycleRules().begin()) == rule);
}

TEST_F(ObjectTaggingTest, LifecycleWithGreaterThan10TagsTest)
{
    TagSet tagSet;
    tagSet.push_back(Tag("key1", "value1"));
    tagSet.push_back(Tag("key2", "value2"));
    tagSet.push_back(Tag("key3", "value3"));
    tagSet.push_back(Tag("key4", "value4"));
    tagSet.push_back(Tag("key5", "value5"));
    tagSet.push_back(Tag("key6", "value6"));
    tagSet.push_back(Tag("key7", "value7"));
    tagSet.push_back(Tag("key8", "value8"));
    tagSet.push_back(Tag("key9", "value9"));
    tagSet.push_back(Tag("key10", "value10"));
    tagSet.push_back(Tag("key11", "value11"));

    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-001");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);
    rule.setTags(tagSet);

    SetBucketLifecycleRequest request(BucketName);
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    auto gOutcome = Client->GetBucketLifecycle(BucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_TRUE(*(gOutcome.result().LifecycleRules().begin()) == rule);
}

TEST_F(ObjectTaggingTest, LifecycleWithDuplicatedTagsTest)
{
    TagSet tagSet;
    tagSet.push_back(Tag("key1", "value1"));
    tagSet.push_back(Tag("key1", "value2"));

    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-001");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);
    rule.setTags(tagSet);

    SetBucketLifecycleRequest request(BucketName);
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "MalformedXML");
}

TEST_F(ObjectTaggingTest, LifecycleWithUnsupportTagsTest)
{
    TagSet tagSet;
    tagSet.push_back(Tag("key1&", "value1"));
    tagSet.push_back(Tag("key2", "value2"));

    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-001");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);
    rule.setTags(tagSet);

    SetBucketLifecycleRequest request(BucketName);
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "MalformedXML");
}

}
}
