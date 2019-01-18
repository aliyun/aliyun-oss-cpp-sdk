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
#include <alibabacloud/oss/Const.h>

namespace AlibabaCloud {
namespace OSS {

class BucketLifecycleSettingsTest : public ::testing::Test {
protected:
    BucketLifecycleSettingsTest()
    {
    }

    ~BucketLifecycleSettingsTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketlifecyclesettings");
        Client->CreateBucket(BucketName);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        Client->DeleteBucket(BucketName);
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

    static bool TestRule(LifecycleRule rule)
    {
        SetBucketLifecycleRequest request(BucketName);
        request.addLifecycleRule(rule);
        auto outcome = Client->SetBucketLifecycle(request);
        if (!outcome.isSuccess())
            return false;

        TestUtils::WaitForCacheExpire(5);
        auto gOutcome = Client->GetBucketLifecycle(BucketName);
        Client->DeleteBucketLifecycle(BucketName);
        TestUtils::WaitForCacheExpire(5);

        if (!gOutcome.isSuccess())
            return false;

        if (gOutcome.result().LifecycleRules().size() == 1 &&
            *(gOutcome.result().LifecycleRules().begin()) == rule) {
            return true;
        }
        return false;
    }
public:
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;
};

std::shared_ptr<OssClient> BucketLifecycleSettingsTest::Client = nullptr;
std::string BucketLifecycleSettingsTest::BucketName = "";

TEST_F(BucketLifecycleSettingsTest, GetBucketLifecycleResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <LifecycleConfiguration>
                      <Rule>
                        <ID>36e24c61-227f-4ea9-aee2-bc5af12dbc90</ID>
                        <Prefix>prefix1</Prefix>
                        <Status>Disabled</Status>
                        <Transition>
                          <Days>60</Days>
                          <StorageClass>IA</StorageClass>
                        </Transition>
                        <Transition>
                          <Days>180</Days>
                          <StorageClass>Archive</StorageClass>
                        </Transition>
                        <Expiration>
                          <Days>240</Days>
                        </Expiration>
                        <AbortMultipartUpload>
                          <Days>30</Days>
                        </AbortMultipartUpload>
                      </Rule>
                      <Rule>
                        <ID>6a7f02d9-97f0-4a58-8a55-f3fe604e2cd5</ID>
                        <Prefix>prefix2</Prefix>
                        <Status>Disabled</Status>
                        <Transition>
                          <CreatedBeforeDate>2018-05-05T00:00:00.000Z</CreatedBeforeDate>
                          <StorageClass>IA</StorageClass>
                        </Transition>
                        <Transition>
                          <CreatedBeforeDate>2018-08-05T00:00:00.000Z</CreatedBeforeDate>
                          <StorageClass>Archive</StorageClass>
                        </Transition>
                        <Expiration>
                          <CreatedBeforeDate>2018-10-05T00:00:00.000Z</CreatedBeforeDate>
                        </Expiration>
                        <AbortMultipartUpload>
                          <CreatedBeforeDate>2018-11-05T00:00:00.000Z</CreatedBeforeDate>
                        </AbortMultipartUpload>
                      </Rule>
                      <Rule>
                        <ID>11598635-ce8d-4a4f-991d-28b11e24e664</ID>
                        <Prefix>prefix3</Prefix>
                        <Status>Enabled</Status>
                        <Transition>
                          <CreatedBeforeDate>2018-05-05T00:00:00.000Z</CreatedBeforeDate>
                          <StorageClass>IA</StorageClass>
                        </Transition>
                        <Transition>
                          <CreatedBeforeDate>2018-08-05T00:00:00.000Z</CreatedBeforeDate>
                          <StorageClass>Archive</StorageClass>
                        </Transition>
                        <Expiration>
                          <CreatedBeforeDate>2018-10-05T00:00:00.000Z</CreatedBeforeDate>
                        </Expiration>
                        <AbortMultipartUpload>
                          <CreatedBeforeDate>2018-11-05T00:00:00.000Z</CreatedBeforeDate>
                        </AbortMultipartUpload>
                      </Rule>
                      <Rule>
                        <ID>1e267ae9-db1d-4396-bea1-4238e3d219c7</ID>
                        <Prefix>prefix4</Prefix>
                        <Status>Enabled</Status>
                        <AbortMultipartUpload>
                          <Days>30</Days>
                        </AbortMultipartUpload>
                      </Rule>
                      <Rule>
                        <ID>ac10167f-b4ba-489b-affc-112a5e581f71</ID>
                        <Prefix>prefix5</Prefix>
                        <Status>Enabled</Status>
                        <Transition>
                          <Days>60</Days>
                          <StorageClass>IA</StorageClass>
                        </Transition>
                        <Transition>
                          <Days>180</Days>
                          <StorageClass>Archive</StorageClass>
                        </Transition>
                        <Expiration>
                          <Days>240</Days>
                        </Expiration>
                      </Rule>
                    </LifecycleConfiguration>)";
    GetBucketLifecycleResult result(xml);
    EXPECT_EQ(result.LifecycleRules().size(), 5UL);
    EXPECT_EQ(result.LifecycleRules().begin()->ID(), "36e24c61-227f-4ea9-aee2-bc5af12dbc90");
    EXPECT_EQ(result.LifecycleRules().begin()->Prefix(), "prefix1");
    EXPECT_EQ(result.LifecycleRules().begin()->Status(), RuleStatus::Disabled);
    EXPECT_EQ(result.LifecycleRules().begin()->TransitionList().size(), 2UL);
    EXPECT_EQ(result.LifecycleRules().begin()->TransitionList().begin()->StorageClass(), StorageClass::IA);
    EXPECT_EQ(result.LifecycleRules().begin()->TransitionList().begin()->Expiration().Days(), 60UL);
    EXPECT_EQ(result.LifecycleRules().begin()->Expiration().Days(), 240UL);
    EXPECT_EQ(result.LifecycleRules().begin()->AbortMultipartUpload().Days(), 30UL);

