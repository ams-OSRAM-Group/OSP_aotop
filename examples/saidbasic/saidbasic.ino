// saidbasic.ino - the demo application for the SAIDbasic board
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
#include <aospi.h>     // aospi_init()
#include <aoosp.h>     // aoosp_init()
#include <aocmd.h>     // aocmd_cint_pollserial()
#include <aoui32.h>    // aoui32_oled_splash()
#include <aomw.h>      // aomw_init()
#include <aoapps.h>    // aoapps_mngr_start()
#include <aotop.h>     // AOTOP_VERSION
#include "saidbasic.h" // application info


/*
DESCRIPTION
This is the official demo of the SAIDbasic board. It contains 4 apps:
scripted animation (from EEPROM), running LED, flags selected by pressing 
a button, and a dithering demo. 

Press the A button to switch between them. The OLED shows the name of the 
running app. Some apps use the X and Y button for extra features, see the 
OLED for a short hint (sometimes long press repeats).

The green "OK" LED should blink (heartbeat); once the red "ERR" LED is on, 
an error occurred, the demo halts; and the OLED shows error details.

An option is to make a boot.cmd file (command "file record") with eg
  apps conf swflag set   dutch europe italy mali
  topo dim 200

HARDWARE
The demo should run on the OSP32 board, connected to the SAIDbasic board. 
Either have a terminator in the SAIDbasic OUT connector, or have a cable from 
the SAIDbasic OUT to the OSP32 IN connector. Optionally, plug an EEPROM with
an animation script in the SAIDbasic I2C header.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
The behavior depends on the app chosen and the buttons pressed.
See also user manual OSP_aotop\extras\manuals\saidbasic.pdf

OUTPUT
  _____         _____ _____  _               _
 / ____|  /\   |_   _|  __ \| |             (_)
| (___   /  \    | | | |  | | |__   __ _ ___ _  ___
 \___ \ / /\ \   | | | |  | | '_ \ / _` / __| |/ __|
 ____) / ____ \ _| |_| |__| | |_) | (_| \__ \ | (__
|_____/_/    \_\_____|_____/|_.__/ \__,_|___/_|\___|
SAIDbasic - version 2.4

spi: init
osp: init
cmd: init
ui32: init
mw: init
apps: init
cmds: registered
apps: registered

No 'boot.cmd' file available to execute
Type 'help' for help
>> aniscript: starting on 18 RGBs
aniscript: playing from EEPROM 50 on SAID 005 
*/


// Library aocmd "upcalls" via aocmd_version_app() to allow the application to print its version.
void aocmd_version_app() {
  Serial.printf( "%s %s\n", SAIDBASIC_LONGNAME, SAIDBASIC_VERSION );
}


// Library aocmd "upcalls" via aocmd_version_extra() to allow the application to print the version of other ingredients
void aocmd_version_extra() {
  Serial.printf( "aolibs  : mw %s, ui32 %s, apps %s, top %s\n", AOMW_VERSION, AOUI32_VERSION, AOAPPS_VERSION, AOTOP_VERSION);
}


// Pick commands that we want in this application
void cmds_register() {
  aocmd_register();           // include all standard commands from aocmd
  aomw_topo_cmd_register();   // include the topo command
  aoapps_mngr_cmd_register(); // include the apps (manager) command
  Serial.printf("cmds: registered\n");
}


// Pick apps that we want in this application
void apps_register() {
  aoapps_aniscript_register();
  aoapps_runled_register();
  aoapps_swflag_register();
  aoapps_dither_register();
  Serial.printf("apps: registered\n");
}


void setup() {
  // Identify over Serial
  Serial.begin(115200);
  do delay(250); while( ! Serial );
  Serial.printf(SAIDBASIC_BANNER);
  Serial.printf("%s - version %s\n\n", SAIDBASIC_LONGNAME, SAIDBASIC_VERSION);

  // Initialize all libraries
  aospi_init(); 
  aoosp_init();
  aocmd_init();
  aoui32_init();
  aomw_init();
  aoapps_init();
  cmds_register();
  apps_register();
  aoapps_swflag_resethw();
  
  // Show end of init
  Serial.printf("\n");
  aoui32_oled_splash(SAIDBASIC_LONGNAME,SAIDBASIC_VERSION); 
  delay(1000);

  // Start the first app
  aoapps_mngr_start();

  // Start command interpreter
  aocmd_file_bootcmd_exec_on_por(); 
  Serial.printf( "Type 'help' for help\n" );
  aocmd_cint_prompt();
}


void loop() {
  // Process incoming characters (commands)
  aocmd_cint_pollserial();

  // Check physical buttons
  aoui32_but_scan();

  // Switch to next app when A was pressed
  if( aoui32_but_wentdown(AOUI32_BUT_A) ) aoapps_mngr_switchnext();

  // Animation step in current application
  aoapps_mngr_step();
}



