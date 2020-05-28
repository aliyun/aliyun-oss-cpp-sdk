#include <alibabacloud/oss/OssClient.h>
#include <iostream>
#include "Config.h"

#if !defined(OSS_DISABLE_BUCKET)
#include "service/ServiceSample.h"
#include "bucket/BucketSample.h"
#endif

#include "object/ObjectSample.h"
#include "presignedurl/PresignedUrlSample.h"

#if !defined(OSS_DISABLE_LIVECHANNEL)
#include "LiveChannel/LiveChannelSample.h"
#endif

#if !defined(OSS_DISABLE_ENCRYPTION)
#include "encryption/EncryptionSample.h"
#endif

using namespace AlibabaCloud::OSS;

void LogCallbackFunc(LogLevel level, const std::string &stream)
{
    if (level == LogLevel::LogOff)
        return;

    std::cout << stream;
}

int main(void)
{
    std::cout << "oss-cpp-sdk samples" << std::endl;
    std::string bucketName = "<YourBucketName>";

    InitializeSdk();

    SetLogLevel(LogLevel::LogDebug);
    SetLogCallback(LogCallbackFunc);

#if !defined(OSS_DISABLE_BUCKET)
    ServiceSample serviceSample;
    serviceSample.ListBuckets();
    serviceSample.ListBucketsWithMarker();
    serviceSample.ListBucketsWithPrefix();

    BucketSample bucketSample(bucketName);
    bucketSample.InvalidBucketName();
    bucketSample.CreateAndDeleteBucket();
    bucketSample.SetBucketAcl();
    bucketSample.SetBucketLogging();
    bucketSample.SetBucketWebsite();
    bucketSample.SetBucketReferer();
    bucketSample.SetBucketLifecycle();
    bucketSample.SetBucketCors();
    bucketSample.GetBucketCors();

    bucketSample.DeleteBucketLogging();
    bucketSample.DeleteBucketWebsite();
    bucketSample.DeleteBucketLifecycle();
    bucketSample.DeleteBucketCors();

    bucketSample.GetBucketAcl();
    bucketSample.GetBucketLocation();
    bucketSample.GetBucketLogging();
    bucketSample.GetBucketWebsite();
    bucketSample.GetBucketReferer();
    bucketSample.GetBucketStat();
    bucketSample.GetBucketLifecycle();
    //bucketSample.DeleteBucketsByPrefix();
#endif

    ObjectSample objectSample(bucketName);
    objectSample.PutObjectFromBuffer();
    objectSample.PutObjectFromFile();
    objectSample.GetObjectToBuffer();
    objectSample.GetObjectToFile();
    objectSample.DeleteObject();
    objectSample.DeleteObjects();
    objectSample.HeadObject();
    objectSample.GetObjectMeta();
    objectSample.AppendObject();
    objectSample.PutObjectProgress();
    objectSample.GetObjectProgress();
    objectSample.PutObjectCallable();
    objectSample.GetObjectCallable();
    objectSample.CopyObject();
    //objectSample.RestoreArchiveObject("your-archive", "oss_archive_object.PNG", 1);

    objectSample.ListObjects();
    objectSample.ListObjectWithMarker();
    objectSample.ListObjectWithEncodeType();

#if !defined(OSS_DISABLE_RESUAMABLE)
    objectSample.UploadObjectProgress();
    objectSample.MultiCopyObjectProcess();
    objectSample.DownloadObjectProcess();
#endif

    PresignedUrlSample signedUrlSample(bucketName);
    signedUrlSample.GenGetPresignedUrl();
    signedUrlSample.PutObjectByUrlFromBuffer();
    signedUrlSample.PutObjectByUrlFromFile();
    signedUrlSample.GetObjectByUrlToBuffer();
    signedUrlSample.GetObjectByUrlToFile();


#if !defined(OSS_DISABLE_LIVECHANNEL)
    // LiveChannel
    LiveChannelSample liveChannelSample(bucketName, "test_channel");
    liveChannelSample.PutLiveChannel();
    liveChannelSample.GetLiveChannelInfo();
    liveChannelSample.GetLiveChannelStat();
    liveChannelSample.ListLiveChannel();
    liveChannelSample.GetLiveChannelHistory();
    liveChannelSample.PostVodPlayList();
    liveChannelSample.GetVodPlayList();
    liveChannelSample.PutLiveChannelStatus();
    liveChannelSample.DeleteLiveChannel();
#endif

#if !defined(OSS_DISABLE_ENCRYPTION)
    // Encryption
    EncryptionSample encryptionSample(bucketName);
    encryptionSample.PutObjectFromBuffer();
    encryptionSample.PutObjectFromFile();
    encryptionSample.GetObjectToBuffer();
    encryptionSample.GetObjectToFile();
#if !defined(DISABLE_RESUAMABLE)
    encryptionSample.UploadObjectProgress();
    encryptionSample.DownloadObjectProcess();
    encryptionSample.MultipartUploadObject();
#endif
#endif

    ShutdownSdk();
    return 0;
}
