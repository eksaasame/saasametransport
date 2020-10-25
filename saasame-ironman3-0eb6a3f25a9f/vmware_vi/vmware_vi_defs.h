/************************************************************************/
/* Copyright 2001-2013 FalconStor Software, Inc. All rights reserved.   */
/* FalconStor Software, Inc. Proprietary and Confidential               */
/************************************************************************/

#ifndef VMWARE_VI_DEFS_H
#define VMWARE_VI_DEFS_H

#ifdef VMWARE_VI_EXPORTS
#define VMWARE_VI_API __declspec(dllexport)
#else
#define VMWARE_VI_API __declspec(dllimport)
#endif

#define SOAP_FMAC1      VMWARE_VI_API
//#define SOAP_FMAC3      VMWARE_VI_API
#define SOAP_FMAC5      VMWARE_VI_API
#define SOAP_CMAC       VMWARE_VI_API

#endif
