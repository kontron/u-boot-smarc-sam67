// SPDX-License-Identifier: GPL-2.0

#define LOG_CATEGORY UCLASS_RNG

#include <dm.h>
#include <rng.h>
#include <asm/system.h>
#include <linux/iopoll.h>

#define TRNG_INPUT		0x00
#define TRNG_INPUT_SIZE		16UL

#define TRNG_STATUS		0x10
#define TRNG_READY		BIT(0)

#define TRNG_CONTROL		0x14
#define ENABLE_TRNG		BIT(10)

#define TRNG_TIMEOUT		500000

struct omap_rng_priv {
	phys_addr_t base;
};

static int omap_rng_read(struct udevice *dev, void *data, size_t len)
{
	struct omap_rng_priv *priv = dev_get_priv(dev);
	uint32_t reg;
	int ret;

	while (len) {
		int tocopy = min(TRNG_INPUT_SIZE, len);

		/* ack the data to generate new data */
		writel(TRNG_READY, priv->base + TRNG_STATUS);

		/* wait until data is available */
		ret = readl_poll_timeout(priv->base + TRNG_STATUS, reg,
					 reg & TRNG_READY, TRNG_TIMEOUT);
		if (ret)
			return ret;

		/* copy the data */
		memcpy(data, (void*)priv->base + TRNG_INPUT, tocopy);
		len -= tocopy;
		data += tocopy;
	}

	return 0;
}

static int omap_rng_probe(struct udevice *dev)
{
	struct omap_rng_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* enable TRNG */
	writel(ENABLE_TRNG, priv->base + TRNG_CONTROL);

	return 0;
}

static const struct dm_rng_ops omap_rng_ops = {
	.read = omap_rng_read,
};

static const struct udevice_id omap_rng_match[] = {
	{ .compatible = "inside-secure,safexcel-eip76", },
	{},
};

U_BOOT_DRIVER(omap_rng) = {
	.name = "omap-rng",
	.id = UCLASS_RNG,
	.of_match = omap_rng_match,
	.ops = &omap_rng_ops,
	.probe = omap_rng_probe,
	.priv_auto = sizeof(struct omap_rng_priv),
};

/*
 * This is just a dummy implementation so the RNG driver is probed. It can be
 * removed if there is a real crypto driver for the SA2UL peripheral.
 */
static const struct udevice_id sa2ul_wrapper_match[] = {
	{ .compatible = "ti,am62-sa3ul", },
	{},
};

U_BOOT_DRIVER(sa2ul_wrapper) = {
	.name = "sa2ul-wrapper",
	.id = UCLASS_NOP,
	.of_match = sa2ul_wrapper_match,
	.bind = dm_scan_fdt_dev,
};
