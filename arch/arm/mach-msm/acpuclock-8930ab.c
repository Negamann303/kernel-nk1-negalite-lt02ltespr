/*
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mach/rpm-regulator.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_bus.h>

#include <linux/acpuclock-defines.h>

#include "acpuclock.h"
#include "acpuclock-krait.h"

/* Corner type vreg VDD values */
#define LVL_NONE	RPM_VREG_CORNER_NONE
#define LVL_LOW		RPM_VREG_CORNER_LOW
#define LVL_NOM		RPM_VREG_CORNER_NOMINAL
#define LVL_HIGH	RPM_VREG_CORNER_HIGH

static struct hfpll_data hfpll_data __initdata = {
	.mode_offset = 0x00,
	.l_offset = 0x08,
	.m_offset = 0x0C,
	.n_offset = 0x10,
	.config_offset = 0x04,
	.config_val = 0x7845C665,
	.has_droop_ctl = true,
	.droop_offset = 0x14,
	.droop_val = 0x0108C000,
	.low_vdd_l_max = 37,
	.nom_vdd_l_max = 74,
	.vdd[HFPLL_VDD_NONE] = LVL_NONE,
	.vdd[HFPLL_VDD_LOW]  = LVL_LOW,
	.vdd[HFPLL_VDD_NOM]  = LVL_NOM,
	.vdd[HFPLL_VDD_HIGH] = LVL_HIGH,
};

static struct scalable scalable_pm8917[] __initdata = {
	[CPU0] = {
		.hfpll_phys_base = 0x00903200,
		.aux_clk_sel_phys = 0x02088014,
		.aux_clk_sel = 3,
		.sec_clk_sel = 2,
		.l2cpmr_iaddr = 0x4501,
		.vreg[VREG_CORE] = { "krait0", 1300000 },
		.vreg[VREG_MEM]  = { "krait0_mem", 1150000 },
		.vreg[VREG_DIG]  = { "krait0_dig", 1150000 },
		.vreg[VREG_HFPLL_A] = { "krait0_s8", 2050000 },
		.vreg[VREG_HFPLL_B] = { "krait0_l23", 1800000 },
	},
	[CPU1] = {
		.hfpll_phys_base = 0x00903300,
		.aux_clk_sel_phys = 0x02098014,
		.aux_clk_sel = 3,
		.sec_clk_sel = 2,
		.l2cpmr_iaddr = 0x5501,
		.vreg[VREG_CORE] = { "krait1", 1300000 },
		.vreg[VREG_MEM]  = { "krait1_mem", 1150000 },
		.vreg[VREG_DIG]  = { "krait1_dig", 1150000 },
		.vreg[VREG_HFPLL_A] = { "krait1_s8", 2050000 },
		.vreg[VREG_HFPLL_B] = { "krait1_l23", 1800000 },
	},
	[L2] = {
		.hfpll_phys_base = 0x00903400,
		.aux_clk_sel_phys = 0x02011028,
		.aux_clk_sel = 3,
		.sec_clk_sel = 2,
		.l2cpmr_iaddr = 0x0500,
		.vreg[VREG_HFPLL_A] = { "l2_s8", 2050000 },
		.vreg[VREG_HFPLL_B] = { "l2_l23", 1800000 },
	},
};

