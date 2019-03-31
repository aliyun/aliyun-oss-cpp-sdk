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
#include <map>
#include <ctime>
#include <iostream>
#include <alibabacloud/oss/Types.h>
#include "FileSystemUtils.h"
#include <alibabacloud/oss/Const.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#define  oss_access(a)  ::_access((a), 0)
#define  oss_mkdir(a)   ::_mkdir(a)
#define  oss_rmdir(a)   ::_rmdir(a)
#define  oss_stat       _stat64
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#define  oss_access(a)  ::access(a, 0)
#define  oss_mkdir(a)   ::mkdir((a), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define  oss_rmdir(a)   ::rmdir(a)
#define  oss_stat       stat
#endif
using namespace AlibabaCloud::OSS;

bool AlibabaCloud::OSS::CreateDirectory(const std::string &folder)
{
    std::string folder_builder;
    std::string sub;
    sub.reserve(folder.size());
    for (auto it = folder.begin(); it != folder.end(); ++it) {
        const char c = *it;
        sub.push_back(c);
        if (c == PATH_DELIMITER || it == folder.end() - 1) {
            folder_builder.append(sub);
            if (oss_access(folder_builder.c_str()) != 0) {
                if (oss_mkdir(folder_builder.c_str()) != 0) {
                    return false;
                }
            }
            sub.clear();
        }
    }
    return true;
}

bool AlibabaCloud::OSS::IsDirectoryExist(std::string folder) {
    if (folder[folder.length() - 1] != PATH_DELIMITER && folder[folder.length() - 1] != '/') {
        folder += PATH_DELIMITER;
    }
    return !oss_access(folder.c_str());
}

bool AlibabaCloud::OSS::RemoveDirectory(const std::string &folder)
{
    return !oss_rmdir(folder.c_str());
}

bool AlibabaCloud::OSS::RemoveFile(const std::string &filepath)
{
    int ret = ::remove(filepath.c_str());
    return !ret;
}

bool AlibabaCloud::OSS::RenameFile(const std::string &from, const std::string &to)
{
    return !::rename(from.c_str(), to.c_str());
}

bool AlibabaCloud::OSS::GetPathLastModifyTime(const std::string & path, time_t &t)
{
    struct oss_stat buf;
	auto filename = path.c_str();
#if defined(_WIN32) && _MSC_VER < 1900
	std::string tmp;
	if (!path.empty() && (path.rbegin()[0] == PATH_DELIMITER)) {
		tmp = path.substr(0, path.size() - 1);
		filename = tmp.c_str();
	} 
#endif
	if (oss_stat(filename, &buf) != 0)
        return false;

    t = buf.st_mtime;
    return true;
}
