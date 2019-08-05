---
title: Profiling Processor Cache Misses with VTune
date: "2019-07-31T12:00:32.169Z"
description: VTune is a processor instruction level profiler,
---

I heard about [VTune](https://software.intel.com/en-us/vtune) a while ago as a processor instruction level profiler, but I never got the chance to play around with it. This blog - and maybe others in future - will narrate my trials to get a hang of VTune.

- [First things first: What's VTune](#First-things-first-Whats-VTune)
  - [Getting VTune](#Getting-VTune)
- [Let's Take VTune for a Ride: Cache Misses](#Lets-Take-VTune-for-a-Ride-Cache-Misses)
  - [Row-Major and Column-Major Array Access](#Row-Major-and-Column-Major-Array-Access)
  - [Profiling with VTune](#Profiling-with-VTune)
  - [Column-Major Access](#Column-Major-Access)
  - [Row-Major Access](#Row-Major-Access)
  - [Can We Make it Faster?](#Can-We-Make-it-Faster)
- [Conclusion](#Conclusion)

## First things first: What is VTune

VTune is a [profiler](https://en.wikipedia.org/wiki/Profiling_(computer_programming)) capable of a similar job to CPU sampled profiling in [XPerf](https://mahdytech.com/2019/01/13/curious-case-999-latency-hike/#f1-profile0) or Visual Studio debugger, with a significant edge: it supports hardware-based event sampling, using a special chip on Intel processors called the [Performance Monitoring Unit](https://software.intel.com/en-us/articles/intel-performance-counter-monitor) (PMU). Using events reported by the PMU, VTune can give an in-depth look into an application's performance, as well as a general "verdict", such as being [front-end bound](https://software.intel.com/en-us/vtune-amplifier-help-front-end-bound) or [back-end bound](https://software.intel.com/en-us/vtune-amplifier-help-back-end-bound).

While OS-reported CPU usage levels are effective in many cases, the time comes when we need deeper insight into what is happening on the hardware level. In my experience, this is mostly beneficial in tight loops or portions of the code on an ultra-hot path: those 50 lines of code that are responsible for more CPU usage than the thousands of lines in the rest of of the application, it is for those 50 lines that VTune can come in handy.

### Getting VTune

I was pleasantly surprised to see that personal use for VTune has become [free](https://software.intel.com/en-us/vtune/choose-download#standalone), up until recently it was only a 30-day evaluation. The 2019 edition comes with a sleek GUI and great integration with Visual Studio. Using VTune from within Visual Studio is straightforward and fits nicely into the usual development cycle.

## Let's Take VTune for a Ride: Cache Misses

We're always taught to build applications to be [cache-friendly](https://www.youtube.com/watch?v=WDIkqP4JbkE&feature=youtu.be), but such things are easier said than done. Now, if we can *detect* being non-cache friendly and fix it, that's powerful. For my first VTune experiment, I want to explore if it can detect cache unfriendliness.

### Row-Major and Column-Major 2D Array Traversal

A classic example for cache misses is the 2D array traversal problem. Say, we need to find sum of all elements in an array, we could either loop on rows, or on columns:

``` cpp
// row major traversal
for (int i = 0; i < numRows; ++i)
    for (int j = 0; j < numColumns; ++j)
        sum += matrix[i][j];

// column major traversal
for (int i = 0; i < numRows; ++i)
    for (int j = 0; j < numColumns; ++j)
        sum += matrix[j][i];
```

To choose a row or a column-major design depends entirely on how the matrix is represented in memory. Assuming `matrix` is an array of pointers (e.g. `int**` or a `vector<vector<int>>` the column major traversal has a huge issue: it will trigger vastly more cache misses than row-major traversal.

> For an overview of what's a cache miss and CPU memory access costs checkout ["Why do CPUs have multiple cache levels?"](https://fgiesen.wordpress.com/2016/08/07/why-do-cpus-have-multiple-cache-levels/)

A picture makes things easier:

![Traversal](./rowcolumnarrays.jpg)
<center>Source: <a href=https://craftofcoding.wordpress.com/2017/02/03/column-major-vs-row-major-arrays-does-it-matter/>craftofcoding</a></center>

When element `1` is read, adjacent elements `2` and `3` are brought into the cache, which comes in handy as we access them next. However, a column-major pattern accesses `4` afterwards, not utilizing `2` or `3` that we already have in cache, and incurring an additional memory access. 

> Note that the above is a toy sample, and there need be more than 3 elements per row, otherwise the whole array could be cached.

### How Slow is This

In order to test the effects of array access pattern, I used a tiny app that builds an array, then tries to sum its elements 1000 times, so as to emulate a real CPU load. I created two versions, one with row-major and one with column-major access for the array, code for both is on [github](https://github.com/aybassiouny/mahdytech/tree/master/content/blog). Then, I ran both examples for varying array sizes: 

<canvas class="js-chart" width="400" height="400" data-chart="
    {
        'data': [
            {
                'label': 'Row-major',
                'data': [33,27,197,111,57,223,1272,2809,12298,41725,208140]
            },
            {
                'label': 'Column-Major',
                'data': [16,23,32,85,257,976,3366,11967,75507,382699,2559339]
            }
        ]
    }
    " 
    data-labels-chart = "1,2,4,8,16,32,64,128,256,512,1024"
    data-yaxis-chart="logarithmic"
    data-yaxis-name = "Latency in Nanoseconds" data-xaxis-name = "Array Size"
    ></canvas>

The graph shows clearly that as array size goes beyond several elements, where whole array might even be cached, row-major access is the very clear winner. Now that we have a ripe test case, let's see how VTune can profile this.

## Profiling with VTune

From Vistual Studio, it is straightforward to trigger a VTune profile by clicking `Profile with VTune Amplifier`, adjusting config, then clicking `Start`: 

![VTune inside Visual Studio](./vtune_inside_vs.PNG)

Note using `Microarchitecture Exploration` under the How, as VTune is capable of doing User-Mode sampling, but I would rather use XPerf or [F1](https://docs.microsoft.com/en-us/visualstudio/profiling/how-to-install-the-stand-alone-profiler?view=vs-2019) for that.

One more note is to make sure to set paths for system symbols. While VTune interface does not stress adding them, results were useless without system symbols:

![Adding Symbols](./symbols_1.PNG)

![Adding Symbols](./symbols_2.PNG)
<center>Adding Symbols to VTune</center>

> One last note - VTune also supports profile using a portable executable, allowing for capture & analysis to happen on different machines.

A profile capture is now ready - let's have a look at the results!

### Column-Major Access

In this test, I used an array of size 2048, and ran that operation for 200 times. Running VTune for the app resulted in:

![Column Major](./column_major.PNG)

As trivial as this example is - I was impressed by the verbose verdict. The `Memory Bound` red box aligns well with our expectations, we're accessing too much memory and too little cache.

### Row-Major Access

Next, I ran same app with a small change in indices used to access the array:  

![Row Major](./row_major.PNG)

Note the "Elapsed Time" of 2.5s, a 7x speedup over the row-major test. Furthermore, the 30.5% `Retiring` box (in green) represents the percentage of instructions the processor successfully predicted & executed, a higher number represents more efficient execution - again aligning with expectations.

Note that VTune makes it clear this is by far not a perfect implementation, and shows a couple of red boxes. The red `Core Bound` box below represents backend non-memory issues, in our case *probably* referring to missing out on [vectorizing instructions](https://www.codingame.com/playgrounds/283/sse-avx-vectorization) for array traversal and sum. The other red box, `Memory Bound` *probably* refers to the inter-dependency of our stores: all array accesses are appended to the same variable `sum`, and dependent stores are hard to parallelize.

## Conclusion

I am pretty impressed with VTune so far, and wonder how does it scale for profiling production-scale applications. Nonetheless, VTune is educational, I have learnt a bunch about hardware, even though I ran one of the simplest examples possible.

Code samples & VTune captures for those interested in further debugging are in the [post material](). Thanks for reading!
___

Help [improve this text]() or discuss it on [reddit](), [hackernews]().