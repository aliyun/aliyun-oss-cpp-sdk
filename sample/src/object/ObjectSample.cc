#include <iostream>
#include "../Config.h"
#include "ObjectSample.h"
#include <alibabacloud/oss/Const.h>
#include <memory>
#include <sstream>
#include <fstream>
#include <algorithm>


static void waitTimeinSec(int time)
{
    std::this_thread::sleep_for(std::chrono::seconds(time));
}

static int64_t getFileSize(const std::string& file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

using namespace AlibabaCloud::OSS;

ObjectSample::ObjectSample(const std::string &bucket) :
    bucket_(bucket)
{
    ClientConfiguration conf;
    client = new OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    CreateBucketRequest request(bucket_);
    client->CreateBucket(request);
}

ObjectSample::~ObjectSample() {
    delete client;
}

void ObjectSample::PrintError(const std::string &funcName, const OssError &error)
{
    std::cout << funcName << " fail" <<
        ",code:" << error.Code() <<
        ",message:" << error.Message() <<
        ",request_id:" << error.RequestId() << std::endl;
}

void ObjectSample::DoesObjectExist()
{
    auto outcome = client->DoesObjectExist(bucket_, "DoesObjectExist");
    std::cout << __FUNCTION__ << " success, Object exist ? " << outcome << std::endl;
}

void ObjectSample::PutFolder()
{
    const std::string key("yourfolder/");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    auto outcome = client->PutObject(bucket_, key, content);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, dir : " << key << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ObjectSample::PutObjectFromBuffer()
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

void ObjectSample::PutObjectFromFile()
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(__FILE__, std::ios::in);
    PutObjectRequest request(bucket_, "PutObjectFromFile", content);
    auto outcome = client->PutObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, ETag:" << outcome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ObjectSample::GetObjectToBuffer()
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

void ObjectSample::GetObjectToFile()
{
    GetObjectRequest request(bucket_, "PutObjectFromFile");
    request.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>("GetObjectToFile.txt", std::ios_base::out | std::ios_base::in | std::ios_base::trunc| std::ios_base::binary); });
    auto outcome = client->GetObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success" << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ObjectSample::DeleteObject()
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    PutObjectRequest request(bucket_, "DeleteObject", content);
    auto outcome = client->PutObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Put Object ETag:" << outcome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    GetObjectRequest getRequest(bucket_, "DeleteObject");
    auto getOutcome = client->GetObject(getRequest);
    if (getOutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Get Object ETag:" << getOutcome.result().Metadata().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, getOutcome.error());
    }

    DeleteObjectRequest delRequest(bucket_, "DeleteObject");
    auto delOutcome = client->DeleteObject(delRequest);
    if (delOutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, Del Object" << std::endl;
    }
    else {
        PrintError(__FUNCTION__, delOutcome.error());
    }

    getOutcome = client->GetObject(getRequest);
    if (getOutcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, ReGet Object ETag:" << getOutcome.result().Metadata().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, getOutcome.error());
    }
}

