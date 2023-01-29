/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfigPre.h"
#include "../../HAL/shared/Marduino.h"

#if HAS_UI_320x480

#include "ui_common.h"
#include "string.h"

using namespace std;

#include "../marlinui.h"
#include "../menu/menu.h"
#include "../../libs/numtostr.h"

#include "../../sd/cardreader.h"
#include "../../module/temperature.h"
#include "../../module/printcounter.h"
#include "../../module/planner.h"
#include "../../module/motion.h"
#include "../marlinui.h"

#ifdef MKS_WIFI_MODULE
  #include "../../module/wifi/wifi.h"
#endif

#if DISABLED(LCD_PROGRESS_BAR) && BOTH(FILAMENT_LCD_DISPLAY, SDSUPPORT)
  #include "../../feature/filwidth.h"
  #include "../../gcode/parser.h"
#endif

#if ENABLED(AUTO_BED_LEVELING_UBL)
  #include "../../feature/bedlevel/bedlevel.h"
#endif

void MarlinUI::tft_idle() {
  #if ENABLED(TOUCH_SCREEN)
    if (TERN0(HAS_TOUCH_SLEEP, lcd_sleep_task())) return;
    if (draw_menu_navigation) {
      add_control(48, 380, PAGE_UP, imgPageUp, encoderTopLine > 0);
      add_control(240, 380, PAGE_DOWN, imgPageDown, encoderTopLine + LCD_HEIGHT < screen_items);
      add_control(144, 380, BACK, imgBack);
      draw_menu_navigation = false;
    }
  #endif

  tft.queue.async();
  
  TERN_(TOUCH_SCREEN, if (tft.queue.is_empty()) touch.idle()); // Touch driver is not DMA-aware, so only check for touch controls after screen drawing is completed
  // TERN_(TOUCH_SCREEN, touch.idle());
}

#if ENABLED(SHOW_BOOTSCREEN)

  void MarlinUI::show_bootscreen() {
    uint32_t filesize = 2493587, progress = 1234567;
    uint32_t percent_done;
    // uint16_t width;
    // double fsize = file_size, csize = cur_size;
    char str[100];

    // fsize /= 1024*1024;
    // csize /= 1024*1024;

    percent_done = progress*100 / filesize;

    sprintf(str, "%3.2f of %3.2f MB (%d%%)", progress/1024./1024., filesize/1024./1024., (uint8_t)percent_done);


    tft.queue.reset();

    tft.canvas(0, 0, TFT_WIDTH, TFT_HEIGHT);
    #if ENABLED(BOOT_MARLIN_LOGO_SMALL)
      #define BOOT_LOGO_W 200   // FBLogo220x20x4
      #define BOOT_LOGO_H 220
      #define SITE_URL_Y (TFT_HEIGHT - 46)
      tft.set_background(COLOR_BACKGROUND);
    #else
      #define BOOT_LOGO_W TFT_WIDTH   // MarlinLogo480x320x16
      #define BOOT_LOGO_H TFT_HEIGHT
      #define SITE_URL_Y (TFT_HEIGHT - 52)
    #endif
    tft.add_image((TFT_WIDTH - BOOT_LOGO_W) / 2, (TFT_HEIGHT - BOOT_LOGO_H) / 2, imgBootScreen);
    // #ifdef WEBSITE_URL
    //   tft_string.set(WEBSITE_URL);
    //   tft.add_text(tft_string.center(TFT_WIDTH), SITE_URL_Y, COLOR_WEBSITE_URL, tft_string);
    // #endif

    // tft.add_text(20, 400, COLOR_WHITE, str);

    tft.queue.sync();
  }

  void MarlinUI::bootscreen_completion(const millis_t sofar) {
    if ((BOOTSCREEN_TIMEOUT) > sofar) safe_delay((BOOTSCREEN_TIMEOUT) - sofar);
    clear_lcd();
  }

#endif

void MarlinUI::draw_kill_screen() {
  tft.queue.reset();
  tft.fill(0, 0, TFT_WIDTH, TFT_HEIGHT, COLOR_KILL_SCREEN_BG);

  uint16_t line = 2;

  menu_line(line++, COLOR_KILL_SCREEN_BG);
  tft_string.set(status_message);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  line++;
  menu_line(line++, COLOR_KILL_SCREEN_BG);
  tft_string.set(GET_TEXT(MSG_HALTED));
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  menu_line(line++, COLOR_KILL_SCREEN_BG);
  tft_string.set(GET_TEXT(MSG_PLEASE_RESET));
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  tft.queue.sync();
}

void draw_heater_status(uint16_t x, uint16_t y, const int8_t Heater) {
  MarlinImage image = imgHotEnd;
  uint16_t Color;
  celsius_t currentTemperature, targetTemperature;

  if (Heater >= 0) { // HotEnd
    currentTemperature = thermalManager.wholeDegHotend(Heater);
    targetTemperature = thermalManager.degTargetHotend(Heater);
  }
  #if HAS_HEATED_BED
    else if (Heater == H_BED) {
      currentTemperature = thermalManager.wholeDegBed();
      targetTemperature = thermalManager.degTargetBed();
    }
  #endif
  #if HAS_TEMP_CHAMBER
    else if (Heater == H_CHAMBER) {
      currentTemperature = thermalManager.wholeDegChamber();
      #if HAS_HEATED_CHAMBER
        targetTemperature = thermalManager.degTargetChamber();
      #else
        targetTemperature = ABSOLUTE_ZERO;
      #endif
    }
  #endif
  #if HAS_TEMP_COOLER
    else if (Heater == H_COOLER) {
      currentTemperature = thermalManager.wholeDegCooler();
      targetTemperature = TERN(HAS_COOLER, thermalManager.degTargetCooler(), ABSOLUTE_ZERO);
    }
  #endif
  else return;

  TERN_(TOUCH_SCREEN, if (targetTemperature >= 0) touch.add_control(HEATER, x, y + 20, 64, 130, Heater));
  // if (targetTemperature >= 0) add_control(128, 220, heater_screen);          // HEATER !!!!!!!!!!!!!!
  tft.canvas(x, y + 20, 64, 130);
  tft.set_background(COLOR_BACKGROUND);

  Color = currentTemperature < 0 ? COLOR_INACTIVE : COLOR_COLD;

  if (Heater >= 0) { // HotEnd
    if (currentTemperature >= 50) Color = COLOR_HOTEND;
  }
  #if HAS_HEATED_BED
    else if (Heater == H_BED) {
      if (currentTemperature >= 50) Color = COLOR_HEATED_BED;
      image = targetTemperature > 0 ? imgBedHeated : imgBed;
    }
  #endif
  #if HAS_TEMP_CHAMBER
    else if (Heater == H_CHAMBER) {
      if (currentTemperature >= 50) Color = COLOR_CHAMBER;
      image = targetTemperature > 0 ? imgChamberHeated : imgChamber;
    }
  #endif
  #if HAS_TEMP_COOLER
    else if (Heater == H_COOLER) {
      if (currentTemperature <= 26) Color = COLOR_COLD;
      if (currentTemperature > 26) Color = COLOR_RED;
      image = targetTemperature > 26 ? imgCoolerHot : imgCooler;
    }
  #endif

  tft.add_image(0, 28, image, Color);

  tft_string.set(i16tostr3rj(currentTemperature));
  tft_string.add(LCD_STR_DEGREE);
  tft_string.trim();
  tft.add_text(tft_string.center(64) + 2, 82, Color, tft_string);

  if (targetTemperature >= 0) {
    tft_string.set(i16tostr3rj(targetTemperature));
    tft_string.add(LCD_STR_DEGREE);
    tft_string.trim();
    tft.add_text(tft_string.center(64) + 2, 8, Color, tft_string);
  }
}

void draw_fan_status(uint16_t x, uint16_t y, const bool blink) {
  TERN_(TOUCH_SCREEN, touch.add_control(FAN, x, y + 32, 64, 130));                                               //FAN !!!!!!!!!!!!!!!!!!!!!!!
  tft.canvas(x, y + 32, 66, 130);
  tft.set_background(COLOR_BACKGROUND);

  uint8_t fanSpeed = thermalManager.fan_speed[0];
  MarlinImage image;

  if (fanSpeed >= 127)
    image = blink ? imgFanFast1 : imgFanFast0;
  else if (fanSpeed > 0)
    image = blink ? imgFanSlow1 : imgFanSlow0;
  else
    image = imgFanIdle;

  tft.add_image(0, 10, image, COLOR_FAN);

  tft_string.set(ui8tostr4pctrj(thermalManager.fan_speed[0]));
  tft_string.trim();
  tft.add_text(tft_string.center(64) + 6, 68, COLOR_FAN, tft_string);
}

