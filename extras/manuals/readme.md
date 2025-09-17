# Manuals

This directory contains slides and videos for an OSP training and various documents.


## Training slides

Slides presented during training sessions:

- [ArduinoOSP-Training-Part1to8](ArduinoOSP-Training-Part1to8.pdf)  
  Slide (and exercises) for a one-day training on the Arduino OSP Evaluation kit.
  For the videos see the next section.

- [ArduinoOSP-Training-Appendix2-uniformcolors](ArduinoOSP-Training-Appendix2-uniformcolors.pdf)  
  Slides for a half-hour training on having the same color point across the chain irrespective of ambient conditions.
 
- [ArduinoOSP-Training-Appendix3-otpburn](ArduinoOSP-Training-Appendix3-otpburn.pdf)  
  Slides for a half-hour training on OTP burning of SAID.


## Training videos

Videos of training sessions (slides in previous section):

- [FHD](https://look.ams-osram.com/m/12d66825f5ad84fe/original/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-1.mp4)
  and
  [HD](https://look.ams-osram.com/asset/f163d697-3f52-432f-b9fe-090f63c256e8/mp4/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-1.mp4)
  video for part 1 _Prerequisite knowledge_.
 
- [FHD](https://look.ams-osram.com/m/28dd7fa04b7994fe/original/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-2.mp4iles)
  and
  [HD](https://look.ams-osram.com/asset/982d62e0-c819-4962-b11e-7a971e2b3052/mp4/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-2.mp4)
  video for part 2 _Boards in the Arduino OSP evaluation kit_.
 
- [FHD](https://look.ams-osram.com/m/22dc8cd7a5af6c15/original/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-3.mp4)
  and
  [HD](https://look.ams-osram.com/asset/5d00076b-e3b6-471c-9171-c9e1bb39ee31/mp4/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-3.mp4)
  video for part 3 _Libraries_.
 
- [FHD](https://look.ams-osram.com/m/1632d2912c44b9/original/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-4.mp4)
  and
  [HD](https://look.ams-osram.com/asset/c80001df-4dd0-41a1-9a6c-b71c7fc20ab6/mp4/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-4.mp4)
  video for part 4 _Telegrams_.
 
- [FHD](https://look.ams-osram.com/m/1bbd58b0e84f5a1f/original/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-5.mp4)
  and
  [HD](https://look.ams-osram.com/asset/4f3ccef5-4dda-4571-b394-43f6fdb85c8a/mp4/ams-OSRAM-Arduino-OSP-ecosystem-Evaluation-Kit-Training-Part-5.mp4)
  video for part 5 _I2C (or Telegrams part II)_.
 
- [FHD](https://look.ams-osram.com/m/757c2d23e2232e1b/original/ams-OSRAM-Arduino-OSP-ecosystem-evaluation-kit-Training-part-6.mp4)
  and
  [HD](https://look.ams-osram.com/asset/2975073f-3e80-4292-bc35-27686ef4dae4/mp4/ams-OSRAM-Arduino-OSP-ecosystem-evaluation-kit-Training-part-6.mp4)
  video for part 6 _Middleware (topo)_.
 
- [FHD](https://look.ams-osram.com/m/4afa509c33f8960a/original/ams-OSRAM-Arduino-OSP-ecosystem-evaluation-kit-Training-part-7-1.mp4)
  and
  [HD](https://look.ams-osram.com/asset/92a1be88-d2b3-4916-aa12-b99770d907e3/mp4/ams-OSRAM-Arduino-OSP-ecosystem-evaluation-kit-Training-part-7-1.mp4)
  video for part 7 _Command interpreter_ and part 8 _Miscellaneous_.


## User manuals

User manuals for demo applications:

- [User manual](saidbasic.pdf) for the [SAIDbasic](../../examples/saidbasic) application.
- [User manual](saiddemo.pdf) for the [SAIDdemo](../../examples/saiddemo) application.


## Documents

Documents on specific topics:

- How to determine the version of a [SAID](saidversions).
- Using a browser to [flash](webflash) a firmware image.
- [Getting started](../../gettingstarted.md) with the evaluation kit.
- Introduction to the [command interpreter](https://github.com/ams-OSRAM/OSP_aocmd?tab=readme-ov-file#example-commands).
- How to bring up your own hardware in example [aospi_bringup](https://github.com/ams-OSRAM/OSP_aospi/tree/main/examples/aospi_bringup).
 

## Examples

Every one of the 8 libraries comes with Arduino style examples. 
The OSP related ones are in:

- [aoosp](https://github.com/ams-OSRAM/OSP_aoosp?tab=readme-ov-file#examples) with e.g.
  CRC computation, blinky, drive current, SAID error reporting, OSP groups, 
  LED status, I2C, SYNC feature, (SAID) ADC, clustering, and using and burning OTP. 

- [aospi](https://github.com/ams-OSRAM/OSP_aospi?tab=readme-ov-file#examples) with e.g.
  low level blinky, bring-up, and MCU mode A.

- [aomw](https://github.com/ams-OSRAM/OSP_aomw?tab=readme-ov-file#examples) with e.g. 
  uniform colors and I2C EEPROM access.


(end)