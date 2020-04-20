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
#include <alibabacloud/oss/OssEncryptionClient.h>
#include "../Config.h"
#include "../Utils.h"
#include "src/utils/FileSystemUtils.h"
#include <fstream>

namespace AlibabaCloud {
namespace OSS {

class CryptoObjectVersioningTest : public ::testing::Test {
protected:
    CryptoObjectVersioningTest()
    {
    }

    ~CryptoObjectVersioningTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        const std::string publicKey =
            "-----BEGIN RSA PUBLIC KEY-----\n"
            "MIGJAoGBALpUiB+w+r3v2Fgw0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGl\n"
            "s5YGoqD4lObG/sCQyaWd0B/XzOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMK\n"
            "zeyVFFFvl9prlr6XpuJQlY0F/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAE=\n"
            "-----END RSA PUBLIC KEY-----";

        const std::string privateKey =
            "-----BEGIN RSA PRIVATE KEY-----\n"
            "MIICXgIBAAKBgQC6VIgfsPq979hYMNEoDG1pfG581FXN7d2GwPPR9d5a8O7+kVGy\n"
            "/PhxpbOWBqKg+JTmxv7AkMmlndAf18zoY5UnNW+d58mYZrPODBiepxdjUD/tYI2N\n"
            "QcgzCs3slRRRb5faa5a+l6biUJWNBf1uKW4y7JPD8eEIseQKCW0oRJVLtQIDAQAB\n"
            "AoGBAJrzWRAhuSLipeMRFZ5cV1B1rdwZKBHMUYCSTTC5amPuIJGKf4p9XI4F4kZM\n"
            "1klO72TK72dsAIS9rCoO59QJnCpG4CvLYlJ37wA2UbhQ1rBH5dpBD/tv3CUyfdtI\n"
            "9CLUsZR3DGBWXYwGG0KGMYPExe5Hq3PUH9+QmuO+lXqJO4IBAkEA6iLee6oBzu6v\n"
            "90zrr4YA9NNr+JvtplpISOiL/XzsU6WmdXjzsFLSsZCeaJKsfdzijYEceXY7zUNa\n"
            "0/qQh2BKoQJBAMu61rQ5wKtql2oR4ePTSm00/iHoIfdFnBNU+b8uuPXlfwU80OwJ\n"
            "Gbs0xBHe+dt4uT53QLci4KgnNkHS5lu4XJUCQQCisCvrvcuX4B6BNf+mbPSJKcci\n"
            "biaJqr4DeyKatoz36mhpw+uAH2yrWRPZEeGtayg4rvf8Jf2TuTOJi9eVWYFBAkEA\n"
            "uIPzyS81TQsxL6QajpjjI52HPXZcrPOis++Wco0Cf9LnA/tczSpA38iefAETEq94\n"
            "NxcSycsQ5br97QfyEsgbMQJANTZ/HyMowmDPIC+n9ExdLSrf4JydARSfntFbPsy1\n"
            "4oC6ciKpRdtAtAtiU8s9eAUSWi7xoaPJzjAHWbmGSHHckg==\n"
            "-----END RSA PRIVATE KEY-----";

        Endpoint = Config::Endpoint;
        Client = std::make_shared<OssEncryptionClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
            ClientConfiguration(), std::make_shared<SimpleRSAEncryptionMaterials>(publicKey, privateKey),
            CryptoConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-crypto-versioning");
        Client->CreateBucket(BucketName);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase() 
    {
        TestUtils::CleanBucketVersioning(*Client, BucketName);
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
    static std::shared_ptr<OssEncryptionClient> Client;
    static std::string BucketName;
    static std::string Endpoint;
};

std::shared_ptr<OssEncryptionClient> CryptoObjectVersioningTest::Client = nullptr;
std::string CryptoObjectVersioningTest::BucketName = "";
std::string CryptoObjectVersioningTest::Endpoint = "";


TEST_F(CryptoObjectVersioningTest, ObjectBasicWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test put, get, head and getmeta 
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("ObjectBasicWithVersioningEnableTest");

    auto pOutcome = Client->PutObject(BucketName, key, content1);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    pOutcome = Client->PutObject(BucketName, key, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    EXPECT_NE(versionId1, versionId2);
    EXPECT_NE(etag1, etag2);

    //head 
    HeadObjectRequest request(BucketName, key);
    auto hOutcome = Client->HeadObject(request);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId(), versionId2);

    request.setVersionId(versionId1);
    hOutcome = Client->HeadObject(request);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId(), versionId1);

