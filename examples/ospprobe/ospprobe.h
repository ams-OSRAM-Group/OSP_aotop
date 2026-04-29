// ospprobe.h - application to tap the telegrams running over an OSP chain
/*****************************************************************************
 * Copyright 2026 by ams OSRAM AG                                            *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************/
#ifndef _OSPPROBE_H_
#define _OSPPROBE_H_


// === Check build setting ==================================================


// Checking IDE settings (Tool menu)
#ifdef ARDUINO_ESP32S3_DEV
  // Good (Tools > Board: ESP32S3 Dev Module)
#else 
  // Bad (Tools > Board: xxx)
  #error Tools > Board: xxx   ===>   ESP32S3 Dev Module
#endif
#if ARDUINO_USB_CDC_ON_BOOT
  // Good (Tools > USB CDC On Boot: Enabled), needed for fast output
#else 
  // BAD (Tools > USB CDC On Boot: Disabled), output too slow
  #error Tools > USB CDC On Boot: Disabled   ===>   set to Enabled
#endif
#if ARDUINO_RUNNING_CORE==1
  // Good (Tools > Arduino Runs On: Core 1), needed, core 0 is for capturing
#else 
  // BAD (Tools > Arduino Runs On: Core 0), interferes with capturing
  #error Tools > Arduino Runs On: Core 0   ===>   Core 1
#endif


// === Version ==============================================================


// Application version (and its history)
#define OSPPROBE_VERSION "4.2"
// 20260403  4.2  Extra delay at startup (for Serial); hint on USB Serial
// 20260331  4.1  Throttled banner printing; change #warning to #error for config check
// 20260113  4.0  Renamed to ospprobe
// 20260107  3.5  Added capturing of responses
// 20260104  3.4  Switched to ESP32S3 SuperMini
// 20260104  3.3  Split off debug pins
// 20260101  3.2  Faster Manchester
// 20251223  3.1  Added Manchester
// 20251219  3.0  Added extra task pinned to core 0
// 20251219  2.0  Switched to new RMT interface
// 20251125  1.0  Created ospload


// Application long name
#define OSPPROBE_LONGNAME "OSPprobe"


// Application banner
#define OSPPROBE_BANNER OSPPROBE_BANNER1 OSPPROBE_BANNER2
#define OSPPROBE_BANNER1 "\n\n\n\n"\
  "  ____   _____ _____                 _\n"\
  " / __ \\ / ____|  __ \\               | |\n"\
  "| |  | | (___ | |__) | __  _ __ ___ | |__   ___\n"\
  "| |  | |\\___ \\|  ___/ '_ \\| '__/ _ \\| '_ \\ / _ \\\n"
#define OSPPROBE_BANNER2 \
  "| |__| |____) | |   | |_) | | | (_) | |_) |  __/\n"\
  " \\____/|_____/|_|   | .__/|_|  \\___/|_.__/ \\___|\n"\
  "                    | |\n"\
  "                    |_|\n"\
  // https://patorjk.com/software/taag/#p=display&v=2&f=Big&t=OSPprobe


#endif

