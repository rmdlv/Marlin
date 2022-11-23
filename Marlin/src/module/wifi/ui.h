#ifndef MKS_WIFI_UI_H
#define MKS_WIFI_UI_H

#ifdef MKS_WIFI_MODULE

#include "../../MarlinCore.h"

#if ENABLED(TFT_320x480) || ENABLED(TFT_320x480_SPI)

void mks_update_status(char *filename, uint32_t current_filesize, uint32_t file_size);
void mks_end_transmit(void);
void mks_upload_screen(void);
#endif

#endif

#endif //MKS_WIFI_MODULE