------Part 1:

Scheduler "schedule()" function takes some int value, which represents task to handle by
scheduler, to take control over that task and decide what it has been told to do during runtime.
That is, it updates its state, watches if time elapsed (if blocked via sleep()), and performs 
usual round-robin duty by jumping over blocked and exited tasks until it finds the one that is
ready to run. Then, the scheduler swaps context of the blocked/exited task with the one that is ready to run.
However, at this point it resulted in segfault.
We discussed it with fellow students but got no result.

Other functions were completed as well, but segfault gave no chance to try them. 