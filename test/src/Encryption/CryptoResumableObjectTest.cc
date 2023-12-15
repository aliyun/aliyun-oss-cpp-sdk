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
#include <alibabacloud/oss/OssEncryptionClient.h>
#include <alibabacloud/oss/Const.h>
#include "../Config.h"
#include "../Utils.h"
#include "src/utils/FileSystemUtils.h"
#include "src/utils/Utils.h"
#include "src/external/json/json.h"
#include <fstream>
#include <ctime>


namespace AlibabaCloud {
namespace OSS {

class CryptoResumableObjectTest : public::testing::Test {
protected:
    CryptoResumableObjectTest()
    {
    }
    ~CryptoResumableObjectTest()override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        PublicKey =
            "-----BEGIN RSA PUBLIC KEY-----\n"
            "MIGJAoGBALpUiB+w+r3v2Fgw0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGl\n"
            "s5YGoqD4lObG/sCQyaWd0B/XzOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMK\n"
            "zeyVFFFvl9prlr6XpuJQlY0F/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAE=\n"
            "-----END RSA PUBLIC KEY-----";

        PrivateKey =
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

        Description["comment"] = "rsa test";
        Description["provider"] = "aliclould";
        Endpoint = Config::Endpoint;
        Client = std::make_shared<OssEncryptionClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
            ClientConfiguration(), std::make_shared<SimpleRSAEncryptionMaterials>(PublicKey, PrivateKey, Description),
            CryptoConfiguration());

        UnEncryptionClient = std::make_shared<OssClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());

        BucketName = TestUtils::GetBucketName("cpp-sdk-crypto-resumableobject");
        Client->CreateBucket(CreateBucketRequest(BucketName));
        UploadPartFailedFlag = 1 << 30;
        DownloadPartFailedFlag = 1 << 30;
        CopyPartFailedFlag = 1 << 30;
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName);
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

    static std::string GetCheckpointFileByResumableUploader(std::string bucket, std::string key,
        std::string checkpointDir, std::string filePath);
    static std::string GetCheckpointFileByResumableDownloader(std::string bucket, std::string key,
        std::string checkpointDir, std::string filePath);
    static std::string GetCheckpointFileByResumableCopier(std::string bucket, std::string key,
        std::string srcBucket, std::string srcKey, std::string checkpointDir);
    static void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData);

public:
    static std::shared_ptr<OssEncryptionClient> Client;
    static std::shared_ptr<OssClient> UnEncryptionClient;
    static std::string BucketName;
    static int UploadPartFailedFlag;
    static int DownloadPartFailedFlag;
    static int CopyPartFailedFlag;
    static std::string PublicKey;
    static std::string PrivateKey;
    static std::map<std::string, std::string> Description;
    static std::string Endpoint;

    class Timer
    {
    public:
        Timer() : begin_(std::chrono::high_resolution_clock::now()) {}
        void reset() { begin_ = std::chrono::high_resolution_clock::now(); }
        int64_t elapsed() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - begin_).count();
        }
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> begin_;
    };
};

std::string CryptoResumableObjectTest::GetCheckpointFileByResumableUploader(std::string bucket,
    std::string key, std::string checkpointDir, std::string filePath)
{
    if (!checkpointDir.empty()) {
        std::stringstream ss;
        ss << "oss://" << bucket << "/" << key;
        auto destPath = ss.str();
        auto safeFileName = ComputeContentETag(filePath) + "--" + ComputeContentETag(destPath);
        return checkpointDir + PATH_DELIMITER + safeFileName;
    }
    return "";
}

std::string CryptoResumableObjectTest::GetCheckpointFileByResumableDownloader(std::string bucket,
    std::string key, std::string checkpointDir, std::string filePath)
{
    if (!checkpointDir.empty()) {
        std::stringstream ss;
        ss << "oss://" << bucket << "/" << key;
        auto srcPath = ss.str();
        auto safeFileName = ComputeContentETag(srcPath) + "--" + ComputeContentETag(filePath);
        return checkpointDir + PATH_DELIMITER + safeFileName;
    }
    return "";
}

std::string CryptoResumableObjectTest::GetCheckpointFileByResumableCopier(std::string bucket, std::string key,
    std::string srcBucket, std::string srcKey, std::string checkpointDir)
{
    if (!checkpointDir.empty()) {
        std::stringstream ss;
        ss << "oss://" << srcBucket << "/" << srcKey;
        auto srcPath = ss.str();
        ss.str("");
        ss << "oss://" << bucket << "/" << key;
        auto destPath = ss.str();

        auto safeFileName = ComputeContentETag(srcPath) + "--" + ComputeContentETag(destPath);
        return checkpointDir + PATH_DELIMITER + safeFileName;
    }
    return "";
}

void CryptoResumableObjectTest::ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData)
{
    std::cout << "ProgressCallback[" << userData << "] => " <<
        increment << "," << transfered << "," << total << std::endl;
}

