// SPDX-License-Identifier: GPL-2.0+
/*
 * Common board specific initialization for Kontron SMARC-sAM67
 *
 * Copyright (c) 2025 Kontron Europe GmbH
 */

#include <asm/arch/hardware.h>
#include <spl.h>

#define MAIN_DEVSTAT_PRIMARY_SPI_CSEL 0x40

const char *spl_board_loader_name(u32 boot_device)
{
	static char name[16];
	u32 devstat = readl(CTRLMMR_MAIN_DEVSTAT);
	struct mmc *mmc;

	switch (boot_device) {
#if CONFIG_IS_ENABLED(DM_MMC)
	case BOOT_DEVICE_MMC1:
		mmc_init_device(0);
		mmc = find_mmc_device(0);
		mmc_init(mmc);
		snprintf(name, sizeof(name), "eMMC (%s)",
			 emmc_hwpart_names[EXT_CSD_EXTRACT_BOOT_PART(mmc->part_config)]);
		return name;
#endif
	case BOOT_DEVICE_MMC2:
		return "SD card (Test mode)";
	case BOOT_DEVICE_SPI:
		if (devstat & MAIN_DEVSTAT_PRIMARY_SPI_CSEL)
			return "External SPI flash";
		else
			return "Failsafe SPI flash";
	case BOOT_DEVICE_DFU:
		return "DFU";
	default:
		return "(unknown)";
	}
}
