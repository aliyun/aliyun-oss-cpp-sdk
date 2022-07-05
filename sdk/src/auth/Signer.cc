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

#include "Signer.h"

using namespace AlibabaCloud::OSS;

byteArray::byteArray(const std::string &str) : len_(str.size())
{
    str_ = (byte*)malloc(sizeof(byte) * len_);
    memcpy((void *)str_, str.c_str(), len_);
}

byteArray::byteArray(byte *str, size_t len) : len_(len)
{
    str_ = (byte*)malloc(sizeof(byte) * (len_+1));
    memcpy((void *)str_, str, len_ + 1);
}

byteArray::byteArray() : str_(nullptr), len_(0)
{
}

byteArray::byteArray(char *str, size_t len) : len_(len)
{ 
    str_ = (byte*)malloc(sizeof(byte) * (len_+1));
    memcpy((void *)str_, str, len_ + 1);
    // std::cerr << str_ << "   " << len_ << std::endl;
    // std::cerr << (unsigned char *)str << "   " << len_ << std::endl;
}

Signer::Signer(Type type, const std::string &name, const std::string &version) : type_(type),
                                                                                 name_(name),
                                                                                 version_(version)
{
}

Signer::~Signer()
{
}

std::string Signer::name() const
{
    return name_;
}

Signer::Type Signer::type() const
{
    return type_;
}

std::string Signer::version() const
{
    return version_;
}