std::shared_ptr<OssEncryptionClient> CryptoResumableObjectTest::Client = nullptr;
std::shared_ptr<OssClient> CryptoResumableObjectTest::UnEncryptionClient = nullptr;
std::string CryptoResumableObjectTest::BucketName = "";
int CryptoResumableObjectTest::UploadPartFailedFlag = 0;
int CryptoResumableObjectTest::DownloadPartFailedFlag = 0;
int CryptoResumableObjectTest::CopyPartFailedFlag = 0;
std::string CryptoResumableObjectTest::PublicKey = "";
std::string CryptoResumableObjectTest::PrivateKey = "";
std::map<std::string, std::string> CryptoResumableObjectTest::Description = std::map<std::string, std::string>();
std::string CryptoResumableObjectTest::Endpoint = "";

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithSizeOverPartSizeTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadObjectOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadObjectOverPartSize").append(".tmp");
    // limit file size between 800KB and 2000KB
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);

    auto getoutcome = Client->GetObject(GetObjectRequest(BucketName, key));

    std::fstream file(tmpFile, std::ios::in | std::ios::binary);
    std::string oriMd5 = ComputeContentMD5(file);
    std::string memMd5 = ComputeContentMD5(*getObjectOutcome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);

    file.close();
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithSizeUnderPartSizeTest)
{
    std::string key = TestUtils::GetObjectKey("ResumableUploadObjectUnderPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadObjectUnderPartSize").append(".tmp");
    int num = rand() % 8;
    TestUtils::WriteRandomDatatoFile(tmpFile, 10240 * num);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableObjectWithDisableRequest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalUploadObjectWithDisableRequest");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUploadObjectWithDisableRequest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));
    Client->DisableRequest();

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(102400);
    request.setThreadNum(1);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100002");

    EXPECT_EQ(RemoveFile(tmpFile), true);
    Client->EnableRequest();
}

