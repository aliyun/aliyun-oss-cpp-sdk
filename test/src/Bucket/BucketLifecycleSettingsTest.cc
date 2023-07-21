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
    rule.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(-400, true));
    rule.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(400, true));

    transition = LifeCycleTransition();
    transition.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(-180, true));
    transition.setStorageClass(StorageClass::IA);
    rule.addTransition(transition);

    transition.Expiration().setCreatedBeforeDate(TestUtils::GetUTCString(-365, true));
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

TEST_F(BucketLifecycleSettingsTest, GetBucketLifecycleResultBranchTest)
{
    GetBucketLifecycleResult result("test");
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <Lifecycle>

                    </Lifecycle>)";
    GetBucketLifecycleResult result1(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <LifecycleConfiguration>
                      <Rule>
                        <ID>36e24c61-227f-4ea9-aee2-bc5af12dbc90</ID>
                        <Prefix>prefix1</Prefix>
                        <Status>Disabled</Status>
                        <Transition>
                        </Transition>
                        <Transition>
                          <Days>180</Days>
                          <StorageClass>Archive</StorageClass>
                        </Transition>
                        <Expiration>
                        </Expiration>
                        <AbortMultipartUpload>
                        </AbortMultipartUpload>
                      </Rule>
                    </LifecycleConfiguration>)";
    GetBucketLifecycleResult result2(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <LifecycleConfiguration>
                      <Rule>
                          <Days>180</Days>
                          <StorageClass>Archive</StorageClass>
                          <Days>240</Days>
                          <Days>30</Days>
                      </Rule>
                    </LifecycleConfiguration>)";
    GetBucketLifecycleResult result3(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <LifecycleConfiguration>
                    <Rule>
                        <ID></ID>
                        <Prefix></Prefix>
                        <Status></Status>
                        <Transition>
                        <CreatedBeforeDate></CreatedBeforeDate>
                        <StorageClass></StorageClass>
                        </Transition>
                        <Expiration>
                        <CreatedBeforeDate></CreatedBeforeDate>
                          <Days></Days>
                        </Expiration>
                        <AbortMultipartUpload>
                        <CreatedBeforeDate></CreatedBeforeDate>
                        </AbortMultipartUpload>
                        <Tag>
                        </Tag>
                        </Rule>
                    </LifecycleConfiguration>)";
    GetBucketLifecycleResult result4(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <LifecycleConfiguration>
                    <Rule>
                        <ID></ID>
                        <Prefix></Prefix>
                        <Status></Status>
                        <Transition>
                          <Days></Days>
                        <CreatedBeforeDate></CreatedBeforeDate>
                        <StorageClass></StorageClass>
                        </Transition>
                        <Expiration>
                        <CreatedBeforeDate></CreatedBeforeDate>
                        </Expiration>
                        <AbortMultipartUpload>
                          <Days></Days>
                        <CreatedBeforeDate></CreatedBeforeDate>
                        </AbortMultipartUpload>
                        <Tag>
                          <Key></Key>
                          <Value></Value>
                        </Tag>
                        </Rule>
                    </LifecycleConfiguration>)";
    GetBucketLifecycleResult result5(xml);

    xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketLifecycleResult result6(xml);
}

