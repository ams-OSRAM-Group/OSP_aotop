// osplink.ino - sending telegrams from the PC (UART over USB to ESP)
/*****************************************************************************
 * Copyright 2024,2025 by ams OSRAM AG                                       *
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
#include <aospi.h>    // aospi_init()
#include <aoosp.h>    // aoosp_init()
#include <aocmd.h>    // aocmd_cint_pollserial()
#include <aomw.h>     // aomw_topo_cmd_register()
#include <aoui32.h>   // aoui32_oled_splash()
#include <aotop.h>    // AOTOP_VERSSION
#include "osplink.h"  // application info

/*
DESCRIPTION
This application allows the PC to send and receive OSP telegrams, using the
serial-over-USB connection from PC to OSP32 (use the USB plug labeled uart
of the ESP32 board). On the PC, use a terminal program like putty or 
the Arduino Serial Monitor to enter textual commands manually. It is also
possible to write a Python script to sends these commands (see for an example
https://github.com/ams-OSRAM/OSP_aocmd/tree/main/python), or even have
a windows GUI application that does that.

The commands have help built-in. Enter the "help" command to get a list
of command or "help xxx" to get help on command xxx. For an introduction
see the [aocmd library](..\..\aolibs\aocmd\readme.md).

HARDWARE
This sketch runs on the OSP32 board. Typically a demo board like 
SAIDbasic is connected.
In Arduino select board "ESP32S3 Dev Module".

NOTES
As an example, "file record" boot.cmd with (osp telegrams)
  osp send 000 reset
  osp send 001 initbidir
  osp send 000 clrerror
  osp send 000 goactive
  osp send 001 setpwmchn 00 FF 00 00 11 11 00 00
  osp send 002 setpwmchn 00 FF 00 00 00 00 11 11

Alternatively, use the low level commands (spi transfer)
  osp tx A0 00 00 22 // 000 reset
  osp tx A0 04 02 A9 // 001 initbidir
  osp tx A0 00 01 0D // 000 clrerror
  osp tx A0 00 05 B1 // 000 goactive
  osp tx A0 07 CF 00 FF 00 00 11 11 00 00 49 // 001 setpwmchn 00 grn
  osp tx A0 0B CF 00 FF 00 00 00 00 11 11 5D // 002 setpwmchn 00 blu

Alternatively, use higher level commands (topo)
  topo build
  topo pwm 0 0000 7fff 0000 // triplet 0 grn
  topo pwm 3 0000 0000 7fff // triplet 3 grn
*/


// Library aocmd "upcalls" via aocmd_version_app() to allow the application to print its version.
void aocmd_version_app() {
  Serial.printf("%s %s\n", OSPLINK_LONGNAME, OSPLINK_VERSION );
}


// Library aocmd "upcalls" via aocmd_version_extra() to allow the application to print the version of other ingredients.
void aocmd_version_extra() {
  Serial.printf( "aolibs  : mw %s ui32 %s top %s\n", AOMW_VERSION, AOUI32_VERSION, AOTOP_VERSION);
}


void setup() {
  // Identify over Serial
  Serial.begin(115200);
  do delay(250); while( ! Serial );
  Serial.printf(OSPLINK_BANNER);
  Serial.printf("%s - version %s\n\n", OSPLINK_LONGNAME, OSPLINK_VERSION);

  // Initialize all libraries
  aospi_init();
  aoosp_init();
  aocmd_init();
  aomw_init();
  aoui32_init();
  aocmd_register();
  aomw_topo_cmd_register();

  // Show end of init
  Serial.printf("\n");
  aoui32_oled_splash(OSPLINK_LONGNAME,OSPLINK_VERSION);

  // Start command interpreter
  aocmd_file_bootcmd_exec_on_por(); 
  Serial.printf( "Type 'help' for help\n" );
  aocmd_cint_prompt();
}


void loop() {
  // Process incoming commands
  aocmd_cint_pollserial();
}
