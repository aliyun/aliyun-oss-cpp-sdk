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

#include <string>
#include <cstring>
#include <sstream>
#include <alibabacloud/oss/Types.h>
#include <alibabacloud/oss/client/ClientConfiguration.h>

namespace AlibabaCloud
{
    namespace OSS
    {
        using byte = unsigned char;
        struct byteArray
        {
            byteArray(const std::string &str);
            byteArray(byte *str, size_t len);
            byteArray(char *str, size_t len);
            byteArray();
            ~byteArray()
            {
                // 构造函数均为拷贝，因此析构需要释放
                if (str_ != nullptr)
                {
                    free((byte *)str_);
                    str_ = nullptr;
                }
                len_ = 0;
            }

            operator std::string()
            {
                return std::string{(char *)(str_), len_} + "\0";
            }

            // std::string toString();

            const byte *str_;
            size_t len_;
        };

        class Signer
        {
        public:
            enum Type
            {
                HmacSha1,
                HmacSha256
            };
            virtual ~Signer();

            // byte array
            virtual byteArray generate(const byteArray &src, const std::string &secret) const = 0;
            std::string name() const;
            Type type() const;
            std::string version() const;

        protected:
            Signer(Type type, const std::string &name, const std::string &version = "1.0");

        private:
            Type type_;
            std::string name_;
            std::string version_;
        };
    }
}
