---
title: "More is Sometimes Less: When Lowering Load can Trigger Higher Latencies"
date: "2019-05-04T12:00:32.169Z"
description: 
---

- [Backdrop](#backdrop)
- [Narrowing it Down](#narrowing-it-down)
  - [Data Store is Innocent](#data-store-is-innocent)
  - [gRPC](#grpc)
  - [Profile Cures All](#profile-cures-all)
- [More Debugging](#more-debugging)
  - [Ignite the Network Engineer Within](#ignite-the-network-engineer-within)
  - [What if](#what-if)
  - [Now, which OS are we on](#now-which-os-are-we-on)
  - [Nagle's algorithm](#nagles-algorithm)
- [Conclusion](#conclusion)

## Backdrop

As in [many of my posts](https://mahdytech.com/2019/01/13/curious-case-999-latency-hike/) - a distributed service was misbehaving. Let's call this service Alvin. This time, I didn't discover the issue myself, it was reported by the client team instead.

I woke up one day to a disgruntled email, understandably so, due to very weird latencies concerning Alvin, that was to be launched soon. That was surprising for me - as I have thoroughly tested this service, especially the latency - their point of complaint.

Another look at the email verified one point - I didn't *exactly* test the conditions they mentioned. Their [QPS](https://en.wikipedia.org/wiki/Queries_per_second) was much lower than mine. I was testing with 40k QPS, while they were testing with only 1k.

I almost got ready to reply dismissing their results and asking them to redo it. After all, if 40k QPS have a [99%](https://en.wikipedia.org/wiki/Percentile) latency less than 10 ms*, how on earth can 1k QPS have a 50ms 99% latency? But I decided otherwise - let's humour them, and write back a scathing email how their numbers don't make sense. So I ran my experiment once again - with lower QPS this time.

Since I am blogging about this - you probably figured it out already: their numbers were correct. I re-ran my dummy client again and again, all with the same result: Lower QPS, not only leads to higher latencies, but just plain higher number of requests with latency over 10ms. This just smoldered all logic I knew about how services should act.

<div class="infogram-embed" data-id="5905fbdf-b2fc-4685-8503-f44bfd823319" data-type="interactive" data-title="Line Chart"></div>

*All latency numbers reported in this article are within the same Data Center.

## Narrowing it Down

The first thing to do when faced with a latency issue is to shortlist the suspects list. Let's dig a bit deeper into Alvin's architecture: 

![AlvinStore](./AlvinStore.svg){.aligncenter}

A good starting point is to list the IO hops done (Network calls/Disk Lookups, etc), and try and figure out which one contains the latency delay. Besides the obvious IO between Alvin and its client,  Alvin is doing an extra IO step - it is calling into a Data Store. However, this Data Store lives in the same cluster with Alvin, so it should be a smaller network hop than the one between Client <=> Alvin. The following suspect list lines up:

1. Client => Alvin
2. Alvin => Data Store
3. Data Store Disk Lookup
4. Data Store => Alvin
5. Alvin => Client

Let's try to strike through some names.

### Data Store is Innocent

First thing I did was converting Alvin into a ping-ping sever: if latency improves, then my Alvin or Data Store's implementation has a bug - nothing unheard of. This yielded our first experiment graph:

<div class="infogram-embed" data-id="75e7d91d-4529-4983-bd33-5b72395fe642" data-type="interactive" data-title="Copy: Line Chart"></div>

<script>console.log('test carol')</script>
Clearly no improvement, our suspect list is shortened to half:

1. Client => Alvin
2. Alvin => Client

I thought I was almost there, I was wrong.

### gRPC

Now is a good time to introduce a new player to the scene: [gRPC](https://github.com/grpc/grpc). `gRPC` is an open-source library by Google for intra-process [RPC](https://en.wikipedia.org/wiki/Remote_procedure_call) communication. While `gRPC` is highly optimized and heavily adopted, this was my first time using it on scale, and I was expecting that my usage was not optimal - to put it mildly.

Having `gRPC` on board introduced the new question: is the problem with my implementation, or is it with `gRPC`? A new suspect is added to the list: 

1. Client => gRPC (server)
2. gRPC (client) => gRPC (server)
3. gRPC (Server) => Alvin

My Client/Alvin implementation does not look a lot different from client/server [async examples](https://github.com/grpc/grpc/tree/v1.19.0/examples/cpp/helloworld). 

> Note: This is a bit of a simplification, since `gRPC` gives consumers power (boilerplate?) of using their own threading model resulting in a bit of intertwined execution stack between `gRPC` and consumer implementation. But let's keep it for the sake of clarity.

### Profile Cures All

At this point I figured: "Easy! I will take a profile and find out which part is causing the delay". A skim of this blog clearly shows I am a [big fan of precise profiling](https://mahdytech.com/2019/01/13/curious-case-999-latency-hike/), and for a reason: CPUs are blazing fast, and in the majority of my experience, they are not the bottleneck. Rather, it's usually the CPU having to stop processing to do something else that brings about delays. Precise CPU profiling is made just for that: it has an accurate record of all the context switching happening, and in so an idea of where delays are.

I took two profiles: one under high QPS (low latency), and one with PingPong low QPS server (high latency). And just for the heck of it, I also took a sampled CPU profile. To my dismay, however, nothing of substance was there. When doing this comparison, I am usually looking for an abnormal callstack. For example: the bad side showing a lot more context switching (10x or more) than the good side. But what I found was an almost matching context switching between good and bad runs.

Ahmed: 0  
Weird bug: 2 (wasted hours running profiles)

## More Debugging

Let's stop for a second, and try to reason out our problem. 

=> In hindsight, profile probably caught what was happening. The issue, however, is probably visible in Kernel Mode callstack, not User Mode.

### Ignite the Network Engineer Within

I have a confession to make: my networking knowledge is abysmal, relatively to how much I deal with networks on a daily basis. Usually, I'd say that's to show how good abstraction layer work. However, I could not delay it any longer: network is the suspect #1 at the moment, and I needed to learn how to debug that.

First, I ran [PsPing](https://docs.microsoft.com/en-us/sysinternals/downloads/psping) and targeted the TCP port my server was listening at. I used default parameters - nothing fancy. Despite running over a 1000 pings, I didn't see any of which exceed 10ms, except for the first one for warmup. 

Then, I tried [tracecert](https://support.microsoft.com/en-ca/help/314868/how-to-use-tracert-to-troubleshoot-tcp-ip-problems-in-windows), maybe there was an issue with one of the hops that I was hitting, but tracecert came back empty handed. 

I was starting to get worried I am never going to figure this out.

### What if

I was desperate at this point. My toolbox was empty, and it was just me.

From the start, the 50ms latency number has been bugging me. 50 ms is *a lot* of time. I decided then that my goal is to keep cutting pieces until I can figure out which part is exactly causing this error. Next came the experiment that worked.

It was pretty simple, and in hindsight, as usual, very obvious. I placed my client in the same machine as Alvin, and sent the request to `localhost`. And the latency jumps were gone!  

<div class="infogram-embed" data-id="23d128c0-96e8-4db6-b65b-c069f1e1348e" data-type="interactive" data-title="Copy: Ping Pong Alvin"></div>

Something is definitely happening with network. But if pings were fine, where was the problem?

### Now, which OS are we on

I knew gRPC is widely adopted, it can't be the case that gRPC network implementation is misbehaving. But then, I was on Windows. and that can make a big difference. 

Then, I made the most important experiment: replaced Alvin with two Ping Pong Go servers: one in Windows, and one in Linux. 

<div class="infogram-embed" data-id="9adef12a-ba95-40e9-b04e-d20dd2f94f96" data-type="interactive" data-title="Copy: Localhost"></div>

Lo and behold: gRPC's Windows server had a problem. 

### Nagle's algorithm

Once I found out this was a Windows issue, I knew what to do: I looked for the internal Windows communication library that I knew to behave well, and queried it for all the Winsock flags it set. I then went to gRPC's, added all the missing ones, and deployed. 

*Almost* there: I removed one by one, and what remained was the infamous [TCP_NODELAY](https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-setsockopt), aka Nagle's algorithm. 

[Nagle's algorithm](https://en.wikipedia.org/wiki/Nagle%27s_algorithm) tries to decrease number of packets sent over the network, by delaying message transmission until outstanding size exceeds certain number of bytes. While this can be nice for the average user, this is devastating for servers. `gRPC` had this flag on for its Linux implementation for TCP sockets, but not for Windows. I fixed that with [1dce1009](https://github.com/grpc/grpc/commit/1dce1009e67ea4b5934a61b1bcf8a217bd12cc76).

## Conclusion

It all made sense at the end. Latency was high at low QPS due to an OS optimization that needed to be turned off. Running on localhost probably did not touch on that network code, hence it all went away when we ran Alvin through localhost. 

I hope the next time you see higher latency coinciding with lower QPS,Nagle's algorithm would be on your checklist. 

-------------

At this point, it seemed almost clear that this is a networking issue. If profiles are not showing anything - network must be misbehaving at some step in the middle. 

- Run tracecert, ping, etc
- Send request within the machine, but using the machine's IP Address: problem persisted
- Send request from localhost, this improves latency!! Getting closer
- Tried using a dummy Windows and linux server Grpc servers, Linux was much better:
  - It's a Windows networking problem!!



<div><script>!function(e,t,s,i){var n="InfogramEmbeds",o=e.getElementsByTagName("script")[0],d=/^http:/.test(e.location)?"http:":"https:";if(/^\/{2}/.test(i)&&(i=d+i),window[n]&&window[n].initialized)window[n].process&&window[n].process();else if(!e.getElementById(s)){var r=e.createElement("script");r.async=1,r.id=s,r.src=i,o.parentNode.insertBefore(r,o)}}(document,0,"infogram-async","https://e.infogram.com/js/dist/embed-loader-min.js");</script></div>