// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for Kontron SMARC-sAM67
 *
 * Copyright (c) 2024 Texas Instruments Incorporated - https://www.ti.com/
 * Copyright (c) 2025 Kontron Europe GmbH
 */

#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <spl.h>
#include <asm/arch/k3-ddr.h>

#define PADCFG_PULL_UP_CFG 0x260007
#define PADCFG_PULL_DOWN_CFG 0x240007
#define PADCFG_STRAPPING_BASE 0xf403c
#define GPIO0_IN 0x600020

int board_init(void)
{
	return 0;
}

#if IS_ENABLED(CONFIG_XPL_BUILD)
/*
 * The board has 8 strapping pins, which each can hold three values: high, low
 * and hiZ. If the fifth pin is low, the first four pins hold a predefined
 * feature set. If the fifth pin is high, individual configuratios are selected
 * (which is unsupported for now and reserved for future use). Pins 6 and 7 are
 * the board revision.
 *
 * There is also the sysinfo subsystem and dm_gpio_get_values_as_int_base3() but
 * unfortunately, we cannot use it as it needs to have device tree support and
 * we need the strapping pins to choose the corresponding device tree.
 */
static int sa67_strapping(void)
{
	uint8_t hi, lo;
	int ret = 0;
	long i;

	/* (1) enable pull-ups */
	for (i = 0; i < 32; i += 4)
		out_32(PADCFG_STRAPPING_BASE + i, PADCFG_PULL_UP_CFG);

	/* (2) read value */
	hi = in_32(GPIO0_IN) >> 15;

	/* (3) enable pull-downs */
	for (i = 0; i < 32; i += 4)
		out_32(PADCFG_STRAPPING_BASE + i, PADCFG_PULL_DOWN_CFG);

	/* (4) read value */
	lo = in_32(GPIO0_IN) >> 15;

	/* (5) calculate the base3 value */
	for (i = 7; i >= 0; i--) {
		ret *= 3;
		if ((hi & BIT(i)) == (lo & BIT(i)))
			ret += !!(hi & BIT(i));
		else
			ret += 2;
	}

	return ret;
}

static uint8_t sa67_memsize(void)
{
	int strapping = sa67_strapping();
	int c0 = strapping % 9;

	/* This is not yet supported, thus return the minimal feature set */
	if ((strapping / 81) % 3)
		return 1;

	return 1 << (c0 & 3);
}

int board_fit_config_name_match(const char *name)
{
	int memory_size = sa67_memsize();

	switch (memory_size) {
	case 1:
		return strcmp(name, "k3-am67a-r5-kontron-sa67-mt53e256m32d1ks");
	case 8:
		return strcmp(name, "k3-am67a-r5-kontron-sa67-mt53e2g32d4de");
	default:
		return 0;
	}
}

void spl_perform_fixups(struct spl_image_info *spl_image)
{
	int memory_size, ret;
	u64 start[2];
	u64 size[2];

	if (IS_ENABLED(CONFIG_K3_DDRSS) && IS_ENABLED(CONFIG_K3_INLINE_ECC))
		return fixup_ddr_driver_for_ecc(spl_image);

	memory_size = sa67_memsize();
	
	switch (memory_size) {
	default:
		printf("DDR size %d is not supported. Falling back to 1GiB\n",
		       memory_size);
		/* fallthrough */
	case 1:
		start[0] = 0x80000000;
		size[0] = 0x40000000;
		start[1] = 0;
		size[1] = 0;
		break;
	case 2:
		start[0] = 0x80000000;
		size[0] = 0x80000000;
		start[1] = 0;
		size[1] = 0;
		break;
	case 4:
		start[0] = 0x80000000;
		size[0] = 0x80000000;
		start[1] = 0x880000000;
		size[1] = 0x80000000;
		break;
	case 8:
		start[0] = 0x80000000;
		size[0] = 0x80000000;
		start[1] = 0x880000000;
		size[1] = 0x180000000;
		break;
	}

	ret = fdt_fixup_memory_banks(spl_image->fdt_addr, start, size,
				     ARRAY_SIZE(start));
	if (ret)
		printf("Error fixing up memory node (%d).\n", ret);
}
#endif
