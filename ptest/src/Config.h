#include <string>

namespace AlibabaCloud
{
namespace OSS
{
namespace PTest
{
    class Config
    {
    public:
        Config() = default;
        static void PrintHelp();
        static void PrintCfgInfo();
        static int ParseArg(int argc, char **argv);
        static int LoadCfgFile();


    public:
        static std::string Version;
        static std::string Endpoint;
        static std::string AccessKeyId;
        static std::string AccessKeySecret;
        static std::string BucketName;
        static std::string OssCfgFile;

        static std::string Command;
        static std::string BaseLocalFile;
        static std::string BaseRemoteKey;

        static int PartSize;
        static int Parallel;
        static int Multithread;
        static int LoopTimes;
        static int LoopDurationS;
        static bool Persistent;
        static bool DifferentSource;
        static bool CrcCheck;

        static int SpeedKBPerSec;

        static bool Debug;
        static bool DumpDetail;
        static bool PrintPercentile;
    };
}
}
}

