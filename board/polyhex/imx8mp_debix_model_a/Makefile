#
# Copyright 2019 NXP
# Copyright 2023 Gilles Talis <gilles.talis@gmail.com>
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mp_debix_model_a.o

ifdef CONFIG_XPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
endif
