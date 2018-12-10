#include <iostream>
#include "../Config.h"
#include "PresignedUrlSample.h"
#include <memory>
#include <sstream>
#include <fstream>
#include <ctime>

using namespace AlibabaCloud::OSS;

PresignedUrlSample::PresignedUrlSample(const std::string &bucket) :
    bucket_(bucket)
{
    ClientConfiguration conf;
    client = new OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    CreateBucketRequest request(bucket_);
    client->CreateBucket(request);

    key_ = "PresignedUrlSample";
    auto content = std::make_shared<std::stringstream>();
    *content << "PresignedUrlSample For Test";
    client->PutObject(PutObjectRequest(bucket_, key_, content));
}

PresignedUrlSample::~PresignedUrlSample() {
    delete client;
}

void PresignedUrlSample::PrintError(const std::string &funcName, const OssError &error)
{
    std::cout << funcName << " fail" <<
        ",code:" << error.Code() <<
        ",message:" << error.Message() <<
        ",request_id:" << error.RequestId() << std::endl;
}

void PresignedUrlSample::GenPutPresignedUrl()
{
    std::string key = "GenPutPresignedUrl";
    GeneratePresignedUrlRequest request(bucket_, key, Http::Put);
    std::time_t t = std::time(nullptr) + 1200;
    request.setExpires(t);

    auto outcome = client->GeneratePresignedUrl(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, url:" << outcome.result().c_str() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}


void PresignedUrlSample::GenGetPresignedUrl()
{
    GeneratePresignedUrlRequest request(bucket_, key_, Http::Get);
    std::time_t t = std::time(nullptr) + 1200;
    request.setExpires(t);
    auto outcome = client->GeneratePresignedUrl(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, url:" << outcome.result().c_str() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void PresignedUrlSample::PutObjectByUrlFromBuffer() 
{
    std::time_t t = std::time(nullptr) + 1200;
    auto genOutcome = client->GeneratePresignedUrl(bucket_, "PutObjectByUrlFromBuffer", t, Http::Put);
    if (genOutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Gen url:" << genOutcome.result().c_str() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, genOutcome.error());
    }

    auto content = std::make_shared<std::stringstream>();
    *content << __FILE__ << __FUNCTION__ << std::endl;

    auto outome = client->PutObjectByUrl(genOutcome.result(), content);
    if (outome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, eTag:" << outome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outome.error());
    }
}

void PresignedUrlSample::PutObjectByUrlFromFile()
{
    std::time_t t = std::time(nullptr) + 1200;
    auto genOutcome = client->GeneratePresignedUrl(bucket_, "PutObjectByUrlFromFile", t, Http::Put);
    if (genOutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Gen url:" << genOutcome.result().c_str() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, genOutcome.error());
    }

    auto outome = client->PutObjectByUrl(genOutcome.result(), __FILE__);
    if (outome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, eTag:" << outome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outome.error());
    }
}

void PresignedUrlSample::GetObjectByUrlToBuffer()
{
    std::time_t t = std::time(nullptr) + 1200;
    auto genOutcome = client->GeneratePresignedUrl(bucket_, "GetObjectByUrlToBuffer", t, Http::Put);
    if (genOutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Gen url:" << genOutcome.result().c_str() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, genOutcome.error());
    }

    auto content = std::make_shared<std::stringstream>();
    *content << __FILE__ << __FUNCTION__ << std::endl;

    auto outome = client->PutObjectByUrl(genOutcome.result(), content);
    if (outome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, eTag:" << outome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outome.error());
    }

    genOutcome = client->GeneratePresignedUrl(bucket_, "GetObjectByUrlToBuffer", t, Http::Get);
    auto getOutome = client->GetObjectByUrl(genOutcome.result());
    if (getOutome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, eTag:" << getOutome.result().Metadata().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outome.error());
    }
}

void PresignedUrlSample::GetObjectByUrlToFile()
{
    std::time_t t = std::time(nullptr) + 1200;
    auto genOutcome = client->GeneratePresignedUrl(bucket_, "GetObjectByUrlToFile", t, Http::Put);
    if (genOutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Gen url:" << genOutcome.result().c_str() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, genOutcome.error());
    }

    auto outome = client->PutObjectByUrl(genOutcome.result(), __FILE__);
    if (outome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, eTag:" << outome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outome.error());
    }

    genOutcome = client->GeneratePresignedUrl(bucket_, "GetObjectByUrlToFile", t, Http::Get);
    auto getOutome = client->GetObjectByUrl(genOutcome.result(), "GetObjectByUrlToFile_Donwload");
    if (getOutome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, eTag:" << getOutome.result().Metadata().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outome.error());
    }
}
