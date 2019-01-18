# Alibaba Cloud OSS C++ Software Development Kit
[中文文档](./README_zh.md)

Alibaba Cloud Object Storage Service (OSS) is a cloud storage service provided by Alibaba Cloud, featuring massive capacity, security, a low cost, and high reliability. You can upload and download data on any application anytime and anywhere by calling APIs, and perform simple management of data through the web console. The OSS can store any type of files and therefore applies to various websites, development enterprises and developers.

The OSS SDK for C++ provides a variety of modern C++ (version C++ 11 or later) interfaces for convenient use of the OSS.

This document introduces how to obtain and call Alibaba Cloud OSS C++ SDK.

If you have any problem while using C++ SDK, please contact us.


## Prerequisites

To use Alibaba Cloud OSS C++ SDK, you must:

* Have an Alibaba Cloud account and an AccessKey. The AccessKey is required when initializing the client. You can create an AccessKey in the Alibaba Cloud console. For more information, see [Create an AccessKey](https://usercenter.console.aliyun.com/?spm=5176.doc52740.2.3.QKZk8w#/manage/ak)

> **Note:** To increase the security of your account, we recommend that you use the AccessKey of the RAM user to access Alibaba Cloud services.


* Install a compiler that supports C + + 11 or later:
	* Visual Studio 2013 or later:
	* or GCC 4.8 or later:
	* or Clang 3.3 or later:

## Building the SDK

1. Download or clone from GitHub [aliyun-oss-cpp-sdk](https://github.com/aliyun/aliyun-oss-cpp-sdk)

* Download https://github.com/aliyun/aliyun-oss-cpp-sdk/archive/master.zip
* Clone source codes

```
git clone https://github.com/aliyun/aliyun-oss-cpp-sdk.git
```

2. Install CMake 3.1 or later, enter SDK to create build files required for the build

```
cd <path/to/aliyun-oss-cpp-sdk>
mkdir build
cd build
cmake ..
```

### Windows

Enter build folder, open alibabacloud-oss-cpp-sdk.sln with Visual Studio.

Or run the the following commands to build and install:

```
msbuild ALL_BUILD.vcxproj
msbuild INSTALL.vcxproj
```

### Linux

Install third-party libraries on the Linux platform, including `libcurl` and `libopenssl`.

Run the following commands on the Redhat/Fedora system to install third-party libraries.
```
sudo dnf install libcurl-devel openssl-devel
```

Run the following commands on the Debian/Ubuntu system to install third-party libraries.
```
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

Run the following commands to build and install sdk:
```
make
sudo make install
```

### Mac
On Mac you should specify openssl path. For example, openssl is installed in /usr/local/Cellar/openssl/1.0.2p, run the following commands
```
cmake -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2p  \
      -DOPENSSL_LIBRARIES=/usr/local/Cellar/openssl/1.0.2p/lib  \
      -DOPENSSL_INCLUDE_DIRS=/usr/local/Cellar/openssl/1.0.2p/include/ ..
make
```

### Android
Build and install third-party libraries, including `libcurl` and `libopenssl` to `$ANDROID_NDK/sysroot`, run the following commands
```
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake  \
      -DANDROID_NDK=$ANDROID_NDK    \
      -DANDROID_ABI=armeabi-v7a     \
      -DANDROID_TOOLCHAIN=clang     \
      -DANDROID_PLATFORM=android-21 \
      -DANDROID_STL=c++_shared ..
make
```

### CMake Option

#### BUILD_SHARED_LIBS
(Default OFF) If turned on, both static and shared libraries are built, and the static library name has a -static suffix. At the same time, the sample project will be linked to the shared library.
```
cmake .. -DBUILD_SHARED_LIBS=ON
```

#### BUILD_TESTS
(Default OFF) If turned on, both test and ptest project will be built.
```
cmake .. -DBUILD_TESTS=ON
```

## Use the C++ SDK

The following code shows how to list buckets owned by the requester. 

> **Note:** Please replace the your-region-id,your-access-key-id and your-access-key-secret in the sample.

```
#include <alibabacloud/oss/OssClient.h>
using namespace AlibabaCloud::OSS;

int main(int argc, char** argv)
{
    // Initialize the SDK
    InitializeSdk();

    // Configure the instance
    ClientConfiguration conf;
    OssClient client("your-region-id", "your-access-key-id", "your-access-key-secret", conf);

    // Create an API request
    ListBucketsRequest request;
    auto outcome = client.ListBuckets(request);
    if (!outcome.isSuccess()) {
        // Handle exceptions
        std::cout << "ListBuckets fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;
        ShutdownSdk();
        return -1;
    }

    // Close the SDK
    ShutdownSdk();
    return 0;
}
```

## License
See the LICENSE file (Apache 2.0 license).




