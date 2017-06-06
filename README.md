# HST - Heterogeneous Scheduling Task
A User Mode Scheduler for FreeRTOS.

## Overview
This project implements a user-level scheduler through a user-level task, called *Heterogeneous Scheduling Task* (HST), in order to support Mixed Critical Systems in a very flexible way, in the FreeRTOS Real-Time Operating System (www.freertos.org).

The design is documented with detail in: *FreeRTOS User Mode Scheduler for Mixed Critical Systems*, Francisco E. Páez, José M. Urriza, Ricardo Cayssials, Javier D. Orozco. 2015 Sixth Argentine Conference on Embedded Systems (CASE). ISBN 978-987-45523-4-1, pp. 37-42, 2015.

The `FreeRTOS` directory contains copies of tested FreeRTOS versions.

## Examples
The `EXAMPLES` directory has multiple example programs that implements various scheduling algorithms by means of the HST. See the `README` file in that directory for instructions on how to build them. Copies of the mbed Microcontroller Library for the various Cortex-M based development boards can be found in the `mbed` directory.

## COPYING
This software is licensed under the GNU General Public License v2.0. A copy of the license can be found in the `LICENSE` file.

FreeRTOS is Copyright (C) 2010 Real Time Engineers Ltd., and is licensed under a modified GNU General Public License (GPL). See: http://www.freertos.org/a00114.html

The mbed Microcontroller Library is Copyright (c) 2006-2013 ARM Limited, and is licensed under the Apache License, Version 2.0.