TEST_F(CryptoResumableObjectTest, MultiResumableUploadWithSizeOverPartSizeTest)
{
    std::string key = TestUtils::GetObjectKey("MultiUploadObjectOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiUploadObjectOverPartSize").append(".tmp");
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    int threadNum = 1 + rand() % 99;

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(threadNum);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadSetMinPartSizeTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectSetMinPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectSetMinPartSize").append(".tmp");
    int num = 1 + rand() % 20;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    int partSize = 1 + rand() % 99;

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(partSize * 102400);
    request.setThreadNum(1);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableUploadWithoutSourceFilePathTest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutFilePath");
    std::string tmpFile;
    UploadObjectRequest request(BucketName, key, tmpFile);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableUploadWithoutRealFileTest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutRealFile");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUplloadObjectWithoutRealFile").append(".tmp");
    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setThreadNum(1);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableUploadWithNotExitsCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalUploadObjectWithNotExitsCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUploadObjectWithNotExitsCheckpoint").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 100);
    std::string checkPoint = "NotExistDir";

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    request.setCheckpointDir(checkPoint);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");

    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectWithCheckpoint").append(".tmp");
    std::string checkpointDir = TestUtils::GetExecutableDirectory();
    int num = 1 + rand() % 10;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    request.setCheckpointDir(checkpointDir);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableUploadWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("MultiUploadObjectWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectWithCheckpoint").append(".tmp");
    std::string checkpointDir = TestUtils::GetExecutableDirectory();
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    int threadNum = 1 + rand() % 99;

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setCheckpointDir(checkpointDir);
    request.setThreadNum(threadNum);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableUploadWithFailedPartUploadTest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalUploadObjectWithFailedPart");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUploadObjectWithFailedPart").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(1024);
    auto failOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(failOutcome.isSuccess(), false);

    request.setPartSize(102400);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadRetryAfterFailedPartTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectWithFailedPart");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectWithFailedPart").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string checkpointKey = TestUtils::GetObjectKey("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointKey), true);
    EXPECT_EQ(IsDirectoryExist(checkpointKey), true);

    // resumable upload object failed
    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(102400);
    request.setThreadNum(1);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    request.setCheckpointDir(checkpointKey);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // retry
    request.setFlags(request.Flags() ^ UploadPartFailedFlag);
    auto retryOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointKey), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableUploadRetryAfterFailedPartTest)
{
    std::string key = TestUtils::GetObjectKey("NormalMultiUploadObjectWithFailedPart");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalMultiUploadObjectWithFailedPart").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string checkpointKey = TestUtils::GetObjectKey("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointKey), true);
    EXPECT_EQ(IsDirectoryExist(checkpointKey), true);

    // resumable upload object failed
    int threadNum = 1 + rand() % 100;
    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(102400);
    request.setThreadNum(threadNum);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    request.setCheckpointDir(checkpointKey);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // retry
    request.setFlags(request.Flags() ^ UploadPartFailedFlag);
    auto retryOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointKey), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadRetryWithSourceFileChangedTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectWithSourceFileChanged");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectWithSourceFileChanged").append(".tmp");
    int num = 2 + rand() % 10;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    std::string sourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string checkpointKey = TestUtils::GetObjectKey("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointKey), true);
    EXPECT_EQ(IsDirectoryExist(checkpointKey), true);

    // resumable upload object failed
    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setThreadNum(1);
    request.setPartSize(102400);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    request.setCheckpointDir(checkpointKey);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // change source file
    EXPECT_EQ(RemoveFile(tmpFile), true);
    // TODO: API only check file size, don't check the contents of file
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (num + 1));
    std::string newSourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
    EXPECT_NE(sourceFileMd5, newSourceFileMd5);

    // retry
    request.setFlags(request.Flags() ^ UploadPartFailedFlag);
    auto retryOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download target object
    std::string targetFile = TestUtils::GetObjectKey("DownloadResumableUploadObject");
    auto getObjectOutcome = Client->GetObject(BucketName, key, targetFile);
    std::shared_ptr<std::iostream> getObjectContent = nullptr;
    getObjectOutcome.result().setContent(getObjectContent);
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);

    std::string targetFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(targetFileMd5, newSourceFileMd5);
    EXPECT_NE(targetFileMd5, sourceFileMd5);

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointKey), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableUploadRetryWithSourceFileChangedTest)
{

    std::string key = TestUtils::GetObjectKey("MultiUploadObjectWithSourceFileChanged");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiUploadObjectWithSourceFileChanged").append(".tmp");
    int num = 2 + rand() % 10;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    std::string sourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string checkpointKey = TestUtils::GetObjectKey("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointKey), true);
    EXPECT_EQ(IsDirectoryExist(checkpointKey), true);

    // upload object failed
    int threadNum = 1 + rand() % 100;
    UploadObjectRequest request(BucketName, key, tmpFile, checkpointKey, 102400, threadNum);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // change source file
    EXPECT_EQ(RemoveFile(tmpFile), true);
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 100);
    std::string newSourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
    EXPECT_NE(sourceFileMd5, newSourceFileMd5);

    // retry
    request.setFlags(request.Flags() ^ UploadPartFailedFlag);
    auto retryOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download
    std::string targetFile = TestUtils::GetObjectKey("DownloadResumableUploadObject");
    auto getObjectOutcome = Client->GetObject(BucketName, key, targetFile);
    std::shared_ptr<std::iostream> getObjectContent = nullptr;
    getObjectOutcome.result().setContent(getObjectContent);
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);

    std::string targetFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(targetFileMd5, newSourceFileMd5);
    EXPECT_NE(targetFileMd5, sourceFileMd5);

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointKey), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadRetryWithCheckpointFileChangedTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectWithCheckpointFileChanged");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectWithCheckpointFileChanged").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string checkpointKey = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointKey), true);
    EXPECT_EQ(IsDirectoryExist(checkpointKey), true);

    // resumable upload object failed
    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(102400);
    request.setThreadNum(1);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    request.setCheckpointDir(checkpointKey);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // change the checkpoint file
    std::string checkpointFilename = GetCheckpointFileByResumableUploader(BucketName, key, checkpointKey, tmpFile);
    std::string checkpointTmpFile = std::string(checkpointFilename).append(".tmp");
    std::ifstream jsonStream(checkpointFilename, std::ios::in | std::ios::binary);
    Json::CharReaderBuilder rbuilder;
    Json::Value readRoot;
    Json::Value writeRoot;
    std::string uploadId = "InvaliedUploadID";
    //if (reader.parse(jsonStream, readRoot)) {
    if (Json::parseFromStream(rbuilder, jsonStream, &readRoot, nullptr)) {
        writeRoot["opType"] = readRoot["opType"].asString();
        writeRoot["uploadID"] = uploadId;
        writeRoot["bucket"] = readRoot["bucket"].asString();
        writeRoot["key"] = readRoot["key"].asString();
        writeRoot["mtime"] = readRoot["mtime"].asString();
        writeRoot["size"] = readRoot["size"].asInt64();
        writeRoot["partSize"] = readRoot["partSize"].asInt64();
        writeRoot["md5Sum"] = readRoot["md5Sum"].asString();
    }
    jsonStream.close();

    std::fstream recordStream(checkpointTmpFile, std::ios::out);
    if (recordStream.is_open()) {
        recordStream << writeRoot;
    }
    recordStream.close();
    EXPECT_EQ(RemoveFile(checkpointFilename), true);
    EXPECT_EQ(RenameFile(checkpointTmpFile, checkpointFilename), true);

    // retry
    request.setFlags(request.Flags() ^ UploadPartFailedFlag);
    auto retryOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointKey), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableUploadRetryWithCheckpointFileChangedTest)
{
    std::string key = TestUtils::GetObjectKey("MultiUploadObjectWithCheckpointFileChanged");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiUploadObjectWithCheckpointFileChanged").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string checkpointKey = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointKey), true);
    EXPECT_EQ(IsDirectoryExist(checkpointKey), true);

    // resumable upload object failed
    int threadNum = 1 + rand() % 100;
    UploadObjectRequest request(BucketName, key, tmpFile, checkpointKey, 102400, threadNum);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // change the checkpoint file
    std::string checkpointFilename = GetCheckpointFileByResumableUploader(BucketName, key, checkpointKey, tmpFile);
    std::string checkpointTmpFile = std::string(checkpointFilename).append(".tmp");
    std::ifstream jsonStream(checkpointFilename, std::ios::in | std::ios::binary);
    Json::CharReaderBuilder rbuilder;
    Json::Value readRoot;
    Json::Value writeRoot;
    std::string uploadId = "InvaliedUploadID";
    //if (reader.parse(jsonStream, readRoot)) {
    if (Json::parseFromStream(rbuilder, jsonStream, &readRoot, nullptr)) {
        writeRoot["opType"] = readRoot["opType"].asString();
        writeRoot["uploadID"] = uploadId;
        writeRoot["bucket"] = readRoot["bucket"].asString();
        writeRoot["key"] = readRoot["key"].asString();
        writeRoot["mtime"] = readRoot["mtime"].asString();
        writeRoot["size"] = readRoot["size"].asInt64();
        writeRoot["partSize"] = readRoot["partSize"].asInt64();
        writeRoot["md5Sum"] = readRoot["md5Sum"].asString();
    }
    jsonStream.close();

    std::fstream recordStream(checkpointTmpFile, std::ios::out);
    if (recordStream.is_open()) {
        recordStream << writeRoot;
    }
    recordStream.close();
    EXPECT_EQ(RemoveFile(checkpointFilename), true);
    EXPECT_EQ(RenameFile(checkpointTmpFile, checkpointFilename), true);

    // retry
    request.setFlags(request.Flags() ^ UploadPartFailedFlag);
    auto retryOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointKey), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithProgressCallbackTest)
{
    std::string key = TestUtils::GetObjectKey("NormalResumableUploadObjectWithCallback");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableUploadObjectWithCallback").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    TransferProgress progressCallback = { ProgressCallback, this };
    UploadObjectRequest request(BucketName, key, tmpFile, checkpointDir, 102400, 1);
    request.setTransferProgress(progressCallback);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    request = UploadObjectRequest(BucketName, key, tmpFile, checkpointDir, 10240000, 1);
    request.setTransferProgress(progressCallback);
    outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);

}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadProgressCallbackWithUploadPartFailedTest)
{
    std::string key = TestUtils::GetObjectKey("NormalResumableUploadObjectWithCallback");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableUploadObjectWithCallback").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    TransferProgress progressCallback = { ProgressCallback, this };
    UploadObjectRequest request(BucketName, key, tmpFile, checkpointDir, 102400, 1);
    request.setFlags(request.Flags() | UploadPartFailedFlag);
    request.setTransferProgress(progressCallback);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // retry
    std::cout << "Retry : " << std::endl;
    request.setFlags(request.Flags() ^ UploadPartFailedFlag);
    auto retryOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableUploadWithThreadNumberOverPartNumber)
{
    std::string key = TestUtils::GetObjectKey("MultiUploadObjectWithThreadNumberOverPartNumber");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiUploadObjectWithThreadNumberOverPartNumber").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));

    int threadNum = 0;
    UploadObjectRequest request(BucketName, key, tmpFile, "");
    request.setPartSize(102400);
    request.setThreadNum(threadNum);
    auto invalidateOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(invalidateOutcome.isSuccess(), false);

    // the thread num over part num
    threadNum = 20;
    request.setThreadNum(threadNum);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithObjectMetaDataSetTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectWithObjectMetaDateSetTest");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectWithObjectMetaDateSetTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));

    ObjectMetaData meta;
    meta.setCacheControl("No-Cache");
    meta.setExpirationTime("Fri, 09 Nov 2018 05:57:16 GMT");
    // upload object
    UploadObjectRequest request(BucketName, key, tmpFile, "", meta);
    request.setPartSize(102400);
    request.setThreadNum(1);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().CacheControl(), "No-Cache");
    EXPECT_EQ(hOutcome.result().ExpirationTime(), "Fri, 09 Nov 2018 05:57:16 GMT");
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithUserMetaDataTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectWithUserDataTest");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalUploadObjectWithUserDataTest").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));

    // upload object
    ObjectMetaData metaDate;
    metaDate.UserMetaData()["test"] = "testvalue";
    UploadObjectRequest request(BucketName, key, tmpFile, "", 102400, 1, metaDate);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto hOutcome = Client->HeadObject(BucketName, key);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "testvalue");
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithObjectAclSetTest)
{
    std::string key = TestUtils::GetObjectKey("NormalUploadObjectWithObjectAclSetTest");
    std::string tmpFile = TestUtils::GetTargetFileName("").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));

    // upload object
    UploadObjectRequest request(BucketName, key, tmpFile, "", 102400, 1);
    CannedAccessControlList acl = CannedAccessControlList::PublicReadWrite;
    request.setAcl(acl);
    request.setEncodingType("url");
    EXPECT_EQ(request.EncodingType(), "url");
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    auto aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, key));
    EXPECT_EQ(aclOutcome.isSuccess(), true);
    EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithNon16AlignmentPartSizeTest)
{
    std::string key = TestUtils::GetObjectKey("NormalResumableUploadWithNon16AlignmentPartSizeTest");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableUploadWithNon16AlignmentPartSizeTest").append(".tmp");
    // limit file size between 800KB and 2000KB
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024 + 3);
    request.setThreadNum(3);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);

    std::fstream file(tmpFile, std::ios::in | std::ios::binary);
    std::string oriMd5 = ComputeContentMD5(file);
    std::string memMd5 = ComputeContentMD5(*getObjectOutcome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
    auto value = getObjectOutcome.result().Metadata().UserMetaData().at("client-side-encryption-part-size");
    EXPECT_EQ("102400", value);

    file.close();
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableUploadWithContentMD5Test)
{
    std::string key = TestUtils::GetObjectKey("NormalResumableUploadWithContentMD5Test");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableUploadWithContentMD5Test").append(".tmp");

    // limit file size between 800KB and 2000KB
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);
    std::fstream file(tmpFile, std::ios::in | std::ios::binary);
    std::string oriMd5 = ComputeContentMD5(file);
    file.close();

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(200 * 1024);
    request.setThreadNum(3);
    request.MetaData().setContentMd5(oriMd5);
    auto outcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    auto getObjectOutcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(getObjectOutcome.isSuccess(), true);

    std::string memMd5 = ComputeContentMD5(*getObjectOutcome.result().Content());
    auto unencryptedMd5 = getObjectOutcome.result().Metadata().UserMetaData().at("client-side-encryption-unencrypted-content-md5");
    EXPECT_EQ(unencryptedMd5, memMd5);

    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableDownloadObjectWithDisableRequest)
{
    // upload object
    std::string key = TestUtils::GetObjectKey("UnnormalResumableDownloadObjectWithDisableRequest");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalResumableDownloadObjectWithDisableRequest").append(".tmp");
    std::string targetKey = TestUtils::GetObjectKey("UnnormalResumableDownloadTargetObject");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (1 + rand() % 10));

    auto putObjectOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);

    // download object
    Client->DisableRequest();
    DownloadObjectRequest request(BucketName, key, targetKey);
    request.setPartSize(102400);
    request.setThreadNum(1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100002");

    EXPECT_EQ(RemoveFile(tmpFile), true);
    RemoveFile(targetKey.append(".temp"));
    Client->EnableRequest();
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithSizeOverPartSizeTest)
{
    // upload object
    std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectOverPartSize").append(".tmp");
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    int num = 1 + rand() % 10;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithSizeUnderPartSizeTest)
{
    // upload object
    std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectUnderPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectUnderPartSize").append(".tmp");
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    int num = 10 + rand() % 10;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setThreadNum(1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableDownloadWithSizeOverPartSizeTest)
{
    // upload
    std::string key = TestUtils::GetObjectKey("MultiResumableDownloadObjectOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiResumableDownloadObjectOverPartSize").append(".tmp");
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    int num = 1 + rand() % 10;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    int threadNum = 1 + rand() % 100;
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(uploadOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(threadNum);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadSetMinPartSizeTest)
{
    // upload
    std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectSetMinPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectSetMinPartSize").append(".tmp");
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    int num = 1 + rand() % 10;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(uploadOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download
    int partSize = 1 + rand() % 99;
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(partSize * 1024);
    request.setThreadNum(1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    request.setPartSize(102400);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableDownloadWithoutTargetFilePathTest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutFilePath");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUplloadObjectWithoutFilePath").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1));
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download
    std::string targetFile;
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(100 * 1024);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("NormalDownloadObjectWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalDownloadObjectWithCheckpoint").append(".tmp");
    std::string checkpointDir = TestUtils::GetExecutableDirectory();
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));

    // upload
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(1);
    request.setCheckpointDir(checkpointDir);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableDownloadWithNotExitsCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalDownloadObjectWithNotExistCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalDownloadObjectWithNotExistCheckpoint").append(".tmp");
    std::string checkpointDir = "NotExistDir";
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));

    // upload
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(100 * 1024);
    request.setCheckpointDir(checkpointDir);
    auto outcome = Client->ResumableDownloadObject(request);
    std::shared_ptr<std::iostream> content = nullptr;
    outcome.result().setContent(content);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
    EXPECT_EQ(outcome.isSuccess(), false);

    RemoveFile(targetFile.append(".temp"));
    RemoveFile(tmpFile);
}

