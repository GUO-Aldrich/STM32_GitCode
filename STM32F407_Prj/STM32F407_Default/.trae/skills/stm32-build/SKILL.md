---
name: "stm32-build"
description: "Build and flash STM32F407 projects with ARM GCC + OpenOCD. Invoke when user asks to compile, build, flash, download firmware, or after code changes need verification."
---

# STM32F407 Build & Flash Skill

This skill provides one-shot **compile â†?link â†?flash â†?verify** workflow for the STM32F407_Default project.

## Toolchain Paths

| Tool | Path |
|------|------|
| ARM GCC | `E:\ARM_GCC\bin\arm-none-eabi-gcc.exe` |
| OpenOCD | `C:\Users\Guo_JT\.eide\tools\openocd_7a1adfbec_mingw32\bin\openocd.exe` |
| Project Root | `c:\Users\Guo_JT\Desktop\STM32_GitCode\STM32F407_Prj\STM32F407_Default` |

## Memory Model

- MCU: **STM32F407VET6** (Cortex-M4, 168MHz, FPU)
- Flash: 512KB @ `0x08000000`
- SRAM: 128KB @ `0x20000000` + 64KB CCM @ `0x10000000`
- Clock: HSE â†?PLL â†?168MHz (APB1=42MHz, APB2=84MHz)
- Linker script: `STM32F407xx_FLASH.ld`

## Build Steps

### 1. Set Environment (Run Once Per Session)

Every terminal session, prepend toolchain to PATH:

```powershell
$env:Path = "E:\ARM_GCC\bin;C:\Users\Guo_JT\.eide\tools\openocd_7a1adfbec_mingw32\bin;" + $env:Path
Set-Location "c:\Users\Guo_JT\Desktop\STM32_GitCode\STM32F407_Prj\STM32F407_Default"
```

### 2. Compile All Sources

Use **PowerShell array syntax** to avoid quote-stripping bugs. Build macros:

```powershell
$CPU  = @('-mcpu=cortex-m4','-mthumb','-mfpu=fpv4-sp-d16','-mfloat-abi=hard')
$INC  = @('-ICore/Inc','-IDrivers/STM32F4xx_HAL_Driver/Inc','-IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy','-IDrivers/CMSIS/Device/ST/STM32F4xx/Include','-IDrivers/CMSIS/Include','-ISource/serial','-ISource/shell')
$DEF  = @('-DUSE_HAL_DRIVER','-DSTM32F407xx')
$CFL  = @('-std=c11','-Og','-Wall','-g','-ffunction-sections','-fdata-sections','-funsigned-char','--specs=nano.specs','--specs=nosys.specs')
$LDFL = @('-Wl,--gc-sections','-Wl,--print-memory-usage','-u','_printf_float','-u','_scanf_float','-lm','-T','STM32F407xx_FLASH.ld','-Wl,-Map=build/Debug/STM32F407_Default.map')
```

#### Compile a single C file:
```powershell
arm-none-eabi-gcc -c @CPU @INC @DEF @CFL -o build/Debug/.obj/Source/serial/cmd.o Source/serial/cmd.c
```

#### Compile single ASM:
```powershell
arm-none-eabi-gcc -c -x assembler-with-cpp @CPU @INC @DEF -g -o build/Debug/.obj/startup_stm32f407xx.o startup_stm32f407xx.s
```

#### Full rebuild (clean + all):
```powershell
Remove-Item build/Debug/.obj -Recurse -Force -ErrorAction SilentlyContinue
$sources = @(
  'Core/Src/gpio.c','Core/Src/main.c','Core/Src/stm32f4xx_hal_msp.c',
  'Core/Src/stm32f4xx_it.c','Core/Src/syscalls.c','Core/Src/sysmem.c',
  'Core/Src/system_stm32f4xx.c','Core/Src/usart.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c',
  'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c',
  'Source/serial/cmd.c','Source/serial/serial.c',
  'Source/shell/shell.c','Source/shell/shell_cmd_list.c',
  'Source/shell/shell_companion.c','Source/shell/shell_ext.c'
)
$objs = @()
foreach ($s in $sources) {
  $o = "build/Debug/.obj/$($s -replace '/','\'.Replace('.c','.o'))"
  New-Item -ItemType Directory -Force -Path (Split-Path $o) | Out-Null
  arm-none-eabi-gcc -c @CPU @INC @DEF @CFL -o $o $s
  if ($LASTEXITCODE -ne 0) { Write-Host "FAILED: $s"; exit 1 }
  $objs += $o
}
$asmObj = 'build/Debug/.obj/startup_stm32f407xx.o'
New-Item -ItemType Directory -Force -Path (Split-Path $asmObj) | Out-Null
arm-none-eabi-gcc -c -x assembler-with-cpp @CPU @INC @DEF -g -o $asmObj startup_stm32f407xx.s
$objs += $asmObj
```

### 3. Link & Post-Build

