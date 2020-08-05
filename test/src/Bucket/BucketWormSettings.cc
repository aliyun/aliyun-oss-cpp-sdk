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

class BucketWormSettings : public ::testing::Test {
protected:
    BucketWormSettings()
    {
    }

    ~BucketWormSettings() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-bucketwormsettings");
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
std::shared_ptr<OssClient> BucketWormSettings::Client = nullptr;
std::string BucketWormSettings::BucketName = "";

TEST_F(BucketWormSettings, InvalidBucketNameTest)
{
    for (auto const invalidBucketName : TestUtils::InvalidBucketNamesList()) {
        auto ibWOutcome = Client->InitiateBucketWorm(InitiateBucketWormRequest(invalidBucketName, 10));
        EXPECT_EQ(ibWOutcome.isSuccess(), false);
        EXPECT_STREQ(ibWOutcome.error().Code().c_str(), "ValidateError");

        auto abWOutcome = Client->AbortBucketWorm(AbortBucketWormRequest(invalidBucketName));
        EXPECT_EQ(abWOutcome.isSuccess(), false);
        EXPECT_STREQ(abWOutcome.error().Code().c_str(), "ValidateError");

        auto cbWOutcome = Client->CompleteBucketWorm(CompleteBucketWormRequest(invalidBucketName, "wormId"));
        EXPECT_EQ(cbWOutcome.isSuccess(), false);
        EXPECT_STREQ(cbWOutcome.error().Code().c_str(), "ValidateError");

        auto ebWOutcome = Client->ExtendBucketWormWorm(ExtendBucketWormRequest(invalidBucketName, "wormId", 20));
        EXPECT_EQ(ebWOutcome.isSuccess(), false);
        EXPECT_STREQ(ebWOutcome.error().Code().c_str(), "ValidateError");

        auto gbWOutcome = Client->GetBucketWorm(GetBucketWormRequest(invalidBucketName));
        EXPECT_EQ(gbWOutcome.isSuccess(), false);
        EXPECT_STREQ(gbWOutcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketWormSettings, BucketWormBasicTest)
{
    auto ibWOutcome = Client->InitiateBucketWorm(InitiateBucketWormRequest(BucketName, 10));
    EXPECT_EQ(ibWOutcome.isSuccess(), true);
    EXPECT_EQ(ibWOutcome.result().WormId().empty(), false);

    auto gbWOutcome = Client->GetBucketWorm(GetBucketWormRequest(BucketName));
    EXPECT_EQ(gbWOutcome.isSuccess(), true);
    EXPECT_EQ(gbWOutcome.result().CreationDate().empty(), false);
    EXPECT_EQ(gbWOutcome.result().Day(), 10UL);
    EXPECT_EQ(gbWOutcome.result().State(), "InProgress");
    EXPECT_EQ(gbWOutcome.result().WormId(), ibWOutcome.result().WormId());

    auto abWOutcome = Client->AbortBucketWorm(AbortBucketWormRequest(BucketName));
    EXPECT_EQ(abWOutcome.isSuccess(), true);

    auto cbWOutcome = Client->CompleteBucketWorm(CompleteBucketWormRequest(BucketName, ibWOutcome.result().WormId()));
    EXPECT_EQ(cbWOutcome.isSuccess(), false);

    ibWOutcome = Client->InitiateBucketWorm(InitiateBucketWormRequest(BucketName, 8));
    EXPECT_EQ(ibWOutcome.isSuccess(), true);
    EXPECT_EQ(ibWOutcome.result().WormId().empty(), false);

    gbWOutcome = Client->GetBucketWorm(GetBucketWormRequest(BucketName));
    EXPECT_EQ(gbWOutcome.isSuccess(), true);
    EXPECT_EQ(gbWOutcome.result().CreationDate().empty(), false);
    EXPECT_EQ(gbWOutcome.result().Day(), 8UL);
    EXPECT_EQ(gbWOutcome.result().State(), "InProgress");
    EXPECT_EQ(gbWOutcome.result().WormId(), ibWOutcome.result().WormId());

    cbWOutcome = Client->CompleteBucketWorm(CompleteBucketWormRequest(BucketName, ibWOutcome.result().WormId()));
    EXPECT_EQ(cbWOutcome.isSuccess(), true);

    gbWOutcome = Client->GetBucketWorm(GetBucketWormRequest(BucketName));
    EXPECT_EQ(gbWOutcome.isSuccess(), true);
    EXPECT_EQ(gbWOutcome.result().State(), "Locked");

    auto ebWOutcome = Client->ExtendBucketWormWorm(ExtendBucketWormRequest(BucketName, ibWOutcome.result().WormId(), 20));
    EXPECT_EQ(ebWOutcome.isSuccess(), true);

    gbWOutcome = Client->GetBucketWorm(GetBucketWormRequest(BucketName));
    EXPECT_EQ(gbWOutcome.isSuccess(), true);
    EXPECT_EQ(gbWOutcome.result().Day(), 20UL);

    abWOutcome = Client->AbortBucketWorm(AbortBucketWormRequest(BucketName));
    EXPECT_EQ(abWOutcome.isSuccess(), false);
    EXPECT_EQ(abWOutcome.error().Code(), "WORMConfigurationLocked");
}

TEST_F(BucketWormSettings, GetBucketWormResult)
{
    std::string xml = 
        R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <WormConfiguration>
          <WormId>ID</WormId>
          <State>Locked</State>
          <RetentionPeriodInDays>1</RetentionPeriodInDays>
          <CreationDate>2018-08-14T15:50:32</CreationDate>
        </WormConfiguration>
        )";
    auto result = GetBucketWormResult(xml);
    EXPECT_EQ(result.CreationDate(), "2018-08-14T15:50:32");
    EXPECT_EQ(result.WormId(), "ID");
    EXPECT_EQ(result.State(), "Locked");
    EXPECT_EQ(result.Day(), 1UL);

    xml =
        R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <WormConfiguration>
          <WormId>ID</WormId>
          <State>InProgress</State>
          <RetentionPeriodInDays>365</RetentionPeriodInDays>
          <CreationDate>2018-08-14T15:50:32</CreationDate>
        </WormConfiguration>
        )";
    result = GetBucketWormResult(xml);
    EXPECT_EQ(result.CreationDate(), "2018-08-14T15:50:32");
    EXPECT_EQ(result.WormId(), "ID");
    EXPECT_EQ(result.State(), "InProgress");
    EXPECT_EQ(result.Day(), 365UL);

    xml =
        R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <WormConfiguration>
          <WormId></WormId>
          <State></State>
          <RetentionPeriodInDays></RetentionPeriodInDays>
          <CreationDate></CreationDate>
        </WormConfiguration>
        )";
    result = GetBucketWormResult(xml);
    EXPECT_EQ(result.CreationDate(), "");
    EXPECT_EQ(result.WormId(), "");
    EXPECT_EQ(result.State(), "");
    EXPECT_EQ(result.Day(), 0UL);

    xml =
        R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <WormConfiguration>
        </WormConfiguration>
        )";
    result = GetBucketWormResult(xml);
    EXPECT_EQ(result.CreationDate(), "");
    EXPECT_EQ(result.WormId(), "");
    EXPECT_EQ(result.State(), "");
    EXPECT_EQ(result.Day(), 0UL);

    xml =
        R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <Invalid>
        </Invalid>
        )";
    result = GetBucketWormResult(xml);
    EXPECT_EQ(result.CreationDate(), "");
    EXPECT_EQ(result.WormId(), "");
    EXPECT_EQ(result.State(), "");
    EXPECT_EQ(result.Day(), 0UL);

    xml =
        R"(
        <?xml version="1.0" encoding="UTF-8"?>
        )";
    result = GetBucketWormResult(xml);
    EXPECT_EQ(result.CreationDate(), "");
    EXPECT_EQ(result.WormId(), "");
    EXPECT_EQ(result.State(), "");
    EXPECT_EQ(result.Day(), 0UL);
}


}
}