TEST_F(CryptoResumableObjectTest, MultiResumableDownloadWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("MultiDownloadObjectWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiDownloadObjectWithCheckpoint").append(".tmp");
    std::string checkpointDir = TestUtils::GetExecutableDirectory();
    std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));

    // upload
    auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download
    int threadNum = 1 + rand() % 99;
    DownloadObjectRequest request(BucketName, key, targetFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(threadNum);
    request.setCheckpointDir(checkpointDir);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);

    std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadMd5, downloadMd5);

    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableDownloadWithDownloadPartFailedTest)
{
    std::string key = TestUtils::GetObjectKey("UnnormalDownloadObjectWithPartFailed");
    std::string tmpFile = TestUtils::GetTargetFileName("UnnormalDownloadObjectWithPartFailed").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    // upload object
    auto uploadOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, 1);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    std::string checkpointFile = GetCheckpointFileByResumableDownloader(BucketName, key, checkpointDir, targetFile);
    std::string failedDownloadFile = targetFile.append(".temp");
    EXPECT_EQ(RemoveFile(checkpointFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(failedDownloadFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadRetryWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("NormalDownloadObjectRetryWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalDownloadObjectRetryWithCheckpoint").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    // upload object
    auto uploadOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, 1);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // retry
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);

    std::string uploadFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadFileMd5, downloadFileMd5);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableDownloadRetryWithCheckpointTest)
{
    std::string key = TestUtils::GetObjectKey("MultiDownloadObjectRetryWithCheckpoint");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiDownloadObjectRetryWithCheckpoint").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    // upload object
    auto uploadOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    int threadNum = 1 + rand() % 100;
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, threadNum);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // retry
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);

    std::string uploadFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string downloadFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(uploadFileMd5, downloadFileMd5);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadRetryWithSourceObjectDeletedTest)
{
    std::string key = TestUtils::GetObjectKey("NormalDownloadObjectRetryWithSourceObjectDeleted");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalDownloadObjectRetryWithSourceObjectDeleted").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    // put object
    auto putObjectOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, 1);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // delete source object
    auto deleteObjectOutcome = Client->DeleteObject(BucketName, key);
    EXPECT_EQ(deleteObjectOutcome.isSuccess(), true);

    // retry
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), false);

    std::string checkpointFile = GetCheckpointFileByResumableDownloader(BucketName, key, checkpointDir, targetFile);
    EXPECT_EQ(RemoveFile(checkpointFile), true);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile.append(".temp")), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadRetryWithCheckpointFileChangedTest)
{
    std::string key = TestUtils::GetObjectKey("NormalDownloadObjectRetryWithCheckpointFileChanged");
    std::string tmpFile = TestUtils::GetTargetFileName("NormalDownloadObjectRetryWithCheckpointFileChanged").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    auto uploadOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, 1);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // change the checkpoint file
    std::string checkpointFile = GetCheckpointFileByResumableDownloader(BucketName, key, checkpointDir, targetFile);
    std::string checkpointTmpFile = std::string(checkpointFile).append(".tmp");
    std::ifstream jsonStream(checkpointFile, std::ios::in | std::ios::binary);
    Json::CharReaderBuilder rbuilder;
    Json::Value readRoot;
    Json::Value writeRoot;
    std::string invaliedKey = "InvaliedKey";
    if (Json::parseFromStream(rbuilder, jsonStream, &readRoot, nullptr)) {
    //if (reader.parse(jsonStream, readRoot)) {
        writeRoot["opType"] = readRoot["opType"].asString();
        writeRoot["bucket"] = readRoot["bucket"].asString();
        writeRoot["key"] = invaliedKey;
        writeRoot["filePath"] = readRoot["filePath"].asString();
        writeRoot["mtime"] = readRoot["mtime"].asString();
        writeRoot["sizesize"] = readRoot["size"].asUInt64();
        writeRoot["partSize"] = readRoot["partSize"].asUInt64();

        for (uint32_t i = 0; i < readRoot["parts"].size(); i++) {
            Json::Value partValue = readRoot["parts"][i];
            writeRoot["parts"][i]["partNumber"] = partValue["partNumber"].asInt();
            writeRoot["parts"][i]["size"] = partValue["size"].asInt64();
            writeRoot["parts"][i]["crc64"] = partValue["crc64"].asUInt64();
        }
        writeRoot["md5Sum"] = readRoot["md5Sum"].asString();

        if (readRoot["rangeStart"] != Json::nullValue && readRoot["rangeEnd"] != Json::nullValue) {
            writeRoot["rangeStart"] = readRoot["rangeStart"].asInt64();
            writeRoot["rangeEnd"] = readRoot["rangeEnd"].asInt64();
        }
    }
    jsonStream.close();

    std::fstream recordStream(checkpointTmpFile, std::ios::out);
    if (recordStream.is_open()) {
        recordStream << writeRoot;
    }
    recordStream.close();
    EXPECT_EQ(RemoveFile(checkpointFile), true);
    EXPECT_EQ(RenameFile(checkpointTmpFile, checkpointFile), true);

    // retry
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(retryOutcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);

    std::string sourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string targetFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(sourceFileMd5, targetFileMd5);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, MultiResumableDownloadRetryWithCheckpointFileChangedTest)
{
    std::string key = TestUtils::GetObjectKey("MultiDownloadObjectRetryWithCheckpointFileChanged");
    std::string tmpFile = TestUtils::GetTargetFileName("MultiDownloadObjectRetryWithCheckpointFileChanged").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));
    std::string targetFile = TestUtils::GetObjectKey("ResumableDownloadTargetObject");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    auto uploadOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(uploadOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    int threadNum = 1 + rand() % 100;
    DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir, 102400, threadNum);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    // change the checkpoint file
    std::string checkpointFile = GetCheckpointFileByResumableDownloader(BucketName, key, checkpointDir, targetFile);
    std::string checkpointTmpFile = std::string(checkpointFile).append(".tmp");
    std::ifstream jsonStream(checkpointFile, std::ios::in | std::ios::binary);
    Json::CharReaderBuilder rbuilder;
    Json::Value readRoot;
    Json::Value writeRoot;
    std::string invaliedKey = "InvaliedKey";
    if (Json::parseFromStream(rbuilder, jsonStream, &readRoot, nullptr)) {
    //if (reader.parse(jsonStream, readRoot)) {
        writeRoot["opType"] = readRoot["opType"].asString();
        writeRoot["bucket"] = readRoot["bucket"].asString();
        writeRoot["key"] = invaliedKey;
        writeRoot["filePath"] = readRoot["filePath"].asString();
        writeRoot["mtime"] = readRoot["mtime"].asString();
        writeRoot["sizesize"] = readRoot["size"].asUInt64();
        writeRoot["partSize"] = readRoot["partSize"].asUInt64();

        for (uint32_t i = 0; i < readRoot["parts"].size(); i++) {
            Json::Value partValue = readRoot["parts"][i];
            writeRoot["parts"][i]["partNumber"] = partValue["partNumber"].asInt();
            writeRoot["parts"][i]["size"] = partValue["size"].asInt64();
            writeRoot["parts"][i]["crc64"] = partValue["crc64"].asUInt64();
        }
        writeRoot["md5Sum"] = readRoot["md5Sum"].asString();

        if (readRoot["rangeStart"] != Json::nullValue && readRoot["rangeEnd"] != Json::nullValue) {
            writeRoot["rangeStart"] = readRoot["rangeStart"].asInt64();
            writeRoot["rangeEnd"] = readRoot["rangeEnd"].asInt64();
        }
    }
    jsonStream.close();

    std::fstream recordStream(checkpointTmpFile, std::ios::out);
    if (recordStream.is_open()) {
        recordStream << writeRoot;
    }
    recordStream.close();
    EXPECT_EQ(RemoveFile(checkpointFile), true);
    EXPECT_EQ(RenameFile(checkpointTmpFile, checkpointFile), true);

    // retry
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(retryOutcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);

    std::string sourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
    std::string targetFileMd5 = TestUtils::GetFileMd5(targetFile);
    EXPECT_EQ(sourceFileMd5, targetFileMd5);
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithProgressCallbackTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithProgressCallback");
    std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithProgressCallback");

    auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    TransferProgress progressCallback = { ProgressCallback, this };
    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setTransferProgress(progressCallback);
    request.setPartSize(102400);
    request.setThreadNum(1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_EQ(RemoveFile(targetKey), true);

    std::string targetKey1 = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithProgressCallback");
    request = DownloadObjectRequest(BucketName, sourceKey, targetKey1);
    request.setTransferProgress(progressCallback);
    request.setPartSize(10240000);
    outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey1), ComputeContentMD5(*putObjectContent));
    EXPECT_EQ(RemoveFile(targetKey1), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadProgressCallbackWithDownloadPartFailedTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectProgressCallbackWithPartFailed");
    std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectProgressCallbackWithPartFailed");
    std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
    EXPECT_EQ(CreateDirectory(checkpointDir), true);
    EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

    auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    TransferProgress progressCallback = { ProgressCallback, this };
    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setCheckpointDir(checkpointDir);
    request.setTransferProgress(progressCallback);
    request.setFlags(request.Flags() | DownloadPartFailedFlag);
    request.setPartSize(102400);
    request.setThreadNum(1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);

    std::cout << "Retry : " << std::endl;
    request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
    auto retryOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
    EXPECT_EQ(retryOutcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_NE(TestUtils::GetFileCRC64(targetKey), outcome.result().Metadata().CRC64());
    EXPECT_EQ(RemoveFile(targetKey), true);
    EXPECT_EQ(RemoveDirectory(checkpointDir), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithRangeLength)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithRangeLength");
    std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithRangeLength");
    auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);
    request.setRange(20, 30);
    request.setThreadNum(1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().ContentLength(), 30 - 20 + 1);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);

    std::string targetKey1 = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithRangeLength1");
    request = DownloadObjectRequest(BucketName, sourceKey, targetKey1);
    request.setPartSize(10240000);
    request.setRange(20, 30);
    auto outcome1 = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome1.isSuccess(), true);
    EXPECT_EQ(outcome1.result().Metadata().ContentLength(), 30 - 20 + 1);
    EXPECT_EQ(outcome1.result().Metadata().hasUserHeader("client-side-encryption-key"), true);

    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), TestUtils::GetFileMd5(targetKey1));

    EXPECT_EQ(RemoveFile(targetKey), true);
    EXPECT_EQ(RemoveFile(targetKey1), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithErrorRangeLength)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithErrorRangeLength");
    std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithErrorRangeLength");
    int length = 102400 * (2 + rand() % 10);
    auto putObjectContent = TestUtils::GetRandomStream(length);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);
    request.setRange(20, -1);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(RemoveFile(targetKey), true);
    EXPECT_EQ(outcome.result().Metadata().ContentLength(), length - 20);
}