    request.setVersionId(versionId2);
    hOutcome = Client->HeadObject(request);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId(), versionId2);

    //Get
    GetObjectRequest gRequest(BucketName, key);
    auto gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    gRequest.setVersionId(versionId1);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);

    gRequest.setVersionId(versionId2);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    auto lOutcome = Client->ListObjects(BucketName, key);
    EXPECT_EQ(lOutcome.isSuccess(), true);
    EXPECT_EQ(lOutcome.result().ObjectSummarys().size(), 1UL);

    ListObjectVersionsRequest lvRequest(BucketName);
    lvRequest.setPrefix(key);
    auto lvOutcome = Client->ListObjectVersions(lvRequest);
    EXPECT_EQ(lvOutcome.isSuccess(), true);
    EXPECT_EQ(lvOutcome.result().ObjectVersionSummarys().size(), 2UL);

    //Delete
    DeleteObjectRequest dRequest(BucketName, key);
    auto dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(dOutcome.result().VersionId().empty(), false);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), true);
    auto dversionId = dOutcome.result().VersionId();

    //Get simple meta
    GetObjectMetaRequest gmRequest(BucketName, key);
    auto gmOutcome = Client->GetObjectMeta(gmRequest);
    EXPECT_EQ(gmOutcome.isSuccess(), false);
    EXPECT_EQ(gmOutcome.error().Code(), "ServerError:404");

    gmRequest.setVersionId(versionId1);
    gmOutcome = Client->GetObjectMeta(gmRequest);
    EXPECT_EQ(gmOutcome.isSuccess(), true);
    EXPECT_EQ(gmOutcome.result().VersionId(), versionId1);

    gmRequest.setVersionId(versionId2);
    gmOutcome = Client->GetObjectMeta(gmRequest);
    EXPECT_EQ(gmOutcome.isSuccess(), true);
    EXPECT_EQ(gmOutcome.result().VersionId(), versionId2);

    //list agian
    lOutcome = Client->ListObjects(BucketName, key);
    EXPECT_EQ(lOutcome.isSuccess(), true);
    EXPECT_EQ(lOutcome.result().ObjectSummarys().size(), 0UL);

    lvRequest.setBucket(BucketName);
    lvRequest.setPrefix(key);
    lvOutcome = Client->ListObjectVersions(lvRequest);
    EXPECT_EQ(lvOutcome.isSuccess(), true);
    EXPECT_EQ(lvOutcome.result().ObjectVersionSummarys().size(), 2UL);
    EXPECT_EQ(lvOutcome.result().DeleteMarkerSummarys().size(), 1UL);

    //Get agian
    gOutcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(gOutcome.isSuccess(), false);

    gRequest.setVersionId(versionId1);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);

    gRequest.setVersionId(versionId2);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    //delete by version id
    dRequest.setVersionId(dversionId);
    dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(dOutcome.result().VersionId(), dversionId);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), true);

    gmOutcome = Client->GetObjectMeta(BucketName, key);
    EXPECT_EQ(gmOutcome.isSuccess(), true);
    EXPECT_EQ(gmOutcome.result().VersionId(), versionId2);

    dRequest.setVersionId(versionId1);
    EXPECT_EQ(dRequest.VersionId(), versionId1);
    dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), false);

    dRequest.setVersionId(versionId2);
    dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), false);

    //list again
    //lvRequest.setBucket(BucketName);
    //lvRequest.setPrefix(key);
    lvOutcome = Client->ListObjectVersions(BucketName, key);
    EXPECT_EQ(lvOutcome.isSuccess(), true);
    EXPECT_EQ(lvOutcome.result().ObjectVersionSummarys().size(), 0UL);
    EXPECT_EQ(lvOutcome.result().DeleteMarkerSummarys().size(), 0UL);
}


