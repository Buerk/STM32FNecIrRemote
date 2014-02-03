/*
  ===================================================================
  * Memory layout for Debugging in RAM
  *
  * Internal memory map
  *   Region      Start           Size
  *   flash0      0x08000000      0x00100000
  *   sram0       0x20000000      0x00020000
  *   sram1       0x10000000      0x00010000
  *   
  *   rom is remapped to RAM
  ===================================================================
*/

MEMORY
{
	rom (rx)  : ORIGIN = 0x20000000, LENGTH = 0x00010000
	ram (rwx) : ORIGIN = 0x20010000, LENGTH = 0x00010000
	ram1 (rwx) : ORIGIN = 0x10000000, LENGTH = 0x00010000
}
