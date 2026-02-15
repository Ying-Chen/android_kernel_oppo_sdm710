# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.

ifeq ($(CONFIG_KASAN),y)
subdir-ccflags-y += -Wno-format
endif

ifndef CONFIG_TARGET_USE_OLDCDT
obj-$(CONFIG_OPLUS_FEATURE_OPROJECT) += oplus_project.o
else
obj-$(CONFIG_OPLUS_FEATURE_OPROJECT) += oplus_project_oldcdt.o
endif