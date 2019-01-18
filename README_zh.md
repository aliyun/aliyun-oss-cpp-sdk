# 阿里云OSS C++工具套件

阿里云对象存储（Object Storage Service，简称OSS），是阿里云对外提供的海量、安全、低成本、高可靠的云存储服务。用户可以通过调用API，在任何应用、任何时间、任何地点上传和下载数据，也可以通过用户Web控制台对数据进行简单的管理。OSS适合存放任意文件类型，适合各种网站、开发企业及开发者使用。

适用于阿里云OSS的 C++ SDK提供了一组现代化的 C++（C++ 11）接口,让您不用复杂编程即可访问阿里云OSS服务。

如果您在使用SDK的过程中遇到任何问题，欢迎前往阿里云SDK问答社区提问，提问前请阅读提问引导。亦可在当前GitHub提交Issues。

完成本文档中的操作开始使用 C++ SDK。


## 前提条件

在使用 C++ SDK 前，确保您已经：

* 注册了阿里云账号并获取了访问密钥（AccessKey）。

> **说明：** 为了保证您的账号安全，建议您使用RAM账号来访问阿里云服务。阿里云账号对拥有的资源有全部权限。RAM账号由阿里云账号授权创建，仅有对特定资源限定的操作权限。详情[参见RAM](https://help.aliyun.com/document_detail/28647.html)。


* 安装支持 C++ 11 或更高版本的编译器：
	* Visual Studio 2013 或以上版本
	* 或 GCC 4.8 或以上版本
	* 或 Clang 3.3 或以上版本

## 从源代码构建 SDK

1. 从 GitHub 下载或 Git 克隆 [aliyun-oss-cpp-sdk](https://github.com/aliyun/aliyun-oss-cpp-sdk)

* 直接下载 https://github.com/aliyun/aliyun-oss-cpp-sdk/archive/master.zip
* 使用 Git 命令获取

```
git clone https://github.com/aliyun/aliyun-oss-cpp-sdk.git
```

2. 安装 cmake 3.1 或以上版本，进入 SDK 创建生成必要的构建文件

```
cd <path/to/aliyun-oss-cpp-sdk>
mkdir build
cd build
cmake ..
```

### Windows

进入 build 目录使用 Visual Studio 打开 alibabacloud-oss-cpp-sdk.sln 生成解决方案。

或者您也可以使用 VS 的开发人员命令提示符，执行以下命令编译并安装：

```
msbuild ALL_BUILD.vcxproj
msbuild INSTALL.vcxproj
```

### Linux

要在 Linux 平台进行编译, 您必须安装依赖的外部库文件 libcurl、libopenssl, 通常情况下，系统的包管理器中的会有提供。

例如：在基于 Redhat / Fedora 的系统上安装这些软件包

```
sudo dnf install libcurl-devel openssl-devel
```
例如：在基于 Debian / Ubuntu 的系统上安装这些软件包
```
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

在安装依赖库后执行以下命令编译并安装：

```
make
sudo make install
```

### Mac
在Mac平台编译时，需要指定openssl 库的路径。例如 openssl安装在 /usr/local/Cellar/openssl/1.0.2p, 请使用如下命令
```
cmake -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2p  \
      -DOPENSSL_LIBRARIES=/usr/local/Cellar/openssl/1.0.2p/lib  \
      -DOPENSSL_INCLUDE_DIRS=/usr/local/Cellar/openssl/1.0.2p/include/ ..
make
```

### Android
例如，在linux 环境下，基于android-ndk-r16 工具链构建工程。可以先把第三方库 `libcurl` 和 `libopenssl` 编译并安装到 `$ANDROID_NDK/sysroot` 下，然后使用如下命令
```
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake  \
      -DANDROID_NDK=$ANDROID_NDK    \
      -DANDROID_ABI=armeabi-v7a     \
      -DANDROID_TOOLCHAIN=clang     \
      -DANDROID_PLATFORM=android-21 \
      -DANDROID_STL=c++_shared ..
make
```

### CMake 选项

#### BUILD_SHARED_LIBS
(默认为关，即OFF) 如果打开，会同时构建静态库和动态库， 静态库名字增加-static后缀。同时，sample工程会链接到sdk的动态库上。
```
cmake .. -DBUILD_SHARED_LIBS=ON
```

#### BUILD_TESTS
(默认为关，即OFF) 如果打开，会构建出test 及 ptest两个测试工程。
```
cmake .. -DBUILD_TESTS=ON
```

## 如何使用 C++ SDK

以下代码展示了如何获取请求者拥有的Bucket。

> **说明：** 您需要替换示例中的 your-region-id、your-access-key-id 和 your-access-key-secret 的值。

```
#include <alibabacloud/oss/OssClient.h>
using namespace AlibabaCloud::OSS;

int main(int argc, char** argv)
{
    // 初始化SDK
    InitializeSdk();

    // 配置实例
    ClientConfiguration conf;
    OssClient client("your-region-id", "your-access-key-id", "your-access-key-secret", conf);

    // 创建API请求
    ListBucketsRequest request;
    auto outcome = client.ListBuckets(request);
    if (!outcome.isSuccess()) {
        // 异常处理
        std::cout << "ListBuckets fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;
        ShutdownSdk();
        return -1;
    }

    // 关闭SDK
    ShutdownSdk();
    return 0;
}
```

## 许可协议
请参阅 LICENSE 文件（Apache 2.0 许可证）。
