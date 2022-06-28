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
#include <src/utils/Crc64.h>
#include "../Config.h"
#include "../Utils.h"

namespace AlibabaCloud {
namespace OSS {

class Crc64Test : public ::testing::Test {
protected:
    Crc64Test()
    {
    }

    ~Crc64Test() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-crctest");
        Client->CreateBucket(CreateBucketRequest(BucketName));
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName);
        Client = nullptr;
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
public:
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;
};

std::shared_ptr<OssClient> Crc64Test::Client = nullptr;
std::string Crc64Test::BucketName = "";

TEST_F(Crc64Test, CalcCRCTest)
{
    std::string data1("123456789");
    uint64_t crc1_pat = UINT64_C(0x995dc9bbdf1939fa);
    uint64_t crc1 = CRC64::CalcCRC(0, (void *)(data1.c_str()), data1.size());
    EXPECT_EQ(crc1, crc1_pat);


    std::string data2("This is a test of the emergency broadcast system.");
    uint64_t crc2_pat = UINT64_C(0x27db187fc15bbc72);
    uint64_t crc2 = CRC64::CalcCRC(0, (void *)(data2.c_str()), data2.size());
    EXPECT_EQ(crc2, crc2_pat);
}

TEST_F(Crc64Test, CalcCRCWithEndingFlagTest)
{
    std::string data1("123456789");
    std::string data2("This is a test of the emergency broadcast system.");

    //little
    uint64_t crc1_pat = UINT64_C(0x995dc9bbdf1939fa);
    uint64_t crc1 = CRC64::CalcCRC(0, (void *)(data1.c_str()), data1.size(), true);
    EXPECT_EQ(crc1, crc1_pat);

    uint64_t crc2_pat = UINT64_C(0x27db187fc15bbc72);
    uint64_t crc2 = CRC64::CalcCRC(0, (void *)(data2.c_str()), data2.size(), true);
    EXPECT_EQ(crc2, crc2_pat);

    //big
    const char *str1 = "12345678";
    const char *str2 = "87654321";
    crc1 = CRC64::CalcCRC(0, (void *)str1, 8, false);
    crc2 = CRC64::CalcCRC(0, (void *)str2, 8, true);
}


TEST_F(Crc64Test, CombineCRCTest)
{
    std::string data1("123456789");
    uint64_t crc1_pat = UINT64_C(0x995dc9bbdf1939fa);
    uint64_t crc1 = CRC64::CalcCRC(0, (void *)(data1.c_str()), data1.size());
    EXPECT_EQ(crc1, crc1_pat);

    std::string data2("This is a test of the emergency broadcast system.");
    uint64_t crc2_pat = UINT64_C(0x27db187fc15bbc72);
    uint64_t crc2 = CRC64::CalcCRC(0, (void *)(data2.c_str()), data2.size());
    EXPECT_EQ(crc2, crc2_pat);

    std::string data3;
    data3.append(data1).append(data2);
    uint64_t crc3 = CRC64::CalcCRC(0, (void *)(data3.c_str()), data3.size());
    uint64_t crc4 = CRC64::CombineCRC(crc1, crc2, data2.size());
    EXPECT_EQ(crc3, crc4);

    uint64_t crc5 = CombineCRC64(crc1, crc2, data2.size());
    EXPECT_EQ(crc3, crc5);
}

TEST_F(Crc64Test, PubObjectCrc64HeaderTest)
{
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("PubObjectCrc64HeaderTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;

    auto outcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CRC64(), crc);
}

TEST_F(Crc64Test, PutObjectCrc64EnablePositiveTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("PutObjectCrc64EnablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    auto outcome = client.PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CRC64(), crc);
}

TEST_F(Crc64Test, PutObjectCrc64EnableNegativeTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    auto key = TestUtils::GetObjectKey("PutObjectCrc64EnableNegativeTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    PutObjectRequest request(BucketName, key, content);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100001");
}

TEST_F(Crc64Test, PutObjectCrc64DisablePositiveTest)
{
    //default is enable
    ClientConfiguration conf;
    conf.enableCrc64 = false;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("PutObjectCrc64DisablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    PutObjectRequest request(BucketName, key, content);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.PutObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CRC64(), crc);
}

TEST_F(Crc64Test, UploadPartCrc64EnablePositiveTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("UploadPartCrc64EnablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;

    auto initOutcome = client.InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, key));
    EXPECT_EQ(initOutcome.isSuccess(), true);
    UploadPartRequest request(BucketName, key, 1, initOutcome.result().UploadId(), content);
    auto outcome = client.UploadPart(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CRC64(), crc);
}

TEST_F(Crc64Test, UploadPartCrc64EnableNegativeTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    auto key = TestUtils::GetObjectKey("UploadPartCrc64EnableNegativeTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;

    auto initOutcome = client.InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, key));
    EXPECT_EQ(initOutcome.isSuccess(), true);
    UploadPartRequest request(BucketName, key, 1, initOutcome.result().UploadId(), content);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.UploadPart(request);    
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100001");
}

TEST_F(Crc64Test, UploadPartCrc64DisablePositiveTest)
{
    //default is enable
    ClientConfiguration conf;
    conf.enableCrc64 = false;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("UploadPartCrc64DisablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;

    auto initOutcome = client.InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, key));
    EXPECT_EQ(initOutcome.isSuccess(), true);
    UploadPartRequest request(BucketName, key, 1, initOutcome.result().UploadId(), content);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.UploadPart(request);    
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CRC64(), crc);
}

