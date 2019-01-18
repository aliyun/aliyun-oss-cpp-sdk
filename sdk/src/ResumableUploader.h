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

#pragma once
#include <alibabacloud/oss/model/UploadObjectRequest.h>
#include <alibabacloud/oss/model/Part.h>
#include <alibabacloud/oss/OssError.h>
#include "ResumableBaseWorker.h"

namespace AlibabaCloud
{
namespace OSS
{
    struct UploadRecord{
        std::string opType;
        std::string uploadID;
        std::string filePath;
        std::string bucket;
        std::string key;
        std::string mtime;
        uint64_t size;
        uint64_t partSize;
        std::string md5Sum;
    };

    class ResumableUploader : public ResumableBaseWorker
    {
    public:
        ResumableUploader(const UploadObjectRequest& request, const OssClientImpl *client): 
            ResumableBaseWorker(request.FilePath(), request.PartSize()), 
            request_(request), 
            client_(client)
        {}
	
        PutObjectOutcome Upload();
        
    private:
        int getPartsToUpload(OssError &err, PartList &partsUploaded, PartList &partsToUpload);
        const std::string getRecordPath();
        int loadRecord();
        int validateRecord();
        int prepare(OssError& err);
        void initRecord(const std::string &uploadID);
		
        const UploadObjectRequest& request_;
        UploadRecord record_;
        const OssClientImpl *client_;
        std::string uploadID_;
        static void UploadPartProcessCallback(size_t increment, int64_t transfered, int64_t total, void *userData);
    };
} 
}
