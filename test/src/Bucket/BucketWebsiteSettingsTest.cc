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
}

TEST_F(BucketWebsiteSettingsTest, SetBucketWebsiteTest2)
{
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
    EXPECT_EQ(routeRule.RuleNumber(), "1");
    RoutingRuleCondition condition = routeRule.RoutingRuleCondition();
    EXPECT_EQ(condition.KeyPrefixEquals(), "abc/");
    EXPECT_EQ(condition.HttpErrorCodeReturnedEquals(), "404");

    RoutingRuleIncludeHeader includeheader1 = condition.IncludeHeaderList().front();
    EXPECT_EQ(includeheader1.Key(), "host");
    EXPECT_EQ(includeheader1.Equals(), "oss-cn-shenzhen.aliyuncs.com");

    RoutingRuleRedirect redirect = routeRule.RoutingRuleRedirect();
    EXPECT_EQ(redirect.PassQueryString(), "true");
    EXPECT_EQ(redirect.MirrorURL(), "http://www.test.com/");
    EXPECT_EQ(redirect.MirrorPassQueryString(), "true");
    EXPECT_EQ(redirect.MirrorFollowRedirect(), "true");
    EXPECT_EQ(redirect.MirrorCheckMd5(), "true");

    RoutingRuleMirrorHeaders header1 = redirect.RoutingRuleMirrorHeaders();
    EXPECT_EQ(header1.PassAll(), "true");
    EXPECT_EQ(header1.Pass().front(), "myheader-key1");
    EXPECT_EQ(header1.Remove().front(), "myheader-key3");

    MirrorHeadersSet set1 = header.MirrorHeadersSetList().front();
    EXPECT_EQ(set1.Key(), "myheader-key5");
    EXPECT_EQ(set1.Value(), "myheader-value5");

    MirrorMultiAlternate alternate1 = redirect.MirrorMultiAlternates().front();
    EXPECT_EQ(alternate1.MirrorMultiAlternateNumber(), "1");
    EXPECT_EQ(alternate1.MirrorMultiAlternateUrl(), "http://www.test1.com/");

    auto dOutcome = Client->DeleteBucketWebsite(DeleteBucketWebsiteRequest(BucketName));
    EXPECT_EQ(dOutcome.isSuccess(), true);

    redirect_.setRedirectType(AlibabaCloud::OSS::RedirectType::External);
    routingrule.setRoutingRuleRedirect(redirect_);
    WebsiteRoutingRuleList websiteRoutingRule1;
    websiteRoutingRule1.push_back(routingrule);

    request1.setWebsiteRoutingRuleList(websiteRoutingRule_);

    outcome1 = Client->SetBucketWebsite(request1);
    EXPECT_EQ(outcome1.isSuccess(), true);

    WebsiteRoutingRuleList websiteRoutingRule2;
    WebsiteRoutingRule routingrule2;
    websiteRoutingRule2.push_back(routingrule2);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule2);

    outcome1 = Client->SetBucketWebsite(request1);

    redirect_.setRedirectType(AlibabaCloud::OSS::RedirectType::AliCDN);
    redirect_.setHostName("test");
    redirect_.setProtocol("test");
    redirect_.setHttpRedirectCode("test");
    redirect_.setReplaceKeyPrefixWith("test");
    redirect_.setReplaceKeyWith("test");
    routingrule.setRoutingRuleRedirect(redirect_);

    WebsiteRoutingRuleList websiteRoutingRule3;
    websiteRoutingRule3.push_back(routingrule);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule3);

    outcome1 = Client->SetBucketWebsite(request1);

    RoutingRuleMirrorHeaders header2;
    header2.setPassAll("true");
    redirect_.setMirrorHeaders(header2);
    redirect_.setRedirectType(AlibabaCloud::OSS::RedirectType::Mirror);
    routingrule.setRoutingRuleRedirect(redirect_);
    WebsiteRoutingRuleList websiteRoutingRule4;
    websiteRoutingRule4.push_back(routingrule);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule4);

    outcome1 = Client->SetBucketWebsite(request1);

    RoutingRuleMirrorHeaders header3;
    stringlist pass3;
    pass3.push_back("myheader-key1");
    header3.setPass(pass3);
    redirect_.setMirrorHeaders(header3);
    redirect_.setRedirectType(AlibabaCloud::OSS::RedirectType::Mirror);
    routingrule.setRoutingRuleRedirect(redirect_);
    WebsiteRoutingRuleList websiteRoutingRule5;
    websiteRoutingRule5.push_back(routingrule);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule5);

    outcome1 = Client->SetBucketWebsite(request1);

    RoutingRuleCondition condition2;
    condition2.setHttpErrorCodeReturnedEquals("404");
    routingrule.setRoutingRuleCondition(condition2);
    WebsiteRoutingRuleList websiteRoutingRule6;
    websiteRoutingRule6.push_back(routingrule);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule6);

    outcome1 = Client->SetBucketWebsite(request1);

    RoutingRuleCondition condition3;
    condition3.setKeyPrefixEquals("abc/");
    routingrule.setRoutingRuleCondition(condition3);
    WebsiteRoutingRuleList websiteRoutingRule7;
    websiteRoutingRule7.push_back(routingrule);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule7);

    outcome1 = Client->SetBucketWebsite(request1);

    RoutingRuleRedirect redirect2;
    redirect2.setRedirectType(AlibabaCloud::OSS::RedirectType::Internal);

    routingrule.setRoutingRuleRedirect(redirect2);

    WebsiteRoutingRuleList websiteRoutingRule8;
    websiteRoutingRule8.push_back(routingrule);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule8);

    outcome1 = Client->SetBucketWebsite(request1);

    RoutingRuleRedirect redirect3;
    redirect3.setRedirectType(AlibabaCloud::OSS::RedirectType::External);

    routingrule.setRoutingRuleRedirect(redirect3);

    WebsiteRoutingRuleList websiteRoutingRule9;
    websiteRoutingRule9.push_back(routingrule);
    request1.setWebsiteRoutingRuleList(websiteRoutingRule9);

    outcome1 = Client->SetBucketWebsite(request1);
}

