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
#include <ctime>
#include <time.h>
#include <ctime>
#include <chrono>

namespace AlibabaCloud {
namespace OSS {

class BucketCorsSettingsTest : public ::testing::Test {
protected:
    BucketCorsSettingsTest()
    {
    }

    ~BucketCorsSettingsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketcorssettings");
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

std::shared_ptr<OssClient> BucketCorsSettingsTest::Client = nullptr;
std::string  BucketCorsSettingsTest::BucketName = "";

static std::string GenSeqId()
{
    auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    tp.time_since_epoch().count();
    return std::to_string(tp.time_since_epoch().count());
}

static std::string GenOriginal()
{
    std::string original("Original ");
    original.append(GenSeqId());
    return original;
}

static CORSRule ConstructDummyCorsRule()
{
    std::string original("Original ");
    original.append(GenSeqId());

    CORSRule rule;
    rule.addAllowedOrigin(original);
    rule.addAllowedMethod("GET");
    rule.addAllowedHeader("HTTP");
    rule.addExposeHeader("HTTP");
    return rule;
}

static CORSRule ConstructDummyCorsRuleWithMultiAllowedMethod()
{
    std::string original("Original ");
    original.append(GenSeqId());

    CORSRule rule;
    rule.addAllowedOrigin(original);
    rule.addAllowedMethod("GET");
    rule.addAllowedMethod("PUT");
    rule.addAllowedMethod("DELETE");
    rule.addAllowedMethod("POST");
    rule.addAllowedMethod("HEAD");
    rule.setMaxAgeSeconds(120);
    return rule;
}

TEST_F(BucketCorsSettingsTest, GetBucketNotSetCorsTest)
{
    auto dOutcome = Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
    EXPECT_EQ(dOutcome.isSuccess(), true);

    auto gOutcome = Client->GetBucketCors(GetBucketCorsRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "NoSuchCORSConfiguration");
}

TEST_F(BucketCorsSettingsTest, EnableBucketCorsEmptyTest)
{
    SetBucketCorsRequest request(BucketName);
    request.addCORSRule(CORSRule());
    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
}

TEST_F(BucketCorsSettingsTest, EnableBucketCorsAddAndDeleteSingleRuleTest)
{
    //SetBucketCorsRequest request(BucketName);
    //request.addCORSRule(ConstructDummyCorsRule());
    CORSRuleList ruleList;
    ruleList.push_back(ConstructDummyCorsRule());
    auto outcome = Client->SetBucketCors(BucketName, ruleList);// (request);
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);
    
    auto gOutcome = Client->GetBucketCors(GetBucketCorsRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().CORSRules().size(), 1UL);

    auto dOutcome = Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
    EXPECT_EQ(dOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    //gOutcome = Client->GetBucketCors(GetBucketCorsRequest(BucketName));
    gOutcome = Client->GetBucketCors(BucketName);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_STREQ(gOutcome.error().Code().c_str(), "NoSuchCORSConfiguration");
}

TEST_F(BucketCorsSettingsTest, EnableBucketCorsAddAndDeleteMultipleRulesTest)
{
    SetBucketCorsRequest request(BucketName);
    request.addCORSRule(ConstructDummyCorsRule());
    request.addCORSRule(ConstructDummyCorsRuleWithMultiAllowedMethod());
    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);

    auto gOutcome = Client->GetBucketCors(GetBucketCorsRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().CORSRules().size(), 2UL);

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, EnableBucketCorsSetAndDeleteMultipleRulesTest)
{
    SetBucketCorsRequest request(BucketName);
    CORSRuleList ruleList;
    ruleList.push_back(ConstructDummyCorsRule());
    ruleList.push_back(ConstructDummyCorsRuleWithMultiAllowedMethod());
    request.setCORSRules(ruleList);
    request.addCORSRule(ConstructDummyCorsRule());
    request.addCORSRule(ConstructDummyCorsRuleWithMultiAllowedMethod());
    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);

