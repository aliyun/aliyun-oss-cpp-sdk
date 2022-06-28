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
#include <alibabacloud/oss/Const.h>
#include "../Config.h"
#include "../Utils.h"
#include "src/utils/FileSystemUtils.h"
#include "src/utils/Utils.h"
#include "src/external/json/json.h"
#include <fstream>
#include <ctime>
#ifdef _WIN32
#include <codecvt>        // std::codecvt_utf8
#endif

namespace AlibabaCloud {
namespace OSS {

class ResumableObjectTest : public::testing::Test {
protected:
    ResumableObjectTest()
    {
    }
    ~ResumableObjectTest()override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-resumableobject");
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
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;
    static int UploadPartFailedFlag;
    static int DownloadPartFailedFlag;
    static int CopyPartFailedFlag;
};

    std::string ResumableObjectTest::GetCheckpointFileByResumableUploader(std::string bucket,
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

    std::string ResumableObjectTest::GetCheckpointFileByResumableDownloader(std::string bucket,
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

    std::string ResumableObjectTest::GetCheckpointFileByResumableCopier(std::string bucket, std::string key,
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

    void ResumableObjectTest::ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData)
    {
        std::cout << "ProgressCallback[" << userData << "] => " <<
            increment << "," << transfered << "," << total << std::endl;
    }

    std::shared_ptr<OssClient> ResumableObjectTest::Client = nullptr;
    std::string ResumableObjectTest::BucketName = "";
    int ResumableObjectTest::UploadPartFailedFlag = 0;
    int ResumableObjectTest::DownloadPartFailedFlag = 0;
    int ResumableObjectTest::CopyPartFailedFlag = 0;

    TEST_F(ResumableObjectTest, NormalResumableUploadWithSizeOverPartSizeTest)
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

        auto getObjectOutcome = Client->GetObject(BucketName, key);
        EXPECT_EQ(getObjectOutcome.isSuccess(), true);

        auto getoutcome = Client->GetObject(GetObjectRequest(BucketName, key));

        std::fstream file(tmpFile, std::ios::in | std::ios::binary);
        std::string oriMd5 = ComputeContentMD5(file);
        std::string memMd5 = ComputeContentMD5(*getObjectOutcome.result().Content());
        EXPECT_EQ(oriMd5, memMd5);

        file.close();
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableUploadWithSizeUnderPartSizeTest)
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

        auto getObjectOutcome = Client->GetObject(BucketName, key);
        EXPECT_EQ(getObjectOutcome.isSuccess(), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableObjectWithDisableRequest)
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

    TEST_F(ResumableObjectTest, MultiResumableUploadWithSizeOverPartSizeTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadSetMinPartSizeTest)
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

    TEST_F(ResumableObjectTest, UnnormalResumableUploadWithoutSourceFilePathTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutFilePath");
        std::string tmpFile;
        UploadObjectRequest request(BucketName, key, tmpFile);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, UnnormalResumableUploadWithoutRealFileTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutRealFile");
        std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUplloadObjectWithoutRealFile").append(".tmp");
        UploadObjectRequest request(BucketName, key, tmpFile);
        request.setThreadNum(1);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, UnnormalResumableUploadWithNotExitsCheckpointTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadWithCheckpointTest)
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

    TEST_F(ResumableObjectTest, MultiResumableUploadWithCheckpointTest)
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

    TEST_F(ResumableObjectTest, UnnormalResumableUploadWithFailedPartUploadTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadRetryAfterFailedPartTest)
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

    TEST_F(ResumableObjectTest, MultiResumableUploadRetryAfterFailedPartTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadRetryWithUploadPartTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalUploadObjectRetryWithUploadPart");
        std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUploadObjectRetryWithUploadPart").append(".tmp");
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

        // read the checkpoint json file
        std::string filename = GetCheckpointFileByResumableUploader(BucketName, key, checkpointKey, tmpFile);
        auto listMultipartOutcome = Client->ListMultipartUploads(ListMultipartUploadsRequest(BucketName));
        EXPECT_EQ(listMultipartOutcome.isSuccess(), true);
        auto multipartUpload = listMultipartOutcome.result().MultipartUploadList()[0];

        // resend the NO.2 part data
        std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
        content->seekg(102400, content->beg);
        UploadPartRequest uploadPartRequest(BucketName, multipartUpload.Key, 2, multipartUpload.UploadId, content);
        uploadPartRequest.setContentLength(102400);
        auto reuploadOutcome = Client->UploadPart(uploadPartRequest);
        content = nullptr;
        uploadPartRequest.setConetent(content);
        EXPECT_EQ(reuploadOutcome.isSuccess(), true);

        // Complete object
        auto listpartsOutcome = Client->ListParts(ListPartsRequest(BucketName, multipartUpload.Key, multipartUpload.UploadId));
        EXPECT_EQ(listpartsOutcome.isSuccess(), true);
        auto partlist = listpartsOutcome.result().PartList();
        CompleteMultipartUploadRequest completeRequest(BucketName, multipartUpload.Key, partlist, multipartUpload.UploadId);
        auto completeOutcome = Client->CompleteMultipartUpload(completeRequest);
        EXPECT_EQ(completeOutcome.isSuccess(), true);

        // download the file
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);
        std::string targetFile = TestUtils::GetObjectKey("DownloadUploadPartObject");
        auto getObjectOutcome = Client->GetObject(BucketName, key, targetFile);
        std::shared_ptr<std::iostream> getObjectContent = nullptr;
        getObjectOutcome.result().setContent(getObjectContent);
        EXPECT_EQ(getObjectOutcome.isSuccess(), true);

        std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string downloadMd5 = TestUtils::GetFileMd5(targetFile);
        EXPECT_EQ(uploadMd5, downloadMd5);

        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveFile(filename), true);
        EXPECT_EQ(RemoveDirectory(checkpointKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableUploadRetryWithSourceFileChangedTest)
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

    TEST_F(ResumableObjectTest, MultiResumableUploadRetryWithSourceFileChangedTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadRetryWithCheckpointFileChangedTest)
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

    TEST_F(ResumableObjectTest, MultiResumableUploadRetryWithCheckpointFileChangedTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadWithProgressCallbackTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalResumableUploadObjectWithCallback");
        std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableUploadObjectWithCallback").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (2 + rand() % 10));
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        std::cout << "this ptr:" << this << std::endl;
        TransferProgress progressCallback = { ProgressCallback, this };
        UploadObjectRequest request(BucketName, key, tmpFile, checkpointDir, 102400, 1);
        request.setTransferProgress(progressCallback);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableUploadProgressCallbackWithUploadPartFailedTest)
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

    TEST_F(ResumableObjectTest, MultiResumableUploadWithThreadNumberOverPartNumber)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadWithObjectMetaDataSetTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadWithUserMetaDataTest)
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

    TEST_F(ResumableObjectTest, NormalResumableUploadWithObjectAclSetTest)
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



