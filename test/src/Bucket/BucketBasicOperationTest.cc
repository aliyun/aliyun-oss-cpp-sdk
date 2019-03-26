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

class BucketBasicOperationTest : public ::testing::Test {
protected:
    BucketBasicOperationTest()
    {
    }

    ~BucketBasicOperationTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase() 
    {
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
    static std::string BucketNamePrefix;
};

std::shared_ptr<OssClient> BucketBasicOperationTest::Client = nullptr;
std::string BucketBasicOperationTest::BucketNamePrefix = "cpp-sdk-bucketbasicoperation";
#define testPrefix  BucketBasicOperationTest::BucketNamePrefix
//testPrefix =  "cpp-sdk-bucketbasicoperation";

TEST_F(BucketBasicOperationTest, CreateBucketInvalidNameTest)
{
    for(auto const &name : TestUtils::InvalidBucketNamesList())
    {
        auto outcome = Client->CreateBucket(CreateBucketRequest(name));
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketBasicOperationTest, DeleteBucketInvalidNameTest)
{
    for (auto const &name : TestUtils::InvalidBucketNamesList())
    {
        auto outcome = Client->DeleteBucket(DeleteBucketRequest(name));
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
    }
}

TEST_F(BucketBasicOperationTest, CreateBucketWithAclTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);
    EXPECT_EQ(Client->DoesBucketExist(bucketName), false);

    //create a new bucket
    Client->CreateBucket(bucketName, StorageClass::Archive, CannedAccessControlList::PublicReadWrite);
    //TestUtils::WaitForCacheExpire(8);
    EXPECT_EQ(Client->DoesBucketExist(bucketName), true);

    auto outcome = Client->GetBucketAcl(bucketName);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Acl(), CannedAccessControlList::PublicReadWrite);

    Client->DeleteBucket(bucketName);
}

TEST_F(BucketBasicOperationTest, CreateAndDeleteArchiveBucketTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    //create a new bucket
    auto outcome = Client->CreateBucket(CreateBucketRequest(bucketName, StorageClass::Archive));
    //TestUtils::WaitForCacheExpire(8);
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), true);

    auto objectName = bucketName;
    objectName.append("firstobject");
    Client->PutObject(PutObjectRequest(bucketName, objectName, std::make_shared<std::stringstream>()));

    auto metaOutcome = Client->HeadObject(HeadObjectRequest(bucketName, objectName));
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_STREQ(metaOutcome.result().HttpMetaData().at("x-oss-storage-class").c_str(), "Archive");
    Client->DeleteObject(DeleteObjectRequest(bucketName, objectName));

    //delete the new created bucket
    Client->DeleteBucket(DeleteBucketRequest(bucketName));
    TestUtils::WaitForCacheExpire(5);
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);
}

TEST_F(BucketBasicOperationTest, CreateAndDeleteBucketDefaultRegionTest)
{
    //point to default region
    ClientConfiguration conf;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(client, bucketName), false);

    //create a new bucket
    client.CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(TestUtils::BucketExists(client, bucketName), true);

    //delete the bucket
    client.DeleteBucket(DeleteBucketRequest(bucketName));
    TestUtils::WaitForCacheExpire(5);
    EXPECT_EQ(TestUtils::BucketExists(client, bucketName), false);
}

TEST_F(BucketBasicOperationTest, CreateAndDeleteBucketTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    //create a new bucket
    Client->CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), true);

    //delete the bucket
    Client->DeleteBucket(DeleteBucketRequest(bucketName));
    TestUtils::WaitForCacheExpire(5);
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);
}

TEST_F(BucketBasicOperationTest, CreateAndDeleteIABucketTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    //create a new bucket
    auto outcome = Client->CreateBucket(CreateBucketRequest(bucketName, StorageClass::IA));
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), true);

    auto objectName = bucketName;
    objectName.append("firstobject");
    Client->PutObject(PutObjectRequest(bucketName, objectName, std::make_shared<std::stringstream>()));

    auto metaOutcome = Client->HeadObject(HeadObjectRequest(bucketName, objectName));
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_STREQ(metaOutcome.result().HttpMetaData().at("x-oss-storage-class").c_str(), "IA");
    Client->DeleteObject(DeleteObjectRequest(bucketName, objectName));

    //delete the new created bucket
    Client->DeleteBucket(DeleteBucketRequest(bucketName));
    TestUtils::WaitForCacheExpire(5);
    TestUtils::BucketExists(*Client, bucketName);
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);
}

