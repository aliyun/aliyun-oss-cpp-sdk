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

class BucketInventoryConfigurationTest : public ::testing::Test {
protected:
    BucketInventoryConfigurationTest()
    {
    }

    ~BucketInventoryConfigurationTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-inventory");
        DstBucketName = TestUtils::GetBucketName("cpp-sdk-inventory-dst");
        Client->CreateBucket(CreateBucketRequest(BucketName));
        Client->CreateBucket(CreateBucketRequest(DstBucketName));
        auto content = TestUtils::GetRandomStream(10);
        PutObjectRequest request(BucketName, "kms-key", content);
        request.MetaData().addHeader("x-oss-server-side-encryption", "KMS");
        auto outcome = Client->PutObject(request);
        auto metaOutcome = Client->HeadObject(BucketName, "kms-key");
        KmsKeyId = metaOutcome.result().HttpMetaData()["x-oss-server-side-encryption-key-id"];
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        Client->DeleteBucket(DeleteBucketRequest(BucketName));
        Client->DeleteBucket(DeleteBucketRequest(DstBucketName));
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
    static std::string DstBucketName;
    static std::string KmsKeyId;
};

std::shared_ptr<OssClient> BucketInventoryConfigurationTest::Client = nullptr;
std::string BucketInventoryConfigurationTest::BucketName = "";
std::string BucketInventoryConfigurationTest::DstBucketName = "";
std::string BucketInventoryConfigurationTest::KmsKeyId = "";