void MarlinUI::draw_status_screen() {
  const bool blink = get_blink();

  TERN_(TOUCH_SCREEN, touch.clear());

  // heaters and fan
  uint16_t i, x, y = TFT_STATUS_TOP_Y;

  for (i = 0 ; i < ITEMS_COUNT; i++) {
    x = (TFT_WIDTH / ITEMS_COUNT - 80) / 2  + (TFT_WIDTH * i / ITEMS_COUNT);
    switch (i) {
      #ifdef ITEM_E0
        case ITEM_E0: draw_heater_status(x, y, H_E0); break;
      #endif
      #ifdef ITEM_E1
        case ITEM_E1: draw_heater_status(x, y, H_E1); break;
      #endif
      #ifdef ITEM_E2
        case ITEM_E2: draw_heater_status(x, y, H_E2); break;
      #endif
      #ifdef ITEM_BED
        case ITEM_BED: draw_heater_status(x, y, H_BED); break;
      #endif
      #ifdef ITEM_CHAMBER
        case ITEM_CHAMBER: draw_heater_status(x, y, H_CHAMBER); break;
      #endif
      #ifdef ITEM_COOLER
        case ITEM_COOLER: draw_heater_status(x, y, H_COOLER); break;
      #endif
      #ifdef ITEM_FAN
        case ITEM_FAN: draw_fan_status(x, y, blink); break;
      #endif
    }
  }

  y += TERN(HAS_UI_480x272, 118, 128);

  // coordinates
  tft.canvas(5, 155, 300, 36);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_rectangle(0, 0, 300, 36, COLOR_AXIS_HOMED);
  tft_string.set(' ');

  uint16_t color_feedrate = feedrate_percentage == 100 ? COLOR_RATE_100 : COLOR_RATE_ALTERED;
  tft.add_image(5, 3, imgFeedRate, color_feedrate);  
  tft.add_text(40, 3, COLOR_WHITE, i16tostr3rj(feedrate_percentage));
  // tft_string.add('%');


  if (TERN0(LCD_SHOW_E_TOTAL, printingIsActive())) {
    #if ENABLED(LCD_SHOW_E_TOTAL)
      tft.add_text( 10, 3, COLOR_AXIS_HOMED , "E");
      const uint8_t escale = e_move_accumulator >= 100000.0f ? 10 : 1; // After 100m switch to cm
      tft_string.set(ftostr4sign(e_move_accumulator / escale));
      tft_string.add(escale == 10 ? 'c' : 'm');
      tft_string.add('m');
      tft.add_text(127 - tft_string.width(), 3, COLOR_AXIS_HOMED, tft_string);
    #endif
  }
  else {
    // tft.add_text( 10, 3, COLOR_AXIS_HOMED , "X");
    // const bool nhx = axis_should_home(X_AXIS);
    // tft_string.set(blink && nhx ? "?" : ftostr4sign(LOGICAL_X_POSITION(current_position.x)));
    // tft.add_text( 68 - tft_string.width(), 3, nhx ? COLOR_AXIS_NOT_HOMED : COLOR_AXIS_HOMED, tft_string);

    // tft.add_text(115, 3, COLOR_AXIS_HOMED , "Y");
    // const bool nhy = axis_should_home(Y_AXIS);
    // tft_string.set(blink && nhy ? "?" : ftostr4sign(LOGICAL_Y_POSITION(current_position.y)));
    // tft.add_text(185 - 12 - tft_string.width(), 3, nhy ? COLOR_AXIS_NOT_HOMED : COLOR_AXIS_HOMED, tft_string);
  }
  tft.add_text(110, 3, COLOR_AXIS_HOMED , "Z");
  uint16_t offset = 32;
  const bool nhz = axis_should_home(Z_AXIS);
  if (blink && nhz)
    tft_string.set('?');
  else {
    const float z = LOGICAL_Z_POSITION(current_position.z);
    tft_string.set(ftostr52sp((int16_t)z));
    tft_string.rtrim();
    offset += tft_string.width();
    tft_string.set(ftostr52sp(z));
    offset -= tft_string.width();
  }
  tft.add_text(192 - tft_string.width() - offset, 3, nhz ? COLOR_AXIS_NOT_HOMED : COLOR_AXIS_HOMED, tft_string);

    // tft.add_text(135, 3, COLOR_WHITE, (ftostr52sp(z)));
 
  uint16_t color_flowrate = planner.flow_percentage[0] == 100 ? COLOR_RATE_100 : COLOR_RATE_ALTERED;
  tft.add_image(210, 3, imgFlowRate, color_flowrate);
  tft.add_text(250, 3, COLOR_WHITE, i16tostr3rj(planner.flow_percentage[active_extruder]));

  y += TERN(HAS_UI_480x272, 38, 48);
  // flow rate

  const uint8_t progress = ui.get_progress_percent();
  char buffer[30];
  duration_t elapsed = print_job_timer.duration();
  #if ENABLED(SHOW_REMAINING_TIME)
    if(progress == 0 || (elapsed.second() / 10) % 2 == 1){
      elapsed.toDigital(buffer);
    }else{
      duration_t remaining = ui.get_remaining_time();
      remaining.toDigital(buffer, false, true);
    }
  #else
      elapsed.toDigital(buffer);
  #endif
  #if ENABLED(TOUCH_SCREEN)
    // TERN_(TOUCH_SCREEN, add_control(128, 200, PAUSE_PRINT, imgTramming, COLOR_WHITE)); //Test Images
    if (printingIsActive() || printingIsPaused()) {
      tft.canvas(0, 340, 320, 50);
      tft.set_background(COLOR_BACKGROUND);
      tft_string.set("Print Time: ");
      tft_string.add(buffer);
      tft.add_text(tft_string.center(320), 10, COLOR_LIGHT_BLUE, tft_string);                    
      tft.canvas(4, 390, 312, 37);
      tft.set_background(COLOR_PROGRESS_BG);
      tft.add_rectangle(0, 0, 312, 37, COLOR_PROGRESS_FRAME);
      if (progress)
        tft.add_bar(1, 1, (310 * progress) / 100, 35, COLOR_PROGRESS_BAR);

      tft_string.set(i16tostr3rj(progress));
      tft_string.add("%");
      tft.add_text(tft_string.center(312), 2, COLOR_WHITE, tft_string);

      if (!printingIsPaused()) {
        TERN_(TOUCH_SCREEN, add_control(20, 200, PAUSE_PRINT, imgPause));
      } else {
        TERN_(TOUCH_SCREEN, add_control(20, 200, RESUME_PRINT, imgResume));
      }
      TERN_(TOUCH_SCREEN, add_control(128, 200, STOP_PRINT, imgStop, true, COLOR_CORAL_RED));
      add_control(236, 200, menu_tune, imgTune);
      TERN_(TOUCH_SCREEN, add_control(128, 280, BABYSTEP_BUTTON, imgBabystep));
    } else {
      // TERN_(TOUCH_SCREEN, add_control(128, 200, BED_Z, imgBedZ, COLOR_WHITE));
      add_control(236, 200, menu_main, imgSettings);
      TERN_(TOUCH_SCREEN, add_control(128, 360, TRAMMING, imgTramming));
      TERN_(TOUCH_SCREEN, add_control(20, 360, BED_Z, imgZoffset));
      #ifdef PSU_CONTROL
        TERN_(TOUCH_SCREEN, add_control(236, 360, POWER_OFF, imgPower));
      #endif
      #ifdef MKS_WIFI_MODULE
        add_control(128, 200, wifi_screen, imgWifi, wifi_link_state == WIFI_CONNECTED);
      #endif
      TERN_(TOUCH_SCREEN, add_control(128, 280, MOVE_AXIS, imgMove));
      #if ENABLED(SDSUPPORT)
        const bool cm = card.isMounted(), pa = printingIsActive();
        add_control(20, 200, menu_media, imgSD, cm && !pa, COLOR_CONTROL_ENABLED, cm && pa ? COLOR_BUSY : COLOR_CONTROL_DISABLED);
      #endif
    }
    // TERN_(SDSUPPORT, add_control(20, 200, menu_media, imgSD, !printingIsActive(), COLOR_CONTROL_ENABLED, card.isMounted() && printingIsActive() ? COLOR_BUSY : COLOR_CONTROL_DISABLED));
  #endif                                                                                                  


TERN_(TOUCH_SCREEN, add_control(20, 280, FEEDRATE, imgFeedRateBig));
TERN_(TOUCH_SCREEN, add_control(236, 280, FLOWRATE, imgFlowRateBig));

  // print duration
  // const uint8_t progress = ui.get_progress_percent();
  // char buffer[30];
  // duration_t elapsed = print_job_timer.duration();
  // #if ENABLED(SHOW_REMAINING_TIME)
  //   if(progress == 0 || (elapsed.second() / 10) % 2 == 1){
  //     elapsed.toDigital(buffer);
  //   }else{
  //     duration_t remaining = ui.get_remaining_time();
  //     remaining.toDigital(buffer, false, true);
  //   }
  // #else
  //   elapsed.toDigital(buffer);
  // #endif

  // sprintf_P(buffer, PSTR("%s    %d %%"), buffer, progress);
  // if (printingIsActive() || printingIsPaused()) {
    // tft.canvas(4, 350, 312, 30);
    // tft.set_background(COLOR_BACKGROUND);
    // tft_string.set("Print Time: ");
    // tft_string.add(buffer);
    // tft.add_text(tft_string.center(312), 2, COLOR_LIGHT_BLUE, tft_string);


    y += TERN(HAS_UI_480x272, 28, 36);
    // progress bar
    // const uint8_t progress = ui.get_progress_percent();
    // tft.canvas(4, 390, 312, 37);
    // tft.set_background(COLOR_PROGRESS_BG);
    // tft.add_rectangle(0, 0, 312, 37, COLOR_PROGRESS_FRAME);
    // if (progress)
    //   tft.add_bar(1, 1, (310 * progress) / 100, 35, COLOR_PROGRESS_BAR);

    // tft_string.set(i16tostr3rj(progress));
    // tft_string.add("%");
    // tft.add_text(tft_string.center(312), 2, COLOR_WHITE, tft_string);

    y += 20;
  // }
  // status message
  // TERN_(MKS_WIFI_MODULE, touch.add_control(WIFI, 0, 420, 320, 30, 0));          // IP !!!!!!!!!!!!!!

  tft.canvas(0, 435, 320, 35);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_rectangle(10, 0, 300, 1, COLOR_WHITE);
  tft_string.set(status_message);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 5, COLOR_STATUS_MESSAGE, tft_string);
}

