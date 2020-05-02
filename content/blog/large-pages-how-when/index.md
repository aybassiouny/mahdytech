---
title: "Large Pages: How and When"
date: "2019-08-06T12:00:32.169Z"
description: VTune is an instruction level profiler. I want to explore if it can detect cache unfriendliness, and fix it.
seotitle: Profiling Processor Cache Misses with VTune
socialPic: column_major.PNG
---

Large Pages

- Conservatively uses CPU translation look-aside buffer (TLB) allowing for faster page translation 
- Uses larger cpu-buffers, allowing for faster page translation, and in result, memory accesses in frequently used memory 
  - This is handy because 
  - usually 1000 times native page sizes
- Particularly good on 64-bit Windows
    - Why? 
- Usage:
    - Obtain `SeLockMemoryPrivilege` privilege
      - By calling [AdjustTokenPrivileges](https://docs.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-adjusttokenprivileges?redirectedfrom=MSDN)
        - It only enables or disables existing privileges, but does not add new ones
        - discover already enabled ones by calling [GetTokenInformation](https://docs.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-gettokeninformation)
        - For some reason, it is not straightforward in the docs how to get that token ...
          - You get it by calling `OpenProcessToken(GetCurrentProcess(), ...)`, make sure to specify `TOKEN_ADJUST_PRIVILEGES` as well 
        - This proved pretty hard to do as my "normal" account didn't have the privileges ...
          - 
      - Get minimum page size by calling [GetLargePageMinimum ](https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-getlargepageminimum?redirectedfrom=MSDN)
        - Typically 2MB, small page is 4kb. i.e. one large page can contain around 512 pages
      - Use `MEM_LARGE_PAGES` when calling VirtualAlloc
- Disclaimer: 
  - Can be hard to obtain if system has been running for some time, due to physical space fragmentation
    - Not only do you need continguous 2MB, but it has to be on a large page boundary (e.g. 0-511 or 512-1023)
    - should do it at startup
  - non-pageable! 
  