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

namespace AlibabaCloud 
{
namespace OSS 
{

    class BucketTaggingTest : public ::testing::Test {
    protected:
        BucketTaggingTest()
        {
        }

        ~BucketTaggingTest() override
        {
        }

        // Sets up the stuff shared by all tests in this test case.
        static void SetUpTestCase()
        {
            Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
            BucketName = TestUtils::GetBucketName("cpp-sdk-buckettagging");
            Client->CreateBucket(CreateBucketRequest(BucketName));
        }

        // Tears down the stuff shared by all tests in this test case.
        static void TearDownTestCase()
        {
            Client->DeleteBucket(DeleteBucketRequest(BucketName));
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

    std::shared_ptr<OssClient> BucketTaggingTest::Client = nullptr;
    std::string BucketTaggingTest::BucketName = "";

    TEST_F(BucketTaggingTest, SetAndDeleteBucketTaggingTest)
    {
        SetBucketTaggingRequest setrequest(BucketName);
        Tag tag1("project","projectone");
        Tag tag2("user", "jsmith");
        TagSet tagset;
        tagset.push_back(tag1);
        tagset.push_back(tag2);
        Tagging taging;
        taging.setTags(tagset);
        setrequest.setTagging(taging);
        auto setoutcome = Client->SetBucketTagging(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        DeleteBucketTaggingRequest delrequest(BucketName);
        auto deloutcome = Client->DeleteBucketTagging(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketTaggingTest, GetBucketTaggingTest)
    {
        SetBucketTaggingRequest setrequest(BucketName);
        Tag tag1("project", "projectone");
        Tag tag2("user", "jsmith");
        TagSet tagset;
        tagset.push_back(tag1);
        tagset.push_back(tag2);
        Tagging taging;
        taging.setTags(tagset);
        setrequest.setTagging(taging);
        auto setoutcome = Client->SetBucketTagging(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        auto getoutcome = Client->GetBucketTagging(GetBucketTaggingRequest(BucketName));
        EXPECT_EQ(getoutcome.isSuccess(), true);
        EXPECT_EQ(getoutcome.result().Tagging().Tags().size(), 2U);
        EXPECT_TRUE(getoutcome.result().RequestId().size() > 0U);

        size_t i = 0;
        for (const auto& tag : getoutcome.result().Tagging().Tags()) {
            EXPECT_EQ(taging.Tags()[i].Key(), tag.Key());
            EXPECT_EQ(taging.Tags()[i].Value(), tag.Value());
            i++;
        }

        DeleteBucketTaggingRequest delrequest(BucketName);
        auto deloutcome = Client->DeleteBucketTagging(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketTaggingTest, GetBucketTaggingResult)
    {
        std::string xml = R"(<?xml version="1.0" ?>
                        <Tagging>
                            <TagSet>
                            <Tag>
                                <Key>project</Key>
                                <Value>projectone</Value>
                            </Tag>
                            <Tag>
                                <Key>user</Key>
                                <Value>jsmith</Value>
                            </Tag>
                            </TagSet>
                        </Tagging>)";
        GetBucketTaggingResult result1(xml);
        EXPECT_EQ(result1.Tagging().Tags().size(), 2U);
        EXPECT_EQ(result1.Tagging().Tags()[0].Key(), "project");
        EXPECT_EQ(result1.Tagging().Tags()[0].Value(), "projectone");
        EXPECT_EQ(result1.Tagging().Tags()[1].Key(), "user");
        EXPECT_EQ(result1.Tagging().Tags()[1].Value(), "jsmith");
    }

    TEST_F(BucketTaggingTest, ListBucketByTaggingTest)
    {
        SetBucketTaggingRequest setrequest(BucketName);
        Tag tag1("project", "projectone");
        TagSet tagset;
        tagset.push_back(tag1);
        Tagging taging;
        taging.setTags(tagset);
        setrequest.setTagging(taging);
        auto setoutcome = Client->SetBucketTagging(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        ListBucketsRequest listrequest;
        listrequest.setTag(tag1);
        auto listoutcome = Client->ListBuckets(listrequest);

        EXPECT_EQ(listoutcome.isSuccess(), true);
        EXPECT_EQ(listoutcome.result().Buckets().size(), 1U);
        EXPECT_EQ(listoutcome.result().Buckets()[0].Name(), BucketName);
    }

    TEST_F(BucketTaggingTest, BucketTaggingWithInvalidResponseBodyTest)
    {
        auto gbtRequest = GetBucketTaggingRequest(BucketName);
        gbtRequest.setResponseStreamFactory([=]() {
            auto content = std::make_shared<std::stringstream>();
            content->write("invlid data", 11);
            return content;
        });
        auto gbtOutcome = Client->GetBucketTagging(gbtRequest);
        EXPECT_EQ(gbtOutcome.isSuccess(), false);
        EXPECT_EQ(gbtOutcome.error().Code(), "ParseXMLError");
    }

    TEST_F(BucketTaggingTest, GetBucketTaggingResultBranchTest)
    {
        GetBucketTaggingResult result("test");

        std::string xml = R"(<?xml version="1.0" ?>
                        <Tagg>
                            <TagSet>
                            <Tag>
                                <Key>project</Key>
                                <Value>projectone</Value>
                            </Tag>
                            <Tag>
                                <Key>user</Key>
                                <Value>jsmith</Value>
                            </Tag>
                            </TagSet>
                        </Tagg>)";
        GetBucketTaggingResult result1(xml);

        xml = R"(<?xml version="1.0" ?>
                        <Tagging>
                            <Tag>
                                <Key>project</Key>
                                <Value>projectone</Value>
                            </Tag>
                            <Tag>
                                <Key>user</Key>
                                <Value>jsmith</Value>
                            </Tag>
                        </Tagging>)";
        GetBucketTaggingResult result2(xml);

        xml = R"(<?xml version="1.0" ?>
                        <Tagging>
                            <TagSet>
                            </TagSet>
                        </Tagging>)";
        GetBucketTaggingResult result3(xml);

        xml = R"(<?xml version="1.0" ?>
                        <Tagging>
                            <TagSet>
                            <Tag>
                            </Tag>
                            <Tag>

                            </Tag>
                            </TagSet>
                        </Tagging>)";
        GetBucketTaggingResult result4(xml);

        xml = R"(<?xml version="1.0" ?>
                        <Tagging>
                            <TagSet>
                            <Tag>
                                <Key></Key>
                                <Value></Value>
                            </Tag>

                            </TagSet>
                        </Tagging>)";
        GetBucketTaggingResult result5(xml);

        xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
        GetBucketTaggingResult result6(xml);
    }