// Low-level draw_edit_screen can be used to draw an edit screen from anyplace
void MenuEditItemBase::draw_edit_screen(FSTR_P const fstr, const char * const value/*=nullptr*/) {
  ui.encoder_direction_normal();
  TERN_(TOUCH_SCREEN, touch.clear());

  uint16_t line = 1;

  menu_line(line++);
  tft_string.set(fstr, itemIndex, itemStringC, itemStringF);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);

  TERN_(AUTO_BED_LEVELING_UBL, if (ui.external_control) line++);  // ftostr52() will overwrite *value so *value has to be displayed first

  menu_line(line);
  tft_string.set(value);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

  #if ENABLED(AUTO_BED_LEVELING_UBL)
    if (ui.external_control) {
      menu_line(line - 1);

      tft_string.set(X_LBL);
      tft.add_text((TFT_WIDTH / 2 - 120), MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
      tft_string.set(ftostr52(LOGICAL_X_POSITION(current_position.x)));
      tft_string.trim();
      tft.add_text((TFT_WIDTH / 2 - 16) - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

      tft_string.set(Y_LBL);
      tft.add_text((TFT_WIDTH / 2 + 16), MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
      tft_string.set(ftostr52(LOGICAL_X_POSITION(current_position.y)));
      tft_string.trim();
      tft.add_text((TFT_WIDTH / 2 + 120) - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);
    }
  #endif

  if (ui.can_show_slider()) {

    #define SLIDER_LENGTH 224
    #define SLIDER_Y_POSITION 220

    tft.canvas((TFT_WIDTH - SLIDER_LENGTH) / 2, SLIDER_Y_POSITION, SLIDER_LENGTH, 16);
    tft.set_background(COLOR_BACKGROUND);

    int16_t position = (SLIDER_LENGTH - 2) * ui.encoderPosition / maxEditValue;
    tft.add_bar(0, 7, 1, 2, ui.encoderPosition == 0 ? COLOR_SLIDER_INACTIVE : COLOR_SLIDER);
    tft.add_bar(1, 6, position, 4, COLOR_SLIDER);
    tft.add_bar(position + 1, 6, SLIDER_LENGTH - 2 - position, 4, COLOR_SLIDER_INACTIVE);
    tft.add_bar(SLIDER_LENGTH - 1, 7, 1, 2, int32_t(ui.encoderPosition) == maxEditValue ? COLOR_SLIDER : COLOR_SLIDER_INACTIVE);

    #if ENABLED(TOUCH_SCREEN)
      tft.add_image((SLIDER_LENGTH - 8) * ui.encoderPosition / maxEditValue, 0, imgSlider, COLOR_SLIDER);
      touch.add_control(SLIDER, (TFT_WIDTH - SLIDER_LENGTH) / 2, SLIDER_Y_POSITION - 8, SLIDER_LENGTH, 60, maxEditValue);
    #endif
  }

  tft.draw_edit_screen_buttons();
}

void TFT::draw_edit_screen_buttons() {
  #if ENABLED(TOUCH_SCREEN)
    add_control(32, TFT_HEIGHT - 64, DECREASE, imgDecrease);
    add_control(224, TFT_HEIGHT - 64, INCREASE, imgIncrease);
    add_control(128, TFT_HEIGHT - 64, CLICK, imgConfirm);
  #endif
}

// The Select Screen presents a prompt and two "buttons"
void MenuItem_confirm::draw_select_screen(FSTR_P const yes, FSTR_P const no, const bool yesno, FSTR_P const pref, const char * const string/*=nullptr*/, FSTR_P const suff/*=nullptr*/) {
  uint16_t line = 1;

  if (!string) line++;

  menu_line(line++);
  tft_string.set(pref);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  if (string) {
    menu_line(line++);
    tft_string.set(string);
    tft_string.trim();
    tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);
  }

  if (suff) {
    menu_line(line);
    tft_string.set(suff);
    tft_string.trim();
    tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);
  }
  #if ENABLED(TOUCH_SCREEN)
    if (no)  add_control( 48, TFT_HEIGHT - 64, CANCEL,  imgCancel,  true, yesno ? HALF(COLOR_CONTROL_CANCEL) : COLOR_CONTROL_CANCEL);
    if (yes) add_control(208, TFT_HEIGHT - 64, CONFIRM, imgConfirm, true, yesno ? COLOR_CONTROL_CONFIRM : HALF(COLOR_CONTROL_CONFIRM));
  #endif
}

#if ENABLED(ADVANCED_PAUSE_FEATURE)

  void MarlinUI::draw_hotend_status(const uint8_t row, const uint8_t extruder) {
    #if ENABLED(TOUCH_SCREEN)
      touch.clear();
      draw_menu_navigation = false;
      touch.add_control(RESUME_CONTINUE , 0, 0, TFT_WIDTH, TFT_HEIGHT);
    #endif

    menu_line(row);
    tft_string.set(GET_TEXT(MSG_FILAMENT_CHANGE_NOZZLE));
    tft_string.add('E');
    tft_string.add((char)('1' + extruder));
    tft_string.add(' ');
    tft_string.add(i16tostr3rj(thermalManager.wholeDegHotend(extruder)));
    tft_string.add(LCD_STR_DEGREE);
    tft_string.add(F(" / "));
    tft_string.add(i16tostr3rj(thermalManager.degTargetHotend(extruder)));
    tft_string.add(LCD_STR_DEGREE);
    tft_string.trim();
    tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);
  }

#endif // ADVANCED_PAUSE_FEATURE

