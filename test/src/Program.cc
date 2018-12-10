/*
 * Copyright 2009-2017 Alibaba Cloud All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <alibabacloud/oss/OssClient.h>
#include <gtest/gtest.h>
#include "Config.h"

int main(int argc, char **argv)
{
    std::cout << "oss-cpp-sdk test" << std::endl;
    Config::ParseArg(argc, argv);
    if (!Config::InitTestEnv()) {
        std::cout << "One of AK,SK or Endpoint is not configured." << std::endl;
        return -1;
    }
    testing::InitGoogleTest(&argc, argv);
    srand((int)time(0));
    AlibabaCloud::OSS::InitializeSdk();
    //for code coverage
    AlibabaCloud::OSS::InitializeSdk();
    if (!AlibabaCloud::OSS::IsSdkInitialized()) {
        std::cout << "oss-cpp-sdk test InitializeSdk fail." << std::endl;
        return -1;
    }
    int ret = RUN_ALL_TESTS();
    AlibabaCloud::OSS::ShutdownSdk();
    //for code coverage
    AlibabaCloud::OSS::ShutdownSdk();
    return ret;
}
