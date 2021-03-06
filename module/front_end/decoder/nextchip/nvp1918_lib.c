/**
 * @file nvp1918_lib.c
 * NextChip NVP1918 8-CH 960H/D1 Video Decoders and Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.12 $
 * $Date: 2015/05/08 03:46:27 $
 *
 * ChangeLog:
 *  $Log: nvp1918_lib.c,v $
 *  Revision 1.12  2015/05/08 03:46:27  sainthuang
 *  add module parameter "audio_master" to configure codec as master or slave
 *
 *	Revision 1.12  2015/05/08 11:29:12  sainthuang	
 *  [audio] 1. add option to set codec as master or slave	
 *  [audio] 2. make record and play to delay 1 clock by setting 0x39  
 *
 *  Revision 1.11  2014/06/17 03:14:11  jerson_l
 *  1. add set/get sharpness api
 *
 *  Revision 1.10  2014/02/13 05:38:11  ccsu
 *  [audio] 1.add audio_chnum module parameter
 *  [audio] 2.modify audio_init to meet decoder id0 is master, id1 is slave
 *
 *  Revision 1.9  2014/01/24 05:45:33  jerson_l
 *  1. remove unused print out message
 *
 *  Revision 1.8  2013/12/25 08:04:45  ccsu
 *  [audio] modify audio init for 8287/8282/8286/8283
 *
 *  Revision 1.7  2013/12/19 07:29:35  ccsu
 *  modify gm8287 channel sequence at audio init
 *
 *  Revision 1.6  2013/10/01 06:12:02  jerson_l
 *  1. switch to new channel mapping mechanism
 *
 *  Revision 1.5  2013/09/05 11:51:08  ccsu
 *  modify decoder id 1/3 is master when cascade at gm8287
 *
 *  Revision 1.4  2013/08/08 05:19:54  jerson_l
 *  1. support system board(1v1) channel mapping
 *
 *  Revision 1.3  2013/07/31 09:00:51  jerson_l
 *  1. fix 4ch mode vin_chid video/audio channel mapping issue
 *
 *  Revision 1.2  2013/07/30 06:19:02  ccsu
 *  fix audio channel mapping
 *
 *  Revision 1.1.1.1  2013/07/25 09:29:13  jerson_l
 *  add front_end device driver
 *
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <mach/ftpmu010.h>

#include "nextchip/nvp1918.h"        ///< from module/include/front_end/decoder

extern int audio_chnum;

/* AB => BA */
#define SWAP_2CHANNEL(x)    do { (x) = (((x) & 0xF0) >> 4 | ((x) & 0x0F) << 4); } while(0);
/* ABCD => CDAB */
#define SWAP_4CHANNEL(x)    do { (x) = (((x) & 0xFF00) >> 8 | ((x) & 0x00FF) << 8); } while(0);
/* ABCDEFGH => GHEFCDAB */
#define SWAP_8CHANNEL(x)    do { (x) = (((x) & 0xFF000000) >> 24 | ((x) & 0x00FF0000) >> 8 | ((x) & 0x0000FF00) << 8 | ((x) & 0x000000FF) << 24); } while(0);

static unsigned char NVP1918_B0_NTSC[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x00,0x00,0xF0,0x00,0x00,0x00,0xAF,0x00,0xA0,0xA0,0xA0,0xA0,0xF8,0xF8,0xF8,0xF8,
/*10*/   0x76,0x76,0x76,0x76,0x80,0x80,0x80,0x80,0x00,0x00,0x55,0x55,0x84,0x84,0x84,0x84,
/*20*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*30*/   0x12,0x12,0x12,0x12,0x2F,0x82,0x0B,0x41,0x0A,0x0A,0x0A,0x0A,0x80,0x80,0x80,0x80,
/*40*/   0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*50*/   0x00,0x00,0x00,0x00,0xF1,0x10,0x32,0x00,0x9c,0x9c,0x9c,0x9c,0x1E,0x1E,0x1E,0x1E,
/*60*/   0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*70*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x11,0x11,0x00,0x00,0x00,0x00,
/*80*/   0x00,0x00,0x00,0x00,0x00,0x00,0xAF,0x00,0xA0,0xA0,0xA0,0xA0,0xF8,0xF8,0xF8,0xF8,
/*90*/   0x76,0x76,0x76,0x76,0x80,0x80,0x80,0x80,0x00,0x00,0x55,0x55,0x84,0x84,0x84,0x84,
/*A0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*B0*/   0x12,0x12,0x12,0x12,0x2F,0x82,0x0B,0x41,0x0A,0x0A,0x0A,0x0A,0x80,0x80,0x80,0x80,
/*C0*/   0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*D0*/   0x00,0x00,0x00,0x00,0xF1,0x54,0x76,0x00,0x9c,0x9c,0x9c,0x9c,0x1E,0x1E,0x1E,0x1E,
/*E0*/   0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x11,0x11,0x00,0x00,0x00,0x00
};

