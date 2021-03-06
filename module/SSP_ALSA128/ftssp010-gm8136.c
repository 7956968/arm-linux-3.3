/**
 * @file ftssp010-gm8136.c
 * Grain Media's SSP PMU settings of GM8136 series SOC
 *
 *
 */
#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <mach/platform/pmu.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include <mach/ftdmac020.h>
#include <linux/synclink.h>
#include "ftssp010-ssp.h"
#include "debug.h"
#include <linux/dmaengine.h>
#define CLK_SRC_12000K  1
/*
  this file is copy from ftssp010-gm8287.c 
  retain this definition for further usage in 8136
*/
#define CLK_SRC_12288K  0
/*
  8136 does not have SSP2(only SSP0 and SSP1), 
  this file is copy from ftssp010-gm8287.c 
  retain this definition for further usage in 8136
*/
#define SSP2_CLK_SRC_24000K  0

#define OSCH_CLK (24583333)  /*must refer to the ssp main clock in real PMU seeting*/

static unsigned int Main_CLK = OSCH_CLK;
static int pmu_fd = -1, pmu_fd_cnt = 0;
#define DMA_UNKNOWN_CH  0x7F

#define GATE_CLK	     0xB8                                                                                                                                     
#define GATE_CLK_BIT	 (0x3 << 4)  

/* Local function declaration */
#define FTSSP010_SETUP_PLATFORM_RESOURCE(_x)    \
static struct resource ftssp010_##_x##_resource[] = {\
        {\
                .start  = SSP_FTSSP010_##_x##_PA_BASE,\
                .end    = SSP_FTSSP010_##_x##_PA_LIMIT,\
                .flags  = IORESOURCE_MEM\
        },\
        {\
                .start  = SSP_FTSSP010_##_x##_IRQ,\
                .end    = SSP_FTSSP010_##_x##_IRQ,\
                .flags  = IORESOURCE_IRQ,\
        },\
}

static void ftssp010_release_device(struct device *dev) {}

#define FTSSP0101_SETUP_PLATFORM_DEVICE(_x)\
static struct platform_device ftssp010_##_x##_device = {\
        .dev            = {.release = ftssp010_release_device},\
        .name           = "ftssp010",\
        .id             = _x,\
        .num_resources  = ARRAY_SIZE(ftssp010_##_x##_resource),\
        .resource       = ftssp010_##_x##_resource\
}

/* define the resources of SSP0-1 */
FTSSP010_SETUP_PLATFORM_RESOURCE(0);
FTSSP010_SETUP_PLATFORM_RESOURCE(1);


/* define the platform device of SSP0-1 */
FTSSP0101_SETUP_PLATFORM_DEVICE(0);
FTSSP0101_SETUP_PLATFORM_DEVICE(1);


static pmuReg_t reg_array_ssp[] = {
	/* reg_off,  bit_masks,   lock_bits,     init_val,   init_mask */
    //{0x28,  0x00 << 14,   0,    0x00 << 14,   0x00 << 14},  //SSP0-1 select 12M as the source
	{0x28,  0x07047806,   0,      (0x1 << 11) | (0x1 << 13) | (0x0 << 24),   	(0x3 << 11) | (0x3 << 13) | (0x7 << 24)},  //SSP0-1 select PLL1 as the source
//	{0x40,	0x00f00000,	  0,	0x00100000,	0x00300000},
//	{0x44,	0x10000000,	  0,	0,	0},    
	{0x74,	 (0x3f << 16) | (0x3f << 8) | (0x3f),	  0,	(0x27 << 16) | (0x0a << 8) | (0x23),	 (0x3f << 16) | (0x3f << 8) | (0x3f)},
//	{0x7c,	0x1 << 29,	0,	0,	0x1 << 29},
	{0xb8,  0x3 << 4,     0,      0,    0x3 << 4}         //SSP0-2 pclk & sclk
    //{0x50,  0x1 << 31,    0  ,     0x1 << 31,           0x1 << 31},         // use GPIO4/5/6/7--> set 0x50 bit31=1
    //{0x5c,  0xf << 12,    0  ,     0xf << 12,           0xf << 12},         // close GPIO25/26/27/28 --> set 0x5c bit12~15=1111
    //{0x60,  ((0x3 << 12) | (0x3FF << 17) | (0x1 << 16)),   (0x3 << 12) | (0x3FF << 17) | (0x1 << 16),  0x0,  (0x3 << 12) | (0x3FF << 17) | (0x1 << 16)},
    //{0x44,  0x3 << 9,   0x0,     0x2 << 9,    0x3 << 9},  //i2s driving capacity
    //{0x50,  0x7  ,        0  ,  0x2 ,         0x7},         // use GPIO37/38/39/40--> set 0x50 bit0~2=2
    //{0x5c,  0xf << 12,    0  ,     0xf << 12,           0xf << 12},         // close GPIO25/26/27/28 --> set 0x5c bit12~15=1111

};