    TEST_F(ResumableObjectTest, UnnormalResumableDownloadObjectWithDisableRequest)
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
        EXPECT_EQ(RemoveFile(targetKey.append(".temp")), true);
        Client->EnableRequest();
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithSizeOverPartSizeTest)
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

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithSizeUnderPartSizeTest)
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

    TEST_F(ResumableObjectTest, MultiResumableDownloadWithSizeOverPartSizeTest)
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

    TEST_F(ResumableObjectTest, NormalResumableDownloadSetMinPartSizeTest)
    {
        // upload
        std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectSetMinPartSize");
        std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectSetMinPartSize").append(".tmp");
        std::string targetFile = TestUtils::GetTargetFileName("ResumableDownloadTargetObject");
        int num = 1 + rand() % 10;
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
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

    TEST_F(ResumableObjectTest, UnnormalResumableDownloadWithoutTargetFilePathTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutFilePath");
        std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUplloadObjectWithoutFilePath").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download
        std::string targetFile;
        DownloadObjectRequest request(BucketName, key, targetFile);
        request.setPartSize(100 * 1024);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");

        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithCheckpointTest)
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

    TEST_F(ResumableObjectTest, UnnormalResumableDownloadWithNotExitsCheckpointTest)
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
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "Checkpoint directory is not exist.");

        EXPECT_EQ(RemoveFile(tmpFile), true);
        RemoveFile(targetFile.append(".temp"));
    }

    TEST_F(ResumableObjectTest, MultiResumableDownloadWithCheckpointTest)
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

    TEST_F(ResumableObjectTest, UnnormalResumableDownloadWithDownloadPartFailedTest)
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

    TEST_F(ResumableObjectTest, NormalResumableDownloadRetryWithCheckpointTest)
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

    TEST_F(ResumableObjectTest, MultiResumableDownloadRetryWithCheckpointTest)
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

    TEST_F(ResumableObjectTest, NormalResumableDownloadRetryWithSourceObjectDeletedTest)
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

    TEST_F(ResumableObjectTest, NormalResumableDownloadRetryWithCheckpointFileChangedTest)
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

        std::string sourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string targetFileMd5 = TestUtils::GetFileMd5(targetFile);
        EXPECT_EQ(sourceFileMd5, targetFileMd5);
        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableDownloadRetryWithCheckpointFileChangedTest)
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

        std::string sourceFileMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string targetFileMd5 = TestUtils::GetFileMd5(targetFile);
        EXPECT_EQ(sourceFileMd5, targetFileMd5);
        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithProgressCallbackTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithProgressCallback");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithProgressCallback");

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        std::cout << "this ptr:" << this << std::endl;
        TransferProgress progressCallback = { ProgressCallback, this };
        DownloadObjectRequest request(BucketName, sourceKey, targetKey);
        request.setTransferProgress(progressCallback);
        request.setPartSize(102400);
        request.setThreadNum(1);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(RemoveFile(targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadProgressCallbackWithDownloadPartFailedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectProgressCallbackWithPartFailed");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectProgressCallbackWithPartFailed");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
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
        EXPECT_EQ(RemoveFile(targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithRangeLength)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithRangeLength");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithRangeLength");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        DownloadObjectRequest request(BucketName, sourceKey, targetKey);
        request.setPartSize(102400);
        request.setRange(20, 30);
        request.setThreadNum(1);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().Metadata().ContentLength(), 30 - 20 + 1);
        EXPECT_EQ(RemoveFile(targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithErrorRangeLength)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithErrorRangeLength");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithErrorRangeLength");
        int length = 102400 * (2 + rand() % 10);
        auto putObjectContent = TestUtils::GetRandomStream(length);
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        DownloadObjectRequest request(BucketName, sourceKey, targetKey);
        request.setPartSize(102400);
        request.setRange(20, -1);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(RemoveFile(targetKey), true);
        EXPECT_EQ(outcome.result().Metadata().ContentLength(), length - 20);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableDownloadWithErrorRangeLength)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalDownloadSourceObjectWithErrorRangeLength");
        std::string targetKey = TestUtils::GetObjectKey("UnnormalDownloadTargetObjectWithErrorRangeLength");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * 2);
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        DownloadObjectRequest request(BucketName, sourceKey, targetKey);
        request.setPartSize(102400);
        request.setRange(102400, 20);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, MultiResumableDwoanloadWithThreadNumberOverPartNumber)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiDownloadSourceObjectWithThreadNumberOverPartNumber");
        std::string targetKey = TestUtils::GetObjectKey("MultiDownloadTargetObjectWithThreadNumberOverPartNumber");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
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
        EXPECT_EQ(RemoveFile(targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDwoanloadWithResponseHeadersSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiDownloadSourceObjectWithResponseHeadersSetTest");
        std::string targetKey = TestUtils::GetObjectKey("MultiDownloadTargetObjectWithResponseHeadersSetTest");
        int length = 102400 * (2 + rand() % 10);
        auto putObjectContent = TestUtils::GetRandomStream(length);
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
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
        EXPECT_EQ(RemoveFile(targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithModifiedSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithModifiedSetTest");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithModifiedSetTest");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
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
        EXPECT_EQ(RemoveFile(targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithMatchSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithMatchSetTest");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithMatchSetTest");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
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
        EXPECT_EQ(RemoveFile(targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDwoanloadWithoutCRCCheckTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalResumableDwoanloadWithoutCRCCheckTest");
        std::string targetKey = TestUtils::GetObjectKey("NormalResumableDwoanloadWithoutCRCCheckTest");
        int length = 102400 * (2 + rand() % 10);
        auto putObjectContent = TestUtils::GetRandomStream(length);
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // download
        ClientConfiguration conf;
        conf.enableCrc64 = false;
        OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
        DownloadObjectRequest request(BucketName, sourceKey, targetKey);
        request.setPartSize(102400);
        request.setThreadNum(1);
        request.addResponseHeaders(RequestResponseHeader::CacheControl, "max-age=3");
        auto outcome = client.ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().Metadata().CacheControl(), "max-age=3");
        EXPECT_EQ(outcome.result().Metadata().ContentLength(), length);
        EXPECT_EQ(RemoveFile(targetKey), true);
    }


    TEST_F(ResumableObjectTest, UnnormalResumableCopyObjectWithDisableRequest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalResumableCopyObjectSourceKey");
        std::string targetKey = TestUtils::GetObjectKey("UnnormalResumableCopyObjectTargetKey");
        std::string tmpFile = TestUtils::GetTargetFileName("UnnormalResumableCopyObject").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 102400 * (1 + rand() % 10));
        // upload object
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, tmpFile);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // copy object
        Client->DisableRequest();
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(102400);
        request.setThreadNum(1);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ClientError:100002");

        EXPECT_EQ(RemoveFile(tmpFile), true);
        Client->EnableRequest();
    }

    TEST_F(ResumableObjectTest, UnnormalResumableCopyOperationTest)
    {
        std::string sourceBucket = TestUtils::GetBucketName("unormal-resumable-copy-bucket-source");
        std::string targetBucket = TestUtils::GetBucketName("unormal-resumable-copy-bucket-target");
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalCopyObjectSourceKey");
        std::string targetKey = TestUtils::GetObjectKey("UnnormalCopyObjectTargetKey");
        EXPECT_EQ(Client->CreateBucket(sourceBucket).isSuccess(), true);
        EXPECT_EQ(Client->CreateBucket(targetBucket).isSuccess(), true);

        // put object into source bucket
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(PutObjectRequest(sourceBucket, sourceKey, putObjectContent));
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);

        // Copy Object to non-existent target bucket
        {
            MultiCopyObjectRequest request("notexist-target-bucket", targetKey, sourceBucket, sourceKey);
            auto outcome = Client->ResumableCopyObject(request);
            EXPECT_EQ(outcome.isSuccess(), false);
            EXPECT_EQ(outcome.error().Code(), "NoSuchBucket");
        }
        // Copy Object to non-existent source bucket
        {
            MultiCopyObjectRequest request(targetBucket, targetKey, "notexist-source-bucket", sourceKey);
            auto outcome = Client->ResumableCopyObject(request);
            EXPECT_EQ(outcome.isSuccess(), false);
        }
        // Copy Object with non-existent source key
        {
            MultiCopyObjectRequest request(targetBucket, targetKey, sourceBucket, "notexist-source-key");
            auto outcome = Client->ResumableCopyObject(request);
            EXPECT_EQ(outcome.isSuccess(), false);
        }
        // set illegal partsize parameter
        {
            MultiCopyObjectRequest request(targetBucket, targetKey, sourceBucket, sourceKey);
            request.setPartSize(rand() % 102400 - 1);
            auto outcome = Client->ResumableCopyObject(request);
            EXPECT_EQ(outcome.isSuccess(), false);
        }

        TestUtils::CleanBucket(*Client, sourceBucket);
        TestUtils::CleanBucket(*Client, targetBucket);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithSizeOverPartSizeTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectOverPartSize");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectOverPartSize");
        // put object into bucket
        int num = 1 + rand() % 10;
        auto putObjectContent = TestUtils::GetRandomStream(102400 * num);
        auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // Copy Object
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(100 * 1024);
        request.setThreadNum(1);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithSizeUnderPartSizeTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectUnderPartSize");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectUnderPartSize");
        // put Object into bucket
        auto putObjectContent = TestUtils::GetRandomStream(1024 * (rand() % 100));
        auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // copy object
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(100 * 1024 + 1);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableCopyWithSizeOverPartSizeTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiCopySourceObjectOverPartSize");
        std::string targetKey = TestUtils::GetObjectKey("MultiCopyTargetObjectOverPartSize");
        // put object
        auto putObjectContent = TestUtils::GetRandomStream(1024 * 100 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // copy object
        int threadNum = 1 + rand() % 100;
        std::string checkpointDir = TestUtils::GetExecutableDirectory();
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(1024 * 100 + 1);
        request.setThreadNum(threadNum);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableCopyWithEmptyTargetKeyTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalCopyObjectSourceKeyWithEmptyTargetKey");
        std::string targetKey;

        // put object into source bucket
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // copy object
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(102401);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, UnnormalResumableCopyWithNotExistCheckpointTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalCopySourceObjectWithNotExistCheckpoint");
        std::string targetKey = TestUtils::GetObjectKey("UnnormalCopyTargetObjectWithNotExistCheckpoint");

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjcetOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjcetOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        std::string checkpointDir = "NotExistCheckpointDir";
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(102401);
        request.setCheckpointDir(checkpointDir);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithCheckpointTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithCheckpoint");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithCheckpoint");

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjcetOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjcetOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(102400);
        request.setThreadNum(1);
        request.setCheckpointDir(TestUtils::GetExecutableDirectory());
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableCopyWithCheckpointTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiCopySourceObjectWithCheckpoint");
        std::string targetKey = TestUtils::GetObjectKey("MultiCopyTargetObjectWithCheckpoint");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjcetOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjcetOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey,
            checkpointDir, 102401, (2 + rand() % 10));
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableCopyWithUploadPartCopyFailedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalCopySourceObjectWithUploadPartCopyFailed");
        std::string targetKey = TestUtils::GetObjectKey("UnnormalCopyTargetObjectWithUploadPartCopyFailed");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, 1);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        std::string checkpointFile = GetCheckpointFileByResumableCopier(BucketName, targetKey, BucketName, sourceKey, checkpointDir);
        EXPECT_EQ(RemoveFile(checkpointFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, UnMultiResumableCopyWithUploadPartCopyFailedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiCopySourceObjectWithUploadPartCopyFailed");
        std::string targetKey = TestUtils::GetObjectKey("MultiCopyTargetObjectWithUploadPartCopyFailed");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        int threadNum = 1 + rand() % 100;
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102400, threadNum);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        std::string checkpointFile = GetCheckpointFileByResumableCopier(BucketName, targetKey, BucketName, sourceKey, checkpointDir);
        EXPECT_EQ(RemoveFile(checkpointFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyRetryWithUploadPartCopyFailedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectRetryWithUploadPartCopyFailed");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectRetryWithUploadPartCopyFailed");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, 1);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        // retry
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableCopyRetryWithUploadPartCopyFailedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiCopySourceObjectRetryWithUploadPartCopyFailed");
        std::string targetKey = TestUtils::GetObjectKey("MultiCopyTargetObjectRetryWithUploadPartCopyFailed");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        int threadNum = 1 + rand() % 100;
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, threadNum);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        // retry
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableCopyRetryWithSourceObjectDeletedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalCopySourceObjectRetryWithSourceObjectDeleted");
        std::string targetKey = TestUtils::GetObjectKey("UnnormalCopyTargetObjectRetryWithSourceObjectDeleted");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, 1);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        // delete the source object
        auto deleteObjectOutcome = Client->DeleteObject(BucketName, sourceKey);
        EXPECT_EQ(deleteObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), false);

        // retry
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        std::string checkpointFile = GetCheckpointFileByResumableCopier(BucketName, targetKey, BucketName, sourceKey, checkpointDir);
        EXPECT_EQ(RemoveFile(checkpointFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, UnMultiResumableCopyRetryWithSourceObjectDeletedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnMultiCopySourceObjectRetryWithSourceObjectDeleted");
        std::string targetKey = TestUtils::GetObjectKey("UnMultiCopyTargetObjectRetryWithSourceObjectDeleted");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        int threadNum = 1 + rand() % 100;
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, threadNum);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        // delete the source object
        auto deleteObjectOutcome = Client->DeleteObject(BucketName, sourceKey);
        EXPECT_EQ(deleteObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), false);

        // retry
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        std::string checkpointFile = GetCheckpointFileByResumableCopier(BucketName, targetKey, BucketName, sourceKey, checkpointDir);
        EXPECT_EQ(RemoveFile(checkpointFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyRetryWithCheckpointFileChangedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("source");
        std::string targetKey = TestUtils::GetObjectKey("target");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, 1);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        // modify checkpoint file
        std::string checkpointFile = GetCheckpointFileByResumableCopier(BucketName, targetKey, BucketName, sourceKey, checkpointDir);
        std::string checkpointTmpFile = std::string(checkpointFile).append(".tmp");
        std::ifstream jsonStream(checkpointFile, std::ios::in | std::ios::binary);
        Json::CharReaderBuilder rbuilder;
        Json::Value readRoot;
        Json::Value writeRoot;
        std::string uploadID = "InvaliedUploadID";
        if (Json::parseFromStream(rbuilder, jsonStream, &readRoot, nullptr)) {
        //if (reader.parse(jsonStream, readRoot)) {
            writeRoot["opType"] = readRoot["opType"].asString();
            writeRoot["uploadID"] = uploadID;
            writeRoot["srcBucket"] = readRoot["srcBucket"].asString();
            writeRoot["srcKey"] = readRoot["srckey"].asString();
            writeRoot["bucket"] = readRoot["bucket"].asString();
            writeRoot["key"] = readRoot["key"].asString();
            writeRoot["size"] = readRoot["size"].asUInt64();
            writeRoot["mtime"] = readRoot["mtime"].asString();
            writeRoot["partSize"] = readRoot["partSize"].asUInt64();
            writeRoot["md5Sum"] = readRoot["md5Sum"].asString();
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
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableCopyRetryWithCheckpointFileChangedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("source");
        std::string targetKey = TestUtils::GetObjectKey("target");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        int threadNum = 1 + rand() % 100;
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, threadNum);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        // modify checkpoint file
        std::string checkpointFile = GetCheckpointFileByResumableCopier(BucketName, targetKey, BucketName, sourceKey, checkpointDir);
        std::string checkpointTmpFile = std::string(checkpointFile).append(".tmp");
        std::ifstream jsonStream(checkpointFile, std::ios::in | std::ios::binary);
        Json::CharReaderBuilder rbuilder;
        Json::Value readRoot;
        Json::Value writeRoot;
        std::string uploadID = "InvaliedUploadID";
        if (Json::parseFromStream(rbuilder, jsonStream, &readRoot, nullptr)) {
        //if (reader.parse(jsonStream, readRoot)) {
            writeRoot["opType"] = readRoot["opType"].asString();
            writeRoot["uploadID"] = uploadID;
            writeRoot["srcBucket"] = readRoot["srcBucket"].asString();
            writeRoot["srcKey"] = readRoot["srckey"].asString();
            writeRoot["bucket"] = readRoot["bucket"].asString();
            writeRoot["key"] = readRoot["key"].asString();
            writeRoot["size"] = readRoot["size"].asUInt64();
            writeRoot["mtime"] = readRoot["mtime"].asString();
            writeRoot["partSize"] = readRoot["partSize"].asUInt64();
            writeRoot["md5Sum"] = readRoot["md5Sum"].asString();
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
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithProgressCallbackTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithProgressCallback");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithProgressCallback");

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        TransferProgress progressCallback = { ProgressCallback, this };
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setTransferProgress(progressCallback);
        request.setPartSize(102400);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyProgressCallbackWithCopyPartFailedTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectProgressCallbackWithPartFailed");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectProgressCallbackWithPartFailed");
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        TransferProgress progressCallback = { ProgressCallback, this };
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setTransferProgress(progressCallback);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        request.setCheckpointDir(checkpointDir);
        request.setPartSize(102400);
        request.setThreadNum(1);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);

        // retry
        std::cout << "Retry : " << std::endl;
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableCopyWithThreadNumberOverPartNumber)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiCopySourceObjectWithThreadNumberOverPartNumber");
        std::string targetKey = TestUtils::GetObjectKey("MultiCopyTargetObjectWithThreadNumberOverPartNumber");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // copy
        int threadNum = 0;
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, "");
        request.setPartSize(102400);
        request.setThreadNum(threadNum);
        auto invalidateOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(invalidateOutcome.isSuccess(), false);

        threadNum = 20;
        request.setThreadNum(threadNum);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithObjectMetaDataSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithObjectMetaDataSet");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithObjectMetaDataSet");

        // put object
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        ObjectMetaData meta;
        meta.setCacheControl("max-age=3");
        meta.setExpirationTime("Fri, 09 Nov 2018 05:57:16 GMT");
        // copy object
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, "", meta);
        request.setPartSize(102400);
        request.setThreadNum(1);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);

        auto hOutcome = Client->HeadObject(BucketName, targetKey);
        EXPECT_EQ(hOutcome.isSuccess(), true);
        EXPECT_EQ(hOutcome.result().CacheControl(), "max-age=3");
        EXPECT_EQ(hOutcome.result().ExpirationTime(), "Fri, 09 Nov 2018 05:57:16 GMT");
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithUserMetaDataSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithUserMetaDataSet");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithUserMetaDataSet");

        // put object
        ObjectMetaData meta;
        meta.UserMetaData()["test"] = "testvalue";
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent, meta);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        
        auto putObjectHeadOutcome = Client->HeadObject(BucketName, sourceKey);
        EXPECT_EQ(putObjectHeadOutcome.isSuccess(), true);
        EXPECT_EQ(putObjectHeadOutcome.result().UserMetaData().at("test"), "testvalue");

        // copy object
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, "", 102400, 1, meta);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);

        auto hOutcome = Client->HeadObject(BucketName, targetKey);
        EXPECT_EQ(hOutcome.isSuccess(), true);
        EXPECT_EQ(hOutcome.result().UserMetaData().at("test"), "testvalue");
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithModifiedSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithModifiedSetTest");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithModifiedSetTest");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // copy
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(102400);
        request.setThreadNum(1);

        // error set Modified-Since time
        request.setSourceIfModifiedSince(TestUtils::GetGMTString(100));
        auto modifiedOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(modifiedOutcome.isSuccess(), false);
        EXPECT_EQ(modifiedOutcome.error().Code(), "ServerError:304");

        // error set Unmodifird-Since time
        request.setSourceIfModifiedSince(TestUtils::GetGMTString(0));
        request.setSourceIfUnModifiedSince(TestUtils::GetGMTString(-100));
        auto unmodifiedOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(unmodifiedOutcome.isSuccess(), false);
        EXPECT_EQ(unmodifiedOutcome.error().Code(), "PreconditionFailed");

        // normal copy
        request.setSourceIfModifiedSince(TestUtils::GetGMTString(-100));
        request.setSourceIfUnModifiedSince(TestUtils::GetGMTString(100));
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithMatchSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithMatchSetTest");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithMatchSetTest");
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        auto hOutcome = Client->HeadObject(BucketName, sourceKey);
        EXPECT_EQ(hOutcome.isSuccess(), true);
        std::string realETag = hOutcome.result().ETag();
        std::vector<std::string> eTagMatchList;
        std::vector<std::string> eTagNoneMatchList;

        // copy
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(102400);
        request.setThreadNum(1);

        // error set If-Match
        request.setSourceIfMatchEtag("invalidateETag");
        auto matchOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(matchOutcome.isSuccess(), false);
        EXPECT_EQ(matchOutcome.error().Code(), "PreconditionFailed");

        // error set If-None-Match
        request.setSourceIfMatchEtag(realETag);
        request.setSourceIfNotMatchEtag(realETag);
        auto noneMatchOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(noneMatchOutcome.isSuccess(), false);
        EXPECT_EQ(noneMatchOutcome.error().Code(), "ServerError:304");

        // normal copy
        request.setSourceIfMatchEtag(realETag);
        request.setSourceIfNotMatchEtag("invalidateETag");
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithMetadataDirectiveTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithMetadataDirectiveTest");
        std::string copyTargetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithCopyMetadataTest");
        std::string replaceTargetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithReplaceMetadataDirectiveTest");

        // put object
        ObjectMetaData meta;
        meta.UserMetaData()["copy"] = "copyvalue";
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent, meta);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // normal copy meta
        MultiCopyObjectRequest copyRequest(BucketName, copyTargetKey, BucketName, sourceKey);
        copyRequest.setPartSize(102400);
        copyRequest.setThreadNum(1);
        copyRequest.setMetadataDirective(CopyActionList::Copy);
        auto copyOutcome = Client->ResumableCopyObject(copyRequest);
        EXPECT_EQ(copyOutcome.isSuccess(), true);
        auto copyHeadOutcome = Client->HeadObject(HeadObjectRequest(BucketName, copyTargetKey));
        EXPECT_EQ(copyHeadOutcome.isSuccess(), true);
        EXPECT_EQ(copyHeadOutcome.result().UserMetaData().at("copy"), "copyvalue");

        // replace 
        ObjectMetaData replaceMeta;
        replaceMeta.UserMetaData()["replace"] = "replacevalue";
        MultiCopyObjectRequest replaceRequest(BucketName, replaceTargetKey, BucketName, sourceKey, "", replaceMeta);
        replaceRequest.setPartSize(102400);
        replaceRequest.setMetadataDirective(CopyActionList::Replace);
        auto replaceOutcome = Client->ResumableCopyObject(replaceRequest);
        EXPECT_EQ(replaceOutcome.isSuccess(), true);
        auto replaceHeadOutcome = Client->HeadObject(HeadObjectRequest(BucketName, replaceTargetKey));
        EXPECT_EQ(replaceHeadOutcome.isSuccess(), true);
        EXPECT_EQ(replaceHeadOutcome.result().UserMetaData().at("replace"), "replacevalue");
        EXPECT_EQ(replaceHeadOutcome.result().UserMetaData().find("copy") == replaceHeadOutcome.result().UserMetaData().end(), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithObjectAclSetTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithObjectAclSetTest");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithObjectAclSetTest");
        // put object
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // set acl
        auto aclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, sourceKey));
        EXPECT_EQ(aclOutcome.isSuccess(), true);
        EXPECT_EQ(aclOutcome.result().Acl(), CannedAccessControlList::Default);

        // copy
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey);
        request.setPartSize(102400);
        request.setAcl(CannedAccessControlList::PublicReadWrite);
        request.setEncodingType("url");
        EXPECT_EQ(request.EncodingType(), "url");
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);

        auto copyAclOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, targetKey));
        EXPECT_EQ(copyAclOutcome.isSuccess(), true);
        EXPECT_EQ(copyAclOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);
    }

    TEST_F(ResumableObjectTest, ResumableUploadWithProgressCallbackTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalResumableUploadObjectWithCallback");
        std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableUploadObjectWithCallback").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 102400);
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);
        std::cout << "this ptr:" << this << std::endl;
        TransferProgress progressCallback = { ProgressCallback, this };
        UploadObjectRequest request(BucketName, key, tmpFile, checkpointDir, 102400, 1);
        request.setTransferProgress(progressCallback);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithRangeAndProgressCallbackTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalDownloadSourceObjectWithRangeLength");
        std::string targetKey = TestUtils::GetObjectKey("NormalDownloadTargetObjectWithRangeLength");
        auto putObjectContent = TestUtils::GetRandomStream(102400 - 1);
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        TransferProgress progressCallback = { ProgressCallback, this };
        DownloadObjectRequest request(BucketName, sourceKey, targetKey);
        request.setPartSize(102400);
        request.setRange(20, 30);
        request.setThreadNum(1);
        request.setTransferProgress(progressCallback);

        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().Metadata().ContentLength(), 30 - 20 + 1);
        EXPECT_EQ(RemoveFile(targetKey), true);
    }

    TEST_F(ResumableObjectTest, OssResumableBaseRequestTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalResumableUploadObjectWithCallback");
        std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableUploadObjectWithCallback").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 102400);
        std::string checkpointDir = TestUtils::GetTargetFileName("checkpoint");
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        UploadObjectRequest request(BucketName, key, tmpFile, checkpointDir, 102400, 1);
        request.setBucket(BucketName);
        request.setKey(key);
        request.setObjectSize(102400);
        request.setObjectMtime("invalid");

        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, ResumableCopierTrafficLimitTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithMetadataDirectiveTest");
        std::string copyTargetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithCopyMetadataTest");
        std::string replaceTargetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithReplaceMetadataDirectiveTest");

        // put object
        ObjectMetaData meta;
        meta.UserMetaData()["copy"] = "copyvalue";
        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent, meta);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        // normal copy meta
        MultiCopyObjectRequest copyRequest(BucketName, copyTargetKey, BucketName, sourceKey);
        copyRequest.setPartSize(102400);
        copyRequest.setThreadNum(1);
        copyRequest.setTrafficLimit(819201);
        auto copyOutcome = Client->ResumableCopyObject(copyRequest);
        EXPECT_EQ(copyOutcome.isSuccess(), true);
    }

    TEST_F(ResumableObjectTest, DownloadObjectRequestBranchTest)
    {
        DownloadObjectRequest request(BucketName, "test", "test");
        request.setRange(1,1);
        Client->ResumableDownloadObject(request);

        OssResumableBaseRequest request1("test","test","test",1,0);
    }