    EXPECT_EQ(result.LifecycleRules().rbegin()->ID(), "ac10167f-b4ba-489b-affc-112a5e581f71");
    EXPECT_EQ(result.LifecycleRules().rbegin()->Prefix(), "prefix5");
    EXPECT_EQ(result.LifecycleRules().rbegin()->Status(), RuleStatus::Enabled);
    EXPECT_EQ(result.LifecycleRules().rbegin()->TransitionList().size(), 2UL);
    EXPECT_EQ(result.LifecycleRules().rbegin()->TransitionList().rbegin()->StorageClass(), StorageClass::Archive);
    EXPECT_EQ(result.LifecycleRules().rbegin()->TransitionList().rbegin()->Expiration().Days(), 180UL);
    EXPECT_EQ(result.LifecycleRules().rbegin()->Expiration().Days(), 240UL);
    EXPECT_EQ(result.LifecycleRules().rbegin()->AbortMultipartUpload().Days(), 0UL);
    EXPECT_EQ(result.LifecycleRules().rbegin()->AbortMultipartUpload().CreatedBeforeDate(), "");
    EXPECT_EQ(result.LifecycleRules().rbegin()->hasAbortMultipartUpload(), false);
}

TEST_F(BucketLifecycleSettingsTest, LifecycleSetGetDeleteRequestTest)
{
    auto rule = LifecycleRule();
    rule.setID("basic-test");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);

    SetBucketLifecycleRequest request(BucketName);
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    GetBucketLifecycleRequest gRequest(BucketName);
    auto gOutcome = Client->GetBucketLifecycle(gRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().LifecycleRules().size(), 1UL);
    if (gOutcome.result().LifecycleRules().size() == 1UL) {
        EXPECT_TRUE(*(gOutcome.result().LifecycleRules().begin()) == rule);
    }

    DeleteBucketLifecycleRequest dRequest(BucketName);
    auto dOutcome = Client->DeleteBucketLifecycle(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
}

TEST_F(BucketLifecycleSettingsTest, LifecycleBasicSettingTest)
{
    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-001");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);
    EXPECT_TRUE(TestRule(rule));

    rule = LifecycleRule();
    rule.setID("StandardExpireRule-002");
    rule.setPrefix("object");
    rule.setStatus(RuleStatus::Disabled);
    rule.Expiration().setDays(365);
    EXPECT_TRUE(TestRule(rule));

    rule = LifecycleRule();
    rule.setID("StandardExpireRule-003");
    rule.setPrefix("object");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(200, true));
    EXPECT_TRUE(TestRule(rule));

    rule = LifecycleRule();
    rule.setID("StandardExpireRule-004");
    rule.setPrefix("object");
    rule.setStatus(RuleStatus::Disabled);
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(365, true));
    EXPECT_TRUE(TestRule(rule));
}

