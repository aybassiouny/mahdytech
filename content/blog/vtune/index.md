---
title: Profiling Processor Cache Misses with VTune
date: "2019-07-01T12:00:32.169Z"
description: 
---

I had heard about [VTune](https://software.intel.com/en-us/vtune) a while ago as a hardcore profiler at the processor instruction level, however I never got the chance to play around with it. This blog - and maybe others in future - will narrate my trials to get a hang of VTune.

## First things first: What's VTune

VTune is a [profiler](https://en.wikipedia.org/wiki/Profiling_(computer_programming)), capable of a very similar job to CPU sampled profiling in XPerf or Visual Studio debugger. However, VTune has a huge edge: it supports hardware-based event sampling, using a special chip on Intel processors called the [Performance Monitoring Unit](https://software.intel.com/en-us/articles/intel-performance-counter-monitor) (PMU). VTune uses events reported by PMU to report a much more detailed description of instruction-level events, such as being [front-end bound](https://software.intel.com/en-us/vtune-amplifier-help-front-end-bound) or [back-end bound](https://software.intel.com/en-us/vtune-amplifier-help-back-end-bound).

Put in another way, while OS-reported CPU usage levels are effective in many (most?) cases, the time comes when we need deeper insight into which bottleneck the processor is facing, and how to make this piece of code faster. In my experience, this is mostly beneficial in tight loops or portions of the code on the ultra-hot path. In many cases, there are 50 lines of code that are responsible for more CPU usage than the thousands of lines in the rest of of the application, it is for those 50 lines that VTune can come in handy.

### Getting VTune

I was pleasantly surprised to see that personal use for VTune has become [free](https://software.intel.com/en-us/vtune/choose-download#standalone), I recall that up until recently it was only an evaluation. The 2019 edition comes with pretty sleek GUI and great integration with Visual Studio, using VTune from within Visual Studio is straightforward and fits nicely into the usual development cycle.

![VTune inside Visual Studio](./vtune_inside_vs.png)



- VTune helps dig deeper more than any other tool, using Hardware Profile TPM 
- I recommend using Visual Studio plugin on windows, it makes continuous profiling (CP tm) much easier
- I am just a newbie with this, I will try to emulate cache misses through the textbook example: row vs column on a big array
- The first time I tried, I had the array at ~14 MBs, however I think array creation was interferring 