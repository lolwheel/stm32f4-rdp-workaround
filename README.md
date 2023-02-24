# STM32F4 readout protection exploit

This repository contains an adaptation of the Flash Patch Breakpoint expoit originally described by Johannes Obermaier in https://www.usenix.org/system/files/woot20-paper-obermaier.pdf for STM32F1 family of embedded controllers.

I've verified that this exploit works. I've managed to dump an RDP level 1 protected STM32F415 controller.

## Things worth mentioning:
* The default baud rate of this rootshell is 256kb.
* The SRAM of F4 seems more sensitive to power loss. Data gets corrupted at room temprerature quickly so you'll have to freeze the chip right before glitching. An upside down dust blower pointed at the target chip works great for this.
* Do not bother sampling the NRST pin for power loss, just kill the power to the chip momentarily. I used an ESP8266 as an attack board and consecutive `digitalWrite(0); digitalWrite(1);` of the target chip power pins with no delay between them was consistently enough to reset it.
* Freezing the chip makes its internal oscillator deviate enoug to mangle the root shell UART output. Wait till the target chip warms back up to the room temperature. Keep typing "h" and hitting "Enter" in the terminal till you start seeing legible response from the root shell.

The code is heavily based on the proof-of-concept sample published by Johannes Obermaier at https://github.com/JohannesObermaier/f103-analysis/tree/master/h3/rootshell

Happy hacking.