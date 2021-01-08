#include <iostream>
#include <ntstatus.h>
#include <windows.h>
#include <ntsecapi.h>
#include <Sddl.h>

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

LSA_HANDLE GetPolicyHandle(WCHAR* SystemName)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
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


int main()
{
    /*
    Get Sid from runnign powershell commands: 
        $objUser = New-Object System.Security.Principal.NTAccount("AHMED")
        $strSID = $objUser.Translate([System.Security.Principal.SecurityIdentifier])
        $strSID.Value
    There is probably a way through Win32 API, but that seems easier
    */
    LPCSTR strSid = "S-1-5-21-1169947419-2731231734-878301561-1001";
    PSID sid;
    WCHAR SystemName[] = L"DESKTOP-J3TTSAD";

    ConvertStringSidToSidA(strSid, &sid);
    AddPrivileges(sid, GetPolicyHandle(SystemName));
}
