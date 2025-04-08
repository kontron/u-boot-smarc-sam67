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
#include <env_internal.h>
#include <fdt_support.h>
#include <fs.h>
#include <malloc.h>
#include <spl.h>
#include <asm/arch/k3-ddr.h>

static int sa67_boot_device(void)
{
	u32 devstat = readl(CTRLMMR_MAIN_DEVSTAT);
	u32 bootmode = (devstat & MAIN_DEVSTAT_PRIMARY_BOOTMODE_MASK) >>
				MAIN_DEVSTAT_PRIMARY_BOOTMODE_SHIFT;
	return bootmode;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	int boot_device = sa67_boot_device();

	if (prio != 0)
		return ENVL_UNKNOWN;

	if (!CONFIG_IS_ENABLED(ENV_IS_IN_MMC) && !CONFIG_IS_ENABLED(ENV_IS_NOWHERE))
		return ENVL_UNKNOWN;

	if (!CONFIG_IS_ENABLED(ENV_IS_IN_MMC))
		return ENVL_NOWHERE;

	/* write and erase always operate on the stored environment */
	if (op == ENVOP_SAVE || op == ENVOP_ERASE)
		return ENVL_MMC;

	/*
	 * failsafe and manufacturer boot will always use the compiled-in
	 * default environment.
	 */
	if (boot_device == BOOT_DEVICE_SPI || boot_device == BOOT_DEVICE_MMC)
		return ENVL_NOWHERE;

	return ENVL_MMC;
}

static void sa67_import_environment(void)
{
	const char *filename = "u-boot-env.txt";
	const int bufsize = 16 * 1024;
	char *buf = malloc(bufsize);
	loff_t size;

	/* this is only for production purposes */
	if (sa67_boot_device() != BOOT_DEVICE_MMC)
		goto out;

	if (fs_set_blk_dev("mmc", "1:1", FS_TYPE_ANY))
		goto out;

	if (!fs_exists(filename))
		goto out;

	if (fs_set_blk_dev("mmc", "1:1", FS_TYPE_ANY))
		goto out;

	if (fs_read(filename, (ulong)buf, 0, bufsize, &size))
		goto out;

	if (!himport_r(&env_htab, buf, size, '\n', H_NOCLEAR, 1, 0, NULL))
		goto out;

	printf("Imported additional environment from %s\n", filename);

out:
	free(buf);
}

static void sa67_set_prompt(void)
{
	int boot_device = sa67_boot_device();

	switch (boot_device) {
	case BOOT_DEVICE_SPI:
		env_set("PS1", "[FAILSAFE] => ");
		break;
	case BOOT_DEVICE_MMC:
		env_set("PS1", "[SDHC] => ");
		break;
	default:
		env_set("PS1", NULL);
		break;
	}
}

int board_init(void)
{
	return 0;
}

int dram_init(void)
{
	return fdtdec_setup_mem_size_base_lowest();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_CMDLINE_PS_SUPPORT))
		sa67_set_prompt();

	sa67_import_environment();

	return 0;
}

#if IS_ENABLED(CONFIG_XPL_BUILD)
void spl_perform_fixups(struct spl_image_info *spl_image)
{
	fixup_memory_node(spl_image);
}
#endif