static pmuRegInfo_t ssp_clk_info = {
    "ssp_clk",
    ARRAY_SIZE(reg_array_ssp),
    ATTR_TYPE_NONE,
    reg_array_ssp,
};

typedef struct pvalue {
    int bit17_21;
    int bit22_24;
} pvalue_t;

typedef struct audio_pll_mclk_map {
    unsigned long pll1;
    unsigned long mclk;
} audio_pll_mclk_map_t;

static audio_pll_mclk_map_t PLL1_MCLK_Map[] = {
    { 860000000, 24571428 },
    { 590000000, 24583333 },
    { 762000000, 24580645 },
    { 885000000, 24583333 },
    { 712500000, 24568965 },
    { 442500000, 24583333 }
};

int ssp_get_pvalue(pvalue_t *pvalue, int div)
{
    int i = 0;
    int flag = 0;

    for (i = 1; i <= 7; i++) {
        if (div % i == 0) {
            pvalue->bit17_21 = div / i;
            if (pvalue->bit17_21 > 32)
                continue;
            else {
                pvalue->bit22_24 = i;
                flag = 1;
                break;
            }
        }
    }

    return flag;
}

/*  SSP2 playback to HDMI, sample rate = 48k, sample size = 16 bits, bit clock = 3072000, SSP2 MAIN CLOCK must = 24MHz
 *  pmu 0x60 [13:12] : select 12.288MHz source for analog die and clk_12288
 *  [12] : 1 - clk_12288 = 12.288MHz, 0 - clk_12288 = ssp6144_clk
 *  [13] : 1 - analog die clock = 12MHz, 0 - analog die clock = clk_12288
 *   Note : ssp6144_clk = ((pll4out1, pll4out2 or pll4out3) / ([24:22]+1)/([21:17]+1))
 *             clk_12288 is SSP and analog die clock's candidate
 *  [16] : gating clock control, 1-gating, 0-un-gating
 *  [21:17] : ssp6144_clk_cnt5_pvalue
 *  [24:22] : ssp6144_clk_cnt3_pvalue
 *  [26:25] : ssp6144_clk_sel, 00-PLL4 CKOUT1, 01-PLL4 CKOUT2, 1x-PLL4 CKOUT3
 *
 *  SSP2 choose clk_12288 as main clock, clk_12288 form ssp6144_clk,
 */
