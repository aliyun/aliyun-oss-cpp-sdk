#include <iostream>
#include <alibabacloud/oss/OssClient.h>

#define MAX_LOOP_TIMES 60

class LiveChannelSample
{
public:
    LiveChannelSample(const std::string& bucket, const std::string& channeName);
    ~LiveChannelSample();

    int PutLiveChannel();
    int GetLiveChannelInfo();
    int GetLiveChannelStat();
    int ListLiveChannel();
    int GetLiveChannelHistory();
    int PostVodPlayList();
    int GetVodPlayList();
    int PutLiveChannelStatus();
    int DeleteLiveChannel();
private:
    AlibabaCloud::OSS::OssClient *client;
    std::string bucket_;
    std::string channelName_;
    std::string rtmpUrl_;
    std::string signedRTMPUrl_;
    time_t timeStart_;
    time_t timeEnd_;
    bool isSuccess_;
};
