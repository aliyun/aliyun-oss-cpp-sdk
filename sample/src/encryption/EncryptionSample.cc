#include <iostream>
#include "../Config.h"
#include "EncryptionSample.h"
#include <alibabacloud/oss/Const.h>
#include <memory>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace AlibabaCloud::OSS;

static int64_t getFileSize(const std::string& file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

static std::string readFromFile(const std::string& file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

EncryptionSample::EncryptionSample(const std::string &bucket) :
    bucket_(bucket)
{
    std::string publicKey = readFromFile(Config::PublicKeyPath);
    std::string privateKey = readFromFile(Config::PrivateKeyPath);
    ClientConfiguration conf;
    CryptoConfiguration cryptoConf;
    auto materials = std::make_shared<SimpleRSAEncryptionMaterials>(publicKey, privateKey);
    client = new OssEncryptionClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, 
        conf, materials, cryptoConf);
    //CreateBucketRequest request(bucket_);
}

EncryptionSample::~EncryptionSample() {
    delete client;
}

void EncryptionSample::PrintError(const std::string &funcName, const OssError &error)
{
    std::cout << funcName << " fail" <<
        ",code:" << error.Code() <<
        ",message:" << error.Message() <<
        ",request_id:" << error.RequestId() << std::endl;
}

void EncryptionSample::PutObjectFromBuffer()
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << __FUNCTION__;
    PutObjectRequest request(bucket_, "PutObjectFromBuffer", content);
    auto outcome = client->PutObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, ETag:" << outcome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void EncryptionSample::PutObjectFromFile()
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(Config::FileToUpload, std::ios::in | std::ios::binary);
    PutObjectRequest request(bucket_, "PutObjectFromFile", content);
    auto outcome = client->PutObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, ETag:" << outcome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void EncryptionSample::GetObjectToBuffer()
{
    GetObjectRequest request(bucket_, "PutObjectFromBuffer");
    auto outcome = client->GetObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Content-Length:" << outcome.result().Metadata().ContentLength() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void EncryptionSample::GetObjectToFile()
{
    GetObjectRequest request(bucket_, "PutObjectFromFile");
    request.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>("GetObjectToFile", std::ios_base::out | std::ios_base::in | std::ios_base::trunc| std::ios_base::binary); });
    auto outcome = client->GetObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success" << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

static void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData)
{
    std::cout << "ProgressCallback[" << userData << "] => " <<
                 increment <<" ," << transfered << "," << total << std::endl;
}

#if !defined(DISABLE_RESUAMABLE)
void EncryptionSample::UploadObjectProgress()
{
    //case 1: checkpoint dir is not enabled
    {
        UploadObjectRequest request(bucket_, "UploadObjectProgress", Config::FileToUpload);

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableUploadObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

    //case 2: checkpoint dir is enabled
    {
        UploadObjectRequest request(bucket_, "UploadObjectProgress", Config::FileToUpload, Config::CheckpointDir);
  
        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableUploadObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

    //case 3: checkpoint dir, multi threads is enabled
    {
        UploadObjectRequest request(bucket_, "UploadObjectProgress", Config::FileToUpload, Config::CheckpointDir, DefaultPartSize, 4);

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableUploadObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }
}

void EncryptionSample::DownloadObjectProcess()
{
    //case 1: no checkpoint dir is coinfig
    {
        DownloadObjectRequest request(bucket_, "DownloadObjectProgress", Config::FileDownloadTo);

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableDownloadObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().Metadata().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

    //case 2: checkpoint dir is config
    {
        DownloadObjectRequest request(bucket_, "DownloadObjectProgress", Config::FileDownloadTo, Config::CheckpointDir);

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableDownloadObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().Metadata().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

    //case 3: checkpoint dir is config, multi threads is config
    {
        DownloadObjectRequest request(bucket_, "DownloadObjectProgress", Config::FileDownloadTo, Config::CheckpointDir, DefaultPartSize, 4);

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableDownloadObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().Metadata().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }
}
#endif

void EncryptionSample::MultipartUploadObject()
{
    std::string fileToUpload = Config::BigFileToUpload;
    std::string key = "MultipartUploadObject";
    auto fileSize = getFileSize(fileToUpload);
    
    //must be 16 bytes alignment
    int64_t partSize = 100 * 1024;

    MultipartUploadCryptoContext cryptoCtx;
    cryptoCtx.setPartSize(partSize);
    cryptoCtx.setDataSize(fileSize);

    InitiateMultipartUploadRequest initUploadRequest(bucket_, key);
    auto uploadIdResult = client->InitiateMultipartUpload(initUploadRequest, cryptoCtx);
    auto uploadId = uploadIdResult.result().UploadId();
    PartList partETagList;

    int partCount = static_cast<int>(fileSize / partSize);
    // Calculate how many parts to be divided
    if (fileSize % partSize != 0) {
        partCount++;
    }

    // Upload multiparts to bucket
    for (int i = 1; i <= partCount; i++) {
        auto skipBytes = partSize * (i - 1);
        auto size = (partSize < fileSize - skipBytes) ? partSize : (fileSize - skipBytes);
        std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(fileToUpload, std::ios::in|std::ios::binary);
        content->seekg(skipBytes, std::ios::beg);
		
        UploadPartRequest uploadPartRequest(bucket_, key, content);
        uploadPartRequest.setContentLength(size);
        uploadPartRequest.setUploadId(uploadId);
        uploadPartRequest.setPartNumber(i);
        auto uploadPartOutcome = client->UploadPart(uploadPartRequest, cryptoCtx);
        if (uploadPartOutcome.isSuccess()) {
            Part part(i, uploadPartOutcome.result().ETag());
            partETagList.push_back(part);
        }
        else {
            PrintError(__FUNCTION__, uploadPartOutcome.error());
        }
    }

    // Complete to upload multiparts
    CompleteMultipartUploadRequest request(bucket_, key);
    request.setUploadId(uploadId);
    request.setPartList(partETagList);
    auto outcome = client->CompleteMultipartUpload(request, cryptoCtx);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, UploadID : " << uploadId << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}
