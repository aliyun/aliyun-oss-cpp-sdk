#include <alibabacloud/oss/OssEncryptionClient.h>

class EncryptionSample
{
public:
    EncryptionSample(const std::string &bucket);
    ~EncryptionSample();
 
    void PutObjectFromBuffer();
    void PutObjectFromFile();
    void GetObjectToBuffer();
    void GetObjectToFile();
    void MultipartUploadObject();

#if !defined(OSS_DISABLE_RESUAMABLE)
    void UploadObjectProgress();
    void DownloadObjectProcess();
#endif

private:
    void PrintError(const std::string &funcName, const AlibabaCloud::OSS::OssError &error);
    AlibabaCloud::OSS::OssEncryptionClient *client;
    std::string bucket_;
};