TEST_F(BucketInventoryConfigurationTest, BucketInventoryConfigurationAllTest)
{
    InventoryConfiguration conf;
    conf.setId("report1");
    conf.setIsEnabled(true);
    conf.setFilter(InventoryFilter("filterPrefix"));

    InventoryOSSBucketDestination dest;
    dest.setFormat(InventoryFormat::CSV);
    dest.setAccountId(Config::RamUID);
    dest.setRoleArn(Config::RamRoleArn);
    dest.setBucket(DstBucketName);
    dest.setPrefix("prefix1");
    dest.setEncryption(InventoryEncryption(InventorySSEKMS(KmsKeyId)));
    conf.setDestination(dest);

    conf.setSchedule(InventoryFrequency::Daily);
    conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

    InventoryOptionalFields field { 
        InventoryOptionalField::Size, InventoryOptionalField::LastModifiedDate, 
        InventoryOptionalField::ETag, InventoryOptionalField::StorageClass, 
        InventoryOptionalField::IsMultipartUploaded, InventoryOptionalField::EncryptionStatus
    };
    conf.setOptionalFields(field);

    auto setOutcome = Client->SetBucketInventoryConfiguration(SetBucketInventoryConfigurationRequest(BucketName, conf));
    EXPECT_EQ(setOutcome.isSuccess(), true);
     
    auto getOutcome = Client->GetBucketInventoryConfiguration(GetBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Id(), "report1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IsEnabled(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Filter().Prefix(), "filterPrefix");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().AccountId(), Config::RamUID);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().RoleArn(), Config::RamRoleArn);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Bucket(), DstBucketName);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEKMS(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().SSEKMS().KeyId(), KmsKeyId);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Schedule(), InventoryFrequency::Daily);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields().size(), 6U);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[2], InventoryOptionalField::ETag);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[3], InventoryOptionalField::StorageClass);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[4], InventoryOptionalField::IsMultipartUploaded);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[5], InventoryOptionalField::EncryptionStatus);

    auto delOutcome = Client->DeleteBucketInventoryConfiguration(DeleteBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(delOutcome.isSuccess(), true);

    //report2
    conf.setId("report2");
    conf.setIsEnabled(false);
    conf.setFilter(InventoryFilter("filterPrefix2"));
    InventoryOSSBucketDestination dest1;
    dest1.setFormat(InventoryFormat::CSV);
    dest1.setAccountId(Config::RamUID);
    dest1.setRoleArn(Config::RamRoleArn);
    dest1.setBucket(DstBucketName);
    dest1.setPrefix("prefix2");
    dest1.setEncryption(InventoryEncryption(InventorySSEOSS()));
    conf.setDestination(dest1);
    conf.setSchedule(InventoryFrequency::Weekly);
    conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::Current);
    InventoryOptionalFields field2 {
        InventoryOptionalField::Size, InventoryOptionalField::LastModifiedDate,
        InventoryOptionalField::ETag, InventoryOptionalField::StorageClass
    };
    conf.setOptionalFields(field2);

    setOutcome = Client->SetBucketInventoryConfiguration(SetBucketInventoryConfigurationRequest(BucketName, conf));
    EXPECT_EQ(setOutcome.isSuccess(), true);

    getOutcome = Client->GetBucketInventoryConfiguration(GetBucketInventoryConfigurationRequest(BucketName, "report2"));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Id(), "report2");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IsEnabled(), false);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Filter().Prefix(), "filterPrefix2");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().AccountId(), Config::RamUID);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().RoleArn(), Config::RamRoleArn);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Bucket(), DstBucketName);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Prefix(), "prefix2");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEKMS(), false);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEOSS(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Schedule(), InventoryFrequency::Weekly);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::Current);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields().size(), 4U);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[2], InventoryOptionalField::ETag);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[3], InventoryOptionalField::StorageClass);

    delOutcome = Client->DeleteBucketInventoryConfiguration(DeleteBucketInventoryConfigurationRequest(BucketName, "report2"));
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(BucketInventoryConfigurationTest, BucketInventoryConfigurationWithoutFilterTest)
{
    InventoryConfiguration conf;
    conf.setId("report1");
    conf.setIsEnabled(true);

    InventoryOSSBucketDestination dest;
    dest.setFormat(InventoryFormat::CSV);
    dest.setAccountId(Config::RamUID);
    dest.setRoleArn(Config::RamRoleArn);
    dest.setBucket(DstBucketName);
    dest.setPrefix("prefix1");
    dest.setEncryption(InventoryEncryption(InventorySSEKMS(KmsKeyId)));
    conf.setDestination(dest);

    conf.setSchedule(InventoryFrequency::Daily);
    conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

    InventoryOptionalFields field{
        InventoryOptionalField::Size, InventoryOptionalField::LastModifiedDate,
        InventoryOptionalField::ETag, InventoryOptionalField::StorageClass,
        InventoryOptionalField::IsMultipartUploaded, InventoryOptionalField::EncryptionStatus
    };
    conf.setOptionalFields(field);

    auto setOutcome = Client->SetBucketInventoryConfiguration(SetBucketInventoryConfigurationRequest(BucketName, conf));
    EXPECT_EQ(setOutcome.isSuccess(), true);

    auto getOutcome = Client->GetBucketInventoryConfiguration(GetBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Id(), "report1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IsEnabled(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Filter().Prefix(), "");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().AccountId(), Config::RamUID);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().RoleArn(), Config::RamRoleArn);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Bucket(), DstBucketName);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEKMS(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().SSEKMS().KeyId(), KmsKeyId);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Schedule(), InventoryFrequency::Daily);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields().size(), 6U);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[2], InventoryOptionalField::ETag);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[3], InventoryOptionalField::StorageClass);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[4], InventoryOptionalField::IsMultipartUploaded);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[5], InventoryOptionalField::EncryptionStatus);

    auto delOutcome = Client->DeleteBucketInventoryConfiguration(DeleteBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(BucketInventoryConfigurationTest, BucketInventoryConfigurationWithoutEncryptionTest)
{
    InventoryConfiguration conf;
    conf.setId("report1");
    conf.setIsEnabled(true);

    InventoryOSSBucketDestination dest;
    dest.setFormat(InventoryFormat::CSV);
    dest.setAccountId(Config::RamUID);
    dest.setRoleArn(Config::RamRoleArn);
    dest.setBucket(DstBucketName);
    dest.setPrefix("prefix1");
    conf.setDestination(dest);

    conf.setSchedule(InventoryFrequency::Daily);
    conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

    InventoryOptionalFields field{
        InventoryOptionalField::Size, InventoryOptionalField::LastModifiedDate,
        InventoryOptionalField::ETag, InventoryOptionalField::StorageClass,
        InventoryOptionalField::IsMultipartUploaded, InventoryOptionalField::EncryptionStatus
    };
    conf.setOptionalFields(field);

    auto setOutcome = Client->SetBucketInventoryConfiguration(SetBucketInventoryConfigurationRequest(BucketName, conf));
    EXPECT_EQ(setOutcome.isSuccess(), true);

    auto getOutcome = Client->GetBucketInventoryConfiguration(GetBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Id(), "report1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IsEnabled(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Filter().Prefix(), "");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().AccountId(), Config::RamUID);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().RoleArn(), Config::RamRoleArn);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Bucket(), DstBucketName);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEKMS(), false);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEOSS(), false);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Schedule(), InventoryFrequency::Daily);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields().size(), 6U);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[2], InventoryOptionalField::ETag);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[3], InventoryOptionalField::StorageClass);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[4], InventoryOptionalField::IsMultipartUploaded);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields()[5], InventoryOptionalField::EncryptionStatus);

    auto delOutcome = Client->DeleteBucketInventoryConfiguration(DeleteBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(BucketInventoryConfigurationTest, BucketInventoryConfigurationWithoutOptionalFieldsTest)
{
    InventoryConfiguration conf;
    conf.setId("report1");
    conf.setIsEnabled(true);

    InventoryOSSBucketDestination dest;
    dest.setFormat(InventoryFormat::CSV);
    dest.setAccountId(Config::RamUID);
    dest.setRoleArn(Config::RamRoleArn);
    dest.setBucket(DstBucketName);
    dest.setPrefix("prefix1");
    conf.setDestination(dest);

    conf.setSchedule(InventoryFrequency::Daily);
    conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

    auto setOutcome = Client->SetBucketInventoryConfiguration(SetBucketInventoryConfigurationRequest(BucketName, conf));
    EXPECT_EQ(setOutcome.isSuccess(), true);

    auto getOutcome = Client->GetBucketInventoryConfiguration(GetBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(getOutcome.isSuccess(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Id(), "report1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IsEnabled(), true);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Filter().Prefix(), "");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().AccountId(), Config::RamUID);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().RoleArn(), Config::RamRoleArn);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Bucket(), DstBucketName);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEKMS(), false);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEOSS(), false);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().Schedule(), InventoryFrequency::Daily);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(getOutcome.result().InventoryConfiguration().OptionalFields().size(), 0U);

    auto delOutcome = Client->DeleteBucketInventoryConfiguration(DeleteBucketInventoryConfigurationRequest(BucketName, "report1"));
    EXPECT_EQ(delOutcome.isSuccess(), true);
}

TEST_F(BucketInventoryConfigurationTest, ListBucketInventoryConfigurationTest)
{
    int i;
    for (i = 1; i < 102; i++) {
        InventoryConfiguration conf;
        conf.setId(std::to_string(i));
        conf.setIsEnabled( (i % 4)? true: false);
        conf.setFilter((i % 5) ? InventoryFilter("filterPrefix"): InventoryFilter());

        InventoryOSSBucketDestination dest;
        dest.setFormat(InventoryFormat::CSV);
        dest.setAccountId(Config::RamUID);
        dest.setRoleArn(Config::RamRoleArn);
        dest.setBucket(DstBucketName);
        dest.setPrefix("prefix1");
        dest.setEncryption(InventoryEncryption(InventorySSEKMS(KmsKeyId)));
        conf.setDestination(dest);

        conf.setSchedule(InventoryFrequency::Daily);
        conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

        InventoryOptionalFields field { 
            InventoryOptionalField::Size, InventoryOptionalField::LastModifiedDate,
            InventoryOptionalField::ETag, InventoryOptionalField::StorageClass,
            InventoryOptionalField::IsMultipartUploaded, InventoryOptionalField::EncryptionStatus 
        };
        InventoryOptionalFields field1 {
            InventoryOptionalField::Size
        };
        conf.setOptionalFields((i % 7) ? field: field1);

        SetBucketInventoryConfigurationRequest request(BucketName);
        request.setInventoryConfiguration(conf);
        auto setoutcome = Client->SetBucketInventoryConfiguration(request);
        EXPECT_EQ(setoutcome.isSuccess(), true);
    }

    ListBucketInventoryConfigurationsRequest listrequest(BucketName);
    auto listOutcome = Client->ListBucketInventoryConfigurations(listrequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);
    EXPECT_EQ(listOutcome.result().NextContinuationToken(), "98");
    EXPECT_EQ(listOutcome.result().IsTruncated(), true);
    EXPECT_EQ(listOutcome.result().InventoryConfigurationList().size(), 100U);
    for (const auto& conf : listOutcome.result().InventoryConfigurationList()) {
        int j = std::strtol(conf.Id().c_str(), nullptr, 10);
        EXPECT_EQ(conf.IsEnabled(), ((j % 4) ? true : false));
        EXPECT_EQ(conf.Filter().Prefix(), ((j % 5) ? "filterPrefix" : ""));
        EXPECT_EQ(conf.Destination().OSSBucketDestination().AccountId(), Config::RamUID);
        EXPECT_EQ(conf.Destination().OSSBucketDestination().RoleArn(), Config::RamRoleArn);
        EXPECT_EQ(conf.Destination().OSSBucketDestination().Bucket(), DstBucketName);
        EXPECT_EQ(conf.Destination().OSSBucketDestination().Prefix(), "prefix1");
        EXPECT_EQ(conf.Destination().OSSBucketDestination().Encryption().hasSSEKMS(), true);
        EXPECT_EQ(conf.Destination().OSSBucketDestination().Encryption().SSEKMS().KeyId(), KmsKeyId);
        EXPECT_EQ(conf.Schedule(), InventoryFrequency::Daily);
        EXPECT_EQ(conf.IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
        EXPECT_EQ(conf.OptionalFields().size(), ((j % 7)? 6U : 1U));
        EXPECT_EQ(conf.OptionalFields()[0], InventoryOptionalField::Size);
        if(conf.OptionalFields().size() > 1U) {
            EXPECT_EQ(conf.OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
            EXPECT_EQ(conf.OptionalFields()[2], InventoryOptionalField::ETag);
            EXPECT_EQ(conf.OptionalFields()[3], InventoryOptionalField::StorageClass);
            EXPECT_EQ(conf.OptionalFields()[4], InventoryOptionalField::IsMultipartUploaded);
            EXPECT_EQ(conf.OptionalFields()[5], InventoryOptionalField::EncryptionStatus);
        }
    }

    listrequest.setContinuationToken(listOutcome.result().NextContinuationToken());
    listOutcome = Client->ListBucketInventoryConfigurations(listrequest);
    EXPECT_EQ(listOutcome.result().NextContinuationToken(), "");
    EXPECT_EQ(listOutcome.result().IsTruncated(), false);
    EXPECT_EQ(listOutcome.result().InventoryConfigurationList().size(), 1U);
    EXPECT_EQ(listOutcome.result().InventoryConfigurationList()[0].Id(), "99");

    for (i = 1; i < 102; i++) {
        DeleteBucketInventoryConfigurationRequest delrequest(BucketName);
        delrequest.setId(std::to_string(i));
        auto delOutcome = Client->DeleteBucketInventoryConfiguration(delrequest);
        EXPECT_EQ(delOutcome.isSuccess(), true);
    }
}

TEST_F(BucketInventoryConfigurationTest, BucketInventoryConfigurationNegativeTest)
{
    InventoryConfiguration conf;
    conf.setId("report1");
    conf.setIsEnabled(true);

    InventoryOSSBucketDestination dest;
    dest.setFormat(InventoryFormat::CSV);
    dest.setAccountId(Config::RamUID);
    dest.setRoleArn(Config::RamRoleArn);
    dest.setBucket("not-exist-bucket");
    dest.setPrefix("prefix1");
    conf.setDestination(dest);

    conf.setSchedule(InventoryFrequency::Daily);
    conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

    auto setOutcome = Client->SetBucketInventoryConfiguration(SetBucketInventoryConfigurationRequest(BucketName, conf));
    EXPECT_EQ(setOutcome.isSuccess(), false);
    EXPECT_EQ(setOutcome.error().Code(), "InvalidArgument");

    auto getOutcome = Client->GetBucketInventoryConfiguration(GetBucketInventoryConfigurationRequest(BucketName, "not-exist-report-id"));
    EXPECT_EQ(getOutcome.isSuccess(), false);
    EXPECT_EQ(getOutcome.error().Code(), "NoSuchInventory");

    auto delOutcome = Client->DeleteBucketInventoryConfiguration(DeleteBucketInventoryConfigurationRequest(BucketName + "-not-exist-bucket", "not-exist-report-id"));
    EXPECT_EQ(delOutcome.isSuccess(), false);
    EXPECT_EQ(delOutcome.error().Code(), "NoSuchBucket");

    delOutcome = Client->DeleteBucketInventoryConfiguration(DeleteBucketInventoryConfigurationRequest(BucketName, "not-exist-report-id"));
    EXPECT_EQ(delOutcome.isSuccess(), true);

    auto listOutcome = Client->ListBucketInventoryConfigurations(ListBucketInventoryConfigurationsRequest(BucketName + "-not-exist-bucket"));
    EXPECT_EQ(listOutcome.isSuccess(), false);
    EXPECT_EQ(listOutcome.error().Code(), "NoSuchBucket");
}

TEST_F(BucketInventoryConfigurationTest, InventoryFilter)
{
    InventoryFilter filter;
    EXPECT_EQ(filter.Prefix(), "");

    filter.setPrefix("filter");
    EXPECT_EQ(filter.Prefix(), "filter");

    InventoryFilter filter1("filter1");
    EXPECT_EQ(filter1.Prefix(), "filter1");
}

TEST_F(BucketInventoryConfigurationTest, InventorySSEKMS)
{
    InventorySSEKMS ssekms;
    EXPECT_EQ(ssekms.KeyId(), "");

    ssekms.setKeyId("Id");
    EXPECT_EQ(ssekms.KeyId(), "Id");

    InventorySSEKMS ssekms1("id1");
    EXPECT_EQ(ssekms1.KeyId(), "id1");
}

TEST_F(BucketInventoryConfigurationTest, InventoryEncryption)
{
    InventorySSEOSS sseoss;
    InventorySSEKMS ssekms("keyId");
    InventoryEncryption encryption;
    EXPECT_EQ(encryption.hasSSEOSS(), false);
    EXPECT_EQ(encryption.hasSSEKMS(), false);
    
    encryption = InventoryEncryption();
    encryption.setSSEOSS(sseoss);
    EXPECT_EQ(encryption.hasSSEOSS(), true);
    EXPECT_EQ(encryption.hasSSEKMS(), false);

    encryption = InventoryEncryption();
    encryption.setSSEKMS(ssekms);
    EXPECT_EQ(encryption.hasSSEOSS(), false);
    EXPECT_EQ(encryption.hasSSEKMS(), true);
    EXPECT_EQ(encryption.SSEKMS().KeyId(), "keyId");

    InventoryEncryption encryption1(sseoss);
    EXPECT_EQ(encryption1.hasSSEOSS(), true);
    EXPECT_EQ(encryption1.hasSSEKMS(), false);

    InventoryEncryption encryption2(InventorySSEKMS("keyId"));
    EXPECT_EQ(encryption2.hasSSEOSS(), false);
    EXPECT_EQ(encryption2.hasSSEKMS(), true);
    EXPECT_EQ(encryption2.SSEKMS().KeyId(), "keyId");
}

TEST_F(BucketInventoryConfigurationTest, InventoryOSSBucketDestination)
{
    InventoryOSSBucketDestination ossBucket;
    EXPECT_EQ(ossBucket.Format(), InventoryFormat::NotSet);
    EXPECT_EQ(ossBucket.AccountId(), "");
    EXPECT_EQ(ossBucket.RoleArn(), "");
    EXPECT_EQ(ossBucket.Bucket(), "");
    EXPECT_EQ(ossBucket.Prefix(), "");
    EXPECT_EQ(ossBucket.Encryption().hasSSEKMS(), false);
    EXPECT_EQ(ossBucket.Encryption().hasSSEOSS(), false);

    ossBucket.setAccountId("AccountId");
    ossBucket.setRoleArn("RoleArn");
    ossBucket.setBucket("Bucket");
    ossBucket.setPrefix("Prefix");
    ossBucket.setEncryption(InventoryEncryption(InventorySSEKMS("keyId")));
    EXPECT_EQ(ossBucket.AccountId(), "AccountId");
    EXPECT_EQ(ossBucket.RoleArn(), "RoleArn");
    EXPECT_EQ(ossBucket.Bucket(), "Bucket");
    EXPECT_EQ(ossBucket.Prefix(), "Prefix");
    EXPECT_EQ(ossBucket.Encryption().hasSSEKMS(), true);
    EXPECT_EQ(ossBucket.Encryption().SSEKMS().KeyId(), "keyId");
    EXPECT_EQ(ossBucket.Encryption().hasSSEOSS(), false);
}

TEST_F(BucketInventoryConfigurationTest, InventoryDestination)
{
    InventoryDestination destination;
    EXPECT_EQ(destination.OSSBucketDestination().Format(), InventoryFormat::NotSet);
    EXPECT_EQ(destination.OSSBucketDestination().AccountId(), "");
    EXPECT_EQ(destination.OSSBucketDestination().RoleArn(), "");
    EXPECT_EQ(destination.OSSBucketDestination().Bucket(), "");
    EXPECT_EQ(destination.OSSBucketDestination().Prefix(), "");
    EXPECT_EQ(destination.OSSBucketDestination().Encryption().hasSSEKMS(), false);
    EXPECT_EQ(destination.OSSBucketDestination().Encryption().hasSSEOSS(), false);

    InventoryOSSBucketDestination ossBucket;
    ossBucket.setFormat(InventoryFormat::CSV);
    ossBucket.setAccountId("AccountId");
    destination.setOSSBucketDestination(ossBucket);
    EXPECT_EQ(destination.OSSBucketDestination().Format(), InventoryFormat::CSV);
    EXPECT_EQ(destination.OSSBucketDestination().AccountId(), "AccountId");
    EXPECT_EQ(destination.OSSBucketDestination().RoleArn(), "");
    EXPECT_EQ(destination.OSSBucketDestination().Bucket(), "");
    EXPECT_EQ(destination.OSSBucketDestination().Prefix(), "");
    EXPECT_EQ(destination.OSSBucketDestination().Encryption().hasSSEKMS(), false);
    EXPECT_EQ(destination.OSSBucketDestination().Encryption().hasSSEOSS(), false);
}

TEST_F(BucketInventoryConfigurationTest, InventoryConfiguration)
{
    InventoryConfiguration configuration;
    EXPECT_EQ(configuration.Id(), "");
    EXPECT_EQ(configuration.IsEnabled(), false);
    EXPECT_EQ(configuration.Schedule(), InventoryFrequency::NotSet);
    EXPECT_EQ(configuration.IncludedObjectVersions(), InventoryIncludedObjectVersions::NotSet);
}

TEST_F(BucketInventoryConfigurationTest, GetBucketInventoryConfigurationResult)
{
    std::string xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
            <Id>report1</Id>
            <IsEnabled>true</IsEnabled>
            <Filter>
                <Prefix>filterPrefix</Prefix>
            </Filter>
            <Destination>
                <OSSBucketDestination>
                    <Format>CSV</Format>
                    <AccountId>123456789012</AccountId>
                    <RoleArn>xxx</RoleArn>
                    <Bucket>acs:oss:::destination-bucket</Bucket>
                    <Prefix>prefix1</Prefix>
                    <Encryption>
                    <SSE-KMS>
                        <KeyId>keyId</KeyId>
                    </SSE-KMS>
                    </Encryption>
                </OSSBucketDestination>
            </Destination>
            <Schedule>
                <Frequency>Daily</Frequency>
            </Schedule>
            <IncludedObjectVersions>All</IncludedObjectVersions>
            <OptionalFields>
                <Field>Size</Field>
                <Field>LastModifiedDate</Field>
                <Field>ETag</Field>
                <Field>StorageClass</Field>
                <Field>IsMultipartUploaded</Field>
                <Field>EncryptionStatus</Field>
            </OptionalFields>
        </InventoryConfiguration>)";
    GetBucketInventoryConfigurationResult result(xml);
    EXPECT_EQ(result.InventoryConfiguration().Id(), "report1");
    EXPECT_EQ(result.InventoryConfiguration().IsEnabled(), true);
    EXPECT_EQ(result.InventoryConfiguration().Filter().Prefix(), "filterPrefix");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Format(), InventoryFormat::CSV);
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().AccountId(), "123456789012");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().RoleArn(), "xxx");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Bucket(), "destination-bucket");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEKMS(), true);
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEOSS(), false);
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Encryption().SSEKMS().KeyId(), "keyId");
    EXPECT_EQ(result.InventoryConfiguration().Schedule(), InventoryFrequency::Daily);
    EXPECT_EQ(result.InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields().size(), 6U);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[2], InventoryOptionalField::ETag);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[3], InventoryOptionalField::StorageClass);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[4], InventoryOptionalField::IsMultipartUploaded);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[5], InventoryOptionalField::EncryptionStatus);

    xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
            <Id>report2</Id>
            <IsEnabled>false</IsEnabled>
            <Filter>
                <Prefix/>
            </Filter>
            <Destination>
                <OSSBucketDestination>
                    <Format>CSV</Format>
                    <AccountId>123456789012</AccountId>
                    <RoleArn>xxx</RoleArn>
                    <Bucket>acs:oss:::destination-bucket1</Bucket>
                    <Prefix>prefix1</Prefix>
                    <Encryption>
                        <SSE-OSS/>
                    </Encryption>
                </OSSBucketDestination>
            </Destination>
            <Schedule>
                <Frequency>Weekly</Frequency>
            </Schedule>
            <IncludedObjectVersions>Current</IncludedObjectVersions>
            <OptionalFields>
                <Field>Size</Field>
                <Field>LastModifiedDate</Field>
                <Field>ETag</Field>
            </OptionalFields>
        </InventoryConfiguration>)";
    result = GetBucketInventoryConfigurationResult(xml);
    EXPECT_EQ(result.InventoryConfiguration().Id(), "report2");
    EXPECT_EQ(result.InventoryConfiguration().IsEnabled(), false);
    EXPECT_EQ(result.InventoryConfiguration().Filter().Prefix(), "");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Format(), InventoryFormat::CSV);
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().AccountId(), "123456789012");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().RoleArn(), "xxx");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Bucket(), "destination-bucket1");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEKMS(), false);
    EXPECT_EQ(result.InventoryConfiguration().Destination().OSSBucketDestination().Encryption().hasSSEOSS(), true);
    EXPECT_EQ(result.InventoryConfiguration().Schedule(), InventoryFrequency::Weekly);
    EXPECT_EQ(result.InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::Current);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields().size(), 3U);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[2], InventoryOptionalField::ETag);
}

