STM32F4 NEC Remote Control Decoder
==================================

Example project for STM32F4-Discovery (STM32F407VG) to decode NEC IR remote control codes.  
Also a led toggles to show the program execution.
The only purpose of this program is to decode the transmitted address and data of NEC remote controllers which are needed for another project.

Connect a IR receiver (for example TSOP31236) at PC1.  
Set a breakpoint in main at line irdaData.newDat = 0.  
Examine the variables irdaData.adr and irdaData.dat.  
They conatin the sent address and the sent data.  


To use the external Compiler select Menu Settings->Tools->Global Compiler Settings
Select ARM GCC (generic) at the drop down menu
Select tab Toolchain executables and set the path to the compiler executable.
For example: C:\Program Files (x86)\GNU Tools ARM Embedded\4.8 2013q4\bin

Select tab Search directoroes and check the settings:  
- $(TARGET_COMPILER_DIR)\..\arm-none-eabi\include  
- $(TARGET_COMPILER_DIR)\..\arm-none-eabi  

If they're missing, add them.


Setting DEBUG -> Debug in RAM  
Setting RELEASE -> Debug or run in FLASH

The project is set up to run with a 8MHz oscillator as used on the STM32F4 Discovery board.
