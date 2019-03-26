#include <alibabacloud/oss/OssClient.h>
#include <alibabacloud/oss/client/RateLimiter.h>
#include <iostream>
#include <memory>
#include "Config.h"
#include <fstream>
#include <future>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>
#include<algorithm>

using namespace AlibabaCloud::OSS;
using namespace AlibabaCloud::OSS::PTest;

static std::chrono::system_clock::time_point totalStartTimePoint;
static std::chrono::system_clock::time_point totalStopTimePoint;
static int64_t totalTransferSize;
static int64_t totalTransferDurationMS;
static int64_t minTransferDurationMS;
static int64_t maxTransferDurationMS;
static int totalSucessCnt;
static int totalFailCnt;
static std::mutex logMtx;
static std::mutex updateMtx;

static uint64_t uploadFileCRC64;

struct taskResult
{
    int taskId;
    bool success;
    int64_t seqNum;
    int64_t transferSize;
    std::chrono::system_clock::time_point startTimePoint;
    std::chrono::system_clock::time_point stopTimePoint;
};

using TaskResultVector = std::vector<taskResult>;

static std::vector<TaskResultVector> totalResults;

class  DefaultRateLimiter : public RateLimiter
{
public:
    DefaultRateLimiter() :rate_(0) {};
    ~DefaultRateLimiter() {};
    virtual void setRate(int rate) { rate_ = rate; };
    virtual int Rate() const { return rate_; };
private:
    int rate_;
};

static std::string to_datetime_string(const std::chrono::system_clock::time_point tp)
{
    auto tt = std::chrono::system_clock::to_time_t(tp);
    struct tm tm;
    char date[60] = { 0 };
#ifdef _WIN32
    localtime_s(&tm, &tt);
    sprintf_s(date, "%d-%02d-%02d %02d:%02d:%02d",
#else
    localtime_r(&tt, &tm);
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
#endif // _WIN32
        (int)tm.tm_year + 1900, (int)tm.tm_mon + 1, (int)tm.tm_mday,
        (int)tm.tm_hour, (int)tm.tm_min, (int)tm.tm_sec);
    return std::string(date);
}