TEST_F(CryptoResumableObjectTest, UnnormalResumableDownloadWithErrorRangeLength)
{
    std::string sourceKey = TestUtils::GetObjectKey("UnnormalDownloadSourceObjectWithErrorRangeLength");
    std::string targetKey = TestUtils::GetObjectKey("UnnormalDownloadTargetObjectWithErrorRangeLength");
    auto putObjectContent = TestUtils::GetRandomStream(102400 * 2);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);
    request.setRange(102400, 20);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ValidateError");
}

TEST_F(CryptoResumableObjectTest, MultiResumableDwoanloadWithThreadNumberOverPartNumber)
{
    std::string sourceKey = TestUtils::GetObjectKey("MultiDownloadSourceObjectWithThreadNumberOverPartNumber");
    std::string targetKey = TestUtils::GetObjectKey("MultiDownloadTargetObjectWithThreadNumberOverPartNumber");
    auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    // download
    int threadNum = 0;
    DownloadObjectRequest request(BucketName, sourceKey, targetKey, "");
    request.setPartSize(102400);
    request.setThreadNum(threadNum);
    auto invalidateOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(invalidateOutcome.isSuccess(), false);

    threadNum = 20;
    request.setThreadNum(threadNum);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_NE(TestUtils::GetFileCRC64(targetKey), outcome.result().Metadata().CRC64());
    EXPECT_EQ(RemoveFile(targetKey), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDwoanloadWithResponseHeadersSetTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("MultiDownloadSourceObjectWithResponseHeadersSetTest");
    std::string targetKey = TestUtils::GetObjectKey("MultiDownloadTargetObjectWithResponseHeadersSetTest");
    int length = 102400 * (2 + rand() % 10);
    auto putObjectContent = TestUtils::GetRandomStream(length);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    // download
    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);
    request.setThreadNum(1);
    request.addResponseHeaders(RequestResponseHeader::CacheControl, "max-age=3");
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().CacheControl(), "max-age=3");
    EXPECT_EQ(outcome.result().Metadata().ContentLength(), length);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_NE(TestUtils::GetFileCRC64(targetKey), outcome.result().Metadata().CRC64());
    EXPECT_EQ(RemoveFile(targetKey), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithModifiedSetTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithModifiedSetTest");
    std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithModifiedSetTest");
    auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    // download
    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);

    // error set Modified-Since time
    request.setModifiedSinceConstraint(TestUtils::GetGMTString(100));
    auto modifiedOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(modifiedOutcome.isSuccess(), false);
    EXPECT_EQ(modifiedOutcome.error().Code(), "ServerError:304");

    // error set Unmodified-Since time 
    request.setModifiedSinceConstraint(TestUtils::GetGMTString(0));
    request.setUnmodifiedSinceConstraint(TestUtils::GetGMTString(-100));
    auto unmodifiedOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(unmodifiedOutcome.isSuccess(), false);
    EXPECT_EQ(unmodifiedOutcome.error().Code(), "PreconditionFailed");

    // normal download
    request.setModifiedSinceConstraint(TestUtils::GetGMTString(-100));
    request.setUnmodifiedSinceConstraint(TestUtils::GetGMTString(100));
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_NE(TestUtils::GetFileCRC64(targetKey), outcome.result().Metadata().CRC64());
    EXPECT_EQ(RemoveFile(targetKey), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDownloadWithMatchSetTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithMatchSetTest");
    std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithMatchSetTest");
    auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    auto hOutcom = Client->HeadObject(BucketName, sourceKey);
    EXPECT_EQ(hOutcom.isSuccess(), true);
    std::string realETag = hOutcom.result().ETag();
    std::vector<std::string> eTagMatchList;
    std::vector<std::string> eTagNoneMatchList;

    // download
    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);

    // error set If-Match
    eTagMatchList.push_back("invalidateETag");
    request.setMatchingETagConstraints(eTagMatchList);
    auto matchOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(matchOutcome.isSuccess(), false);
    EXPECT_EQ(matchOutcome.error().Code(), "PreconditionFailed");

    // error set If-None-Match
    eTagMatchList.clear();
    eTagNoneMatchList.push_back(realETag);
    request.setMatchingETagConstraints(eTagMatchList);
    request.setNonmatchingETagConstraints(eTagNoneMatchList);
    auto noneMatchOutcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(noneMatchOutcome.isSuccess(), false);
    EXPECT_EQ(noneMatchOutcome.error().Code(), "ServerError:304");
       
    // normal download
    eTagNoneMatchList.clear();
    eTagMatchList.push_back(realETag);
    eTagNoneMatchList.push_back("invalidateETag");
    request.setMatchingETagConstraints(eTagMatchList);
    request.setNonmatchingETagConstraints(eTagNoneMatchList);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().ContentLength(), hOutcom.result().ContentLength());
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_NE(TestUtils::GetFileCRC64(targetKey), outcome.result().Metadata().CRC64());
    EXPECT_EQ(RemoveFile(targetKey), true);
}

