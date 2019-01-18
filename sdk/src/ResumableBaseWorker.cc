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

#include <alibabacloud/oss/Const.h>
#include "ResumableBaseWorker.h"
#include "utils/FileSystemUtils.h"

using namespace AlibabaCloud::OSS;


ResumableBaseWorker::ResumableBaseWorker(uint64_t objectSize, uint64_t partSize) :
    hasRecord_(false),
    objectSize_(objectSize),
    consumedSize_(0),
    partSize_(partSize)
{
}

ResumableBaseWorker::ResumableBaseWorker(const std::string& filePath, uint64_t partSize) :
    hasRecord_(false),
    consumedSize_(0),
    partSize_(partSize)
{
    std::fstream content(filePath, std::ios::in | std::ios::binary);
    objectSize_ = GetIOStreamLength(content);
    content.close();
}

int ResumableBaseWorker::validate(OssError& err)
{
    recordPath_ = getRecordPath();
    if (!recordPath_.empty()) {
        if (0 != loadRecord()) {
            RemoveFile(recordPath_);
        }
    }

    if (hasRecord_) {
        if (0 != validateRecord()) {
            RemoveFile(recordPath_);
            if (0 != prepare(err)) {
                return -1;
            }
        }
    }
    else {
        if (0 != prepare(err)) {
            return -1;
        }
    }
    return 0;
}

void ResumableBaseWorker::determinePartSize()
{
    uint64_t partSize = partSize_;
    uint64_t objectSize = objectSize_;
    uint64_t partCount = (objectSize - 1) / partSize + 1;
    while (partCount > PartNumberUpperLimit) {
        partSize = partSize * 2;
        partCount = (objectSize - 1) / partSize + 1;
    }

    partSize_ = partSize;
}