TEST_F(BucketInventoryConfigurationTest, ListBucketInventoryConfigurationsResult)
{
    std::string xml = R"(
        <?xml version="1.0" ?>
        <ListInventoryConfigurationsResult>
            <InventoryConfiguration>
                <Id>report1</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                    <Prefix>filterPrefix</Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                    <Format>CSV</Format>
                    <AccountId>123456789012</AccountId>
                    <RoleArn>xxx</RoleArn>
                    <Bucket>acs:oss:::destination-bucket</Bucket>
                    <Prefix>prefix1</Prefix>
                    <Encryption>
                        <SSE-KMS>
                            <KeyId>keyId</KeyId>
                        </SSE-KMS>
                    </Encryption>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency>Daily</Frequency>
                </Schedule>
                <IncludedObjectVersions>All</IncludedObjectVersions>
                <OptionalFields>
                    <Field>Size</Field>
                    <Field>LastModifiedDate</Field>
                    <Field>ETag</Field>
                    <Field>StorageClass</Field>
                    <Field>IsMultipartUploaded</Field>
                    <Field>EncryptionStatus</Field>
                </OptionalFields>
            </InventoryConfiguration>
            <InventoryConfiguration>
                <Id>report2</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                <Prefix>filterPrefix</Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                        <Format>CSV</Format>
                        <AccountId>123456789012</AccountId>
                        <RoleArn>xxx</RoleArn>
                        <Bucket>acs:oss:::destination-bucket</Bucket>
                        <Prefix>prefix1</Prefix>
                        <Encryption>
                        <SSE-OSS/>
                        </Encryption>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency>Weekly</Frequency>
                </Schedule>
                <IncludedObjectVersions>Current</IncludedObjectVersions>
                <OptionalFields>
                    <Field>Size</Field>
                    <Field>LastModifiedDate</Field>
                    <Field>ETag</Field>
                    <Field>StorageClass</Field>
                    <Field>IsMultipartUploaded</Field>
                </OptionalFields>
            </InventoryConfiguration>
            <IsTruncated>true</IsTruncated>
            <NextContinuationToken>test</NextContinuationToken>
        </ListInventoryConfigurationsResult>)";

    ListBucketInventoryConfigurationsResult result(xml);
    EXPECT_EQ(result.InventoryConfigurationList()[0].Id(), "report1");
    EXPECT_EQ(result.InventoryConfigurationList()[0].IsEnabled(), true);
    EXPECT_EQ(result.InventoryConfigurationList()[0].Filter().Prefix(), "filterPrefix");
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().Format(), InventoryFormat::CSV);
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().AccountId(), "123456789012");
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().RoleArn(), "xxx");
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().Bucket(), "destination-bucket");
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().Encryption().hasSSEKMS(), true);
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().Encryption().hasSSEOSS(), false);
    EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().OSSBucketDestination().Encryption().SSEKMS().KeyId(), "keyId");
    EXPECT_EQ(result.InventoryConfigurationList()[0].Schedule(), InventoryFrequency::Daily);
    EXPECT_EQ(result.InventoryConfigurationList()[0].IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields().size(), 6U);
    EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[2], InventoryOptionalField::ETag);
    EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[3], InventoryOptionalField::StorageClass);
    EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[4], InventoryOptionalField::IsMultipartUploaded);
    EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[5], InventoryOptionalField::EncryptionStatus);

    EXPECT_EQ(result.InventoryConfigurationList()[1].Id(), "report2");
    EXPECT_EQ(result.InventoryConfigurationList()[1].IsEnabled(), true);
    EXPECT_EQ(result.InventoryConfigurationList()[1].Filter().Prefix(), "filterPrefix");
    EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().OSSBucketDestination().Format(), InventoryFormat::CSV);
    EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().OSSBucketDestination().AccountId(), "123456789012");
    EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().OSSBucketDestination().RoleArn(), "xxx");
    EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().OSSBucketDestination().Bucket(), "destination-bucket");
    EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().OSSBucketDestination().Prefix(), "prefix1");
    EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().OSSBucketDestination().Encryption().hasSSEKMS(), false);
    EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().OSSBucketDestination().Encryption().hasSSEOSS(), true);
    EXPECT_EQ(result.InventoryConfigurationList()[1].Schedule(), InventoryFrequency::Weekly);
    EXPECT_EQ(result.InventoryConfigurationList()[1].IncludedObjectVersions(), InventoryIncludedObjectVersions::Current);
    EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields().size(), 5U);
    EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[0], InventoryOptionalField::Size);
    EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[1], InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[2], InventoryOptionalField::ETag);
    EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[3], InventoryOptionalField::StorageClass);
    EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[4], InventoryOptionalField::IsMultipartUploaded);

    EXPECT_EQ(result.IsTruncated(), true);
    EXPECT_EQ(result.NextContinuationToken(), "test");

}

