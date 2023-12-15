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

namespace AlibabaCloud {
namespace OSS {

class BucketWebsiteSettingsTest : public ::testing::Test {
protected:
    BucketWebsiteSettingsTest()
    {
    }

    ~BucketWebsiteSettingsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketwebsitesettings");
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
std::shared_ptr<OssClient> BucketWebsiteSettingsTest::Client = nullptr;
std::string BucketWebsiteSettingsTest::BucketName = "";

TEST_F(BucketWebsiteSettingsTest, InvalidBucketNameTest)
{
    std::string indexDoc = "index.html";
    for (auto const& invalidBucketName : TestUtils::InvalidBucketNamesList()) {
        auto outcome = Client->SetBucketWebsite(invalidBucketName, indexDoc);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketWebsiteSettingsTest, GetBucketNotSetWebsiteTest)
{
    auto dOutcome = Client->DeleteBucketWebsite(DeleteBucketWebsiteRequest(BucketName));
    EXPECT_EQ(dOutcome.isSuccess(), true);

    auto gOutcome = Client->GetBucketWebsite(GetBucketWebsiteRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "NoSuchWebsiteConfiguration");
}

TEST_F(BucketWebsiteSettingsTest, DeleteBucketWebsiteNegativeTest)
{
    auto bucketName = TestUtils::GetBucketName("cpp-sdk-bucketwebsitesettings");
    auto outcome = Client->DeleteBucketWebsite(bucketName);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketWebsiteSettingsTest, SetBucketWebsiteTest)
{
    auto gOutcome = Client->GetBucketWebsite(GetBucketWebsiteRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "NoSuchWebsiteConfiguration");

    const std::string indexPage = "index.html";
    const std::string errorPage = "NotFound.html";
    SetBucketWebsiteRequest request(BucketName);
    request.setIndexDocument(indexPage);
    request.setErrorDocument(errorPage);
    auto outcome = Client->SetBucketWebsite(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    gOutcome = Client->GetBucketWebsite(GetBucketWebsiteRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_STREQ(gOutcome.result().IndexDocument().c_str(), indexPage.c_str());
    EXPECT_STREQ(gOutcome.result().ErrorDocument().c_str(), errorPage.c_str());

    outcome = Client->SetBucketWebsite(BucketName, "index2.html", "NotFound2.html");
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    gOutcome = Client->GetBucketWebsite(BucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().IndexDocument(), "index2.html");
    EXPECT_EQ(gOutcome.result().ErrorDocument(), "NotFound2.html");
}

TEST_F(BucketWebsiteSettingsTest, SetBucketWebsiteInvalidInputTest)
{
    SetBucketWebsiteRequest request(BucketName);
    for (auto const &invalidPageName : TestUtils::InvalidPageNamesList()) {
        request.setIndexDocument(invalidPageName);
        request.setIndexDocument(invalidPageName);
        auto outcome = Client->SetBucketWebsite(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        if (invalidPageName.empty()) {
            EXPECT_EQ(outcome.error().Message(), "Index document must not be empty.");
        }
        else {
            EXPECT_EQ(outcome.error().Message(), "Invalid index document, must be end with.html.");
        }
    }

    request.setIndexDocument("index.html");
    request.setErrorDocument(".html");
    auto outcome = Client->SetBucketWebsite(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.error().Message(), "Invalid error document, must be end with .html.");
}

TEST_F(BucketWebsiteSettingsTest, GetBucketWebsiteWithInvalidResponseBodyTest)
{
    auto sbwRequest = SetBucketWebsiteRequest(BucketName);
    sbwRequest.setIndexDocument("index.html");
    sbwRequest.setErrorDocument("NotFound.html");
    Client->SetBucketWebsite(sbwRequest);

    auto gbwRequest = GetBucketWebsiteRequest(BucketName);
    gbwRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gbwOutcome = Client->GetBucketWebsite(gbwRequest);
    EXPECT_EQ(gbwOutcome.isSuccess(), false);
    EXPECT_EQ(gbwOutcome.error().Code(), "ParseXMLError");
}

TEST_F(BucketWebsiteSettingsTest, GetBucketWebsiteResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <WebsiteConfiguration>
                          <IndexDocument>
                            <Suffix>index.html</Suffix>
                          </IndexDocument>
                          <ErrorDocument>
                            <Key>NotFound.html</Key>
                          </ErrorDocument>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result(xml);
    EXPECT_EQ(result.IndexDocument(), "index.html");
    EXPECT_EQ(result.ErrorDocument(), "NotFound.html");

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <WebsiteConfiguration>
            <IndexDocument>
                <Suffix></Suffix>
            </IndexDocument>
            <ErrorDocument>
                <Key></Key>
            </ErrorDocument>
        </WebsiteConfiguration>
        )";
    result = GetBucketWebsiteResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <WebsiteConfiguration>
            <IndexDocument>
            </IndexDocument>
            <ErrorDocument>
            </ErrorDocument>
        </WebsiteConfiguration>
        )";
    result = GetBucketWebsiteResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <WebsiteConfiguration>
        </WebsiteConfiguration>
        )";
    result = GetBucketWebsiteResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <Other>
        </Other>
        )";
    result = GetBucketWebsiteResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        )";
    result = GetBucketWebsiteResult(xml);

    xml = R"(
        invalid xml
        )";
    result = GetBucketWebsiteResult(xml);
}

}
}