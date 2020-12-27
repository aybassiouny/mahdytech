---
title: "Large-Pages in Windows: The Why & the How"
date: "2020-07-11T12:00:32.169Z"
description: Large-Pages
seotitle: Large-Pages
socialPic: Rammap.PNG
---

Through the years, I have stumbled onto the concept "Large-Pages" several times - mainly seeing others use them in our codebase - but I have not gone much further than exploring the wikipedia page. I knew they make lookups in large meomory chunks fast, but that's about it. Until recently, when I added them to some app and they didn't give me any perf gains, that I realized I don't know what they are, and I can't really debug any further. Unless I go and lear more! 

# The What

To start things off, what is a page? According to [Wikipedia](https://en.wikipedia.org/wiki/Page_(computer_memory)): 

```
[A page]  is the smallest unit of data for memory management in
a virtual memory operating system.
```

The way I like to think about it, a page of chunk of contiguous memory, a big array. The kernel memory manager uses the page as its smallest unit to manage. 

For example, when we lookup a memory address, the one thing that decides if we do a hard fault or not, is whether the page containing that address is in physical memory or not. 

A page is of a fixed size. In Windows, that's usually about 4 KB. A `Large-Page` is, as the name suggests, of bigger size, usually 2 MB. So why is it better or faster? To understand that, one needs to first understand how address translation happens. 

When a program mentions a memory address `0x00000085C92FFBE4`, the kernel needs to locate its page first. In order to do this there are two familier ways:

- Do the work, which involves multiple lookups in Page Table
- Look it up from a cache! (in this case the CPU's [TLB](https://www.geeksforgeeks.org/whats-difference-between-cpu-cache-and-tlb/) cache)

Large Pages are much more cache friendly than normal sized pages - because they are large! Hence, there are fewer of them, and are easier to cache.

<Graphic demonstrating this> 

#### Bonus: Huge Pages

You read that right -  size matters when it comes to pages, and Kernel allows allocating Huge Pages (that are about 1GB), however there is no special config for it. The OS will automatically use one Huge Page if allocation size is enough, and other conditions are satisified - as we'll discuss in a bit. 

## The How

I didn't find it straightforward to setup Large-Pages, especially on my Windows Home setup. Let's go through the steps: 

### 1. Add User Privilege `SeLockMemoryPrivilege` 

Large-Pages [cannot be paged out](https://devblogs.microsoft.com/oldnewthing/20110128-00/?p=11643)! In other words, they always stay in the physical memory. Hence, in order to allocate such pages, user needs to be have `SeLockMemoryPrivilege` Privilege. Note that even an admin account might not have that privilege. Good news is, this needs to be done once, then all allocations benefit. 

One way to give yourself the is through Group Policy (for Home users like me, it needs to be [turned on first](https://superuser.com/a/1229992)). However, I did it the hard way just for kicks: 

1. Get your windows SID by running: 
```
$objUser = New-Object System.Security.Principal.NTAccount("AHMED")
$strSID = $objUser.Translate([System.Security.Principal.SecurityIdentifier])
$strSID.Value
```
2. Call `ConvertStringSidToSidA` to convert that to PSID that Win32 API regonizes
3. Call `AddPrivileges(sid, GetPolicyHandle());`, where [GetPolicyHandle](https://docs.microsoft.com/en-us/windows/win32/secmgmt/opening-a-policy-object-handle) and [AddPrivileges](https://docs.microsoft.com/en-us/windows/win32/secmgmt/managing-account-permissions) functions are provided by the docs

Code is hosted on [github [ADD LINK]]()

### 2. Turn it on before use

While acquiring the privilege is a once-per-user action, every process that needs to allocate large pages need to turn on the privilege first, by calling `AdjustTokenPrivileges`. This needs to be done by an admin account, however the allocation itself does not require admin privileges. 

### 3. Use it

A call to `VirtualAlloc` finally allocates the needed meomory: 

```
VirtualAlloc(NULL, buffersize, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE)
```

Note that memory allocated will be commited as well, it is not possible to just reserve a [large-page allocation](https://docs.microsoft.com/en-us/windows/win32/memory/large-page-support). 

One last note, after such memory is allocated, it's not possible to see it in Task Manager's "Memory/Working Set", and is rather viewable from "Commit Size" tab: 

![Task Manager](./task_manager.png)

## Give me the numbers 

I ran a simple experiment to see if large pages are really worth the time. I tried to simulate a memory-bound piece of code that accesses a large chunk of memory randomly. I then measured how many accesses can be done per second, when allocating the large chunk normally, vs as a large page.  

![Compare ](./comparison.gif)
Left: Large-page enabled, right: disabled

Large page memory access are far ahead! Not really a surprise, but it's nice to see in action. Code is hosted on [github [ADD LINK]]()

## Last notes

Large Pages are especially important if you have a large chunk of memory to allocate, and access frequently. Its major downside: it's hard to allocate, and once allocated, it "sits" there in the physical memory, even if the process doing the allocating is idle, as large-page memory is non-pageable.

So why didn't they give me the gains I expected in the beginning of the blog? Well, turns out that this chunk was *already* getting allocated as a large-page, something I missed due to a mess of who's-allocating-what. The thing about large-pages - they are not very visible.

<!-- 
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
  

experiment: 
- Create 10 GB of memory garbage, see how many random lookups/sec can be done
- Same with Large page, compare random lookups/sec


Outline: 
- Intro: 
  - learnt about it from someone introducing it to code base
  - recently I tried to apply it but it didn't give me the effects I needed, I thought I would dig deeper
- What are they?
  - What is a Page
  - What is a Large-Page
  - Why are Large-Pages Interesting
  - Bonus: Huge Pages
-  The How
   -  Requirements: SeLockMemoryPrivilege 👍
      -  Why do we need to lock? 👍
      -  blog by Old New Thing 👍
   -  First, add the privilege 👍
      -  On a server? Probably run this code automatically 👍
      -  Need to be done once 👍 
   -  Acquire Privlege before use 👍
      -  Needs to be done by admin as well 👍
   -  Finally, use the `MEM_LARGE_PAGES` flag during allocation 👍
   -  You cannot see it as a working set - but it's allocated! use commit size or vmmap to see it 👍

 -->