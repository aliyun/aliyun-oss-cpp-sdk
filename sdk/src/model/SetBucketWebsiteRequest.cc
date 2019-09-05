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

#include <alibabacloud/oss/model/SetBucketWebsiteRequest.h>
#include <sstream>
#include <../utils/Utils.h>
#include "ModelError.h"

using namespace AlibabaCloud::OSS;

SetBucketWebsiteRequest::SetBucketWebsiteRequest(const std::string& bucket) :
    OssBucketRequest(bucket),
    indexDocumentIsSet_(false),
    errorDocumentIsSet_(false)
{
    setFlags(Flags() | REQUEST_FLAG_CONTENTMD5);
}

std::string SetBucketWebsiteRequest::payload() const
{
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    ss << "<WebsiteConfiguration>" << std::endl;
    ss << "  <IndexDocument>" << std::endl;
    ss << "    <Suffix>" << indexDocument_ << "</Suffix>" << std::endl;
    ss << "  </IndexDocument>" << std::endl;
    if (errorDocumentIsSet_) {
        ss << "  <ErrorDocument>" << std::endl;
        ss << "    <Key>" << errorDocument_ << "</Key>" << std::endl;
        ss << "  </ErrorDocument>" << std::endl;
    }
	if (!websiteRoutingRule_.empty()) {
		ss << "  <RoutingRules>" << std::endl;
		for (const auto& rule : websiteRoutingRule_) {
			ss << "  <RoutingRule>" << std::endl;
			ss << "  <RuleNumber>" << rule.getRuleNumber() << "</RuleNumber>" << std::endl;
			if (rule.getRoutingRuleCondition().hasRoutingRuleCondition()) {
				ss << "<Condition>" << std::endl;
				if (rule.getRoutingRuleCondition().hasKeyPrefixEquals()) {
					ss << "<KeyPrefixEquals>" << rule.getRoutingRuleCondition().getKeyPrefixEquals() << "</KeyPrefixEquals>" << std::endl;
				}
				if (rule.getRoutingRuleCondition().hasHttpErrorCodeReturnedEquals()) {
					ss << "<HttpErrorCodeReturnedEquals>" << rule.getRoutingRuleCondition().getHttpErrorCodeReturnedEquals() << "</HttpErrorCodeReturnedEquals>" << std::endl;
				}
				if (rule.getRoutingRuleCondition().hasRoutingRuleIncludeHeaderList()) {
					ss << "<IncludeHeader>" << std::endl;

					for (const auto& header : rule.getRoutingRuleCondition().getIncludeHeaderList()) {
						ss << "  <Key>" << header.getKey() << "</Key>" << std::endl;
						ss << "  <Equals>" << header.getEquals() << "</Equals>" << std::endl;
					}
					ss << "</IncludeHeader>" << std::endl;
				}
				ss << "</Condition>" << std::endl;
			}
			if (rule.getRoutingRuleRedirect().getRedirectType() != AlibabaCloud::OSS::RedirectType::INVALID) {
				ss << "  <Redirect>" << std::endl;
				ss << "  <RedirectType>" << ToRedirectTypeName(rule.getRoutingRuleRedirect().getRedirectType()) << "</RedirectType>" << std::endl;
				if (!rule.getRoutingRuleRedirect().getPassQueryString().empty())
				{
					ss << "  <PassQueryString>" << rule.getRoutingRuleRedirect().getPassQueryString() << "</PassQueryString>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getMirrorURL().empty())
				{
					ss << "  <MirrorURL>" << rule.getRoutingRuleRedirect().getMirrorURL() << "</MirrorURL>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getMirrorPassQueryString().empty())
				{
					ss << "  <MirrorPassQueryString>" << rule.getRoutingRuleRedirect().getMirrorPassQueryString() << "</MirrorPassQueryString>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getMirrorFollowRedirect().empty())
				{
					ss << "  <MirrorFollowRedirect>" << rule.getRoutingRuleRedirect().getMirrorFollowRedirect() << "</MirrorFollowRedirect>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getMirrorCheckMd5().empty())
				{
					ss << "  <MirrorCheckMd5>" << rule.getRoutingRuleRedirect().getMirrorCheckMd5() << "</MirrorCheckMd5>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getMirrorMultiAlternates().empty())
				{
					ss << "<MirrorMultiAlternates>" << std::endl;
					for (const auto& data : rule.getRoutingRuleRedirect().getMirrorMultiAlternates())
					{
						ss << "<MirrorMultiAlternate>" << std::endl;

						ss << "<MirrorMultiAlternateNumber>" << data.getMirrorMultiAlternateNumber() << "</MirrorMultiAlternateNumber>" << std::endl;
						ss << "<MirrorMultiAlternateURL>" << data.getMirrorMultiAlternateUrl() << "</MirrorMultiAlternateURL>" << std::endl;

						ss << "</MirrorMultiAlternate>" << std::endl;
					}
					ss << "</MirrorMultiAlternates>" << std::endl;
				}
				if (rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().hasRoutingRuleMirrorHeaders())
				{
					ss << "  <MirrorHeaders>" << std::endl;
					if (!rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getPassAll().empty())
					{
						ss << "  <PassAll>" << rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getPassAll() << "</PassAll>" << std::endl;
					}
					if (!rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getPass().empty())
					{
						for (const auto& pass : rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getPass())
						{
							ss << "  <Pass>" << pass << "</Pass>" << std::endl;
						}
					}
					if (!rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getRemove().empty())
					{
						for (const auto& remove : rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getRemove())
						{
							ss << "  <Remove>" << remove << "</Remove>" << std::endl;
						}
					}
					if (!rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getMirrorHeadersSetList().empty())
					{
						ss << "  <Set>" << std::endl;
						for (const auto& set : rule.getRoutingRuleRedirect().getRoutingRuleMirrorHeaders().getMirrorHeadersSetList())
						{

							ss << "  <Key>" << set.getKey() << "</Key>" << std::endl;
							ss << "  <Value>" << set.getValue() << "</Value>" << std::endl;
						}
						ss << "  </Set>" << std::endl;
					}

					ss << "  </MirrorHeaders>" << std::endl;

				}
				if (!rule.getRoutingRuleRedirect().getProtocol().empty())
				{
					ss << "  <Protocol>" << rule.getRoutingRuleRedirect().getProtocol() << "</Protocol>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getHostName().empty())
				{
					ss << "  <HostName>" << rule.getRoutingRuleRedirect().getHostName() << "</HostName>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getHttpRedirectCode().empty())
				{
					ss << "  <HttpRedirectCode>" << rule.getRoutingRuleRedirect().getHttpRedirectCode() << "</HttpRedirectCode>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getReplaceKeyPrefixWith().empty())
				{
					ss << "  <ReplaceKeyPrefixWith>" << rule.getRoutingRuleRedirect().getReplaceKeyPrefixWith() << "</ReplaceKeyPrefixWith>" << std::endl;
				}
				if (!rule.getRoutingRuleRedirect().getReplaceKeyWith().empty())
				{
					ss << "  <ReplaceKeyWith>" << rule.getRoutingRuleRedirect().getReplaceKeyWith() << "</ReplaceKeyWith>" << std::endl;
				}
				ss << "  </Redirect>" << std::endl;
			}
			ss << "  </RoutingRule>" << std::endl;
		}
		ss << "  </RoutingRules>" << std::endl;
	}
    ss << "</WebsiteConfiguration>" << std::endl;
    return ss.str();
}

ParameterCollection SetBucketWebsiteRequest::specialParameters() const
{
    ParameterCollection parameters;
    parameters["website"] = "";
    return parameters;
}

static bool IsValidWebpage(const std::string &webpage)
{
    const std::string pageSuffix = ".html";
    return (webpage.size() > pageSuffix.size()) &&
        (webpage.substr(webpage.size() - 5).compare(".html") == 0);
}

int SetBucketWebsiteRequest::validate() const
{
    int ret = OssBucketRequest::validate();
    if (ret != 0) 
        return ret;

    if (indexDocument_.empty())
        return ARG_ERROR_WEBSITE_INDEX_DOCCUMENT_EMPTY;

    if (!IsValidWebpage(indexDocument_))
        return ARG_ERROR_WEBSITE_INDEX_DOCCUMENT_NAME_INVALID;

    if (errorDocumentIsSet_ && !IsValidWebpage(errorDocument_))
        return ARG_ERROR_WEBSITE_ERROR_DOCCUMENT_NAME_INVALID;

    return 0;
}

