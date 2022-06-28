#include <string.h>
#include <string>
#include "Config.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace AlibabaCloud::OSS::PTest;


std::string Config::Version = "cpp-sdk-perf-test-1.0.0 ";
std::string Config::Endpoint = "";
std::string Config::AccessKeyId = "";
std::string Config::AccessKeySecret = "";
std::string Config::BucketName = "";
std::string Config::OssCfgFile = "oss.ini";

std::string Config::Command = "";
std::string Config::BaseLocalFile = "";
std::string Config::BaseRemoteKey = "";

int Config::PartSize = 100 * 1024 * 1024;
int Config::Parallel = 5;
int Config::Multithread = 1;
int Config::LoopTimes = 1;
int Config::LoopDurationS = -1;
bool Config::Persistent = false;
bool Config::DifferentSource = false;
bool Config::CrcCheck = true;
int Config::SpeedKBPerSec = 0;   //

bool Config::Debug = false;
bool Config::DumpDetail = false;
bool Config::PrintPercentile = false;

static std::string LeftTrim(const char* source)
{
    std::string copy(source);
    copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](unsigned char ch) { return !::isspace(ch); }));
    return copy;
}

static std::string RightTrim(const char* source)
{
    std::string copy(source);
    copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](unsigned char ch) { return !::isspace(ch); }).base(), copy.end());
    return copy;
}

static std::string Trim(const char* source)
{
    return LeftTrim(RightTrim(source).c_str());
}

static std::string LeftTrimQuotes(const char* source)
{
    std::string copy(source);
    copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](int ch) { return !(ch == '"'); }));
    return copy;
}

static std::string RightTrimQuotes(const char* source)
{
    std::string copy(source);
    copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](int ch) { return !(ch == '"'); }).base(), copy.end());
    return copy;
}

static std::string TrimQuotes(const char* source)
{
    return LeftTrimQuotes(RightTrimQuotes(source).c_str());
}

void Config::PrintHelp()
{
    std::cout << "\n";
    std::cout << "Usage: cpp-sdk-ptest  [-h] [-v] [-c COMMAND] [-f LOCALFILE]      \n";
    std::cout << "                      [-k REMOTEKEY] [-p PARALLEL]      \n";
    std::cout << "                      [-m MULTITHREAD] [--partSize PARTSIZE]      \n";
    std::cout << "                      [-loopTimes TIMES]|[--loopDuration SEC]|[--persistent]      \n";
    std::cout << "                      [--differentsource]      \n";
    std::cout << "Optional arguments:      \n";
    std::cout << "  -h, --help          show this help mestd::coutage and exit.           \n";
    std::cout << "  -v                  show program's version number and exit.    \n";
    std::cout << "  -c COMMAND          Command Type : upload(up), upload_resumable(upr), upload_async(upa), download(dn), download_async(dna) .  \n";
    std::cout << "  -b BUCKETNAME       bucket name.                \n";
    std::cout << "  -f LOCALFILE        local filename to transfer.                \n";
    std::cout << "  -k REMOTEKEY        remote object key.                         \n";
    std::cout << "  -p PARALLEL         parallel threads parameter for resumable breakpoint transfer.      \n";
    std::cout << "  -m MULTITHREAD      multithread number for transfer.           \n";
    std::cout << "  --partSize PARTSIZE part size parameter for resume breakpoint transfer.                \n";
    std::cout << "  --loopTimes TIMES   how many times to do test.                   \n";
    std::cout << "  --loopDuration SEC  how many seconds to do test.                   \n";
    std::cout << "  --persistent        Whether run the command persistantly.      \n";
    std::cout << "  --differentsource   Whether transfer from different source files.  \n";
    std::cout << "  --limit SPEED       Whether to limit the upload or download speed, in kB/s.  \n";
    std::cout << "  --detail            print detail inforamtion for each testcase. \n";
    std::cout << "  --percentile        print the 90th and 95th percentile value. \n";


    std::cout << "\nExamples :  \n";
    std::cout << "    cpp-sdk-ptest -c upload -f mylocalfilename -k myobjectkeyname \n";
    std::cout << "    cpp-sdk-ptest -c up -f mylocalfilename -k myobjectkeyname -b mybucketname\n";
    std::cout << "    cpp-sdk-ptest -c upload_resumable -f mylocalfilename -k myobjectkeyname -p 5 \n";
    std::cout << "    cpp-sdk-ptest -c upload_multipart -f mylocalfilename -k myobjectkeyname -p 5 \n";
    std::cout << "    cpp-sdk-ptest -c upr -f mylocalfilename -k myobjectkeyname -p 10 \n";
    std::cout << "    cpp-sdk-ptest -c upload_async -f mylocalfilename -k myobjectkeyname \n";
    std::cout << "    cpp-sdk-ptest -c upa -f mylocalfilename -k myobjectkeyname -m 5 \n";
    std::cout << "    cpp-sdk-ptest -c up -f mylocalfilename -k myobjectkeyname -m 5 \n";
    std::cout << "    cpp-sdk-ptest -c download -f mylocalfilename -k myobjectkeyname \n";
    std::cout << "    cpp-sdk-ptest -c download_resumable -f mylocalfilename -k myobjectkeyname -p 5 \n";
    std::cout << "    cpp-sdk-ptest -c dnr -f mylocalfilename -k myobjectkeyname \n";
    std::cout << "    cpp-sdk-ptest -c dnr -f mylocalfilename -k myobjectkeyname -p 5 \n";
    std::cout << "    cpp-sdk-ptest -c download_async -f mylocalfilename -k myobjectkeyname \n";
    std::cout << "    cpp-sdk-ptest -c dna -f mylocalfilename -k myobjectkeyname -m 5 \n";
    std::cout << "    cpp-sdk-ptest -c dn -f mylocalfilename -k myobjectkeyname -m 5 \n";
}