void pmu_cal_clk(ftssp010_card_t *card)
{
    unsigned int pll4_out1, div;
    unsigned int pll4_out2, pll4_out3;
    unsigned int ssp3_clk = 24000000;
    pvalue_t pvalue;
    int pll4_clkout1 = 0, pll4_clkout2 = 0, pll4_clkout3 = 0;
    int bit25_26 = 0, flag = 0;
    int pmu_fd = card->pmu_fd;

    memset(&pvalue, 0x0, sizeof(pvalue_t));

    pll4_out1 = ftpmu010_get_attr(ATTR_TYPE_PLL4);   //hz
    pll4_out2 = pll4_out1 * 2 / 3;   // pll4_out1 / 1.5
    pll4_out3 = pll4_out1 * 2 / 5;   // pll4_out1 / 2.5

    div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL4, ssp3_clk, 1);

    flag = ssp_get_pvalue(&pvalue, div);
    if (flag == 1)
        pll4_clkout1 = 1;

    if (flag == 0) {
        div = pll4_out2 / Main_CLK;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout2 = 1;
    }

    if (flag == 0) {
        div = pll4_out3 / Main_CLK;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout3 = 1;
    }

    if (pll4_clkout1)
        bit25_26 = 0;
    if (pll4_clkout2)
        bit25_26 = 1;
    if (pll4_clkout3)
        bit25_26 = 2;

    if (ftpmu010_write_reg(pmu_fd, 0x60, ((0x2 << 12) | (0x0 << 16) | ((pvalue.bit17_21 - 1) << 17) | ((pvalue.bit22_24 - 1) << 22) | (bit25_26 << 25)), ((0x3 << 12) | (0x3FF << 17) | (0x1 << 16))))
        panic("%s, register ssp2 pvalue fail\n", __func__);
    // SSP2 clk_sel = clk_12288
    if (ftpmu010_write_reg(pmu_fd, 0x28, 0x0 << 20, 0x3 << 20))
        panic("%s, register ssp2 clock on fail\n", __func__);
}

void pmu_cal_clk1(ftssp010_card_t *card)
{
    unsigned int pll4_out1, div;
    unsigned int pll4_out2, pll4_out3;
    unsigned int ssp0_clk = 24000000;
    pvalue_t pvalue;
    int pll4_clkout1 = 0, pll4_clkout2 = 0, pll4_clkout3 = 0;
    int bit25_26 = 0, flag = 0;
    int pmu_fd = card->pmu_fd;

    memset(&pvalue, 0x0, sizeof(pvalue_t));

    pll4_out1 = ftpmu010_get_attr(ATTR_TYPE_PLL4);   //hz
    pll4_out2 = pll4_out1 * 2 / 3;   // pll4_out1 / 1.5
    pll4_out3 = pll4_out1 * 2 / 5;   // pll4_out1 / 2.5

    div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL4, ssp0_clk, 1);
printk("%d %d\n", pll4_out1, div);
    flag = ssp_get_pvalue(&pvalue, div);
    if (flag == 1)
        pll4_clkout1 = 1;

    if (flag == 0) {
        div = pll4_out2 / Main_CLK;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout2 = 1;
    }

    if (flag == 0) {
        div = pll4_out3 / Main_CLK;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout3 = 1;
    }

    if (pll4_clkout1)
        bit25_26 = 0;
    if (pll4_clkout2)
        bit25_26 = 1;
    if (pll4_clkout3)
        bit25_26 = 2;
printk("111 %d %d %d\n", bit25_26, pvalue.bit17_21, pvalue.bit22_24);
    if (ftpmu010_write_reg(pmu_fd, 0x60, ((0x2 << 12) | (0x0 << 16) | ((pvalue.bit17_21 - 1) << 17) | ((pvalue.bit22_24 - 1) << 22) | (bit25_26 << 25)), ((0x3 << 12) | (0x3FF << 17) | (0x1 << 16))))
        panic("%s, register ssp3 pvalue fail\n", __func__);
    // SSP3 clk_sel = clk_12288
printk("222\n");
    if (ftpmu010_write_reg(pmu_fd, 0x28, 0x0 << 24, 0x3 << 24))
        panic("%s, register ssp3 clock on fail\n", __func__);
}


