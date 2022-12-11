<p align="center"><img src="buildroot/share/pixmaps/logo/flying_bear.png" height="250" style="@media (prefers-color-scheme: dark) {-webkit-filter: invert(100%); filter: invert(100%); }" alt="Flying Bear logo" /></p>

<h1 align="center">Flying Bear 3D Printer Custom Firmware</h1>

<p align="center">
    <a href="/LICENSE"><img alt="GPL-V3.0 License" src="https://img.shields.io/github/license/marlinfirmware/marlin.svg"></a>
    <a href="https://github.com/FB-G6/Marlin/releases"><img alt="Last Release Date" src="https://img.shields.io/github/release-date/FB-G6/Marlin"></a>
    <a href="https://github.com/FB-G6/Marlin/actions"><img alt="CI Status" src="https://github.com/FB-G6/Marlin/actions/workflows/release.yml/badge.svg"></a>
</p>

## FB-G6 (Marlin) description:

Custom firmware for FBG6 (ColorUI interface).
Changes in the firmware for comparison with the base marlin:

#### Changelog
### v1.7.0

1. Add input shaping
2. Fix bug enable\disable screen Input shaping

### v1.6.0

1. The screen is flipped to a vertical view.
2. Support WIFI and long names sent by WIFI.
3. LA included.
4. Added preheat screens for nozzle and table.
5. Manual 4-point table calibration (Bed traming) is enabled.
6. Selected PWM parameters for blower fans (reduced noise for stock fans).
7. Enabled mesh calibration of the table, which gives the parameter Bed Z (aka Z offset).
8. Babystep is saved automatically to Bed Z variable after every parking.
9. Added the function of manual replacement of the filament as on the drain (buttons E+, E-, Stop). By default extrudes 200mm bar. You can stop at any time with the Stop button.
10. Added Wi-Fi icon showing the ip address of the printer and the name of the network to which it is connected.

## Example Configurations

Before building Marlin you'll need to configure it for your specific hardware. Your vendor should have already provided source code with configurations for the installed firmware, but if you ever decide to upgrade you'll need updated configuration files. Marlin users have contributed dozens of tested example configurations to get you started. Visit the [MarlinFirmware/Configurations](https://github.com/MarlinFirmware/Configurations) repository to find the right configuration for your hardware.

## Building FB-G6 (Marlin)

**Install PlatformIO**

Linux:
```
source ~/.platformio/penv/bin/activate
platformio run -e mks_robin_nano_v3
```

Simulator mode (Linux):
```
# Activate virtual env
source ~/.platformio/penv/bin/activate
# Change Configuration.h un comment
# #define MOTHERBOARD BOARD_LINUX_RAMPS  // Config for Simulator mode
# Run and build simulator
platformio run -e simulator_linux_release
# Run simulator mode
.pio/build/simulator_linux_release/MarlinSimulator
```
### Supported 3D Printers

  Platform|3D printer
  --------|---
  [STM32F407 - STM32F417](https://www.st.com/en/microcontrollers-microprocessors/stm32f407-417.html) ARM® Cortex-M4|Flying bear Ghost 6

## FB-G6 (Marlin) Support

The Issue Queue is reserved for Bug Reports and Feature Requests. To get help with configuration and troubleshooting, please use the following resources:

- [FB-G6 (Marlin)](https://t.me/fbg6_s) - Discuss in Telegram issues with FB-G6 (Marlin) users and developers
- [Marlin Configuration](https://www.youtube.com/results?search_query=marlin+configuration) on YouTube
- [Marlin Documentation](https://marlinfw.org) - Official Marlin documentation

## Administration

Regular users can open and close their own issues, but only the administrators can do project-related things like add labels, merge changes, set milestones, and kick trolls. The current FB-G6 (Marlin) admin team consists of:

<table align="center">
<tr>
<td>

 🇷🇺  **Kirill**  
       [@adequator](https://github.com/adequator)  

</td>
<td>

 🇷🇺  **Pavel**  
       [@Zar_Asura](https://github.com/Zar-Asura)
 
</td>
<td>

 🇷🇺  **Vladimir**  
       [@big-vl](https://github.com/big-vl)
 
</td>
</tr>
</table>

## License

FB-G6 (Marlin) is published under the [GPL license](/LICENSE) because we believe in open development. The GPL comes with both rights and obligations. Whether you use FB-G6 (Marlin) firmware as the driver for your open or closed-source product, you must keep FB-G6 (Marlin) open, and you must provide your compatible FB-G6 (Marlin) source code to end users upon request. The most straightforward way to comply with the FB-G6 (Marlin) license is to make a fork of FB-G6 (Marlin) on Github, perform your modifications, and direct users to your modified fork.

While we can't prevent the use of this code in products (3D printers, CNC, etc.) that are closed source or crippled by a patent, we would prefer that you choose another firmware or, better yet, make your own.
