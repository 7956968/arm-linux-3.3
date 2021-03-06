/**
 * @file plat_8139.c
 * platform releated api for GM8139
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.18 $
 * $Date: 2016/01/11 10:14:46 $
 *
 * ChangeLog:
 *  $Log: plat_8139.c,v $
 *  Revision 1.18  2016/01/11 10:14:46  sainthuang
 *  remove comment typo
 *
 *  Revision 1.17  2016/01/11 09:14:52  sainthuang
 *  turn off ssp0 and ssp 1 clock 
 *
 *  Revision 1.16  2014/09/02 06:28:06  jerson_l
 *  1. set external clock default driving from 2mA to 4mA
 *
 *  Revision 1.15  2014/07/16 02:01:48  ccsu
 *  add ssp1 pinmux pmu setting
 *
 *  Revision 1.14  2014/07/14 09:52:30  jerson_l
 *  1. change VERSION definition to PLAT_COMMON_VERSION
 *
 *  Revision 1.13  2014/07/14 07:23:52  ccsu
 *  move version control define to platform.h
 *
 *  Revision 1.12  2014/07/14 06:48:28  ccsu
 *  add version control
 *
 *  Revision 1.11  2014/07/14 02:44:00  jerson_l
 *  1. add ext_clk driving control api
 *
 *  Revision 1.10  2014/06/17 11:01:32  ccsu
 *  modify plat_set_ssp_clk func to select ssp clock source from pll2 or pll3
 *
 *  Revision 1.9  2014/02/24 05:53:52  ccsu
 *  1. add plat_set_ssp_clk to set ssp0/ssp1 main clock
 *  2. add plat_set_ssp1_src to set ssp1 source from ADDA or decoder
 *
 *  Revision 1.8  2014/01/22 12:57:56  easonli
 *  *:[frontend] add digital_codec_name for compiling
 *
 *  Revision 1.7  2013/12/18 06:48:00  easonli
 *  *:[frontend] modify codec_name function
 *
 *  Revision 1.6  2013/12/06 07:53:08  easonli
 *  *:[frontend] fixed ssp settings of GM8139
 *
 *  Revision 1.5  2013/11/29 05:34:12  ccsu
 *  add ssp 0/1 pmu setting
 *
 *  Revision 1.4  2013/11/19 02:53:50  easonli
 *  *:[frontend] add codec_name for 8139
 *
 *  Revision 1.3  2013/11/04 09:23:36  easonli
 *  *:[frontend] add audio functions
 *
 *  Revision 1.2  2013/10/31 05:17:56  jerson_l
 *  1. add SSCG control api
 *  2. add GPIO#59 support
 *  3. add clock source from OSC_30MHz support
 *
 *  Revision 1.1  2013/10/14 02:42:01  jerson_l
 *  1. add GM8139 platfrom support
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <asm/gpio.h>
#include <mach/ftpmu010.h>
#include <mach/fmem.h>
#include <linux/synclink.h>

#include "platform.h"

#define PLAT_EXTCLK_MAX             1       ///< X_EXT_CLKOUT
#define PLAT_X_CAP_RST_GPIO_PIN     5       ///< GPIO#5 as X_CAP_RST pin

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_SEMAPHORE(plat_lock);
#else
static DECLARE_MUTEX(plat_lock);
#endif

#define PINMUX_54   0
#define PINMUX_58   0
#define PINMUX_64   0

static int plat_pmu_fd = -1;
static int plat_gpio_init[PLAT_GPIO_ID_MAX] = {[0 ... (PLAT_GPIO_ID_MAX - 1)] = 0};
static audio_funs_t plat_audio_funs;

/*
 * GM8139 PMU registers for video decoder
 */
