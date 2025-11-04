// SPDX-License-Identifier: GPL-2.0+

#include <dm.h>
#include <log.h>
#include <sysinfo.h>
#include <asm/gpio.h>
#include <sysinfo/sa67.h>

struct sysinfo_sa67_priv {
	struct gpio_desc brdcfg_gpios[5];
	struct gpio_desc rev_gpios[2];
	int revision;
	int brdcfg;
};

static int sysinfo_sa67_detect(struct udevice *dev)
{
	struct sysinfo_sa67_priv *priv = dev_get_priv(dev);
	int ret;

	ret = dm_gpio_get_values_as_int_base3_pctrl(dev,
						    priv->rev_gpios,
						    ARRAY_SIZE(priv->rev_gpios));
	if (ret < 0)
		return ret;

	priv->revision = ret;

	ret = dm_gpio_get_values_as_int_base3_pctrl(dev,
						    priv->brdcfg_gpios,
						    ARRAY_SIZE(priv->brdcfg_gpios));
	if (ret < 0)
		return ret;

	priv->brdcfg = ret;

	return 0;
}

static int sysinfo_sa67_get_bool(struct udevice *dev, int id, bool *val)
{
	struct sysinfo_sa67_priv *priv = dev_get_priv(dev);
	int c0 = priv->brdcfg % 9;
	int c1 = (priv->brdcfg / 9) % 9;

	/*
	 * This only supports the preconfigured table mode. There are also
	 * custom board variants on the upper range.
	 */
	if (priv->brdcfg >= 81)
		return -ENOENT;

	switch (id) {
	case BOARD_HAS_GBE1:
		*val = c0 & 4;
		break;
	case BOARD_HAS_LVDS1:
		*val = (c1 & 2) == 0;
		break;
	case BOARD_HAS_DP0:
		*val = (c1 & 3) == 1;
		break;
	case BOARD_HAS_DSI1:
		*val = (c1 & 3) == 2;
		break;
	case BOARD_HAS_EDP1:
		*val = (c1 & 3) == 3;
		break;
	case BOARD_HAS_RTC_RV3032:
		*val = c1 & 4;
		break;
	case BOARD_HAS_RTC_RV8263:
		*val = !(c1 & 4);
		break;
	default:
		return -ENOENT;
	}

	return 0;
}

static int sysinfo_sa67_get_int(struct udevice *dev, int id, int *val)
{
	struct sysinfo_sa67_priv *priv = dev_get_priv(dev);
	int c0 = priv->brdcfg % 9;

	if (priv->brdcfg >= 81)
		return -ENOENT;

	switch (id) {
	case BOARD_REVISION:
		*val = priv->revision;
		break;
	case BOARD_CFG:
		*val = priv->brdcfg;
		break;
	case BOARD_MEMORY:
		*val = 1 << (c0 & 3);
		break;
	default:
		return -ENOENT;
	}

	return 0;
}

static int sysinfo_sa67_get_str(struct udevice *dev, int id, size_t size, char *val)
{
	return -ENOENT;
};


static const struct sysinfo_ops sysinfo_sa67_ops = {
	.detect = sysinfo_sa67_detect,
	.get_bool = sysinfo_sa67_get_bool,
	.get_int = sysinfo_sa67_get_int,
	.get_str = sysinfo_sa67_get_str,
};

static int sysinfo_sa67_probe(struct udevice *dev)
{
	struct sysinfo_sa67_priv *priv = dev_get_priv(dev);
	int ret;

	ret = gpio_request_list_by_name(dev, "revision-gpios", priv->rev_gpios,
					ARRAY_SIZE(priv->rev_gpios),
					GPIOD_IS_IN);
	if (ret != ARRAY_SIZE(priv->rev_gpios)) {
		debug("could not get revision-gpios (err = %d)\n", ret);
		return ret;
	}

	ret = gpio_request_list_by_name(dev, "boardcfg-gpios", priv->brdcfg_gpios,
					ARRAY_SIZE(priv->brdcfg_gpios),
					GPIOD_IS_IN);
	if (ret != ARRAY_SIZE(priv->brdcfg_gpios)) {
		debug("could not get boardcfg-gpios (err = %d)\n", ret);
		return ret;
	}

	return 0;
}

static const struct udevice_id sysinfo_sa67_ids[] = {
	{ .compatible = "kontron,sysinfo-sa67" },
	{}
};

U_BOOT_DRIVER(sysinfo_sa67) = {
	.name           = "sysinfo_sa67",
	.id             = UCLASS_SYSINFO,
	.of_match       = sysinfo_sa67_ids,
	.ops		= &sysinfo_sa67_ops,
	.priv_auto	= sizeof(struct sysinfo_sa67_priv),
	.probe          = sysinfo_sa67_probe,
};
