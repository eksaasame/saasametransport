// service.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_TASK_SCHEDULER__
#define __MACHO_WINDOWS_TASK_SCHEDULER__

#include "boost\shared_ptr.hpp"
#include "common\exception_base.hpp"
#include "boost\date_time.hpp"
namespace macho{ namespace windows{

class task_scheduler{
public:
    typedef boost::shared_ptr<task_scheduler> ptr;
    
    class task{
    public:
        typedef boost::shared_ptr<task> ptr;
        typedef std::vector<ptr> vtr;
        virtual bool is_running() = 0;
        virtual bool is_scheduled() = 0;
        virtual DWORD exit_code() = 0;
        virtual HRESULT status() = 0;
        virtual bool run() = 0;
    };

    virtual task::ptr get_task(std::wstring name) = 0;
    virtual task::vtr get_tasks() = 0;
    virtual task::ptr create_task(std::wstring name, std::wstring application_path, std::wstring parameters) = 0;
    virtual bool      delete_task(std::wstring name) = 0;
    virtual bool      run_task(std::wstring name) = 0;
    virtual bool inline is_running_task(std::wstring name){
        DWORD exit_code;
        return is_running_task(name, exit_code);
    }
    virtual bool      is_running_task(std::wstring name, DWORD& exit_code) = 0;
    static task_scheduler::ptr get();
};
};//namespace windows
};//namespace macho

#ifndef MACHO_HEADER_ONLY

#include <memory>
#include <atlbase.h>
#include <atlcom.h>
#include <initguid.h>
#include <comdef.h>
#include <comutil.h>
#include <Wbemidl.h>
#include <windows.h>
#include <initguid.h>
#include <mstask.h>
#include <msterr.h>

#pragma comment(lib, "wbemuuid.lib")
using namespace macho;
using namespace macho::windows;

class windows_task : virtual public task_scheduler::task{
public:
    windows_task(ATL::CComPtr<ITask> &task) : _task(task){}
    virtual bool is_running(){
        return status() == SCHED_S_TASK_RUNNING;
    }
    virtual bool is_scheduled(){
        return status() != SCHED_S_TASK_NOT_SCHEDULED;
    }
    virtual DWORD exit_code(){
        DWORD dwExitCode;
        HRESULT hr = _task->GetExitCode(&dwExitCode);
        if (SUCCEEDED(hr))
            return dwExitCode;
        else
            return -1;
    }
    virtual HRESULT status(){
        HRESULT status;
        HRESULT hr = _task->GetStatus(&status);
        if (SUCCEEDED(hr))
            return status;
        else
            return S_FALSE;
    }
    virtual bool run(){
        return SUCCEEDED(_task->Run());
    }
private:
    ATL::CComPtr<ITask>  _task;
};

#define TASKS_TO_RETRIEVE                    5

