/*
* Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/
#include <aws/iam/model/Statement.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::IAM::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;

Statement::Statement() : 
    m_sourcePolicyIdHasBeenSet(false),
    m_sourcePolicyTypeHasBeenSet(false),
    m_startPositionHasBeenSet(false),
    m_endPositionHasBeenSet(false)
{
}

Statement::Statement(const XmlNode& xmlNode) : 
    m_sourcePolicyIdHasBeenSet(false),
    m_sourcePolicyTypeHasBeenSet(false),
    m_startPositionHasBeenSet(false),
    m_endPositionHasBeenSet(false)
{
  *this = xmlNode;
}

Statement& Statement::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode sourcePolicyIdNode = resultNode.FirstChild("SourcePolicyId");
    if(!sourcePolicyIdNode.IsNull())
    {
      m_sourcePolicyId = StringUtils::Trim(sourcePolicyIdNode.GetText().c_str());
      m_sourcePolicyIdHasBeenSet = true;
    }
    XmlNode sourcePolicyTypeNode = resultNode.FirstChild("SourcePolicyType");
    if(!sourcePolicyTypeNode.IsNull())
    {
      m_sourcePolicyType = PolicySourceTypeMapper::GetPolicySourceTypeForName(StringUtils::Trim(sourcePolicyTypeNode.GetText().c_str()).c_str());
      m_sourcePolicyTypeHasBeenSet = true;
    }
    XmlNode startPositionNode = resultNode.FirstChild("StartPosition");
    if(!startPositionNode.IsNull())
    {
      m_startPosition = startPositionNode;
      m_startPositionHasBeenSet = true;
    }
    XmlNode endPositionNode = resultNode.FirstChild("EndPosition");
    if(!endPositionNode.IsNull())
    {
      m_endPosition = endPositionNode;
      m_endPositionHasBeenSet = true;
    }
  }

  return *this;
}

void Statement::OutputToStream(Aws::OStream& oStream, const char* location, unsigned index, const char* locationValue) const
{
  if(m_sourcePolicyIdHasBeenSet)
  {
      oStream << location << index << locationValue << ".SourcePolicyId=" << StringUtils::URLEncode(m_sourcePolicyId.c_str()) << "&";
  }
  if(m_sourcePolicyTypeHasBeenSet)
  {
      oStream << location << index << locationValue << ".SourcePolicyType=" << PolicySourceTypeMapper::GetNameForPolicySourceType(m_sourcePolicyType) << "&";
  }
  if(m_startPositionHasBeenSet)
  {
      Aws::StringStream startPositionLocationAndMemberSs;
      startPositionLocationAndMemberSs << location << index << locationValue << ".StartPosition";
      m_startPosition.OutputToStream(oStream, startPositionLocationAndMemberSs.str().c_str());
  }
  if(m_endPositionHasBeenSet)
  {
      Aws::StringStream endPositionLocationAndMemberSs;
      endPositionLocationAndMemberSs << location << index << locationValue << ".EndPosition";
      m_endPosition.OutputToStream(oStream, endPositionLocationAndMemberSs.str().c_str());
  }
}

void Statement::OutputToStream(Aws::OStream& oStream, const char* location) const
{
  if(m_sourcePolicyIdHasBeenSet)
  {
      oStream << location << ".SourcePolicyId=" << StringUtils::URLEncode(m_sourcePolicyId.c_str()) << "&";
  }
  if(m_sourcePolicyTypeHasBeenSet)
  {
      oStream << location << ".SourcePolicyType=" << PolicySourceTypeMapper::GetNameForPolicySourceType(m_sourcePolicyType) << "&";
  }
  if(m_startPositionHasBeenSet)
  {
      Aws::String startPositionLocationAndMember(location);
      startPositionLocationAndMember += ".StartPosition";
      m_startPosition.OutputToStream(oStream, startPositionLocationAndMember.c_str());
  }
  if(m_endPositionHasBeenSet)
  {
      Aws::String endPositionLocationAndMember(location);
      endPositionLocationAndMember += ".EndPosition";
      m_endPosition.OutputToStream(oStream, endPositionLocationAndMember.c_str());
  }
}