#if ENABLED(AUTO_BED_LEVELING_UBL)
  #define GRID_OFFSET_X   8
  #define GRID_OFFSET_Y   8
  #define GRID_WIDTH      192
  #define GRID_HEIGHT     192
  #define CONTROL_OFFSET  16

  void MarlinUI::ubl_plot(const uint8_t x_plot, const uint8_t y_plot) {

    tft.canvas(GRID_OFFSET_X, GRID_OFFSET_Y, GRID_WIDTH, GRID_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft.add_rectangle(0, 0, GRID_WIDTH, GRID_HEIGHT, COLOR_WHITE);

    for (uint16_t x = 0; x < (GRID_MAX_POINTS_X); x++)
      for (uint16_t y = 0; y < (GRID_MAX_POINTS_Y); y++)
        if (position_is_reachable({ bedlevel.get_mesh_x(x), bedlevel.get_mesh_y(y) }))
          tft.add_bar(1 + (x * 2 + 1) * (GRID_WIDTH - 4) / (GRID_MAX_POINTS_X) / 2, GRID_HEIGHT - 3 - ((y * 2 + 1) * (GRID_HEIGHT - 4) / (GRID_MAX_POINTS_Y) / 2), 2, 2, COLOR_UBL);

    tft.add_rectangle((x_plot * 2 + 1) * (GRID_WIDTH - 4) / (GRID_MAX_POINTS_X) / 2 - 1, GRID_HEIGHT - 5 - ((y_plot * 2 + 1) * (GRID_HEIGHT - 4) / (GRID_MAX_POINTS_Y) / 2), 6, 6, COLOR_UBL);

    const xy_pos_t pos = { bedlevel.get_mesh_x(x_plot), bedlevel.get_mesh_y(y_plot) },
                   lpos = pos.asLogical();

    tft.canvas(320, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2 - MENU_ITEM_HEIGHT, 120, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(X_LBL);
    tft.add_text(0, MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
    tft_string.set(ftostr52(lpos.x));
    tft_string.trim();
    tft.add_text(120 - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    tft.canvas(320, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2, 120, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(Y_LBL);
    tft.add_text(0, MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
    tft_string.set(ftostr52(lpos.y));
    tft_string.trim();
    tft.add_text(120 - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    tft.canvas(320, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2 + MENU_ITEM_HEIGHT, 120, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(Z_LBL);
    tft.add_text(0, MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
    tft_string.set(isnan(bedlevel.z_values[x_plot][y_plot]) ? "-----" : ftostr43sign(bedlevel.z_values[x_plot][y_plot]));
    tft_string.trim();
    tft.add_text(120 - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    constexpr uint8_t w = (TFT_WIDTH) / 10;
    tft.canvas(GRID_OFFSET_X + (GRID_WIDTH - w) / 2, GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET - 5, w, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(ui8tostr3rj(x_plot));
    tft_string.trim();
    tft.add_text(tft_string.center(w), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    tft.canvas(GRID_OFFSET_X + GRID_WIDTH + CONTROL_OFFSET + 16 - 24, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2, w, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(ui8tostr3rj(y_plot));
    tft_string.trim();
    tft.add_text(tft_string.center(w), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    #if ENABLED(TOUCH_SCREEN)
      touch.clear();
      draw_menu_navigation = false;
      add_control(GRID_OFFSET_X + GRID_WIDTH + CONTROL_OFFSET,      GRID_OFFSET_Y + CONTROL_OFFSET,                    UBL,  (ENCODER_STEPS_PER_MENU_ITEM) * (GRID_MAX_POINTS_X), imgUp);
      add_control(GRID_OFFSET_X + GRID_WIDTH + CONTROL_OFFSET,      GRID_OFFSET_Y + GRID_HEIGHT - CONTROL_OFFSET - 32, UBL, -(ENCODER_STEPS_PER_MENU_ITEM) * (GRID_MAX_POINTS_X), imgDown);
      add_control(GRID_OFFSET_X + CONTROL_OFFSET,                   GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET,      UBL, -(ENCODER_STEPS_PER_MENU_ITEM), imgLeft);
      add_control(GRID_OFFSET_X + GRID_WIDTH - CONTROL_OFFSET - 32, GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET,      UBL,   ENCODER_STEPS_PER_MENU_ITEM, imgRight);
      add_control(320, GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET, CLICK, imgLeveling);
      add_control(224, TFT_HEIGHT - 34, BACK, imgBack);
    #endif
  }
#endif // AUTO_BED_LEVELING_UBL

#if ENABLED(BABYSTEP_ZPROBE_OFFSET)
  #include "../../feature/babystep.h"
#endif

#if HAS_BED_PROBE
  #include "../../module/probe.h"
#endif

#define Z_SELECTION_Z 1
#define Z_SELECTION_Z_PROBE -1

struct MotionAxisState {
  xy_int_t xValuePos, yValuePos, zValuePos, eValuePos, stepValuePos, zTypePos, eNamePos;
  float currentStepSize = 10.0;
  int z_selection = Z_SELECTION_Z;
  uint8_t e_selection = 0;
  bool blocked = false;
  char message[32];
};

MotionAxisState motionAxisState;

#define E_BTN_COLOR COLOR_YELLOW
#define X_BTN_COLOR COLOR_CORAL_RED
#define Y_BTN_COLOR COLOR_VIVID_GREEN
#define Z_BTN_COLOR COLOR_LIGHT_BLUE

#define BTN_WIDTH 64                          //48
#define BTN_HEIGHT 52                         //39
#define X_MARGIN 20                           //15
#define Y_MARGIN 15                           //11

static void quick_feedback() {
  #if HAS_CHIRP
    ui.chirp(); // Buzz and wait. Is the delay needed for buttons to settle?
    #if BOTH(HAS_MARLINUI_MENU, HAS_BEEPER)
      for (int8_t i = 5; i--;) { buzzer.tick(); delay(2); }
    #elif HAS_MARLINUI_MENU
      delay(10);
    #endif
  #endif
}

#define CUR_STEP_VALUE_WIDTH 44     //38
static void drawCurStepValue() {
  tft_string.set(ftostr52sp(motionAxisState.currentStepSize));
  tft.canvas(motionAxisState.stepValuePos.x, motionAxisState.stepValuePos.y, CUR_STEP_VALUE_WIDTH+14, 24);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_text(CUR_STEP_VALUE_WIDTH - tft_string.width(), 0, COLOR_AXIS_HOMED, tft_string);
  tft.queue.sync();
  tft_string.set(F("mm"));
  tft.canvas(motionAxisState.stepValuePos.x, motionAxisState.stepValuePos.y + 24, CUR_STEP_VALUE_WIDTH, 24);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_text(CUR_STEP_VALUE_WIDTH - tft_string.width(), 0, COLOR_AXIS_HOMED, tft_string);
}

// static void drawCurZSelection() {
//   tft_string.set('Z');
//   tft.canvas(motionAxisState.zTypePos.x, motionAxisState.zTypePos.y, tft_string.width(), 24);
//   tft.set_background(COLOR_BACKGROUND);
//   tft.add_text(0, 0, Z_BTN_COLOR, tft_string);
//   tft.queue.sync();
//   tft_string.set(F("Offset"));
//   tft.canvas(motionAxisState.zTypePos.x, motionAxisState.zTypePos.y + 34, tft_string.width(), 24);
//   tft.set_background(COLOR_BACKGROUND);
//   if (motionAxisState.z_selection == Z_SELECTION_Z_PROBE) {
//     tft.add_text(0, 0, Z_BTN_COLOR, tft_string);
//   }
// }


// static void drawCurESelection() {
//   tft.canvas(motionAxisState.eNamePos.x, motionAxisState.eNamePos.y, BTN_WIDTH, BTN_HEIGHT);
//   tft.set_background(COLOR_BACKGROUND);
//   tft_string.set('E');
//   tft.add_text(0, 0, E_BTN_COLOR , tft_string);
//   tft.add_text(tft_string.width(), 0, E_BTN_COLOR, ui8tostr3rj(motionAxisState.e_selection));
// }

static void drawMessage(PGM_P const msg) {
  tft.canvas(X_MARGIN, TFT_HEIGHT - Y_MARGIN - 28, TFT_WIDTH - X_MARGIN*2, 28);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(msg);
  tft.add_text(0, 0, COLOR_YELLOW, tft_string);
}

static void drawMessage(FSTR_P const fmsg) { drawMessage(FTOP(fmsg)); }

static void drawAxisValue(const AxisEnum axis) {
  const float value = (
    TERN_(HAS_BED_PROBE, axis == Z_AXIS && motionAxisState.z_selection == Z_SELECTION_Z_PROBE ? probe.offset.z :)
    ui.manual_move.axis_value(axis)
  );
  xy_int_t pos;
  uint16_t color;
  switch (axis) {
    case X_AXIS: pos = motionAxisState.xValuePos; color = X_BTN_COLOR; break;
    case Y_AXIS: pos = motionAxisState.yValuePos; color = Y_BTN_COLOR; break;
    case Z_AXIS: pos = motionAxisState.zValuePos; color = Z_BTN_COLOR; break;
    // case E_AXIS: pos = motionAxisState.eValuePos; color = E_BTN_COLOR; break;
    default: return;
  }
  tft.canvas(pos.x, pos.y, 64, 28);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(ftostr52sp(value));
  tft.add_text(0, 0, color, tft_string);
}

static void moveAxis(const AxisEnum axis, const int8_t direction) {
  quick_feedback();

  #if ENABLED(PREVENT_COLD_EXTRUSION)
    if (axis == E_AXIS && thermalManager.tooColdToExtrude(motionAxisState.e_selection)) {
      drawMessage(F("Too cold"));
      return;
    }
  #endif

  const float diff = motionAxisState.currentStepSize * direction;

  if (axis == Z_AXIS && motionAxisState.z_selection == Z_SELECTION_Z_PROBE) {
    #if ENABLED(BABYSTEP_ZPROBE_OFFSET)
      const int16_t babystep_increment = direction * BABYSTEP_SIZE_Z;
      const bool do_probe = DISABLED(BABYSTEP_HOTEND_Z_OFFSET) || active_extruder == 0;
      const float bsDiff = planner.mm_per_step[Z_AXIS] * babystep_increment,
                  new_probe_offset = probe.offset.z + bsDiff,
                  new_offs = TERN(BABYSTEP_HOTEND_Z_OFFSET
                    , do_probe ? new_probe_offset : hotend_offset[active_extruder].z - bsDiff
                    , new_probe_offset
                  );
      if (WITHIN(new_offs, Z_PROBE_OFFSET_RANGE_MIN, Z_PROBE_OFFSET_RANGE_MAX)) {
        babystep.add_steps(Z_AXIS, babystep_increment);
        if (do_probe)
          probe.offset.z = new_offs;
        else
          TERN(BABYSTEP_HOTEND_Z_OFFSET, hotend_offset[active_extruder].z = new_offs, NOOP);
        drawMessage(NUL_STR); // clear the error
        drawAxisValue(axis);
      }
      else {
        drawMessage(GET_TEXT_F(MSG_LCD_SOFT_ENDSTOPS));
      }
    #elif HAS_BED_PROBE
      // only change probe.offset.z
      probe.offset.z += diff;
      if (direction < 0 && current_position[axis] < Z_PROBE_OFFSET_RANGE_MIN) {
        current_position[axis] = Z_PROBE_OFFSET_RANGE_MIN;
        drawMessage(GET_TEXT_F(MSG_LCD_SOFT_ENDSTOPS));
      }
      else if (direction > 0 && current_position[axis] > Z_PROBE_OFFSET_RANGE_MAX) {
        current_position[axis] = Z_PROBE_OFFSET_RANGE_MAX;
        drawMessage(GET_TEXT_F(MSG_LCD_SOFT_ENDSTOPS));
      }
      else {
        drawMessage(NUL_STR); // clear the error
      }
      drawAxisValue(axis);
    #endif
    return;
  }

  if (!ui.manual_move.processing) {
    // Get motion limit from software endstops, if any
    float min, max;
    soft_endstop.get_manual_axis_limits(axis, min, max);

    // Delta limits XY based on the current offset from center
    // This assumes the center is 0,0
    #if ENABLED(DELTA)
      if (axis != Z_AXIS && axis != E_AXIS) {
        max = SQRT(sq((float)(DELTA_PRINTABLE_RADIUS)) - sq(current_position[Y_AXIS - axis])); // (Y_AXIS - axis) == the other axis
        min = -max;
      }
    #endif

    // Get the new position
    const bool limited = ui.manual_move.apply_diff(axis, diff, min, max);
    #if IS_KINEMATIC
      UNUSED(limited);
    #else
      PGM_P const msg = limited ? GET_TEXT(MSG_LCD_SOFT_ENDSTOPS) : NUL_STR;
      drawMessage(msg);
    #endif

    ui.manual_move.soon(axis OPTARG(MULTI_E_MANUAL, motionAxisState.e_selection));
  }

  drawAxisValue(axis);
}

static void e_plus()  { moveAxis(E_AXIS, 1);  }
static void e_minus() { moveAxis(E_AXIS, -1); }
static void x_minus() { moveAxis(X_AXIS, -1); }
static void x_plus()  { moveAxis(X_AXIS, 1);  }
static void y_plus()  { moveAxis(Y_AXIS, 1);  }
static void y_minus() { moveAxis(Y_AXIS, -1); }
static void z_plus()  { moveAxis(Z_AXIS, 1);  }
static void z_minus() { moveAxis(Z_AXIS, -1); }

#if ENABLED(TOUCH_SCREEN)
  static void e_select() {
    motionAxisState.e_selection++;
    if (motionAxisState.e_selection >= EXTRUDERS) {
      motionAxisState.e_selection = 0;
    }

    quick_feedback();
    // drawCurESelection();
    // drawAxisValue(E_AXIS);
  }

  static void do_home() {
    quick_feedback();
    drawMessage(GET_TEXT_F(MSG_LEVEL_BED_HOMING));
    queue.inject_P(G28_STR);
    // Disable touch until home is done
    TERN_(HAS_TFT_XPT2046, touch.disable());
    // drawAxisValue(E_AXIS);
    drawAxisValue(X_AXIS);
    drawAxisValue(Y_AXIS);
    drawAxisValue(Z_AXIS);
  }

  static void step_size() {
    motionAxisState.currentStepSize = motionAxisState.currentStepSize / 10.0;
    if (motionAxisState.currentStepSize < 0.0015) motionAxisState.currentStepSize = 10.0;
    quick_feedback();
    drawCurStepValue();
  }
#endif

#if BOTH(HAS_BED_PROBE, TOUCH_SCREEN)
  static void z_select() {
    motionAxisState.z_selection *= -1;
    quick_feedback();
    drawCurZSelection();
    drawAxisValue(Z_AXIS);
  }
#endif

static void disable_steppers() {
  quick_feedback();
  queue.inject(F("M84"));
}

static void drawBtn(int x, int y, const char *label, intptr_t data, MarlinImage img, uint16_t bgColor, bool enabled = true) {
  uint16_t width = Images[imgBtn52Rounded].width;
  uint16_t height = Images[imgBtn52Rounded].height;

  if (!enabled) bgColor = COLOR_CONTROL_DISABLED;

  tft.canvas(x, y, width, height);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_image(0, 0, imgBtn52Rounded, bgColor, COLOR_BACKGROUND, COLOR_DARKGREY);

  // TODO: Make an add_text() taking a font arg
  if (label) {
    tft_string.set(label);
    tft_string.trim();
    tft.add_text(tft_string.center(width), height / 2 - tft_string.font_height() / 2, bgColor, tft_string);
  }
  else {
    tft.add_image(0, 0, img, bgColor, COLOR_BACKGROUND, COLOR_DARKGREY);
  }

  TERN_(HAS_TFT_XPT2046, if (enabled) touch.add_control(BUTTON, x, y, width, height, data));
}
void MarlinUI::move_axis_screen() {
  // Reset
  defer_status_screen(true);
  motionAxisState.blocked = false;
  TERN_(HAS_TFT_XPT2046, touch.enable());

  ui.clear_lcd();

  TERN_(TOUCH_SCREEN, touch.clear());

  const bool busy = printingIsActive();

  // Mesh Button

  tft.canvas(100, 17, 120, 46);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set("Mesh");
  tft.add_rectangle(0, 0, 120, 46, !busy ? COLOR_WHITE : COLOR_CONTROL_DISABLED);
  tft.add_text(tft_string.center(120), 8, !busy ? COLOR_WHITE : COLOR_CONTROL_DISABLED, tft_string);
  TERN_(TOUCH_SCREEN, if (!busy) touch.add_control(MESH_LEVEL, 100, 17, 120, 46));

  // Babystep Button
  // tft.canvas(107, 17, 106, 46);
  // tft.set_background(COLOR_BACKGROUND);
  // tft_string.set("Babystep");
  // uint16_t baby_color;
  // if (!busy) { 
  //   baby_color = COLOR_CONTROL_DISABLED;
  //   } else {
  //     baby_color = COLOR_WHITE;
  //   }
  // tft.add_rectangle(0, 0, 106, 46, baby_color);
  // tft.add_text(tft_string.center(106), 8, baby_color, tft_string);
  // TERN_(TOUCH_SCREEN, if (busy) touch.add_control(BABYSTEP_BUTTON, 107, 17, 106, 46));
  // add_control(20, 200, menu_media, imgSD, cm && !pa, COLOR_CONTROL_ENABLED, cm && pa ? COLOR_BUSY : COLOR_CONTROL_DISABLED);
  
  // Level Button
  // tft.canvas(180, 17, 120, 46);
  // tft.set_background(COLOR_BACKGROUND);
  // tft_string.set("Level");
  // tft.add_rectangle(0, 0, 120, 46, !busy ? COLOR_WHITE : COLOR_CONTROL_DISABLED);
  // tft.add_text(tft_string.center(120), 8, !busy ? COLOR_WHITE : COLOR_CONTROL_DISABLED, tft_string);
  // TERN_(TOUCH_SCREEN, if (!busy) touch.add_control(TRAMMING, 180, 17, 120, 46));


  // Babysteps during printing? Select babystep for Z probe offset
  if (busy && ENABLED(BABYSTEP_ZPROBE_OFFSET))
    motionAxisState.z_selection = Z_SELECTION_Z_PROBE;

  // ROW 1 -> E+ Y+ CurY Z+
  int x = X_MARGIN, y = Y_MARGIN + 80, spacing = 0;

  drawBtn(x, y, "E+", (intptr_t)e_plus, imgUp, E_BTN_COLOR, !busy);

  spacing = (TFT_WIDTH - X_MARGIN * 2 - 3 * BTN_WIDTH) / 2;
  x += BTN_WIDTH + spacing;
  uint16_t yplus_x = x;
  // y += 60;
  drawBtn(x, y, "Y+", (intptr_t)y_plus, imgUp, Y_BTN_COLOR, !busy);
  

  // Cur Y
  x += BTN_WIDTH;
  // motionAxisState.yValuePos.x = x + 2 - 50;
  // motionAxisState.yValuePos.y = y + 50 + 10 + 100 + 30 + 15 - 5;
  // drawAxisValue(Y_AXIS);

  x += spacing;
  // y = Y_MARGIN;
  drawBtn(x, y, "Z+", (intptr_t)z_plus, imgUp, Z_BTN_COLOR, !busy || ENABLED(BABYSTEP_ZPROBE_OFFSET)); //only enabled when not busy or have baby step

  // ROW 2 -> "Ex"  X-  HOME X+  "Z"
  y += BTN_HEIGHT + (TFT_HEIGHT - Y_MARGIN * 2 - 4 * BTN_HEIGHT) / 3 - 50;
  x = X_MARGIN;
  spacing = (TFT_WIDTH - X_MARGIN * 2 - 5 * BTN_WIDTH) / 4;

  // motionAxisState.eNamePos.x = x + 10;
  // motionAxisState.eNamePos.y = y - 50;
  // drawCurESelection();

  TERN_(HAS_TFT_XPT2046, if (!busy) touch.add_control(BUTTON, x - 10, y - 40, BTN_WIDTH, BTN_HEIGHT, (intptr_t)e_select));

  uint16_t xminus_x = x;
  drawBtn(x, y, "X-", (intptr_t)x_minus, imgLeft, X_BTN_COLOR, !busy);

  x += BTN_WIDTH * 2 + spacing * 2; //imgHome is 64x64
  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH / 2 - Images[imgHome].width / 2, y - (Images[imgHome].width - BTN_HEIGHT) / 2, BUTTON, (intptr_t)do_home, imgHome, !busy));

  x += BTN_WIDTH * 2 + spacing * 3 + 10;
  uint16_t xplus_x = x - 20;
  drawBtn(x, y, "X+", (intptr_t)x_plus, imgRight, X_BTN_COLOR, !busy);


  x += BTN_WIDTH + spacing;
  // motionAxisState.zTypePos.x = x + 25;
  // motionAxisState.zTypePos.y = y - 50;
  // drawCurZSelection();

  #if BOTH(HAS_BED_PROBE, TOUCH_SCREEN)
    if (!busy) touch.add_control(BUTTON, x, y, BTN_WIDTH, 34 * 2, (intptr_t)z_select);
  #endif

  // ROW 3 -> E- CurX Y-  Z-
  y += BTN_HEIGHT + (TFT_HEIGHT - Y_MARGIN * 2 - 4 * BTN_HEIGHT) / 3 - 50;
  x = X_MARGIN;
  spacing = (TFT_WIDTH - X_MARGIN * 2 - 3 * BTN_WIDTH) / 2;

  drawBtn(x, y, "E-", (intptr_t)e_minus, imgDown, E_BTN_COLOR, !busy);

  // Cur E             Координаты Е
  // motionAxisState.eValuePos.x = x;
  // motionAxisState.eValuePos.y = y + BTN_HEIGHT + 2 + 10 - 5;
  // drawAxisValue(E_AXIS);

  // Cur X              Координаты Х
  // motionAxisState.xValuePos.x = BTN_WIDTH + (TFT_WIDTH - X_MARGIN * 2 - 5 * BTN_WIDTH) / 4 - 8; //X- pos
  // motionAxisState.xValuePos.y = y - 75;
  // drawAxisValue(X_AXIS);

  x += BTN_WIDTH + spacing;
  drawBtn(x, y, "Y-", (intptr_t)y_minus, imgDown, Y_BTN_COLOR, !busy);

  x += BTN_WIDTH + spacing;
  drawBtn(x, y, "Z-", (intptr_t)z_minus, imgDown, Z_BTN_COLOR, !busy || ENABLED(BABYSTEP_ZPROBE_OFFSET)); //only enabled when not busy or have baby step

  // Cur Z              Координаты Z
  // motionAxisState.zValuePos.x = x;
  // motionAxisState.zValuePos.y = y + BTN_HEIGHT + 2 + 10 - 5;
  // drawAxisValue(Z_AXIS);

  // ROW 4 -> step_size  disable steppers back
  y = TFT_HEIGHT - Y_MARGIN - BTN_HEIGHT; //
  x = xplus_x - CUR_STEP_VALUE_WIDTH;
  motionAxisState.stepValuePos.x = xminus_x + 10;
  motionAxisState.stepValuePos.y = TFT_HEIGHT - Y_MARGIN - BTN_HEIGHT - 30;
  if (!busy) {
    drawCurStepValue();
    TERN_(HAS_TFT_XPT2046, touch.add_control(BUTTON, motionAxisState.stepValuePos.x, motionAxisState.stepValuePos.y, CUR_STEP_VALUE_WIDTH, BTN_HEIGHT, (intptr_t)step_size));
  }

  // aligned with x+
  drawBtn(yplus_x, y - 30, "off", (intptr_t)disable_steppers, imgCancel, COLOR_WHITE, !busy);

  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH - X_MARGIN - BTN_WIDTH + 25, y - 20, BACK, imgBack));


  motionAxisState.xValuePos.x = 40;
  motionAxisState.xValuePos.y = 338;
  motionAxisState.yValuePos.x = 135;
  motionAxisState.yValuePos.y = 338;
  motionAxisState.zValuePos.x = 240;
  motionAxisState.zValuePos.y = 338;

  tft.canvas(10, 335, 300, 36); //335
  tft.set_background(COLOR_BACKGROUND);
  tft.add_rectangle(0, 0, 300, 36, COLOR_WHITE);
  tft.add_text(10, 3, COLOR_WHITE, "X");
  tft.add_text(100, 3, COLOR_WHITE, "Y");
  tft.add_text(210, 3, COLOR_WHITE, "Z");
  drawAxisValue(X_AXIS);
  drawAxisValue(Y_AXIS);  
  drawAxisValue(Z_AXIS);  

  // tft.add_text(35, 3, X_BTN_COLOR, ftostr52sp(ui.manual_move.axis_value(X_AXIS)));
  // tft.add_text(145, 3, Y_BTN_COLOR, ftostr52sp(ui.manual_move.axis_value(Y_AXIS)));
  // tft.add_text(245, 3, Z_BTN_COLOR, ftostr52sp(ui.manual_move.axis_value(Z_AXIS)));
  
}

#ifdef MKS_WIFI_MODULE
void MarlinUI::wifi_screen() {

  defer_status_screen(true);
  ui.clear_lcd();
  TERN_(TOUCH_SCREEN, touch.clear());

  tft.canvas(0, 200, 320, 80);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set("SSID: ");
  tft_string.add(wifiPara.ap_name);
  tft.add_text(tft_string.center(320), 0, COLOR_PRINT_TIME, tft_string);
  tft_string.set("IP: ");
  tft_string.add(ipPara.ip_addr);
  tft.add_text(tft_string.center(320), 40, COLOR_PRINT_TIME, tft_string);
  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH - X_MARGIN - BTN_WIDTH + 25, 410, BACK, imgBack));
}
#endif

void MarlinUI::heater_screen() {
  const bool busy = printingIsActive();
  const bool paused = printingIsPaused();
  celsius_t currentTemperature, targetTemperature;
  currentTemperature = thermalManager.wholeDegHotend(H_E0);
  targetTemperature = thermalManager.degTargetHotend(H_E0);
  TERN_(TOUCH_SCREEN, touch.clear());
  defer_status_screen(true);

  tft.canvas(0, 20, 320, 80);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_EXTRUDER_STATUS));
  tft_string.add(" : ");
  tft_string.add(i16tostr3rj(currentTemperature));
  tft_string.add(" -> ");
  tft_string.add(i16tostr3rj(targetTemperature));
  tft.add_text(tft_string.center(320), 15, COLOR_YELLOW, tft_string);

  int temp_hotend = 0;
  int count = 1;
    for (uint8_t i =0; i <= 1; i++){
      for (uint8_t j=0; j <= 2; j++){
        if (count <= PREHEAT_COUNT){
          tft.canvas(XX_OFFSET + PREHEAT_BTN_WIDTH * j + X_SPACING * j, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * i + Y_SPACING * i, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT);
          tft.set_background(COLOR_BACKGROUND);
          // label = preheat_label[count];
          switch (count)
          {
            #ifdef PREHEAT_1_LABEL
              case 1: tft_string.set(PREHEAT_1_LABEL);
                temp_hotend = ui.material_preset[0].hotend_temp;
                break;
            #endif
            #ifdef PREHEAT_2_LABEL
              case 2: tft_string.set(PREHEAT_2_LABEL);
                temp_hotend = ui.material_preset[1].hotend_temp;
                break;
            #endif
            #ifdef PREHEAT_3_LABEL
              case 3: tft_string.set(PREHEAT_3_LABEL);
                temp_hotend = ui.material_preset[2].hotend_temp;
                break;
            #endif
            #ifdef PREHEAT_4_LABEL
              case 4: tft_string.set(PREHEAT_4_LABEL);
                temp_hotend = ui.material_preset[3].hotend_temp;
                break;
            #endif
            #ifdef PREHEAT_5_LABEL
              case 5: tft_string.set(PREHEAT_5_LABEL);
                temp_hotend = ui.material_preset[4].hotend_temp;
                break;
            #endif
            #ifdef PREHEAT_6_LABEL
              case 6: tft_string.set(PREHEAT_6_LABEL);
                temp_hotend = ui.material_preset[5].hotend_temp;
                break;
            #endif
            default: break;
          }
          // tft_string.set(label);
          tft.add_rectangle(0, 0, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, COLOR_WHITE);
          tft.add_text(tft_string.center(PREHEAT_BTN_WIDTH), 15, COLOR_WHITE, tft_string);
          TERN_(TOUCH_SCREEN, touch.add_control(HEAT_EXT, XX_OFFSET + PREHEAT_BTN_WIDTH * j + X_SPACING * j, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * i + Y_SPACING * i, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, temp_hotend));
          count++;
        }
      }
    }
  if (!busy & !paused) {
    tft.canvas(XX_OFFSET + PREHEAT_BTN_WIDTH * 0 + X_SPACING * 0, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * 2 + Y_SPACING * 2, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set("E+");
    tft.add_rectangle(0, 0, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, COLOR_YELLOW);
    tft.add_text(tft_string.center(PREHEAT_BTN_WIDTH), 15, COLOR_YELLOW, tft_string);
    TERN_(TOUCH_SCREEN, touch.add_control(FILAMENT_MOVE, XX_OFFSET + PREHEAT_BTN_WIDTH * 0 + X_SPACING * 0, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * 2 + Y_SPACING * 2, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, 1));


    tft.canvas(XX_OFFSET + PREHEAT_BTN_WIDTH * 1 + X_SPACING * 1, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * 2 + Y_SPACING * 2, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(GET_TEXT(MSG_STOP_BUTTON));
    tft.add_rectangle(0, 0, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, COLOR_RED);
    tft.add_text(tft_string.center(PREHEAT_BTN_WIDTH), 15, COLOR_RED, tft_string);
    TERN_(TOUCH_SCREEN, touch.add_control(FILAMENT_MOVE, XX_OFFSET + PREHEAT_BTN_WIDTH * 1 + X_SPACING * 1, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * 2 + Y_SPACING * 2, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, 0));


    tft.canvas(XX_OFFSET + PREHEAT_BTN_WIDTH * 2 + X_SPACING * 2, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * 2 + Y_SPACING * 2, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set("E-");
    tft.add_rectangle(0, 0, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, COLOR_YELLOW);
    tft.add_text(tft_string.center(PREHEAT_BTN_WIDTH), 15, COLOR_YELLOW, tft_string);
    TERN_(TOUCH_SCREEN, touch.add_control(FILAMENT_MOVE, XX_OFFSET + PREHEAT_BTN_WIDTH * 2 + X_SPACING * 2, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * 2 + Y_SPACING * 2, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, -1));
  }



  // Preheat Manual
  tft.canvas(10, 330, 120, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_MANUAL_BUTTON));
  tft.add_rectangle(0, 0, 120, 64, COLOR_DARK_ORANGE);
  tft.add_text(tft_string.center(120), 15, COLOR_DARK_ORANGE, tft_string);

  // Cooling
  tft.canvas(190, 330, 120, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_COOLING_BUTTON));
  tft.add_rectangle(0, 0, 120, 64, COLOR_AQUA);
  tft.add_text(tft_string.center(120), 15, COLOR_AQUA, tft_string);

  TERN_(TOUCH_SCREEN, touch.add_control(HEAT_EXT, 190, 330, 120, 64, 0));
  TERN_(TOUCH_SCREEN, touch.add_control(HEATER_MANUAL, 10, 330, 120, 64, H_E0));
  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH - X_MARGIN - BTN_WIDTH + 25, 420, BACK, imgBack));

}

void MarlinUI::bed_screen() {

  celsius_t currentTemperature, targetTemperature;
  currentTemperature = thermalManager.wholeDegBed();
  targetTemperature = thermalManager.degTargetBed();

  // ui.clear_lcd();
  TERN_(TOUCH_SCREEN, touch.clear());

  defer_status_screen(true);
  //

  tft.canvas(0, 20, 320, 80);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_BED_STATUS));
  tft_string.add(" : ");
  tft_string.add(i16tostr3rj(currentTemperature));
  tft_string.add(" -> ");
  tft_string.add(i16tostr3rj(targetTemperature));
  tft.add_text(tft_string.center(320), 15, COLOR_YELLOW, tft_string);

  int temp_bed = 0;
  int count = 1;
    for (uint8_t i =0; i <= 1; i++){
      for (uint8_t j=0; j <= 2; j++){
        if (count <= PREHEAT_COUNT){
          tft.canvas(XX_OFFSET + PREHEAT_BTN_WIDTH * j + X_SPACING * j, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * i + Y_SPACING * i, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT);
          tft.set_background(COLOR_BACKGROUND);
          // label = preheat_label[count];
          switch (count)
          {
            #ifdef PREHEAT_1_LABEL
              case 1: tft_string.set(PREHEAT_1_LABEL);
                temp_bed = ui.material_preset[0].bed_temp;
                break;
            #endif
            #ifdef PREHEAT_2_LABEL
              case 2: tft_string.set(PREHEAT_2_LABEL);
                temp_bed = ui.material_preset[1].bed_temp;
                break;
            #endif
            #ifdef PREHEAT_3_LABEL
              case 3: tft_string.set(PREHEAT_3_LABEL);
                temp_bed = ui.material_preset[2].bed_temp;
                break;
            #endif
            #ifdef PREHEAT_4_LABEL
              case 4: tft_string.set(PREHEAT_4_LABEL);
                temp_bed = ui.material_preset[3].bed_temp;
                break;
            #endif
            #ifdef PREHEAT_5_LABEL
              case 5: tft_string.set(PREHEAT_5_LABEL);
                temp_bed = ui.material_preset[4].bed_temp;
                break;
            #endif
            #ifdef PREHEAT_6_LABEL
              case 6: tft_string.set(PREHEAT_6_LABEL);
                temp_bed = ui.material_preset[5].bed_temp;
                break;
            #endif
            default: break;
          }
          // tft_string.set(label);
          tft.add_rectangle(0, 0, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, COLOR_WHITE);
          tft.add_text(tft_string.center(PREHEAT_BTN_WIDTH), 15, COLOR_WHITE, tft_string);
          TERN_(TOUCH_SCREEN, touch.add_control(HEAT_BED, XX_OFFSET + PREHEAT_BTN_WIDTH * j + X_SPACING * j, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * i + Y_SPACING * i, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, temp_bed));
          count++;
        }
      }
    }

  // Preheat Manual
  tft.canvas(10, 330, 120, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_MANUAL_BUTTON));
  tft.add_rectangle(0, 0, 120, 64, COLOR_DARK_ORANGE);
  tft.add_text(tft_string.center(120), 15, COLOR_DARK_ORANGE, tft_string);

  // Cooling
  tft.canvas(190, 330, 120, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_COOLING_BUTTON));
  tft.add_rectangle(0, 0, 120, 64, COLOR_AQUA);
  tft.add_text(tft_string.center(120), 15, COLOR_AQUA, tft_string);
  TERN_(TOUCH_SCREEN, touch.add_control(HEAT_BED, 190, 330, 120, 64, 0));
  TERN_(TOUCH_SCREEN, touch.add_control(HEATER_MANUAL, 10, 330, 120, 64, H_BED));
  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH - X_MARGIN - BTN_WIDTH + 25, 420, BACK, imgBack));

}

