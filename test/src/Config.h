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

#include <string>
class Config
{
public:
    static void ParseArg(int argc, char **argv);
    static bool InitTestEnv();
    static std::string GetDataPath();
public:
    static std::string AccessKeyId;
    static std::string AccessKeySecret;
    static std::string Endpoint;
    static std::string SecondEndpoint;
    static std::string CallbackServer;
    static std::string CfgFilePath;
    static std::string PayerAccessKeyId;
    static std::string PayerAccessKeySecret;
    static std::string PayerUID;
    static std::string RamRoleArn;
    static std::string RamUID;
};

