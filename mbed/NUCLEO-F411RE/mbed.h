/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MBED_H
#define MBED_H

#define MBED_LIBRARY_VERSION 141

#if MBED_CONF_RTOS_PRESENT
// RTOS present, this is valid only for mbed OS 5
#define MBED_MAJOR_VERSION 5
#define MBED_MINOR_VERSION 4
#define MBED_PATCH_VERSION 4

#else
// mbed 2
#define MBED_MAJOR_VERSION 2
#define MBED_MINOR_VERSION 0
#define MBED_PATCH_VERSION MBED_LIBRARY_VERSION
#endif

#define MBED_ENCODE_VERSION(major, minor, patch) ((major)*10000 + (minor)*100 + (patch))

#define MBED_VERSION MBED_ENCODE_VERSION(MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION)

#if MBED_CONF_RTOS_PRESENT
#include "rtos/rtos.h"
#endif

#if MBED_CONF_NSAPI_PRESENT
#include "netsocket/nsapi.h"
#endif

#if MBED_CONF_EVENTS_PRESENT
#include "events/mbed_events.h"
#endif

#if MBED_CONF_FILESYSTEM_PRESENT
#include "filesystem/mbed_filesystem.h"
#endif

#include "../NUCLEO-F411RE/platform/mbed_toolchain.h"
#include "../NUCLEO-F411RE/platform/platform.h"
#include "../NUCLEO-F411RE/platform/mbed_application.h"

// Useful C libraries
#include <math.h>
#include <time.h>

// mbed Debug libraries
#include "../NUCLEO-F411RE/platform/mbed_error.h"
#include "../NUCLEO-F411RE/platform/mbed_interface.h"
#include "../NUCLEO-F411RE/platform/mbed_assert.h"

// mbed Peripheral components
#include "../NUCLEO-F411RE/drivers/DigitalIn.h"
#include "../NUCLEO-F411RE/drivers/DigitalOut.h"
#include "../NUCLEO-F411RE/drivers/DigitalInOut.h"
#include "../NUCLEO-F411RE/drivers/BusIn.h"
#include "../NUCLEO-F411RE/drivers/BusOut.h"
#include "../NUCLEO-F411RE/drivers/BusInOut.h"
#include "../NUCLEO-F411RE/drivers/PortIn.h"
#include "../NUCLEO-F411RE/drivers/PortInOut.h"
#include "../NUCLEO-F411RE/drivers/PortOut.h"
#include "../NUCLEO-F411RE/drivers/AnalogIn.h"
#include "../NUCLEO-F411RE/drivers/AnalogOut.h"
#include "../NUCLEO-F411RE/drivers/PwmOut.h"
#include "../NUCLEO-F411RE/drivers/Serial.h"
#include "../NUCLEO-F411RE/drivers/SPI.h"
#include "../NUCLEO-F411RE/drivers/SPISlave.h"
#include "../NUCLEO-F411RE/drivers/I2C.h"
#include "../NUCLEO-F411RE/drivers/I2CSlave.h"
#include "../NUCLEO-F411RE/drivers/Ethernet.h"
#include "../NUCLEO-F411RE/drivers/CAN.h"
#include "../NUCLEO-F411RE/drivers/RawSerial.h"
#include "../NUCLEO-F411RE/drivers/FlashIAP.h"

// mbed Internal components
#include "../NUCLEO-F411RE/drivers/Timer.h"
#include "../NUCLEO-F411RE/drivers/Ticker.h"
#include "../NUCLEO-F411RE/drivers/Timeout.h"
#include "../NUCLEO-F411RE/drivers/LowPowerTimeout.h"
#include "../NUCLEO-F411RE/drivers/LowPowerTicker.h"
#include "../NUCLEO-F411RE/drivers/LowPowerTimer.h"
#include "../NUCLEO-F411RE/drivers/LocalFileSystem.h"
#include "../NUCLEO-F411RE/drivers/InterruptIn.h"
#include "../NUCLEO-F411RE/platform/mbed_wait_api.h"
#include "../NUCLEO-F411RE/hal/sleep_api.h"
#include "../NUCLEO-F411RE/platform/mbed_sleep.h"
#include "../NUCLEO-F411RE/platform/mbed_rtc_time.h"

// mbed Non-hardware components
#include "../NUCLEO-F411RE/platform/Callback.h"
#include "../NUCLEO-F411RE/platform/FunctionPointer.h"

using namespace mbed;
using namespace std;

#endif