static int pmu_init(ftssp010_card_t *card)
{
    if (!card) {
        printk("In %s: card is NULL.\n", __func__);
        return 0;
    }

    /* totally we have 2 ssps */
    if ((card->cardno < 0) || (card->cardno > 1)) {
        printk("In %s: cardno(%d) is not reasonable.\n", __func__, card->cardno);
        panic("Worong SSP cardno!!\n");
    }

    /* gm8136 */
    if (card->cardno <= 1) {
        if (pmu_fd < 0)
            pmu_fd = ftpmu010_register_reg(&ssp_clk_info);
        card->pmu_fd = pmu_fd;
        if (card->pmu_fd < 0) {
            printk("In %s: card(%d) register pmu setting failed\n", __func__, card->cardno);
            goto register_fail;
        }
        pmu_fd_cnt ++;
    }

    //if (card->cardno == 0)
      //  pmu_cal_clk1(card);
	  
/*Need to confirm*/
#if SSP2_CLK_SRC_24000K
    if (card->cardno == 2) {
        pmu_cal_clk(card);
    }
#endif
    return 0;

register_fail:
    panic("%s fail! \n", __func__);

    return -1;
}

static int pmu_deinit(ftssp010_card_t *card)
{
    if (ftpmu010_write_reg(card->pmu_fd, GATE_CLK , 0x3 << 4, GATE_CLK_BIT)!= 0)
	  {
	    printk("SSP_ALSA128 %s set as 0x%08X fail\n", __func__, GATE_CLK);                                                                                                        
    } 

    if (card->cardno <= 1) {/* GM8136 */
        pmu_fd_cnt --;
        if (!pmu_fd_cnt)
            ftpmu010_deregister_reg(card->pmu_fd);
    }

    return 0;
}

/*
 * Follows AHB DMA REQ/ACK routing Table
 */
static int ftssp010_get_dma_ch(int card_no, u32 *rx_ch, u32 *tx_ch)
{
    if (unlikely((rx_ch == NULL) || (tx_ch == NULL))) {
        printk("%s fails: rx_ch == NULL || tx_ch == NULL\n", __func__);
        return -EINVAL;
    }

    /* the following table is for AHB DMA */
    switch (card_no) {
      case 0:
        *rx_ch = 10;
        *tx_ch = 9;
        break;
      case 1:
        *rx_ch = 12;
        *tx_ch = 11;
        break;	
      default:
        panic("%s, invalid value: %d \n", __func__, card_no);
        break;
    }

    return 0;
}

