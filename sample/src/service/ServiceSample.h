#include <alibabacloud/oss/OssClient.h>

class ServiceSample
{
public:
    ServiceSample();
    ~ServiceSample();
    void ListBuckets();
    void ListBucketsWithMarker();
    void ListBucketsWithPrefix();
private:
    void PrintError(const std::string &funcName, const AlibabaCloud::OSS::OssError &error);
    AlibabaCloud::OSS::OssClient *client;
};
