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
            //Client = std::make_shared<OssClient>("10.101.201.13:8186", "LTAIumgOGzWCux6U", "jtEW2hGstlILuILgbWkQSfvJdT8SwC", ClientConfiguration());
            BucketName = TestUtils::GetBucketName("cppsdkbucketInventoryConfiguration");
            //BucketName = "lyxfortest1";
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

    std::shared_ptr<OssClient> BucketInventoryConfigurationTest::Client = nullptr;
    std::string BucketInventoryConfigurationTest::BucketName = "";

    TEST_F(BucketInventoryConfigurationTest, SetAndDeleteBucketInventoryConfigurationTest)
    {
        SetBucketInventoryConfigurationRequest setrequest(BucketName);
        InventoryConfiguration conf;
        conf.setId("report1");
        conf.setIsEnabled(true);
        Filter filter;
        filter.setPrefix("filterPrefix");
        conf.setFilter(filter);

        OSSBucketDestination dest;
        dest.setFormat("CSV");
        dest.setAccountId(Config::AccountId);
        dest.setRoleArn(Config::OssStsArn);
        dest.setBucket("acs:oss:::lyxfortest");
        dest.setPrefix("prefix1");
        dest.setEncryption("keyId");

        conf.setDestination(dest);
        conf.setSchedule(InventoryFrequency::Daily);
        conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

        OptionalFields field{ InventoryOptionalFields::Size, InventoryOptionalFields::LastModifiedDate, InventoryOptionalFields::ETag, InventoryOptionalFields::StorageClass, InventoryOptionalFields::IsMultipartUploaded, InventoryOptionalFields::EncryptionStatus};
        conf.setOptionalFields(field);

        setrequest.setInventoryConfiguration(conf);
        auto setoutcome = Client->SetBucketInventoryConfiguration(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        DeleteBucketInventoryConfigurationRequest delrequest(BucketName);
        delrequest.setId("report1");
        auto deloutcome = Client->DeleteBucketInventoryConfiguration(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketInventoryConfigurationTest, GetBucketInventoryConfigurationTest)
    {
        SetBucketInventoryConfigurationRequest setrequest(BucketName);
        InventoryConfiguration conf;
        conf.setId("report1");
        conf.setIsEnabled(true);
        Filter filter;
        filter.setPrefix("filterPrefix");
        conf.setFilter(filter);

        OSSBucketDestination dest;
        dest.setFormat("CSV");
        dest.setAccountId(Config::AccountId);
        dest.setRoleArn(Config::OssStsArn);
        dest.setBucket("acs:oss:::lyxfortest");
        dest.setPrefix("prefix1");
        dest.setEncryption("keyId");

        conf.setDestination(dest);
        conf.setSchedule(InventoryFrequency::Daily);
        conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

        OptionalFields field{ InventoryOptionalFields::Size, InventoryOptionalFields::LastModifiedDate, InventoryOptionalFields::ETag, InventoryOptionalFields::StorageClass, InventoryOptionalFields::IsMultipartUploaded, InventoryOptionalFields::EncryptionStatus };
        conf.setOptionalFields(field);

        setrequest.setInventoryConfiguration(conf);
        auto setoutcome = Client->SetBucketInventoryConfiguration(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        GetBucketInventoryConfigurationRequest getrequest(BucketName);
        getrequest.setId("report1");
        auto getoutcome = Client->GetBucketInventoryConfiguration(getrequest);
        EXPECT_EQ(getoutcome.isSuccess(), true);

        DeleteBucketInventoryConfigurationRequest delrequest(BucketName);
        delrequest.setId("report1");
        auto deloutcome = Client->DeleteBucketInventoryConfiguration(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketInventoryConfigurationTest, ListBucketInventoryConfigurationTest)
    {
        SetBucketInventoryConfigurationRequest setrequest(BucketName);
        InventoryConfiguration conf;
        int i = 1;
        for (; i < 102; i++) {
            conf.setId(std::to_string(i));
            conf.setIsEnabled(true);
            Filter filter;
            filter.setPrefix("filterPrefix");
            conf.setFilter(filter);

            OSSBucketDestination dest;
            dest.setFormat("CSV");
            dest.setAccountId(Config::AccountId);
            dest.setRoleArn(Config::OssStsArn);
            dest.setBucket("acs:oss:::lyxfortest");
            dest.setPrefix("prefix1");
            dest.setEncryption("keyId");

            conf.setDestination(dest);
            conf.setSchedule(InventoryFrequency::Daily);
            conf.setIncludedObjectVersions(InventoryIncludedObjectVersions::All);

            OptionalFields field{ InventoryOptionalFields::Size, InventoryOptionalFields::LastModifiedDate, InventoryOptionalFields::ETag, InventoryOptionalFields::StorageClass, InventoryOptionalFields::IsMultipartUploaded, InventoryOptionalFields::EncryptionStatus };
            conf.setOptionalFields(field);

            setrequest.setInventoryConfiguration(conf);
            auto setoutcome = Client->SetBucketInventoryConfiguration(setrequest);
            EXPECT_EQ(setoutcome.isSuccess(), true);
        }

        ListBucketInventoryConfigurationRequest listrequest(BucketName);
        auto listoutcome = Client->ListBucketInventoryConfiguration(listrequest);
        EXPECT_EQ(listoutcome.isSuccess(), true);
        EXPECT_EQ(listoutcome.result().NextContinuationToken(), "98");

        i = 1;
        for (; i < 102; i++) {
            DeleteBucketInventoryConfigurationRequest delrequest(BucketName);
            delrequest.setId(std::to_string(i));
            auto deloutcome = Client->DeleteBucketInventoryConfiguration(delrequest);
            EXPECT_EQ(deloutcome.isSuccess(), true);
        }
    }

    TEST_F(BucketInventoryConfigurationTest, GetBucketInventoryConfigurationResult)
    {
        std::string xml = R"(<?xml version="1.0" ?>
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
        EXPECT_EQ(result.InventoryConfiguration().Destination().Format(), "CSV");
        EXPECT_EQ(result.InventoryConfiguration().Destination().AccountId(), "123456789012");
        EXPECT_EQ(result.InventoryConfiguration().Destination().RoleArn(), "xxx");
        EXPECT_EQ(result.InventoryConfiguration().Destination().Bucket(), "acs:oss:::destination-bucket");
        EXPECT_EQ(result.InventoryConfiguration().Destination().Prefix(), "prefix1");
        EXPECT_EQ(result.InventoryConfiguration().Destination().Encryption(), "keyId");
        EXPECT_EQ(result.InventoryConfiguration().Schedule(), InventoryFrequency::Daily);
        EXPECT_EQ(result.InventoryConfiguration().IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
        EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[0], InventoryOptionalFields::Size);
        EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[1], InventoryOptionalFields::LastModifiedDate);
        EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[2], InventoryOptionalFields::ETag);
        EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[3], InventoryOptionalFields::StorageClass);
        EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[4], InventoryOptionalFields::IsMultipartUploaded);
        EXPECT_EQ(result.InventoryConfiguration().OptionalFields()[5], InventoryOptionalFields::EncryptionStatus);
    }

    TEST_F(BucketInventoryConfigurationTest, ListBucketInventoryConfigurationResult)
    {
        std::string xml = R"(<?xml version="1.0" ?>
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
                        <IsTruncated>true</IsTruncated>
                        <NextContinuationToken>test</NextContinuationToken>
                        </ListInventoryConfigurationsResult>)";

        ListBucketInventoryConfigurationResult result(xml);
        EXPECT_EQ(result.InventoryConfigurationList()[0].Id(), "report1");
        EXPECT_EQ(result.InventoryConfigurationList()[0].IsEnabled(), true);
        EXPECT_EQ(result.InventoryConfigurationList()[0].Filter().Prefix(), "filterPrefix");
        EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().Format(), "CSV");
        EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().AccountId(), "123456789012");
        EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().RoleArn(), "xxx");
        EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().Bucket(), "acs:oss:::destination-bucket");
        EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().Prefix(), "prefix1");
        EXPECT_EQ(result.InventoryConfigurationList()[0].Destination().Encryption(), "keyId");
        EXPECT_EQ(result.InventoryConfigurationList()[0].Schedule(), InventoryFrequency::Daily);
        EXPECT_EQ(result.InventoryConfigurationList()[0].IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
        EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[0], InventoryOptionalFields::Size);
        EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[1], InventoryOptionalFields::LastModifiedDate);
        EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[2], InventoryOptionalFields::ETag);
        EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[3], InventoryOptionalFields::StorageClass);
        EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[4], InventoryOptionalFields::IsMultipartUploaded);
        EXPECT_EQ(result.InventoryConfigurationList()[0].OptionalFields()[5], InventoryOptionalFields::EncryptionStatus);

        EXPECT_EQ(result.InventoryConfigurationList()[1].Id(), "report2");
        EXPECT_EQ(result.InventoryConfigurationList()[1].IsEnabled(), true);
        EXPECT_EQ(result.InventoryConfigurationList()[1].Filter().Prefix(), "filterPrefix");
        EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().Format(), "CSV");
        EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().AccountId(), "123456789012");
        EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().RoleArn(), "xxx");
        EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().Bucket(), "acs:oss:::destination-bucket");
        EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().Prefix(), "prefix1");
        EXPECT_EQ(result.InventoryConfigurationList()[1].Destination().Encryption(), "keyId");
        EXPECT_EQ(result.InventoryConfigurationList()[1].Schedule(), InventoryFrequency::Daily);
        EXPECT_EQ(result.InventoryConfigurationList()[1].IncludedObjectVersions(), InventoryIncludedObjectVersions::All);
        EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[0], InventoryOptionalFields::Size);
        EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[1], InventoryOptionalFields::LastModifiedDate);
        EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[2], InventoryOptionalFields::ETag);
        EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[3], InventoryOptionalFields::StorageClass);
        EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[4], InventoryOptionalFields::IsMultipartUploaded);
        EXPECT_EQ(result.InventoryConfigurationList()[1].OptionalFields()[5], InventoryOptionalFields::EncryptionStatus);

        EXPECT_EQ(result.IsTruncated(), true);
        EXPECT_EQ(result.NextContinuationToken(), "test");

    }

    TEST_F(BucketInventoryConfigurationTest, GetBucketInventoryConfigurationResultBranchTest)
    {
        std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
        GetBucketInventoryConfigurationResult result(xml);

        GetBucketInventoryConfigurationResult result1("test");

        xml = R"(<?xml version="1.0" ?>
                        <Inventory>
                        </Inventory>)";
        GetBucketInventoryConfigurationResult result2(xml);

        xml = R"(<?xml version="1.0" ?>
                        <InventoryConfiguration>
                        </InventoryConfiguration>)";
        GetBucketInventoryConfigurationResult result3(xml);

        xml = R"(<?xml version="1.0" ?>
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

        xml = R"(<?xml version="1.0" ?>
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

        xml = R"(<?xml version="1.0" ?>
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

        xml = R"(<?xml version="1.0" ?>
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

        xml = R"(<?xml version="1.0" ?>
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
        ListBucketInventoryConfigurationResult result(xml);

        ListBucketInventoryConfigurationResult result1("test");

        xml = R"(<?xml version="1.0" ?>
                        <ListInventoryConfigurations>
                        </ListInventoryConfigurations>)";
        ListBucketInventoryConfigurationResult result2(xml);

        xml = R"(<?xml version="1.0" ?>
                        <ListInventoryConfigurationsResult>
                        </ListInventoryConfigurationsResult>)";
        ListBucketInventoryConfigurationResult result3(xml);

        xml = R"(<?xml version="1.0" ?>
                        <ListInventoryConfigurationsResult>
                        <InventoryConfiguration>                        
                        </InventoryConfiguration>
                        <IsTruncated></IsTruncated>
                        <NextContinuationToken></NextContinuationToken>
                        </ListInventoryConfigurationsResult>)";
        ListBucketInventoryConfigurationResult result4(xml);

        xml = R"(<?xml version="1.0" ?>
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
        ListBucketInventoryConfigurationResult result5(xml);

        xml = R"(<?xml version="1.0" ?>
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
        ListBucketInventoryConfigurationResult result6(xml);

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
        ListBucketInventoryConfigurationResult result7(xml);

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
        ListBucketInventoryConfigurationResult result8(xml);
    }
}
}