void Config::PrintCfgInfo()
{
    std::cout << "\n";
    std::cout << "version         : " << Config::Version << std::endl;
    std::cout << "endpoint        : " << Config::Endpoint << std::endl;
    std::cout << "accessKeyId     : " << Config::AccessKeyId << std::endl;
    std::cout << "bucketName      : " << Config::BucketName << std::endl;
    std::cout << "command         : " << Config::Command << std::endl;
    std::cout << "localfile       : " << Config::BaseLocalFile << std::endl;
    std::cout << "remotekey       : " << Config::BaseRemoteKey << std::endl;
    if (!Config::Command.compare("upload_resumable") || Config::Command.compare("download_resumable")) 
    {
    std::cout << "parallel        : " << Config::Parallel << std::endl;
    std::cout << "partSize        : " << Config::PartSize << std::endl;
    }
    std::cout << "multithread     : " << Config::Multithread << std::endl;
    std::cout << "loopTimes       : " << Config::LoopTimes << std::endl;
    std::cout << "loopDuration    : " << Config::LoopDurationS << std::endl;
    std::cout << "persistent      : " << Config::Persistent << std::endl;
    std::cout << "differentsource : " << Config::DifferentSource << std::endl;
    std::cout << "limit speed     : " << Config::SpeedKBPerSec << std::endl;
}

int Config::ParseArg(int argc, char **argv)
{
    if (argc < 2)
    {
        PrintHelp();
        return -1;
    }

    int i = 1;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
                PrintHelp();
                return -1;
            }
            else if (!strcmp("-v", argv[i])) {
                std::cout << Config::Version << std::endl;
                return -1;
            }
            else if (!strcmp("-c", argv[i])) {
                Config::Command = argv[i + 1];
                if (Config::Command == "up")
                {
                    Config::Command = "upload";
                }
                else if (Config::Command == "upr")
                {
                    Config::Command = "upload_resumable";
                }
                else if (Config::Command == "upa")
                {
                    Config::Command = "upload_async";
                }
                else if (Config::Command == "dn")
                {
                    Config::Command = "download";
                }
                else if (Config::Command == "dnr")
                {
                    Config::Command = "download_resumable";
                }
                else if (Config::Command == "dna")
                {
                    Config::Command = "download_async";
                }
                i++;
            }
            else if (!strcmp("-b", argv[i])) {
                Config::BucketName = argv[i + 1];
                i++;
            }
            else if (!strcmp("-f", argv[i])) {
                Config::BaseLocalFile = argv[i + 1];
                i++;
            }
            else if (!strcmp("-k", argv[i])) {
                Config::BaseRemoteKey = argv[i + 1];
                i++;
            }
            else if (!strcmp("-p", argv[i])) {
                Config::Parallel = std::atoi(argv[i + 1]);
                i++;
            }
            else if (!strcmp("-m", argv[i])) {
                Config::Multithread = std::atoi(argv[i + 1]);
                i++;
            }
            else if (!strcmp("-d", argv[i])) {
                Config::Debug = true;
            }
            else if (!strcmp("--partSize", argv[i])) {
                Config::PartSize = std::atoi(argv[i + 1]);
                i++;
            }
            else if (!strcmp("--loopTimes", argv[i])) {
                Config::LoopTimes = std::atoi(argv[i + 1]);
                Config::LoopDurationS = -1;
                i++;
            }
            else if (!strcmp("--loopDuration", argv[i])) {
                Config::LoopDurationS = std::atoi(argv[i + 1]);
                Config::LoopTimes = -1;
                i++;
            }
            else if (!strcmp("--persistent", argv[i])) {
                Config::Persistent = true;
                i++;
            }
            else if (!strcmp("--differentsource", argv[i])) {
                i++;
            }
            else if (!strcmp("--limit", argv[i])) {
                Config::SpeedKBPerSec = std::atoi(argv[i + 1]);
                i++;
            }
            else if (!strcmp("--detail", argv[i])) {
                Config::DumpDetail = true;
            }
            else if (!strcmp("--percentile", argv[i])) {
                Config::PrintPercentile = true;
            }
        }
        i++;
    };
    return 0;
}


int Config::LoadCfgFile()
{
    std::fstream in(Config::OssCfgFile, std::ios::in | std::ios::binary);
    if (!in.good()) {
        std::cout << "Open oss.ini file fail." << std::endl;
        return 1;
    }

    char buffer[256];
    char *ptr;

    while (in.getline(buffer, 256)) {
        ptr = strchr(buffer, '=');
        if (!ptr) {
            continue;
        }
        if (!strncmp(buffer, "AccessKeyId", 11)) {
            Config::AccessKeyId = TrimQuotes(Trim(ptr + 1).c_str());
        } 
        else if (!strncmp(buffer, "AccessKeySecret", 15)) {
            Config::AccessKeySecret = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "Endpoint", 8)) {
            Config::Endpoint = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "BucketName", 10)) {
            Config::BucketName = TrimQuotes(Trim(ptr + 1).c_str());
        }
    }

    if (Config::AccessKeyId.empty() ||
        Config::AccessKeySecret.empty() ||
        Config::Endpoint.empty() ||
        Config::BucketName.empty()) {
        std::cout << "one of AK, SK, endpoint and BucketName is not config, please set the oss.ini file." << std::endl;
        return 1;
    }

    return 0;
}