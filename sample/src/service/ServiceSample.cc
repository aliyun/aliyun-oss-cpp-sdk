#include <iostream>

#include "../Config.h"
#include "ServiceSample.h"
using namespace AlibabaCloud::OSS;

ServiceSample::ServiceSample() {
    ClientConfiguration conf;
    client = new OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
}

ServiceSample::~ServiceSample() {
    delete client;
}

void ServiceSample::PrintError(const std::string &funcName, const OssError &error)
{
    std::cout << funcName << " fail" <<
        ",code:" << error.Code() <<
        ",message:" << error.Message() <<
        ",request_id:" << error.RequestId() << std::endl;
}

void ServiceSample::ListBuckets()  
{
    ListBucketsRequest request;
    auto outcome = client->ListBuckets(request);

    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ <<" success, and bucket count is" << outcome.result().Buckets().size() << std::endl;
    } else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ServiceSample::ListBucketsWithMarker()
{
    ListBucketsRequest request;
    request.setMaxKeys(100);
    bool IsTruncated = false;
    size_t total = 0;
    do {
        auto outcome = client->ListBuckets(request);

        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " success, " <<
                        "and bucket count is " << outcome.result().Buckets().size() <<
                        "next marker is " << outcome.result().NextMarker() << std::endl;
            total += outcome.result().Buckets().size();
        } else {
            PrintError(__FUNCTION__, outcome.error());
            break;
        }

        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
    } while (IsTruncated);

    std::cout << __FUNCTION__ <<" done, and total is " << total << std::endl;
}

void ServiceSample::ListBucketsWithPrefix()
{
    ListBucketsRequest request;
    request.setMaxKeys(1);
    request.setPrefix("cpp-sdk");
    bool IsTruncated = false;
    size_t total = 0;
    do {
        auto outcome = client->ListBuckets(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " success, " <<
                "and bucket count is " << outcome.result().Buckets().size() <<
                "next marker is " << outcome.result().NextMarker() << std::endl;
            total += outcome.result().Buckets().size();
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
            break;
        }

        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
    } while (IsTruncated);

    std::cout << __FUNCTION__ << " done, and total is " << total << std::endl;
}

