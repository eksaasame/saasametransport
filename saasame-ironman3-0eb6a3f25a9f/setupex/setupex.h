// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SETUPEX_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SETUPEX_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef SETUPEX_EXPORTS
#define SETUPEX_API __declspec(dllexport)
#else
#define SETUPEX_API __declspec(dllimport)
#endif

SETUPEX_API UINT Install(MSIHANDLE hInstall);
SETUPEX_API UINT Maintenance(MSIHANDLE hInstall);
SETUPEX_API UINT Uninstall(MSIHANDLE hInstall);
SETUPEX_API UINT Upgrade(MSIHANDLE hInstall);
SETUPEX_API UINT StartDB(MSIHANDLE hInstall);
SETUPEX_API UINT SetNetworksMTU(MSIHANDLE hInstall);
SETUPEX_API UINT UninstallTransport(MSIHANDLE hInstall);
SETUPEX_API UINT UpgradeTransport(MSIHANDLE hInstall);

