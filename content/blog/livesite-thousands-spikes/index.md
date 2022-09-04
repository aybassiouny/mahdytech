---
title: "Livesite by a thousand spikes 📈"
date: "2022-08-30T12:00:32.169Z"
description: An infrequent issue on many machines can disguise as a cluster-wide issue.
seotitle: When Lower Load Triggers Higher Latencies
featuredImage: .\fishing.png
---

While my team was moving a service, lets call it Billy, to a new cluster with differential hardware, Billy's performance degraded. Now my team knows I am into this kind of thing, so they asked me to take a look. I thought it'd be a matter of increasing the number of threads or something similarly straightforward, and said "consider it done". In hindsight, way too much confidence 😅.

- [**First things first**](#first-things-first)
- [**Digging Deeper**](#digging-deeper)
- [**Take a step back, and retry**](#take-a-step-back-and-retry)
- [**Prepare the bait**](#prepare-the-bait)
- [**Spike's in the bait**](#spikes-in-the-bait)

## **First things first**

My first move is to get a concrete idea of what kind of "perf issues" we have here. With some luck, I can get easily ready monitoring graphs for Billy, and my go-to graph is availability:

We have availability SLAs for all services, if availability is below that SLA, we have got a problem. Billy had low availability (than our SLA), so we've got a real problem here. There are many reasons for an availability drop – as many as the reasons a query can fail. Good client monitoring is essential to triage why we have a low availability; it could be for example a client issue, or a server issue, and if it is a server issue could be due to latency or maybe server is too busy handling other requests. In Billy's case, it was high server latency.

![Billy's Latency](BillyLatency.png) 

I started by preparing a simple change that updated some configs (e.g. number of threads used, maximum number of requests processed at a time, etc) and hoped that would do the trick – it didn't. Latency and availability stayed about the same.

## **Digging Deeper**

In my experience, a latency regression is a sign of some introduced resource contention, 90% of the time it is heap contention during allocating or freeing memory. On Windows, this contention shows up clearly on an ETW capture. I used to use [XPerf]() for taking ETW captures, but now I find [WPR]() more convenient, I usually suffice with the CPU events using the command:

```
Wpr -start cpu.verbose
```

The ".verbose" part is important as it means we're getting callstacks as well – hard to optimize code without knowing which line is causing the issue. I took a very brief 5 second capture (CPU.Verbose can grow huge very quickly especially on servers with 100+ cores), I then open the captured etl file with wpa on my devbox, and directly jump to the "CPU precise" events:

![WPA](wpa1.png)

CPU precise contains "context switching" events. These are events emitted by the kernel when threads stopped what they were doing ("switched out"), as well as when they were resumed ("switched back in"). This view relates to latency issues because, well, CPUs are fast. Like, crazy fast these days. For my (and most) workloads, delays don't happen when CPUs are churning off work non-stop. Rather, latency delays usually happen because threads got stopped for some reason (blocks/waits or CPU starvation), and that's exactly what the "CPU Precise" events tell us more about.

![ContextSwitching](context_switch.jpg)

Anatomy of a context switch event. Source: [CPU Analysis | Microsoft Docs](https://docs.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis)

Back to Billy's profile, I drilled down the callstack with the highest combo of number of waits, average wait time, as well as CPU usage. I saw that there was a lock during freeing some object that was causing a lot of threads to wait:

![WPA](wpa2.png)

It looked promising so I went ahead and changed the allocator used to one that does not free its memory (albeit uses more CPU, but CPU starvation was not an issue here). I deployed my change, and tested again, and, latency was not affected 🤷‍♂️ (In hindsight the average wait in the above graph should been enough hint to me that this is not it)

## **Take a step back, and retry**

My first thought was to go back to the capture and find another locking callstack to fix. But that felt don quixote-esque. Maybe I missed something?

I had an idea - maybe one machine was causing the latency issue? I took a look at a per-machine graphs, instead of the ones earlier that are averaged across many machines:

![Per Machine Latency](per_machine_latency.png)

It was not just one machine causing latency increase, it was _all_ the machines. As it turned out, each machine on its own _usually_ has normal latency, except once every 15-20 min it would have a crazy high spike, and come down immediately. Now that's a different beast from what I was expecting.

The etw profile I discussed above only lasted for 10 seconds! That might sound crazy short but I usually test under \> 1-2k QPS, so in those 5 seconds I have maybe 5k or 10k requests, which is usually good enough sample. Also, I like keeping profiles as small as possible, as that 10 second ETW profilr can already inch close to 500 MBs in size.

However, when your problem happens once every 15 minutes, you can't just take a 5 second profile and hope you catch it, odds are vastly against you. I had to take a different approach.

## **Prepare the bait**

The first thing I needed to do is to make sure my captures are "circular", meaning that it would not matter how long my capture is, only the chunk is actually recorded and the overall capture would still be a reasonable size. [Wprp profiles](https://docs.microsoft.com/en-us/windows-hardware/test/wpt/authoring-recording-profiles) offers this capability, as you can specify a limit on how many buffers are used & their size, and when buffers are full the oldest events just get overwritten.

```
<SystemCollectorId="WPR"Name=" WPR"\>
  <BufferSizeValue="1024"/\>
  <BuffersValue="3072"/\>
</SystemCollector\>
```

BufferSize & Buffers limits the capture size. Full file is [here](github)

The second part was to time the capture to stop automatically when there's a spike. As the spikes happen every 10-15 minutes, and I can only take a maximum of ~1 minutes captures (otherwise they are too big), it became important for the stopping mechanism to become automatic, so I had my script query Billy's latency every 30 seconds, and whenever latency was extremely high (over 500ms!) I had my capture stop. After some (longer than it should) struggle with PoweShell, the final capture script looked something like:

```
Wpr -start CPU.wprp -start MyCustomEvents.wprp
While(true)
    $latency = queryLatency()
    If ($latency > 500ms)
        Break
    Sleep 30s
Wpr -stop bigfile.etl
```

I had it all in place: I started my capture on several servers to maximize my chances, and started waiting for my bait to catch.

![Fishing](fishing.png)

What [HuggingFace's](https://huggingface.co/) AI thinks "programmer fishing" looks like – looks alright to me

After a couple of failed tries (I hate powershell) I had my capture. I opened it and verified - my capture stopped on the exact minute a latency spike happened. At this point was excited! Inside this capture I had the CPU events that covered the latency spike! I have my solution inside - I just need to look in the right place.

## **Spike's in the bait**

Using the "MyCustomEvents.wprp" profile above, I was also capturing custom events that shows me latency for each request, this was very helpful. I could immediately jump to requests with the latency spike. I found 10 requests with crazy high latencies (over 1 second) while the rest had an average of just over 5 ms.

![WPA](wpa3.png)

Each row is a different event representing a request. You can see how latencies starting some point in time start getting very high. I picked one request, and zoomed in.

![WPA](wpa4.png)

I then picked the thread id for that request, 21560, and searched the thread number in the "CPU Precise" event tab. I was looking for what happened right before that thread emitted this event.

![WPA](wpa5.png)

I could not believe my eyes! This thread apparently had a 1 second wait around the same time my event was emitted, exactly matching the latency of this request, around 1 second! I looked at the callstack and saw the [dreadful call](https://en.cppreference.com/w/cpp/thread/sleep_for):

```
std::this_thread::sleep_for()
```

VERY interesting! Some function on our hotpath is waiting for 1 second. Can this be it?

I zoomed out, and here it was! All the events of this 1 second wait being called matched exactly with the requests having high latency!

![WPA](wpa6.png)

With little digging, it turned out some library we're consuming decides on rare occasions to sleep for 1 second, and for some reason this happens more often on the new machines.

Definitely one of my more interesting debugging stories :) Thanks for reading