static unsigned char NVP1918_B1_NTSC[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x02,0x88,0x88,0x88,0x88,0x88,0x1A,0x80,0x03,0x00,0x10,0x32,0x54,0x76,0x98,0xBA,
/*10*/   0xDC,0xFE,0xE4,0x80,0x00,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,
/*20*/   0x88,0x88,0x80,0x10,0x18,0x16,0x00,0x00,0x00,0x88,0xFF,0xC0,0xAA,0xAA,0xAA,0xAA,
/*30*/   0xAA,0x72,0x00,0x72,0x00,0x00,0x00,0x00,0x00,0x00,0xA1,0xa0,0xa0,0x00,0x08,0x00,
/*40*/   0x00,0x00,0x00,0x00,0x14,0x01,0x10,0x40,0x60,0x00,0x03,0x10,0x00,0x00,0x00,0x00,
/*50*/   0x37,0x37,0x1B,0x05,0x00,0x07,0x00,0x09,0x00,0x00,0x08,0x06,0x00,0x46,0x00,0x00,
/*60*/   0xAA,0x1C,0x18,0xFF,0xAA,0x3C,0xFF,0xFF,0xAA,0x1B,0x00,0x00,0xAA,0x3B,0x00,0x00,
/*70*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01,
/*80*/   0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x32,0x54,0x76,
/*90*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*A0*/   0x40,0x00,0x47,0x35,0x8D,0x85,0x7e,0x73,0x7C,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
/*B0*/   0x00,0x00,0x00,0x00,0x40,0x30,0x40,0x30,0x40,0x30,0x40,0x30,0x00,0x00,0x00,0x00,
/*C0*/   0x54,0x76,0x10,0x32,0x54,0x76,0x10,0x32,0x88,0x88,0x0F,0x00,0x70,0x70,0x70,0x70,
/*D0*/   0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*E0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char NVP1918_B2_NTSC[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,
/*10*/   0x00,0x00,0x12,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*20*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*30*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*40*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*50*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*60*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*70*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*80*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*90*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*A0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*B0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*C0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*D0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*E0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char NVP1918_B3_NTSC[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0xE0,0x09,0x0C,0x9F,0x00,0x20,0x40,0x80,0x50,0x38,0x0F,0x00,0x04,0x10,0x30,0x00,
/*10*/   0x06,0x06,0x00,0x80,0x37,0x80,0x49,0x37,0xEF,0xDF,0xDF,0x20,0x0A,0x0C,0x00,0x88,
/*20*/   0x80,0x00,0x23,0x00,0x2A,0xDC,0xF0,0x57,0x90,0x1F,0x52,0x78,0x00,0x68,0x00,0x07,
/*30*/   0xE0,0x43,0xA2,0x00,0x00,0x15,0x25,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,
/*40*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x44,0x00,0x00,0x20,0x10,0x00,0x00,
/*50*/   0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*60*/   0x53,0x00,0x01,0x00,0x99,0x99,0x99,0x99,0x55,0x55,0x00,0x00,0x00,0x00,0x00,0x00,
/*70*/   0xB8,0x01,0x06,0x06,0x11,0x01,0xE0,0x13,0x03,0x22,0x00,0x00,0x00,0x00,0x00,0x00,
/*80*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*90*/   0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*A0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*B0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*C0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*D0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*E0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char NVP1918_B0_PAL[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x00,0x00,0xF0,0x00,0x00,0x00,0xAF,0x00,0xDD,0xDD,0xDD,0xDD,0x08,0x08,0x08,0x08,
/*10*/   0x6b,0x6b,0x6b,0x6b,0x80,0x80,0x80,0x80,0x00,0x00,0x55,0x55,0x84,0x84,0x84,0x84,
/*20*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*30*/   0x11,0x11,0x11,0x11,0x2F,0x02,0x0B,0x41,0x0A,0x0A,0x0A,0x0A,0x80,0x80,0x80,0x80,
/*40*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x04,
/*50*/   0x04,0x04,0x04,0x04,0x01,0x10,0x32,0x00,0xB8,0xB8,0xB8,0xB8,0x1E,0x1E,0x1E,0x1E,
/*60*/   0x00,0x00,0x00,0x00,0x0D,0x0D,0x0D,0x0D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*70*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x11,0x11,0x00,0x00,0x00,0x00,
/*80*/   0x00,0x00,0x00,0x00,0x00,0x00,0xAF,0x00,0xDD,0xDD,0xDD,0xDD,0x08,0x08,0x08,0x08,
/*90*/   0x6b,0x6b,0x6b,0x6b,0x80,0x80,0x80,0x80,0x00,0x00,0x55,0x55,0x84,0x84,0x84,0x84,
/*A0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*B0*/   0x11,0x11,0x11,0x11,0x2F,0x02,0x0B,0x41,0x0A,0x0A,0x0A,0x0A,0x80,0x80,0x80,0x80,
/*C0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x04,
/*D0*/   0x04,0x04,0x04,0x04,0x01,0x54,0x76,0x00,0xB8,0xB8,0xB8,0xB8,0x1E,0x1E,0x1E,0x1E,
/*E0*/   0x00,0x00,0x00,0x00,0x0D,0x0D,0x0D,0x0D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x11,0x11,0x00,0x00,0x00,0x00
};

static unsigned char NVP1918_B1_PAL[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x02,0x88,0x88,0x88,0x88,0x88,0x1A,0x80,0x03,0x00,0x10,0x32,0x54,0x76,0x98,0xBA,
/*10*/   0xDC,0xFE,0xE4,0x80,0x00,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,
/*20*/   0x88,0x88,0x80,0x10,0x18,0x16,0x00,0x00,0x00,0x88,0xFF,0xC0,0xAA,0xAA,0xAA,0xAA,
/*30*/   0xAA,0x72,0x00,0x72,0x00,0x00,0x00,0x00,0x00,0x00,0xA1,0xa0,0xa0,0x00,0x08,0x00,
/*40*/   0x00,0x00,0x00,0x00,0x14,0x01,0x10,0x40,0x60,0x00,0x03,0x10,0x00,0x00,0x00,0x00,
/*50*/   0x37,0x37,0x1B,0x05,0x00,0x07,0x00,0x09,0x00,0x00,0x08,0x06,0x00,0x46,0x00,0x00,
/*60*/   0xAA,0x1C,0x18,0xFF,0xAA,0x3C,0xFF,0xFF,0xAA,0x1B,0x00,0x00,0xAA,0x3B,0x00,0x00,
/*70*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01,
/*80*/   0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x32,0x54,0x76,
/*90*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*A0*/   0x4D,0x00,0x47,0x35,0x83,0x8a,0x8a,0x71,0x84,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
/*B0*/   0x00,0x00,0x00,0x00,0x40,0x30,0x40,0x30,0x40,0x30,0x40,0x30,0x00,0x00,0x00,0x00,
/*C0*/   0x54,0x76,0x10,0x32,0x54,0x76,0x10,0x32,0x88,0x88,0x0F,0x00,0x70,0x70,0x70,0x70,
/*D0*/   0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*E0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char NVP1918_B2_PAL[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,
/*10*/   0x00,0x00,0xFF,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*20*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*30*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*40*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*50*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*60*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*70*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*80*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*90*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*A0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*B0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*C0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*D0*/   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*E0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char NVP1918_B3_PAL[] = {
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0xE0,0x09,0x0C,0x9F,0x00,0x20,0x40,0x80,0x50,0x38,0x0F,0x00,0x04,0x10,0x30,0x04,
/*10*/   0x06,0x06,0x00,0x80,0x37,0x80,0x49,0x37,0xEF,0xDF,0xDF,0x20,0x0A,0x0C,0x00,0x88,
/*20*/   0x80,0x00,0x23,0x00,0x2A,0xCC,0xF0,0x57,0x90,0x1F,0x52,0x78,0x00,0x68,0x00,0x07,
/*30*/   0xE0,0x43,0xA2,0x00,0x00,0x17,0x25,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,
/*40*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x44,0x00,0x00,0x20,0x10,0x00,0x00,
/*50*/   0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*60*/   0x53,0x00,0x01,0x00,0x99,0x99,0x99,0x99,0x55,0x55,0x00,0x00,0x00,0x00,0x00,0x00,
/*70*/   0xB8,0x01,0x06,0x06,0x11,0x01,0xE0,0x13,0x03,0x22,0x00,0x00,0x00,0x00,0x00,0x00,
/*80*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*90*/   0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*A0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*B0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*C0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*D0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*E0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*F0*/   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static int nvp1918_clock_init(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x50); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x50); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x50); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x50); if(ret < 0) goto exit;

exit:
    return ret;
}

static int nvp1918_system_init(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* BANK 1 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4a, 0x03); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4b, 0x10); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x48, 0x60); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x48, 0x61); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x48, 0x60); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x5e, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x38, 0x10); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x38, 0x00); if(ret < 0) goto exit;

    msleep(100);

    /* Software Reset */
    ret = nvp1918_i2c_write(id, 0xFF, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x48, 0x40); if(ret < 0) goto exit;

    msleep(10);

    ret = nvp1918_i2c_write(id, 0x48, 0x60); if(ret < 0) goto exit;