#ifdef _WIN32
    static std::wstring StringToWString(std::string& str)
    {
        //just cover 0x00~0x7f char
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    static std::string WStringToString(std::wstring& str)
    {
        //just cover 0x00~0x7f char
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(str);
    }

    static void WriteRandomDatatoFile(const std::wstring &file, int length)
    {
        std::fstream of(file, std::ios::out | std::ios::binary | std::ios::trunc);
        of << TestUtils::GetRandomString(length);
        of.close();
    }

    static std::string GetFileMd5(const std::wstring file)
    {
        std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(file, std::ios::in | std::ios::binary);
        return ComputeContentMD5(*content);
    }

    std::wstring GetCheckpointFileByResumableUploaderW(std::string bucket, std::string key, 
        std::wstring checkpointDir, std::wstring filePath)
    {
        std::stringstream ss;
        ss << "oss://" << bucket << "/" << key;
        auto destPath = ss.str();
        auto safeFileName = ComputeContentETag(WStringToString(filePath)) + "--" + ComputeContentETag(destPath);
        return checkpointDir + WPATH_DELIMITER + StringToWString(safeFileName);
    }

    static std::wstring GetCheckpointFileByResumableDownloaderW(std::string bucket, std::string key, 
        std::wstring checkpointDir, std::wstring filePath)
    {
        std::stringstream ss;
        ss << "oss://" << bucket << "/" << key;
        auto srcPath = ss.str();
        auto safeFileName = ComputeContentETag(srcPath) + "--" + ComputeContentETag(WStringToString(filePath));
        return checkpointDir + WPATH_DELIMITER + StringToWString(safeFileName);
    }

    static std::wstring GetCheckpointFileByResumableCopierW(std::string bucket, std::string key,
        std::string srcBucket, std::string srcKey, std::wstring checkpointDir)
    {
        std::stringstream ss;
        ss << "oss://" << srcBucket << "/" << srcKey;
        auto srcPath = ss.str();
        ss.str("");
        ss << "oss://" << bucket << "/" << key;
        auto destPath = ss.str();
        auto safeFileName = ComputeContentETag(srcPath) + "--" + ComputeContentETag(destPath);
        return checkpointDir + WPATH_DELIMITER + StringToWString(safeFileName);
    }

    //wstring path
    TEST_F(ResumableObjectTest, NormalResumableUploadWithSizeOverPartSizeWTest)
    {
        std::string key = TestUtils::GetObjectKey("ResumableUploadObjectOverPartSizeW");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("ResumableUploadObjectOverPartSizeW").append(".tmp"));
        // limit file size between 800KB and 2000KB
        int num = 8 + rand() % 12;
        WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

        UploadObjectRequest request(BucketName, key, tmpFile);
        request.setPartSize(100 * 1024);
        request.setThreadNum(1);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        auto getObjectOutcome = Client->GetObject(BucketName, key);
        EXPECT_EQ(getObjectOutcome.isSuccess(), true);

        auto getoutcome = Client->GetObject(GetObjectRequest(BucketName, key));

        std::fstream file(tmpFile, std::ios::in | std::ios::binary);
        std::string oriMd5 = ComputeContentMD5(file);
        std::string memMd5 = ComputeContentMD5(*getObjectOutcome.result().Content());
        EXPECT_EQ(oriMd5, memMd5);

        file.close();
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableUploadWithSizeUnderPartSizeWTest)
    {
        std::string key = TestUtils::GetObjectKey("ResumableUploadObjectUnderPartSizeW");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("ResumableUploadObjectUnderPartSizeW").append(".tmp"));
        int num = rand() % 8;
        WriteRandomDatatoFile(tmpFile, 10240 * num);

        UploadObjectRequest request(BucketName, key, tmpFile);
        request.setPartSize(100 * 1024);
        request.setThreadNum(1);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        auto getObjectOutcome = Client->GetObject(BucketName, key);
        EXPECT_EQ(getObjectOutcome.isSuccess(), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableObjectWithDisableRequestWTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUploadObjectWithDisableRequestW");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("UnnormalUploadObjectWithDisableRequestW").append(".tmp"));
        WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));
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

    TEST_F(ResumableObjectTest, MultiResumableUploadWithSizeOverPartSizeWTest)
    {
        std::string key = TestUtils::GetObjectKey("MultiUploadObjectOverPartSizeW");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("MultiUploadObjectOverPartSizeW").append(".tmp"));
        int num = 8 + rand() % 12;
        WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
        int threadNum = 1 + rand() % 99;

        UploadObjectRequest request(BucketName, key, tmpFile);
        request.setPartSize(100 * 1024);
        request.setThreadNum(threadNum);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableUploadSetMinPartSizeWTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalUploadObjectSetMinPartSizeW");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("NormalUploadObjectSetMinPartSizeW").append(".tmp"));
        int num = 1 + rand() % 20;
        WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
        int partSize = 1 + rand() % 99;

        UploadObjectRequest request(BucketName, key, tmpFile);
        request.setPartSize(partSize * 102400);
        request.setThreadNum(1);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableUploadWithoutSourceFilePathWTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutFilePathW");
        std::wstring tmpFile;
        UploadObjectRequest request(BucketName, key, tmpFile);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, UnnormalResumableUploadWithoutRealFileWTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutRealFileW");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("UnnormalUplloadObjectWithoutRealFileW").append(".tmp"));
        UploadObjectRequest request(BucketName, key, tmpFile);
        request.setThreadNum(1);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, UnnormalResumableUploadWithNotExitsCheckpointWTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUploadObjectWithNotExitsCheckpoint");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("UnnormalUploadObjectWithNotExitsCheckpoint").append(".tmp"));
        WriteRandomDatatoFile(tmpFile, 100);

        UploadObjectRequest request(BucketName, key, tmpFile, L"NotExistDir");
        request.setPartSize(100 * 1024);
        request.setThreadNum(1);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");

        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableUploadRetryAfterFailedPartWTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalResumableUploadRetryAfterFailedPartWTest");
        std::wstring tmpFile = StringToWString(TestUtils::GetTargetFileName("NormalResumableUploadRetryAfterFailedPartWTest").append(".tmp"));
        WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
        std::wstring checkpointKey = StringToWString(TestUtils::GetObjectKey("checkpoint"));
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

        EXPECT_EQ(IsFileExist(GetCheckpointFileByResumableUploaderW(BucketName, key, checkpointKey, tmpFile)), true);

        // retry
        request.setFlags(request.Flags() ^ UploadPartFailedFlag);
        auto retryOutcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointKey), true);
    }

    TEST_F(ResumableObjectTest, ResumableUploadWithMixPathTypeTest)
    {
        std::wstring checkpointDir = StringToWString(TestUtils::GetTargetFileName("checkpoint"));
        EXPECT_EQ(CreateDirectory(checkpointDir), true);

        UploadObjectRequest request(BucketName, "targetKey", "filePath");
        request.setCheckpointDir(checkpointDir);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "The type of filePath and checkpointDir should be the same, either string or wstring.");

        UploadObjectRequest request1(BucketName, "targetKey", L"filePath");
        request1.setCheckpointDir(WStringToString(checkpointDir));
        outcome = Client->ResumableUploadObject(request1);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "The type of filePath and checkpointDir should be the same, either string or wstring.");

        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithSizeOverPartSizeWTest)
    {
        // upload object
        std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectOverPartSizeW");
        std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectOverPartSizeW").append(".tmp");
        int num = 1 + rand() % 10;
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download object
        std::wstring targetFile = StringToWString(TestUtils::GetTargetFileName("ResumableDownloadTargetObjectW"));
        DownloadObjectRequest request(BucketName, key, targetFile);
        request.setPartSize(100 * 1024);
        request.setThreadNum(1);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string downloadMd5 = GetFileMd5(targetFile);
        EXPECT_EQ(uploadMd5, downloadMd5);

        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithSizeUnderPartSizeWTest)
    {
        // upload object
        std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectUnderPartSize");
        std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectUnderPartSize").append(".tmp");
        int num = 10 + rand() % 10;
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download object
        std::wstring targetFile = StringToWString(TestUtils::GetTargetFileName("ResumableDownloadTargetObjectW"));
        DownloadObjectRequest request(BucketName, key, targetFile);
        request.setThreadNum(1);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string downloadMd5 = GetFileMd5(targetFile);
        EXPECT_EQ(uploadMd5, downloadMd5);

        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableDownloadWithSizeOverPartSizeWTest)
    {
        // upload
        std::string key = TestUtils::GetObjectKey("MultiResumableDownloadObjectOverPartSizeW");
        std::string tmpFile = TestUtils::GetTargetFileName("MultiResumableDownloadObjectOverPartSizeW").append(".tmp");
        int num = 1 + rand() % 10;
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
        int threadNum = 1 + rand() % 100;
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download
        std::wstring targetFile = StringToWString(TestUtils::GetTargetFileName("ResumableDownloadTargetObjectW"));
        DownloadObjectRequest request(BucketName, key, targetFile);
        request.setPartSize(100 * 1024);
        request.setThreadNum(threadNum);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string downloadMd5 = GetFileMd5(targetFile);
        EXPECT_EQ(uploadMd5, downloadMd5);

        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadSetMinPartSizeWTest)
    {
        // upload
        std::string key = TestUtils::GetObjectKey("ResumableDownloadObjectSetMinPartSizeW");
        std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadObjectSetMinPartSizeW").append(".tmp");
        int num = 1 + rand() % 10;
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num);
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download
        std::wstring targetFile = StringToWString(TestUtils::GetTargetFileName("ResumableDownloadTargetObjectW"));
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
        std::string downloadMd5 = GetFileMd5(targetFile);
        EXPECT_EQ(uploadMd5, downloadMd5);

        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableDownloadWithoutTargetFilePathWTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalUplloadObjectWithoutFilePathW");
        std::string tmpFile = TestUtils::GetTargetFileName("UnnormalUplloadObjectWithoutFilePathW").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download
        std::wstring targetFile;
        DownloadObjectRequest request(BucketName, key, targetFile);
        request.setPartSize(100 * 1024);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");

        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadWithCheckpointWTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalDownloadObjectWithCheckpointW");
        std::string tmpFile = TestUtils::GetTargetFileName("NormalDownloadObjectWithCheckpointW").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));

        // upload
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download
        std::wstring checkpointDir = TestUtils::GetExecutableDirectoryW();
        std::wstring targetFile = StringToWString(TestUtils::GetTargetFileName("ResumableDownloadTargetObjectW"));
        DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir);
        request.setPartSize(100 * 1024);
        request.setThreadNum(1);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string downloadMd5 = GetFileMd5(targetFile);
        EXPECT_EQ(uploadMd5, downloadMd5);

        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableDownloadWithNotExitsCheckpointWTest)
    {
        std::string key = TestUtils::GetObjectKey("UnnormalDownloadObjectWithNotExistCheckpointW");
        std::string tmpFile = TestUtils::GetTargetFileName("UnnormalDownloadObjectWithNotExistCheckpointW").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));

        // upload
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download
        std::wstring checkpointDir = L"NotExistDir";
        std::wstring targetFile = StringToWString(TestUtils::GetTargetFileName("ResumableDownloadTargetObjectW"));
        DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir);
        request.setPartSize(100 * 1024);
        auto outcome = Client->ResumableDownloadObject(request);
        std::shared_ptr<std::iostream> content = nullptr;
        outcome.result().setContent(content);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "Checkpoint directory is not exist.");

        EXPECT_EQ(RemoveFile(tmpFile), true);
        RemoveFile(targetFile.append(L".temp"));
    }

    TEST_F(ResumableObjectTest, MultiResumableDownloadWithCheckpointWTest)
    {
        std::string key = TestUtils::GetObjectKey("MultiDownloadObjectWithCheckpointW");
        std::string tmpFile = TestUtils::GetTargetFileName("MultiDownloadObjectWithCheckpointW").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (1 + rand() % 10));

        // upload
        auto uploadOutcome = Client->ResumableUploadObject(UploadObjectRequest(BucketName, key, tmpFile));
        EXPECT_EQ(uploadOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

        // download
        std::wstring checkpointDir = TestUtils::GetExecutableDirectoryW();
        std::wstring targetFile = StringToWString(TestUtils::GetTargetFileName("ResumableDownloadTargetObjectW"));
        int threadNum = 1 + rand() % 99;
        DownloadObjectRequest request(BucketName, key, targetFile, checkpointDir);
        request.setPartSize(100 * 1024);
        request.setThreadNum(threadNum);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);

        std::string uploadMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string downloadMd5 = GetFileMd5(targetFile);
        EXPECT_EQ(uploadMd5, downloadMd5);

        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveFile(tmpFile), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableDownloadRetryWithCheckpointWTest)
    {
        std::string key = TestUtils::GetObjectKey("NormalResumableDownloadRetryWithCheckpointWTest");
        std::string tmpFile = TestUtils::GetTargetFileName("NormalResumableDownloadRetryWithCheckpointWTest").append(".tmp");
        TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * (2 + rand() % 10));
        std::wstring targetFile = StringToWString(TestUtils::GetObjectKey("ResumableDownloadTargetObject"));
        std::wstring checkpointDir = StringToWString(TestUtils::GetTargetFileName("checkpoint"));
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

        EXPECT_EQ(IsFileExist(GetCheckpointFileByResumableDownloaderW(BucketName, key, checkpointDir, targetFile)), true);

        // retry
        request.setFlags(request.Flags() ^ DownloadPartFailedFlag);
        auto retryOutcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);

        std::string uploadFileMd5 = TestUtils::GetFileMd5(tmpFile);
        std::string downloadFileMd5 = GetFileMd5(targetFile);
        EXPECT_EQ(uploadFileMd5, downloadFileMd5);
        EXPECT_EQ(RemoveFile(tmpFile), true);
        EXPECT_EQ(RemoveFile(targetFile), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, ResumableDownloadWithMixPathTypeTest)
    {
        std::wstring checkpointDir = StringToWString(TestUtils::GetTargetFileName("checkpoint"));
        EXPECT_EQ(CreateDirectory(checkpointDir), true);

        DownloadObjectRequest request(BucketName, "targetKey", "filePath");
        request.setCheckpointDir(checkpointDir);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "The type of filePath and checkpointDir should be the same, either string or wstring.");

        DownloadObjectRequest request1(BucketName, "targetKey", L"filePath");
        request1.setCheckpointDir(WStringToString(checkpointDir));
        outcome = Client->ResumableDownloadObject(request1);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "The type of filePath and checkpointDir should be the same, either string or wstring.");

        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, UnnormalResumableCopyWithNotExistCheckpointWTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("UnnormalCopySourceObjectWithNotExistCheckpointW");
        std::string targetKey = TestUtils::GetObjectKey("UnnormalCopyTargetObjectWithNotExistCheckpointW");

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjcetOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjcetOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        std::wstring checkpointDir = L"NotExistCheckpointDir";
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir);
        request.setPartSize(102401);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyWithCheckpointWTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObjectWithCheckpointW");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObjectWithCheckpointW");

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjcetOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjcetOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, TestUtils::GetExecutableDirectoryW());
        request.setPartSize(102400);
        request.setThreadNum(1);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
    }

    TEST_F(ResumableObjectTest, MultiResumableCopyWithCheckpointWTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("MultiCopySourceObjectWithCheckpointW");
        std::string targetKey = TestUtils::GetObjectKey("MultiCopyTargetObjectWithCheckpointW");

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjcetOutcome = Client->PutObject(PutObjectRequest(BucketName, sourceKey, putObjectContent));
        EXPECT_EQ(putObjcetOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        std::wstring checkpointDir = StringToWString(TestUtils::GetTargetFileName("checkpoint"));
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey,
            checkpointDir, 102401, (2 + rand() % 10));
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, NormalResumableCopyRetryWithCheckpointWTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalResumableCopyRetryWithCheckpointWTest");
        std::string targetKey = TestUtils::GetObjectKey("NormalResumableCopyRetryWithCheckpointWTest-1");
        std::wstring checkpointDir = StringToWString(TestUtils::GetTargetFileName("checkpoint"));
        EXPECT_EQ(CreateDirectory(checkpointDir), true);
        EXPECT_EQ(IsDirectoryExist(checkpointDir), true);

        auto putObjectContent = TestUtils::GetRandomStream(102400 * (2 + rand() % 10));
        auto putObjectOutcome = Client->PutObject(BucketName, sourceKey, putObjectContent);
        EXPECT_EQ(putObjectOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, sourceKey), true);

        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, checkpointDir, 102401, 1);
        request.setFlags(request.Flags() | CopyPartFailedFlag);
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), false);

        EXPECT_EQ(IsFileExist(GetCheckpointFileByResumableCopierW(BucketName, targetKey, BucketName, sourceKey, checkpointDir)), true);

        // retry
        request.setFlags(request.Flags() ^ CopyPartFailedFlag);
        auto retryOutcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(retryOutcome.isSuccess(), true);
        EXPECT_EQ(Client->DoesObjectExist(BucketName, targetKey), true);
        EXPECT_EQ(RemoveDirectory(checkpointDir), true);
    }

    TEST_F(ResumableObjectTest, ResumableCopyWithMixPathTypeTest)
    {
        MultiCopyObjectRequest request(BucketName, "targetKey", BucketName, "sourceKey", "checkPoint");
        EXPECT_EQ(request.CheckpointDirW().empty(), true);
        EXPECT_EQ(request.CheckpointDir(), "checkPoint");
        
        request.setCheckpointDir(L"check");
        EXPECT_EQ(request.CheckpointDirW(), L"check");
        EXPECT_EQ(request.CheckpointDir().empty(), true);
    }

