# RE:Flex Dance I/O v2 Firmware

## Source

This repository contains source files for the firmware of the RE:Flex Dance I/O board. I suggest the cross-platform editor [Visual Studio Code](https://code.visualstudio.com/) to work with this project. The environment used in this project can be configured using [this tutorial](https://hbfsrobotics.com/blog/configuring-vs-code-arm-development-stm32cubemx) as a template, without the need for the 'STM32 Workspace Setup' or 'CubeMX Blink Example' sections.

- **IO-Board.ioc / .mxproject** - These files are projects files for [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html). They are used in generating initial skeleton code of the project if a new project is desired from the base requirements.
- **Makefile / STM32F303CCTx_FLASH.ld / startup_stm32f303xc.s** - The makefile / linker script build and correctly flash the executable to the microcontroller. The startup script will initialize the microcontroller and its peripherals on boot, and will then jump to the main code execution.
- **STM32F303.svd** - This file contains a list of register maps and device information for the microcontroller. It allows the cortex-debug extension to monitor the microcontroller's internal values for the sake of debugging.
- **Src/Inc/Drivers Folders** - These are the source code files for the project. Everything in here is the meat of the project, driving peripherals and defining core behaviour. All code is eventually built into an executable that runs solely on the microcontroller in the I/O board.
- **.vscode** - This folder contains files for Visual Studio Code's plugins to build, debug and flash the project. I've included my setup as an example, however your files here may vary from mine depending on your build environment.

## Release

The release contains firmware to program the RE:Flex Dance I/O board. At current, this is best accomplished via an [ST-Link/V2 programmer](https://www.st.com/en/development-tools/st-link-v2.html). You can check the panel boards pinout to connect the device for flashing. The tutorial listed above also provides some methods for making/flashing the firmware via hotkeys in VS Code. 

## Future Improvements

- Implementation of USB firmware updates in firmware. Resetting the board and pointing it at the USB bootloader (along with necessary python interface updates) would allow the end user to easily update the firmware without using an ST-Link.
- Implementation of UART firmware update mechanism (along with the necessary board changes to the UART transceivers, and the python interface) will make the panel boards update-able via the I/O board. This would improve project accessibility for end users. This change would require storing a local copy of the firmware to flash onto the panel boards, which would be fine due to increased flash memory size on the I/O board. It would also require some further commands to receive from USB to jump into 'board programming' mode.
- A new UART addressing method will be required in order to daisy chain boards together. Multiprocessor mode on STM32 devices seems like a good candidate.
- The UART bus system has a limited command set. More commands could be implemented for testing, debugging, jumping into the UART programmer, resetting the boards, etc.

## License

For license details, see LICENSE file