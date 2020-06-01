// LargePageTes.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <ntstatus.h>
#include <windows.h>
#include <ntsecapi.h>
#include <Sddl.h>

bool InitLsaString(
    PLSA_UNICODE_STRING pLsaString,
    LPCWSTR pwszString);

std::string GetErrorAsString(DWORD errorMessageID)
{
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

#define TARGET_SYSTEM_NAME L"DESKTOP-J3TTSAD"
//#define STATUS_SUCCESS 0
LSA_HANDLE GetPolicyHandle()
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR SystemName[] = TARGET_SYSTEM_NAME;
    USHORT SystemNameLength;
    LSA_UNICODE_STRING lusSystemName;
    NTSTATUS ntsResult;
    LSA_HANDLE lsahPolicyHandle;

    // Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    //Initialize an LSA_UNICODE_STRING to the server name.
    SystemNameLength = wcslen(SystemName);
    lusSystemName.Buffer = SystemName;
    lusSystemName.Length = SystemNameLength * sizeof(WCHAR);
    lusSystemName.MaximumLength = (SystemNameLength + 1) * sizeof(WCHAR);

    // Get a handle to the Policy object.
    ntsResult = LsaOpenPolicy(
        &lusSystemName,    //Name of the target system.
        &ObjectAttributes, //Object attributes.
        POLICY_ALL_ACCESS, //Desired access permissions.
        &lsahPolicyHandle  //Receives the policy handle.
    );

    if (ntsResult != STATUS_SUCCESS)
    {
        // An error occurred. Display it as a win32 error code.
        auto winError = LsaNtStatusToWinError(ntsResult);
        wprintf(L"OpenPolicy returned %lu\n", winError);
        std::cout << "Error message: " << GetErrorAsString(winError) << std::endl;
        return NULL;
    }
    return lsahPolicyHandle;
}

bool InitLsaString(
    PLSA_UNICODE_STRING pLsaString,
    LPCWSTR pwszString
)
{
    DWORD dwLen = 0;

    if (NULL == pLsaString)
        return FALSE;

    if (NULL != pwszString)
    {
        dwLen = wcslen(pwszString);
        if (dwLen > 0x7ffe)   // String is too large
            return FALSE;
    }

    // Store the string.
    pLsaString->Buffer = (WCHAR*)pwszString;
    pLsaString->Length = (USHORT)dwLen * sizeof(WCHAR);
    pLsaString->MaximumLength = (USHORT)(dwLen + 1) * sizeof(WCHAR);

    return TRUE;
}

void AddPrivileges(PSID AccountSID, LSA_HANDLE PolicyHandle)
{
    LSA_UNICODE_STRING lucPrivilege;
    NTSTATUS ntsResult;

    // Create an LSA_UNICODE_STRING for the privilege names.
    if (!InitLsaString(&lucPrivilege, L"SeLockMemoryPrivilege"))
    {
        wprintf(L"Failed InitLsaString\n");
        return;
    }

    ntsResult = LsaAddAccountRights(
        PolicyHandle,  // An open policy handle.
        AccountSID,    // The target SID.
        &lucPrivilege, // The privileges.
        1              // Number of privileges.
    );
    if (ntsResult == STATUS_SUCCESS)
    {
        wprintf(L"Privilege added.\n");
    }
    else
    {
        wprintf(L"Privilege was not added - %lu \n",
            LsaNtStatusToWinError(ntsResult));
        std::cout << "Error message: " << GetErrorAsString(LsaNtStatusToWinError(ntsResult)) << std::endl;
    }
}

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
        printf("AdjustTokenPrivileges error: %u\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
        printf("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}


int main()
{
    /*
    Get Sid from runnign powershell commands: 
        $objUser = New-Object System.Security.Principal.NTAccount("AHMED")
        $strSID = $objUser.Translate([System.Security.Principal.SecurityIdentifier])
        $strSID.Value
    There is probably a Win32 way, but that seems easier
    */
    int x = 5;
    std::cout << &x << std::endl;
    LPCSTR strSid = "S-1-5-21-1169946419-2737151734-878301561-1001";
    PSID sid;
    ConvertStringSidToSidA(strSid, &sid);
    AddPrivileges(sid, GetPolicyHandle());
}