exit:
    return ret;
}

static int nvp1918_ntsc_common_init(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* BANK 0 */
    ret = nvp1918_i2c_write(id, 0xff, 0x00);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B0_NTSC, sizeof(NVP1918_B0_NTSC));
    if(ret < 0)
        goto exit;

    /* BANK 1 */
    ret = nvp1918_i2c_write(id, 0xff, 0x01);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B1_NTSC, sizeof(NVP1918_B1_NTSC));
    if(ret < 0)
        goto exit;

    /* BANK 2 */
    ret = nvp1918_i2c_write(id, 0xff, 0x02);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B2_NTSC, sizeof(NVP1918_B2_NTSC));
    if(ret < 0)
        goto exit;

    /* BANK 3 */
    ret = nvp1918_i2c_write(id, 0xff, 0x03);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B3_NTSC, sizeof(NVP1918_B3_NTSC));
    if(ret < 0)
        goto exit;

    ret = nvp1918_clock_init(id);
    if(ret < 0)
        goto exit;

exit:
    return ret;
}

static int nvp1918_pal_common_init(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* Bank 0 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B0_PAL, sizeof(NVP1918_B0_PAL));
    if(ret < 0)
        goto exit;

    /* Bank 1 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x01);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B1_PAL, sizeof(NVP1918_B1_PAL));
    if(ret < 0)
        goto exit;

    /* Bank 2 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x02);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B2_PAL, sizeof(NVP1918_B2_PAL));
    if(ret < 0)
        goto exit;

    /* Bank 3 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x03);
    if(ret < 0)
        goto exit;
    ret = nvp1918_i2c_array_write(id, 0x00, NVP1918_B3_PAL, sizeof(NVP1918_B3_PAL));
    if(ret < 0)
        goto exit;

    ret = nvp1918_clock_init(id);
    if(ret < 0)
        goto exit;

exit:
    return ret;
}

static int nvp1918_ntsc_720h_4ch(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* NTSC Common */
    ret = nvp1918_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x08, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x09, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0A, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0B, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x88, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8A, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0xA0); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x18, 0x22); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x19, 0x22); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x98, 0x22); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x99, 0x22); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x35, 0x82); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xB5, 0x82); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x41, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x42, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x43, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC0, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC1, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC2, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC3, 0x01); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x4C, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4D, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4E, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4F, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x00); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x50, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x51, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x52, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x53, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD0, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD1, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD2, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD3, 0x00); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x54, 0xF1); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD4, 0xF1); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x55, 0x10); if(ret < 0) goto exit; ///< CHID_VIN2, CHID_VIN1
    ret = nvp1918_i2c_write(id, 0x56, 0x32); if(ret < 0) goto exit; ///< CHID_VIN4, CHID_VIN3
    ret = nvp1918_i2c_write(id, 0xD5, 0x54); if(ret < 0) goto exit; ///< CHID_VIN6, CHID_VIN5
    ret = nvp1918_i2c_write(id, 0xD6, 0x76); if(ret < 0) goto exit; ///< CHID_VIN8, CHID_VIN7

    ret = nvp1918_i2c_write(id, 0x58, 0x96); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x59, 0x96); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x5A, 0x96); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x5B, 0x96); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD8, 0x96); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD9, 0x96); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xDA, 0x96); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xDB, 0x96); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x64, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x65, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x66, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x67, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xe4, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xe5, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xe6, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xe7, 0x08); if(ret < 0) goto exit;

    /* Bank 1 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xA0, 0x40); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0xC0, 0x10); if(ret < 0) goto exit; ///< VPortA_SEQ2, VPortA_SEQ1
    ret = nvp1918_i2c_write(id, 0xC1, 0x32); if(ret < 0) goto exit; ///< VPortA_SEQ4, VPortA_SEQ3
    ret = nvp1918_i2c_write(id, 0xC2, 0x54); if(ret < 0) goto exit; ///< VPortB_SEQ2, VPortB_SEQ1
    ret = nvp1918_i2c_write(id, 0xC3, 0x76); if(ret < 0) goto exit; ///< VPortB_SEQ4, VPortB_SEQ3
    ret = nvp1918_i2c_write(id, 0xC4, 0x10); if(ret < 0) goto exit; ///< VPortC_SEQ2, VPortC_SEQ1
    ret = nvp1918_i2c_write(id, 0xC5, 0x32); if(ret < 0) goto exit; ///< VPortC_SEQ4, VPortC_SEQ3
    ret = nvp1918_i2c_write(id, 0xC6, 0x54); if(ret < 0) goto exit; ///< VPortD_SEQ2, VPortD_SEQ1
    ret = nvp1918_i2c_write(id, 0xC7, 0x76); if(ret < 0) goto exit; ///< VPortD_SEQ4, VPortD_SEQ3

    ret = nvp1918_i2c_write(id, 0xC8, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC9, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x70); if(ret < 0) goto exit;

    /* Bank 2 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x02); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x12, 0x00); if(ret < 0) goto exit;

    /* Bank 3 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x03); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x20, 0x80); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x25, 0xDC); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x53, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x68, 0xff); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x69, 0xff); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x2c, 0x04); if(ret < 0) goto exit; ///< keep video loss output 50MHz/60MHz BIT3 0:off, 1:on
    ret = nvp1918_i2c_write(id, 0x62, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x63, 0xFF); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x81, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x83, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x85, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x87, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8D, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8F, 0x40); if(ret < 0) goto exit;

    ret = nvp1918_system_init(id); if(ret < 0) goto exit;

    printk("NVP1918#%d ==> NTSC 720H 144MHz MODE!!\n", id);

exit:
    return ret;
}

static int nvp1918_pal_720h_4ch(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* PAL Common */
    ret = nvp1918_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* Bank 0 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x08, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x09, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0A, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0B, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x88, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8A, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0xdd); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x18, 0x33); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x19, 0x33); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x98, 0x33); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x99, 0x33); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x35, 0x02); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xB5, 0x02); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x41, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x42, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x43, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC0, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC1, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC2, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC3, 0x00); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x4C, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4D, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4E, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4F, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x50, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x51, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x52, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x53, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD0, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD1, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD2, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD3, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x54, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD4, 0x01); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x55, 0x10); if(ret < 0) goto exit; ///< CHID_VIN2, CHID_VIN1
    ret = nvp1918_i2c_write(id, 0x56, 0x32); if(ret < 0) goto exit; ///< CHID_VIN4, CHID_VIN3
    ret = nvp1918_i2c_write(id, 0xD5, 0x54); if(ret < 0) goto exit; ///< CHID_VIN6, CHID_VIN5
    ret = nvp1918_i2c_write(id, 0xD6, 0x76); if(ret < 0) goto exit; ///< CHID_VIN8, CHID_VIN7

    ret = nvp1918_i2c_write(id, 0x58, 0xa7); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x59, 0xa7); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x5A, 0xa7); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x5B, 0xa7); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD8, 0xa7); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD9, 0xa7); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xDA, 0xa7); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xDB, 0xa7); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x64, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x65, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x66, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x67, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE4, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE5, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE6, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE7, 0x0D); if(ret < 0) goto exit;

    /* Bank 1 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xA0, 0x4D); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0xC0, 0x10); if(ret < 0) goto exit; ///< VPortA_SEQ2, VPortA_SEQ1
    ret = nvp1918_i2c_write(id, 0xC1, 0x32); if(ret < 0) goto exit; ///< VPortA_SEQ4, VPortA_SEQ3
    ret = nvp1918_i2c_write(id, 0xC2, 0x54); if(ret < 0) goto exit; ///< VPortB_SEQ2, VPortB_SEQ1
    ret = nvp1918_i2c_write(id, 0xC3, 0x76); if(ret < 0) goto exit; ///< VPortB_SEQ4, VPortB_SEQ3
    ret = nvp1918_i2c_write(id, 0xC4, 0x10); if(ret < 0) goto exit; ///< VPortC_SEQ2, VPortC_SEQ1
    ret = nvp1918_i2c_write(id, 0xC5, 0x32); if(ret < 0) goto exit; ///< VPortC_SEQ4, VPortC_SEQ3
    ret = nvp1918_i2c_write(id, 0xC6, 0x54); if(ret < 0) goto exit; ///< VPortD_SEQ2, VPortD_SEQ1
    ret = nvp1918_i2c_write(id, 0xC7, 0x76); if(ret < 0) goto exit; ///< VPortD_SEQ4, VPortD_SEQ3

    ret = nvp1918_i2c_write(id, 0xC8, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC9, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x70); if(ret < 0) goto exit;

    /* Bank 2 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x02); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x12, 0x00); if(ret < 0) goto exit;

    /* Bank 3 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x03); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x20, 0x80); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x25, 0xCC); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x53, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x68, 0xff); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x69, 0xff); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x62, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x63, 0xFF); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x81, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x83, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x85, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x87, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8D, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8F, 0x40); if(ret < 0) goto exit;

    ret = nvp1918_system_init(id); if(ret < 0) goto exit;

    printk("NVP1918#%d ==> PAL 720H 144MHz MODE!!\n", id);

exit:
    return ret;
}

static int nvp1918_ntsc_960h_4ch(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* NTSC Common */
    ret = nvp1918_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* Bank 0 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x08, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x09, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0A, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0B, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x88, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8A, 0xA0); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0xA0); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x18, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x19, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x98, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x99, 0x00); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x35, 0x82); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xB5, 0x82); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x41, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x42, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x43, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC0, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC1, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC2, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC3, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4C, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4D, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4E, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4F, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x50, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x51, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x52, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x53, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD0, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD1, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD2, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD3, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x54, 0xF1); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD4, 0xF1); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x55, 0x10); if(ret < 0) goto exit; ///< CHID_VIN2, CHID_VIN1
    ret = nvp1918_i2c_write(id, 0x56, 0x32); if(ret < 0) goto exit; ///< CHID_VIN4, CHID_VIN3
    ret = nvp1918_i2c_write(id, 0xD5, 0x54); if(ret < 0) goto exit; ///< CHID_VIN6, CHID_VIN5
    ret = nvp1918_i2c_write(id, 0xD6, 0x76); if(ret < 0) goto exit; ///< CHID_VIN8, CHID_VIN7

    ret = nvp1918_i2c_write(id, 0x64, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x65, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x66, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x67, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE4, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE5, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE6, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE7, 0x08); if(ret < 0) goto exit;

    /* Bank 1 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xA0, 0x40); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0xC0, 0x10); if(ret < 0) goto exit; ///< VPortA_SEQ2, VPortA_SEQ1
    ret = nvp1918_i2c_write(id, 0xC1, 0x32); if(ret < 0) goto exit; ///< VPortA_SEQ4, VPortA_SEQ3
    ret = nvp1918_i2c_write(id, 0xC2, 0x54); if(ret < 0) goto exit; ///< VPortB_SEQ2, VPortB_SEQ1
    ret = nvp1918_i2c_write(id, 0xC3, 0x76); if(ret < 0) goto exit; ///< VPortB_SEQ4, VPortB_SEQ3
    ret = nvp1918_i2c_write(id, 0xC4, 0x10); if(ret < 0) goto exit; ///< VPortC_SEQ2, VPortC_SEQ1
    ret = nvp1918_i2c_write(id, 0xC5, 0x32); if(ret < 0) goto exit; ///< VPortC_SEQ4, VPortC_SEQ3
    ret = nvp1918_i2c_write(id, 0xC6, 0x54); if(ret < 0) goto exit; ///< VPortD_SEQ2, VPortD_SEQ1
    ret = nvp1918_i2c_write(id, 0xC7, 0x76); if(ret < 0) goto exit; ///< VPortD_SEQ4, VPortD_SEQ3

    ret = nvp1918_i2c_write(id, 0xC8, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC9, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x70); if(ret < 0) goto exit;

    /* Bank 2 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x02); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x12, 0xFF); if(ret < 0) goto exit;

    /* Bank 3 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x03); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x20, 0x80); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x62, 0x01); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x25, 0xDC); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x2c, 0x04); if(ret < 0) goto exit; ///< keep video loss output 50MHz/60MHz BIT3 0:off, 1:on
    ret = nvp1918_i2c_write(id, 0x53, 0xFF); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x63, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x80, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x81, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x83, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x85, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x87, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8D, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8F, 0x08); if(ret < 0) goto exit;

    ret = nvp1918_system_init(id); if(ret < 0) goto exit;

    printk("NVP1918#%d ==> NTSC 960H 144MHz MODE!!\n", id);

exit:
    return ret;
}

static int nvp1918_pal_960h_4ch(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* PAL Common */
    ret = nvp1918_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* Bank 0 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x08, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x09, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0A, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0B, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x88, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8A, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0xdd); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x18, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x19, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x98, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x99, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x35, 0x02); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xB5, 0x02); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x41, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x42, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x43, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC0, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC1, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC2, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC3, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4C, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4D, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4E, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x4F, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x50, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x51, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x52, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x53, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD0, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD1, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD2, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD3, 0x04); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x54, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xD4, 0x01); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x55, 0x10); if(ret < 0) goto exit; ///< CHID_VIN2, CHID_VIN1
    ret = nvp1918_i2c_write(id, 0x56, 0x32); if(ret < 0) goto exit; ///< CHID_VIN4, CHID_VIN3
    ret = nvp1918_i2c_write(id, 0xD5, 0x54); if(ret < 0) goto exit; ///< CHID_VIN6, CHID_VIN5
    ret = nvp1918_i2c_write(id, 0xD6, 0x76); if(ret < 0) goto exit; ///< CHID_VIN8, CHID_VIN7

    ret = nvp1918_i2c_write(id, 0x64, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x65, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x66, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x67, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE4, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE5, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE6, 0x0D); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xE7, 0x0D); if(ret < 0) goto exit;

    /* Bank 1 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x01); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xA0, 0x4D); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0xC0, 0x10); if(ret < 0) goto exit; ///< VPortA_SEQ2, VPortA_SEQ1
    ret = nvp1918_i2c_write(id, 0xC1, 0x32); if(ret < 0) goto exit; ///< VPortA_SEQ4, VPortA_SEQ3
    ret = nvp1918_i2c_write(id, 0xC2, 0x54); if(ret < 0) goto exit; ///< VPortB_SEQ2, VPortB_SEQ1
    ret = nvp1918_i2c_write(id, 0xC3, 0x76); if(ret < 0) goto exit; ///< VPortB_SEQ4, VPortB_SEQ3
    ret = nvp1918_i2c_write(id, 0xC4, 0x10); if(ret < 0) goto exit; ///< VPortC_SEQ2, VPortC_SEQ1
    ret = nvp1918_i2c_write(id, 0xC5, 0x32); if(ret < 0) goto exit; ///< VPortC_SEQ4, VPortC_SEQ3
    ret = nvp1918_i2c_write(id, 0xC6, 0x54); if(ret < 0) goto exit; ///< VPortD_SEQ2, VPortD_SEQ1
    ret = nvp1918_i2c_write(id, 0xC7, 0x76); if(ret < 0) goto exit; ///< VPortD_SEQ4, VPortD_SEQ3

    ret = nvp1918_i2c_write(id, 0xC8, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xC9, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCC, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCD, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCE, 0x70); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xCF, 0x70); if(ret < 0) goto exit;

    /* Bank 2 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x02); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x12, 0xFF); if(ret < 0) goto exit;

    /* Bank 3 */
    ret = nvp1918_i2c_write(id, 0xFF, 0x03); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x20, 0x80); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x62, 0x01); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0x25, 0xCC); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x53, 0xFF); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x63, 0x00); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x80, 0x40); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x81, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x83, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x85, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x87, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x89, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8B, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8D, 0x08); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x8F, 0x08); if(ret < 0) goto exit;

    ret = nvp1918_system_init(id); if(ret < 0) goto exit;

    printk("NVP1918#%d ==> PAL 960H 144MHz MODE!!\n", id);