static struct scalable scalable[] __initdata = {
	[CPU0] = {
		.hfpll_phys_base = 0x00903200,
		.aux_clk_sel_phys = 0x02088014,
		.aux_clk_sel = 3,
		.sec_clk_sel = 2,
		.l2cpmr_iaddr = 0x4501,
		.vreg[VREG_CORE] = { "krait0", MAX_VDD_SC },
		.vreg[VREG_MEM]  = { "krait0_mem", MAX_VDD_MEM_DIG },
		.vreg[VREG_DIG]  = { "krait0_dig", MAX_VDD_MEM_DIG },
		.vreg[VREG_HFPLL_A] = { "krait0_hfpll", 1800000 },
	},
	[CPU1] = {
		.hfpll_phys_base = 0x00903300,
		.aux_clk_sel_phys = 0x02098014,
		.aux_clk_sel = 3,
		.sec_clk_sel = 2,
		.l2cpmr_iaddr = 0x5501,
		.vreg[VREG_CORE] = { "krait1", MAX_VDD_SC },
		.vreg[VREG_MEM]  = { "krait1_mem", MAX_VDD_MEM_DIG },
		.vreg[VREG_DIG]  = { "krait1_dig", MAX_VDD_MEM_DIG },
		.vreg[VREG_HFPLL_A] = { "krait1_hfpll", 1800000 },
	},
	[L2] = {
		.hfpll_phys_base = 0x00903400,
		.aux_clk_sel_phys = 0x02011028,
		.aux_clk_sel = 3,
		.sec_clk_sel = 2,
		.l2cpmr_iaddr = 0x0500,
		.vreg[VREG_HFPLL_A] = { "l2_hfpll", 1800000 },
	},
};

static struct msm_bus_paths bw_level_tbl[] __initdata = {
	[0] =  BW_MBPS(528), /* At least  66 MHz on bus. */
	[1] = BW_MBPS(1064), /* At least 133 MHz on bus. */
	[2] = BW_MBPS(1600), /* At least 200 MHz on bus. */
	[3] = BW_MBPS(2128), /* At least 266 MHz on bus. */
	[4] = BW_MBPS(3200), /* At least 400 MHz on bus. */
	[5] = BW_MBPS(4264), /* At least 533 MHz on bus. */
	[6] = BW_MBPS(4800), /* At least 600 MHz on bus. */
	[7] = BW_MBPS(6128), /* At least 766 MHz on bus. */
	[8] = BW_MBPS(6400), /* At least 800 MHz on bus. */
};

static struct msm_bus_scale_pdata bus_scale_data __initdata = {
	.usecase = bw_level_tbl,
	.num_usecases = ARRAY_SIZE(bw_level_tbl),
	.active_only = 1,
	.name = "acpuclk-8930ab",
};

static struct l2_level l2_freq_tbl[] __initdata = {
	[0]  = { {  384000, PLL_8, 0, 0x00 },  LVL_LOW, 1050000, 2 },
	[1]  = { {  432000, HFPLL, 2, 0x20 },  LVL_NOM, 1050000, 2 },
	[2]  = { {  486000, HFPLL, 2, 0x24 },  LVL_NOM, 1050000, 2 },
	[3]  = { {  540000, HFPLL, 2, 0x28 },  LVL_NOM, 1050000, 3 },
	[4]  = { {  594000, HFPLL, 1, 0x16 },  LVL_NOM, 1050000, 3 },
	[5]  = { {  648000, HFPLL, 1, 0x18 },  LVL_NOM, 1050000, 3 },
	[6]  = { {  702000, HFPLL, 1, 0x1A },  LVL_NOM, 1050000, 4 },
	[7]  = { {  756000, HFPLL, 1, 0x1C }, LVL_HIGH, 1150000, 4 },
	[8]  = { {  810000, HFPLL, 1, 0x1E }, LVL_HIGH, 1150000, 4 },
	[9]  = { {  864000, HFPLL, 1, 0x20 }, LVL_HIGH, 1150000, 5 },
	[10] = { {  918000, HFPLL, 1, 0x22 }, LVL_HIGH, 1150000, 5 },
	[11] = { {  972000, HFPLL, 1, 0x24 }, LVL_HIGH, 1150000, 5 },
	[12] = { { 1026000, HFPLL, 1, 0x26 }, LVL_HIGH, 1150000, 6 },
	[13] = { { 1080000, HFPLL, 1, 0x28 }, LVL_HIGH, 1150000, 6 },
	[14] = { { 1134000, HFPLL, 1, 0x2A }, LVL_HIGH, 1150000, 6 },
	[15] = { { 1188000, HFPLL, 1, 0x2C }, LVL_HIGH, 1150000, 7 },
	[16] = { { 1242000, HFPLL, 1, 0x2E }, LVL_HIGH, MAX_VDD_MEM_DIG, 7 },
	[17] = { { 1296000, HFPLL, 1, 0x30 }, LVL_HIGH, MAX_VDD_MEM_DIG, 7 },
	[18] = { { 1404000, HFPLL, 1, 0x32 }, LVL_HIGH, MAX_VDD_MEM_DIG, 8 },
	[19] = { { 1458000, HFPLL, 1, 0x34 }, LVL_HIGH, MAX_VDD_MEM_DIG, 8 },
	[20] = { { 1512000, HFPLL, 1, 0x36 }, LVL_HIGH, MAX_VDD_MEM_DIG, 8 },			
	{ }
};