TEST_F(Crc64Test, GetObjectCrc64EnablePositiveTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("GetObjectCrc64EnablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    client.PutObject(BucketName, key, content);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    GetObjectRequest request(BucketName, key);
    auto outcome = client.GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().CRC64(), crc);
}

TEST_F(Crc64Test, GetObjectCrc64EnableNegativeTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    auto key = TestUtils::GetObjectKey("GetObjectCrc64EnableNegativeTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    client.PutObject(BucketName, key, content);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    GetObjectRequest request(BucketName, key);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100001");
}

TEST_F(Crc64Test, GetObjectCrc64DisablePositiveTest)
{
    //default is enable
    ClientConfiguration conf;
    conf.enableCrc64 = false;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("GetObjectCrc64DisablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    client.PutObject(BucketName, key, content);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    GetObjectRequest request(BucketName, key);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().CRC64(), crc);
}

TEST_F(Crc64Test, GetObjectCrc64SaveToHeaderTest)
{
    //default is enable
    ClientConfiguration conf;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("GetObjectCrc64SaveToHeaderPositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    client.PutObject(BucketName, key, content);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    GetObjectRequest request(BucketName, key);
    request.setFlags(request.Flags() | REQUEST_FLAG_SAVE_CLIENT_CRC64);
    auto outcome = client.GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    uint64_t clientCRC64 = std::strtoull(outcome.result().Metadata().HttpMetaData().at("x-oss-hash-crc64ecma-by-client").c_str(), nullptr, 10);
    EXPECT_EQ(crc, clientCRC64);

    //range request
    request.setRange(0, 1);
    outcome = client.GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    crc = CRC64::CalcCRC(0, (void *)(data.c_str()), 2);
    clientCRC64 = std::strtoull(outcome.result().Metadata().HttpMetaData().at("x-oss-hash-crc64ecma-by-client").c_str(), nullptr, 10);
    EXPECT_EQ(crc, clientCRC64);

    //NoSaveCRC
    request.setFlags(request.Flags() & (~REQUEST_FLAG_SAVE_CLIENT_CRC64));
    request.setRange(0, 1);
    outcome = client.GetObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_TRUE(outcome.result().Metadata().HttpMetaData().find("x-oss-hash-crc64ecma-by-client") ==
        outcome.result().Metadata().HttpMetaData().end());
}


TEST_F(Crc64Test, PutObjectByUrlCrc64EnablePositiveTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("PutObjectByUrlCrc64EnablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    auto urlOutcome = client.GeneratePresignedUrl(GeneratePresignedUrlRequest(BucketName, key, Http::Put));
    PutObjectByUrlRequest request(urlOutcome.result(), content);
    auto outcome = client.PutObjectByUrl(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CRC64(), crc);
}

TEST_F(Crc64Test, PutObjectByUrlCrc64EnableNegativeTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    auto key = TestUtils::GetObjectKey("PutObjectByUrlCrc64EnableNegativeTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    auto urlOutcome = client.GeneratePresignedUrl(GeneratePresignedUrlRequest(BucketName, key, Http::Put));
    PutObjectByUrlRequest request(urlOutcome.result(), content);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.PutObjectByUrl(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100001");
}

TEST_F(Crc64Test, PutObjectByUrlCrc64DisablePositiveTest)
{
    //default is enable
    ClientConfiguration conf;
    conf.enableCrc64 = false;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("PutObjectByUrlCrc64DisablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    auto urlOutcome = client.GeneratePresignedUrl(GeneratePresignedUrlRequest(BucketName, key, Http::Put));
    PutObjectByUrlRequest request(urlOutcome.result(), content);
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.PutObjectByUrl(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().CRC64(), crc);
}

TEST_F(Crc64Test, GetObjectByUrlCrc64EnablePositiveTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("GetObjectByUrlCrc64EnablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    client.PutObject(BucketName, key, content);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    auto urlOutcome = client.GeneratePresignedUrl(GeneratePresignedUrlRequest(BucketName, key, Http::Get));
    GetObjectByUrlRequest request(urlOutcome.result());
    auto outcome = client.GetObjectByUrl(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().CRC64(), crc);
}

TEST_F(Crc64Test, GetObjectByUrlCrc64EnableNegativeTest)
{
    //default is enable
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    std::string data("This is a test of the emergency broadcast system.");
    auto key = TestUtils::GetObjectKey("GetObjectByUrlCrc64EnableNegativeTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    client.PutObject(BucketName, key, content);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    auto urlOutcome = client.GeneratePresignedUrl(GeneratePresignedUrlRequest(BucketName, key, Http::Get));
    GetObjectByUrlRequest request(urlOutcome.result());
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.GetObjectByUrl(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100001");
}

TEST_F(Crc64Test, GetObjectByUrlCrc64DisablePositiveTest)
{
    //default is enable
    ClientConfiguration conf;
    conf.enableCrc64 = false;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    std::string data("This is a test of the emergency broadcast system.");
    uint64_t crc = CRC64::CalcCRC(0, (void *)(data.c_str()), data.size());
    auto key = TestUtils::GetObjectKey("GetObjectByUrlCrc64DisablePositiveTest");
    auto content = std::make_shared<std::stringstream>();
    *content << data;
    client.PutObject(BucketName, key, content);
    EXPECT_EQ(client.DoesObjectExist(BucketName, key), true);

    auto urlOutcome = client.GeneratePresignedUrl(GeneratePresignedUrlRequest(BucketName, key, Http::Get));
    GetObjectByUrlRequest request(urlOutcome.result());
    request.setFlags(request.Flags() | 0x80000000);
    auto outcome = client.GetObjectByUrl(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().CRC64(), crc);
}

}
}
