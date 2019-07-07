---
title: Profiling Processor Cache Misses with VTune
date: "2019-07-01T12:00:32.169Z"
description: 
---

I had heard about [VTune](https://software.intel.com/en-us/vtune) a while ago as a hardcore profiler at the processor instruction level, however I never got the chance to play around with it. This blog - and maybe others in future - will narrate my trials to get a hang of VTune.

## First things first: What's VTune

VTune is a [profiler](https://en.wikipedia.org/wiki/Profiling_(computer_programming)), capable of a very similar job to CPU sampled profiling in [XPerf](https://mahdytech.com/2019/01/13/curious-case-999-latency-hike/#f1-profile0) or Visual Studio debugger. However, VTune has a huge edge: it supports hardware-based event sampling, using a special chip on Intel processors called the [Performance Monitoring Unit](https://software.intel.com/en-us/articles/intel-performance-counter-monitor) (PMU). VTune uses events reported by PMU to report a much more detailed description of instruction-level events, such as being [front-end bound](https://software.intel.com/en-us/vtune-amplifier-help-front-end-bound) or [back-end bound](https://software.intel.com/en-us/vtune-amplifier-help-back-end-bound).

Put in another way, while OS-reported CPU usage levels are effective in many (most?) cases, the time comes when we need deeper insight into which bottleneck the processor is facing, and how to make this piece of code faster. In my experience, this is mostly beneficial in tight loops or portions of the code on the ultra-hot path. In many cases, there are 50 lines of code that are responsible for more CPU usage than the thousands of lines in the rest of of the application, it is for those 50 lines that VTune can come in handy.

### Getting VTune

I was pleasantly surprised to see that personal use for VTune has become [free](https://software.intel.com/en-us/vtune/choose-download#standalone), I recall that up until recently it was only an evaluation. The 2019 edition comes with pretty sleek GUI and great integration with Visual Studio, using VTune from within Visual Studio is straightforward and fits nicely into the usual development cycle.

![VTune inside Visual Studio](./vtune_inside_vs.png)

Note using "Microarchitecture Exploration" under the How, VTune is capable of doing User-Mode sampling, but I would rather use XPerf or [F1](https://docs.microsoft.com/en-us/visualstudio/profiling/how-to-install-the-stand-alone-profiler?view=vs-2019) for that.

## Let's Take VTune for a Ride: Cache Misses

We're always taught to build applications to be [cache-friendly](https://www.youtube.com/watch?v=WDIkqP4JbkE&feature=youtu.be), but such things are easier said than done. Now, if we can *detect* being non-cache friendly and fix it, that's powerful.

### Row-Major and Column-Major Array Access

The classic example for cache misses is usually the very simple array traversal problem. Say, we need to find sum of all elements in an array, we could either loop on rows, or on columns:

``` cpp
// row major
for (int i = 0; i < numRows; ++i)
    for (int j = 0; j < numColumns; ++j)
        sum += matrix[i][j];

// column major
for (int i = 0; i < numRows; ++i)
    for (int j = 0; j < numColumns; ++j)
        sum += matrix[j][i];
```

The column major traversal has a huge issue: it will trigger vastly more cache misses than row-major traversal.

> For an overview of what's a cache miss and CPU memory access costs checkout ["Why do CPUs have multiple cache levels?"](https://fgiesen.wordpress.com/2016/08/07/why-do-cpus-have-multiple-cache-levels/)

Let's take a look how each traversal looks like:

![Traversal](.\rowcolumnarrays.jpg)
<center>Source: <a href=https://craftofcoding.wordpress.com/2017/02/03/column-major-vs-row-major-arrays-does-it-matter/>craftofcoding</a></center>

When element `1` is read, adjacent elements `2` and `3` are brought into cache, which comes in handy when we access them next, as we don't need to do a memory access again. However, in a column-major access pattern we lose this advantage, as after we bring `2` and `3` we don't access them and access `4` instead, and bring its neighbors: `5` and `6`, then, oops, we don't use that either and access `7`.

Let's see how VTune can profile this.

### Profiling with VTune

At first, I tried using a small array of several MBs, but array creation itself was 

Then, I had forgotten adding symbols - and while the VTuner interface does not stress adding them, results were useless without symbols. Don't forget to add them from capture dialog:

![Adding Symbols](.\symbols_1.png)
![Adding Symbols](.\symbols_2.png)
<center>Adding Symbols to VTune</center>

- VTune helps dig deeper more than any other tool, using Hardware Profile TPM 
- I am just a newbie with this, I will try to emulate cache misses through the textbook example: row vs column on a big array
- The first time I tried, I had the array at ~14 MBs, however I think array creation was interferring 