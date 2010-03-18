#ifndef __EEPROM_H__
#define __EEPROM_H__

int ee_readback(int program_btn,char* buf,int bcnt_max);
int ee_overwrite(int program_btn,char* buf,int bcnt);

#endif
