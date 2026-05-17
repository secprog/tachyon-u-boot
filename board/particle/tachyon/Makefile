# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2025 Particle Industries, Inc.

obj-y	+= tachyon.o
obj-y	+= qcom_dram.o
obj-y	+= efs.o
obj-y	+= efs_impl.o
obj-$(CONFIG_ACPI) += acpi.o

# DSDT is compiled from dsdt.asl by U-Boot's build system (see scripts/Makefile.lib)
# The ASL is preprocessed with cpp, compiled by iasl, and produces dsdt_generated.c
# which contains AmlCode[] - this links with lib/acpi/dsdt.c at runtime.
obj-$(CONFIG_GENERATE_ACPI_TABLE) += dsdt_generated.o