TEST_F(BucketInventoryConfigurationTest, GetBucketInventoryConfigurationResultBranchTest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    GetBucketInventoryConfigurationResult result(xml);

    GetBucketInventoryConfigurationResult result1("test");

    xml = R"(
        <?xml version="1.0" ?>
        <Inventory>
        </Inventory>)";
    GetBucketInventoryConfigurationResult result2(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
        </InventoryConfiguration>)";
    GetBucketInventoryConfigurationResult result3(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
            <Id></Id>
            <IsEnabled></IsEnabled>
            <Filter>
            </Filter>
            <Destination>
            </Destination>
            <Schedule>
            </Schedule>
            <IncludedObjectVersions></IncludedObjectVersions>
            <OptionalFields>
            </OptionalFields>
        </InventoryConfiguration>)";
    GetBucketInventoryConfigurationResult result4(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
            <Id>report1</Id>
            <IsEnabled>true</IsEnabled>
            <Filter>
                <Prefix></Prefix>
            </Filter>
            <Destination>
                <OSSBucketDestination>

                </OSSBucketDestination>
            </Destination>
            <Schedule>
                <Frequency></Frequency>
            </Schedule>
            <IncludedObjectVersions>All</IncludedObjectVersions>
            <OptionalFields>
                <Field></Field>
                <Field></Field>
                <Field></Field>
                <Field></Field>
                <Field></Field>
                <Field></Field>
            </OptionalFields>
        </InventoryConfiguration>)";
    GetBucketInventoryConfigurationResult result5(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
            <Id>report1</Id>
            <IsEnabled>true</IsEnabled>
            <Filter>
                <Prefix>filterPrefix</Prefix>
            </Filter>
            <Destination>
                <OSSBucketDestination>
                    <Format></Format>
                    <AccountId></AccountId>
                    <RoleArn></RoleArn>
                    <Bucket></Bucket>
                    <Prefix></Prefix>
                    <Encryption>
                    </Encryption>
                </OSSBucketDestination>
            </Destination>
            <Schedule>
                <Frequency>Daily</Frequency>
            </Schedule>
            <IncludedObjectVersions>All</IncludedObjectVersions>
            <OptionalFields>
                <Field>Size</Field>
                <Field>LastModifiedDate</Field>
                <Field>ETag</Field>
                <Field>StorageClass</Field>
                <Field>IsMultipartUploaded</Field>
                <Field>EncryptionStatus</Field>
            </OptionalFields>
        </InventoryConfiguration>)";
    GetBucketInventoryConfigurationResult result6(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
            <Id>report1</Id>
            <IsEnabled>true</IsEnabled>
            <Filter>
                <Prefix>filterPrefix</Prefix>
            </Filter>
            <Destination>
                <OSSBucketDestination>
                    <Format>CSV</Format>
                    <AccountId>123456789012</AccountId>
                    <RoleArn>xxx</RoleArn>
                    <Bucket>acs:oss:::destination-bucket</Bucket>
                    <Prefix>prefix1</Prefix>
                    <Encryption>
                        <SSE-KMS>
                        </SSE-KMS>
                    </Encryption>
                </OSSBucketDestination>
            </Destination>
            <Schedule>
                <Frequency>Daily</Frequency>
            </Schedule>
            <IncludedObjectVersions>All</IncludedObjectVersions>
            <OptionalFields>
                <Field>Size</Field>
                <Field>LastModifiedDate</Field>
                <Field>ETag</Field>
                <Field>StorageClass</Field>
                <Field>IsMultipartUploaded</Field>
                <Field>EncryptionStatus</Field>
            </OptionalFields>
        </InventoryConfiguration>)";
    GetBucketInventoryConfigurationResult result7(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <InventoryConfiguration>
            <Id>report1</Id>
            <IsEnabled>true</IsEnabled>
            <Filter>
                <Prefix>filterPrefix</Prefix>
            </Filter>
            <Destination>
                <OSSBucketDestination>
                    <Format>CSV</Format>
                    <AccountId>123456789012</AccountId>
                    <RoleArn>xxx</RoleArn>
                    <Bucket>acs:oss:::destination-bucket</Bucket>
                    <Prefix>prefix1</Prefix>
                    <Encryption>
                        <SSE-KMS>
                            <KeyId></KeyId>
                        </SSE-KMS>
                    </Encryption>
                </OSSBucketDestination>
            </Destination>
            <Schedule>
                <Frequency>Daily</Frequency>
            </Schedule>
            <IncludedObjectVersions>All</IncludedObjectVersions>
            <OptionalFields>
                <Field>Size</Field>
                <Field>LastModifiedDate</Field>
                <Field>ETag</Field>
                <Field>StorageClass</Field>
                <Field>IsMultipartUploaded</Field>
                <Field>EncryptionStatus</Field>
            </OptionalFields>
        </InventoryConfiguration>)";
    GetBucketInventoryConfigurationResult result8(xml);

}

