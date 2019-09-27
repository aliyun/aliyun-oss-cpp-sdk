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
    void UploadObjectProgress();
    void DownloadObjectProcess();
    void MultipartUploadObject();

private:
    void PrintError(const std::string &funcName, const AlibabaCloud::OSS::OssError &error);
    AlibabaCloud::OSS::OssEncryptionClient *client;
    std::string bucket_;
};
