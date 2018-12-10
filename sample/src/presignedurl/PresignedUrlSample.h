#include <alibabacloud/oss/OssClient.h>

class PresignedUrlSample
{
public:
    PresignedUrlSample(const std::string &bucket);
    ~PresignedUrlSample();

    void GenPutPresignedUrl();
    void GenGetPresignedUrl();
    void PutObjectByUrlFromBuffer();
    void PutObjectByUrlFromFile();
    void GetObjectByUrlToBuffer();
    void GetObjectByUrlToFile();


private:
    void PrintError(const std::string &funcName, const AlibabaCloud::OSS::OssError &error);
    AlibabaCloud::OSS::OssClient *client;
    std::string bucket_;
    std::string key_;
};
