// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018, The Linux Foundation. All rights reserved.
// Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.

#include <dm.h>
#include <power-domain.h>
#include <power-domain-uclass.h>
#include <soc/qcom/cmd-db.h>
#include <soc/qcom/rpmh.h>
#include <dt-bindings/power/qcom-rpmpd.h>
#include <dm/device_compat.h>
#include <linux/err.h>
#include <linux/errno.h>

#define RPMH_ARC_MAX_LEVELS	16
#define RPMHPD_CORNER_MAX	((unsigned int)-1)

struct rpmhpd {
	struct udevice *dev;
	unsigned int enable_corner;
	u32 level[RPMH_ARC_MAX_LEVELS];
	size_t level_count;
	bool enabled;
	const char *res_name;
	u32 addr;
	bool skip_retention_level;
};

struct rpmhpd_desc {
	struct rpmhpd **rpmhpds;
	size_t num_pds;
};

static struct rpmhpd lcx = {
	.res_name = "lcx.lvl",
	.enable_corner = RPMHPD_CORNER_MAX,
};

static struct rpmhpd lmx = {
	.res_name = "lmx.lvl",
	.enable_corner = RPMHPD_CORNER_MAX,
};

static struct rpmhpd mmcx_ao;
static struct rpmhpd mmcx = {
	.res_name = "mmcx.lvl",
};

static struct rpmhpd mmcx_ao = {
	.res_name = "mmcx.lvl",
};

static struct rpmhpd *sa8775p_rpmhpds[] = {
	[SA8775P_MMCX] = &mmcx,
	[SA8775P_MMCX_AO] = &mmcx_ao,
};

static const struct rpmhpd_desc sa8775p_desc = {
	.rpmhpds = sa8775p_rpmhpds,
	.num_pds = ARRAY_SIZE(sa8775p_rpmhpds),
};

static struct rpmhpd *sc7280_rpmhpds[] = {
	[SC7280_LMX] = &lmx,
	[SC7280_LCX] = &lcx,
};

static const struct rpmhpd_desc sc7280_desc = {
	.rpmhpds = sc7280_rpmhpds,
	.num_pds = ARRAY_SIZE(sc7280_rpmhpds),
};

static struct rpmhpd *stub_rpmhpds[] = {};

static const struct rpmhpd_desc stub_desc = {
	.rpmhpds = stub_rpmhpds,
	.num_pds = ARRAY_SIZE(stub_rpmhpds),
};

static int rpmhpd_send_corner(struct rpmhpd *pd, enum rpmh_state state,
			      unsigned int corner)
{
	struct tcs_cmd cmd = {
		.addr = pd->addr,
		.data = corner,
	};

	return rpmh_write(pd->dev, state, &cmd, 1);
}

static int rpmhpd_power_on(struct power_domain *pd)
{
	const struct rpmhpd_desc *desc;
	struct rpmhpd *curr;
	int ret;

	desc = (const struct rpmhpd_desc *)dev_get_driver_data(pd->dev);
	if (!desc)
		return -EINVAL;

	if (pd->id >= desc->num_pds)
		return -EINVAL;

	curr = desc->rpmhpds[pd->id];
	if (!curr) {
		log_warning("Power domain id (%ld) not supported\n", pd->id);
		return 0;
	}

	ret = rpmhpd_send_corner(curr, RPMH_ACTIVE_ONLY_STATE,
				 curr->enable_corner);
	log_warning("qcom-rpmhpd: power_on id=%ld res=%s corner=%u ret=%d\n",
		    pd->id, curr->res_name, curr->enable_corner, ret);
	if (!ret)
		curr->enabled = true;

	return ret;
}

static int rpmhpd_power_off(struct power_domain *pd)
{
	const struct rpmhpd_desc *desc;
	struct rpmhpd *curr;
	int ret;

	desc = (const struct rpmhpd_desc *)dev_get_driver_data(pd->dev);
	if (!desc)
		return -EINVAL;

	if (pd->id >= desc->num_pds)
		return -EINVAL;

	curr = desc->rpmhpds[pd->id];
	if (!curr) {
		log_warning("Power domain id (%ld) not supported\n", pd->id);
		return 0;
	}

	ret = rpmhpd_send_corner(curr, RPMH_ACTIVE_ONLY_STATE, 0);
	if (!ret)
		curr->enabled = false;

	return ret;
}

