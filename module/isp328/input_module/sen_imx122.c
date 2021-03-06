/**
 * @file sen_imx122.c
 * Sony IMX122 sensor driver
 *
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.co
 *
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>

#define PFX "sen_imx122"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"
#include "spi_common.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1920
#define DEFAULT_IMAGE_HEIGHT    1080
#define DEFAULT_XCLK            37125000

static ushort sensor_w = DEFAULT_IMAGE_WIDTH;
module_param(sensor_w, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_w, "sensor image width");

static ushort sensor_h = DEFAULT_IMAGE_HEIGHT;
module_param(sensor_h, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_h, "sensor image height");

static uint sensor_xclk = DEFAULT_XCLK;
module_param(sensor_xclk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_xclk, "sensor external clock frequency");

static uint mirror = 0;
module_param(mirror, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mirror, "sensor mirror function");

static uint flip = 0;
module_param(flip, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flip, "sensor flip function");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

static uint inv_clk = 0;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

static ushort sensor_spi = 1;  //0:I2C, 1: SPI
module_param(sensor_spi, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_spi, "sensor SPI interface");

//=============================================================================
// global
//============================================================================
#define SENSOR_NAME         "IMX122"
#define SENSOR_MAX_WIDTH    1920
#define SENSOR_MAX_HEIGHT   1090
#define ID_IMX122          0x00
#define ID_IMX222          0x40
#define ID_IMX322          0x70

#define H_CYCLE_1080P       2200
#define V_CYCLE_1080P       1125

#define H_CYCLE_720P       825
#define V_CYCLE_720P       750

static sensor_dev_t *g_psensor = NULL;

#define DELAY_CODE      0xFF

typedef struct sensor_info {
    int is_init;
    u32 HMax;
    u32 VMax;
    u32 t_row;          // unit: 1/10 us
    int htp;
    int mirror;
    int flip;
    u32 min_a_gain;
    u32 max_a_gain;
    u32 min_d_gain;
    u32 max_d_gain;
    u32 curr_a_gain;
    u32 curr_d_gain;
    struct semaphore lock;  
} sensor_info_t;

typedef struct sensor_reg {
    u8 id;
    u8 addr;
    u8 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
    {0x02, 0x00, 0x31},  // TESTEN + STANDBY
    {0x02, 0x11, 0x00},  // OPORTSEL
    {0x02, 0x2D, 0x40},  // DCKDLY
    {0x02, 0x02, 0x0F},  // HD1080p mode
    {0x02, 0x14, 0x00},  // WINPH_L
    {0x02, 0x15, 0x00},  // WINPH_H
    {0x02, 0x16, 0x3c},  // WINPV_L
    {0x02, 0x17, 0x00},  // WINPV_H
    {0x02, 0x18, 0xC0},  // WINWH_L
    {0x02, 0x19, 0x07},  // WINWH_H
    {0x02, 0x1A, 0x51},  // WINWV_L
    {0x02, 0x1B, 0x04},  // WINWV_H
    {0x02, 0x9A, 0x26},  // 12B1080P_L
    {0x02, 0x9B, 0x02},  // 12B1080P_H

    {0x02, 0xCE, 0x16},  // PRES
    {0x02, 0xCF, 0x82},  // DRES_L
    {0x02, 0xD0, 0x00},  // DRES_H
    {0x02, 0x01, 0x00},  // VREVERSE

    {0x02, 0x12, 0x82},  // 0:SSBRK, 1: ADRES, 7:fix value
    {0x02, 0x0f, 0x00},  // SVS_L
    {0x02, 0x10, 0x00},  // SVS_H
    {0x02, 0x0d, 0x00},  // SPL_L
    {0x02, 0x0e, 0x00},  // SPL_H
    {0x02, 0x08, 0x00},  // SHS1_L
    {0x02, 0x09, 0x00},  // SHS1_H
    {0x02, 0x1E, 0x00},  // GAIN
    {0x02, 0x20, 0xF0},  // BLKLEVEL_L
    {0x02, 0x21, 0x00},  // 0: BLKLEVEL_H, 4,5:XHSLNG, 7:10BITA
    {0x02, 0x22, 0x41},  // 0~2:XVLNG 6:MUST=1  7:720PMODE
    {0x02, 0x7a, 0x00},
    {0x02, 0x7b, 0x00},
    {0x02, 0x98, 0x26},
    {0x02, 0x99, 0x02},
    {0x02, 0x03, 0x4C},  // HMAX_L
    {0x02, 0x04, 0x04},  // HMAX_H
    {0x02, 0x05, 0x65},  // VMAX_L
    {0x02, 0x06, 0x04},  // VMAX_H
    {0x02, 0x2C, 0x00},  // 0:XMSTA

    {0x02, 0x13, 0x40},  // fix value
    {0x02, 0x1C, 0x50},  // fix value
    {0x02, 0x1F, 0x31},  // fix value
    {0x03, 0x17, 0x0D},  // fix value
    {0x02, 0x27, 0x20},  // fix value 2015/04/15

    {0x02, 0x00, 0x30},  // STANDBY off
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_i2c[] = {
    //1080@30fps-10bit,Parallel CMOS SDR
    //Input 37.125MHz to INCK
    {0x30, 0x00, 0x31},
    {0x01, 0x00, 0x00},
    {0x30, 0x2C, 0x01},
    //Disable i2c Timeout
    {0x30, 0x4E, 0x00},
    {0x00, 0x08, 0x00},
    {0x00, 0x09, 0x3C},
    {0x01, 0x01, 0x00},
    {0x01, 0x04, 0x00},
    {0x01, 0x12, 0x0A},
    {0x01, 0x13, 0x0A},
    {0x02, 0x02, 0x00},
    {0x02, 0x03, 0x00},
    {0x03, 0x40, 0x04},
    {0x03, 0x41, 0x65},
    {0x03, 0x42, 0x04},
    {0x03, 0x43, 0x4C},
    {0x30, 0x01, 0x00},
    {0x30, 0x02, 0x0F},
    {0x30, 0x03, 0x4C},
    {0x30, 0x04, 0x04},
    {0x30, 0x05, 0x65},
    {0x30, 0x06, 0x04},
    {0x30, 0x07, 0x00},
    {0x30, 0x11, 0x00},
    {0x30, 0x12, 0x80},
    {0x30, 0x16, 0x3C},
    {0x30, 0x1F, 0x73},
    {0x30, 0x20, 0x3C},
    {0x30, 0x21, 0x80},
    {0x30, 0x22, 0x00},
    {0x30, 0x27, 0x20},
    {0x30, 0x7A, 0x40},
    {0x30, 0x7B, 0x02},
    {0x30, 0x98, 0x26},
    {0x30, 0x99, 0x02},
    {0x30, 0x9A, 0x4C},
    {0x30, 0x9B, 0x04},
    {0x30, 0xCE, 0x16},
    {0x30, 0xCF, 0x82},
    {0x30, 0xD0, 0x00},
    {0x31, 0x17, 0x0D},
    {DELAY_CODE, 0x00, 100},//wait 100ms
    {0x30, 0x2C, 0x00},
    {DELAY_CODE, 0x00, 100},//wait 100ms
    {0x30, 0x00, 0x30},
    {0x01, 0x00, 0x01},
};
#define NUM_OF_INIT_REGS_I2C (sizeof(sensor_init_regs_i2c) / sizeof(sensor_reg_t))


typedef struct gain_set {
    u8  ad_reg;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table[] = {  
    {0x0,	 64},  {0x1,     66}, {0x2,	    69},  {0x3,     71},  {0x4,     73}, 
    {0x5,	 76},  {0x6,     79}, {0x7,	    82},  {0x8,     84},  {0x9,     87}, 
    {0xA,	 90},  {0xB,     94}, {0xC,	    97},  {0xD,    100},  {0xE,    104},  
    {0xF,	107},  {0x10,	111}, {0x11,   115},  {0x12,   119},  {0x13,   123},  
    {0x14,	128},  {0x15,	132}, {0x16,   137},  {0x17,   142},  {0x18,   147},  
    {0x19,	152},  {0x1A,	157}, {0x1B,   163},  {0x1C,   168},  {0x1D,   174},  
    {0x1E,	180},  {0x1F,	187}, {0x20,   193},  {0x21,   200},  {0x22,   207},  
    {0x23,	214},  {0x24,	222}, {0x25,   230},  {0x26,   238},  {0x27,   246}, 
    {0x28,	255},  {0x29,	264}, {0x2A,   273},  {0x2B,   283},  {0x2C,   293}, 
    {0x2D,	303},  {0x2E,	313}, {0x2F,   324},  {0x30,   336},  {0x31,   348}, 
    {0x32,	360},  {0x33,	373}, {0x34,   386},  {0x35,   399},  {0x36,   413}, 
    {0x37,	428},  {0x38,	443}, {0x39,   458},  {0x3A,   474},  {0x3B,   491}, 
    {0x3C,	508},  {0x3D,	526}, {0x3E,   545},  {0x3F,   564},  {0x40,   584}, 
    {0x41,	604},  {0x42,	625}, {0x43,   647},  {0x44,   670},  {0x45,   694}, 
    {0x46,	718},  {0x47,	743}, {0x48,   769},  {0x49,   796},  {0x4A,   824}, 
    {0x4B,	853},  {0x4C,	883}, {0x4D,   914},  {0x4E,   947},  {0x4F,   980}, 
    {0x50,  1014}, {0x51,  1050}, {0x52,  1087},  {0x53,  1125},  {0x54,  1165}, 
    {0x55,  1206}, {0x56,  1248}, {0x57,  1292},  {0x58,  1337},  {0x59,  1384}, 
    {0x5A,  1433}, {0x5B,  1483}, {0x5C,  1535},  {0x5D,  1589},  {0x5E,  1645}, 
    {0x5F,  1703}, {0x60,  1763}, {0x61,  1825},  {0x62,  1889},  {0x63,  1955}, 
    {0x64,  2024}, {0x65,  2095}, {0x66,  2169},  {0x67,  2245},  {0x68,  2324}, 
    {0x69,  2405}, {0x6A,  2490}, {0x6B,  2577},  {0x6C,  2668},  {0x6D,  2762}, 
    {0x6E,  2859}, {0x6F,  2959}, {0x70,  3063},  {0x71,  3171},  {0x72,  3282}, 
    {0x73,  3398}, {0x74,  3517}, {0x75,  3641},  {0x76,  3769},  {0x77,  3901}, 
    {0x78,  4038}, {0x79,  4180}, {0x7A,  4327},  {0x7B,  4479},  {0x7C,  4636}, 
    {0x7D,  4799}, {0x7E,  4968}, {0x7F,  5143},  {0x80,  5323},  {0x81,  5510}, 
    {0x82,  5704}, {0x83,  5904}, {0x84,  6112},  {0x85,  6327},  {0x86,  6549}, 
    {0x87,  6779}, {0x88,  7017}, {0x89,  7264},  {0x8A,  7519},  {0x8B,  7784}, 
    {0x8C,  8057},           
};

#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//============================================================================
// SPI
//============================================================================
#define SPI_CLK_SET()   isp_api_spi_clk_set_value(1)
#define SPI_CLK_CLR()   isp_api_spi_clk_set_value(0)
#define SPI_DI_READ()   isp_api_spi_di_get_value()
#define SPI_DO_SET()    isp_api_spi_do_set_value(1)
#define SPI_DO_CLR()    isp_api_spi_do_set_value(0)
#define SPI_CS_SET()    isp_api_spi_cs_set_value(1)
#define SPI_CS_CLR()    isp_api_spi_cs_set_value(0)

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            0x34 >> 1
#include "i2c_common.c"

//============================================================================
// internal functions
//============================================================================
static void _delay(void)
{
    int i;
    for (i=0; i<0x1f; i++);
}

static int imx122_read_reg(u8 id, u8 addr, u32 *pval)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int i;
    u32 tmp;
    struct i2c_msg  msgs[2];
    unsigned char   tmp1[2], tmp2[2];
    u32 err_cnt;

    if (sensor_spi){
        down(&pinfo->lock); 
        _delay();
        SPI_CS_CLR();
        _delay();
        // address phase
        tmp = (addr << 8) | (0x80 | id);
        for (i=0; i<16; i++) {
            SPI_CLK_CLR();
            _delay();
            if (tmp & BIT0)
                SPI_DO_SET();
            else
                SPI_DO_CLR();
            tmp >>= 1;
            SPI_CLK_SET();
            _delay();
        }
        // data phase
        *pval = 0;
        for (i=0; i<8; i++) {
            SPI_CLK_CLR();
            _delay();
            SPI_CLK_SET();
            _delay();
            if (SPI_DI_READ() & BIT0)
                *pval |= (1 << i);
        }
        SPI_CS_SET();
        up(&pinfo->lock);

        return 0;
    }
    else{         
        tmp1[0]       = id; 
        tmp1[1]       = addr;
        msgs[0].addr  = I2C_ADDR;
        msgs[0].flags = 0;
        msgs[0].len   = 2;
        msgs[0].buf   = tmp1;

        tmp2[0]       = 0;
        msgs[1].addr  = I2C_ADDR;
        msgs[1].flags = 1;
        msgs[1].len   = 1;
        msgs[1].buf   = tmp2;

        err_cnt = 0;
        while(err_cnt < 5){
            if (sensor_i2c_transfer(msgs, 2) == 0)
                break;
            err_cnt++;
        }
        if (err_cnt == 5)
            return -1;        
        
        *pval = tmp2[0];
        
        return 0;
    }
}

static int imx122_write_reg(u8 id, u8 addr, u32 val)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int i;
    u32 tmp;
    struct i2c_msg  msgs;
    unsigned char   buf[3];
    u32 err_cnt;

    if (sensor_spi){     
        down(&pinfo->lock); 
        _delay();
        SPI_CS_CLR();
        _delay();
        // address phase & data phase
        tmp = (val << 16) | (addr << 8) | id;
        for (i=0; i<24; i++) {
            SPI_CLK_CLR();
            _delay();
            if (tmp & BIT0)
                SPI_DO_SET();
            else
                SPI_DO_CLR();
            tmp >>= 1;
            SPI_CLK_SET();
            _delay();
        }
        SPI_CS_SET();
        up(&pinfo->lock);
        return 0;
    }
    else{        
        buf[0]     = id;
        buf[1]     = addr;
        buf[2]     = val & 0xFF;
        msgs.addr  = I2C_ADDR;
        msgs.flags = 0;
        msgs.len   = 3;
        msgs.buf   = buf;
        
        err_cnt = 0;
        while(err_cnt < 5){
            if (sensor_i2c_transfer(&msgs, 1) == 0)
                break;
            err_cnt++;
        }
        if (err_cnt == 5)
            return -1;
   
        return 0;        
    }
}

static int read_reg(u32 addr, u32 *pval)
{
    u8 id, r_addr;

    id = (addr >> 8) & 0xFF;
    r_addr = addr & 0xFF;
    return imx122_read_reg(id, r_addr, pval);
}

static int write_reg(u32 addr, u32 val)
{
    u8 id, r_addr;

    
    id = (addr >> 8) & 0xFF;
    r_addr = addr & 0xFF;

    
    return imx122_write_reg(id, r_addr, val);

}

static u32 get_pclk(u32 xclk)
{
    u32 pclk = 74250000;
    u32 reg;

    if (sensor_spi)
        imx122_read_reg(0x2, 0x11, &reg);
    else
        imx122_read_reg(0x30, 0x11, &reg);
    reg = reg & 0x03;

    if(reg==0)
        pclk = xclk << 1;
    else if(reg==1)
        pclk = xclk;
    else if(reg==1)
        pclk = xclk >> 1;
    else
        pclk = xclk >> 2;

    isp_info("** xclk(%d) pclk(%d)\n",xclk, pclk);
    return pclk;
}

static void adjust_vblk(int fps)
{    
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, max_fps, min_fps;                                 

    if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720){
        min_fps = 4;
        max_fps = 60;
    }
    else{
        min_fps = 3;
        max_fps = 30;
    }
    if (fps < min_fps)
        fps = min_fps;
    if (fps > max_fps)
        fps = max_fps;
    
    if (sensor_spi)
        imx122_write_reg(0x2, 0x00, 0x31);// STANDBY on  
    else
        imx122_write_reg(0x01, 0x00, 0x00);// STANDBY on  
    _delay(); 
    pinfo->htp = g_psensor->pclk / 2 / fps / pinfo->VMax;           
    //printk("htp = %d, pdev->pclk= %d, fps=%d, pinfo->VMax=%d\n", pinfo->htp, pdev->pclk, fps, pinfo->VMax);
    pinfo->HMax = pinfo->htp;
    reg_h = (pinfo->htp >> 8) & 0xFF;
    reg_l = pinfo->htp & 0xFF;  
    if (sensor_spi){
        imx122_write_reg(0x2, 0x03, reg_l);// HMAX_L
        imx122_write_reg(0x2, 0x04, reg_h);// HMAX_H                                       
    }
    else{
        imx122_write_reg(0x03, 0x43, reg_l);// HMAX_L
        imx122_write_reg(0x03, 0x42, reg_h);// HMAX_H                                       
    }
    //imx122_write_reg(0x2, 0x11, 0x00); //FRSEL  
    //imx122_write_reg(0x2, 0x9A, 0x26);
    //imx122_write_reg(0x2, 0x9B, 0x02); 
    _delay();               
    if (sensor_spi)
        imx122_write_reg(0x2, 0x00, 0x30); // STANDBY off           
    else
        imx122_write_reg(0x01, 0x00, 0x01);// STANDBY off  
    pinfo->t_row = (pinfo->htp * 20000) / (g_psensor->pclk / 1000);
    isp_info("t_row=%d\n", pinfo->t_row);
    
    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;

    g_psensor->fps = fps;
    isp_info("adjust_vblk, fps=%d, HMax=0x%x\n", g_psensor->fps, pinfo->HMax);
    //printk("t_row = %d, g_psensor->min_exp =%d, pclk=%d\n", pinfo->t_row, g_psensor->min_exp, pdev->pclk);  
     
}

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 val_l, val_h;

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT))
        return -EINVAL;

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    
    if (width <= 1280 && height <= 720){
        if (sensor_spi){
            imx122_write_reg(0x02, 0x04, 0x03);
            imx122_write_reg(0x02, 0x03, 0x39);
            imx122_write_reg(0x02, 0x06, 0x02);
            imx122_write_reg(0x02, 0x05, 0xee);

            imx122_write_reg(0x02, 0x02, 0x01);
            imx122_write_reg(0x02, 0x11, 0x00);
            imx122_write_reg(0x02, 0x12, 0x80); //SPI only
            imx122_write_reg(0x02, 0x16, 0xf1);
            imx122_write_reg(0x02, 0x20, 0x3c); //SPI only
            imx122_write_reg(0x02, 0x21, 0x00);
            imx122_write_reg(0x02, 0x22, 0x80);
            imx122_write_reg(0x02, 0x7a, 0x00);
            imx122_write_reg(0x02, 0x7b, 0x00);
            imx122_write_reg(0x02, 0xce, 0x00);
            imx122_write_reg(0x02, 0xcf, 0x00);
        }
        else{
            imx122_write_reg(0x03, 0x40, 0x02);
            imx122_write_reg(0x03, 0x41, 0xee);
            imx122_write_reg(0x03, 0x42, 0x03);
            imx122_write_reg(0x03, 0x43, 0x39);

            imx122_write_reg(0x30, 0x02, 0x01);
            imx122_write_reg(0x30, 0x11, 0x00);
            imx122_write_reg(0x30, 0x16, 0xf0);
            imx122_write_reg(0x30, 0x21, 0x00);
            imx122_write_reg(0x30, 0x22, 0x80);
            imx122_write_reg(0x30, 0x7a, 0x00);
            imx122_write_reg(0x30, 0x7b, 0x00);
            imx122_write_reg(0x30, 0xce, 0x00);
            imx122_write_reg(0x30, 0xcf, 0x00);
        }
        g_psensor->out_win.width = H_CYCLE_720P*2;
        g_psensor->out_win.height = V_CYCLE_720P;
        g_psensor->img_win.x = (142 + ((1280 - width) >> 1)) &~BIT0;
        g_psensor->img_win.y = (24 + ((720 - height) >> 1)) &~BIT0;
    }
    else{        
        if (sensor_spi){
            imx122_write_reg(0x02, 0x04, 0x04);
            imx122_write_reg(0x02, 0x03, 0x4c);
            imx122_write_reg(0x02, 0x06, 0x04);
            imx122_write_reg(0x02, 0x05, 0x65);

            imx122_write_reg(0x02, 0x02, 0x0f);
            imx122_write_reg(0x02, 0x11, 0x00);
            imx122_write_reg(0x02, 0x12, 0x82);
            imx122_write_reg(0x02, 0x16, 0x3c);
            imx122_write_reg(0x02, 0x20, 0xf0);
            imx122_write_reg(0x02, 0x21, 0x00);
            imx122_write_reg(0x02, 0x22, 0x41);
            imx122_write_reg(0x02, 0x7a, 0x00);
            imx122_write_reg(0x02, 0x7b, 0x00);
            imx122_write_reg(0x02, 0xce, 0x16);
            imx122_write_reg(0x02, 0xcf, 0x82);
        }
        else{
            imx122_write_reg(0x03, 0x40, 0x04);
            imx122_write_reg(0x03, 0x41, 0x65);
            imx122_write_reg(0x03, 0x42, 0x04);
            imx122_write_reg(0x03, 0x43, 0x4c);

            imx122_write_reg(0x30, 0x02, 0x0f);
            imx122_write_reg(0x30, 0x11, 0x00);
            imx122_write_reg(0x30, 0x16, 0x3c);
            imx122_write_reg(0x30, 0x21, 0x80);
            imx122_write_reg(0x30, 0x22, 0x00);
            imx122_write_reg(0x30, 0x7a, 0x40);
            imx122_write_reg(0x30, 0x7b, 0x02);
            imx122_write_reg(0x30, 0xce, 0x16);
            imx122_write_reg(0x30, 0xcf, 0x82);
        }
        g_psensor->out_win.width = H_CYCLE_1080P;
        g_psensor->out_win.height = V_CYCLE_1080P;
        g_psensor->img_win.x = (142 + ((SENSOR_MAX_WIDTH - width) >> 1)) &~BIT0;
        g_psensor->img_win.y = (24 + ((SENSOR_MAX_HEIGHT - height) >> 1)) &~BIT0;
    }
    

    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    if (sensor_spi){
        imx122_read_reg(0x2, 0x03, &val_l);
        imx122_read_reg(0x2, 0x04, &val_h);
        pinfo->HMax = (val_h << 8) | val_l;
        imx122_read_reg(0x2, 0x05, &val_l);
        imx122_read_reg(0x2, 0x06, &val_h);
    }
    else{
        imx122_read_reg(0x03, 0x43, &val_l);
        imx122_read_reg(0x03, 0x42, &val_h);
        pinfo->HMax = (val_h << 8) | val_l;
        imx122_read_reg(0x03, 0x41, &val_l);
        imx122_read_reg(0x03, 0x40, &val_h);
    }

    pinfo->VMax = (val_h << 8) | val_l;

    g_psensor->pclk = get_pclk(g_psensor->xclk);

    adjust_vblk(g_psensor->fps);

    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;
    
    //printk("pinfo->VMax = %d, pinfo->HMax = %d, pinfo->t_row = %d, g_psensor->pclk = %d\n", pinfo->HMax, pinfo->VMax, pinfo->t_row, g_psensor->pclk);
    
    //printk("t_row = %d, min_exp =%d\n", pinfo->t_row, g_psensor->min_exp);  

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;	
    u32 val_l, val_h;
    u32 lines, vmax, shs1;

    if (!g_psensor->curr_exp) {
        if (sensor_spi){
            imx122_read_reg(0x2, 0x05, &val_l);
            imx122_read_reg(0x2, 0x06, &val_h);
            vmax = (val_h << 8) | val_l;
            imx122_read_reg(0x2, 0x08, &val_l);
            imx122_read_reg(0x2, 0x09, &val_h);
        }
        else{
            imx122_read_reg(0x03, 0x41, &val_l);
            imx122_read_reg(0x03, 0x40, &val_h);
            vmax = (val_h << 8) | val_l;
            imx122_read_reg(0x02, 0x03, &val_l);
            imx122_read_reg(0x02, 0x02, &val_h);
        }
        shs1 = (val_h << 8) | val_l;

        lines = vmax - shs1;
        g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, shs1;
    int HMax;
        
    int org_t_row;

    org_t_row = (pinfo->HMax * 20000) / (g_psensor->pclk / 1000);         
    lines = ((exp * 1000) + org_t_row/2) / org_t_row;                          

    if (lines == 0)
        lines = 1;      

    //printk("exp=%d, lines=%d\n", exp, lines);

    if (lines > pinfo->VMax) {
        if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720){
            org_t_row = (pinfo->HMax * 20000) / (g_psensor->pclk / 1000);         
            lines = ((exp * 1000) + org_t_row/2) / org_t_row;                          

            if (lines >= 0x2ee0)
                HMax = 0x19c8;
            else if (lines >= 0x1770)
                HMax = 0xce4;
            else if (lines >= 0xbb8)
                HMax = 0x672;
            else
                HMax = 0x339;
        }
        else{
            org_t_row = (pinfo->HMax * 20000) / (g_psensor->pclk / 1000);         
            lines = ((exp * 1000) + org_t_row/2) / org_t_row;                          

            if(lines >= 16723)                                        
                HMax = 0x2260;
            else if(lines >= 5500)                                        
                HMax = 0x1130;
            else if(lines >= 4123)
                HMax = 0x898;
            else if(lines >= 1680)
                HMax = 0x528;
            else
                HMax = 0x44c;
        }

        //isp_info("set_exp HMax=0x%x, pHMax=0x%x\n", HMax, pinfo->HMax);
        if (HMax < pinfo->HMax)
            HMax = pinfo->HMax;
        
        if (sensor_spi){
            imx122_write_reg(0x2, 0x03, (HMax & 0xFF));
            imx122_write_reg(0x2, 0x04, ((HMax >> 8) & 0xFF));
            pinfo->t_row = (HMax * 20000) / (g_psensor->pclk / 1000); 
            lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row; 
    	    imx122_write_reg(0x2, 0x05, (lines & 0xFF));
    	    imx122_write_reg(0x2, 0x06, ((lines >> 8) & 0xFF));
            imx122_write_reg(0x2, 0x08, 0);
            imx122_write_reg(0x2, 0x09, 0);
        }
        else{
            imx122_write_reg(0x03, 0x43, (HMax & 0xFF));
            imx122_write_reg(0x03, 0x42, ((HMax >> 8) & 0xFF));
            pinfo->t_row = (HMax * 20000) / (g_psensor->pclk / 1000); 
            lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row; 
    	    imx122_write_reg(0x03, 0x41, (lines & 0xFF));
    	    imx122_write_reg(0x03, 0x40, ((lines >> 8) & 0xFF));
            imx122_write_reg(0x02, 0x03, 0);
            imx122_write_reg(0x02, 0x02, 0);
        }
        //printk("long HMax=%d, pinfo->t_row=%d, lines=%d\n", HMax, pinfo->t_row, lines);
    } else {       
        HMax = pinfo->HMax;

        if (sensor_spi){
            imx122_write_reg(0x2, 0x03, (HMax & 0xFF));
            imx122_write_reg(0x2, 0x04, ((HMax >> 8) & 0xFF));
            pinfo->t_row = (HMax * 20000) / (g_psensor->pclk / 1000); 
            lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row; 
    	    imx122_write_reg(0x2, 0x05, (pinfo->VMax & 0xFF));
    	    imx122_write_reg(0x2, 0x06, ((pinfo->VMax >> 8) & 0xFF));
            shs1 = pinfo->VMax - lines;
            imx122_write_reg(0x2, 0x08, (shs1 & 0xFF));
            imx122_write_reg(0x2, 0x09, ((shs1 >> 8) & 0xFF));
        }
        else{
            imx122_write_reg(0x03, 0x43, (HMax & 0xFF));
            imx122_write_reg(0x03, 0x42, ((HMax >> 8) & 0xFF));
            pinfo->t_row = (HMax * 20000) / (g_psensor->pclk / 1000); 
            lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row; 
    	    imx122_write_reg(0x03, 0x41, (pinfo->VMax & 0xFF));
    	    imx122_write_reg(0x03, 0x40, ((pinfo->VMax >> 8) & 0xFF));
            shs1 = pinfo->VMax - lines;
            imx122_write_reg(0x02, 0x03, (shs1 & 0xFF));
            imx122_write_reg(0x02, 0x02, ((shs1 >> 8) & 0xFF));        
        }
        
        //printk("Normal HMax=%d, pinfo->t_row=%d, lines=%d\n", HMax, pinfo->t_row, lines);    
    }

    //printk("lines=%d ,pinfo->VMax=%d\n", lines, pinfo->VMax);  

    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000; 
    
    //printk("g_psensor->curr_exp=%d\n", g_psensor->curr_exp);

    return ret;
}

#define MAX_GAIN_IDX  0x8C
static u32 get_gain(void)
{
    u32 ad_gain;

    if (g_psensor->curr_gain == 0) {        

        if (sensor_spi)
            imx122_read_reg(0x2, 0x1E, &ad_gain);
        else
            imx122_read_reg(0x30, 0x1E, &ad_gain);
            
        if (ad_gain > MAX_GAIN_IDX)
            ad_gain = MAX_GAIN_IDX;    
        g_psensor->curr_gain = gain_table[ad_gain].gain_x;       
    }

    return g_psensor->curr_gain;
}

static u32 get_a_gain(void);
static u32 get_d_gain(void);

static int set_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0, i;
    u32 ad_gain;

    // search most suitable gain into gain table
    ad_gain = gain_table[0].ad_reg;

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0)
        ad_gain = gain_table[i-1].ad_reg;
    if (sensor_spi)
        imx122_write_reg(0x2, 0x1E, ad_gain);
    else
        imx122_write_reg(0x30, 0x1E, ad_gain);
    g_psensor->curr_gain = gain_table[i-1].gain_x;

    pinfo->curr_a_gain = 0;
    get_a_gain();
    pinfo->curr_d_gain = 0;
    get_d_gain();

    return ret;
}

#define MAX_A_GAIN_IDX  0x50
#define MAX_D_GAIN_IDX  0x8C
static u32 get_a_gain(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 a_gain;

    if (pinfo->curr_a_gain == 0) {        

        if (sensor_spi)
            imx122_read_reg(0x2, 0x1E, &a_gain);
        else
            imx122_read_reg(0x30, 0x1E, &a_gain);
        if (a_gain > MAX_A_GAIN_IDX)
            a_gain = MAX_A_GAIN_IDX;
        pinfo->curr_a_gain = gain_table[a_gain].gain_x;       
    }

    return pinfo->curr_a_gain;

}

static int set_a_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int i;
    u32 reg;

    if (pinfo->curr_d_gain > 64)
        return pinfo->curr_a_gain;
    
    // search most suitable gain into gain table
    reg = gain_table[0].ad_reg;

    for (i=0; i<MAX_A_GAIN_IDX+1; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0)
        reg = gain_table[i-1].ad_reg;
    if (sensor_spi)
        imx122_write_reg(0x2, 0x1E, reg);
    else
        imx122_write_reg(0x30, 0x1E, reg);
    pinfo->curr_a_gain = gain_table[i-1].gain_x;
  
    return pinfo->curr_a_gain;
}

static u32 get_d_gain(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 d_gain;

    if (pinfo->curr_d_gain == 0) {        

        if (sensor_spi)
            imx122_read_reg(0x2, 0x1E, &d_gain);
        else
            imx122_read_reg(0x30, 0x1E, &d_gain);
        if (d_gain <= MAX_A_GAIN_IDX)
            pinfo->curr_d_gain = 64;
        else{
            if (d_gain > MAX_D_GAIN_IDX)
                d_gain = MAX_D_GAIN_IDX;
            pinfo->curr_d_gain = (gain_table[d_gain].gain_x * 64 + gain_table[MAX_A_GAIN_IDX].gain_x / 2)
                    / gain_table[MAX_A_GAIN_IDX].gain_x;       
        }
    }
        
    return pinfo->curr_d_gain;
}

static int set_d_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int i;
    u32 d_gain, pre_d_gain, reg;

    if (pinfo->curr_a_gain < gain_table[MAX_A_GAIN_IDX].gain_x)
        return pinfo->curr_d_gain;
    
    // search most suitable gain into gain table
    reg = gain_table[MAX_A_GAIN_IDX].ad_reg;

    for (i=MAX_A_GAIN_IDX; i<MAX_D_GAIN_IDX+1; i++) {
        d_gain = (gain_table[i].gain_x * 64 + gain_table[MAX_A_GAIN_IDX].gain_x / 2)
                    / gain_table[MAX_A_GAIN_IDX].gain_x;   
        if (d_gain > gain)
            break;
        pre_d_gain = d_gain;
    }
    if (i > MAX_A_GAIN_IDX)
        reg = gain_table[i-1].ad_reg;
    if (sensor_spi)
        imx122_write_reg(0x2, 0x1E, reg);
    else
        imx122_write_reg(0x30, 0x1E, reg);
    pinfo->curr_d_gain = pre_d_gain; 
  
    return pinfo->curr_d_gain;
}

static int get_mirror(void)
{
    u32 reg;

    if (sensor_spi){
        imx122_read_reg(0x2, 0x01, &reg);
        return (reg & BIT1) ? 1 : 0;
    }
    else{
        imx122_read_reg(0x01, 0x01, &reg);
        return (reg & BIT0) ? 1 : 0;
    }
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;   

    if (sensor_spi){
        imx122_read_reg(0x2, 0x01, &reg);
        if (enable)
            reg |= BIT1;
        else
            reg &=~BIT1;
        imx122_write_reg(0x2, 0x01, reg);
    }
    else{
        imx122_read_reg(0x01, 0x01, &reg);
        if (enable)
            reg |= BIT0;
        else
            reg &=~BIT0;
        imx122_write_reg(0x01, 0x01, reg);
    }        
    
    pinfo->mirror = enable;
   
    return 0;
}

static int get_flip(void)
{
    u32 reg;

    if (sensor_spi){
        imx122_read_reg(0x2, 0x01, &reg);
        return (reg & BIT0) ? 1 : 0;
    }
    else{
        imx122_read_reg(0x01, 0x01, &reg);
        return (reg & BIT1) ? 1 : 0;
    }
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;
    
    if (sensor_spi){
        imx122_read_reg(0x2, 0x01, &reg);
        if (enable)
            reg |= BIT0;
        else
            reg &=~BIT0;
        imx122_write_reg(0x2, 0x01, reg);
    }
    else{
        imx122_read_reg(0x01, 0x01, &reg);
        if (enable)
            reg |= BIT1;
        else
            reg &=~BIT1;
        imx122_write_reg(0x01, 0x01, reg);
    }        
    pinfo->flip = enable;        

    return 0;
}

static int get_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;

    switch (cmd) {
    case ID_MIRROR:
        *(int*)arg = get_mirror();
        break;
    case ID_FLIP:
        *(int*)arg = get_flip();
        break;
    case ID_FPS:
        *(int*)arg = g_psensor->fps;
        break;    
    case ID_A_GAIN:
        *(int*)arg = get_a_gain();
        break;
    case ID_MIN_A_GAIN:
        *(int*)arg = pinfo->min_a_gain;
        break;
    case ID_MAX_A_GAIN:
        *(int*)arg = pinfo->max_a_gain;
        break;
    case ID_D_GAIN:
        *(int*)arg = get_d_gain();
        break;
    case ID_MIN_D_GAIN:
        *(int*)arg = pinfo->min_d_gain;
        break;
    case ID_MAX_D_GAIN:
        *(int*)arg = pinfo->max_d_gain;
        break;
    default:
        ret = -EFAULT;
        break;
    }

    return ret;
}

static int set_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
    case ID_MIRROR:
        set_mirror((int)arg);
        break;
    case ID_FLIP:
        set_flip((int)arg);
        break;
    case ID_FPS:
        adjust_vblk((int)arg);
        break;    
    case ID_A_GAIN:
        ret = set_a_gain((int)arg);
        break;
    case ID_D_GAIN:
        ret = set_d_gain((int)arg);
        break;
    default:
        ret = -EFAULT;
        break;
    }

    return ret;
}

static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    int i;
    u32 reg;

    if (pinfo->is_init)
        return 0;

    //Check Chip ID
    if (sensor_spi)
        imx122_read_reg(0x03, 0xE8, &reg);
    else
        imx122_read_reg(0x31, 0xE8, &reg);
    
    if(reg == ID_IMX122){
        printk("[IMX122]\n");
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "IMX122");
    }else if(reg == ID_IMX222){
        printk("[IMX222]\n");
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "IMX222");
    }else if(reg == ID_IMX322){
        printk("[IMX322]\n");
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "IMX322");
    }else{
        printk("[Non_Know]\n");
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "Non_Know");
    }


    if (sensor_spi){
        for (i=0; i<NUM_OF_INIT_REGS; i++) {
            if (imx122_write_reg(sensor_init_regs[i].id, sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);            
                return -EINVAL;
            }
        }
    }
    else{
        for (i=0; i<NUM_OF_INIT_REGS_I2C; i++) {
            if (sensor_init_regs_i2c[i].id == DELAY_CODE){
                mdelay(sensor_init_regs_i2c[i].val);
            }
            else if (imx122_write_reg(sensor_init_regs_i2c[i].id, sensor_init_regs_i2c[i].addr, sensor_init_regs_i2c[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);            
                return -EINVAL;
            }
        }
    }

    // get mirror and flip status
    pinfo->mirror = get_mirror();
    pinfo->flip = get_flip();

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);    

    return ret;
}

//============================================================================
// external functions
//=============================================================================
static void imx122_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    if (sensor_spi)
        isp_api_spi_exit();
    else
        sensor_remove_i2c_driver();

    //sensor_remove_i2c_driver();
    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int imx122_construct(u32 xclk, u16 width, u16 height)
{
    sensor_info_t *pinfo = NULL;
    int ret = 0;

    // initial CAP_RST
    ret = isp_api_cap_rst_init();
    if (ret < 0)
        return ret;

    // clear CAP_RST
    isp_api_cap_rst_set_value(0);

    // set EXT_CLK frequency and turn on it.
    ret = isp_api_extclk_set_freq(xclk);
    if (ret < 0)
        return ret;
    ret = isp_api_extclk_onoff(1);
    if (ret < 0)
        return ret;
    mdelay(50);

    // set CAP_RST
    isp_api_cap_rst_set_value(1);

    // allocate ae private data
    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!");
        return -ENOMEM;
    }
    g_psensor->private = pinfo;
    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->interface = IF_PARALLEL;
    g_psensor->num_of_lane = 0;
    g_psensor->protocol = 0;
    g_psensor->fmt = RAW12;
    if (sensor_spi)
        g_psensor->bayer_type = BAYER_GB;
    else
        g_psensor->bayer_type = BAYER_RG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 142;
    g_psensor->img_win.y = 24;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = 64;
    g_psensor->max_gain = 8057;
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 1;
    g_psensor->fps = fps;	
    g_psensor->fn_init = init;
    g_psensor->fn_read_reg = read_reg;
    g_psensor->fn_write_reg = write_reg;    
    g_psensor->fn_set_size = set_size;
    g_psensor->fn_get_exp = get_exp;
    g_psensor->fn_set_exp = set_exp;
    g_psensor->fn_get_gain = get_gain;
    g_psensor->fn_set_gain = set_gain;
    g_psensor->fn_get_property = get_property;
    g_psensor->fn_set_property = set_property;
    pinfo->min_a_gain = gain_table[0].gain_x;
    pinfo->max_a_gain = gain_table[MAX_A_GAIN_IDX].gain_x;
    pinfo->curr_a_gain = 0;
    pinfo->min_d_gain = 64;
    pinfo->max_d_gain = 7784 * 64 / 1014;
    pinfo->curr_d_gain = 0;

    sema_init(&pinfo->lock, 1);

    if (sensor_spi){
        isp_info("sensor SPI\n");
        if ((ret = isp_api_spi_init()))
            return ret;
    }
    else{
        isp_info("sensor I2C\n");
        if ((ret = sensor_init_i2c_driver()) < 0)
            goto construct_err;
    }

    if ((ret = init()) < 0)
        goto construct_err;       

    return 0;

construct_err:
    imx122_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init imx122_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        isp_err("input module version(%x) is not equal to fisp328.ko(%x)!",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        ret = -EINVAL;
        goto init_err1;
    }

    g_psensor = kzalloc(sizeof(sensor_dev_t), GFP_KERNEL);
    if (!g_psensor) {
        ret = -ENODEV;
        goto init_err1;
    }

    if ((ret = imx122_construct(sensor_xclk, sensor_w, sensor_h)) < 0){
        printk("imx122_construct Error \n");
        goto init_err2;
        }
    // register sensor device to ISP driver        
    if ((ret = isp_register_sensor(g_psensor)) < 0){
        printk("isp_register_sensor Error \n");
        goto init_err2;
     } 
   
    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit imx122_exit(void)
{
    isp_unregister_sensor(g_psensor);
    imx122_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(imx122_init);
module_exit(imx122_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor IMX122");
MODULE_LICENSE("GPL");
