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
    for (auto const invalidBucketName : TestUtils::InvalidBucketNamesList()) {
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

    auto dOutcome = Client->DeleteBucketWebsite(DeleteBucketWebsiteRequest(BucketName));
    EXPECT_EQ(dOutcome.isSuccess(), true);

    SetBucketWebsiteRequest request1(BucketName);
    request1.setIndexDocument("index.html");
    request1.setIndexDocument("error.html");
    WebsiteRoutingRuleList websiteRoutingRule_;
    WebsiteRoutingRule routingrule;
    RoutingRuleCondition condition_;
    RoutingRuleRedirect redirect_;

    condition_.setKeyPrefixEquals("abc/");
    condition_.setHttpErrorCodeReturnedEquals("404");
    RoutingRuleIncludeHeader includeheader;
    RoutingRuleIncludeHeaderList includeheaderlist;
    includeheader.setKey("host");
    includeheader.setEquals("oss-cn-shenzhen.aliyuncs.com");
    includeheaderlist.push_back(includeheader);
    condition_.setRoutingRuleIncludeHeaderList(includeheaderlist);

    redirect_.setRedirectType(AlibabaCloud::OSS::RedirectType::Mirror);
    redirect_.setPassQueryString("true");
    redirect_.setMirrorURL("http://www.test.com/");
    redirect_.setMirrorPassQueryString("true");
    redirect_.setMirrorFollowRedirect("true");
    redirect_.setMirrorCheckMd5("true");

    RoutingRuleMirrorHeaders header;
    header.setPassAll("true");
    stringlist pass;
    stringlist remove;
    pass.push_back("myheader-key1");
    remove.push_back("myheader-key3");
    header.setPass(pass);
    header.setRemove(remove);
    MirrorHeadersSetList setlist;
    MirrorHeadersSet set;
    set.setKey("myheader-key5");
    set.setValue("myheader-value5");
    setlist.push_back(set);
    header.setMirrorHeadersSetList(setlist);
    redirect_.setMirrorHeaders(header);

    MirrorMultiAlternates alternateslist;
    MirrorMultiAlternate alternate;
    alternate.setMirrorMultiAlternateNumber("1");
    alternate.setMirrorMultiAlternateUrl("http://www.test1.com/");
    alternateslist.push_back(alternate);
    redirect_.setMirrorMultiAlternates(alternateslist);

    routingrule.setRoutingRuleCondition(condition_);
    routingrule.setRoutingRuleRedirect(redirect_);
    routingrule.setRuleNumber("1");

    websiteRoutingRule_.push_back(routingrule);

    request1.setWebsiteRoutingRuleList(websiteRoutingRule_);

    auto outcome1 = Client->SetBucketWebsite(request1);
    EXPECT_EQ(outcome1.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);

    auto gOutcome1 = Client->GetBucketWebsite(GetBucketWebsiteRequest(BucketName));

    WebsiteRoutingRule routeRule = gOutcome1.result().getWebsiteRoutingRuleList().front();
    EXPECT_EQ(routeRule.getRuleNumber(), "1");
    RoutingRuleCondition condition = routeRule.getRoutingRuleCondition();
    EXPECT_EQ(condition.getKeyPrefixEquals(), "abc/");
    EXPECT_EQ(condition.getHttpErrorCodeReturnedEquals(), "404");

    RoutingRuleIncludeHeader includeheader1 = condition.getIncludeHeaderList().front();
    EXPECT_EQ(includeheader1.getKey(), "host");
    EXPECT_EQ(includeheader1.getEquals(), "oss-cn-shenzhen.aliyuncs.com");

    RoutingRuleRedirect redirect = routeRule.getRoutingRuleRedirect();
    EXPECT_EQ(redirect.getPassQueryString(), "true");
    EXPECT_EQ(redirect.getMirrorURL(), "http://www.test.com/");
    EXPECT_EQ(redirect.getMirrorPassQueryString(), "true");
    EXPECT_EQ(redirect.getMirrorFollowRedirect(), "true");
    EXPECT_EQ(redirect.getMirrorCheckMd5(), "true");

    RoutingRuleMirrorHeaders header1 = redirect.getRoutingRuleMirrorHeaders();
    EXPECT_EQ(header1.getPassAll(), "true");
    EXPECT_EQ(header1.getPass().front(), "myheader-key1");
    EXPECT_EQ(header1.getRemove().front(), "myheader-key3");

    MirrorHeadersSet set1 = header.getMirrorHeadersSetList().front();
    EXPECT_EQ(set1.getKey(), "myheader-key5");
    EXPECT_EQ(set1.getValue(), "myheader-value5");

    MirrorMultiAlternate alternate1 = redirect.getMirrorMultiAlternates().front();
    EXPECT_EQ(alternate1.getMirrorMultiAlternateNumber(), "1");
    EXPECT_EQ(alternate1.getMirrorMultiAlternateUrl(), "http://www.test1.com/");

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

TEST_F(BucketWebsiteSettingsTest, GetBucketWebsiteResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <WebsiteConfiguration>
                        <IndexDocument>
                        <Suffix>index.html</Suffix>
                        </IndexDocument>
                        <ErrorDocument>
                        <Key>error.html</Key>
                        </ErrorDocument>
                        <RoutingRules>
                        <RoutingRule>
                        <RuleNumber>1</RuleNumber>
                        <Condition>
                        <IncludeHeader>
                        <Key>host</Key>
                        <Equals>test.oss-cn-beijing-internal.aliyuncs.com</Equals>
                        </IncludeHeader>
                        <KeyPrefixEquals>abc/</KeyPrefixEquals>
                        <HttpErrorCodeReturnedEquals>404</HttpErrorCodeReturnedEquals>
                        </Condition>
                        <Redirect>
                        <RedirectType>Mirror</RedirectType>
                        <PassQueryString>true</PassQueryString>
                        <MirrorURL>http://www.test.com/</MirrorURL>
                        <MirrorPassQueryString>true</MirrorPassQueryString>
                        <MirrorFollowRedirect>true</MirrorFollowRedirect>
                        <MirrorCheckMd5>false</MirrorCheckMd5>
                        <MirrorHeaders>
                        <PassAll>true</PassAll>
                        <Pass>myheader-key1</Pass>
                        <Pass>myheader-key2</Pass>
                        <Remove>myheader-key3</Remove>
                        <Remove>myheader-key4</Remove>
                        <Set>
                        <Key>myheader-key5</Key>
                        <Value>myheader-value5</Value>
                        </Set>
                        </MirrorHeaders>
                        <Protocol>http</Protocol>
                        <HostName>www.test.com</HostName>
                        <PassQueryString>false</PassQueryString>
                        <ReplaceKeyWith>test</ReplaceKeyWith>
                        <ReplaceKeyPrefixWith>test</ReplaceKeyPrefixWith>
                        <HttpRedirectCode>301</HttpRedirectCode>
                        <MirrorMultiAlternates>
                        <MirrorMultiAlternate>
                        <MirrorMultiAlternateNumber>test</MirrorMultiAlternateNumber>
                        <MirrorMultiAlternateURL>test</MirrorMultiAlternateURL>
                        </MirrorMultiAlternate>
                        </MirrorMultiAlternates>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result(xml);
    EXPECT_EQ(result.IndexDocument(), "index.html");
    EXPECT_EQ(result.ErrorDocument(), "error.html");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRuleNumber(), "1");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleCondition().getKeyPrefixEquals(), "abc/");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleCondition().getHttpErrorCodeReturnedEquals(), "404");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleCondition().getIncludeHeaderList()[0].getKey(), "host");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleCondition().getIncludeHeaderList()[0].getEquals(), "test.oss-cn-beijing-internal.aliyuncs.com");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getPassQueryString(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRedirectType(), AlibabaCloud::OSS::RedirectType::Mirror);
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getMirrorURL(), "http://www.test.com/");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getMirrorPassQueryString(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getMirrorFollowRedirect(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getMirrorCheckMd5(), "false");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getPassAll(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getPass()[0], "myheader-key1");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getPass()[1], "myheader-key2");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getRemove()[0], "myheader-key3");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getRemove()[1], "myheader-key4");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getMirrorHeadersSetList()[0].getKey(), "myheader-key5");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getMirrorHeadersSetList()[0].getValue(), "myheader-value5");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getProtocol(), "http");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getHostName(), "www.test.com");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getHttpRedirectCode(), "301");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getReplaceKeyPrefixWith(), "test");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getReplaceKeyWith(), "test");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getMirrorMultiAlternates()[0].getMirrorMultiAlternateNumber(), "test");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].getRoutingRuleRedirect().getMirrorMultiAlternates()[0].getMirrorMultiAlternateUrl(), "test");
}

}
}