TEST_F(BucketLifecycleSettingsTest, LifecycleAdvancedSettingTest)
{
    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-101");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(400);
    rule.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(400, true));
    
    auto transition = LifeCycleTransition();
    transition.Expiration().setDays(180);
    transition.setStorageClass(StorageClass::IA);
    rule.addTransition(transition);
    
    transition.Expiration().setDays(365);
    transition.setStorageClass(StorageClass::Archive);
    rule.addTransition(transition);
    EXPECT_TRUE(TestRule(rule));
    
    rule = LifecycleRule();
    rule.setID("StandardExpireRule-102");
    rule.setPrefix("object");
    rule.setStatus(RuleStatus::Disabled);
    rule.Expiration().setDays(365);
    rule.AbortMultipartUpload().setDays(200);

    transition.Expiration().setDays(250);
    transition.setStorageClass(StorageClass::Archive);
    rule.addTransition(transition);
    EXPECT_TRUE(TestRule(rule));

    rule = LifecycleRule();
    rule.setID("StandardExpireRule-103");
    rule.setPrefix("object");
    rule.setStatus(RuleStatus::Disabled);
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(365, true));
    rule.AbortMultipartUpload().setDays(200);
    EXPECT_TRUE(TestRule(rule));

    rule = LifecycleRule();
    rule.setID("StandardExpireRule-104");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(400, true));
    rule.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(400, true));

    transition = LifeCycleTransition();
    transition.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(180, true));
    transition.setStorageClass(StorageClass::IA);
    rule.addTransition(transition);

    transition.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(365, true));
    transition.setStorageClass(StorageClass::Archive);
    rule.addTransition(transition);
    EXPECT_TRUE(TestRule(rule));
}

TEST_F(BucketLifecycleSettingsTest, SetLifecycleRequestLifecycleRuleEmptyTest)
{
    SetBucketLifecycleRequest request(BucketName);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "LifecycleRule should not be null or empty") != nullptr);
}

TEST_F(BucketLifecycleSettingsTest, SetLifecycleRequestLifecycleRuleNoConfigTest)
{
    SetBucketLifecycleRequest request(BucketName);
    auto rule = LifecycleRule();
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "Configure at least one of file and fragment lifecycle.") != nullptr);
}

TEST_F(BucketLifecycleSettingsTest, SetLifecycleRequestRuleForBucketPasitiveTest)
{
    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-301");
    rule.setStatus(RuleStatus::Disabled);
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(365, true));
    EXPECT_TRUE(TestRule(rule));
}

TEST_F(BucketLifecycleSettingsTest, SetLifecycleRequestRuleForBucketNegativeTest)
{
    SetBucketLifecycleRequest request(BucketName);
    auto rule = LifecycleRule();
    rule.setID("StandardExpireRule-401");
    rule.setStatus(RuleStatus::Disabled);
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(365, true));
    request.addLifecycleRule(rule);

    rule = LifecycleRule();
    rule.setID("StandardExpireRule-402");
    rule.setPrefix("object");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(365, true));
    request.addLifecycleRule(rule);

    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "You have a rule for a prefix") != nullptr);
}

TEST_F(BucketLifecycleSettingsTest, SetLifecycleRequestInvalidBucketNameTest)
{
    SetBucketLifecycleRequest request("InvalidBucketName");
    auto rule = LifecycleRule();
    request.addLifecycleRule(rule);
    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "The bucket name is invalid") != nullptr);
}