static int64_t get_file_size(const std::string& file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

static uint64_t get_file_crc64(const std::string& file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    const int buffer_size = 2048;
    char streamBuffer[buffer_size];
    uint64_t initcrc = 0;
    while (f.good())
    {
        f.read(streamBuffer, buffer_size);
        auto bytesRead = f.gcount();
        if (bytesRead > 0)
        {
            initcrc = ComputeCRC64(initcrc, static_cast<void *>(streamBuffer), static_cast<size_t>(bytesRead));
        }
    }
    f.close();
    return initcrc;
}

static int calculate_part_count(int64_t totalSize, int singleSize)
{
    auto partCount = totalSize / singleSize;
    if (totalSize % singleSize != 0)
    {
        partCount++;
    }
    return static_cast<int>(partCount);
}

static bool is_test_done(int loopcnt, std::chrono::system_clock::time_point startTp)
{
    if (Config::Persistent)
        return false;

    if ((Config::LoopTimes > 0) && (loopcnt <= Config::LoopTimes))
        return false;

    if (Config::LoopDurationS > 0) {
        auto currTp = std::chrono::system_clock::now();
        int  diff = (int)(std::chrono::duration_cast<std::chrono::seconds>(currTp - startTp)).count();

        if (diff < Config::LoopDurationS)
            return false;
    }

    return true;
}

static void log_msg(std::ostream & out, const std::string &msg)
{
    std::unique_lock<std::mutex> lck(logMtx);
    out << msg;
    out.flush();
}

static void log_result_detail_msg(std::ostream& out)
{
    std::unique_lock<std::mutex> lck(logMtx);
    std::stringstream ss;
    out << "Dump Detail:" << std::endl;
    out << "Result, TaskId, SeqNum, DurationMs, TransferSize, TransferRate MB/s" << std::endl;
    for (const auto& results : totalResults) {
        for (const auto& result : results) {
            ss.str("");
            ss << std::setiosflags(std::ios::fixed) << std::setprecision(2);
            int64_t durationMs = (std::chrono::duration_cast<std::chrono::milliseconds>(result.stopTimePoint - result.startTimePoint)).count();
            ss << (result.success ? "OK" : "NG") <<
                "," << result.taskId <<
                "," << result.seqNum <<
                "," << durationMs;
            if (result.success) {
                ss << "," << result.transferSize;
                ss << "," << ((double)result.transferSize / 1024.0f / 1024.0f) / ((double)durationMs / 1000.0f);
            }
            else {
                ss << ", ,";
            }
            ss << std::endl;
            out << ss.str();
        }
    }
    out.flush();
}

static void get_90th_95th_latency_percentile(int64_t &per90, int64_t &per95)
{
    std::vector<int64_t> latentcy;
    for (const auto& results : totalResults) {
        for (const auto& result : results) {
            int64_t durationMs = (std::chrono::duration_cast<std::chrono::milliseconds>(result.stopTimePoint - result.startTimePoint)).count();
            latentcy.push_back(durationMs);
        }
    }
    std::sort(latentcy.begin(), latentcy.end());

    per90 = latentcy[latentcy.size() * 90 / 100];
    per95 = latentcy[latentcy.size() * 95 / 100];
}

static std::string get_task_key(int taskId)
{
    std::string key(Config::BaseRemoteKey);
    if (Config::Command == "upload") {
        key.append("-").append(std::to_string(taskId));
    }
    else if (Config::Command == "upload_resumable") {
        key.append("-resumable").append(std::to_string(taskId));
    }
    else if (Config::Command == "upload_async") {
        key.append("-async").append(std::to_string(taskId));
    }
    return key;
}

static std::string get_task_fileName(int taskId)
{
    std::string fileName(Config::BaseLocalFile);
    if (Config::Command == "download") {
        fileName.append("-").append(std::to_string(taskId));
    }
    else if (Config::Command == "download_resumable") {
        fileName.append("-resumable").append(std::to_string(taskId));
    }
    else if (Config::Command == "download_async") {
        fileName.append("-async").append(std::to_string(taskId));
    }
    return fileName;
}

static taskResult upload(const OssClient &client, const std::string &key, const std::string &fileToUpload)
{
    taskResult result;
    std::stringstream ss;
    result.transferSize = get_file_size(fileToUpload);
    result.startTimePoint = std::chrono::system_clock::now();

    auto outcome = client.PutObject(Config::BucketName, key, fileToUpload);

    result.success = outcome.isSuccess();
    result.stopTimePoint = std::chrono::system_clock::now();
    return result;
}

static taskResult upload_async(const OssClient &client, const std::string &key, const std::string &fileToUpload)
{
    taskResult result;
    result.transferSize = get_file_size(fileToUpload);

    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(fileToUpload, std::ios::in|std::ios::binary);
    auto outcomeCallable = client.PutObjectCallable(PutObjectRequest(Config::BucketName, key, content));
    auto outcome = outcomeCallable.get();

    result.startTimePoint = std::chrono::system_clock::now();

    result.success = outcome.isSuccess();
    result.stopTimePoint = std::chrono::system_clock::now();
    return result;
}

static taskResult upload_resumable(const OssClient &client, const std::string &key, const std::string &fileToUpload)
{
    taskResult result;
    result.transferSize = get_file_size(fileToUpload);

    UploadObjectRequest request(Config::BucketName, key, fileToUpload);
    request.setPartSize(Config::PartSize);
    request.setThreadNum(Config::Parallel);

    result.startTimePoint = std::chrono::system_clock::now();

    auto outcome = client.ResumableUploadObject(request);

    result.stopTimePoint = std::chrono::system_clock::now();
    result.success = outcome.isSuccess();

    return result;
}

struct SubTaskInfo {
    int partNum;
    int64_t offset;
};

static taskResult upload_multipart(const OssClient &client, const std::string &key, const std::string &fileToUpload)
{
    taskResult result;
    result.transferSize = get_file_size(fileToUpload);
    result.startTimePoint = std::chrono::system_clock::now();

    // Set the part size 
    const int partSize = Config::PartSize;
    const int64_t fileLength = result.transferSize;
    auto partCount = calculate_part_count(fileLength, partSize);

    std::vector< SubTaskInfo> SubTaskInfos;
    for (auto i = 0; i < partCount; i++) {
        SubTaskInfo subinfo;
        subinfo.partNum = i + 1;
        subinfo.offset  = partSize;
        subinfo.offset *= i;
        SubTaskInfos.push_back(subinfo);
    }

    std::vector<PutObjectOutcomeCallable> outcomes;
    bool failed = false;
    std::string code;
    std::string message;
    InitiateMultipartUploadRequest request(Config::BucketName, key);
    auto initOutcome = client.InitiateMultipartUpload(request);
    if (initOutcome.isSuccess()) {
        while (!SubTaskInfos.empty() && !failed) {
            if (outcomes.size() < static_cast<size_t>(Config::Parallel)) {
                SubTaskInfo subinfo = *SubTaskInfos.begin();
                SubTaskInfos.erase(SubTaskInfos.begin());
                std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(fileToUpload, std::ios::in | std::ios::binary);
                content->seekg(subinfo.offset);
                UploadPartRequest request(Config::BucketName, key, content);
                request.setPartNumber(subinfo.partNum);
                request.setUploadId(initOutcome.result().UploadId());
                request.setContentLength(Config::PartSize);
                auto outcomeCallable = client.UploadPartCallable(request);
                outcomes.emplace_back(std::move(outcomeCallable));
            }
            //check all outcomes 
            for (auto it = outcomes.begin(); !failed && it != outcomes.end();) {
                auto status = it->wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    auto outcome = it->get();
                    if (!outcome.isSuccess()) {
                        failed  = true;
                        code    = outcome.error().Code();
                        message = outcome.error().Message();
                    }
                    it = outcomes.erase(it);
                }
                else {
                    it++;
                }
            }
        }

        for (auto it = outcomes.begin(); !failed && it != outcomes.end(); it++) {
            auto outcome = it->get();
            if (!outcome.isSuccess()) {
                failed  = true;
                code    = outcome.error().Code();
                message = outcome.error().Message();
            }
        }

        if (!failed) {
            auto listOutcome = client.ListParts(ListPartsRequest(Config::BucketName, key, initOutcome.result().UploadId()));
            if (listOutcome.isSuccess()) {
                auto cOutcome = client.CompleteMultipartUpload(
                    CompleteMultipartUploadRequest(Config::BucketName, key, 
                        listOutcome.result().PartList(),
                        initOutcome.result().UploadId()));
                if (!cOutcome.isSuccess()) {
                    failed = true;
                    code = listOutcome.error().Code();
                    message = listOutcome.error().Message();
                }
                else {
                    if (cOutcome.result().CRC64() != uploadFileCRC64) {
                        failed  = true;
                        code    = "CRC64 check fail.";
                        message = "CRC64 check fail.";
                    }
                }
            }
            else {
                failed  = true;
                code    = listOutcome.error().Code();
                message = listOutcome.error().Message();
            }
        }
    }
    else {
        failed = true;
        code = initOutcome.error().Code();
        message = initOutcome.error().Message();
    }

    result.stopTimePoint = std::chrono::system_clock::now();
    result.success = !failed;

    return result;
}

