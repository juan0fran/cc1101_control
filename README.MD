# Code for using CC1101 in a ISR based environment. 
ISR can be managed with IRQ handlers for the OS supported by the uC (as IRQ from STM32)
Mode is FIXED PACKET LENGTH at 255 bytes (default)
All other settings can be selected
SPI functions must be implemented depending on the OS