﻿/*
 * PROJECT:   Mouri Internal Library Essentials
 * FILE:      Mile.Windows.Platform.cpp
 * PURPOSE:   Mouri Internal Library Essentials Base Implementation for Windows
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include "Mile.Windows.Platform.h"

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HANDLE WINAPI MileGetProcessHeap()
{
    return ::GetProcessHeap();
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileHeapAlloc(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ SIZE_T dwBytes,
    _Out_ LPVOID* lpNewMem)
{
    *lpNewMem = ::HeapAlloc(hHeap, dwFlags, dwBytes);
    return *lpNewMem ? S_OK : ::HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileHeapReAlloc(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ LPVOID lpMem,
    _In_ SIZE_T dwBytes,
    _Out_ LPVOID* lpNewMem)
{
    *lpNewMem = ::HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
    return *lpNewMem ? S_OK : ::HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileHeapFree(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ LPVOID lpMem)
{
    if (!::HeapFree(hHeap, dwFlags, lpMem))
    {
        return ::HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileAdjustTokenPrivileges(
    _In_ HANDLE TokenHandle,
    _In_ BOOL DisableAllPrivileges,
    _In_opt_ PTOKEN_PRIVILEGES NewState,
    _In_ DWORD BufferLength,
    _Out_opt_ PTOKEN_PRIVILEGES PreviousState,
    _Out_opt_ PDWORD ReturnLength)
{
    ::AdjustTokenPrivileges(
        TokenHandle,
        DisableAllPrivileges,
        NewState,
        BufferLength,
        PreviousState,
        ReturnLength);

    return ::HRESULT_FROM_WIN32(::GetLastError());
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileGetTokenInformation(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_opt_ LPVOID TokenInformation,
    _In_ DWORD TokenInformationLength,
    _Out_ PDWORD ReturnLength)
{
    if (!::GetTokenInformation(
        TokenHandle,
        TokenInformationClass,
        TokenInformation,
        TokenInformationLength,
        ReturnLength))
    {
        return ::HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileSetTokenInformation(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _In_ LPVOID TokenInformation,
    _In_ DWORD TokenInformationLength)
{
    if (!::SetTokenInformation(
        TokenHandle,
        TokenInformationClass,
        TokenInformation,
        TokenInformationLength))
    {
        return ::HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileGetPrivilegeValue(
    _In_ LPCWSTR Name,
    _Out_ PLUID Value)
{
    if (!::LookupPrivilegeValueW(nullptr, Name, Value))
    {
        return ::HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C HRESULT WINAPI MileStartService(
    _In_ LPCWSTR ServiceName,
    _Out_ LPSERVICE_STATUS_PROCESS ServiceStatus)
{
    HRESULT hr = E_INVALIDARG;

    if (ServiceStatus && ServiceName)
    {
        hr = S_OK;

        ::memset(ServiceStatus, 0, sizeof(LPSERVICE_STATUS_PROCESS));

        SC_HANDLE hSCM = ::OpenSCManagerW(
            nullptr, nullptr, SC_MANAGER_CONNECT);
        if (hSCM)
        {
            SC_HANDLE hService = ::OpenServiceW(
                hSCM, ServiceName, SERVICE_QUERY_STATUS | SERVICE_START);
            if (hService)
            {
                DWORD nBytesNeeded = 0;
                DWORD nOldCheckPoint = 0;
                ULONGLONG nLastTick = 0;
                bool bStartServiceWCalled = false;

                while (::QueryServiceStatusEx(
                    hService,
                    SC_STATUS_PROCESS_INFO,
                    reinterpret_cast<LPBYTE>(ServiceStatus),
                    sizeof(SERVICE_STATUS_PROCESS),
                    &nBytesNeeded))
                {
                    if (SERVICE_STOPPED == ServiceStatus->dwCurrentState)
                    {
                        // Failed if the service had stopped again.
                        if (bStartServiceWCalled)
                        {
                            hr = S_FALSE;
                            break;
                        }

                        if (!::StartServiceW(hService, 0, nullptr))
                        {
                            hr = ::HRESULT_FROM_WIN32(::GetLastError());
                            break;
                        }

                        bStartServiceWCalled = true;
                    }
                    else if (
                        SERVICE_STOP_PENDING
                        == ServiceStatus->dwCurrentState ||
                        SERVICE_START_PENDING
                        == ServiceStatus->dwCurrentState)
                    {
                        ULONGLONG nCurrentTick = ::GetTickCount64();

                        if (!nLastTick)
                        {
                            nLastTick = nCurrentTick;
                            nOldCheckPoint = ServiceStatus->dwCheckPoint;

                            // Same as the .Net System.ServiceProcess, wait
                            // 250ms.
                            ::SleepEx(250, FALSE);
                        }
                        else
                        {
                            // Check the timeout if the checkpoint is not
                            // increased.
                            if (ServiceStatus->dwCheckPoint
                                <= nOldCheckPoint)
                            {
                                ULONGLONG nDiff = nCurrentTick - nLastTick;
                                if (nDiff > ServiceStatus->dwWaitHint)
                                {
                                    hr = ::HRESULT_FROM_WIN32(
                                        ERROR_TIMEOUT);
                                    break;
                                }
                            }

                            // Continue looping.
                            nLastTick = 0;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                ::CloseServiceHandle(hService);
            }
            else
            {
                hr = ::HRESULT_FROM_WIN32(::GetLastError());
            }

            ::CloseServiceHandle(hSCM);
        }
        else
        {
            hr = ::HRESULT_FROM_WIN32(::GetLastError());
        }
    }

    return hr;
}

/**
 * @remark You can read the definition for this function in
 *         "Mile.Windows.Platform.h".
 */
EXTERN_C ULONGLONG WINAPI MileGetTickCount()
{
    LARGE_INTEGER Frequency, PerformanceCount;

    if (::QueryPerformanceFrequency(&Frequency))
    {
        if (::QueryPerformanceCounter(&PerformanceCount))
        {
            return (PerformanceCount.QuadPart * 1000 / Frequency.QuadPart);
        }
    }

    return ::GetTickCount64();
}