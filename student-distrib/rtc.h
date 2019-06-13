/* rtc.h - Defines used in interactions with the rtc device
 * vim:ts=4 noexpandtab
 */

#ifndef	_RTC_H
#define _RTC_H

#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"

/* defined ports for RTC */
#define RTCREGA 		0x8A
#define RTCREGB 		0x8B
#define RTCPORT1		0x70
#define RTCPORT2 		0x71
#define RTCORED 		0x40
#define RTCIRQ8 		8
#define RTCIRQ2			2
#define BASENUM			15
#define DEFAULTB		4
#define RTCREGC 		0x8C
#define ERRORMAG 		0x4F2E
#define RTCUNUN			0xF0

/* initialization RTC device */
extern void rtc_init();

/* rtc interrupt handler */
extern void _idt_rtc_irq_handler();

/* rtc rtc_open */
extern uint32_t rtc_open(const uint8_t* filename);

/* rtc rtc_read */
extern uint32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* rtc rtc_write */
extern uint32_t rtc_write(int32_t fd, const uint32_t* buf, int32_t nbytes);

/* rtc rtc_close */
extern uint32_t rtc_close(int32_t fd);

/* rtc change_freq */
extern void change_freq(uint8_t count);

#endif