static struct acpu_level tbl_PVS0_1700MHz[] __initdata = {
	{ 1, {   384000, PLL_8, 0, 0x00 }, L2(0),  1000000 },
	{ 1, {   486000, HFPLL, 2, 0x24 }, L2(5),  1000000 },
	{ 1, {   594000, HFPLL, 1, 0x16 }, L2(5),  1000000 },
	{ 1, {   702000, HFPLL, 1, 0x1A }, L2(5),  1025000 },
	{ 1, {   810000, HFPLL, 1, 0x1E }, L2(10), 1050000 },
	{ 1, {   918000, HFPLL, 1, 0x22 }, L2(10), 1075000 },
	{ 1, {  1026000, HFPLL, 1, 0x26 }, L2(10), 1100000 },
	{ 1, {  1134000, HFPLL, 1, 0x2A }, L2(15), 1125000 },
	{ 1, {  1242000, HFPLL, 1, 0x2E }, L2(15), 1150000 },
	{ 1, {  1350000, HFPLL, 1, 0x32 }, L2(15), 1175000 },
	{ 1, {  1458000, HFPLL, 1, 0x36 }, L2(15), 1200000 },
	{ 1, {  1566000, HFPLL, 1, 0x3A }, L2(15), 1225000 },
	{ 1, {  1674000, HFPLL, 1, 0x3E }, L2(15), 1250000 },
	{ 1, {  1728000, HFPLL, 1, 0x40 }, L2(15), 1275000 },
	{ 0, { 0 } }
};

static struct acpu_level tbl_PVS1_1700MHz[] __initdata = {
	{ 1, {   384000, PLL_8, 0, 0x00 }, L2(0),   975000 },
	{ 1, {   486000, HFPLL, 2, 0x24 }, L2(5),   975000 },
	{ 1, {   594000, HFPLL, 1, 0x16 }, L2(5),  1000000 },
	{ 1, {   702000, HFPLL, 1, 0x1A }, L2(5),  1000000 },
	{ 1, {   810000, HFPLL, 1, 0x1E }, L2(10), 1025000 },
	{ 1, {   918000, HFPLL, 1, 0x22 }, L2(10), 1050000 },
	{ 1, {  1026000, HFPLL, 1, 0x26 }, L2(10), 1075000 },
	{ 1, {  1134000, HFPLL, 1, 0x2A }, L2(15), 1100000 },
	{ 1, {  1242000, HFPLL, 1, 0x2E }, L2(15), 1125000 },
	{ 1, {  1350000, HFPLL, 1, 0x32 }, L2(15), 1150000 },
	{ 1, {  1458000, HFPLL, 1, 0x36 }, L2(15), 1175000 },
	{ 1, {  1566000, HFPLL, 1, 0x3A }, L2(15), 1200000 },
	{ 1, {  1674000, HFPLL, 1, 0x3E }, L2(15), 1225000 },
	{ 1, {  1728000, HFPLL, 1, 0x40 }, L2(15), 1250000 },
	{ 0, { 0 } }
};