TEST_F(BucketLifecycleSettingsTest, GetBucketLifecycleResultWithVersioningTest)
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
                        <ID>delete example</ID>
                        <Prefix>logs/</Prefix>
                        <Status>Enabled</Status>
                        <Expiration>
                          <ExpiredObjectDeleteMarker>true</ExpiredObjectDeleteMarker>
                        </Expiration>
                        <NoncurrentVersionExpiration>
                          <NoncurrentDays>5</NoncurrentDays>
                        </NoncurrentVersionExpiration>
                        <AbortMultipartUpload>
                          <Days>1</Days>
                        </AbortMultipartUpload>
                      </Rule>
                      <Rule>
                        <ID>transit example</ID>
                        <Prefix>data/</Prefix>
                        <Status>Enabled</Status>
                        <Transition>
                          <Days>30</Days>
                          <StorageClass>IA</StorageClass>
                        </Transition>
                        <Transition>
                          <Days>130</Days>
                          <StorageClass>Archive</StorageClass>
                        </Transition>
                        <NoncurrentVersionTransition>
                          <NoncurrentDays>10</NoncurrentDays>
                          <StorageClass>IA</StorageClass>
                        </NoncurrentVersionTransition>
                        <NoncurrentVersionTransition>
                          <NoncurrentDays>20</NoncurrentDays>
                          <StorageClass>Archive</StorageClass>
                        </NoncurrentVersionTransition>
                      </Rule>
                    </LifecycleConfiguration>)";
    GetBucketLifecycleResult result(xml);
    EXPECT_EQ(result.LifecycleRules().size(), 4UL);
    //rule 36e24c61-227f-4ea9-aee2-bc5af12dbc90
    EXPECT_EQ(result.LifecycleRules().begin()->ID(), "36e24c61-227f-4ea9-aee2-bc5af12dbc90");
    EXPECT_EQ(result.LifecycleRules().begin()->Prefix(), "prefix1");
    EXPECT_EQ(result.LifecycleRules().begin()->Status(), RuleStatus::Disabled);
    EXPECT_EQ(result.LifecycleRules().begin()->TransitionList().size(), 2UL);
    EXPECT_EQ(result.LifecycleRules().begin()->TransitionList().begin()->StorageClass(), StorageClass::IA);
    EXPECT_EQ(result.LifecycleRules().begin()->TransitionList().begin()->Expiration().Days(), 60U);
    EXPECT_EQ(result.LifecycleRules().begin()->Expiration().Days(), 240U);
    EXPECT_EQ(result.LifecycleRules().begin()->ExpiredObjectDeleteMarker(), false);
    EXPECT_EQ(result.LifecycleRules().begin()->AbortMultipartUpload().Days(), 30U);

    //rule delete example
    EXPECT_EQ(result.LifecycleRules().at(2).ID(), "delete example");
    EXPECT_EQ(result.LifecycleRules().at(2).Prefix(), "logs/");
    EXPECT_EQ(result.LifecycleRules().at(2).Status(), RuleStatus::Enabled);
    EXPECT_EQ(result.LifecycleRules().at(2).TransitionList().size(), 0UL);
    EXPECT_EQ(result.LifecycleRules().at(2).Expiration().Days(), 0U);
    EXPECT_EQ(result.LifecycleRules().at(2).ExpiredObjectDeleteMarker(), true);
    EXPECT_EQ(result.LifecycleRules().at(2).NoncurrentVersionExpiration().Days(), 5U);
    EXPECT_EQ(result.LifecycleRules().at(2).AbortMultipartUpload().Days(), 1U);
    EXPECT_EQ(result.LifecycleRules().at(2).hasAbortMultipartUpload(), true);
    EXPECT_EQ(result.LifecycleRules().at(2).hasTransitionList(), false);
    EXPECT_EQ(result.LifecycleRules().at(2).hasNoncurrentVersionTransitionList(), false);

    //rule transit example
    EXPECT_EQ(result.LifecycleRules().at(3).ID(), "transit example");
    EXPECT_EQ(result.LifecycleRules().at(3).Prefix(), "data/");
    EXPECT_EQ(result.LifecycleRules().at(3).Status(), RuleStatus::Enabled);
    EXPECT_EQ(result.LifecycleRules().at(3).TransitionList().size(), 2UL);
    EXPECT_EQ(result.LifecycleRules().at(3).TransitionList().at(0).StorageClass(), StorageClass::IA);
    EXPECT_EQ(result.LifecycleRules().at(3).TransitionList().at(0).Expiration().Days(), 30U);
    EXPECT_EQ(result.LifecycleRules().at(3).TransitionList().at(1).StorageClass(), StorageClass::Archive);
    EXPECT_EQ(result.LifecycleRules().at(3).TransitionList().at(1).Expiration().Days(), 130U);
    EXPECT_EQ(result.LifecycleRules().at(3).NoncurrentVersionTransitionList().size(), 2U);
    EXPECT_EQ(result.LifecycleRules().at(3).NoncurrentVersionTransitionList().at(0).StorageClass(), StorageClass::IA);
    EXPECT_EQ(result.LifecycleRules().at(3).NoncurrentVersionTransitionList().at(0).Expiration().Days(), 10U);
    EXPECT_EQ(result.LifecycleRules().at(3).NoncurrentVersionTransitionList().at(1).StorageClass(), StorageClass::Archive);
    EXPECT_EQ(result.LifecycleRules().at(3).NoncurrentVersionTransitionList().at(1).Expiration().Days(), 20U);
    EXPECT_EQ(result.LifecycleRules().at(3).hasAbortMultipartUpload(), false);
    EXPECT_EQ(result.LifecycleRules().at(3).hasExpiration(), false);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <LifecycleConfiguration>
            <Rule>
                <ID>delete example</ID>
                <Prefix>logs/</Prefix>
                <Status>Enabled</Status>
                <Expiration>
                    <ExpiredObjectDeleteMarker></ExpiredObjectDeleteMarker>
                </Expiration>
                <NoncurrentVersionExpiration>
                    <NoncurrentDays></NoncurrentDays>
                </NoncurrentVersionExpiration>
            </Rule>
            <Rule>
            <ID>transit example</ID>
            <Prefix>data/</Prefix>
            <Status>Enabled</Status>
            <Transition>
                <Days>30</Days>
                <StorageClass>IA</StorageClass>
            </Transition>
            <Transition>
                <Days>130</Days>
                <StorageClass>Archive</StorageClass>
            </Transition>
            <NoncurrentVersionTransition>
                <NoncurrentDays></NoncurrentDays>
                <StorageClass></StorageClass>
            </NoncurrentVersionTransition>
            <NoncurrentVersionTransition>
                <NoncurrentDays></NoncurrentDays>
                <StorageClass></StorageClass>
            </NoncurrentVersionTransition>
            </Rule>
        </LifecycleConfiguration>)";
    result = GetBucketLifecycleResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <LifecycleConfiguration>
            <Rule>
                <ID>delete example</ID>
                <Prefix>logs/</Prefix>
                <Status>Enabled</Status>
                <Expiration>
                </Expiration>
                <NoncurrentVersionExpiration>
                </NoncurrentVersionExpiration>
            </Rule>
            <Rule>
            <ID>transit example</ID>
            <Prefix>data/</Prefix>
            <Status>Enabled</Status>
            <Transition>
                <Days>30</Days>
                <StorageClass>IA</StorageClass>
            </Transition>
            <Transition>
                <Days>130</Days>
                <StorageClass>Archive</StorageClass>
            </Transition>
            <NoncurrentVersionTransition>
            </NoncurrentVersionTransition>
            <NoncurrentVersionTransition>
            </NoncurrentVersionTransition>
            </Rule>
        </LifecycleConfiguration>)";
    result = GetBucketLifecycleResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <LifecycleConfiguration>
            <Rule>
            </Rule>
            <Rule>
            </Rule>
        </LifecycleConfiguration>)";
    result = GetBucketLifecycleResult(xml);
}