exit:
    return ret;
}

int nvp1918_video_set_mode(int id, NVP1918_VMODE_T mode)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || mode >= NVP1918_VMODE_MAX)
        return -1;

    switch(mode) {
        case NVP1918_VMODE_NTSC_720H_4CH:
            ret = nvp1918_ntsc_720h_4ch(id);
            break;
        case NVP1918_VMODE_NTSC_960H_4CH:
            ret = nvp1918_ntsc_960h_4ch(id);
            break;
        case NVP1918_VMODE_PAL_720H_4CH:
            ret = nvp1918_pal_720h_4ch(id);
            break;
        case NVP1918_VMODE_PAL_960H_4CH:
            ret = nvp1918_pal_960h_4ch(id);
            break;
        default:
            ret = -1;
            printk("NVP1918#%d driver not support video output mode(%d)!!\n", id, mode);
            break;
    }

    return ret;
}
EXPORT_SYMBOL(nvp1918_video_set_mode);

int nvp1918_video_get_mode(int id)
{
    int ret;
    int is_ntsc;
    int is_960h;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    /* get ch#0 current register setting for determine device running mode */
    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;
    is_ntsc = (((nvp1918_i2c_read(id, 0x08)>>5)&0x03) == 0x01) ? 1 : 0;

    ret = nvp1918_i2c_write(id, 0xFF, 0x03);
    if(ret < 0)
        goto exit;
    is_960h = ((nvp1918_i2c_read(id, 0x63)&0x01) == 0x00) ? 1 : 0;

    if(is_ntsc) {
        if(is_960h)
            ret = NVP1918_VMODE_NTSC_960H_4CH;
        else
            ret = NVP1918_VMODE_NTSC_720H_4CH;
    }
    else {
        if(is_960h)
            ret = NVP1918_VMODE_PAL_960H_4CH;
        else
            ret = NVP1918_VMODE_PAL_720H_4CH;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_get_mode);

int nvp1918_video_mode_support_check(int id, NVP1918_VMODE_T mode)
{
    if(id >= NVP1918_DEV_MAX)
        return 0;

    switch(mode) {
        case NVP1918_VMODE_NTSC_720H_4CH:
        case NVP1918_VMODE_NTSC_960H_4CH:
        case NVP1918_VMODE_PAL_720H_4CH:
        case NVP1918_VMODE_PAL_960H_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(nvp1918_video_mode_support_check);

int nvp1918_video_set_contrast(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_write(id, ((ch/4) ? (0x90+(ch%4)) : (0x10+(ch%4))), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_set_contrast);

int nvp1918_video_get_contrast(int id, int ch)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_read(id, ((ch/4) ? (0x90+(ch%4)) : (0x10+(ch%4))));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_get_contrast);

int nvp1918_video_set_brightness(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_write(id, ((ch/4) ? (0x8c+(ch%4)) : (0x0c+(ch%4))), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_set_brightness);

int nvp1918_video_get_brightness(int id, int ch)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_read(id, ((ch/4) ? (0x8c+(ch%4)) : (0x0c+(ch%4))));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_get_brightness);

int nvp1918_video_set_saturation(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_write(id, ((ch/4) ? (0xbc+(ch%4)) : (0x3c+(ch%4))), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_set_saturation);

int nvp1918_video_get_saturation(int id, int ch)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_read(id, ((ch/4) ? (0xbc+(ch%4)) : (0x3c+(ch%4))));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_get_saturation);

int nvp1918_video_set_hue(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_write(id, ((ch/4) ? (0xc0+(ch%4)) : (0x40+(ch%4))), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_set_hue);

int nvp1918_video_get_hue(int id, int ch)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_read(id, ((ch/4) ? (0xc0+(ch%4)) : (0x40+(ch%4))));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_get_hue);

int nvp1918_video_set_sharpness(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_write(id, ((ch/4) ? (0x94+(ch%4)) : (0x14+(ch%4))), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_set_sharpness);

int nvp1918_video_get_sharpness(int id, int ch)
{
    int ret;

    if(id >= NVP1918_DEV_MAX || ch >= NVP1918_DEV_CH_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_read(id, ((ch/4) ? (0x94+(ch%4)) : (0x14+(ch%4))));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_video_get_sharpness);

int nvp1918_status_get_novid(int id)
{
    int ret;

    if(id >= NVP1918_DEV_MAX)
        return -1;

    ret = nvp1918_i2c_write(id, 0xFF, 0x01);
    if(ret < 0)
        goto exit;

    ret = nvp1918_i2c_read(id, 0xD8);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_status_get_novid);

static int nvp1918_audio_set_master_chan_seq(int id, char *RSEQ)
{
    u16 data;
    int ret;

    data = RSEQ[0];
    ret = nvp1918_i2c_write(id, 0x0A, data); if(ret < 0) goto exit;
    data = RSEQ[1];
    ret = nvp1918_i2c_write(id, 0x0B, data); if(ret < 0) goto exit;
    data = RSEQ[2];
    ret = nvp1918_i2c_write(id, 0x0C, data); if(ret < 0) goto exit;
    data = RSEQ[3];
    ret = nvp1918_i2c_write(id, 0x0D, data); if(ret < 0) goto exit;
    data = RSEQ[4];
    ret = nvp1918_i2c_write(id, 0x0E, data); if(ret < 0) goto exit;
    data = RSEQ[5];
    ret = nvp1918_i2c_write(id, 0x0F, data); if(ret < 0) goto exit;
    data = RSEQ[6];
    ret = nvp1918_i2c_write(id, 0x10, data); if(ret < 0) goto exit;
    data = RSEQ[7];
    ret = nvp1918_i2c_write(id, 0x11, data); if(ret < 0) goto exit;
exit:
    return ret;
}

static int nvp1918_audio_set_slave_chan_seq(int id)
{
    int ret;
    // L: 1/2/3/4/5/6/7/8
    ret = nvp1918_i2c_write(id, 0x0A, 0x10); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0B, 0x32); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0C, 0x54); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0D, 0x76); if(ret < 0) goto exit;

    // R: 9/10/11/12/13/14/15/16
    ret = nvp1918_i2c_write(id, 0x0E, 0x98); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x0F, 0xBA); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x10, 0xDC); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x11, 0xFE); if(ret < 0) goto exit;