TEST_F(CryptoResumableObjectTest, NormalResumableDwoanloadWithoutCRCCheckTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("NormalResumableDwoanloadWithoutCRCCheckTest");
    std::string targetKey = TestUtils::GetObjectKey("NormalResumableDwoanloadWithoutCRCCheckTest");
    int length = 102400 * (2 + rand() % 10);
    auto putObjectContent = TestUtils::GetRandomStream(length);
    auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

    // download
    ClientConfiguration conf;
    conf.enableCrc64 = false;
    auto client = std::make_shared<OssEncryptionClient>(Endpoint, Config::AccessKeyId, Config::AccessKeySecret,
        ClientConfiguration(), std::make_shared<SimpleRSAEncryptionMaterials>(PublicKey, PrivateKey, Description),
        CryptoConfiguration());

    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);
    request.setThreadNum(1);
    request.addResponseHeaders(RequestResponseHeader::CacheControl, "max-age=3");
    auto outcome = client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().CacheControl(), "max-age=3");
    EXPECT_EQ(outcome.result().Metadata().ContentLength(), length);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_EQ(RemoveFile(targetKey), true);
}

TEST_F(CryptoResumableObjectTest, ResumableDwoanloadWithUnEncryptedObjectTest)
{
    std::string sourceKey = TestUtils::GetObjectKey("ResumableDwoanloadWithUnEncryptedObjectTest");
    std::string targetKey = TestUtils::GetObjectKey("NormalResumableDwoanloadWithoutCRCCheckTest");
    int length = 102400 * (2 + rand() % 10);
    auto putObjectContent = TestUtils::GetRandomStream(length);
    auto putObjectOutcome = UnEncryptionClient->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
    EXPECT_EQ(putObjectOutcome.isSuccess(), true);
    auto hOutcome = UnEncryptionClient->HeadObject(BucketName, sourceKey);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().hasUserHeader("client-side-encryption-key"), false);

    DownloadObjectRequest request(BucketName, sourceKey, targetKey);
    request.setPartSize(102400);
    auto outcome = Client->ResumableDownloadObject(request);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(outcome.result().Metadata().ContentLength(), length);
    EXPECT_EQ(outcome.result().Metadata().hasUserHeader("client-side-encryption-key"), false);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), ComputeContentMD5(*putObjectContent));
    EXPECT_EQ(RemoveFile(targetKey), true);
}

