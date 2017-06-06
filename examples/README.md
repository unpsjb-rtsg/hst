# Example Programs
Example projects that implements different scheduling policies using the User Mode Scheduler.

To choose which example project build, change the `HST_SCHED` parameter of the `Makefile.mine` file to one of these values:
* `dp`: Dual Priority scheduling.
* `edf`: Earliest Deadline First scheduling.
* `rm`: Rate Monotonic scheduling.
* `ss`: Rate Monotonic scheduling with Slack Stealing method.

All the examples implements a system with four periodic tasks, with periods of 3000, 4000, 6000 and 12000 *ticks*.

The `utils` directory contains utility functions used by these examples.