TEST_F(BucketBasicOperationTest, GetBucketInfoTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    //create a new bucket
    Client->CreateBucket(CreateBucketRequest(bucketName, StorageClass::IA));

    GetBucketInfoRequest request(bucketName);
    auto bfOutcome = Client->GetBucketInfo(request);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().Acl(), CannedAccessControlList::Private);
    
    Client->SetBucketAcl(SetBucketAclRequest(bucketName, CannedAccessControlList::PublicRead));
    TestUtils::WaitForCacheExpire(5);
    bfOutcome = Client->GetBucketInfo(request);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().Acl(), CannedAccessControlList::PublicRead);

    Client->SetBucketAcl(SetBucketAclRequest(bucketName, CannedAccessControlList::PublicReadWrite));
    TestUtils::WaitForCacheExpire(5);
    bfOutcome = Client->GetBucketInfo(request);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);

    EXPECT_EQ(bfOutcome.result().Location().empty(), false);
    EXPECT_EQ(bfOutcome.result().ExtranetEndpoint().empty(), false);
    EXPECT_EQ(bfOutcome.result().IntranetEndpoint().empty(), false);

    EXPECT_EQ(bfOutcome.result().StorageClass(), StorageClass::IA);
    EXPECT_STREQ(bfOutcome.result().Name().c_str(), bucketName.c_str());

    //delete the bucket
    Client->DeleteBucket(DeleteBucketRequest(bucketName));
}

TEST_F(BucketBasicOperationTest, GetBucketLocationTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    //create a new bucket
    Client->CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), true);

    //get bucket location
    auto locOutcome = Client->GetBucketLocation(GetBucketLocationRequest(bucketName));
    EXPECT_EQ(locOutcome.isSuccess(), true);
    EXPECT_EQ(locOutcome.result().Location().compare(0,4,"oss-", 4), 0);

    //delete the bucket
    Client->DeleteBucket(DeleteBucketRequest(bucketName));
}

TEST_F(BucketBasicOperationTest, GetBucketLocationNegativeTest)
{
    auto outcome = Client->GetBucketLocation("no-exist-bucket-location");
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketBasicOperationTest, GetBucketStatTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    //create a new bucket
    Client->CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), true);

    //put object
    auto objectName = bucketName;
    objectName.append("firstobject");
    Client->PutObject(PutObjectRequest(bucketName, objectName, std::make_shared<std::stringstream>("1234")));

    auto bsOutcome = Client->GetBucketStat(GetBucketStatRequest(bucketName));
    EXPECT_EQ(bsOutcome.isSuccess(), true);
    EXPECT_EQ(bsOutcome.result().Storage(), 4ULL);
    EXPECT_EQ(bsOutcome.result().ObjectCount(), 1ULL);
    EXPECT_EQ(bsOutcome.result().MultipartUploadCount(), 0ULL);

    //delete the bucket
    Client->DeleteObject(DeleteObjectRequest(bucketName, objectName));
    Client->DeleteBucket(DeleteBucketRequest(bucketName));
}

TEST_F(BucketBasicOperationTest, GetNonExistBucketInfoTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    auto outcome = Client->GetBucketInfo(GetBucketInfoRequest(bucketName));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "NoSuchBucket");
}

TEST_F(BucketBasicOperationTest, GetNonExistBucketStatTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    auto outcome = Client->GetBucketStat(GetBucketStatRequest(bucketName));
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_STREQ(outcome.error().Code().c_str(), "NoSuchBucket");
}

