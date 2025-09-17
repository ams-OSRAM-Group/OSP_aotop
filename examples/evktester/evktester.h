// evktester.h - test runner for EVK test cases
/*****************************************************************************
 * Copyright 2025 by ams OSRAM AG                                            *
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
#ifndef _EVKTESTER_H_
#define _EVKTESTER_H_


// Application version (and its history)
#define EVKTESTER_VERSION "1.0"
// 20250808  1.0  Added SAIDsense test, updated SAIDbasic (new iox4b4l)
// 20250325  0.7  Small typos fixed
// 20250324  0.6  Added two line instruction at 0.start
// 20250324  0.5  Added rainbow, bouncingblock, colormix
// 20250321  0.4  Added ioxled, ioxbut
// 20250321  0.3  Added usbpow, term2, loop, white, ops32i2c
// 20250320  0.2  Added structure, mac, renames
// 20250318  0.1  Created


// Application long name
#define EVKTESTER_LONGNAME "EVKtester"


// Application banner
#define EVKTESTER_BANNER "\n\n\n\n"\
  " ________      ___  ___            _\n"\
  "|  ____\\ \\    / / |/ / |          | |\n"\
  "| |__   \\ \\  / /| ' /| |_ ___  ___| |_ ___ _ __\n"\
  "|  __|   \\ \\/ / |  < | __/ _ \\/ __| __/ _ \\ '__|\n"\
  "| |____   \\  /  | . \\| ||  __/\\__ \\ ||  __/ |\n"\
  "|______|   \\/   |_|\\_\\\\__\\___||___/\\__\\___|_|\n"\
  // https://patorjk.com/software/taag/#p=display&v=2&f=Big&t=EVKtester


#endif