TEST_F(BucketLifecycleSettingsTest, LifecycleRuleWithVersioningTest)
{
    auto rule1 = LifecycleRule();
    auto rule2 = LifecycleRule();
    EXPECT_EQ(rule1 == rule2, true);

    //ExpiredObjectDeleteMarker
    rule1 = LifecycleRule();
    rule1.setExpiredObjectDeleteMarker(true);
    rule2 = LifecycleRule();
    rule2.setExpiredObjectDeleteMarker(true);
    EXPECT_EQ(rule1 == rule2, true);

    rule1 = LifecycleRule();
    rule2 = LifecycleRule();
    rule2.setExpiredObjectDeleteMarker(true);
    EXPECT_EQ(rule1 == rule2, false);

    //NoncurrentVersionExpiration
    rule1 = LifecycleRule();
    rule1.setNoncurrentVersionExpiration(LifeCycleExpiration(30U));
    rule2 = LifecycleRule();
    rule2.setNoncurrentVersionExpiration(LifeCycleExpiration(30U));
    EXPECT_EQ(rule1 == rule2, true);

    rule1 = LifecycleRule();
    rule1.setNoncurrentVersionExpiration(LifeCycleExpiration(10U));
    rule2 = LifecycleRule();
    rule2.setNoncurrentVersionExpiration(LifeCycleExpiration(30U));
    EXPECT_EQ(rule1 == rule2, false);

    rule1 = LifecycleRule();
    rule2 = LifecycleRule();
    rule2.setNoncurrentVersionExpiration(LifeCycleExpiration(30U));
    EXPECT_EQ(rule1 == rule2, false);

    //NoncurrentVersionTransition
    rule1 = LifecycleRule();
    rule1.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(10U), StorageClass::IA));
    rule2 = LifecycleRule();
    rule2.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(10U), StorageClass::IA));
    EXPECT_EQ(rule1 == rule2, true);

    rule1 = LifecycleRule();
    rule2 = LifecycleRule();
    rule2.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(10U), StorageClass::IA));
    EXPECT_EQ(rule1 == rule2, false);

    rule1 = LifecycleRule();
    rule1.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(20U), StorageClass::IA));
    rule2 = LifecycleRule();
    rule2.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(10U), StorageClass::IA));
    EXPECT_EQ(rule1 == rule2, false);

    rule1 = LifecycleRule();
    rule1.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(20U), StorageClass::IA));
    rule2 = LifecycleRule();
    rule2.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(20U), StorageClass::Archive));
    EXPECT_EQ(rule1 == rule2, false);

    rule1 = LifecycleRule();
    rule1.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration("2018-05-05T00:00:00.000Z"), StorageClass::IA));
    rule2 = LifecycleRule();
    rule2.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration("2018-05-06T00:00:00.000Z"), StorageClass::IA));
    EXPECT_EQ(rule1 == rule2, false);

    //LifecycleRule() get & set test
    auto rule = LifecycleRule();
    LifeCycleExpiration expiration;
    expiration.setDays(30U);
    rule.setNoncurrentVersionExpiration(expiration);
    EXPECT_EQ(rule.NoncurrentVersionExpiration().Days(), expiration.Days());
    EXPECT_EQ(rule.NoncurrentVersionExpiration().CreatedBeforeDate(), expiration.CreatedBeforeDate());

    rule.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(20U), StorageClass::IA));
    rule.addNoncurrentVersionTransition(LifeCycleTransition(LifeCycleExpiration(200U), StorageClass::Archive));

    EXPECT_EQ(rule.hasNoncurrentVersionTransitionList(), true);
    EXPECT_EQ(rule.NoncurrentVersionTransitionList().size(), 2UL);
    EXPECT_EQ(rule.NoncurrentVersionTransitionList().at(0).StorageClass(), StorageClass::IA);
    EXPECT_EQ(rule.NoncurrentVersionTransitionList().at(1).StorageClass(), StorageClass::Archive);
}