exit:
    return ret;
}

static int nvp1918_audio_init(int id, int ch_num, int master)
{
    int ret;
    int banksave = 0;
    int cascade  = 0;
    unsigned char bitrates   = AUDIO_BITS_8B;
    unsigned char samplerate = AUDIO_SAMPLERATE_16K;
    //u32 chip_id = ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16;
    int i, j, ch, ch1;
    char RSEQ[8] = {0};
    u32 *temp;
    u16 *temp1;

    if (ch_num == 16)
        cascade = 1;

    /* NVP1918 just support 2/4/8/16 channel number */
    if (ch_num >= 8){
        bitrates = AUDIO_BITS_8B;
    }
    else if (ch_num == 4){
        bitrates = AUDIO_BITS_16B;
    }
    else if (ch_num == 2){
        bitrates = AUDIO_BITS_16B;
    }

    banksave = nvp1918_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1918_i2c_write(id, 0xff, 0x01); if(ret < 0) goto exit;     ///< select bank 1
    ret = nvp1918_i2c_write(id, 0x22, 0x00); if(ret < 0) goto exit;     ///< Audio No MIXed out - Gain 0 (AOGAIN=0)
    ret = nvp1918_i2c_write(id, 0x23, 0x19); if(ret < 0) goto exit;     ///< Audio No MIXed out - No audio Output (MIX_OUTSEL=25)
    ret = nvp1918_i2c_write(id, 0x01, 0x88); if(ret < 0) goto exit;     ///< Audio input gain init - Gain 1.0
    ret = nvp1918_i2c_write(id, 0x02, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x03, 0x88); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x04, 0x88); if(ret < 0) goto exit;

    if (cascade) {
        // at gm8210, 0(0x66)/2(0x62) is master --> first stage, 1(0x64)/3(0x60) is slave --> last stage
        // at gm8287, 0(0x64) is master --> first stage, 1(0x66) is slave --> last stage
        // at gm8282, 0(0x60) is master --> first stage, 1(0x64) is slave --> last stage
        if ((id == 1)||(id == 3)) { // configure slave
            ret = nvp1918_i2c_write(id, 0x06, 0x19); if(ret < 0) goto exit;                                         ///< RM cascade last stage
            ret = nvp1918_i2c_write(id, 0x07, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;   ///< RM master
            ret = nvp1918_i2c_write(id, 0x13, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;   ///< PB slave
        }
        else {                      // configure master
            ret = nvp1918_i2c_write(id, 0x06, 0x1a); if(ret < 0) goto exit;                                         ///< RM cascade first stage
            ret = nvp1918_i2c_write(id, 0x07, 0x80 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;   ///< RM master
            ret = nvp1918_i2c_write(id, 0x13, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;   ///< PB slave
        }
    }
    else {
		ret = nvp1918_i2c_write(id, 0x23, 0x20 | 0x10); if(ret < 0) goto exit;
        ret = nvp1918_i2c_write(id, 0x06, 0x1B); if(ret < 0) goto exit;                                             ///< RM single operation (CHIP_STAGE=3)
        ret = nvp1918_i2c_write(id, 0x07, 0x80 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;       ///< RM master
        ret = nvp1918_i2c_write(id, 0x13, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;       ///< PB slave
    }
	//If  codec is slave(I2S is master), re-config all codec chip as slave (need to comfirm when cascade)
	if (master == 0) 
		ret = nvp1918_i2c_write(id, 0x07, (0x00 << 7) | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;       ///< RM slave

    if (2 == ch_num) {
        ret = nvp1918_i2c_write(id, 0x08, 0x00); if(ret < 0) goto exit;            ///< RM channel number 2
    } else if (4 == ch_num) {
        ret = nvp1918_i2c_write(id, 0x08, 0x01); if(ret < 0) goto exit;            ///< RM channel number 4
    } else if (8 == ch_num) {
        ret = nvp1918_i2c_write(id, 0x08, 0x02); if(ret < 0) goto exit;            ///< RM channel number 8
    } else if (16 == ch_num) {
        ret = nvp1918_i2c_write(id, 0x08, 0x03); if(ret < 0) goto exit;            ///< RM channel number 16
    }

    if (cascade) {
        if ((id == 0)||(id == 2)) {     // 0/2 is master, configure master
            for(i = 0; i < 4; i++) {
                j = i * 2;
                ch = nvp1918_vin_to_ch(id, j);
                ch1 = nvp1918_vin_to_ch(id, j+1);
                RSEQ[i] = (ch1 << 4 | ch);

                ch = nvp1918_vin_to_ch(id + 1, j);
                ch1 = nvp1918_vin_to_ch(id + 1, j+1);
                ch += 8;
                ch1 += 8;
                RSEQ[i+4] = (ch1 << 4 | ch);
            }
            for (i = 0; i < 8; i++)
                SWAP_2CHANNEL(RSEQ[i]);

            for (i = 0; i < 2; i++) {
                j = i*4;
                temp = (u32 *)&RSEQ[j];
                SWAP_8CHANNEL(*temp);
            }
            ret = nvp1918_audio_set_master_chan_seq(id, RSEQ);
        }
        else    // 1/3 is slave
            ret = nvp1918_audio_set_slave_chan_seq(id);
    }
    else {   // 8 channel
        for(i = 0; i < 4; i++) {
            j = i * 2;
            ch = nvp1918_vin_to_ch(id, j);
            ch1 = nvp1918_vin_to_ch(id, j+1);
            RSEQ[i] = (ch1 << 4 | ch);
        }

        for (i = 0; i < 8; i++)
            SWAP_2CHANNEL(RSEQ[i]);

        for (i = 0; i < 4; i++) {
            j = i*2;
            temp1 = (u16 *)&RSEQ[j];
            SWAP_4CHANNEL(*temp1);
        }

        RSEQ[4] = RSEQ[2];
        RSEQ[5] = RSEQ[3];
        RSEQ[2] = 0x98;
        RSEQ[3] = 0xba;
        RSEQ[6] = 0xdc;
        RSEQ[7] = 0xfe;

        ret = nvp1918_audio_set_master_chan_seq(id, RSEQ);
    }

    ret = nvp1918_i2c_write(id, 0x23, 0x10); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0x22, 0x80); if(ret < 0) goto exit;          ///< Audio No MIXed out - Gain 1.0 (AOGAIN=8)
    ret = nvp1918_i2c_write(id, 0x14, 0x00); if(ret < 0) goto exit;          ///< playback select channel - channel 0 (PB_SEL=0)
	/*
		It is make sense to rise RM_DELAY(bit_7) and PB_DELAY(bit_6) to 1 to fit I2S (0: not dealy.  1 : delay 1 clk)
		If not rise, the result would be noisy when 1918 is slave
	*/
	ret = nvp1918_i2c_write(id, 0x39, (0x01 << 7) | (0x01 << 6)); if(ret < 0) goto exit;   

    ret = nvp1918_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;      ///< restore bank setting

exit:
    return ret;
}

static int nvp1918_audio_set_sample_rate(int id, NVP1918_SAMPLERATE_T samplerate)
{
    int banksave, data, umask;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1918_i2c_write(id, 0xff, 0x01); if(ret < 0) goto exit;          ///< select bank 1
    data = nvp1918_i2c_read(id, 0x07);
    umask = (unsigned char)(AUDIO_SAMPLERATE_16K << 3);
    data &= ~umask;
    data |= (samplerate << 3);
    ret = nvp1918_i2c_write(id, 0x07, data); if(ret < 0) goto exit;

    data = nvp1918_i2c_read(id, 0x13);
    umask = (unsigned char)(AUDIO_SAMPLERATE_16K << 3);
    data &= ~umask;
    data |= (samplerate << 3);
    ret = nvp1918_i2c_write(id, 0x13, data); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;      ///< restore bank setting

exit:
    return ret;
}

static int nvp1918_audio_set_sample_size(int id, NVP1918_SAMPLESIZE_T samplesize)
{
    int banksave, data, umask;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1918_i2c_write(id, 0xff, 0x01); if(ret < 0) goto exit;          ///< select bank 1
    data = nvp1918_i2c_read(id, 0x07);
    umask = (unsigned char)(AUDIO_BITS_8B << 2);
    data &= ~umask;
    data |= (samplesize << 2);
    ret = nvp1918_i2c_write(id, 0x07, data); if(ret < 0) goto exit;

    data = nvp1918_i2c_read(id, 0x13);
    umask = (unsigned char)(AUDIO_BITS_8B << 2);
    data &= ~umask;
    data |= (samplesize << 2);
    ret = nvp1918_i2c_write(id, 0x13, data); if(ret < 0) goto exit;

    ret = nvp1918_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;      ///< restore bank setting

exit:
    return ret;
}

int nvp1918_audio_set_mute(int id, int on)
{
    int banksave;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);      ///< save bank
    ret = nvp1918_i2c_write(id, 0xff, 0x01);  if(ret < 0) goto exit;             ///< select bank 1
    ret = nvp1918_i2c_write(id, 0x22, on ? 0x00 : 0x80); if(ret < 0) goto exit;  ///< Audio No MIXed out - Gain 0|1 (AOGAIN=0|1)
    ret = nvp1918_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;          ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_audio_set_mute);

int nvp1918_audio_get_mute(int id)
{
    int banksave, aogain;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);       ///< save bank

    ret = nvp1918_i2c_write(id, 0xff, 0x01); if(ret < 0) goto exit;    ///< select bank 1
    aogain = nvp1918_i2c_read(id, 0x22);
    aogain = aogain >> 4;

    nvp1918_i2c_write(id, 0xff, banksave);       ///< restore bank setting

    return aogain;

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_audio_get_mute);

int nvp1918_audio_set_volume(int id, int volume)
{
    int banksave;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);      ///< save bank
    ret = nvp1918_i2c_write(id, 0xff, 0x01);  if(ret < 0) goto exit;             ///< select bank 1
    ret = nvp1918_i2c_write(id, 0x22, (volume << 4)); if(ret < 0) goto exit;     ///< AOGAIN setting
    ret = nvp1918_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;          ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_audio_set_volume);

int nvp1918_audio_get_output_ch(int id)
{
    int banksave, ch;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);       ///< save bank

    ret = nvp1918_i2c_write(id, 0xff, 0x01); if(ret < 0) goto exit;    ///< select bank 1
    ch = nvp1918_i2c_read(id, 0x23);
    ch &= 0x1f;

    nvp1918_i2c_write(id, 0xff, banksave);       ///< restore bank setting

    return ch;

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_audio_get_output_ch);

int nvp1918_audio_set_output_ch(int id, int ch)
{
    int banksave, data;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);      ///< save bank
    ret = nvp1918_i2c_write(id, 0xff, 0x01);  if(ret < 0) goto exit;             ///< select bank 1
    data = nvp1918_i2c_read(id, 0x23);
    data &= 0x20;
    data |= ch;
    ret = nvp1918_i2c_write(id, 0x23, data); if(ret < 0) goto exit;              ///< MIX_OUTSEL
    ret = nvp1918_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;          ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_audio_set_output_ch);