static taskResult download(const OssClient &client, const std::string &key, const std::string &fileToSave)
{
    taskResult result;
    result.startTimePoint = std::chrono::system_clock::now();

    auto outcome = client.GetObject(Config::BucketName, key, fileToSave);

    result.stopTimePoint = std::chrono::system_clock::now();
    result.success = outcome.isSuccess();
    if (outcome.isSuccess()) {
        result.transferSize = outcome.result().Metadata().ContentLength();
    }
    return result;
}

static taskResult download_async(const OssClient &client, const std::string &key, const std::string &fileToSave)
{
    taskResult result;

    GetObjectRequest request(Config::BucketName, key);
    request.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>(fileToSave, std::ios_base::out | std::ios_base::trunc | std::ios::binary); });
    result.startTimePoint = std::chrono::system_clock::now();

    auto outcomeCallable = client.GetObjectCallable(request);
    auto outcome = outcomeCallable.get();

    result.stopTimePoint = std::chrono::system_clock::now();
    result.success = outcome.isSuccess();
    if (outcome.isSuccess()) {
        result.transferSize = outcome.result().Metadata().ContentLength();
    }
    return result;
}

static taskResult download_resumable(const OssClient &client, const std::string &key, const std::string &fileToSave)
{
    taskResult result;
    DownloadObjectRequest request(Config::BucketName, key, fileToSave);
    request.setPartSize(Config::PartSize);
    request.setThreadNum(Config::Parallel);
    result.startTimePoint = std::chrono::system_clock::now();

    auto outcome = client.ResumableDownloadObject(request);

    result.stopTimePoint = std::chrono::system_clock::now();
    result.success = outcome.isSuccess();
    if (outcome.isSuccess()) {
        result.transferSize = outcome.result().Metadata().ContentLength();
    }
    return result;
}