TEST_F(BucketLifecycleSettingsTest, SetAndGetLifecycleRuleWithVersioningTest)
{
    auto bucketName = BucketName;
    bucketName.append("-lc-version");

    auto client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());

    auto cOutcome = client->CreateBucket(bucketName);
    EXPECT_EQ(cOutcome.isSuccess(), true);

    auto bsOutcome = client->SetBucketVersioning(SetBucketVersioningRequest(bucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = client->GetBucketInfo(bucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    SetBucketLifecycleRequest request(bucketName);

    //rule 1
    auto rule1 = LifecycleRule();
    rule1.setID("StandardExpireRule-101");
    rule1.setPrefix("standard/");
    rule1.setStatus(RuleStatus::Enabled);
    rule1.Expiration().setDays(400);
    rule1.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(400, true));

    auto transition = LifeCycleTransition();
    transition.Expiration().setDays(180);
    transition.setStorageClass(StorageClass::IA);
    rule1.addTransition(transition);

    transition.Expiration().setDays(365);
    transition.setStorageClass(StorageClass::Archive);
    rule1.addTransition(transition);

    request.addLifecycleRule(rule1);

    //rule 2
    auto rule2 = LifecycleRule();
    rule2.setID("transit example");
    rule2.setPrefix("test");
    rule2.setStatus(RuleStatus::Enabled);
    rule2.Expiration().setDays(100);
    rule2.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(400, true));

    transition = LifeCycleTransition();
    transition.Expiration().setDays(180);
    transition.setStorageClass(StorageClass::IA);
    rule2.addNoncurrentVersionTransition(transition);

    transition.Expiration().setDays(365);
    transition.setStorageClass(StorageClass::Archive);
    rule2.addNoncurrentVersionTransition(transition);

    request.addLifecycleRule(rule2);

    //rule 3
    auto rule3 = LifecycleRule();
    rule3.setID("delete example");
    rule3.setPrefix("log/");
    rule3.setStatus(RuleStatus::Enabled);
    rule3.setExpiredObjectDeleteMarker(true);
    rule3.AbortMultipartUpload().setCreatedBeforeDate(TestUtils::GetUTCString(300, true));

    rule3.NoncurrentVersionExpiration().setDays(200U);
    request.addLifecycleRule(rule3);


    auto sOutcome = client->SetBucketLifecycle(request);
    EXPECT_EQ(sOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);

    auto gOutcome = client->GetBucketLifecycle(bucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().LifecycleRules().size(), 3UL);
    EXPECT_EQ(gOutcome.result().LifecycleRules().at(0), rule1);
    EXPECT_EQ(gOutcome.result().LifecycleRules().at(1), rule2);
    EXPECT_EQ(gOutcome.result().LifecycleRules().at(2), rule3);

    auto dOutcome = client->DeleteBucketLifecycle(bucketName);
    EXPECT_EQ(dOutcome.isSuccess(), true);

    //Only ExpiredObjectDeleteMarker
    auto rule4 = LifecycleRule();
    rule4.setID("only delete marker");
    rule4.setPrefix("log/");
    rule4.setStatus(RuleStatus::Enabled);
    rule4.setExpiredObjectDeleteMarker(true);

    request.clearLifecycleRules();
    request.addLifecycleRule(rule4);
    sOutcome = client->SetBucketLifecycle(request);
    EXPECT_EQ(sOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    gOutcome = client->GetBucketLifecycle(bucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().LifecycleRules().size(), 1UL);
    EXPECT_EQ(gOutcome.result().LifecycleRules().at(0), rule4);

    //Only NoncurrentVersionTransition
    auto rule5 = LifecycleRule();
    rule5.setID("only transit");
    rule5.setPrefix("test");
    rule5.setStatus(RuleStatus::Enabled);

    transition = LifeCycleTransition();
    transition.Expiration().setDays(180);
    transition.setStorageClass(StorageClass::IA);
    rule5.addNoncurrentVersionTransition(transition);
    transition.Expiration().setDays(365);
    transition.setStorageClass(StorageClass::Archive);
    rule5.addNoncurrentVersionTransition(transition);

    request.clearLifecycleRules();
    request.addLifecycleRule(rule5);
    sOutcome = client->SetBucketLifecycle(request);
    EXPECT_EQ(sOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    gOutcome = client->GetBucketLifecycle(bucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().LifecycleRules().size(), 1UL);
    EXPECT_EQ(gOutcome.result().LifecycleRules().at(0), rule5);

    //Only NoncurrentVersionExpiration
    auto rule6 = LifecycleRule();
    rule6.setID("only expiration");
    rule6.setPrefix("log/");
    rule6.setStatus(RuleStatus::Enabled);
    rule6.setNoncurrentVersionExpiration(LifeCycleExpiration(30U));

    request.clearLifecycleRules();
    request.addLifecycleRule(rule6);
    sOutcome = client->SetBucketLifecycle(request);
    EXPECT_EQ(sOutcome.isSuccess(), true);

    TestUtils::WaitForCacheExpire(5);
    gOutcome = client->GetBucketLifecycle(bucketName);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().LifecycleRules().size(), 1UL);
    EXPECT_EQ(gOutcome.result().LifecycleRules().at(0), rule6);

    client->DeleteBucket(bucketName);
}

TEST_F(BucketLifecycleSettingsTest, GetBucketLifecycleWithInvalidResponseBodyTest)
{
    auto rule = LifecycleRule();
    rule.setID("basic-test");
    rule.setPrefix("test");
    rule.setStatus(RuleStatus::Enabled);
    rule.Expiration().setDays(200);
    auto sblRequest = SetBucketLifecycleRequest(BucketName);
    sblRequest.addLifecycleRule(rule);
    Client->SetBucketLifecycle(sblRequest);

    auto gblfRequest = GetBucketLifecycleRequest(BucketName);
    gblfRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto gblfOutcome = Client->GetBucketLifecycle(gblfRequest);
    EXPECT_EQ(gblfOutcome.isSuccess(), false);
    EXPECT_EQ(gblfOutcome.error().Code(), "ParseXMLError");
}

}
}