TEST_F(BucketWebsiteSettingsTest, GetBucketWebsiteResult2)
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
                        <RoutingRule>
                        <RuleNumber>2</RuleNumber>
                        <Condition>
                        <IncludeHeader>
                        <Key>host</Key>
                        <Equals>test.oss-cn-beijing-internal.aliyuncs.com</Equals>
                        </IncludeHeader>
                        <KeyPrefixEquals>abc/</KeyPrefixEquals>
                        <HttpErrorCodeReturnedEquals>404</HttpErrorCodeReturnedEquals>
                        </Condition>
                        <Redirect>
                        <RedirectType>AliCDN</RedirectType>
                        <PassQueryString>true</PassQueryString>                      
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
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RuleNumber(), "1");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleCondition().KeyPrefixEquals(), "abc/");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleCondition().HttpErrorCodeReturnedEquals(), "404");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleCondition().IncludeHeaderList()[0].Key(), "host");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleCondition().IncludeHeaderList()[0].Equals(), "test.oss-cn-beijing-internal.aliyuncs.com");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().PassQueryString(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RedirectType(), AlibabaCloud::OSS::RedirectType::Mirror);
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().MirrorURL(), "http://www.test.com/");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().MirrorPassQueryString(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().MirrorFollowRedirect(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().MirrorCheckMd5(), "false");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RoutingRuleMirrorHeaders().PassAll(), "true");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RoutingRuleMirrorHeaders().Pass()[0], "myheader-key1");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RoutingRuleMirrorHeaders().Pass()[1], "myheader-key2");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RoutingRuleMirrorHeaders().Remove()[0], "myheader-key3");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RoutingRuleMirrorHeaders().Remove()[1], "myheader-key4");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RoutingRuleMirrorHeaders().MirrorHeadersSetList()[0].Key(), "myheader-key5");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().RoutingRuleMirrorHeaders().MirrorHeadersSetList()[0].Value(), "myheader-value5");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().MirrorMultiAlternates()[0].MirrorMultiAlternateNumber(), "test");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[0].RoutingRuleRedirect().MirrorMultiAlternates()[0].MirrorMultiAlternateUrl(), "test");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[1].RoutingRuleRedirect().Protocol(), "http");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[1].RoutingRuleRedirect().HostName(), "www.test.com");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[1].RoutingRuleRedirect().HttpRedirectCode(), "301");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[1].RoutingRuleRedirect().ReplaceKeyPrefixWith(), "test");
    EXPECT_EQ(result.getWebsiteRoutingRuleList()[1].RoutingRuleRedirect().ReplaceKeyWith(), "test");
}

