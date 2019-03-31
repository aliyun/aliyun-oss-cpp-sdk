#include <iostream>
#include "../Config.h"
#include "BucketSample.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

using namespace AlibabaCloud::OSS;

static void waitTimeinSec(int time)
{
#ifdef _WIN32
    Sleep(time * 1000);
#else
    sleep(time);
#endif
}

BucketSample::BucketSample(const std::string &bucket):
    bucket_(bucket)
{
    ClientConfiguration conf;
    client = new OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    CreateBucketRequest request(bucket_);
    client->CreateBucket(request);
}

BucketSample::~BucketSample() {
    delete client;
}

void BucketSample::PrintError(const std::string &funcName, const OssError &error)
{
    std::cout << funcName << " fail" <<
        ",code:" << error.Code() <<
        ",message:" << error.Message() <<
        ",request_id:" << error.RequestId() << std::endl;
}

void BucketSample::InvalidBucketName()
{
    SetBucketAclRequest request("invalid-BucketName", CannedAccessControlList::Private);
    auto outcome = client->SetBucketAcl(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " to private success " << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    SetBucketLoggingRequest log_request("invalid-BucketName", bucket_, "LogPrefix");
    auto log_outcome = client->SetBucketLogging(log_request);
    if (log_outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success " << std::endl;
    }
    else {
        PrintError(__FUNCTION__, log_outcome.error());
    }
}

void BucketSample::CreateAndDeleteBucket()
{
    std::string bucket = bucket_ + "-createbucketsample";
    CreateBucketRequest request(bucket, StorageClass::IA, CannedAccessControlList::PublicReadWrite);
    auto outcome = client->CreateBucket(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " create bucket success " << std::endl;
    } else {
        PrintError(__FUNCTION__, outcome.error());
    }

    DeleteBucketRequest drequest(bucket);
    auto doutcome = client->DeleteBucket(drequest);
    if (doutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " delete bucket success " << std::endl;
    }
    else {
        PrintError(__FUNCTION__, doutcome.error());
    }
}

void BucketSample::SetBucketAcl()
{
    SetBucketAclRequest request(bucket_, CannedAccessControlList::Private);
    auto outcome = client->SetBucketAcl(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " to private success " << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    request.setAcl(CannedAccessControlList::PublicReadWrite);
    auto outcome1 = client->SetBucketAcl(request);
    if (outcome1.isSuccess()) {
        std::cout << __FUNCTION__ << " to public-read-write success " << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome1.error());
    }
}

void BucketSample::SetBucketLogging()
{
    SetBucketLoggingRequest request(bucket_, bucket_, "LogPrefix");
    auto outcome = client->SetBucketLogging(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId().c_str() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::SetBucketWebsite()
{
    SetBucketWebsiteRequest request(bucket_);
    request.setIndexDocument("index.html");
    request.setIndexDocument("error.html");
    auto outcome = client->SetBucketWebsite(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::SetBucketReferer()
{
    SetBucketRefererRequest request(bucket_);
    request.addReferer("http://www.referersample.com");
    request.addReferer("https://www.referersample.com");
    request.addReferer("https://www.?.referersample.com");
    request.addReferer("https://www.*.cn");
    auto outcome = client->SetBucketReferer(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::SetBucketLifecycle()
{
    SetBucketLifecycleRequest request(bucket_);
    std::string date("2022 - 10 - 12T00:00 : 00.000Z");
    
    auto rule1 = LifecycleRule();
    rule1.setID("rule1");
    rule1.setPrefix("test/");
    rule1.setStatus(RuleStatus::Enabled);
    rule1.setExpiration(3);
    
    auto rule2 = LifecycleRule();
    rule2.setID("rule2");
    rule2.setPrefix("test/");
    rule2.setStatus(RuleStatus::Disabled);
    rule2.setExpiration(date);
    
    auto rule3 = LifecycleRule();
    rule3.setID("rule3");
    rule3.setPrefix("test1/");
    rule3.setStatus(RuleStatus::Enabled);
    rule3.setAbortMultipartUpload(3);
    
    auto rule4 = LifecycleRule();
    rule4.setID("rule4");
    rule4.setPrefix("test1/");
    rule4.setStatus(RuleStatus::Enabled);
    rule4.setAbortMultipartUpload(date);
    
    LifecycleRuleList list{rule1, rule2, rule3, rule4};
    request.setLifecycleRules(list);
    auto outcome = client->SetBucketLifecycle(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::SetBucketCors()
{
    SetBucketCorsRequest request(bucket_);
    auto rule1 = CORSRule();
    // Note: AllowedOrigin & AllowdMethod must not be empty.
    rule1.addAllowedOrigin("http://www.a.com");
    rule1.addAllowedMethod("POST");
    rule1.addAllowedHeader("*");
    rule1.addExposeHeader("x-oss-test");
    request.addCORSRule(rule1);
    
    auto rule2 = CORSRule();
    rule2.addAllowedOrigin("http://www.b.com");
    rule2.addAllowedMethod("GET");
    rule2.addExposeHeader("x-oss-test2");
    rule2.setMaxAgeSeconds(100);
    request.addCORSRule(rule2);
    
    auto outcome = client->SetBucketCors(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::DeleteBucketLogging()
{
    DeleteBucketLoggingRequest request(bucket_);
    auto outcome = client->DeleteBucketLogging(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::DeleteBucketWebsite()
{
    DeleteBucketWebsiteRequest request(bucket_);
    auto outcome = client->DeleteBucketWebsite(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::DeleteBucketLifecycle()
{
    DeleteBucketLifecycleRequest request(bucket_);
    auto outcome = client->DeleteBucketLifecycle(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::DeleteBucketCors()
{
    auto outcome = client->DeleteBucketCors(bucket_);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, request_id:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::ListObjects()
{
    ListObjectsRequest request(bucket_);
    auto outcome = client->ListObjects(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success " << 
            "and object count is " << outcome.result().ObjectSummarys().size() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::ListObjectWithMarker()
{
    ListObjectsRequest request(bucket_);
    request.setMaxKeys(1);
    bool IsTruncated = false;
    size_t total = 0;
    do {
        auto outcome = client->ListObjects(request);

        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " success, " <<
                "and object count is " << outcome.result().ObjectSummarys().size() <<
                "next marker is " << outcome.result().NextMarker() << std::endl;
            total += outcome.result().ObjectSummarys().size();
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

void BucketSample::ListObjectWithEncodeType()
{
    ListObjectsRequest request(bucket_);
    request.setEncodingType("url");
    bool IsTruncated = false;
    size_t total = 0;
    request.setMaxKeys(1);
    ListObjectOutcome outcome;
    do {
        outcome = client->ListObjects(request);

        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " success, " <<
                "and object count is " << outcome.result().ObjectSummarys().size() <<
                "next marker is " << outcome.result().NextMarker() << std::endl;
            total += outcome.result().ObjectSummarys().size();
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

void BucketSample::GetBucketAcl()
{
    GetBucketAclRequest request(bucket_);
    auto outcome = client->GetBucketAcl(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, ori acl: " << outcome.result().Acl() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    SetBucketAclRequest request1(bucket_, CannedAccessControlList::PublicRead);
    client->SetBucketAcl(request1);
    waitTimeinSec(1);

    outcome = client->GetBucketAcl(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, after set public-read, acl:" << outcome.result().Acl() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::GetBucketLocation()
{
    GetBucketLocationRequest request(bucket_);
    auto outcome = client->GetBucketLocation(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, location: " << outcome.result().Location() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    outcome = client->GetBucketLocation(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, location: " << outcome.result().Location() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::GetBucketLogging()
{
    GetBucketLoggingRequest request(bucket_);
    auto outcome = client->GetBucketLogging(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, default TargetBucket: " << outcome.result().TargetBucket() << 
            ",TargetPrefix: " << outcome.result().TargetPrefix() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    SetBucketLoggingRequest request1(bucket_, bucket_, "LogPrefix-00");
    client->SetBucketLogging(request1);
    waitTimeinSec(1);

    outcome = client->GetBucketLogging(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, after set itself with LogPrefix-00 , TargetBucket: " << outcome.result().TargetBucket() <<
            " ,TargetPrefix: " << outcome.result().TargetPrefix() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    std::string bucket_01 = bucket_ + "01";
    CreateBucketRequest request0(bucket_01);
    client->CreateBucket(request0);

    SetBucketLoggingRequest request2(bucket_, bucket_01, "LogPrefix-01");
    client->SetBucketLogging(request2);
    waitTimeinSec(1);

    outcome = client->GetBucketLogging(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, after set other with LogPrefix-01 TargetBucket: " << outcome.result().TargetBucket() <<
            " ,TargetPrefix: " << outcome.result().TargetPrefix() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::GetBucketWebsite()
{
    GetBucketWebsiteRequest request(bucket_);
    auto outcome = client->GetBucketWebsite(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, default, IndexDocument: " << outcome.result().IndexDocument() <<
            " ,ErrorDocument: " << outcome.result().ErrorDocument() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    SetBucketWebsiteRequest request0(bucket_);
    request0.setIndexDocument("index.html");
    client->SetBucketWebsite(request0);
    waitTimeinSec(15);

    outcome = client->GetBucketWebsite(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, after set index.html, IndexDocument: " << outcome.result().IndexDocument() <<
            " ,ErrorDocument: " << outcome.result().ErrorDocument() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    request0.setIndexDocument("index1.html");
    request0.setErrorDocument("error1.html");
    client->SetBucketWebsite(request0);
    waitTimeinSec(15);

    outcome = client->GetBucketWebsite(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, after set index1.html, error1.html,IndexDocument: " << outcome.result().IndexDocument() <<
            " ,ErrorDocument: " << outcome.result().ErrorDocument() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::GetBucketReferer()
{
    GetBucketRefererRequest request(bucket_);
    auto outcome = client->GetBucketReferer(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, deffault AllowEmptyReferer: " << outcome.result().AllowEmptyReferer() <<
            " ,Referer size: " << outcome.result().RefererList().size() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    SetBucketRefererRequest request0(bucket_);
    request0.addReferer("http://www.referersample.com");
    request0.addReferer("https://www.referersample.com");
    request0.addReferer("https://www.?.referersample.com");
    request0.addReferer("https://www.*.cn");
    client->SetBucketReferer(request0);
    waitTimeinSec(15);

    outcome = client->GetBucketReferer(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, after set 4 refer, AllowEmptyReferer: " << outcome.result().AllowEmptyReferer() <<
            " ,Referer size: " << outcome.result().RefererList().size() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }


    request0.clearRefererList();
    request0.addReferer("https://www.?.referersample.com");
    request0.addReferer("https://www.*.cn");
    client->SetBucketReferer(request0);
    waitTimeinSec(15);

    outcome = client->GetBucketReferer(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, after set 2 refer, AllowEmptyReferer: " << outcome.result().AllowEmptyReferer() <<
            " ,Referer size: " << outcome.result().RefererList().size() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::GetBucketLifecycle()
{
    auto outcome = client->GetBucketLifecycle(bucket_);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, rule size:%d, rules:" << std::endl;
        for (auto const rule : outcome.result().LifecycleRules()) {
            std::cout << "rule:" << rule.ID() << "," << rule.Prefix() << "," << rule.Status() << ","
                "hasExpiration:" << rule.hasExpiration() << "," <<
                "hasTransitionList:" << rule.hasTransitionList() << "," <<
                "hasAbortMultipartUpload:" << rule.hasAbortMultipartUpload() << std::endl;
        }
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::GetBucketStat()
{
    GetBucketStatRequest request(bucket_);
    auto outcome = client->GetBucketStat(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, storage: " << outcome.result().Storage() <<
            " ,ObjectCount: " << outcome.result().ObjectCount() << 
            " ,MultipartUploadCount:" << outcome.result().MultipartUploadCount() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::GetBucketCors()
{
    auto outcome = client->GetBucketCors(bucket_);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success" << std::endl;
        for (auto const rule : outcome.result().CORSRules()) {
            std::cout << "Get Bucket Cors List:" << std::endl;
            for (auto const origin : rule.AllowedOrigins()) {
                std::cout << "Allowed origin:" << origin << std::endl;
            }
        }
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void BucketSample::CleanAndDeleteBucket(const std::string &bucket)
{
    if (!client->DoesBucketExist(bucket))
        return;

    //abort in progress multipart uploading
    auto listOutcome = client->ListMultipartUploads(ListMultipartUploadsRequest(bucket));
    if (listOutcome.isSuccess()) {
        for (auto const &upload : listOutcome.result().MultipartUploadList())
        {
            client->AbortMultipartUpload(AbortMultipartUploadRequest(bucket, upload.Key, upload.UploadId));
        }
    }

    //List And Delete Object
    ListObjectsRequest request(bucket);
    bool IsTruncated = false;
    do {
        auto outcome = client->ListObjects(request);
        if (outcome.isSuccess()) {
            for (auto const &obj : outcome.result().ObjectSummarys()) {
                auto dOutcome = client->DeleteObject(DeleteObjectRequest(bucket, obj.Key()));
                std::cout << __FUNCTION__ << "Delete Object:" << obj.Key() << 
                    ", result:" << dOutcome.isSuccess() << std::endl;
            }
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
            break;
        }
        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
    } while (IsTruncated);

    // Delete the bucket.
    client->DeleteBucket(DeleteBucketRequest(bucket));
    std::cout << __FUNCTION__ << " done" << std::endl;
}

void BucketSample::DeleteBucketsByPrefix()
{
    std::string prefix = "cpp-sdk-";
    ListBucketsRequest request;
    request.setPrefix(prefix);
    auto outcome = client->ListBuckets(request);
    if (!outcome.isSuccess()) {
        PrintError(__FUNCTION__, outcome.error());
        return;
    }

    for (auto const &bucket : outcome.result().Buckets()) 
    {
        CleanAndDeleteBucket(bucket.Name());
    }

    std::cout << __FUNCTION__ << " done, and total is " << outcome.result().Buckets().size() << std::endl;
}

void BucketSample::DoesBucketExist()
{
    auto outcome = client->DoesBucketExist(bucket_);
    std::cout << __FUNCTION__ << " success, Bucket exist ? " << outcome << std::endl;
}