static pmuReg_t plat_pmu_reg_8139[] = {
    /*
     * IP Main Clock Select Setting Register [offset=0x28]
     * ----------------------------------------------------
     * [1]  ext_clkout_src  ==> 0: PLL3(540/594MHz), 1: OSCH(30MHz)
     * [14] ssp0 clk        ==> 0: PLL1(540Mhz), 1: PLL2(400MHz)
     * [15] ssp1 clk        ==> 0: PLL1(540Mhz), 1: PLL2(400MHz)
     * [18] ext_clkout_sscg ==> 0: bypass, 1: SSCG
     */
    {
     .reg_off   = 0x28,
     .bits_mask = 0x0004c002,
     .lock_bits = 0x00000000,   ///< default don't lock these bit
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * Driving Capacity and Slew Rate and Hold time Control 0 [offset=0x40]
     * ----------------------------------------------------
     * [21:20] ext_clkout_driving ==> 0:2mA 1:4mA 2:6mA 3:8mA
     * [22]    ext_clkout_schmitt ==> 0:normal 1: schmitt-triggre
     * [23]    ext_clkout_slew    ==> 0:fast 1:slow
     */
    {
     .reg_off   = 0x40,
     .bits_mask = 0x00f00000,
     .lock_bits = 0x00000000,   ///< default don't lock these bit
     .init_val  = 0x00100000,   ///< default driving to 4mA
     .init_mask = 0x00300000,
    },

    /*
     * Multi-Function Port Setting Register 0 [offset=0x50]
     * ----------------------------------------------------
     * [13:12] ext_clkout_pin ==> 0: GPIO_0[6], 1: EXT_CLKOUT
     */
    {
     .reg_off   = 0x50,
     .bits_mask = 0x00003000,
     .lock_bits = 0x00000000,   ///< default don't lock these bit
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * Multi-Function Port Setting Register 5 [offset=0x64]
     * ----------------------------------------------------
     * [11:8] spi1_sclk_pin ==> 0: GPIO_1[27], 1: SPI1_SCLK
     */
    {
     .reg_off   = 0x64,
     .bits_mask = 0x00000f00,
     .lock_bits = 0x00000000,   ///< default don't lock these bit
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * SSCG Register                          [offset=0x6c]
     * ----------------------------------------------------
     * [17:16] ext_clkout_sscg_mr ==> 0~3
     */
    {
     .reg_off   = 0x6c,
     .bits_mask = 0x00030000,
     .lock_bits = 0x00000000,   ///< default don't lock these bit
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * EXT/panel pixel/LCDC scalar -- x-bit counter [offset=0x78]
     * ----------------------------------------------------
     * [21:16] ext_clkout_div
     */
    {
     .reg_off   = 0x78,
     .bits_mask = 0x003f0000,
     .lock_bits = 0x00000000,   ///< default don't lock these bit
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * hardcore clk off : 1-gating clock [offset=0xac]
     * ----------------------------------------------------
     * [4]  ext_clkout_gate      => 0: not gating 1: gating
     * [16] ext_clkout_sscg_gate => 0: not gating 1: gating
     */
    {
     .reg_off   = 0xac,
     .bits_mask = 0x00010010,
     .lock_bits = 0x00000000,   ///< default don't lock these bit
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * SSP/ADC/ADDA x-bit counter
     * -------------------------------------------------
     * [5:0]  SSP0 clock divided value ==> 44 (540/(44+1) = 12Mhz)
     * [13:8] SSP1 clock divided value ==> 44 (540/(44+1) = 12Mhz)
     */
    {
     .reg_off   = 0x74,
     .bits_mask = (0x3f << 8) | (0x3f),
     .lock_bits = 0x0,
     .init_val  = (0x2c << 8) | (0x2c),
     .init_mask = (0x3f << 8) | (0x3f),
    },

    /*
     * system control register
     * -------------------------------------------------
     * [29] SSP1 source select ==> 0: ADDA(DA) 1: external pin
     */
    {
     .reg_off   = 0x7c,
     .bits_mask = 0x1 << 29,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x1 << 29,
    },

    /*
     * SSP0~1 pclk & sclk [offset=0xb8]
     * ---------------------------------------------------------
     * [4]  ssp0 pclk & sclk
     * [5]  ssp1 pclk & sclk
     */
     {
     .reg_off   = 0xb8,
     .bits_mask = 0x3 << 4,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x3 << 4,
    },
#if PINMUX_54
    /*
     * ssp1 pinmux with CAP0 D4~D7, set mode 3
     * -------------------------------------------------
     * [15:14] CAP0_D4
     * [17:16] CAP0_D5
     * [19:18] CAP0_D6
     * [21:20] CAP0_D7
     */
    {
     .reg_off   = 0x54,
     .bits_mask = 0xff << 14,
     .lock_bits = 0x0,
     .init_val  = (0x3 << 14) | (0x3 << 16) | (0x3 << 18) | (0x3 << 20),
     .init_mask = 0xff << 14,
    },
#endif
#if PINMUX_58
    /*
     * ssp1 pinmux with NAND D4~D7, set mode 1
     * -------------------------------------------------
     * [11:10] NAND_D4
     * [13:12] NAND_D5
     * [15:14] NAND_D6
     * [17:16] NAND_D7
     */
    {
     .reg_off   = 0x58,
     .bits_mask = 0xff << 10,
     .lock_bits = 0x0,
     .init_val  = (0x1 << 10) | (0x1 << 12) | (0x1 << 14) | (0x1 << 16),
     .init_mask = 0xff << 10,
    },
#endif
#if PINMUX_64
    /*
     * ssp1 pinmux with SPI SCLK/RXD/TXD/FS, set mode 1
     * -------------------------------------------------
     * [3:2] SPI1_FS
     * [5:4] SPI1_TXD
     * [7:6] SPI1_RXD
     * [11:8] SPI1_SCLK
     */
    {
     .reg_off   = 0x64,
     .bits_mask = 0xffc,
     .lock_bits = 0x0,
     .init_val  = (0x1 << 2) | (0x1 << 4) | (0x1 << 6) | (0x1 << 8) ,
     .init_mask = 0xffc,
    },
#endif
};

static pmuRegInfo_t plat_pmu_reg_info_8139 = {
    "FE_8139",
    ARRAY_SIZE(plat_pmu_reg_8139),
    ATTR_TYPE_NONE,
    &plat_pmu_reg_8139[0]
};

/*
 * in GM8138&GM8139, SSP1 source can select from ADDA'DA or decoder
 * index = 0 for ADDA, index = 1 for external decoder
 */
int plat_set_ssp1_src(int index)
{
    int ret = 0;
    u32 tmp = 0;
    u32 mask = 0;

    if (plat_pmu_fd < 0)
        return -1;

    if (index > 1) {
        printk("%s fails: plat_set_ssp1_src not OK\n", __FUNCTION__);
        return -EINVAL;
    }

    if (index == PLAT_SRC_ADDA)         // source select ADDA
        tmp = 0;

    if (index == PLAT_SRC_DECODER)      // source select decoder
        tmp = 0x1 << 29;

    mask = BIT29;

    down(&plat_lock);

    if (ftpmu010_write_reg(plat_pmu_fd, 0x7c, tmp, mask) < 0) {
        panic("In %s: setting SSP1 offset(0x%x) failed.\n", __func__, 0x7c);
    }

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_set_ssp1_src);

/* in GM8138&GM8139,
   ADDA main clock is 12MHz, SSP main clock can be 12MHz or 24MHz
   SSP mclk = 12Mhz is not fit to all sample rate,
   so we need to modify ssp main clock.
*/
int plat_set_ssp_clk(int cardno, int mclk, int clk_pll2)
{
    int ret = 0;
    u32 tmp = 0;
    u32 mask = 0;
    unsigned long div = 0;

    if (plat_pmu_fd < 0)
        return -1;

    /* gm8139 should only use SSP0 and SSP1 as audio interface */
    if (unlikely((cardno != 0) && (cardno != 1))) {
        printk("%s fails: plat_set_ssp_clk not OK\n", __FUNCTION__);
        return -EINVAL;
    }

    if (cardno == 0)
        mask = (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5);

    if (cardno == 1)
        mask = (BIT8|BIT9|BIT10|BIT11|BIT12|BIT13);

    if (clk_pll2 == 0)      // clock source from pll3
        div = PLL3_CLK_IN;
    if (clk_pll2 == 1)      // clock source from pll2/2
        div = PLL2_CLK_IN / 2;

    do_div(div, mclk);
    div -= 1;

    if (cardno == 0)
        tmp |= (div << 0);

    if (cardno == 1)
        tmp |= (div << 8);

    down(&plat_lock);
    // set clock divided value
    if (ftpmu010_write_reg(plat_pmu_fd, 0x74, tmp, mask) < 0) {
        panic("In %s: setting SSP(%d) offset(0x%x) failed.\n", __func__, cardno, 0x74);
    }
    // set clock source, ssp0 default clock source from pll3, set from pll2
    if (clk_pll2 == 1) {
        tmp = 1 << 14;
        mask = BIT14;
        if (ftpmu010_write_reg(plat_pmu_fd, 0x28, tmp, mask) < 0)
            panic("In %s: setting SSP(%d) offset(0x%x) failed.\n", __func__, cardno, 0x28);
    }

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_set_ssp_clk);

int plat_extclk_onoff(int on)
{
    int ret = 0;

    if(plat_pmu_fd < 0)
        return -1;

    down(&plat_lock);

    if(on) {
        /* switch pin to external clock */
        ret = ftpmu010_write_reg(plat_pmu_fd, 0x50, 0x1<<12, 0x3<<12);
        if(ret < 0)
            goto exit;

        /* disable external clock gate */
        ret = ftpmu010_write_reg(plat_pmu_fd, 0xac, 0, 0x1<<4);
        if(ret < 0)
            goto exit;
    }
    else {
        /* enable external clock and SSCG clock gate */
        ret = ftpmu010_write_reg(plat_pmu_fd, 0xac, (0x1<<4)|(0x1<<16), (0x1<<4)|(0x1<<16));
        if(ret < 0)
            goto exit;
    }

exit:
    if(ret < 0)
        printk("%s EXT_CLK output failed!!\n", ((on == 0) ? "disable" : "enable"));

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_onoff);

int plat_extclk_set_freq(int clk_id, u32 freq)
{
    int ret = 0;
    u32 src_clk;
    u32 tmp, div;

    if(plat_pmu_fd < 0)
        return -1;

    if(clk_id >= PLAT_EXTCLK_MAX || freq == 0)
        return -1;

    down(&plat_lock);

    /* get external clock source */
    tmp = ftpmu010_read_reg(0x28);
    switch((tmp>>1)&0x1) {
        case 1: /* OSC_30MHz */
            src_clk = 30000000;
            div     = src_clk/freq;
            break;
        default: /* PLL3 */
            src_clk = PLL3_CLK_IN;
            div     = ftpmu010_clock_divisor2(ATTR_TYPE_PLL3, freq, 1);
            break;
    }

    if(div)
        div--;

    /* set external clock divide value */
    ret = ftpmu010_write_reg(plat_pmu_fd, 0x78, (div&0x3f)<<16, 0x3f<<16);

    if(ret < 0)
        printk("set platform EXT_CLK#%d frequency(%uHz) failed!!\n", clk_id, freq);
    else
        printk("EXT_CLK#%d output ==> %uHz\n", clk_id, src_clk/(div+1));

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_set_freq);

int plat_extclk_set_sscg(PLAT_EXTCLK_SSCG_T sscg_mr)
{
    int ret = 0;

    switch(sscg_mr) {
        case PLAT_EXTCLK_SSCG_MR0:
        case PLAT_EXTCLK_SSCG_MR1:
        case PLAT_EXTCLK_SSCG_MR2:
        case PLAT_EXTCLK_SSCG_MR3:
            /* disable SSCG clock gate */
            ret = ftpmu010_write_reg(plat_pmu_fd, 0xac, 0, (0x1<<16));
            if(ret < 0)
                goto exit;

            /* switch SSCG MR Level */
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x6c, ((sscg_mr-1)<<16), (0x3<<16));
            if(ret < 0)
                goto exit;

            /* set cap_clk out from SSCG */
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, (0x1<<18), (0x1<<18));
            if(ret < 0)
                goto exit;
            break;
        case PLAT_EXTCLK_SSCG_DISABLE:
        default:
            /* set cap_clk out SSCG bypass */
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, 0, (0x1<<18));
            if(ret < 0)
                goto exit;

            /* enable SSCG clock gate */
            ret = ftpmu010_write_reg(plat_pmu_fd, 0xac, (0x1<<16), (0x1<<16));
            if(ret < 0)
                goto exit;
            break;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(plat_extclk_set_sscg);

int plat_extclk_set_driving(PLAT_EXTCLK_DRIVING_T driving)
{
    int ret = 0;

    if(plat_pmu_fd < 0 || driving >= PLAT_EXTCLK_DRIVING_MAX)
        return -1;

    down(&plat_lock);

    /* external clock driving setting */
    ret = ftpmu010_write_reg(plat_pmu_fd, 0x40, (driving&0x3)<<20, 0x3<<20);
    if(ret < 0)
        goto exit;

exit:
    if(ret < 0)
        printk("EXT_CLK driving setup failed!!\n");

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_set_driving);

int plat_audio_register_function(audio_funs_t *func)
{
    plat_audio_funs.codec_name = func->codec_name;
    plat_audio_funs.digital_codec_name = func->digital_codec_name;
    if (func->sound_switch == NULL) {
        printk(KERN_ERR "%s: sound_switch is NULL\n", __func__);
        return -EFAULT;
    }
    plat_audio_funs.sound_switch = func->sound_switch;
    return 0;
}
EXPORT_SYMBOL(plat_audio_register_function);

void plat_audio_deregister_function(void)
{
    plat_audio_funs.codec_name = NULL;
    plat_audio_funs.sound_switch = NULL;
}
EXPORT_SYMBOL(plat_audio_deregister_function);

int plat_audio_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    if (plat_audio_funs.sound_switch) {
        return plat_audio_funs.sound_switch(ssp_idx, chan_num, is_on);
    } else {
        printk(KERN_WARNING"%s: No audio codec is registered\n", __func__);
        return 0;
    }
}
EXPORT_SYMBOL(plat_audio_sound_switch);

const char *plat_audio_get_codec_name(void)
{
    if (plat_audio_funs.codec_name)
        return plat_audio_funs.codec_name;
    else
        return "ADDA302";
}
EXPORT_SYMBOL(plat_audio_get_codec_name);

const char *plat_audio_get_digital_codec_name(void)
{
    if (plat_audio_funs.digital_codec_name)
        return plat_audio_funs.digital_codec_name;
    else
        return "ADDA302";
}
EXPORT_SYMBOL(plat_audio_get_digital_codec_name);

int plat_extclk_set_src(PLAT_EXTCLK_SRC_T src)
{
    int ret = 0;

    if(plat_pmu_fd < 0)
        return -1;

    down(&plat_lock);

    switch(src) {
        case PLAT_EXTCLK_SRC_PLL3:
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, 0, 0x2);
            break;
        case PLAT_EXTCLK_SRC_OSC_30MHZ:
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, 0x2, 0x2);
            break;
        default:
            ret = -1;
            break;
    }

    if(ret < 0) {
        printk("set platform external clock source failed!!\n");
    }

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_set_src);

int plat_register_gpio_pin(PLAT_GPIO_ID_T gpio_id, PLAT_GPIO_DIRECTION_T direction, int out_value)
{
    int ret = 0;
    u32 bit_mask;

    if(plat_pmu_fd < 0)
        return -1;

    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_NONE:
            break;
        case PLAT_GPIO_ID_X_CAP_RST:
            /* GPIO pin request */
            if(gpio_request(PLAT_X_CAP_RST_GPIO_PIN, "x_cap_rst") < 0) {
                printk("X_CAP_RST gpio request failed!\n");
                ret = -1;
                goto err;
            }

            /* Config direction */
            if(direction == PLAT_GPIO_DIRECTION_IN)
                gpio_direction_input(PLAT_X_CAP_RST_GPIO_PIN);
            else
                gpio_direction_output(PLAT_X_CAP_RST_GPIO_PIN, out_value);

            plat_gpio_init[gpio_id] = 1;
            break;
        case PLAT_GPIO_ID_59:
            /* check pmu bit lock or not? */
            bit_mask = 0xf<<8;
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x64, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x64, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x64 bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x64, 0, bit_mask);
            if(ret < 0)
                goto err;

            if(gpio_request(59, "gpio_59") < 0) {
                printk("gpio#59 request failed!\n");
                ret = -1;
                goto err;
            }

            /* Config direction */
            if(direction == PLAT_GPIO_DIRECTION_IN)
                gpio_direction_input(59);
            else
                gpio_direction_output(59, out_value);

            plat_gpio_init[gpio_id] = 1;
            break;
        default:
            printk("register gpio pin failed!(unknown gpio_id:%d)\n", gpio_id);
            ret = -1;
            goto err;
    }

err:
    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_register_gpio_pin);