static struct acpu_level tbl_PVS2_1700MHz[] __initdata = {
	{ 1, {   384000, PLL_8, 0, 0x00 }, L2(0),   950000 },
	{ 1, {   486000, HFPLL, 2, 0x24 }, L2(5),   950000 },
	{ 1, {   594000, HFPLL, 1, 0x16 }, L2(5),   950000 },
	{ 1, {   702000, HFPLL, 1, 0x1A }, L2(5),   975000 },
	{ 1, {   810000, HFPLL, 1, 0x1E }, L2(10), 1000000 },
	{ 1, {   918000, HFPLL, 1, 0x22 }, L2(10), 1025000 },
	{ 1, {  1026000, HFPLL, 1, 0x26 }, L2(10), 1050000 },
	{ 1, {  1134000, HFPLL, 1, 0x2A }, L2(15), 1075000 },
	{ 1, {  1242000, HFPLL, 1, 0x2E }, L2(15), 1100000 },
	{ 1, {  1350000, HFPLL, 1, 0x32 }, L2(15), 1125000 },
	{ 1, {  1458000, HFPLL, 1, 0x36 }, L2(15), 1150000 },
	{ 1, {  1566000, HFPLL, 1, 0x3A }, L2(15), 1175000 },
	{ 1, {  1674000, HFPLL, 1, 0x3E }, L2(15), 1200000 },
	{ 1, {  1728000, HFPLL, 1, 0x40 }, L2(15), 1225000 },
	{ 0, { 0 } }
};

static struct acpu_level tbl_PVS3_1700MHz[] __initdata = {
#ifdef CONFIG_LOW_CPUCLOCKS
	{ 1, {    81000, HFPLL, 2, 0x08 }, L2(3),   862500 },
	{ 1, {   135000, HFPLL, 2, 0x0A }, L2(3),   875000 },
	{ 1, {   162000, HFPLL, 2, 0x0C }, L2(3),   887500 },
	{ 1, {   189000, HFPLL, 2, 0x0E }, L2(3),   900000 },
	{ 1, {   216000, HFPLL, 2, 0x10 }, L2(8),   912500 },
	{ 1, {   270000, HFPLL, 2, 0x14 }, L2(8),   925000 },
	{ 1, {   324000, HFPLL, 2, 0x18 }, L2(8),   937500 },
	{ 1, {   378000, HFPLL, 2, 0x1A }, L2(8),   950000 },
#endif
	{ 1, {   384000, HFPLL, 1, 0x0E }, L2(7),   962500 },
	{ 1, {   432000, HFPLL, 1, 0x10 }, L2(7),   975000 },
	{ 1, {   486000, HFPLL, 1, 0x12 }, L2(7),   987500 },
	{ 1, {   540000, HFPLL, 1, 0x14 }, L2(7),  1000000 },
	{ 1, {   594000, HFPLL, 1, 0x16 }, L2(7),  1012500 },
	{ 1, {   648000, HFPLL, 1, 0x18 }, L2(7),  1025000 },
	{ 1, {   702000, HFPLL, 1, 0x1A }, L2(7),  1037500 },
	{ 1, {   756000, HFPLL, 1, 0x1C }, L2(7),  1050000 },
	{ 1, {   810000, HFPLL, 1, 0x1E }, L2(12),  1062500 },
	{ 1, {   864000, HFPLL, 1, 0x20 }, L2(12),  1075000 },
	{ 1, {   918000, HFPLL, 1, 0x22 }, L2(12),  1087500 },
	{ 1, {   972000, HFPLL, 1, 0x24 }, L2(12),  1100000 },
	{ 1, {  1026000, HFPLL, 1, 0x26 }, L2(12),  1112500 },
	{ 1, {  1080000, HFPLL, 1, 0x28 }, L2(17),  1125000 },
	{ 1, {  1134000, HFPLL, 1, 0x2A }, L2(17), 1137500 },
	{ 1, {  1188000, HFPLL, 1, 0x2C }, L2(17), 1150000 },
	{ 1, {  1242000, HFPLL, 1, 0x2E }, L2(17), 1162500 },
	{ 1, {  1296000, HFPLL, 1, 0x30 }, L2(17), 1175000 },
	{ 1, {  1350000, HFPLL, 1, 0x32 }, L2(17), 1187500 },
	{ 1, {  1404000, HFPLL, 1, 0x34 }, L2(17), 1200000 },	
	{ 1, {  1458000, HFPLL, 1, 0x36 }, L2(17), 1212500 },
	{ 1, {  1512000, HFPLL, 1, 0x38 }, L2(17), 1225000 },	
	{ 1, {  1566000, HFPLL, 1, 0x3A }, L2(17), 1237500 },
	{ 1, {  1620000, HFPLL, 1, 0x3C }, L2(17), 1250000 },
	{ 1, {  1674000, HFPLL, 1, 0x3E }, L2(17), 1262500 },
	{ 1, {  1728000, HFPLL, 1, 0x40 }, L2(17), 1275000 },
#ifdef CONFIG_HIGH_CPUCLOCKS
	{ 1, {  1782000, HFPLL, 1, 0x42 }, L2(17), 1287500 },
//	{ 1, {  1836000, HFPLL, 1, 0x44 }, L2(17), 1287500 },
//	{ 1, {  1890000, HFPLL, 1, 0x46 }, L2(17), 1300000 },
#endif	
	{ 0, { 0 } }
};

