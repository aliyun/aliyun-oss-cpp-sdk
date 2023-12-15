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
#include "src/utils/Utils.h"

namespace AlibabaCloud {
namespace OSS {

class EndpointTest : public ::testing::Test {
protected:
    EndpointTest()
    {
    }

    ~EndpointTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-endpoint");
        Client->CreateBucket(CreateBucketRequest(BucketName));
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase() 
    {
        OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        TestUtils::CleanBucket(client, BucketName);
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

std::shared_ptr<OssClient> EndpointTest::Client = nullptr;
std::string EndpointTest::BucketName = "";


TEST_F(EndpointTest, PathStyleTest)
{
    auto conf = ClientConfiguration();
    conf.isPathStyle = true;
    auto client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    auto gboutcome = client->GetBucketAcl(BucketName);
    EXPECT_EQ(gboutcome.isSuccess(), false);
    EXPECT_EQ(gboutcome.error().Code(), "SecondLevelDomainForbidden");

    auto hooutcome = client->GetObject(BucketName, "no-exist-key");
    EXPECT_EQ(hooutcome.isSuccess(), false);
    EXPECT_EQ(hooutcome.error().Code(), "SecondLevelDomainForbidden");

    auto looutcome = client->ListObjects(BucketName);
    EXPECT_EQ(looutcome.isSuccess(), false);
    EXPECT_EQ(looutcome.error().Code(), "SecondLevelDomainForbidden");

    ListBucketsRequest lbrequest;
    lbrequest.setPrefix(BucketName);
    lbrequest.setMaxKeys(100);
    auto lboutcome = client->ListBuckets(lbrequest);
    EXPECT_EQ(lboutcome.isSuccess(), true);
    EXPECT_EQ(lboutcome.result().Buckets().size(), 1UL);

    GeneratePresignedUrlRequest request(BucketName, "no-exist-key", Http::Get);
    auto urlOutcome = client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    std::string path = "/" + BucketName + "/no-exist-key";
    EXPECT_EQ(urlOutcome.result().find(path.c_str()) != std::string::npos, true);
}

TEST_F(EndpointTest, CnameTest)
{
    Url url(Config::Endpoint);

    auto cnameEndpoint = BucketName + "." + url.authority();

    auto conf = ClientConfiguration();
    conf.isCname = true;
    auto client = std::make_shared<OssClient>(cnameEndpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    auto gboutcome = client->GetBucketAcl(BucketName);
    EXPECT_EQ(gboutcome.isSuccess(), true);
    EXPECT_EQ(gboutcome.result().Acl(), CannedAccessControlList::Private);

    auto hooutcome = client->GetObject(BucketName, "no-exist-key");
    EXPECT_EQ(hooutcome.isSuccess(),  false);
    EXPECT_EQ(hooutcome.error().Code(), "NoSuchKey");

    auto looutcome = client->ListObjects(BucketName);
    EXPECT_EQ(looutcome.isSuccess(), true);

    ListBucketsRequest lbrequest;
    lbrequest.setPrefix(BucketName);
    lbrequest.setMaxKeys(100);
    auto lboutcome = client->ListBuckets(lbrequest);
    EXPECT_EQ(lboutcome.isSuccess(), false);
    EXPECT_EQ(lboutcome.error().Code(), "SignatureDoesNotMatch");
    EXPECT_EQ(lboutcome.error().Host(), cnameEndpoint);

    GeneratePresignedUrlRequest request(BucketName, "no-exist-key", Http::Get);
    auto urlOutcome = client->GeneratePresignedUrl(request);
    EXPECT_EQ(urlOutcome.isSuccess(), true);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "NoSuchKey");

}

}
}