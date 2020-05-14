---
title: "Large Pages: How and When"
date: "2019-08-06T12:00:32.169Z"
description: VTune is an instruction level profiler. I want to explore if it can detect cache unfriendliness, and fix it.
seotitle: Profiling Processor Cache Misses with VTune
socialPic: column_major.PNG
---

The Fantastic Large-Pages and How to Define Them

- Intro: 
  - learnt about it from someone introducing it to code base
  - recently I tried to apply it but it didn't give me the effects I needed, I thought I would dig deeper
- What are they?
  - What is a Page
  - What is a Large-Page
  - Why are Large-Pages Interesting
  - Bonus: Huge Pages
-  The How
   -  Requirements: SeLockMemoryPrivilege 
      -  Why do we need to lock? 
      -  blog by Old New Thing
   -  First, add the privilege
      -  On a server? Probably run this code automatically
      -  Need to be done once
   -  Acquire Privlege before use
      -  Needs to be done by admin as well
   -  Finally, use the `MEM_LARGE_PAGES` flag during allocation
   -  You cannot see it as a working set - but it's allocated! use commit size or vmmap to see it 
-  Give me the numbers
   -  Experiment: How many memory access per second
      -  Run over the setup
         -  Note on how decreasing what's done in this thread (making the program meomory bound) increased speed of large pages by a lot, but normal-pages by not so much
      -  Video
      -  Graph for increasing memory size vs speed (if interesting)
      -  VTune: emphasis on memory accesses
-  Last notes: 
   -  When to use it: 
      -  Big allocations, especially ones that we'd access frequently
      -  downside: it's non-pageable, so you gotta know well-ahead of time that you can affodd that much allocation in physical memory 
   -  What happened in my case, is that it turned out that chunk of memory was already allocated as a LargePage, passed in by someone else. 

Large Pages

- What's a page: 
  - A page is the unit at which OS handles physical memory, it's the level of translation from Virtual to Physical address. 
- Large Pages conservatively uses the cache of CPU translation (look-aside buffer (TLB)) allowing for faster page translation (higher probability of finding the physical address in the cache)
  - A large page is usually 1000x native page size
  - Usually, every page translation requires 3 lookups: PDPT > PD > PT > page N (in x86, in x64 there are 4, as there's "page map level 4")
  - This comes handy in memory-bound applications, where there is too much memory accessing happening
- Particularly good on 64-bit Windows
  - Where there's usually more addressable memory available and higher possiblity of acquiring large pages when requested
- Huge-Pages are also possible! These are 1GB each, no need for special parameters though, Windows Memory Manager decides internally whether to grant Large or Huge pages based on allocation size and meomory availability
- Usage:
  - Obtain `SeLockMemoryPrivilege` privilege
    - By calling [AdjustTokenPrivileges](https://docs.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-adjusttokenprivileges?redirectedfrom=MSDN)
      - It only enables or disables existing privileges, but does not add new ones
      - discover already enabled ones by calling [GetTokenInformation](https://docs.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-gettokeninformation)
      - For some reason, it is not straightforward in the docs how to get that token ...
        - You get it by calling `OpenProcessToken(GetCurrentProcess(), ...)`, make sure to specify `TOKEN_ADJUST_PRIVILEGES` as well 
      - This proved pretty hard to do as my "normal" Windows home account didn't have the privileges*
  - Get minimum page size by calling [GetLargePageMinimum ](https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-getlargepageminimum?redirectedfrom=MSDN)
    - Typically 2MB, small page is 4kb. i.e. one large page can contain around 512 pages
  - Use `MEM_LARGE_PAGES` flag when calling VirtualAlloc, e.g.: `VirtualAlloc(NULL, pageSize * N_PAGES_TO_ALLOC, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE)`
- Disclaimer: 
  - Can be hard to obtain if system has been running for some time, due to physical space fragmentation
    - Not only do you need continguous 2MB, but it has to be on a large page boundary (e.g. 0-511 or 512-1023)
    - should do it at startup
  - non-pageable! [Ramyond Chen](https://devblogs.microsoft.com/oldnewthing/20110128-00/?p=11643) goes into detail of why Windows didn't bother supporting this, 
  

* How to give yourself privileges: 

An easy way to do it is through Group Policy (for Home users like me, it needs to be [turned on first](https://superuser.com/a/1229992)). However, I did it the hard way just for kicks: 

1. Get your windows SID by running: 
```
$objUser = New-Object System.Security.Principal.NTAccount("AHMED")
$strSID = $objUser.Translate([System.Security.Principal.SecurityIdentifier])
$strSID.Value
```
2. Call `ConvertStringSidToSidA` to convert that to PSID that Win32 API regonizes
3. Call `AddPrivileges(sid, GetPolicyHandle());`, where [GetPolicyHandle](https://docs.microsoft.com/en-us/windows/win32/secmgmt/opening-a-policy-object-handle) and [AddPrivileges](https://docs.microsoft.com/en-us/windows/win32/secmgmt/managing-account-permissions) functions are provided by the docs

Code is hosted on [github]()


experiment: 
- Create 10 GB of memory garbage, see how many random lookups/sec can be done
- Same with Large page, compare random lookups/sec