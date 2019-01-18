#include <iostream>
#include <time.h>
#include "../Config.h"
#include "LiveChannelSample.h"

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

LiveChannelSample::LiveChannelSample(const std::string& bucket, const std::string& channelName):bucket_(bucket),channelName_(channelName),
    timeStart_(0),timeEnd_(0),isSuccess_(false)
{
    
    ClientConfiguration conf;
    client = new OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    CreateBucketRequest request(bucket_);
    client->CreateBucket(request);
}

LiveChannelSample::~LiveChannelSample()
{
    DeleteBucketRequest request(bucket_);
    auto deleteOutcome = client->DeleteBucket(request);

    if(!deleteOutcome.isSuccess())
    {
        std::cerr<<"delete bucket "+bucket_+" fail";
    }

    if(nullptr != client)
    {
        delete client;
        client = nullptr;
    }
}

int LiveChannelSample::PutLiveChannel()
{
    PutLiveChannelRequest request(bucket_, channelName_, "HLS");
    auto putOutcome = client->PutLiveChannel(request);
    if(putOutcome.isSuccess())
    {
        isSuccess_ = true;
        rtmpUrl_ = putOutcome.result().PublishUrl();
        std::cout << "rtmp publish url is "+rtmpUrl_ << std::endl;
        std::cout << "rtmp play url is " + putOutcome.result().PlayUrl() << std::endl;
        return 0;
    }
    return -1;
}

int LiveChannelSample::GetLiveChannelInfo()
{
    isSuccess_ = false;
    GetLiveChannelInfoRequest request(bucket_, channelName_);
    auto getOutcome= client->GetLiveChannelInfo(request);
    if(getOutcome.isSuccess())
    {
        isSuccess_ = true;
        const GetLiveChannelInfoResult& result = getOutcome.result();
        std::cout << "LiveChannel Info :" << std::endl;
        std::cout << "Description: " << result.Description() << std::endl;
        std::cout << "Status: " << result.Status() << std::endl;
        std::cout << "ChannelType: " << result.Type() << std::endl;
        std::cout << "FragDuration: " << result.FragDuration() << std::endl;
        std::cout << "FragCount: " << result.FragCount() << std::endl;
        std::cout << "PlaylistName: " << result.PlaylistName() << std::endl;
        return 0;
    }
    return -1;
}

int LiveChannelSample::GetLiveChannelStat()
{
    isSuccess_ = false;
    GetLiveChannelStatRequest request(bucket_, channelName_);
    auto getOutcome = client->GetLiveChannelStat(request);
    if(getOutcome.isSuccess())
    {
        isSuccess_ = true;
        const GetLiveChannelStatResult& result = getOutcome.result();
        // before push rtmp stream, the live channel is in Idle status
        std::cout << "LiveChannelStatus: " << result.Status() << std::endl;
        return 0;
    }
    return -1;
}

int LiveChannelSample::ListLiveChannel()
{
    isSuccess_ = false;
    ListLiveChannelRequest request(bucket_);
    auto listOutcome = client->ListLiveChannel(request);
    if(listOutcome.isSuccess())
    {
        isSuccess_ = true;
        const ListLiveChannelResult& result = listOutcome.result();
        std::cout << "ListLiveChannelResult: " << std::endl;
        std::cout << "Prefix: " << result.Prefix() << std::endl;
        std::cout << "Marker: " << result.Marker() << std::endl;
        std::cout << "NextMarker: " << result.NextMarker() << std::endl;
        std::cout << "IsTruncated: " << result.IsTruncated() << std::endl;
        std::cout << "MaxKeys: " << result.MaxKeys() << std::endl;
        std::vector<LiveChannelInfo> vec = result.LiveChannelList();
        for(std::size_t i=0; i<vec.size(); i++)
        {
            std::cout << "   LiveChannelInfo: " << std::endl;
            std::cout << "    Name: " << vec[i].name << std::endl;
            std::cout << "    description: " << vec[i].description << std::endl;
            std::cout << "    status: " << vec[i].status << std::endl;
            std::cout << "    lastModified: " << vec[i].lastModified << std::endl;
            std::cout << "    publishUrl: " << vec[i].publishUrl << std::endl;
            std::cout << "    playUrl: " << vec[i].playUrl << std::endl;
        }
        return 0;
    }
    return -1;
}