void ObjectSample::DeleteObjects()
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << __FUNCTION__;
    PutObjectRequest request(bucket_, "", content);
    for (int i = 0; i < 10; i++) {
        std::string key("DeleteObjects-");
        key.append(std::to_string(i)).append(".txt");
        request.setKey(key);
        auto outcome = client->PutObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << " success, Gen Object:" << key << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

    DeleteObjectsRequest delRequest(bucket_);
    for (int i = 0; i < 5; i++) {
        std::string key("DeleteObjects-");
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    auto outcome = client->DeleteObjects(delRequest);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, no quiet mode, Deleted Object count:" << outcome.result().keyList().size() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    delRequest.clearKeyList();
    delRequest.setQuiet(true);
    for (int i = 5; i < 10; i++) {
        std::string key("DeleteObjects-");
        key.append(std::to_string(i)).append(".txt");
        delRequest.addKey(key);
    }
    outcome = client->DeleteObjects(delRequest);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, quiet mode" << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ObjectSample::HeadObject()
{
    // uploads the file
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "Thank you for using Aliyun Object Storage Service!";
    client->PutObject(bucket_, "HeadObject", content);

    auto outcome = client->HeadObject(bucket_, "HeadObject");
    if (outcome.isSuccess()) {
        auto headMeta = outcome.result();
        std::cout << __FUNCTION__ << " success, ContentType:" 
            << headMeta.ContentType() << "; ContentLength:" << headMeta.ContentLength() 
            << "; CacheControl:" << headMeta.CacheControl() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    // delete the object
    client->DeleteObject(bucket_, "HeadObject");
}

void ObjectSample::GetObjectMeta()
{
    auto meta = ObjectMetaData();
    // sets the content type.
    meta.setContentType("text/plain");
    // sets the cache control
    meta.setCacheControl("max-age=3");
    // sets the custom metadata.
    meta.UserMetaData()["meta"] = "meta-value";

    // uploads the file
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "Thank you for using Aliyun Object Storage Service!";
    client->PutObject(bucket_, "GetObjectMeta", content);
    
    // get the object meta information
    auto outcome = client->GetObjectMeta(bucket_, "GetObjectMeta");
    if (outcome.isSuccess()) {
        auto metadata = outcome.result();
        std::cout << __FUNCTION__ << " success, ETag:" << metadata.ETag() << "; LastModified:" 
            << metadata.LastModified() << "; Size:" << metadata.ContentLength() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    // delete the object
    client->DeleteObject(bucket_, "GetObjectMeta");
}

void ObjectSample::AppendObject()
{
    auto meta = ObjectMetaData();
    meta.setContentType("text/plain");
    std::string key("AppendObject");
    
    // Append an object from buffer, keep in mind that position should be set to zero at first time.
    std::shared_ptr<std::iostream> content1 = std::make_shared<std::stringstream>();
    *content1 << __FUNCTION__;
    AppendObjectRequest request(bucket_, key, content1, meta);
    request.setPosition(0L);
    auto result = client->AppendObject(request);
    
    // Continue to append the object from file descriptor at last position
    std::shared_ptr<std::iostream> content2 = std::make_shared<std::stringstream>();
    *content2 << __FUNCTION__;
    auto position = result.result().Length();
    AppendObjectRequest appendObjectRequest(bucket_, key, content2);
    appendObjectRequest.setPosition(position);
    auto outcome = client->AppendObject(appendObjectRequest);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, CRC64:" << outcome.result().CRC64() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }

    // View object content
    auto object = client->GetObject(bucket_, key);
    if (object.isSuccess()) {
        char ch[100];
        *(object.result().Content()) >> ch;
        std::string str = ch;
        std::cout << "AppendObject Content:" << str << std::endl;
    }
    // Delete the appendable object
    client->DeleteObject(bucket_, key);
}

static void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData)
{
    std::cout << "ProgressCallback[" << userData << "] => " <<
                 increment <<" ," << transfered << "," << total << std::endl;
}

void ObjectSample::PutObjectProgress()
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(__FILE__, std::ios::in|std::ios::binary);
    PutObjectRequest request(bucket_, "PutObjectProgress", content);
    TransferProgress progressCallback = { ProgressCallback , this };
    request.setTransferProgress(progressCallback);
    auto outcome = client->PutObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ <<"[" << this << "]" << " success, ETag:" << outcome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ObjectSample::UploadObjectProgress() 
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

void ObjectSample::MultiCopyObjectProcess() 
{
    //case 1: checkpoint dir is not enabled
    {
        MultiCopyObjectRequest request(bucket_, "MultiCopyObjectProcess", bucket_, "MultiCopyObjectProcess_Src");

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableCopyObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

    //case 2: checkpoint dir is config
    {
        MultiCopyObjectRequest request(bucket_, "MultiCopyObjectProcess", bucket_, "MultiCopyObjectProcess_Src", Config::CheckpointDir);

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableCopyObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

    //case 3: checkpoint dir is config, multi threads is config
    {
        MultiCopyObjectRequest request(bucket_, "MultiCopyObjectProcess", bucket_, "MultiCopyObjectProcess_Src", Config::CheckpointDir, DefaultPartSize, 4);

        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcome = client->ResumableCopyObject(request);
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().ETag() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }
}

void ObjectSample::DownloadObjectProcess() 
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

void ObjectSample::GetObjectProgress()
{
    GetObjectRequest request(bucket_, "PutObjectProgress");
    TransferProgress progressCallback = { ProgressCallback , this };
    request.setTransferProgress(progressCallback);
    request.setRange(0, 99);
    auto outcome = client->GetObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().RequestId() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ObjectSample::PutObjectCallable()
{
    std::vector<PutObjectOutcomeCallable> outcomes;

    for (int i = 0; i < 5; i++) {
        std::string key = "PutObjectCallable_";
        key.append(std::to_string(i));
        //std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(__FILE__, std::ios::in | std::ios::binary);
        std::shared_ptr<std::stringstream> content = std::make_shared<std::stringstream>();
        *content << __FUNCTION__ << __FILE__ << std::endl;
        PutObjectRequest request(bucket_, key, content);
        TransferProgress progressCallback = { ProgressCallback , this };
        request.setTransferProgress(progressCallback);
        auto outcomeCallable = client->PutObjectCallable(request);
        outcomes.emplace_back(std::move(outcomeCallable));
    }

    std::cout << __FUNCTION__ << "[" << this << "]" << " start put object" << std::endl;

    waitTimeinSec(10);


    for (size_t i = 0; i < outcomes.size(); i++) {
        auto outcome = outcomes[i].get();
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, ETag:" << outcome.result().ETag().c_str() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }

}

void ObjectSample::GetObjectCallable()
{
    std::vector<GetObjectOutcomeCallable> outcomes;
    std::shared_ptr<std::stringstream> content = std::make_shared<std::stringstream>();
    *content << __FUNCTION__ << __FILE__ << __FILE__ << __FILE__ << __FILE__ << __FILE__ << std::endl;
    client->PutObject(PutObjectRequest(bucket_, "GetObjectCallable", content));

    GetObjectRequest request(bucket_, "GetObjectCallable");
    TransferProgress progressCallback = { ProgressCallback , this };
    request.setTransferProgress(progressCallback);
    //request.setRange(0, 29);

    for (int i = 0; i < 5; i++) {
        auto outcomeCallable = client->GetObjectCallable(request);
        outcomes.emplace_back(std::move(outcomeCallable));
    }

    std::cout << __FUNCTION__ << "[" << this << "]" << " start get object" << std::endl;
    waitTimeinSec(5);

    for (size_t i = 0; i < outcomes.size(); i++) {
        auto outcome = outcomes[i].get();
        if (outcome.isSuccess()) {
            std::cout << __FUNCTION__ << "[" << this << "]" << " success, RequestId:" << outcome.result().RequestId().c_str() << std::endl;
        }
        else {
            PrintError(__FUNCTION__, outcome.error());
        }
    }
}

void ObjectSample::MultipartUploadObject()
{
    std::string key = "MultipartUploadObject";
    InitiateMultipartUploadRequest initUploadRequest(bucket_, key);
    auto uploadIdResult = client->InitiateMultipartUpload(initUploadRequest);
    auto uploadId = uploadIdResult.result().UploadId();
    std::string fileToUpload = Config::BigFileToUpload;
    int64_t partSize = 100 * 1024;
    PartList partETagList;

    auto fileSize = getFileSize(fileToUpload);
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
        auto uploadPartOutcome = client->UploadPart(uploadPartRequest);
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
    auto outcome = client->CompleteMultipartUpload(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, UploadID : " << uploadId << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}

void ObjectSample::ResumableObject()
{
    
}

void ObjectSample::RestoreArchiveObject(const std::string bucket, const std::string key, int maxWaitTimeInSeconds = 600)
{
    auto result = client->RestoreObject(bucket, key);
    if (!result.isSuccess()) {
        // not the archive object
        PrintError(__FUNCTION__, result.error());
        return ;
    }

    std::string onGoingRestore("ongoing-request=\"false\"");
    while (maxWaitTimeInSeconds > 0) {
        auto meta = client->HeadObject(bucket, key);

        std::string	restoreStatus = meta.result().HttpMetaData()["x-oss-restore"];
        std::transform(restoreStatus.begin(), restoreStatus.end(), restoreStatus.begin(), ::tolower);
        if (!restoreStatus.empty() && 
            restoreStatus.compare(0, onGoingRestore.size(), onGoingRestore)==0) {
            std::cout << __FUNCTION__ << " success, restore status:" << restoreStatus << std::endl;
            // success to restore archive object
            break;
        }

        std::cout << __FUNCTION__ << " info, WaitTime:" << maxWaitTimeInSeconds
            << "; restore status:" << restoreStatus << std::endl;

        waitTimeinSec(10);
        maxWaitTimeInSeconds--;
	}

    if (maxWaitTimeInSeconds == 0) {
        std::cout << __FUNCTION__ << " fail, TimeoutException" << std::endl;
    }
}

void ObjectSample::CopyObject()
{
    // put a object as source object key
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << __FUNCTION__;
    PutObjectRequest putObjectRequest(bucket_, "CopyObjectSourceKey", content);
    client->PutObject(putObjectRequest);

    CopyObjectRequest request(bucket_, "CopyObjectTargetKey");
    request.setCopySource(bucket_, "CopyObjectSourceKey");
    auto outcome = client->CopyObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, ETag:" << outcome.result().ETag() << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}


void ObjectSample::PutObjectCallback()
{
    std::string callbackUrl = Config::CallbackServer;
    std::string callbackBody = "bucket=${bucket}&object=${object}&etag=${etag}&size=${size}&mimeType=${mimeType}&my_var1=${x:var1}";

    ObjectCallbackBuilder builder(callbackUrl, callbackBody, "", ObjectCallbackBuilder::Type::URL);
    std::string value = builder.build();

    ObjectCallbackVariableBuilder varBuilder;
    varBuilder.addCallbackVariable("x:var1", "value1");
    varBuilder.addCallbackVariable("x:var2", "value2");
    std::string varValue = varBuilder.build();

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << __FUNCTION__;
    PutObjectRequest request(bucket_, "PutObjectCallback", content);
    request.MetaData().addHeader("x-oss-callback", value);
    request.MetaData().addHeader("x-oss-callback-var", varValue);
    auto outcome = client->PutObject(request);
    if (outcome.isSuccess()) {
        std::cout << __FUNCTION__ << " success, ETag:" << outcome.result().ETag();
        if (outcome.result().Content() != nullptr) {
            std::istreambuf_iterator<char> isb(*outcome.result().Content().get()), end;
            std::cout << ", callback data:" << std::string(isb, end);
        }
        std::cout << std::endl;
    }
    else {
        PrintError(__FUNCTION__, outcome.error());
    }
}