TEST_F(CryptoObjectVersioningTest, ResumableUploadWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //multi-part mode
    std::string key = TestUtils::GetObjectKey("ResumableUploadWithVersioningEnableTestOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadWithVersioningEnableTestOverPartSize").append(".tmp");
    // limit file size between 800KB and 2000KB
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(3);
    auto ruOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(ruOutcome.isSuccess(), true);
    EXPECT_EQ(ruOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(ruOutcome.result().VersionId().empty(), false);

    GetObjectRequest gRequest(BucketName, key);
    gRequest.setVersionId(ruOutcome.result().VersionId());
    auto gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile), ComputeContentMD5(*gOutcome.result().Content()));
    RemoveFile(tmpFile);

    //put object mode
    std::string key1 = TestUtils::GetObjectKey("ResumableUploadWithVersioningEnableTestUnderPartSize");
    std::string tmpFile1 = TestUtils::GetTargetFileName("ResumableUploadWithVersioningEnableTestUnderPartSize").append(".tmp");
    num = rand() % 8;
    TestUtils::WriteRandomDatatoFile(tmpFile1, 10240 * num);

    UploadObjectRequest request1(BucketName, key1, tmpFile1);
    request1.setPartSize(100 * 1024);
    request1.setThreadNum(1);
    auto ruOutcome1 = Client->ResumableUploadObject(request1);
    EXPECT_EQ(ruOutcome1.isSuccess(), true);
    EXPECT_EQ(ruOutcome1.result().RequestId().size(), 24UL);
    EXPECT_EQ(ruOutcome1.result().VersionId().empty(), false);

    GetObjectRequest gRequest1(BucketName, key1);
    gRequest1.setVersionId(ruOutcome1.result().VersionId());
    auto gOutcome1 = Client->GetObject(gRequest1);
    EXPECT_EQ(gOutcome1.isSuccess(), true);
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile1), ComputeContentMD5(*gOutcome1.result().Content()));
    RemoveFile(tmpFile1);
}

TEST_F(CryptoObjectVersioningTest, ResumableDownloadWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //multi-part mode
    std::string key = TestUtils::GetObjectKey("ResumableDownloadWithVersioningEnableTestOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadWithVersioningEnableTestOverPartSize").append(".tmp");
    std::string tmpFileDonwload = TestUtils::GetTargetFileName("ResumableDownloadWithVersioningEnableTestOverPartSize").append(".dat");
    // limit file size between 800KB and 2000KB
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

    auto pOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);

    auto dOutcome = Client->DeleteObject(BucketName, key);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId().empty(), false);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), true);

    DownloadObjectRequest rdRequest(BucketName, key, tmpFileDonwload);
    rdRequest.setPartSize(100 * 1024);
    rdRequest.setThreadNum(3);
    auto rdOutcome = Client->ResumableDownloadObject(rdRequest);
    EXPECT_EQ(rdOutcome.isSuccess(), false);
    EXPECT_EQ(rdOutcome.error().Code(), "ServerError:404");

    rdRequest.setVersionId(pOutcome.result().VersionId());
    rdOutcome = Client->ResumableDownloadObject(rdRequest);
    EXPECT_EQ(rdOutcome.isSuccess(), true);
    EXPECT_EQ(rdOutcome.result().VersionId(), pOutcome.result().VersionId());
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile), TestUtils::GetFileMd5(tmpFileDonwload));

    RemoveFile(tmpFileDonwload);

    //get object mode
    std::string tmpFileDonwload1 = TestUtils::GetTargetFileName("ResumableDownloadWithVersioningEnableTestUnderPartSize").append(".dat");
    DownloadObjectRequest rdRequest1(BucketName, key, tmpFileDonwload1);
    rdRequest1.setPartSize(4 * 1024 * 1024);
    rdOutcome = Client->ResumableDownloadObject(rdRequest1);
    EXPECT_EQ(rdOutcome.isSuccess(), false);
    EXPECT_EQ(rdOutcome.error().Code(), "ServerError:404");

    rdRequest1.setVersionId(pOutcome.result().VersionId());
    rdOutcome = Client->ResumableDownloadObject(rdRequest1);
    EXPECT_EQ(rdOutcome.isSuccess(), true);
    EXPECT_EQ(rdOutcome.result().VersionId(), pOutcome.result().VersionId());
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile), TestUtils::GetFileMd5(tmpFileDonwload1));

    RemoveFile(tmpFileDonwload1);
    RemoveFile(tmpFile);
}

}
}