static void runSingleTask(int taskId)
{
    taskResult result;
    TaskResultVector &resultVec = totalResults[taskId];
    std::string key = get_task_key(taskId);
    std::string fileName = get_task_fileName(taskId);
    std::stringstream ss;
    auto startTime = std::chrono::system_clock::now();

    int64_t taskMinTransferDurationMs = 0x7FFFFFFF;
    int64_t taskMaxTransferDurationMs = -1;
    int64_t taskAvgTrasnferDurationMs = -1;
    int64_t taskTransferSize = 0;
    int64_t taskTransferDurationMs = 0;
    int taskSucessCnt = 0;
    int taskFailCnt   = 0;

    ClientConfiguration conf;
    auto rateLimiter = std::make_shared<DefaultRateLimiter>();
    if (Config::SpeedKBPerSec > 0) {
        rateLimiter->setRate(Config::SpeedKBPerSec);
        conf.sendRateLimiter = rateLimiter;
        conf.recvRateLimiter = rateLimiter;
    }

    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    if (Config::LoopTimes == 0)
    {
        Config::Persistent = true;
    }

    ss.str();
    ss << "\n++++ START " << Config::Command << 
        " : taskID=" << taskId << 
        ", LocalFile=" << fileName <<
        ", RemoteKey=" << key <<
        ", StartTime=" << to_datetime_string(startTime) << std::endl;
    log_msg(std::cout, ss.str());

    int runIndex = 1;
    //while ((runIndex <= Config::LoopTimes) || Config::Persistent)
    while (!is_test_done(runIndex, startTime))
    {
        if (Config::Command == "upload") {
            result = upload(client, key, fileName);
        }
        else if (Config::Command == "upload_resumable") {
            result = upload_resumable(client, key, fileName);
        }
        else if (Config::Command == "upload_async") {
            result = upload_async(client, key, fileName);
        }
        else if (Config::Command == "upload_multipart") {
            result = upload_multipart(client, key, fileName);
        }
        else if (Config::Command == "download") {
            result = download(client, key, fileName);
        }
        else if (Config::Command == "download_resumable") {
            result = download_resumable(client, key, fileName);
        }
        else if (Config::Command == "download_async") {
            result = download_async(client, key, fileName);
        } 
        else {
            std::cout << "The Command Type Error " << Config::Command << std::endl;
            Config::PrintHelp();
            break;
        }

        if (result.success) {
            int64_t trasnferDuration = (std::chrono::duration_cast<std::chrono::milliseconds>(result.stopTimePoint - result.startTimePoint)).count();
            taskTransferSize += result.transferSize;
            taskTransferDurationMs += trasnferDuration;
            taskSucessCnt += 1;

            taskMinTransferDurationMs = std::min(trasnferDuration, taskMinTransferDurationMs);
            taskMaxTransferDurationMs = std::max(trasnferDuration, taskMaxTransferDurationMs);
        }
        else {
            taskFailCnt += 1;
        }

        if (Config::DumpDetail || Config::PrintPercentile) {
            result.taskId = taskId;
            result.seqNum = runIndex;
            resultVec.push_back(result);
        }
        runIndex++;
    }

    if (taskSucessCnt > 0) {
        taskAvgTrasnferDurationMs = taskTransferDurationMs / taskSucessCnt;
    }

    double taskTransferRateMBPerS = ((double)taskTransferSize / 1024.0f / 1024.0f) / ((double)taskTransferDurationMs / 1000.0f);

    ss.str("");
    ss << "\n---- STOP  " << Config::Command <<
        " : taskID=" << taskId <<
        ", LocalFile=" << fileName <<
        ", RemoteKey=" << key <<
        ", StopTime=" << to_datetime_string(result.stopTimePoint) << " " <<
        ", runTimes=" << runIndex <<
        ", OK=" << taskSucessCnt << 
        ", NG=" << taskFailCnt << std::setiosflags(std::ios::fixed) << std::setprecision(2) <<
        ", AvgTransferRate="<< taskTransferRateMBPerS << " MB / S" <<
        ", Latency(min,max,avg)=(" << taskMinTransferDurationMs << "," << taskMaxTransferDurationMs << "," << taskAvgTrasnferDurationMs << ") Ms"<< std::endl;
    log_msg(std::cout, ss.str());

    {
    std::unique_lock<std::mutex> lck(updateMtx);
    totalTransferSize += taskTransferSize;
    totalTransferDurationMS += taskTransferDurationMs;
    totalSucessCnt += taskSucessCnt;
    totalFailCnt += taskFailCnt;
    minTransferDurationMS = std::min(minTransferDurationMS, taskMinTransferDurationMs);
    maxTransferDurationMS = std::max(maxTransferDurationMS, taskMaxTransferDurationMs);
    }

}