int nvp1918_audio_set_play_ch(int id, unsigned char ch)
{
    int banksave;
    int ret;

    banksave = nvp1918_i2c_read(id, 0xff);      ///< save bank
    ret = nvp1918_i2c_write(id, 0xff, 0x01); if(ret < 0) goto exit;          ///< select bank 1
    ret = nvp1918_i2c_write(id, 0x14, (ch & 0x0F)); if(ret < 0) goto exit;
    ret = nvp1918_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;      ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_audio_set_play_ch);

int nvp1918_audio_set_mode(int id, NVP1918_VMODE_T mode, NVP1918_SAMPLESIZE_T samplesize, NVP1918_SAMPLERATE_T samplerate, int master)
{
    int ch_num = 0;
    int ret;

    ch_num = audio_chnum;
    if(ch_num < 0)
        return -1;

    /* nvp1918 audio initialize */
    ret = nvp1918_audio_init(id, ch_num, master); if(ret < 0) goto exit;

    /* set audio sample rate */
    ret = nvp1918_audio_set_sample_rate(id, samplerate); if(ret < 0) goto exit;

    /* set audio sample size */
    ret = nvp1918_audio_set_sample_size(id, samplesize); if(ret < 0) goto exit;

    /* set nvp1918 AOGAIN mute */
    ret = nvp1918_audio_set_mute(id, 0); if(ret < 0) goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1918_audio_set_mode);
