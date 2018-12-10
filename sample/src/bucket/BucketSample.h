#include <alibabacloud/oss/OssClient.h>

class BucketSample
{
public:
    BucketSample(const std::string &bucket);
    ~BucketSample();

    void DoesBucketExist();

    void InvalidBucketName();
    void CreateAndDeleteBucket();
    void SetBucketAcl();
    void SetBucketLogging();
    void SetBucketWebsite();
    void SetBucketReferer();
    void SetBucketLifecycle();
    void SetBucketCors();

    void DeleteBucketLogging();
    void DeleteBucketWebsite();
    void DeleteBucketLifecycle();
    void DeleteBucketCors();

    void ListObjects();
    void ListObjectWithMarker();
    void ListObjectWithEncodeType();

    void GetBucketAcl();
    void GetBucketLocation();
    void GetBucketLogging();
    void GetBucketWebsite();
    void GetBucketReferer();
    void GetBucketLifecycle();
    void GetBucketStat();
    void GetBucketCors();

    void CleanAndDeleteBucket(const std::string &bucket);
    void DeleteBucketsByPrefix();

private:
    void PrintError(const std::string &funcName, const AlibabaCloud::OSS::OssError &error);
    AlibabaCloud::OSS::OssClient *client;
    std::string bucket_;
};