class windows_task_scheduler : virtual public task_scheduler {
public:
    windows_task_scheduler(){
        HRESULT hr = S_OK;
        // Call CoInitialize to initialize the COM library and then CoCreateInstance to get the Task Scheduler object.
        hr = CoCreateInstance(CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void **)&_scheduler);
        //if (!SUCCEEDED(hr))
    }
    virtual task::vtr get_tasks(){
        task::vtr results;
        ATL::CComPtr<IEnumWorkItems> pIEnum = NULL;
        HRESULT hr = _scheduler->Enum(&pIEnum);
        if (!SUCCEEDED(hr)){}
        else{
            DWORD   dwFetchedTasks = 0;
            // Call IEnumWorkItems::Next to retrieve tasks. Note that this example tries to retrieve five tasks for each call.
            LPWSTR *lpwszNames;
            TCHAR   szEnumTaskName[MAX_PATH] = { 0 };
            while (SUCCEEDED(pIEnum->Next(TASKS_TO_RETRIEVE, &lpwszNames, &dwFetchedTasks)) && (dwFetchedTasks != 0)){
                // Process each task. Note that this example prints the name of each task to the screen.
                while (dwFetchedTasks){
                    memset(szEnumTaskName, 0, MAX_PATH * sizeof(TCHAR));
                    _stprintf_s(szEnumTaskName, MAX_PATH, _T("%s"), lpwszNames[--dwFetchedTasks]);
                    task::ptr t = get_task(szEnumTaskName);
                    if (t)
                        results.push_back(t);
                    CoTaskMemFree(lpwszNames[dwFetchedTasks]);
                }
                CoTaskMemFree(lpwszNames);
            }
        }
        return results;
    }
    virtual task::ptr get_task(std::wstring name){
        ATL::CComPtr<ITask>  pITask;
        HRESULT hr = _scheduler->Activate(name.c_str(), IID_ITask, (IUnknown**)&pITask);
        if (SUCCEEDED(hr)){
            return task_scheduler::task::ptr(new windows_task(pITask));
        }
        return NULL;
    }
    
    virtual bool      run_task(std::wstring name){
        task::ptr t = get_task(name);
        if (t) return t->run();
        else return false;
    }

    virtual bool      is_running_task(std::wstring name, DWORD& exit_code){
        task::ptr t = get_task(name);
        if (t) {
            if (t->is_running())
                return true;
            else
                exit_code = t->exit_code();
        }
        else{
            exit_code = -1;
        }
        return false;
    }

    virtual task::ptr create_task(std::wstring name, std::wstring application_path, std::wstring parameters){
        
        ATL::CComPtr<ITask>  pITask;
        HRESULT hr = _scheduler->NewWorkItem(name.c_str(), CLSID_CTask, IID_ITask, (IUnknown**)&pITask);
        if (SUCCEEDED(hr)){
            WORD            piNewTrigger;
            ATL::CComPtr<ITaskTrigger>  pITaskTrigger;
            TASK_TRIGGER    pTrigger;
            hr = pITask->CreateTrigger(&piNewTrigger, &pITaskTrigger);
            if (SUCCEEDED(hr)){
                // Define TASK_TRIGGER structure. Note that wBeginDay, wBeginMonth, and wBeginYear must be set to a valid day, month, and year respectively.
                // Add code to set trigger structure?
                ZeroMemory(&pTrigger, sizeof(TASK_TRIGGER));
                pTrigger.wBeginDay = 1;                 // Required
                pTrigger.wBeginMonth = 1;                 // Required
                pTrigger.wBeginYear = 1999;              // Required
                pTrigger.cbTriggerSize = sizeof(TASK_TRIGGER);
                pTrigger.TriggerType = TASK_EVENT_TRIGGER_AT_SYSTEMSTART;
                if (!SUCCEEDED(hr = pITaskTrigger->SetTrigger(&pTrigger))){
                }
                else if (!SUCCEEDED(hr = pITask->SetApplicationName(application_path.c_str()))){
                }
                else if (!SUCCEEDED(hr = pITask->SetParameters(parameters.c_str()))){
                }
                else if (!SUCCEEDED(hr = pITask->SetAccountInformation(L"", NULL))){
                }
                else{
                    // Call IPersistFile::Save to save trigger to disk.
                    ATL::CComPtr<IPersistFile> pIPersistFile;
                    if (SUCCEEDED(hr = pITask->QueryInterface(IID_IPersistFile, (void **)&pIPersistFile))){
                        if (SUCCEEDED(hr = pIPersistFile->Save(NULL, TRUE)))
                            return task::ptr(new windows_task(pITask));
                    }
                }
            }
        }
        return NULL;
    }
    virtual bool      delete_task(std::wstring name){
        return SUCCEEDED(_scheduler->Delete(name.c_str()));
    }
    virtual ~windows_task_scheduler(){}
private:
    ATL::CComPtr<ITaskScheduler>  _scheduler;
};

task_scheduler::ptr task_scheduler::get(){
    return task_scheduler::ptr(new windows_task_scheduler());
}

#endif
#endif