static int ftssp010_driver_probe(struct platform_device *pdev)
{
    struct resource *res = NULL;
    ftssp010_card_t *card = platform_get_drvdata(pdev);
    int irq = -1, ret = -1;
    u32 rx_ch, tx_ch;
	
	/*8136 has 2 SSP*/
    if (card->cardno > 1)
        panic("%s, invalid card%d \n", __func__, card->cardno);

    card->dmac_pbase = DMAC_FTDMAC020_PA_BASE;
    card->dmac_vbase = (u32)ioremap_nocache(card->dmac_pbase, PAGE_SIZE);
    if (unlikely(!card->dmac_vbase))
        panic("%s, no virtual memory! \n", __func__);

    //get the SSP controller's memory I/O, this will be pyhsical address
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (unlikely(res == NULL)) {
        printk("%s fail: can't get resource\n", __func__);
        ret = -ENXIO;
        goto err_final;
    }

    card->pbase = res->start;
    card->mem_len = res->end - res->start + 1;

    if (unlikely((irq = platform_get_irq(pdev, 0)) < 0)) {
        printk("%s fail: can't get irq\n", __func__);
        ret = irq;
        goto err_final;
    }

    card->irq = irq;
    ftssp010_get_dma_ch(card->cardno, &rx_ch, &tx_ch);

    /* request a capture dma channel */
    if (rx_ch != DMA_UNKNOWN_CH) {
        dmaengine_t  *dmaengine = &card->capture.dmaengine;

        dma_cap_zero(dmaengine->cap_mask);
        dma_cap_set(DMA_SLAVE, dmaengine->cap_mask);

        dmaengine->dma_slave_config.id = 0;    //local AHB-DMA
        dmaengine->dma_slave_config.src_sel = FTDMA020_AHBMASTER_0;
        dmaengine->dma_slave_config.dst_sel = FTDMA020_AHBMASTER_1;

        dmaengine->dma_slave_config.handshake = rx_ch;   //hardware handshake pin
        dmaengine->dma_chan = dma_request_channel(dmaengine->cap_mask, ftdmac020_chan_filter,
                                            (void *)&dmaengine->dma_slave_config);

        if (!dmaengine->dma_chan) {
            printk("%s, No RX DMA channel for cardno:%d \n", __func__, card->cardno);
            ret = -ENOMEM;
            goto err_final;
        }
    }

    /* request a playback dma channel */
    if (tx_ch != DMA_UNKNOWN_CH) {
        dmaengine_t  *dmaengine = &card->playback.dmaengine;

        dma_cap_zero(dmaengine->cap_mask);
        dma_cap_set(DMA_SLAVE, dmaengine->cap_mask);

        dmaengine->dma_slave_config.id = 0;    //local AHB-DMA
        dmaengine->dma_slave_config.src_sel = FTDMA020_AHBMASTER_1;
        dmaengine->dma_slave_config.dst_sel = FTDMA020_AHBMASTER_0;

        dmaengine->dma_slave_config.handshake = tx_ch;   //hardware handshake pin
        dmaengine->dma_chan = dma_request_channel(dmaengine->cap_mask, ftdmac020_chan_filter,
                                            (void *)&dmaengine->dma_slave_config);

        if (!dmaengine->dma_chan) {
            printk("%s, No TX DMA channel for cardno:%d \n", __func__, card->cardno);
            ret = -ENOMEM;
            /* release rx channel */
            if (card->capture.dmaengine.dma_chan != NULL) {
                dma_release_channel(card->capture.dmaengine.dma_chan);
                card->capture.dmaengine.dma_chan = NULL;
            }
            goto err_final;
        }
    }

    //optional, but this is better for future debug
    if (!request_mem_region(res->start, (res->end - res->start + 1), dev_name(&pdev->dev))) {
        printk("%s fail: could not reserve memory region\n", __func__);
        ret = -ENOMEM;
        goto err_final;
    }

    //map pyhsical address to virtual address
    card->vbase = (u32) ioremap_nocache(res->start, (res->end - res->start + 1));
    if (unlikely(card->vbase == 0)) {
        printk("%s fail: counld not do ioremap\n", __func__);
        ret = -ENOMEM;
        goto err_ioremap;
    }

    /* init pmu */
    if (pmu_init(card) < 0)
        panic("%s, cardno:%d register pmu fail! \n", __func__, card->cardno);

    printk("SSP-128bit cardno = %d, pbase = 0x%x, vbase = 0x%x, irq = %d\n", card->cardno,
                                                        card->pbase, card->vbase, card->irq);
    return 0;

  err_ioremap:
    release_mem_region(card->pbase, card->mem_len);

  err_final:
    __iounmap((void *)card->dmac_vbase);
    card->dmac_vbase = 0;

    return ret;
}

static int ftssp010_driver_remove(struct platform_device *pdev)
{
    ftssp010_card_t *card = platform_get_drvdata(pdev);

    __iounmap((void *)card->dmac_vbase);
    card->dmac_vbase = 0;

    /* de-init pmu */
    pmu_deinit(card);

    return 0;
}

/* this driver will service SSP0, SSP1 and SSP2 */
static struct platform_driver ftssp010_driver = {
    .probe = ftssp010_driver_probe,
    .remove = ftssp010_driver_remove,
    .driver = {
               .name = "ftssp010",
               .owner = THIS_MODULE,
               },
};

static int ftssp010_device_bind_driver_data(struct platform_device *pdev, ftssp010_card_t * card)
{
    /* drv data */
    platform_set_drvdata(pdev, card);

    return platform_device_register(pdev);
}

int ftssp010_init_platform_device(ftssp010_card_t * card)
{
    if (card->cardno == 0) {
        return ftssp010_device_bind_driver_data(&ftssp010_0_device, card);
    } else if (card->cardno == 1) {
        return ftssp010_device_bind_driver_data(&ftssp010_1_device, card);
    } /*else if (card->cardno == 2) {
        return ftssp010_device_bind_driver_data(&ftssp010_2_device, card);
    }*/
    return -1;
}