void plat_deregister_gpio_pin(PLAT_GPIO_ID_T gpio_id)
{
    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_X_CAP_RST:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(PLAT_X_CAP_RST_GPIO_PIN);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_59:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(59);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        default:
            break;
    }

    up(&plat_lock);
}
EXPORT_SYMBOL(plat_deregister_gpio_pin);

int plat_gpio_set_value(PLAT_GPIO_ID_T gpio_id, int value)
{
    int ret = 0;

    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_NONE:
            break;
        case PLAT_GPIO_ID_X_CAP_RST:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(PLAT_X_CAP_RST_GPIO_PIN, value);
            else {
                printk("gpio set value failed!(un-register gpio_id:%d)\n", gpio_id);
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_59:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(59, value);
            else {
                printk("gpio set value failed!(un-register gpio_id:%d)\n", gpio_id);
                ret = -1;
            }
            break;
        default:
            printk("gpio set value failed!(unknown gpio_id:%d)\n", gpio_id);
            ret = -1;
            break;
    }

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_gpio_set_value);

int plat_gpio_get_value(PLAT_GPIO_ID_T gpio_id)
{
    int ret = 0;

    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_X_CAP_RST:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(PLAT_X_CAP_RST_GPIO_PIN);
            break;
        case PLAT_GPIO_ID_59:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(59);
            break;
        default:
            break;
    }

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_gpio_get_value);

