// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019, Linaro Limited
 */

#include <dm.h>
#include <rand.h>
#include <rng.h>
#include <time.h>
#include <linux/string.h>

static int pseudo_rng_read(struct udevice *dev, void *data, size_t len)
{
	unsigned int i, seed, random;
	unsigned char *buf = data;
	size_t nrem, nloops;

	if (!len)
		return 0;

	nloops = len / sizeof(random);
	seed = get_timer(0) ^ rand();
	srand(seed);

	for (i = 0, nrem = len; i < nloops; i++) {
		random = rand();
		memcpy(buf, &random, sizeof(random));
		buf += sizeof(random);
		nrem -= sizeof(random);
	}

	if (nrem) {
		random = rand();
		memcpy(buf, &random, nrem);
	}

	return 0;
}

static const struct dm_rng_ops pseudo_rng_ops = {
	.read = pseudo_rng_read,
};

static const struct udevice_id pseudo_rng_match[] = {
	{
		.compatible = "sandbox,sandbox-rng",
	},
	{},
};

U_BOOT_DRIVER(pseudo_rng) = {
	.name = "pseudo-rng",
	.id = UCLASS_RNG,
	.of_match = pseudo_rng_match,
	.ops = &pseudo_rng_ops,
};