static struct acpu_level tbl_PVS4_1700MHz[] __initdata = {
	{ 1, {   384000, PLL_8, 0, 0x00 }, L2(0),   925000 },
	{ 1, {   486000, HFPLL, 2, 0x24 }, L2(5),   925000 },
	{ 1, {   594000, HFPLL, 1, 0x16 }, L2(5),   925000 },
	{ 1, {   702000, HFPLL, 1, 0x1A }, L2(5),   925000 },
	{ 1, {   810000, HFPLL, 1, 0x1E }, L2(10),  950000 },
	{ 1, {   918000, HFPLL, 1, 0x22 }, L2(10),  975000 },
	{ 1, {  1026000, HFPLL, 1, 0x26 }, L2(10), 1000000 },
	{ 1, {  1134000, HFPLL, 1, 0x2A }, L2(15), 1025000 },
	{ 1, {  1242000, HFPLL, 1, 0x2E }, L2(15), 1050000 },
	{ 1, {  1350000, HFPLL, 1, 0x32 }, L2(15), 1075000 },
	{ 1, {  1458000, HFPLL, 1, 0x36 }, L2(15), 1100000 },
	{ 1, {  1566000, HFPLL, 1, 0x3A }, L2(15), 1125000 },
	{ 1, {  1674000, HFPLL, 1, 0x3E }, L2(15), 1150000 },
	{ 1, {  1728000, HFPLL, 1, 0x40 }, L2(15), 1175000 },
	{ 0, { 0 } }
};

static struct acpu_level tbl_PVS5_1700MHz[] __initdata = {
	{ 1, {   384000, PLL_8, 0, 0x00 }, L2(0),   900000 },
	{ 1, {   486000, HFPLL, 2, 0x24 }, L2(5),   900000 },
	{ 1, {   594000, HFPLL, 1, 0x16 }, L2(5),   900000 },
	{ 1, {   702000, HFPLL, 1, 0x1A }, L2(5),   900000 },
	{ 1, {   810000, HFPLL, 1, 0x1E }, L2(10),  925000 },
	{ 1, {   918000, HFPLL, 1, 0x22 }, L2(10),  950000 },
	{ 1, {  1026000, HFPLL, 1, 0x26 }, L2(10),  975000 },
	{ 1, {  1134000, HFPLL, 1, 0x2A }, L2(15), 1000000 },
	{ 1, {  1242000, HFPLL, 1, 0x2E }, L2(15), 1025000 },
	{ 1, {  1350000, HFPLL, 1, 0x32 }, L2(15), 1050000 },
	{ 1, {  1458000, HFPLL, 1, 0x36 }, L2(15), 1075000 },
	{ 1, {  1566000, HFPLL, 1, 0x3A }, L2(15), 1100000 },
	{ 1, {  1674000, HFPLL, 1, 0x3E }, L2(15), 1125000 },
	{ 1, {  1728000, HFPLL, 1, 0x40 }, L2(15), 1150000 },
	{ 0, { 0 } }
};