void plat_identifier_check(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);  ///< compile check, panic if check fail
}
EXPORT_SYMBOL(plat_identifier_check);

static int __init plat_init(void)
{
    int ret = 0;
    int fd;

    printk("fe_common version %d\n", PLAT_COMMON_VERSION);

    /* register pmu register for register access */
    fd = ftpmu010_register_reg(&plat_pmu_reg_info_8139);
    if(fd < 0) {
        printk("register pmu register failed!!\n");
        ret = -1;
        goto exit;
    }

    plat_pmu_fd = fd;

exit:
    return ret;
}

static void __exit plat_exit(void)
{
    int i;
	
	/* turn off ssp0 and ssp 1 clock */		
    ftpmu010_write_reg(plat_pmu_fd, 0xB8 , 0x3 << 4, 0x3 << 4);
    /* free gpio pin */
    for(i=0; i<PLAT_GPIO_ID_MAX; i++) {
        switch(i) {
            case PLAT_GPIO_ID_X_CAP_RST:
                if(plat_gpio_init[i])
                    gpio_free(PLAT_X_CAP_RST_GPIO_PIN);
                break;
            case PLAT_GPIO_ID_59:
                if(plat_gpio_init[i])
                    gpio_free(59);
                break;
            default:
                break;
        }
    }

    /* deregister pmu register */
    if(plat_pmu_fd >= 0)
        ftpmu010_deregister_reg(plat_pmu_fd);
}

module_init(plat_init);
module_exit(plat_exit);

MODULE_DESCRIPTION("Grain Media GM8139 Front-End Plaftorm Common Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
