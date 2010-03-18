#ifndef __USB_DUAL_H__
#define __USB_DUAL_H__

#include <stdint.h>

extern int b_mode_kybd;

void usbs_usb_com_vector(void);
void usbs_usb_gen_vector(void);

void usbs_usb_init(void);                       // initialize everything
uint8_t usbs_usb_configured(void);              // is the USB port configured

#endif