void MarlinUI::fan_screen() {

  TERN_(TOUCH_SCREEN, touch.clear());

  defer_status_screen(true);
  //

  tft.canvas(0, 20, 320, 80);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_MODEL_FAN));
  tft_string.add(" : ");
  tft_string.add(ui8tostr4pctrj(thermalManager.fan_speed[0]));
  tft.add_text(tft_string.center(320), 15, COLOR_YELLOW, tft_string);

  uint8_t set_fan_speed;
  int count = 0;
  // int fan_preset_numbers = 4;
  for (uint8_t i =0; i <= 2; i++){
    for (uint8_t j=0; j <= 2; j++){
      if (count <= 5){
        tft.canvas(XX_OFFSET + PREHEAT_BTN_WIDTH * j + X_SPACING * j, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * i + Y_SPACING * i, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT);
        tft.set_background(COLOR_BACKGROUND);
        // label = preheat_label[count];
        switch (count) {
          case 0: set_fan_speed = 0; break;
          case 1: set_fan_speed = 64; break;
          case 2: set_fan_speed = 128; break;
          case 3: set_fan_speed = 191; break; 
          case 4: set_fan_speed = 255; break;
          default: break;
        }

        if (count == 5){
          tft_string.set(GET_TEXT(MSG_SET_BUTTON));
          tft.add_rectangle(0, 0, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, COLOR_WHITE);
          tft.add_text(tft_string.center(PREHEAT_BTN_WIDTH), 15, COLOR_WHITE, tft_string);
          TERN_(TOUCH_SCREEN, touch.add_control(FAN_MANUAL, XX_OFFSET + PREHEAT_BTN_WIDTH * j + X_SPACING * j, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * i + Y_SPACING * i, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT));
        } else {
          // set_fan_speed = 255/fan_preset_numbers*count;
          tft_string.set(ui8tostr4pctrj(set_fan_speed));
          tft.add_rectangle(0, 0, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, COLOR_WHITE);
          tft.add_text(tft_string.center(PREHEAT_BTN_WIDTH), 15, COLOR_WHITE, tft_string);
          TERN_(TOUCH_SCREEN, touch.add_control(SET_FAN_SPEED, XX_OFFSET + PREHEAT_BTN_WIDTH * j + X_SPACING * j, 80 + YY_OFFSET + PREHEAT_BTN_HEIGHT * i + Y_SPACING * i, PREHEAT_BTN_WIDTH, PREHEAT_BTN_HEIGHT, set_fan_speed));
        }
        count++;
      }
    }
  }

  // bool chamber_fan = false;
  tft.canvas(0, 240, 320, 60);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(GET_TEXT(MSG_CHAMBER_FAN));
  tft_string.add(" : ");
  if (ui.chamber_fan) {tft_string.add(GET_TEXT(MSG_LCD_ON));} else {tft_string.add(GET_TEXT(MSG_LCD_OFF));}
  // tft_string.add(ui8tostr4pctrj(thermalManager.fan_speed[0]));
  tft.add_text(tft_string.center(320), 15, COLOR_YELLOW, tft_string);

  tft.canvas(10, 300, 120, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_rectangle(0, 0, 120, 64, COLOR_WHITE);
  tft_string.set(GET_TEXT(MSG_LCD_ON));
  tft.add_text(tft_string.center(120), 15, COLOR_WHITE, tft_string);
  TERN_(TOUCH_SCREEN, touch.add_control(CHAMBER_FAN, 10, 300, 120, 64, true));

  tft.canvas(180, 300, 120, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_rectangle(0, 0, 120, 64, COLOR_WHITE);  
  tft_string.set(GET_TEXT(MSG_LCD_OFF));
  tft.add_text(tft_string.center(120), 15, COLOR_WHITE, tft_string);
  TERN_(TOUCH_SCREEN, touch.add_control(CHAMBER_FAN, 180, 300, 120, 64, false));

  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH - X_MARGIN - BTN_WIDTH + 25, 420, BACK, imgBack));

}

