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
#pragma once
#include <aws/iam/IAM_EXPORTS.h>
#include <aws/iam/IAMRequest.h>
#include <aws/core/utils/memory/stl/AWSString.h>

namespace Aws
{
namespace IAM
{
namespace Model
{

  /**
   */
  class AWS_IAM_API CreateSAMLProviderRequest : public IAMRequest
  {
  public:
    CreateSAMLProviderRequest();
    Aws::String SerializePayload() const override;

    /**
     * <p>An XML document generated by an identity provider (IdP) that supports SAML
     * 2.0. The document includes the issuer's name, expiration information, and keys
     * that can be used to validate the SAML authentication response (assertions) that
     * are received from the IdP. You must generate the metadata document using the
     * identity management software that is used as your organization's IdP. </p>
     * <p>For more information, see <a
     * href="http://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_providers_saml.html">About
     * SAML 2.0-based Federation</a> in the <i>IAM User Guide</i></p>
     */
    inline const Aws::String& GetSAMLMetadataDocument() const{ return m_sAMLMetadataDocument; }

    /**
     * <p>An XML document generated by an identity provider (IdP) that supports SAML
     * 2.0. The document includes the issuer's name, expiration information, and keys
     * that can be used to validate the SAML authentication response (assertions) that
     * are received from the IdP. You must generate the metadata document using the
     * identity management software that is used as your organization's IdP. </p>
     * <p>For more information, see <a
     * href="http://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_providers_saml.html">About
     * SAML 2.0-based Federation</a> in the <i>IAM User Guide</i></p>
     */
    inline void SetSAMLMetadataDocument(const Aws::String& value) { m_sAMLMetadataDocumentHasBeenSet = true; m_sAMLMetadataDocument = value; }

    /**
     * <p>An XML document generated by an identity provider (IdP) that supports SAML
     * 2.0. The document includes the issuer's name, expiration information, and keys
     * that can be used to validate the SAML authentication response (assertions) that
     * are received from the IdP. You must generate the metadata document using the
     * identity management software that is used as your organization's IdP. </p>
     * <p>For more information, see <a
     * href="http://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_providers_saml.html">About
     * SAML 2.0-based Federation</a> in the <i>IAM User Guide</i></p>
     */
    inline void SetSAMLMetadataDocument(Aws::String&& value) { m_sAMLMetadataDocumentHasBeenSet = true; m_sAMLMetadataDocument = value; }

    /**
     * <p>An XML document generated by an identity provider (IdP) that supports SAML
     * 2.0. The document includes the issuer's name, expiration information, and keys
     * that can be used to validate the SAML authentication response (assertions) that
     * are received from the IdP. You must generate the metadata document using the
     * identity management software that is used as your organization's IdP. </p>
     * <p>For more information, see <a
     * href="http://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_providers_saml.html">About
     * SAML 2.0-based Federation</a> in the <i>IAM User Guide</i></p>
     */
    inline void SetSAMLMetadataDocument(const char* value) { m_sAMLMetadataDocumentHasBeenSet = true; m_sAMLMetadataDocument.assign(value); }

    /**
     * <p>An XML document generated by an identity provider (IdP) that supports SAML
     * 2.0. The document includes the issuer's name, expiration information, and keys
     * that can be used to validate the SAML authentication response (assertions) that
     * are received from the IdP. You must generate the metadata document using the
     * identity management software that is used as your organization's IdP. </p>
     * <p>For more information, see <a
     * href="http://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_providers_saml.html">About
     * SAML 2.0-based Federation</a> in the <i>IAM User Guide</i></p>
     */
    inline CreateSAMLProviderRequest& WithSAMLMetadataDocument(const Aws::String& value) { SetSAMLMetadataDocument(value); return *this;}

    /**
     * <p>An XML document generated by an identity provider (IdP) that supports SAML
     * 2.0. The document includes the issuer's name, expiration information, and keys
     * that can be used to validate the SAML authentication response (assertions) that
     * are received from the IdP. You must generate the metadata document using the
     * identity management software that is used as your organization's IdP. </p>
     * <p>For more information, see <a
     * href="http://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_providers_saml.html">About
     * SAML 2.0-based Federation</a> in the <i>IAM User Guide</i></p>
     */
    inline CreateSAMLProviderRequest& WithSAMLMetadataDocument(Aws::String&& value) { SetSAMLMetadataDocument(value); return *this;}

    /**
     * <p>An XML document generated by an identity provider (IdP) that supports SAML
     * 2.0. The document includes the issuer's name, expiration information, and keys
     * that can be used to validate the SAML authentication response (assertions) that
     * are received from the IdP. You must generate the metadata document using the
     * identity management software that is used as your organization's IdP. </p>
     * <p>For more information, see <a
     * href="http://docs.aws.amazon.com/IAM/latest/UserGuide/id_roles_providers_saml.html">About
     * SAML 2.0-based Federation</a> in the <i>IAM User Guide</i></p>
     */
    inline CreateSAMLProviderRequest& WithSAMLMetadataDocument(const char* value) { SetSAMLMetadataDocument(value); return *this;}

    /**
     * <p>The name of the provider to create.</p>
     */
    inline const Aws::String& GetName() const{ return m_name; }

    /**
     * <p>The name of the provider to create.</p>
     */
    inline void SetName(const Aws::String& value) { m_nameHasBeenSet = true; m_name = value; }

    /**
     * <p>The name of the provider to create.</p>
     */
    inline void SetName(Aws::String&& value) { m_nameHasBeenSet = true; m_name = value; }

    /**
     * <p>The name of the provider to create.</p>
     */
    inline void SetName(const char* value) { m_nameHasBeenSet = true; m_name.assign(value); }

    /**
     * <p>The name of the provider to create.</p>
     */
    inline CreateSAMLProviderRequest& WithName(const Aws::String& value) { SetName(value); return *this;}

    /**
     * <p>The name of the provider to create.</p>
     */
    inline CreateSAMLProviderRequest& WithName(Aws::String&& value) { SetName(value); return *this;}

    /**
     * <p>The name of the provider to create.</p>
     */
    inline CreateSAMLProviderRequest& WithName(const char* value) { SetName(value); return *this;}

  private:
    Aws::String m_sAMLMetadataDocument;
    bool m_sAMLMetadataDocumentHasBeenSet;
    Aws::String m_name;
    bool m_nameHasBeenSet;
  };

} // namespace Model
} // namespace IAM
} // namespace Aws
