---
title: Why I don't use Task Manager for Memory Metrics
date: "2019-01-05T22:40:32.169Z"
description: Task Manager can be lacking as a memory tracker. Let's cover alternatives to replace it. First, let’s discuss how memory allocations work in Windows.
path: /2019/01/05/umdh-slow-leaks/
featuredImage: ProcessExplorer.PNG
---

As a windows user for many many years, Task Manager is a friend. Through the years, I have used it to kill thousands of misbehaving apps, and getting info about which ones are exhausting my resources. Until I started working with machines that has 100s of GBs of memory and apps to match. In this post I would like to discuss how it can be lacking as a memory tracker, and go over alternatives that could replace it. First, let's discuss how memory allocations work in Windows.

- [Allocations in Windows](#allocations-in-windows)
  - [Reserving vs Committing memory](#reserving-vs-committing-memory)
  - [OS Paging](#os-paging)
- [Memory Tracking](#memory-tracking)
- [Effective Memory Metrics](#effective-memory-metrics)
- [Debugging with Memory Info](#debugging-with-memory-info)

tl;dr: Task Manager hides info about Process Paged Memory and its Virtual Space. Use `Process Explorer` instead.

## Allocations in Windows

Whenever a new process starts, OS gives it reserves some space for this process. In x86 systems, this space is 4GB, with usually 2GB for kernel use, and the rest for the process. For this post, let's ignore kernel usage. For x64-systems reserved process memory can grow to a whopping 64TB. How come can we allocate up to several TBs when we actually have a measly 8GB machine? We'd need first to understand reserving vs committing memory.  

### Reserving vs Committing memory

Not all parts of that huge address space are equal. Some parts of Process Address Space is actually backed by either physical RAM, or by disk (explained [below](#OS-Paging)). Memory that is backed is referred to as `Committed`. Memory otherwise, and that's the vast majority of a process's address space, is `Reserved` memory. In C++, reserving a piece of memory can by achieved through a call to [VirtualAlloc](https://msdn.microsoft.com/en-us/library/windows/desktop/aa366887%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396). Committed memory is then the actual resource limit in the OS, since it's backed by hardware. Let's give it a look.

### OS Paging

OS Paging is an amazing idea. Basically, the OS *realizes* some parts of the memory are not used a lot by your app. Now, why waste previous physical memory on that? A process in kernel makes this unused space is written to disk instead. Once it gets accessed again, it gets brought back into memory.

For a more detailed explanation of how memory works in windows, I cannot recommend enough Mark Russinovich's [Mysteries of Memory Management Revealed](https://www.youtube.com/watch?v=TrFEgHr72Yg).  

## Memory Tracking

Now that's a lot of info to track - with endless [scenarios](#Debugging-with-Memory-Info) to apply.  Whom to turn to? of course it's Task Manager!

Memory that is backed by RAM is generally referred to as `Working Set`, while `Private Bytes`, in general, are the overall committed memory. Dlls make definitions a little more complicated, so let's ignore them for now. In other words:

```
Private Bytes [Committed Memory] =  Private Bytes + Page File 
```

By default, Task Manager shows Working Set under any process:

![Default Task Manager](./TaskManagerWorkingSet.PNG "Task Manager shows Working Set by default")

And that's the number I used to look at all the time. Little did I know, Task Manager *actually* has commit info, but it's under the column `Commit Size`. I so far could not find Virtual Memory info in there.

![Task Manager after adding Commit Size](./TaskManagerCommitSize.PNG "It is possible to add Commit Size")
*Task Manager allows adding Commit Size by right-clicking columns and adding it*

## Effective Memory Metrics

Thankfully, there are many other resources to examine Perf in Windows. Every windows machine has `PerfMon` that can be used to expose very detailed info about each process and the system in general:

![PerfMon](./PerfMon.PNG "PerfMon allows examining very detailed measurements about system")

Interestingly, PerfMon can actually possible to examine & compare metrics across two or more machines in the network. It's very powerful, but Task Manager is obviously more user friendly. In order to get an in-the-middle solution, I recommend [Process Explorer](https://docs.microsoft.com/en-us/sysinternals/downloads/process-explorer):

![Process Explorer](./ProcessExplorer.PNG "Process Explorer showing all Private Bytes, Working Set, and Virtual Size")![Process Explorer System Info](./ProcessExplorer.PNG "Process Explorer showing overall system info")

Boom! Visual Studio, why are yous till 32-bit (notice its Virtual Size)? My computer's peak memory usage has been at 89% of its limit, not too shabby. This comes in useful [later](#Debugging-with-Memory-Info). 

## Debugging with Memory Info

Fortunately, this info is not just some OS trivia. It has time and time helped me solve various debugging various problems. 

Most importantly, is figuring out the *untouched* parts of committed memory. The paged part of the process represents a very important piece of information: memory committed and not used frequently or rarely used.

Even if this memory is going to be used occasionally, it is important to realize that this access is going to be expensive, and doing this on the hot path is a no-no. Leaked memory should show up as part of this value too.

For this reasons, I have previously heard it suggested to remove `PageFiles` completely, effectively making `Private Bytes == Working Set`. However, it's a double-edged idea though. This renders the OS unable to discard memory for misbehaving apps, which could sometimes include OS apps allocating data not needed in memory.  