static int rpmhpd_update_level_mapping(struct rpmhpd *rpmhpd)
{
	const u16 *buf;
	bool use_max_corner = rpmhpd->enable_corner == RPMHPD_CORNER_MAX;
	int i;

	buf = cmd_db_read_aux_data(rpmhpd->res_name, &rpmhpd->level_count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	rpmhpd->level_count >>= 1;
	if (!rpmhpd->level_count || rpmhpd->level_count > RPMH_ARC_MAX_LEVELS)
		return -EINVAL;

	if (use_max_corner)
		rpmhpd->enable_corner = 0;

	for (i = 0; i < rpmhpd->level_count; i++) {
		if (rpmhpd->skip_retention_level &&
		    buf[i] == RPMH_REGULATOR_LEVEL_RETENTION)
			continue;

		rpmhpd->level[i] = buf[i];

		if (!use_max_corner && !rpmhpd->level[rpmhpd->enable_corner] &&
		    rpmhpd->level[i])
			rpmhpd->enable_corner = i;

		if (i > 0 && !rpmhpd->level[i]) {
			rpmhpd->level_count = i;
			break;
		}
	}

	if (use_max_corner)
		rpmhpd->enable_corner = rpmhpd->level_count - 1;

	return 0;
}

static int rpmhpd_probe(struct udevice *dev)
{
	const struct rpmhpd_desc *desc;
	struct rpmhpd *pd;
	int ret;
	int i;

	desc = (const struct rpmhpd_desc *)dev_get_driver_data(dev);
	if (!desc)
		return -EINVAL;

	for (i = 0; i < desc->num_pds; i++) {
		pd = desc->rpmhpds[i];
		if (!pd)
			continue;

		pd->dev = dev;
		pd->addr = cmd_db_read_addr(pd->res_name);
		if (!pd->addr) {
			dev_err(dev, "Could not find RPMh address for resource %s\n",
				pd->res_name);
			return -ENODEV;
		}

		ret = cmd_db_read_slave_id(pd->res_name);
		if (ret != CMD_DB_HW_ARC) {
			dev_err(dev, "RPMh slave ID mismatch for %s\n",
				pd->res_name);
			return -EINVAL;
		}

		ret = rpmhpd_update_level_mapping(pd);
		if (ret)
			return ret;

		log_warning("qcom-rpmhpd: %s addr=%#x enable_corner=%u levels=%zu\n",
			    pd->res_name, pd->addr, pd->enable_corner,
			    pd->level_count);
	}

	return 0;
}

static const struct power_domain_ops qcom_rpmhpd_power_ops = {
	.on = rpmhpd_power_on,
	.off = rpmhpd_power_off,
};

static const struct udevice_id rpmhpd_match_table[] = {
	{ .compatible = "qcom,sa8775p-rpmhpd", .data = (ulong)&sa8775p_desc },
	{ .compatible = "qcom,sc7280-rpmhpd", .data = (ulong)&sc7280_desc },
	{ .compatible = "qcom,qcs615-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,qcs8300-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,qdu1000-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sa8155p-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sa8540p-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sc7180-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sc8180x-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sc8280xp-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sdm670-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sdm845-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sm6350-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sm8150-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sm8250-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sm8350-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sm8450-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sm8550-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,sm8650-rpmhpd", .data = (ulong)&stub_desc },
	{ .compatible = "qcom,x1e80100-rpmhpd", .data = (ulong)&stub_desc },
	{ }
};

U_BOOT_DRIVER(qcom_rpmhpd_drv) = {
	.name = "qcom_rpmhpd_drv",
	.id = UCLASS_POWER_DOMAIN,
	.of_match = rpmhpd_match_table,
	.probe = rpmhpd_probe,
	.ops = &qcom_rpmhpd_power_ops,
};
