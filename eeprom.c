#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "eeprom.h"

#define MAX_STR_BCNT				255

#define ADDRESS_BTN1_LEN			0
#define ADDRESS_BTN2_LEN			1
#define ADDRESS_BTN3_LEN			2
#define ADDRESS_BTN1_STR			4
#define ADDRESS_BTN2_STR			260
#define ADDRESS_BTN3_STR			516

int ee_read_btn_valid_len(int program_btn)
{
	int address;
	if (program_btn==1) address = ADDRESS_BTN1_LEN;
	else if (program_btn==2) address = ADDRESS_BTN2_LEN;
	else if (program_btn==3) address = ADDRESS_BTN3_LEN;
	else return -1;

	return eeprom_read_byte((unsigned char *) address);
}

int ee_readback(int program_btn,char* buf,int bcnt_max)
{
	int bcnt_reqd = ee_read_btn_valid_len(program_btn);
	if (bcnt_max < bcnt_reqd) return -1;

	int address;
	if (program_btn==1) address = ADDRESS_BTN1_STR;
        else if (program_btn==2) address = ADDRESS_BTN2_STR;
        else if (program_btn==3) address = ADDRESS_BTN3_STR;
        else return -1;

	memset(buf,0,bcnt_max);
	eeprom_read_block(buf, (unsigned char *) address, bcnt_reqd);

	return bcnt_reqd;
}

int ee_overwrite(int program_btn,char* buf,int bcnt)
{
	if (bcnt > MAX_STR_BCNT) return -1;

        int address;
        if (program_btn==1) address = ADDRESS_BTN1_LEN;
        else if (program_btn==2) address = ADDRESS_BTN2_LEN;
        else if (program_btn==3) address = ADDRESS_BTN3_LEN;
        else return -1;

	while (!eeprom_is_ready()) ;
	eeprom_write_byte((unsigned char *) address, (unsigned char)bcnt);

        if (program_btn==1) address = ADDRESS_BTN1_STR;
        else if (program_btn==2) address = ADDRESS_BTN2_STR;
        else if (program_btn==3) address = ADDRESS_BTN3_STR;
        else return -1;

	while (!eeprom_is_ready()) ;
	eeprom_write_block(buf, (unsigned char *) address, bcnt);

	return 0;
}
