#include "SetLargePagePrivilege.h"
#include <Windows.h>
#include <iostream>

BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup 
        &luid))        // receives LUID of privilege
    {
        printf("LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
        std::cout << "AdjustTokenPrivileges error: " << GetLastError() << std::endl;;
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
        std::cout << "The token does not have the specified privilege." << std::endl;
        return FALSE;
    }

    return TRUE;
}


void SetLargePagePrivilege()
{
    HANDLE hToken;
    auto res = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    if (!res)
    {
        std::cout << "Failed to get token" << std::endl;
    }

    res = SetPrivilege(hToken, L"SeLockMemoryPrivilege", TRUE);
    if (!res)
    {
        std::cout << "Failed to set privilege" << std::endl;
    }
}