static void ftssp010_release_resource(ftssp010_card_t *card)
{
    iounmap((void *)card->vbase);
    release_mem_region(card->pbase, card->mem_len);

    if (card->capture.dmaengine.dma_chan) {
        dma_release_channel(card->capture.dmaengine.dma_chan);
        card->capture.dmaengine.dma_chan = NULL;
    }

    if (card->playback.dmaengine.dma_chan) {
        dma_release_channel(card->playback.dmaengine.dma_chan);
        card->playback.dmaengine.dma_chan = NULL;
    }

    return;
}

void ftssp010_deinit_platform_device(ftssp010_card_t * card)
{
    printk("deinit SOUNDCARD#%d\n", card->cardno);

    ftssp010_release_resource(card);
    if (card->cardno == 0) {
        return platform_device_unregister(&ftssp010_0_device);
    } else if (card->cardno == 1) {
        return platform_device_unregister(&ftssp010_1_device);
    } /*else if (card->cardno == 2) {
        return platform_device_unregister(&ftssp010_2_device);
    }*/
}

int ftssp010_init_platform_driver(void)
{
    return platform_driver_register(&ftssp010_driver);
}

void ftssp0101_deinit_platform_driver(void)
{
    platform_driver_unregister(&ftssp010_driver);
}

int ftssp010_find_mclk(void)
{
    int i, max;
    unsigned long pll1;

	/*bcause the clock source setting in PMU is PLL1*/
    pll1 = PLL1_CLK_IN;
    max = sizeof(PLL1_MCLK_Map)/sizeof(audio_pll_mclk_map_t);;

    for (i=0; i<max; i++) {
        if (PLL1_MCLK_Map[i].pll1 == pll1) {
            Main_CLK = PLL1_MCLK_Map[i].mclk; 
            break;
        }
    }

    return (i>=max)?(-1):(0);
}


//mclk: the SSP_CLK
static int pmu_set_mclk(ftssp010_card_t *card, u32 speed, u32 *mclk, u32 *bclk,
                                                        ftssp_clock_table_t *clk_tab)
{
    int ret = 0, i;

    for (i = 0; i < clk_tab->number; i ++) {
        if (speed == clk_tab->fs_table[i]) {
            break;
        }
    }

    if (i == clk_tab->number) {
        err("freq (%d) not support, exit", speed);
        return -EINVAL;
    }

    if (clk_tab->bclk_table == NULL)
        panic("%s, error: The bclk table is null! \n", __func__);

	if (ftssp010_find_mclk() < 0)
		panic("%s, error: The mclk getting error! \n", __func__);
	
    *bclk = clk_tab->bclk_table[i];
    *mclk = Main_CLK;
/*need to confirm*/	
#if SSP2_CLK_SRC_24000K
    if (card->cardno == 2)
        *mclk = 24000000;
#endif
    return ret;
}

static int ftssp010_pmu_setting(ftssp010_card_t *card)
{
    int ret = 0, cardno = card->cardno;
    u32 mask = 0;
    int pmu_fd = card->pmu_fd;

    switch (cardno) {
      case 0:
      case 1:
        mask = 0x1 << (cardno + 4); //bit 6-8
        if (ftpmu010_write_reg(pmu_fd, 0xB8, 0, mask)) //pclk & sclk
            panic("%s, error in cardno %d \n", __func__, cardno);
        break;
 	  default:
        panic("%s, error in cardno %d \n", __func__, cardno);
        break;
    }

    return ret;
}

ftssp_hardware_platform_t ftssp_hw_platform = {
    .rates = SNDRV_PCM_RATE_8000_48000,
    .rate_min = 8000,
    .rate_max = 96000,
    .pmu_set_mclk = pmu_set_mclk,
    .pmu_set_pmu = ftssp010_pmu_setting,
};

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM SSP-ALSA driver");
MODULE_LICENSE("GPL");