TEST_F(CryptoResumableObjectTest, ResumableUploadAndDownloadTrafficLimitTest)
{
    Timer timer;
    std::string key = TestUtils::GetObjectKey("ResumableUploadAndDownloadTrafficLimitTest");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadAndDownloadTrafficLimitTest").append(".tmp");
    /*set content 800 KB*/
    TestUtils::WriteRandomDatatoFile(tmpFile, 800 * 1024);

    //upload
    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(10240000);
    /* set upload traffic limit 200KB/s*/
    request.setTrafficLimit(819200 * 2);
    auto theory_time = (800 * 1024 * 8) / (819200 * 2);
    timer.reset();
    auto uOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(uOutcome.isSuccess(), true);
    auto diff_put = timer.elapsed();
    EXPECT_NEAR(diff_put, theory_time, 1.0);

    //download
    std::string targetKey = TestUtils::GetObjectKey("ResumableUploadAndDownloadTrafficLimitTest");
    DownloadObjectRequest dRequest(BucketName, key, targetKey);
    dRequest.setTrafficLimit(8192000);
    dRequest.setPartSize(10240000);

    auto dOutcome = Client->ResumableDownloadObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().Metadata().ContentLength(), 800 * 1024);
    EXPECT_EQ(dOutcome.result().Metadata().hasHeader("x-oss-qos-delay-time"), true);
    EXPECT_EQ(dOutcome.result().Metadata().hasUserHeader("client-side-encryption-key"), true);
    EXPECT_EQ(TestUtils::GetFileMd5(targetKey), TestUtils::GetFileMd5(tmpFile));

    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetKey), true);
 }


}
}