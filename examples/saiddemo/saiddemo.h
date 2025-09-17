// saiddemo.h - the demo application for the Arduino OSP evaluation kit
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
#ifndef _SAIDDEMO_H_
#define _SAIDDEMO_H_


// Application version (and its history)
#define SAIDDEMO_VERSION "3.0"
// 20250806  3.0  Copied SAIDbasic to SAIDdemo (removing apps aniscript, dither, adding sensor)
// 20250725  2.7  Using new aomw_iox4b4l io aomw_iox.
// 20241017  2.6  Started app before command interpreter
// 20241010  2.5  Added aotop version
// 20240805  2.4  Added minimal documentation to top of ino
// 20240702  2.3  Reorder libs in aocmd_version_extra()
// 20240611  2.2  Added aocmd_version_extra()
// 20240609  2.1  Renamed command 'tele' to 'osp'
// 20240606  2.0  Added "aniscript"
// 20240531  1.5  Fixed bugs (osp info sync, dim up swflags)
// 20240530  1.4  Added app "swflag"
// 20240529  1.3  Added app "dither"
// 20240529  1.2  Added app "runled"
// 20240523  1.1  Added app-manager
// 20240523  1.0  Created


// Application long name
#define SAIDDEMO_LONGNAME "SAIDdemo"


// Application banner
#define SAIDDEMO_BANNER "\n\n\n\n"\
  "  _____         _____ _____      _                      \n"\
  " / ____|  /\\   |_   _|  __ \\    | |                     \n"\
  "| (___   /  \\    | | | |  | | __| | ___ _ __ ___   ___  \n"\
  " \\___ \\ / /\\ \\   | | | |  | |/ _` |/ _ \\ '_ ` _ \\ / _ \\ \n"\
  " ____) / ____ \\ _| |_| |__| | (_| |  __/ | | | | | (_) |\n"\
  "|_____/_/    \\_\\_____|_____/ \\__,_|\\___|_| |_| |_|\\___/ \n"\
  // https://patorjk.com/software/taag/#p=display&f=Big&t=SAIDdemo%0A
    

#endif