TEST_F(BucketBasicOperationTest, ListBucketspagingTest)
{
    //get a random bucketName
    auto bucketName = TestUtils::GetBucketName(testPrefix);

    //assert bucket does not exist
    EXPECT_EQ(TestUtils::BucketExists(*Client, bucketName), false);

    for (int i = 0; i < 5; i++)
    {
        auto name = bucketName;
        name.append("-").append(std::to_string(i));
        Client->CreateBucket(CreateBucketRequest(name));
    }

    //list all
    ListBucketsRequest request;
    request.setPrefix(bucketName);
    request.setMaxKeys(100);
    auto outcome = Client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Buckets().size(), 5UL);

    outcome = Client->ListBuckets();
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_TRUE(outcome.result().Buckets().size() > 5UL);

    //list by step
    request.setMaxKeys(2);
    bool IsTruncated = false;
    size_t total = 0;
    do {
        outcome = Client->ListBuckets(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_LT(outcome.result().Buckets().size(), 3UL);
        total += outcome.result().Buckets().size();
        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
    } while (IsTruncated);
    EXPECT_EQ(total, 5UL);

    //delete all
    request.setMaxKeys(100);
    outcome = Client->ListBuckets(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    for (auto const &bucket : outcome.result().Buckets()) {
        Client->DeleteBucket(DeleteBucketRequest(bucket.Name()));
    }
}

TEST_F(BucketBasicOperationTest, GetBucketInfoResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketInfo>
                          <Bucket>
                            <CreationDate>2013-07-31T10:56:21.000Z</CreationDate>
                            <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                            <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                            <Location>oss-cn-hangzhou</Location>
                            <Name>oss-example</Name>
                            <Owner>
                              <DisplayName>username</DisplayName>
                              <ID>271834739143143</ID>
                            </Owner>
                            <AccessControlList>
                              <Grant>private</Grant>
                            </AccessControlList>
                          </Bucket>
                        </BucketInfo>)";
    GetBucketInfoResult result(xml);
    EXPECT_EQ(result.CreationDate(), "2013-07-31T10:56:21.000Z");
    EXPECT_EQ(result.Location(), "oss-cn-hangzhou");
    EXPECT_EQ(result.ExtranetEndpoint(), "oss-cn-hangzhou.aliyuncs.com");
    EXPECT_EQ(result.Acl(), CannedAccessControlList::Private);
}

TEST_F(BucketBasicOperationTest, ListBucketsResultTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListAllMyBucketsResult>
                          <Owner>
                            <ID>1305433695277957</ID>
                            <DisplayName>1305433695277957</DisplayName>
                          </Owner>
                          <Buckets>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:26.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-0</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:26.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-1</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:26.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-2</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:27.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-3</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                          </Buckets>
                        </ListAllMyBucketsResult>)";
    ListBucketsResult result(xml);
    EXPECT_EQ(result.Buckets().size(), 4UL);
}

TEST_F(BucketBasicOperationTest, ListBucketsResultWithEncodeTypeTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <ListAllMyBucketsResult>
                          <Owner>
                            <ID>1305433695277957</ID>
                            <DisplayName>1305433695277957</DisplayName>
                          </Owner>
                          <Buckets>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:26.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-0</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:26.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-1</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:26.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-2</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                            <Bucket>
                              <CreationDate>2018-10-27T07:42:27.000Z</CreationDate>
                              <ExtranetEndpoint>oss-cn-hangzhou.aliyuncs.com</ExtranetEndpoint>
                              <IntranetEndpoint>oss-cn-hangzhou-internal.aliyuncs.com</IntranetEndpoint>
                              <Location>oss-cn-hangzhou</Location>
                              <Name>cpp-sdk-bucketbasicoperation-bucket-1540626145748-3</Name>
                              <StorageClass>Standard</StorageClass>
                            </Bucket>
                          </Buckets>
                        </ListAllMyBucketsResult>)";
    ListBucketsResult result(xml);
    EXPECT_EQ(result.Buckets().size(), 4UL);
}


TEST_F(BucketBasicOperationTest, GetBucketStatResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <BucketStat>
                          <Storage>1024123</Storage>
                          <ObjectCount>1000</ObjectCount>
                          <MultipartUploadCount>20</MultipartUploadCount>
                        </BucketStat>)";
    GetBucketStatResult result(xml);
    EXPECT_EQ(result.MultipartUploadCount(), 20ULL);
    EXPECT_EQ(result.ObjectCount(), 1000ULL);
    EXPECT_EQ(result.Storage(), 1024123ULL);
}

TEST_F(BucketBasicOperationTest, GetBucketLocationResult)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
                        <LocationConstraint xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">oss-cn-hangzhou</LocationConstraint>)";
    GetBucketLocationResult result(xml);
    EXPECT_EQ(result.Location(), "oss-cn-hangzhou");
}

}
}