#ifdef FINISH_SCREEN
void MarlinUI::finish_screen(){
  char buffer[22]; 

  TERN_(TOUCH_SCREEN, touch.clear());
  defer_status_screen(true);  //Blocking Autoback to status screen

  //Print Head
  tft.canvas(0, 20, 320, 200);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set("Printing Finished");
  tft.add_text(tft_string.center(320), 15, COLOR_YELLOW, tft_string);
  
  //Print Time
  duration_t(print_job_timer.duration()).toString(buffer);
  tft_string.set(buffer);
  tft.add_text(tft_string.center(320), 90, COLOR_YELLOW, tft_string);

  //Print Full filename
  tft_string.set(card.longest_filename());
  tft.add_text(tft_string.center(320), 150, COLOR_WHITE, tft_string);

  //Print Button
  tft.canvas(170, 330, 140, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set("Print Again");
  tft.add_rectangle(0, 0, 140, 64, COLOR_DARK_ORANGE);
  tft.add_text(tft_string.center(140), 15, COLOR_ORANGE, tft_string);
  TERN_(TOUCH_SCREEN, touch.add_control(RETRY_PRINT, 170, 330, 140, 64));

  //Print Back Button
  tft.canvas(10, 330, 140, 64);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set("Back");
  tft.add_rectangle(0, 0, 140, 64, COLOR_DARK_ORANGE);
  tft.add_text(tft_string.center(140), 15, COLOR_ORANGE, tft_string);
  TERN_(TOUCH_SCREEN, touch.add_control(BACK, 10, 330, 140, 64));
  // TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH - X_MARGIN - BTN_WIDTH + 25, 420, BACK, imgBack));

}
#endif

#endif //HAS_UI_320x480