TEST_F(BucketWebsiteSettingsTest, GetBucketWebsiteResultBranchTest)
{
    GetBucketWebsiteResult result("test");

    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <Website>
                        
                        </Website>)";
    GetBucketWebsiteResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <WebsiteConfiguration>
                        <Suffix>index.html</Suffix>
                        <Key>error.html</Key>
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
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <WebsiteConfiguration>
                        <IndexDocument>
                        </IndexDocument>
                        <ErrorDocument>
                        </ErrorDocument>
                        <RoutingRules>
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
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <WebsiteConfiguration>
                        <IndexDocument>
                        <Suffix>index.html</Suffix>
                        </IndexDocument>
                        <ErrorDocument>
                        <Key>error.html</Key>
                        </ErrorDocument>
                        <RoutingRules>
                        <RoutingRule>
                        <IncludeHeader>
                        <Key>host</Key>
                        <Equals>test.oss-cn-beijing-internal.aliyuncs.com</Equals>
                        </IncludeHeader>
                        <KeyPrefixEquals>abc/</KeyPrefixEquals>
                        <HttpErrorCodeReturnedEquals>404</HttpErrorCodeReturnedEquals>
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
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
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
                        </IncludeHeader>
                        <KeyPrefixEquals>abc/</KeyPrefixEquals>
                        <HttpErrorCodeReturnedEquals>404</HttpErrorCodeReturnedEquals>
                        </Condition>
                        <Redirect>
                        <PassAll>true</PassAll>
                        <Pass>myheader-key1</Pass>
                        <Pass>myheader-key2</Pass>
                        <Remove>myheader-key3</Remove>
                        <Remove>myheader-key4</Remove>
                        <Set>
                        <Key>myheader-key5</Key>
                        <Value>myheader-value5</Value>
                        </Set>

                        <MirrorMultiAlternate>
                        <MirrorMultiAlternateNumber>test</MirrorMultiAlternateNumber>
                        <MirrorMultiAlternateURL>test</MirrorMultiAlternateURL>
                        </MirrorMultiAlternate>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result5(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <WebsiteConfiguration>
                        <IndexDocument>
                        <Suffix></Suffix>
                        </IndexDocument>
                        <ErrorDocument>
                        <Key></Key>
                        </ErrorDocument>
                        <RoutingRules>
                        <RoutingRule>
                        <RuleNumber></RuleNumber>
                        <Condition>
                        <IncludeHeader>
                        <Key></Key>
                        <Equals></Equals>
                        </IncludeHeader>
                        <KeyPrefixEquals></KeyPrefixEquals>
                        <HttpErrorCodeReturnedEquals></HttpErrorCodeReturnedEquals>
                        </Condition>
                        <Redirect>
                        <RedirectType></RedirectType>
                        <PassQueryString></PassQueryString>
                        <MirrorURL></MirrorURL>
                        <MirrorPassQueryString></MirrorPassQueryString>
                        <MirrorFollowRedirect></MirrorFollowRedirect>
                        <MirrorCheckMd5></MirrorCheckMd5>
                        <MirrorHeaders>
                        <PassAll></PassAll>
                        <Pass></Pass>
                        <Pass></Pass>
                        <Remove></Remove>
                        <Remove></Remove>
                        <Set>
                        <Key></Key>
                        <Value></Value>
                        </Set>
                        </MirrorHeaders>
                        <Protocol></Protocol>
                        <HostName></HostName>
                        <PassQueryString></PassQueryString>
                        <ReplaceKeyWith></ReplaceKeyWith>
                        <ReplaceKeyPrefixWith></ReplaceKeyPrefixWith>
                        <HttpRedirectCode></HttpRedirectCode>
                        <MirrorMultiAlternates>
                        <MirrorMultiAlternate>
                        <MirrorMultiAlternateNumber></MirrorMultiAlternateNumber>
                        <MirrorMultiAlternateURL></MirrorMultiAlternateURL>
                        </MirrorMultiAlternate>
                        </MirrorMultiAlternates>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result6(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketWebsiteResult result7(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
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
                        </Condition>
                        <Redirect>
                        <RedirectType>Mirror</RedirectType>
                        <PassQueryString>true</PassQueryString>
                        <MirrorURL>http://www.test.com/</MirrorURL>
                        <MirrorPassQueryString>true</MirrorPassQueryString>
                        <MirrorFollowRedirect>true</MirrorFollowRedirect>
                        <MirrorCheckMd5>false</MirrorCheckMd5>
                        <MirrorHeaders>
                        <Set>
                        </Set>
                        </MirrorHeaders>
                        <Protocol>http</Protocol>
                        <HostName>www.test.com</HostName>
                        <PassQueryString>false</PassQueryString>
                        <ReplaceKeyWith>test</ReplaceKeyWith>
                        <ReplaceKeyPrefixWith>test</ReplaceKeyPrefixWith>
                        <HttpRedirectCode>301</HttpRedirectCode>
                        <MirrorMultiAlternates>
                        </MirrorMultiAlternates>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result8(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
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
                        </Condition>
                        <Redirect>
                        <RedirectType>Mirror</RedirectType>
                        <PassQueryString>true</PassQueryString>
                        <MirrorURL>http://www.test.com/</MirrorURL>
                        <MirrorPassQueryString>true</MirrorPassQueryString>
                        <MirrorFollowRedirect>true</MirrorFollowRedirect>
                        <MirrorCheckMd5>false</MirrorCheckMd5>
                        <MirrorHeaders>
                        <Pass>test</Pass>
                        <remove>test</remove>
                        </MirrorHeaders>
                        <Protocol>http</Protocol>
                        <HostName>www.test.com</HostName>
                        <PassQueryString>false</PassQueryString>
                        <ReplaceKeyWith>test</ReplaceKeyWith>
                        <ReplaceKeyPrefixWith>test</ReplaceKeyPrefixWith>
                        <HttpRedirectCode>301</HttpRedirectCode>
                        <MirrorMultiAlternates>
                        <MirrorMultiAlternate>
                        </MirrorMultiAlternate>
                        </MirrorMultiAlternates>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result9(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
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
                        </Condition>
                        <Redirect>
                        <RedirectType>External</RedirectType>
                        <PassQueryString>true</PassQueryString>
                        <MirrorURL>http://www.test.com/</MirrorURL>
                        <MirrorPassQueryString>true</MirrorPassQueryString>
                        <MirrorFollowRedirect>true</MirrorFollowRedirect>
                        <MirrorCheckMd5>false</MirrorCheckMd5>
                        <MirrorHeaders>
                        <Pass>test</Pass>
                        <remove>test</remove>
                        </MirrorHeaders>
                        <Protocol>http</Protocol>
                        <HostName>www.test.com</HostName>
                        <PassQueryString>false</PassQueryString>
                        <ReplaceKeyWith>test</ReplaceKeyWith>
                        <ReplaceKeyPrefixWith>test</ReplaceKeyPrefixWith>
                        <HttpRedirectCode>301</HttpRedirectCode>
                        <MirrorMultiAlternates>
                        <MirrorMultiAlternate>
                        </MirrorMultiAlternate>
                        </MirrorMultiAlternates>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result10(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
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
                        </Condition>
                        <Redirect>
                        <RedirectType>External</RedirectType>
                        <PassQueryString>true</PassQueryString>
                        <MirrorURL>http://www.test.com/</MirrorURL>
                        <MirrorPassQueryString>true</MirrorPassQueryString>
                        <MirrorFollowRedirect>true</MirrorFollowRedirect>
                        <MirrorCheckMd5>false</MirrorCheckMd5>
                        <MirrorHeaders>
                        <Pass>test</Pass>
                        <remove>test</remove>
                        </MirrorHeaders>
                        <Protocol></Protocol>
                        <HostName></HostName>
                        <PassQueryString>false</PassQueryString>
                        <ReplaceKeyWith></ReplaceKeyWith>
                        <ReplaceKeyPrefixWith></ReplaceKeyPrefixWith>
                        <HttpRedirectCode></HttpRedirectCode>
                        <MirrorMultiAlternates>
                        <MirrorMultiAlternate>
                        </MirrorMultiAlternate>
                        </MirrorMultiAlternates>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result11(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
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
                        </Condition>
                        <Redirect>
                        <RedirectType>External</RedirectType>
                        <PassQueryString>true</PassQueryString>
                        <MirrorURL>http://www.test.com/</MirrorURL>
                        <MirrorPassQueryString>true</MirrorPassQueryString>
                        <MirrorFollowRedirect>true</MirrorFollowRedirect>
                        <MirrorCheckMd5>false</MirrorCheckMd5>
                        <MirrorHeaders>
                        <Pass>test</Pass>
                        <remove>test</remove>
                        </MirrorHeaders>
                        <PassQueryString>false</PassQueryString>
                        <MirrorMultiAlternates>
                        <MirrorMultiAlternate>
                        </MirrorMultiAlternate>
                        </MirrorMultiAlternates>
                        </Redirect>
                        </RoutingRule>
                        </RoutingRules>
                        </WebsiteConfiguration>)";
    GetBucketWebsiteResult result12(xml);
}
}
}