    auto gOutcome = Client->GetBucketCors(GetBucketCorsRequest(BucketName));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().CORSRules().size(), 4UL);

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, EnableBucketCorsAddInvalidSingleRuleTest)
{
    SetBucketCorsRequest request(BucketName);
    CORSRule rule;
    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedMethod("GETGET");
    request.addCORSRule(rule);
    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, SetBucketCorsRequestInvalidArgumentTest)
{
    SetBucketCorsRequest request(BucketName);
    CORSRule rule;

    //rules over 10
    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedMethod("GET");
    for (int i = 0; i < 12; i++) {
        request.addCORSRule(rule);
    }
    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    //no method
    request.clearCORSRules();
    rule.clear();
    rule.addAllowedOrigin(GenOriginal());
    request.addCORSRule(rule);
    outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    //
    request.clearCORSRules();
    rule.clear();
    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedMethod("GET");
    for (int i = 0; i < 9; i++)
    {
        request.addCORSRule(rule);
    }
    outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    request.addCORSRule(rule);
    request.addCORSRule(rule);
    outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, SetBucketCorsRequestAllowedOriginAsteriskTest)
{
    SetBucketCorsRequest request(BucketName);
    CORSRule rule;

    rule.addAllowedOrigin("*");
    rule.addAllowedOrigin("*");
    rule.addAllowedMethod("GET");
    request.addCORSRule(rule);

    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    request.clearCORSRules();
    rule.clear();
    rule.addAllowedOrigin("*");
    rule.addAllowedMethod("GET");
    request.addCORSRule(rule);

    outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, SetBucketCorsRequestAllowedHeaderAsteriskTest)
{
    SetBucketCorsRequest request(BucketName);
    CORSRule rule;

    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedHeader("*");
    rule.addAllowedHeader("*");
    rule.addAllowedMethod("GET");
    request.addCORSRule(rule);

    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    request.clearCORSRules();
    rule.clear();
    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedHeader("*");
    rule.addAllowedMethod("GET");
    request.addCORSRule(rule);

    outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    TestUtils::WaitForCacheExpire(5);

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, SetBucketCorsRequestExposeHeaderAsteriskTest)
{
    SetBucketCorsRequest request(BucketName);
    CORSRule rule;

    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedMethod("GET");
    rule.addExposeHeader("*");
    request.addCORSRule(rule);

    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, SetBucketCorsRequestMaxAgeSecondsTest)
{
    SetBucketCorsRequest request(BucketName);
    CORSRule rule;
    
    //less -1
    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedMethod("GET");
    rule.addExposeHeader("x-oss-test");
    rule.setMaxAgeSeconds(-10);
    request.addCORSRule(rule);

    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    request.clearCORSRules();
    rule.clear();
    rule.addAllowedOrigin(GenOriginal());
    rule.addAllowedMethod("GET");
    rule.addExposeHeader("x-oss-test");
    rule.setMaxAgeSeconds(999999999 + 1);
    request.addCORSRule(rule);

    outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");

    Client->DeleteBucketCors(DeleteBucketCorsRequest(BucketName));
}