#else
    //not support wstring path in non-windows
    TEST_F(ResumableObjectTest, NormalResumableUploadWithSizeOverPartSizeWTest)
    {
        UploadObjectRequest request(BucketName, "key", L"TestKey");
        request.setPartSize(100 * 1024);
        request.setThreadNum(1);
        auto outcome = Client->ResumableUploadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "Only support wstring path in windows os.");

        UploadObjectRequest request1(BucketName, "key", L"testFile", L"TestDir");
        request1.setPartSize(100 * 1024);
        request1.setThreadNum(1);
        outcome = Client->ResumableUploadObject(request1);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "Only support wstring path in windows os.");
    }

    TEST_F(ResumableObjectTest, MultiResumableDownloadWithCheckpointWTest)
    {
        DownloadObjectRequest request(BucketName, "key", L"TestKey");
        request.setPartSize(100 * 1024);
        request.setThreadNum(2);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "Only support wstring path in windows os.");

        DownloadObjectRequest request1(BucketName, "key", L"TestKey", L"TestDir");
        request1.setPartSize(100 * 1024);
        request1.setThreadNum(2);
        outcome = Client->ResumableDownloadObject(request1);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "Only support wstring path in windows os.");
    }

    TEST_F(ResumableObjectTest, MultiResumableCopyWithWPathTest)
    {
        MultiCopyObjectRequest request(BucketName, "key", BucketName, "srcKey",
            L"checkpoint", 102401, (2 + rand() % 10));
        auto outcome = Client->ResumableCopyObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
        EXPECT_EQ(outcome.error().Message(), "Only support wstring path in windows os.");
    }

