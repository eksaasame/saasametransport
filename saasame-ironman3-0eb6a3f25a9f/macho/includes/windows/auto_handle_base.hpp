// auto_handle_base.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_AUTO_HANDLE_BASE__
#define __MACHO_WINDOWS_AUTO_HANDLE_BASE__

#include "..\config\config.hpp"

namespace macho{

namespace windows{

///////////////////////////////////////////////////////////////////////////
// Data type representing the address of the object's cleanup function.
// I used UINT_PTR so that this class works properly in 64-bit Windows.
typedef VOID (WINAPI* PFNAUTOHANDLEBASE)(UINT_PTR);

///////////////////////////////////////////////////////////////////////////

// Each template instantiation requires a data type, address of cleanup
// function, and a value that indicates an invalid value.
template<class TYPE, PFNAUTOHANDLEBASE pfn, UINT_PTR tInvalid = NULL>
class auto_handle_base {
public:
   // Default constructor assumes an invalid value (nothing to clean up)
   auto_handle_base() { m_t = tInvalid; }

   // This constructor sets the value to the specified value
   auto_handle_base(TYPE t) : m_t((UINT_PTR) t) { }

   // The destructor performs the cleanup.
   ~auto_handle_base() { cleanup(); }

   // Helper methods to tell if the value represents a valid object or not.
   bool is_valid() { return(m_t != tInvalid); }
   bool is_invalid() { return(!is_valid()); }

   // Reassigning the object forces the current object to be cleaned up.
   TYPE operator=(TYPE t) {
      cleanup();
      m_t = (UINT_PTR) t;
      return(*this);
   }

   // Returns the value (supports both 32-bit and 64-bit Windows).
   operator TYPE() {
      // If TYPE is a 32-bit value, cast m_t to 32-bit TYPE
      // If TYPE is a 64-bit value, case m_t to 64-bit TYPE
       return((sizeof(TYPE) == 4) ? (TYPE) PtrToUint( (const void *) m_t) : (TYPE) m_t);
   }

   // Clean up the object if the value represents a valid object
   void cleanup() {
      if (is_valid()) {
         // In 64-bit Windows, all parameters are 64-bits,
         // so no casting is required
         pfn(m_t);         // Close the object.
         m_t = tInvalid;   // We no longer represent a valid object.
      }
   }

private:
   UINT_PTR m_t;           // The member representing the object
};


/////////////////////////////////////////////////////////////////////////
// Macros to make it easier to declare instances of the template
// class for specific data types.

#define MAKE_AUTO_HANDLE_CLASS(className, tData, pfnAutoHandle) \
   typedef auto_handle_base<tData, (PFNAUTOHANDLEBASE) pfnAutoHandle> className;

#define MAKE_AUTO_HANDLE_CLASS_EX(className, tData, pfnAutoHandle, tInvalid) \
   typedef auto_handle_base<tData, (PFNAUTOHANDLEBASE) pfnAutoHandle, \
   (INT_PTR) tInvalid> className;

// Instances of the template C++ class for common data types.
MAKE_AUTO_HANDLE_CLASS(auto_handle, HANDLE, CloseHandle);
MAKE_AUTO_HANDLE_CLASS(auto_local_heap_handle, HLOCAL, LocalFree);
MAKE_AUTO_HANDLE_CLASS(auto_global_heap_handle, HGLOBAL, GlobalFree);
MAKE_AUTO_HANDLE_CLASS(auto_reg_key_handle, HKEY, RegCloseKey);
MAKE_AUTO_HANDLE_CLASS(auto_service_handle, SC_HANDLE, CloseServiceHandle);
MAKE_AUTO_HANDLE_CLASS(auto_window_station_handle, HWINSTA, CloseWindowStation);
MAKE_AUTO_HANDLE_CLASS(auto_desktop_handle, HDESK, CloseDesktop);
MAKE_AUTO_HANDLE_CLASS(auto_view_of_file_handle, PVOID, UnmapViewOfFile);
MAKE_AUTO_HANDLE_CLASS(auto_loaded_library_handle, HMODULE, FreeLibrary);

MAKE_AUTO_HANDLE_CLASS_EX(auto_file_handle, HANDLE, CloseHandle, INVALID_HANDLE_VALUE);
MAKE_AUTO_HANDLE_CLASS_EX(auto_find_volume_handle, HANDLE, FindVolumeClose, INVALID_HANDLE_VALUE);
MAKE_AUTO_HANDLE_CLASS_EX(auto_find_file_handle, HANDLE, FindClose, INVALID_HANDLE_VALUE);

/////////////////////////////////////////////////////////////////////////

// Special class for releasing a reserved region.
// Special class is required because VirtualFree requires 3 parameters
class auto_release_region {
public:
   auto_release_region(PVOID pv = NULL) : m_pv(pv) { }
   ~auto_release_region() { cleanup(); }

   PVOID operator=(PVOID pv) {
      cleanup();
      m_pv = pv;
      return(m_pv);
   }
   operator PVOID() { return(m_pv); }
   void cleanup() {
      if (m_pv != NULL) {
         VirtualFree(m_pv, 0, MEM_RELEASE);
         m_pv = NULL;
      }
   }

private:
   PVOID m_pv;
};

/////////////////////////////////////////////////////////////////////////

// Special class for releasing a reserved region.
// Special class is required because HeapFree requires 3 parameters
class auto_heap_free {
public:
   auto_heap_free(PVOID pv = NULL, HANDLE hHeap = GetProcessHeap())
      : m_pv(pv), m_hHeap(hHeap) { }
   ~auto_heap_free() { cleanup(); }

   PVOID operator=(PVOID pv) {
      cleanup();
      m_pv = pv;
      return(m_pv);
   }
   operator PVOID() { return(m_pv); }
   void cleanup() {
      if (m_pv != NULL) {
         HeapFree(m_hHeap, 0, m_pv);
         m_pv = NULL;
      }
   }

private:
   HANDLE m_hHeap;
   PVOID m_pv;
};

};//namespace windows
};//namespace macho

#endif