TEST_F(BucketCorsSettingsTest, SetBucketCorsNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-bucket-cors");
    SetBucketCorsRequest request(name);
    request.addCORSRule(ConstructDummyCorsRule());
    auto outcome = Client->SetBucketCors(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketCorsSettingsTest, GetBucketCorsWithInvalidResponseBodyTest)
{
    CORSRuleList ruleList;
    ruleList.push_back(ConstructDummyCorsRule());
    Client->SetBucketCors(BucketName, ruleList);

    auto gbcRequest = GetBucketCorsRequest(BucketName);
    gbcRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gbcOutcome = Client->GetBucketCors(gbcRequest);
    EXPECT_EQ(gbcOutcome.isSuccess(), false);
    EXPECT_EQ(gbcOutcome.error().Code(), "ParseXMLError");
}

TEST_F(BucketCorsSettingsTest, GetBucketCorsResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CORSConfiguration>
                            <CORSRule>
                              <AllowedOrigin>*</AllowedOrigin>
                              <AllowedMethod>GET</AllowedMethod>
                              <AllowedHeader>*</AllowedHeader>
                              <ExposeHeader>x-oss-test</ExposeHeader>
                              <MaxAgeSeconds>100</MaxAgeSeconds>
                            </CORSRule>
                            <CORSRule>
                              <AllowedOrigin>www.test.com</AllowedOrigin>
                              <AllowedMethod>PUT</AllowedMethod>
                              <AllowedHeader>*</AllowedHeader>
                              <ExposeHeader></ExposeHeader>
                              <MaxAgeSeconds>100</MaxAgeSeconds>
                            </CORSRule>

                        </CORSConfiguration>)";
    GetBucketCorsResult result(xml);
    EXPECT_EQ(result.CORSRules().size(), 2UL);
    EXPECT_EQ(*(result.CORSRules().begin()->AllowedMethods().begin()), "GET");
    EXPECT_EQ(*(result.CORSRules().rbegin()->AllowedMethods().begin()), "PUT");

    EXPECT_EQ(*(result.CORSRules().begin()->AllowedOrigins().begin()), "*");
    EXPECT_EQ(*(result.CORSRules().rbegin()->AllowedOrigins().begin()), "www.test.com");
}

TEST_F(BucketCorsSettingsTest, GetBucketCorsResultWithEmpty)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CORSConfiguration>
                            <CORSRule>
                              <AllowedOrigin></AllowedOrigin>
                              <AllowedMethod></AllowedMethod>
                              <AllowedHeader></AllowedHeader>
                              <ExposeHeader></ExposeHeader>
                              <MaxAgeSeconds></MaxAgeSeconds>
                            </CORSRule>
                            <CORSRule>
                              <AllowedOrigin></AllowedOrigin>
                              <AllowedMethod></AllowedMethod>
                              <AllowedHeader></AllowedHeader>
                              <ExposeHeader></ExposeHeader>
                              <MaxAgeSeconds></MaxAgeSeconds>
                            </CORSRule>
                        </CORSConfiguration>)";
    GetBucketCorsResult result(xml);
    EXPECT_EQ(result.CORSRules().size(), 2UL);
}

TEST_F(BucketCorsSettingsTest, DeleteBucketTest)
{
    auto dOutcome = Client->DeleteBucketCors(BucketName);
    EXPECT_EQ(dOutcome.isSuccess(), true);
}

TEST_F(BucketCorsSettingsTest, DeleteBucketCorsInvalidValidateTest)
{
    auto deloutcome = Client->DeleteBucketCors(DeleteBucketCorsRequest("Invalid-bucket-test"));

    EXPECT_EQ(deloutcome.isSuccess(), false);
    EXPECT_EQ(deloutcome.error().Code(), "ValidateError");
}

TEST_F(BucketCorsSettingsTest, GetBucketCorsResultBranchTest)
{
    GetBucketCorsResult result("test");
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <CORS>
                            <CORSRule>
                              <AllowedOrigin></AllowedOrigin>
                              <AllowedMethod></AllowedMethod>
                              <AllowedHeader></AllowedHeader>
                              <ExposeHeader></ExposeHeader>
                              <MaxAgeSeconds></MaxAgeSeconds>
                            </CORSRule>
                            <CORSRule>
                              <AllowedOrigin></AllowedOrigin>
                              <AllowedMethod></AllowedMethod>
                              <AllowedHeader></AllowedHeader>
                              <ExposeHeader></ExposeHeader>
                              <MaxAgeSeconds></MaxAgeSeconds>
                            </CORSRule>
                        </CORS>)";
    GetBucketCorsResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketCorsResult result2(xml);
}
}
}