#endif

    TEST_F(ResumableObjectTest, MultiResumableInvalidBucketNameTest)
    {
        DownloadObjectRequest request("Invalid-Bucket", "key", L"filePath", L"TestKey");
        EXPECT_EQ(request.CheckpointDir().empty(), true);
        EXPECT_EQ(request.CheckpointDirW(), L"TestKey");
        request.setCheckpointDir("TestKey");
        EXPECT_EQ(request.CheckpointDir(), "TestKey");
        EXPECT_EQ(request.CheckpointDirW().empty(), true);
        request.setPartSize(100 * 1024);
        request.setThreadNum(2);
        auto outcome = Client->ResumableDownloadObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }

    TEST_F(ResumableObjectTest, MultiCopyObjectRequestTest)
    {
        std::string sourceKey = TestUtils::GetObjectKey("NormalCopySourceObject");
        std::string targetKey = TestUtils::GetObjectKey("NormalCopyTargetObject");

        // copy
        MultiCopyObjectRequest request(BucketName, targetKey, BucketName, sourceKey, L"CheckPoint");
        EXPECT_EQ(request.CheckpointDirW(), L"CheckPoint");

        MultiCopyObjectRequest request1(BucketName, targetKey, BucketName, sourceKey, L"CheckPoint1", ObjectMetaData());
        EXPECT_EQ(request1.CheckpointDirW(), L"CheckPoint1");

        MultiCopyObjectRequest request2(BucketName, targetKey, BucketName, sourceKey, L"CheckPoint2", 100*1024, 2, ObjectMetaData());
        EXPECT_EQ(request2.CheckpointDirW(), L"CheckPoint2");
    }

    TEST_F(ResumableObjectTest, UploadObjectRequestTest)
    {
        std::string key = TestUtils::GetObjectKey("UploadObjectRequestTest");

        UploadObjectRequest reqeust(BucketName, key, L"filePath1", L"checkPoint1", 100 * 1024, 2, ObjectMetaData());
        EXPECT_EQ(reqeust.CheckpointDirW(), L"checkPoint1");

        UploadObjectRequest reqeust1(BucketName, key, L"filePath2", L"checkPoint2", ObjectMetaData());
        EXPECT_EQ(reqeust1.CheckpointDirW(), L"checkPoint2");
    }
    
}
}