int LiveChannelSample::GetLiveChannelHistory()
{
    isSuccess_ = false;
    int i = 0;
    GetLiveChannelHistoryResult result;
    
    timeStart_ = time(NULL);
    timeEnd_ = timeStart_ + 3600;
    std::cout << "timeStart: " << timeStart_ << " End: " << timeEnd_ << std::endl;
    time_t expireTime = timeEnd_;
    GenerateRTMPSignedUrlRequest request(bucket_, channelName_, "test_play_list.m3u8", expireTime);
    auto generateOutcome = client->GenerateRTMPSignedUrl(request);
    if(!generateOutcome.isSuccess())
    {
        std::cout << "GenerateRTMPSignedUrl fail" <<std::endl;
        return -1;
    }else{
        signedRTMPUrl_ = generateOutcome.result();
        std::cout << "GenerateRTMPSignedUrl success URL: " << signedRTMPUrl_ << std::endl;
    }

    // use ffmpeg or some other tools to push rtmp stream asynchronizely to signed url
    // we will wait 15 minutes at most

    while(1)
    {
        if(i>MAX_LOOP_TIMES)
            break;
        GetLiveChannelHistoryRequest request(bucket_, channelName_);
        auto getOutcome = client->GetLiveChannelHistory(request);
        if(getOutcome.isSuccess())
        {
            std::vector<LiveRecord> vec = getOutcome.result().LiveRecordList();
            if(vec.size() > 0)
            {
                isSuccess_ = true;
                result = getOutcome.result();
                std::cout << "LiveRecordList size " << vec.size() << std::endl;
                for(size_t i=0; i<vec.size(); i++)
                {
                    std::cout << "    LiveRecord: " << std::endl;
                    std::cout << "       startTime: " << vec[i].startTime << std::endl;
                    std::cout << "       endTime: " << vec[i].endTime << std::endl;
                    std::cout << "       remoteAddr: " << vec[i].remoteAddr << std::endl;
                }
                waitTimeinSec(30);
                break;
            }else{
                std::cout << "warning: LiveRecordList size is 0" << std::endl;
                waitTimeinSec(15);
            }
        }
        i++;
    }
    return 0;
}

int LiveChannelSample::PostVodPlayList()
{
    isSuccess_ = false;
    uint64_t startTime = time(NULL) - 3600;
    uint64_t endTime = time(NULL) + 3600;
    PostVodPlaylistRequest request(bucket_, channelName_, "test_play_list.m3u8", startTime, endTime);
    auto postOutcome = client->PostVodPlaylist(request);
    if(postOutcome.isSuccess())
    {
        isSuccess_ = true;
        std::cout << "PostVodPlayList Success " << std::endl;
    }
    return 0;
}

int LiveChannelSample::GetVodPlayList()
{
    isSuccess_ = false;
    uint64_t startTime = time(NULL) - 3600;
    uint64_t endTime = time(NULL) + 3600;
    GetVodPlaylistRequest request(bucket_, channelName_, startTime, endTime);
    auto getOutcome = client->GetVodPlaylist(request);
    
    if(getOutcome.isSuccess())
    {
        isSuccess_ = true;
        std::cout << "GetVodPlayList Success " << std::endl;
        std::cout << "GetVodPlaylist result " << getOutcome.result().PlaylistContent() << std::endl;
    }
    return 0;
}
int LiveChannelSample::PutLiveChannelStatus()
{
    isSuccess_ = false;
    PutLiveChannelStatusRequest request(bucket_, channelName_, LiveChannelStatus::DisabledStatus);
    auto putOutcome = client->PutLiveChannelStatus(request);
    if(putOutcome.isSuccess())
    {
        isSuccess_ = true;
        std::cout << "PutLiveChannelStatus Success " << std::endl;
    }
    return 0;
}

int LiveChannelSample::DeleteLiveChannel()
{
    isSuccess_ = false;
    DeleteLiveChannelRequest request(bucket_, channelName_);
    auto deleteOutcome = client->DeleteLiveChannel(request);
    if(deleteOutcome.isSuccess())
    {
        isSuccess_ = true;
        std::cout << "DeleteLiveChannelSuccess " << std::endl;
    }
    return 0;
}
