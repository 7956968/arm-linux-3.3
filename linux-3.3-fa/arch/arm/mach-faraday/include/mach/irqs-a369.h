/*
 *  arch/arm/mach-faraday/include/mach/irqs-a369.h
 *
 *  Copyright (C) 2010 Faraday Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MACH_IRQS_A369_H
#define __MACH_IRQS_A369_H

#ifdef CONFIG_PLATFORM_A369

/*
 * Interrupt numbers of Hierarchical Architecture
 */

#ifdef CONFIG_CPU_FMP626

/* GIC DIST in FMP626 */
#define IRQ_LOCALTIMER		29
#define IRQ_LOCALWDOG		30
#define PLATFORM_LEGACY_IRQ	31
#define IRQ_FMP626_GIC_START	32

#define IRQ_FMP626_PMU_CPU0	(IRQ_FMP626_GIC_START + 17)
#define IRQ_FMP626_PMU_CPU1	(IRQ_FMP626_GIC_START + 18)
#define IRQ_FMP626_PMU_SCU0	(IRQ_FMP626_GIC_START + 21)
#define IRQ_FMP626_PMU_SCU1	(IRQ_FMP626_GIC_START + 22)
#define IRQ_FMP626_PMU_SCU2	(IRQ_FMP626_GIC_START + 23)
#define IRQ_FMP626_PMU_SCU3	(IRQ_FMP626_GIC_START + 24)

#define IRQ_A369_START	256	/* this is determined by irqs supported by GIC */

#else	/* CONFIG_CPU_FMP626 */

#define IRQ_A369_START	0

#endif	/* CONFIG_CPU_FMP626 */

#define IRQ_A369_FTPWMTMR010_0_T0	(IRQ_A369_START + 8)
#define IRQ_A369_FTPWMTMR010_0_T1	(IRQ_A369_START + 9)
#define IRQ_A369_FTPWMTMR010_0_T2	(IRQ_A369_START + 10)
#define IRQ_A369_FTPWMTMR010_0_T3	(IRQ_A369_START + 11)
#define IRQ_A369_FTDDRII030_0		(IRQ_A369_START + 12)
#define IRQ_A369_FTAHBB020_3		(IRQ_A369_START + 13)
#define IRQ_A369_FTAPBB020_0		(IRQ_A369_START + 14)
#define IRQ_A369_FTDMAC020_0_TC		(IRQ_A369_START + 15)
#define IRQ_A369_FTDMAC020_0_ERR	(IRQ_A369_START + 16)
#define IRQ_A369_FTDMAC020_1_TC		(IRQ_A369_START + 17)
#define IRQ_A369_FTDMAC020_1_ERR	(IRQ_A369_START + 18)
#define IRQ_A369_FTTSC010_0_ADC		(IRQ_A369_START + 19)
#define IRQ_A369_FTTSC010_0_PANEL	(IRQ_A369_START + 20)
#define IRQ_A369_FTKBC010_0		(IRQ_A369_START + 21)
#define IRQ_A369_FTLCDC200_0_MERR	(IRQ_A369_START + 22)
#define IRQ_A369_FTLCDC200_0_FUR	(IRQ_A369_START + 23)
#define IRQ_A369_FTLCDC200_0_BAUPD	(IRQ_A369_START + 24)
#define IRQ_A369_FTLCDC200_0_VSTATUS	(IRQ_A369_START + 25)
#define IRQ_A369_FTMCP100_0		(IRQ_A369_START + 27)
#define IRQ_A369_FTMCP220_0		(IRQ_A369_START + 28)
#define IRQ_A369_FTNANDC021_0		(IRQ_A369_START + 30)
#define IRQ_A369_FTAES020_0		(IRQ_A369_START + 31)
#define IRQ_A369_FTGMAC100_0_IRQ	(IRQ_A369_START + 32)
#define IRQ_A369_FTSATA100_0		(IRQ_A369_START + 33)
#define IRQ_A369_FTSATA110_0		(IRQ_A369_START + 34)
#define IRQ_A369_FTPCIE3914_0		(IRQ_A369_START + 35)
#define IRQ_A369_FUSBH200_0		(IRQ_A369_START + 36)
#define IRQ_A369_FOTG2XX_0		(IRQ_A369_START + 37)
#define IRQ_A369_FTSDC010_0		(IRQ_A369_START + 38)
#define IRQ_A369_FTSDC010_1		(IRQ_A369_START + 39)
#define IRQ_A369_FTIDE020_0		(IRQ_A369_START + 40)
#define IRQ_A369_FTSCU010_0		(IRQ_A369_START + 41)
#define IRQ_A369_FTRTC011_0_ALRM	(IRQ_A369_START + 42)
#define IRQ_A369_FTRTC011_0_SEC		(IRQ_A369_START + 43)
#define IRQ_A369_FTRTC011_0_MIN		(IRQ_A369_START + 44)
#define IRQ_A369_FTRTC011_0_HOUR	(IRQ_A369_START + 45)
#define IRQ_A369_FTWDT010_0		(IRQ_A369_START + 46)
#define IRQ_A369_FTGPIO010_0		(IRQ_A369_START + 47)
#define IRQ_A369_FTGPIO010_1		(IRQ_A369_START + 48)
#define IRQ_A369_FTSSP010_0		(IRQ_A369_START + 49)
#define IRQ_A369_FTSSP010_1		(IRQ_A369_START + 50)
#define IRQ_A369_FTIIC010_0		(IRQ_A369_START + 51)
#define IRQ_A369_FTIIC010_1		(IRQ_A369_START + 52)
#define IRQ_A369_FTUART010_0		(IRQ_A369_START + 53)
#define IRQ_A369_FTUART010_1		(IRQ_A369_START + 54)
#define IRQ_A369_FTUART010_2		(IRQ_A369_START + 55)
#define IRQ_A369_FTUART010_3		(IRQ_A369_START + 56)

/* AHBB */
#define IRQ_FTAHBB020_3_START		(IRQ_A369_START + 64)
#define IRQ_FTAHBB020_3_EXINT(x)	(IRQ_FTAHBB020_3_START + (x))

/* PCI */
#define IRQ_FTPCIE3914_0_START		(IRQ_FTAHBB020_3_START + 32)

#define IRQ_FTPCIE3914_0_A		(IRQ_FTPCIE3914_0_START + 0)
#define IRQ_FTPCIE3914_0_B		(IRQ_FTPCIE3914_0_START + 1)
#define IRQ_FTPCIE3914_0_C		(IRQ_FTPCIE3914_0_START + 2)
#define IRQ_FTPCIE3914_0_D		(IRQ_FTPCIE3914_0_START + 3)

#define NR_IRQS				(IRQ_FTPCIE3914_0_START + 4)

#endif	/* CONFIG_PLATFORM_A369 */

#endif	/* __MACH_IRQS_A369_H */
