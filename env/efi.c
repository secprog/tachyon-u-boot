// SPDX-License-Identifier: GPL-2.0+
/*
 * U-Boot environment backend using non-volatile UEFI variables.
 */

#define LOG_CATEGORY LOGC_ENV

#include <env.h>
#include <env_internal.h>
#include <efi_loader.h>
#include <efi_variable.h>
#include <errno.h>
#include <linux/stddef.h>
#include <asm/cache.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#define ENV_EFI_VARIABLE_ATTRS \
	(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
#define ENV_EFI_VARIABLE_GUID \
	EFI_GUID(0xbd234d7e, 0x840d, 0x4f60, 0xa9, 0xce, \
		 0x1d, 0x8d, 0x2a, 0x67, 0x86, 0x31)

static const efi_guid_t env_efi_guid = ENV_EFI_VARIABLE_GUID;

static int env_efi_init_variables(void)
{
	efi_status_t ret;

	ret = efi_disks_register();
	if (ret != EFI_SUCCESS)
		return -EIO;

	ret = efi_init_variables();
	if (ret != EFI_SUCCESS)
		return -EIO;

	return 0;
}

static int env_efi_load(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, CONFIG_ENV_SIZE);
	efi_uintn_t size = CONFIG_ENV_SIZE;
	efi_status_t ret;
	int err;

	err = env_efi_init_variables();
	if (err)
		goto err_env_relocate;

	ret = efi_get_variable_int(u"UBootEnv", &env_efi_guid, NULL, &size,
				   buf, NULL);
	if (ret == EFI_NOT_FOUND) {
		env_set_default(NULL, 0);
		return -ENOMSG;
	}
	if (ret == EFI_BUFFER_TOO_SMALL || size != CONFIG_ENV_SIZE) {
		env_set_default("bad EFI env size", 0);
		return -ENOMSG;
	}
	if (ret != EFI_SUCCESS)
		goto err_env_relocate;

	err = env_import(buf, 1, H_EXTERNAL);
	if (!err)
		gd->env_valid = ENV_VALID;

	return err;

err_env_relocate:
	env_set_default(NULL, 0);

	return -EIO;
}

static int env_efi_save(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	efi_status_t ret;
	int err;

	err = env_efi_init_variables();
	if (err)
		return err;

	err = env_export(env_new);
	if (err)
		return err;

	ret = efi_set_variable_int(u"UBootEnv", &env_efi_guid,
				   ENV_EFI_VARIABLE_ATTRS, sizeof(*env_new),
				   env_new, true);
	if (ret != EFI_SUCCESS)
		return -EIO;

	ret = efi_var_to_file();
	if (ret != EFI_SUCCESS)
		return -EIO;

	gd->env_valid = ENV_VALID;

	return 0;
}

static int env_efi_erase(void)
{
	efi_status_t ret;
	int err;

	err = env_efi_init_variables();
	if (err)
		return err;

	ret = efi_set_variable_int(u"UBootEnv", &env_efi_guid, 0, 0, NULL,
				   false);
	if (ret != EFI_SUCCESS && ret != EFI_NOT_FOUND)
		return -EIO;

	ret = efi_var_to_file();
	if (ret != EFI_SUCCESS)
		return -EIO;

	gd->env_valid = ENV_INVALID;

	return 0;
}

U_BOOT_ENV_LOCATION(efi) = {
	.location	= ENVL_EFI,
	ENV_NAME("UEFI")
	.load		= env_efi_load,
	.save		= ENV_SAVE_PTR(env_efi_save),
	.erase		= ENV_ERASE_PTR(env_efi_erase),
};