```powershell
arm-none-eabi-gcc @objs @CPU @LDFL -o build/Debug/STM32F407_Default.elf
arm-none-eabi-objcopy -O binary build/Debug/STM32F407_Default.elf build/Debug/STM32F407_Default.bin
arm-none-eabi-objcopy -O ihex   build/Debug/STM32F407_Default.elf build/Debug/STM32F407_Default.hex
arm-none-eabi-size build/Debug/STM32F407_Default.elf
```

Expected output from `size`:
```
   text    data     bss     dec     hex filename
  xxxxxx    xxxxx    xxxxx  xxxxxx  xxxxxxx build/Debug/STM32F407_Default.elf
```

### 4. Flash with OpenOCD

Connect via **CMSIS-DAP** debugger (interface=cmsis-dap-v1, target=stm32f4x):

```powershell
openocd -f interface/cmsis-dap-v1.cfg -f target/stm32f4x.cfg -c "program build/Debug/STM32F407_Default.elf verify reset exit"
```

For bin file (explicit base address):
```powershell
openocd -f interface/cmsis-dap-v1.cfg -f target/stm32f4x.cfg -c "program build/Debug/STM32F407_Default.bin verify 0x08000000 reset exit"
```

### 5. Verify Flash Readback

```powershell
openocd -f interface/cmsis-dap-v1.cfg -f target/stm32f4x.cfg -c "init; halt; flash verify_image build/Debug/STM32F407_Default.bin 0x08000000; exit"
```

### 6. Read Back Chip Info

```powershell
openocd -f interface/cmsis-dap-v1.cfg -f target/stm32f4x.cfg -c "init; halt; stm32f4x.cpu mww 0x40021000 0x45670123; exit"
```

## Shell Interaction (After Flash)

Open serial terminal on USART1 (PA9=TX, PA10=RX):
- Baud: **115200**, 8N1
- Use `minicom`, `putty`, `mobaxterm`, or VSCode Serial Monitor
- Built-in commands: `help`, `cmds`, `cfg`, `time`, `led`, `reboot`, `plus`, `Debug`

## Key Peripheral Summary

| Peripheral | Pins | Mode | Note |
|------------|------|------|------|
| USART1 | PA9(TX), PA10(RX) | AF7 | Shell debug, 115200 8N1 |
| GPIO | PB2, PC5 | Output PP | LED blink demo, toggled every 500ms |
| SWD | PA13, PA14 | Debug | Programming/debugging |

## Known Issues & Workarounds

| Issue | Fix |
|-------|-----|
| `led` shell command operated PD10/PD11 (not initialized) | Fixed in Source/serial/cmd.c to toggle PB2/PC5 instead |
| PLLQ=4 gives 84MHz (not 48MHz) | USB FS unusable; change PLL params if USB needed |
| unify_builder.exe needs .NET 6.0 | Use direct GCC commands above instead of EIDE builder |
| PowerShell strips quotes in single-string variables | Use **array syntax** `@('arg1','arg2')` for all flag groups |
| USART1 RX buffer is 32 bytes | Long input may truncate; shell input buffer is 512 bytes total |

## Quick Reference Cheatsheet

```powershell
# QUICK BUILD (modify as needed)
$env:Path="E:\ARM_GCC\bin;C:\Users\Guo_JT\.eide\tools\openocd_7a1adfbec_mingw32\bin;"+$env:Path
cd "c:\Users\Guo_JT\Desktop\STM32_GitCode\STM32F407_Prj\STM32F407_Default"

# COMPILE ONE FILE (e.g., after editing cmd.c)
$CPU=@('-mcpu=cortex-m4','-mthumb','-mfpu=fpv4-sp-d16','-mfloat-abi=hard')
$INC=@('-ICore/Inc','-IDrivers/STM32F4xx_HAL_Driver/Inc','-IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy','-IDrivers/CMSIS/Device/ST/STM32F4xx/Include','-IDrivers/CMSIS/Include','-ISource/serial','-ISource/shell')
$DEF=@('-DUSE_HAL_DRIVER','-DSTM32F407xx')
$CFL=@('-std=c11','-Og','-Wall','-g','-ffunction-sections','-fdata-sections','--specs=nano.specs','--specs=nosys.specs')

arm-none-eabi-gcc -c @CPU @INC @DEF @CFL -o build/Debug/.obj/Source/serial/cmd.o Source/serial/cmd.c

# LINK
$objs = Get-ChildItem build/Debug/.obj -Recurse -Filter *.o | % { $_.FullName.Replace('c:\Users\Guo_JT\Desktop\STM32_GitCode\STM32F407_Prj\STM32F407_Default\','') }
$LDFL=@('-Wl,--gc-sections','-u','_printf_float','-u','_scanf_float','-lm','-T','STM32F407xx_FLASH.ld')
arm-none-eabi-gcc @objs @CPU @LDFL -o build/Debug/STM32F407_Default.elf
arm-none-eabi-objcopy -O binary build/Debug/STM32F407_Default.elf build/Debug/STM32F407_Default.bin
arm-none-eabi-size build/Debug/STM32F407_Default.elf

# FLASH
openocd -f interface/cmsis-dap-v1.cfg -f target/stm32f4x.cfg -c "program build/Debug/STM32F407_Default.elf verify reset exit"
```