    TEST_F(BucketTaggingTest, SetBucketTaggingFailTest)
    {
        SetBucketTaggingRequest setrequest("INVALIDNAME");
        auto setoutcome = Client->SetBucketTagging(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), false);


        DeleteBucketTaggingRequest delrequest("INVALIDNAME");
        auto deloutcome = Client->DeleteBucketTagging(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), false);

        auto getoutcome = Client->GetBucketTagging(GetBucketTaggingRequest("INVALIDNAME"));
        EXPECT_EQ(getoutcome.isSuccess(), false);
    }

    TEST_F(BucketTaggingTest, SetBucketTaggingKeyTest)
    {
        SetBucketTaggingRequest setrequest(BucketName);
        Tag tag1("project", "projectone");
        Tag tag2("user", "jsmith");
        TagSet tagset;
        tagset.push_back(tag1);
        tagset.push_back(tag2);
        Tagging taging;
        taging.setTags(tagset);
        setrequest.setTagging(taging);
        auto setoutcome = Client->SetBucketTagging(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        DeleteBucketTaggingRequest delrequest(BucketName);
        Tagging taging1;
        TagSet tagset1;
        tagset1.push_back(tag1);
        taging1.setTags(tagset1);
        delrequest.setTagging(taging1);
        auto deloutcome = Client->DeleteBucketTagging(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);

        auto getoutcome = Client->GetBucketTagging(GetBucketTaggingRequest(BucketName));
        EXPECT_EQ(getoutcome.isSuccess(), true);
        EXPECT_EQ(getoutcome.result().Tagging().Tags().size(), 1U);
        EXPECT_EQ(getoutcome.result().Tagging().Tags()[0].Key(), tag2.Key());

        delrequest.setTagging(taging);
        deloutcome = Client->DeleteBucketTagging(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);

        getoutcome = Client->GetBucketTagging(GetBucketTaggingRequest(BucketName));
        EXPECT_EQ(getoutcome.isSuccess(), true);
        EXPECT_EQ(getoutcome.result().Tagging().Tags().size(), 0U);
    }
}
}