void statistic_report_begin()
{
    //calc upload file crc64 for
    if (Config::Command.compare(0, 6, "upload") == 0) {
        uploadFileCRC64 = get_file_crc64(Config::BaseLocalFile);
    }

    totalTransferDurationMS = 1;
    minTransferDurationMS = 0x7FFFFFFF;
    maxTransferDurationMS = -1;
    totalTransferSize = 0;
    totalStartTimePoint = std::chrono::system_clock::now();
    totalSucessCnt = 0;
    totalFailCnt = 0;
    totalResults.resize(Config::Multithread);
    std::stringstream ss;
    ss << std::endl <<"The Begin : StartTime =" << to_datetime_string(totalStartTimePoint) << std::endl;
    log_msg(std::cout, ss.str());
}

void statistic_report_end()
{
    totalStopTimePoint = std::chrono::system_clock::now();
    auto tp_diff = std::chrono::duration_cast<std::chrono::milliseconds>(totalStopTimePoint - totalStartTimePoint);
    int64_t totalTimeMS = tp_diff.count();
    std::stringstream ss;
    ss << std::endl << std::setiosflags(std::ios::fixed) << std::setprecision(2);
    ss << std::endl << "The End : StopTime =" << to_datetime_string(totalStopTimePoint) << ", TotalTime= " << tp_diff.count()/1000LL << " S" <<
                    ", Command=" << Config::Command <<
                    ", LocalFile=" << Config::BaseLocalFile <<
                    ", RemoteKey=" << Config::BaseRemoteKey <<
                    ", Parallel=" << Config::Parallel <<
                    ", Multithread=" << Config::Multithread << std::endl <<
                    ", LimitSpeed=" << Config::SpeedKBPerSec << " KB/S" << std::endl;

    double transferSizeMB    = (double)totalTransferSize / 1024.0f / 1024.0f;
    double transferDurationS = (double)totalTimeMS / 1000.0f;
    double transferRateMBPerS = transferSizeMB / transferDurationS;
    int64_t avgTrasnferDurationMs = totalSucessCnt ? (totalTransferDurationMS / totalSucessCnt) : -1;
    ss << "#### StatisticReport: AvgTransferRate="<< transferRateMBPerS << " MB/S" <<
                                ", TotalSize=" << transferSizeMB << " MB"
                                ", TotalDuration=" << transferDurationS << " Seconds." << 
                                ", OK=" << totalSucessCnt << 
                                ", NG=" << totalFailCnt  << 
                                ", Latency(min,max,avg)=(" << minTransferDurationMS << "," << maxTransferDurationMS << "," << avgTrasnferDurationMs << ") Ms";
    
    if (Config::PrintPercentile) {
        int64_t per90, per95;
        get_90th_95th_latency_percentile(per90, per95);
        ss << ", Latency Percentile(90th, 95th) Ms=(" << per90 << "," << per95 << ")";
    }
    ss << std::endl;
    log_msg(std::cout, ss.str());

    if (Config::DumpDetail) {
        log_result_detail_msg(std::cout);
    }
}

void LogCallbackFunc(LogLevel level, const std::string &stream)
{
    if (level == LogLevel::LogOff)
        return;

    std::cout << stream;
}

int main(int argc, char **argv)
{
    std::vector<std::future<void>> taskVec;
    if (Config::ParseArg(argc, argv) != 0) {
        return 0;
    }

    if (Config::LoadCfgFile() != 0) {
        return 0;
    }

    AlibabaCloud::OSS::InitializeSdk();

    if (Config::Debug) {
        AlibabaCloud::OSS::SetLogLevel(AlibabaCloud::OSS::LogLevel::LogAll);
        AlibabaCloud::OSS::SetLogCallback(LogCallbackFunc);
    }

    statistic_report_begin();

    for (int i = 0; i < Config::Multithread; i++) {
        auto task = std::async(std::launch::async, runSingleTask, i);
        taskVec.emplace_back(std::move(task));
    }

    for (int i = 0; i < Config::Multithread; i++) {
        taskVec[i].get();
    }

    statistic_report_end();

    AlibabaCloud::OSS::ShutdownSdk();

    return 0;
}