static struct acpu_level tbl_PVS6_1700MHz[] __initdata = {
	{ 1, {   384000, PLL_8, 0, 0x00 }, L2(0),   875000 },
	{ 1, {   486000, HFPLL, 2, 0x24 }, L2(5),   875000 },
	{ 1, {   594000, HFPLL, 1, 0x16 }, L2(5),   875000 },
	{ 1, {   702000, HFPLL, 1, 0x1A }, L2(5),   875000 },
	{ 1, {   810000, HFPLL, 1, 0x1E }, L2(10),  900000 },
	{ 1, {   918000, HFPLL, 1, 0x22 }, L2(10),  925000 },
	{ 1, {  1026000, HFPLL, 1, 0x26 }, L2(10),  950000 },
	{ 1, {  1134000, HFPLL, 1, 0x2A }, L2(15),  975000 },
	{ 1, {  1242000, HFPLL, 1, 0x2E }, L2(15), 1000000 },
	{ 1, {  1350000, HFPLL, 1, 0x32 }, L2(15), 1025000 },
	{ 1, {  1458000, HFPLL, 1, 0x36 }, L2(15), 1050000 },
	{ 1, {  1566000, HFPLL, 1, 0x3A }, L2(15), 1075000 },
	{ 1, {  1674000, HFPLL, 1, 0x3E }, L2(15), 1100000 },
	{ 1, {  1728000, HFPLL, 1, 0x40 }, L2(15), 1125000 },
	{ 0, { 0 } }
};

static struct pvs_table pvs_tables[NUM_SPEED_BINS][NUM_PVS] __initdata = {
	[0][0] = { tbl_PVS0_1700MHz, sizeof(tbl_PVS0_1700MHz), 0 },
	[0][1] = { tbl_PVS1_1700MHz, sizeof(tbl_PVS1_1700MHz), 0 },
	[0][2] = { tbl_PVS2_1700MHz, sizeof(tbl_PVS2_1700MHz), 0 },
	[0][3] = { tbl_PVS3_1700MHz, sizeof(tbl_PVS3_1700MHz), 0 },
	[0][4] = { tbl_PVS4_1700MHz, sizeof(tbl_PVS4_1700MHz), 0 },
	[0][5] = { tbl_PVS5_1700MHz, sizeof(tbl_PVS5_1700MHz), 0 },
	[0][6] = { tbl_PVS6_1700MHz, sizeof(tbl_PVS6_1700MHz), 0 },
};

static struct acpuclk_krait_params acpuclk_8930ab_params __initdata = {
	.scalable = scalable,
	.scalable_size = sizeof(scalable),
	.hfpll_data = &hfpll_data,
	.pvs_tables = pvs_tables,
	.l2_freq_tbl = l2_freq_tbl,
	.l2_freq_tbl_size = sizeof(l2_freq_tbl),
	.bus_scale = &bus_scale_data,
	.pte_efuse_phys = 0x007000C0,
#ifdef CONFIG_LOW_CPUCLOCKS
	.stby_khz = 378000,
#else	
	.stby_khz = 384000,
#endif
};

static int __init acpuclk_8930ab_probe(struct platform_device *pdev)
{
	struct acpuclk_platform_data *pdata = pdev->dev.platform_data;
	if (pdata && pdata->uses_pm8917)
		acpuclk_8930ab_params.scalable = scalable_pm8917;

	return acpuclk_krait_init(&pdev->dev, &acpuclk_8930ab_params);
}

static struct platform_driver acpuclk_8930ab_driver = {
	.driver = {
		.name = "acpuclk-8930ab",
		.owner = THIS_MODULE,
	},
};

static int __init acpuclk_8930ab_init(void)
{
	return platform_driver_probe(&acpuclk_8930ab_driver,
				     acpuclk_8930ab_probe);
}
device_initcall(acpuclk_8930ab_init);