TEST_F(BucketInventoryConfigurationTest, ListBucketInventoryConfigurationResultBranchtest)
{
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
    ListBucketInventoryConfigurationsResult result(xml);

    ListBucketInventoryConfigurationsResult result1("test");

    xml = R"(
        <?xml version="1.0" ?>
        <ListInventoryConfigurations>
        </ListInventoryConfigurations>)";
    ListBucketInventoryConfigurationsResult result2(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <ListInventoryConfigurationsResult>
        </ListInventoryConfigurationsResult>)";
    ListBucketInventoryConfigurationsResult result3(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <ListInventoryConfigurationsResult>
            <InventoryConfiguration>                        
            </InventoryConfiguration>
            <IsTruncated></IsTruncated>
            <NextContinuationToken></NextContinuationToken>
        </ListInventoryConfigurationsResult>)";
    ListBucketInventoryConfigurationsResult result4(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <ListInventoryConfigurationsResult>
            <InventoryConfiguration>
                <Id></Id>
                <IsEnabled></IsEnabled>
                <Filter>
                </Filter>
                <Destination>
                </Destination>
                <Schedule>
                </Schedule>
                <IncludedObjectVersions></IncludedObjectVersions>
                <OptionalFields>
                </OptionalFields>
            </InventoryConfiguration>
            <InventoryConfiguration>
                <Id></Id>
                <IsEnabled></IsEnabled>
                <Filter>
                </Filter>
                <Destination>
                </Destination>
                <Schedule>
                </Schedule>
                <IncludedObjectVersions></IncludedObjectVersions>
                <OptionalFields>
                </OptionalFields>
            </InventoryConfiguration>
            <IsTruncated>true</IsTruncated>
            <NextContinuationToken>test</NextContinuationToken>
        </ListInventoryConfigurationsResult>)";
    ListBucketInventoryConfigurationsResult result5(xml);

    xml = R"(
        <?xml version="1.0" ?>
        <ListInventoryConfigurationsResult>
            <InventoryConfiguration>
                <Id>report1</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                    <Prefix></Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency></Frequency>
                </Schedule>
                <IncludedObjectVersions>All</IncludedObjectVersions>
                <OptionalFields>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                </OptionalFields>
            </InventoryConfiguration>
            <InventoryConfiguration>
                <Id>report2</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                    <Prefix></Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency></Frequency>
                </Schedule>
                <IncludedObjectVersions>All</IncludedObjectVersions>
                <OptionalFields>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                    <Field></Field>
                </OptionalFields>
            </InventoryConfiguration>
            <IsTruncated>true</IsTruncated>
            <NextContinuationToken>test</NextContinuationToken>
        </ListInventoryConfigurationsResult>)";
    ListBucketInventoryConfigurationsResult result6(xml);

    xml = R"(<?xml version="1.0" ?>
        <ListInventoryConfigurationsResult>
            <InventoryConfiguration>
                <Id>report1</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                    <Prefix>filterPrefix</Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                        <Format></Format>
                        <AccountId></AccountId>
                        <RoleArn></RoleArn>
                        <Bucket></Bucket>
                        <Prefix></Prefix>
                        <Encryption>
                        </Encryption>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency>Daily</Frequency>
                </Schedule>
                <IncludedObjectVersions>All</IncludedObjectVersions>
                <OptionalFields>
                    <Field>Size</Field>
                    <Field>LastModifiedDate</Field>
                    <Field>ETag</Field>
                    <Field>StorageClass</Field>
                    <Field>IsMultipartUploaded</Field>
                    <Field>EncryptionStatus</Field>
                </OptionalFields>
            </InventoryConfiguration>
            <InventoryConfiguration>
                <Id>report2</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                    <Prefix>filterPrefix</Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                        <Format>CSV</Format>
                        <AccountId>123456789012</AccountId>
                        <RoleArn>xxx</RoleArn>
                        <Bucket>acs:oss:::destination-bucket</Bucket>
                        <Prefix>prefix1</Prefix>
                        <Encryption>
                            <SSE-KMS>
                            </SSE-KMS>
                        </Encryption>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency>Daily</Frequency>
                </Schedule>
                <IncludedObjectVersions>All</IncludedObjectVersions>
                <OptionalFields>
                    <Field>Size</Field>
                    <Field>LastModifiedDate</Field>
                    <Field>ETag</Field>
                    <Field>StorageClass</Field>
                    <Field>IsMultipartUploaded</Field>
                    <Field>EncryptionStatus</Field>
                </OptionalFields>
            </InventoryConfiguration>
            <IsTruncated>true</IsTruncated>
            <NextContinuationToken>test</NextContinuationToken>
        </ListInventoryConfigurationsResult>)";
    ListBucketInventoryConfigurationsResult result7(xml);

    xml = R"(<?xml version="1.0" ?>
        <ListInventoryConfigurationsResult>
            <InventoryConfiguration>
                <Id>report1</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                    <Prefix>filterPrefix</Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                        <Format>CSV</Format>
                        <AccountId>123456789012</AccountId>
                        <RoleArn>xxx</RoleArn>
                        <Bucket>acs:oss:::destination-bucket</Bucket>
                        <Prefix>prefix1</Prefix>
                        <Encryption>
                            <SSE-KMS>
                            <KeyId></KeyId>
                            </SSE-KMS>
                        </Encryption>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency>Daily</Frequency>
                </Schedule>
                <IncludedObjectVersions>All</IncludedObjectVersions>
                <OptionalFields>
                    <Field>Size</Field>
                    <Field>LastModifiedDate</Field>
                    <Field>ETag</Field>
                    <Field>StorageClass</Field>
                    <Field>IsMultipartUploaded</Field>
                    <Field>EncryptionStatus</Field>
                </OptionalFields>
            </InventoryConfiguration>
            <InventoryConfiguration>
                <Id>report2</Id>
                <IsEnabled>true</IsEnabled>
                <Filter>
                    <Prefix>filterPrefix</Prefix>
                </Filter>
                <Destination>
                    <OSSBucketDestination>
                        <Format>CSV</Format>
                        <AccountId>123456789012</AccountId>
                        <RoleArn>xxx</RoleArn>
                        <Bucket>acs:oss:::destination-bucket</Bucket>
                        <Prefix>prefix1</Prefix>
                        <Encryption>
                            <SSE-KMS>
                                <KeyId></KeyId>
                            </SSE-KMS>
                        </Encryption>
                    </OSSBucketDestination>
                </Destination>
                <Schedule>
                    <Frequency>Daily</Frequency>
                </Schedule>
                <IncludedObjectVersions>All</IncludedObjectVersions>
                <OptionalFields>
                    <Field>Size</Field>
                    <Field>LastModifiedDate</Field>
                    <Field>ETag</Field>
                    <Field>StorageClass</Field>
                    <Field>IsMultipartUploaded</Field>
                    <Field>EncryptionStatus</Field>
                </OptionalFields>
            </InventoryConfiguration>
            <IsTruncated>true</IsTruncated>
            <NextContinuationToken>test</NextContinuationToken>
        </ListInventoryConfigurationsResult>)";
    ListBucketInventoryConfigurationsResult result8(xml);
}
}
}