TEST_F(BucketLifecycleSettingsTest, LifecycleRuleEqualOperationTest)
{
    //ID NG
    auto rule  = LifecycleRule();
    auto rule1 = LifecycleRule();
    rule.setID("id0");
    rule1.setID("id1");
    EXPECT_FALSE(rule == rule1);

    //Prefix NG
    rule  = LifecycleRule();
    rule1 = LifecycleRule();
    rule.setID("id");
    rule1.setID("id");
    rule.setPrefix("prefix0");
    rule.setPrefix("prefix1");
    EXPECT_FALSE(rule == rule1);

    //RuleStatus NG
    rule  = LifecycleRule();
    rule1 = LifecycleRule();
    rule.setID("id");
    rule1.setID("id");
    rule.setPrefix("prefix");
    rule1.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1.setStatus(RuleStatus::Disabled);
    EXPECT_FALSE(rule == rule1);

    //Expiration CreatedBeforeDate NG
    rule = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(10));
    rule1.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(20));
    EXPECT_FALSE(rule == rule1);

    //Expiration Days NG
    rule = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    rule.Expiration().setDays(10);
    rule1.Expiration().setDays(20);
    EXPECT_FALSE(rule == rule1);

    //AbortMultipartUpload CreatedBeforeDate NG
    rule = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    rule.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(10));
    rule1.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(20));
    EXPECT_FALSE(rule == rule1);

    //AbortMultipartUpload Days NG
    rule = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    rule.AbortMultipartUpload().setDays(20);
    rule1.AbortMultipartUpload().setDays(50);
    EXPECT_FALSE(rule == rule1);

    //TransitionList size NG
    rule = LifecycleRule();
    rule1 = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    auto transition = LifeCycleTransition();
    transition.Expiration().setDays(10);
    transition.setStorageClass(StorageClass::IA);
    rule.addTransition(transition);
    rule1.addTransition(transition);
    transition.Expiration().setDays(60);
    transition.setStorageClass(StorageClass::Archive);
    rule1.addTransition(transition);
    EXPECT_FALSE(rule == rule1);

    //Transition StorageClass NG
    rule = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    transition = LifeCycleTransition();
    transition.Expiration().setDays(10);
    transition.setStorageClass(StorageClass::IA);
    rule.addTransition(transition);
    transition.Expiration().setDays(10);
    transition.setStorageClass(StorageClass::Archive);
    rule1.addTransition(transition);
    EXPECT_FALSE(rule == rule1);

    //Transition Days NG
    rule = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    transition = LifeCycleTransition();
    transition.Expiration().setDays(10);
    transition.setStorageClass(StorageClass::IA);
    rule.addTransition(transition);
    transition.Expiration().setDays(20);
    transition.setStorageClass(StorageClass::IA);
    rule1.addTransition(transition);
    EXPECT_FALSE(rule == rule1);

    //Transition CreatedBeforeDate NG
    rule = LifecycleRule();
    rule.setID("id");
    rule.setPrefix("prefix");
    rule.setStatus(RuleStatus::Enabled);
    rule1 = rule;
    transition = LifeCycleTransition();
    transition.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(10));
    transition.setStorageClass(StorageClass::IA);
    rule.addTransition(transition);
    transition.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(20));
    transition.setStorageClass(StorageClass::IA);
    rule1.addTransition(transition);
    EXPECT_FALSE(rule == rule1);
}

TEST_F(BucketLifecycleSettingsTest, LifeCycleExpirationTest)
{
    auto expiration  = LifeCycleExpiration(30);
    auto expiration1 = LifeCycleExpiration();
    expiration1.setDays(30);
    EXPECT_EQ(expiration.Days(), expiration1.Days());

    expiration  = LifeCycleExpiration(TestUtils::GetUTCString(10));
    expiration1 = LifeCycleExpiration();
    expiration1.setCreatedBeforeDate(TestUtils::GetUTCString(10));
    EXPECT_EQ(expiration.CreatedBeforeDate(), expiration1.CreatedBeforeDate());

    auto utcStr = TestUtils::GetUTCString(10);
    expiration = LifeCycleExpiration(utcStr);
    EXPECT_EQ(expiration.Days(), 0UL);
    EXPECT_EQ(expiration.CreatedBeforeDate(), utcStr);
    expiration.setDays(20);
    EXPECT_EQ(expiration.Days(), 20UL);
    EXPECT_EQ(expiration.CreatedBeforeDate(), "");
    expiration.setCreatedBeforeDate(utcStr);
    EXPECT_EQ(expiration.Days(), 0UL);
    EXPECT_EQ(expiration.CreatedBeforeDate(), utcStr);
}

TEST_F(BucketLifecycleSettingsTest, LifeCycleTransitionTest)
{
    auto expiration = LifeCycleExpiration(30);
    auto transition = LifeCycleTransition(expiration, StorageClass::IA);
    EXPECT_EQ(transition.Expiration().Days(), 30UL);
    EXPECT_EQ(transition.StorageClass(), StorageClass::IA);

    expiration = LifeCycleExpiration(60);
    transition = LifeCycleTransition();
    transition.setExpiration(expiration);
    EXPECT_EQ(transition.Expiration().Days(), 60UL);
}

TEST_F(BucketLifecycleSettingsTest, LifeCycleRuleLimitTest)
{
    SetBucketLifecycleRequest request(BucketName);
    for (int i = 0; i < LifecycleRuleLimit + 1; i++) {
        auto rule = LifecycleRule();
        std::string prefix = "prefix"; prefix.append(std::to_string(i));
        rule.setPrefix(prefix);
        rule.Expiration().setDays(10 + i);
        request.addLifecycleRule(rule);
    }

    auto outcome = Client->SetBucketLifecycle(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_TRUE(strstr(outcome.error().Message().c_str(), "One bucket not allow exceed one thousand") != nullptr);
}


TEST_F(BucketLifecycleSettingsTest, DeleteBucketLifecycleNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-lifecycle");
    auto outcome = Client->DeleteBucketLifecycle(name);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketLifecycleSettingsTest, GetBucketLifecycleNegativeTest)
{
    auto name = TestUtils::GetBucketName("no-exist-lifecycle");
    auto outcome = Client->GetBucketLifecycle(name);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

}
}