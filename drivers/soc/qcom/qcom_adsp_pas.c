// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal Qualcomm ADSP PAS bootstrap for early PMIC-GLINK.
 *
 * This follows the Linux qcom_q6v5_pas + mdt_loader sequence closely enough
 * to start the ADSP/LPASS firmware before the PMIC-GLINK SMEM edge is used.
 */

#include <blk.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <dm/read.h>
#include <dm/uclass.h>
#include <elf.h>
#include <errno.h>
#include <fs.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <memalign.h>
#include <part.h>
#include <power-domain.h>
#include <smem.h>
#include <string.h>
#include <scsi.h>
#include <time.h>
#include <ufs.h>
#include <asm-generic/global_data.h>
#include <soc/qcom/qcom_adsp_pas.h>
#include <linux/arm-smccc.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

#define QCOM_ADSP_PAS_ID			1
#define QCOM_ADSP_COMPAT			"qcom,sc7280-adsp-pas"
#define QCOM_ADSP_DEFAULT_FW			"adsp.mdt"
#define QCOM_MODEM_PARTITION			"modem_a"
#define QCOM_ADSP_START_TIMEOUT_MS		5000
#define QPAS_MAX_POWER_DOMAINS			4
#define QPAS_SMP2P_MAX_ENTRY			16
#define QPAS_SMP2P_MAX_ENTRY_NAME		16
#define QPAS_SMP2P_MAGIC			0x504d5324
#define QPAS_SMP2P_VERSION			1

#define QCOM_MDT_TYPE_MASK			(7 << 24)
#define QCOM_MDT_TYPE_HASH			(2 << 24)
#define QCOM_MDT_RELOCATABLE			BIT(27)

#define QCOM_SCM_MAX_ARGS			10
#define QCOM_SCM_MAX_RETS			3
#define QCOM_SCM_SVC_PIL			0x02
#define QCOM_SCM_PIL_PAS_INIT_IMAGE		0x01
#define QCOM_SCM_PIL_PAS_MEM_SETUP		0x02
#define QCOM_SCM_PIL_PAS_AUTH_AND_RESET		0x05
#define QCOM_SCM_PIL_PAS_SHUTDOWN		0x06
#define QCOM_SCM_V2_EBUSY			-12
#define QCOM_SCM_ENOMEM			-5
#define QCOM_SCM_EOPNOTSUPP			-4
#define QCOM_SCM_EINVAL_ADDR			-3
#define QCOM_SCM_EINVAL_ARG			-2
#define QCOM_SCM_ERROR				-1
#define QCOM_SCM_INTERRUPTED			1
#define QCOM_SCM_EBUSY_WAIT_MS			30
#define QCOM_SCM_EBUSY_MAX_RETRY		20

/*
 * DEBUG: Set to 1 to skip SCM auth_and_reset + SMP2P wait entirely.
 * Use this to verify the board stays alive without starting the ADSP.
 * If the board still powers off, the issue is NOT the ADSP firmware itself
 * but rather power domains / memory-region mapping / earlier steps.
 * Set back to 0 for normal operation.
 */
#define QCOM_ADSP_SKIP_BOOT			0

#define SCM_SMC_FNID(s, c)			((((s) & 0xff) << 8) | ((c) & 0xff))

#define QCOM_SCM_VAL				0
#define QCOM_SCM_RO				1
#define QCOM_SCM_RW				2
#define QCOM_SCM_ARGS_IMPL(num, a, b, c, d, e, f, g, h, i, j, ...) \
	((((a) & 0x3) << 4) | (((b) & 0x3) << 6) | \
	 (((c) & 0x3) << 8) | (((d) & 0x3) << 10) | \
	 (((e) & 0x3) << 12) | (((f) & 0x3) << 14) | \
	 (((g) & 0x3) << 16) | (((h) & 0x3) << 18) | \
	 (((i) & 0x3) << 20) | (((j) & 0x3) << 22) | \
	 ((num) & 0xf))
#define QCOM_SCM_ARGS(...) \
	QCOM_SCM_ARGS_IMPL(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

struct qcom_scm_desc {
	u32 svc;
	u32 cmd;
	u32 arginfo;
	u64 args[QCOM_SCM_MAX_ARGS];
};

struct qcom_scm_res {
	u64 result[QCOM_SCM_MAX_RETS];
};

struct qpas_fw {
	void *data;
	size_t size;
	struct blk_desc *desc;
	int part;
	char path[128];
};

struct qpas_power_domains {
	struct power_domain pd[QPAS_MAX_POWER_DOMAINS];
	int count;
	int enabled;
};

struct qpas_smp2p_info {
	ofnode node;
	ofnode inbound;
	u32 remote_pid;
	u32 inbound_item;
	u32 fatal_bit;
	u32 ready_bit;
	char entry_name[QPAS_SMP2P_MAX_ENTRY_NAME];
};

struct qpas_smp2p_smem_item {
	__le32 magic;
	u8 version;
	u8 features[3];
	__le16 local_pid;
	__le16 remote_pid;
	__le16 total_entries;
	__le16 valid_entries;
	__le32 flags;

	struct {
		u8 name[QPAS_SMP2P_MAX_ENTRY_NAME];
		__le32 value;
	} entries[QPAS_SMP2P_MAX_ENTRY];
};

static bool qpas_booted;

static int qpas_enable_clocks(struct udevice *dev)
{
	struct clk_bulk clocks;
	int ret;

	ret = clk_get_bulk(dev, &clocks);
	if (ret) {
		log_warning("qcom-adsp-pas: clk_get_bulk ret=%d; XO may be unmanaged in U-Boot\n",
			    ret);
		return 0;
	}

	ret = clk_enable_bulk(&clocks);
	log_warning("qcom-adsp-pas: clk_enable_bulk ret=%d count=%d\n",
		    ret, clocks.count);
	return ret;
}

static int qpas_enable_power_domains(struct udevice *dev,
				     struct qpas_power_domains *pds)
{
	int count;
	int ret;
	int i;

	count = dev_count_phandle_with_args(dev, "power-domains",
					    "#power-domain-cells", 0);
	if (count == -ENOENT) {
		log_warning("qcom-adsp-pas: missing required power-domains in DT\n");
		return -ENODEV;
	}

	if (count < 0) {
		log_warning("qcom-adsp-pas: power-domain count ret=%d\n",
			    count);
		return count;
	}

	if (!count) {
		log_warning("qcom-adsp-pas: empty required power-domains list\n");
		return -ENODEV;
	}

	if (count > QPAS_MAX_POWER_DOMAINS) {
		log_warning("qcom-adsp-pas: too many power domains count=%d max=%d\n",
			    count, QPAS_MAX_POWER_DOMAINS);
		return -E2BIG;
	}

	pds->count = count;

	for (i = 0; i < count; i++) {
		ret = power_domain_get_by_index(dev, &pds->pd[i], i);
		if (ret) {
			log_warning("qcom-adsp-pas: power_domain_get index=%d ret=%d\n",
				    i, ret);
			goto err_off;
		}

		ret = power_domain_on(&pds->pd[i]);
		log_warning("qcom-adsp-pas: power_domain_on index=%d id=%lu ret=%d\n",
			    i, pds->pd[i].id, ret);
		if (ret)
			goto err_off;

		pds->enabled++;
	}

	return 0;

err_off:
	while (pds->enabled > 0) {
		pds->enabled--;
		power_domain_off(&pds->pd[pds->enabled]);
	}

	return ret;
}

static void qpas_disable_power_domains(struct qpas_power_domains *pds)
{
	int ret;

	while (pds->enabled > 0) {
		pds->enabled--;
		ret = power_domain_off(&pds->pd[pds->enabled]);
		log_warning("qcom-adsp-pas: power_domain_off index=%d id=%lu ret=%d\n",
			    pds->enabled, pds->pd[pds->enabled].id, ret);
	}
}

static int qpas_get_remote_pid(ofnode node, u32 *remote_pid)
{
	ofnode child;
	int ret;

	ofnode_for_each_subnode(child, node) {
		ret = ofnode_read_u32(child, "qcom,remote-pid", remote_pid);
		if (!ret)
			return 0;
	}

	return -ENOENT;
}

static int qpas_find_smp2p_irq_bits(ofnode node, struct qpas_smp2p_info *info)
{
	struct ofnode_phandle_args args;
	const char *name;
	int count;
	int ret;
	int i;

	count = ofnode_read_string_count(node, "interrupt-names");
	if (count < 0)
		return count;

	info->fatal_bit = U32_MAX;
	info->ready_bit = U32_MAX;

	for (i = 0; i < count; i++) {
		ret = ofnode_read_string_index(node, "interrupt-names", i,
					       &name);
		if (ret)
			return ret;

		if (strcmp(name, "fatal") && strcmp(name, "ready"))
			continue;

		ret = ofnode_parse_phandle_with_args(node,
						     "interrupts-extended",
						     "#interrupt-cells", 0,
						     i, &args);
		if (ret)
			return ret;

		if (!ofnode_equal(args.node, info->inbound))
			continue;

		if (args.args_count < 1 || args.args[0] >= 32)
			return -EINVAL;

		if (!strcmp(name, "fatal"))
			info->fatal_bit = args.args[0];
		else
			info->ready_bit = args.args[0];
	}

	if (info->fatal_bit == U32_MAX || info->ready_bit == U32_MAX)
		return -ENOENT;

	return 0;
}

static int qpas_find_smp2p(ofnode node, struct qpas_smp2p_info *info)
{
	const char *entry_name;
	ofnode child;
	ofnode smp2p;
	u32 smem[2];
	u32 remote_pid;
	int ret;

	ret = qpas_get_remote_pid(node, &remote_pid);
	if (ret) {
		log_warning("qcom-adsp-pas: SMP2P remote-pid lookup ret=%d\n",
			    ret);
		return ret;
	}

	ofnode_for_each_compatible_node(smp2p, "qcom,smp2p") {
		ret = ofnode_read_u32(smp2p, "qcom,remote-pid",
				      &info->remote_pid);
		if (ret || info->remote_pid != remote_pid)
			continue;

		ret = ofnode_read_u32_array(smp2p, "qcom,smem", smem,
					    ARRAY_SIZE(smem));
		if (ret)
			return ret;

		ofnode_for_each_subnode(child, smp2p) {
			if (!ofnode_read_bool(child, "interrupt-controller"))
				continue;

			entry_name = ofnode_read_string(child,
							"qcom,entry-name");
			if (!entry_name)
				continue;

			info->node = smp2p;
			info->inbound = child;
			info->inbound_item = smem[1];
			strncpy(info->entry_name, entry_name,
				sizeof(info->entry_name));
			info->entry_name[sizeof(info->entry_name) - 1] = '\0';

			ret = qpas_find_smp2p_irq_bits(node, info);
			if (ret)
				return ret;

			log_warning("qcom-adsp-pas: SMP2P remote_pid=%u inbound_item=%u entry='%s' fatal_bit=%u ready_bit=%u\n",
				    info->remote_pid, info->inbound_item,
				    info->entry_name, info->fatal_bit,
				    info->ready_bit);
			return 0;
		}
	}

	return -ENOENT;
}

static bool qpas_smp2p_entry_name_eq(const u8 name[QPAS_SMP2P_MAX_ENTRY_NAME],
				     const char *want)
{
	char buf[QPAS_SMP2P_MAX_ENTRY_NAME + 1];

	memcpy(buf, name, QPAS_SMP2P_MAX_ENTRY_NAME);
	buf[QPAS_SMP2P_MAX_ENTRY_NAME] = '\0';

	return !strncmp(buf, want, QPAS_SMP2P_MAX_ENTRY_NAME);
}

static int qpas_smp2p_read_entry(struct udevice *smem,
				 const struct qpas_smp2p_info *info,
				 u32 *value)
{
	struct qpas_smp2p_smem_item *item;
	size_t size = 0;
	u16 total;
	u16 valid;
	u32 magic;
	int i;

	item = smem_get(smem, info->remote_pid, info->inbound_item, &size);
	if (IS_ERR_OR_NULL(item))
		return -EAGAIN;

	if (size < sizeof(*item))
		return -EINVAL;

	magic = le32_to_cpu(item->magic);
	if (magic != QPAS_SMP2P_MAGIC || item->version != QPAS_SMP2P_VERSION)
		return -EAGAIN;

	total = le16_to_cpu(item->total_entries);
	valid = le16_to_cpu(item->valid_entries);
	if (total > QPAS_SMP2P_MAX_ENTRY || valid > total)
		return -EINVAL;

	for (i = 0; i < valid; i++) {
		if (!qpas_smp2p_entry_name_eq(item->entries[i].name,
					      info->entry_name))
			continue;

		*value = le32_to_cpu(item->entries[i].value);
		return 0;
	}

	return -EAGAIN;
}

static int qpas_wait_for_start(ofnode node)
{
	struct qpas_smp2p_info info = {};
	struct udevice *smem;
	ulong start;
	u32 last_value = U32_MAX;
	u32 value;
	bool pending_logged = false;
	ulong last_heartbeat = 0;
	int ret;

	ret = qpas_find_smp2p(node, &info);
	if (ret) {
		log_warning("qcom-adsp-pas: SMP2P discovery ret=%d\n", ret);
		return ret;
	}

	ret = uclass_first_device_err(UCLASS_SMEM, &smem);
	if (ret) {
		log_warning("qcom-adsp-pas: SMP2P smem lookup ret=%d\n", ret);
		return ret;
	}

	log_warning("qcom-adsp-pas: waiting ADSP SMP2P ready (remote_pid=%u) timeout=%d ms\n",
		    info.remote_pid, QCOM_ADSP_START_TIMEOUT_MS);

	start = get_timer(0);
	do {
		ret = qpas_smp2p_read_entry(smem, &info, &value);
		if (ret == -EAGAIN) {
			if (!pending_logged) {
				log_warning("qcom-adsp-pas: waiting for SMP2P entry '%s' (elapsed=%lu ms)\n",
					    info.entry_name, get_timer(start));
				pending_logged = true;
			}
			mdelay(10);
			continue;
		}
		if (ret) {
			log_warning("qcom-adsp-pas: SMP2P read ret=%d\n", ret);
			return ret;
		}

		if (value != last_value) {
			log_warning("qcom-adsp-pas: SMP2P entry '%s' value=%08x\n",
				    info.entry_name, value);
			last_value = value;
		}

		if (value & BIT(info.fatal_bit)) {
			log_warning("qcom-adsp-pas: ADSP fatal asserted value=%08x bit=%u\n",
				    value, info.fatal_bit);
			return -EIO;
		}

		if (value & BIT(info.ready_bit)) {
			log_warning("qcom-adsp-pas: ADSP ready asserted value=%08x bit=%u (elapsed=%lu ms)\n",
				    value, info.ready_bit, get_timer(start));
			return 0;
		}

		/* Periodic heartbeat to track how long board survives */
		if (get_timer(last_heartbeat) >= 500) {
			log_warning("qcom-adsp-pas: SMP2P heartbeat elapsed=%lu ms value=%08x\n",
				    get_timer(start), value);
			last_heartbeat = get_timer(0);
		}

		mdelay(10);
	} while (get_timer(start) < QCOM_ADSP_START_TIMEOUT_MS);

	log_warning("qcom-adsp-pas: ADSP SMP2P ready timeout\n");
	return -ETIMEDOUT;
}

static int qcom_scm_remap_error(long err)
{
	switch (err) {
	case QCOM_SCM_ERROR:
		return -EIO;
	case QCOM_SCM_EINVAL_ADDR:
	case QCOM_SCM_EINVAL_ARG:
		return -EINVAL;
	case QCOM_SCM_EOPNOTSUPP:
		return -EOPNOTSUPP;
	case QCOM_SCM_ENOMEM:
		return -ENOMEM;
	case QCOM_SCM_V2_EBUSY:
		return -EBUSY;
	}

	return -EINVAL;
}

static int qcom_scm_call(const struct qcom_scm_desc *desc,
			 struct qcom_scm_res *out)
{
	struct arm_smccc_quirk quirk = { .id = ARM_SMCCC_QUIRK_QCOM_A6 };
	struct arm_smccc_res res;
	unsigned long a0;
	int retry = 0;

	a0 = ARM_SMCCC_CALL_VAL(ARM_SMCCC_STD_CALL, ARM_SMCCC_SMC_64,
				ARM_SMCCC_OWNER_SIP,
				SCM_SMC_FNID(desc->svc, desc->cmd));

	do {
		quirk.state.a6 = 0;

		do {
			arm_smccc_smc_quirk(a0, desc->arginfo,
					    desc->args[0], desc->args[1],
					    desc->args[2], desc->args[3],
					    quirk.state.a6, 0, &res, &quirk);
			if (res.a0 == QCOM_SCM_INTERRUPTED)
				a0 = res.a0;
		} while (res.a0 == QCOM_SCM_INTERRUPTED);

		if (res.a0 != QCOM_SCM_V2_EBUSY)
			break;

		if (retry++ >= QCOM_SCM_EBUSY_MAX_RETRY)
			break;

		mdelay(QCOM_SCM_EBUSY_WAIT_MS);
	} while (1);

	if (out) {
		out->result[0] = res.a1;
		out->result[1] = res.a2;
		out->result[2] = res.a3;
	}

	return res.a0 ? qcom_scm_remap_error(res.a0) : 0;
}

static int qcom_scm_pas_init_image(u32 pas_id, const void *metadata,
				   size_t size, void **ctxp)
{
	struct qcom_scm_desc desc = {
		.svc = QCOM_SCM_SVC_PIL,
		.cmd = QCOM_SCM_PIL_PAS_INIT_IMAGE,
		.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_RW),
		.args[0] = pas_id,
	};
	struct qcom_scm_res res;
	void *mdata;
	phys_addr_t phys;
	size_t aligned;
	int ret;

	aligned = ALIGN(size, SZ_4K);
	mdata = memalign(SZ_4K, aligned);
	if (!mdata)
		return -ENOMEM;

	memcpy(mdata, metadata, size);
	flush_cache(rounddown((ulong)mdata, ARCH_DMA_MINALIGN),
		    roundup((ulong)mdata + aligned, ARCH_DMA_MINALIGN) -
		    rounddown((ulong)mdata, ARCH_DMA_MINALIGN));

	phys = map_to_sysmem(mdata);
	desc.args[1] = phys;

	log_warning("qcom-adsp-pas: init_image pas_id=%u metadata=%p phys=%llx size=%zu\n",
		    pas_id, mdata, (unsigned long long)phys, size);

	ret = qcom_scm_call(&desc, &res);
	if (ret || res.result[0]) {
		log_warning("qcom-adsp-pas: init_image ret=%d scm_result=%llu\n",
			    ret, res.result[0]);
		free(mdata);
		return ret ? ret : (int)res.result[0];
	}

	*ctxp = mdata;
	return 0;
}

static int qcom_scm_pas_mem_setup(u32 pas_id, phys_addr_t addr, size_t size)
{
	struct qcom_scm_desc desc = {
		.svc = QCOM_SCM_SVC_PIL,
		.cmd = QCOM_SCM_PIL_PAS_MEM_SETUP,
		.arginfo = QCOM_SCM_ARGS(3),
		.args[0] = pas_id,
		.args[1] = addr,
		.args[2] = size,
	};
	struct qcom_scm_res res;
	int ret;

	log_warning("qcom-adsp-pas: mem_setup pas_id=%u addr=%llx size=%zu\n",
		    pas_id, (unsigned long long)addr, size);

	ret = qcom_scm_call(&desc, &res);
	if (ret || res.result[0])
		log_warning("qcom-adsp-pas: mem_setup ret=%d scm_result=%llu\n",
			    ret, res.result[0]);

	return ret ? ret : (int)res.result[0];
}

static int qcom_scm_pas_auth_and_reset(u32 pas_id)
{
	struct qcom_scm_desc desc = {
		.svc = QCOM_SCM_SVC_PIL,
		.cmd = QCOM_SCM_PIL_PAS_AUTH_AND_RESET,
		.arginfo = QCOM_SCM_ARGS(1),
		.args[0] = pas_id,
	};
	struct qcom_scm_res res;
	int ret;

	log_warning("qcom-adsp-pas: auth_and_reset pas_id=%u\n", pas_id);

	ret = qcom_scm_call(&desc, &res);
	if (ret || res.result[0])
		log_warning("qcom-adsp-pas: auth_and_reset ret=%d scm_result=%llu\n",
			    ret, res.result[0]);

	return ret ? ret : (int)res.result[0];
}

static int qcom_scm_pas_shutdown(u32 pas_id)
{
	struct qcom_scm_desc desc = {
		.svc = QCOM_SCM_SVC_PIL,
		.cmd = QCOM_SCM_PIL_PAS_SHUTDOWN,
		.arginfo = QCOM_SCM_ARGS(1),
		.args[0] = pas_id,
	};
	struct qcom_scm_res res;
	int ret;

	log_warning("qcom-adsp-pas: shutdown pas_id=%u\n", pas_id);

	ret = qcom_scm_call(&desc, &res);
	if (ret || res.result[0])
		log_warning("qcom-adsp-pas: shutdown ret=%d scm_result=%llu\n",
			    ret, res.result[0]);

	return ret ? ret : (int)res.result[0];
}

static void qpas_prepare_storage(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_UFS)) {
		ret = ufs_probe();
		log_warning("qcom-adsp-pas: ufs_probe ret=%d\n", ret);
	}

	if (IS_ENABLED(CONFIG_SCSI)) {
		ret = scsi_scan(false);
		log_warning("qcom-adsp-pas: scsi_scan ret=%d\n", ret);
	}
}

static int qpas_find_modem_partition(struct blk_desc **descp, int *partp)
{
	struct disk_partition info;
	struct udevice *dev;
	struct blk_desc *desc;
	int part;
	
	qpas_prepare_storage();
	
	uclass_foreach_dev_probe(UCLASS_BLK, dev) {
		desc = dev_get_uclass_plat(dev);
		if (!desc) {
			log_warning("qcom-adsp-pas: blk dev=%s has no desc\n",
				    dev->name);
			continue;
		}

		log_warning("qcom-adsp-pas: checking blk dev=%s devnum=%d hwpart=%d uclass_id=%d part_type=%d lba=%llu blksz=%lu\n",
			    dev->name,
			    desc->devnum,
			    desc->hwpart,
			    desc->uclass_id,
			    desc->part_type,
			    (u64)desc->lba,
			    (ulong)desc->blksz);

		if (desc->part_type == PART_TYPE_UNKNOWN)
			log_warning("qcom-adsp-pas: blk dev=%s devnum=%d has PART_TYPE_UNKNOWN; still trying %s\n",
				    dev->name, desc->devnum, QCOM_MODEM_PARTITION);

		part = part_get_info_by_name(desc, QCOM_MODEM_PARTITION, &info);
		log_warning("qcom-adsp-pas: part_get_info_by_name dev=%s devnum=%d name=%s ret=%d\n",
			    dev->name, desc->devnum, QCOM_MODEM_PARTITION, part);

		if (part >= 0) {
			*descp = desc;
			*partp = part;
			log_warning("qcom-adsp-pas: found %s on blk dev=%s devnum=%d hwpart=%d part=%d start=%llu size=%llu\n",
				    QCOM_MODEM_PARTITION,
				    dev->name,
				    desc->devnum,
				    desc->hwpart,
				    part,
				    (u64)info.start,
				    (u64)info.size);
			return 0;
		}
	}

	log_warning("qcom-adsp-pas: %s partition not found\n",
		    QCOM_MODEM_PARTITION);
	return -ENOENT;
}

static int qpas_read_file_exact(struct blk_desc *desc, int part,
				const char *path, void **bufp, size_t *sizep)
{
	void *buf;
	loff_t size;
	loff_t read;
	int ret;

	log_warning("qcom-adsp-pas: fs select devnum=%d hwpart=%d part=%d path=%s\n",
		    desc->devnum, desc->hwpart, part, path);

	ret = fs_set_blk_dev_with_part(desc, part);
	log_warning("qcom-adsp-pas: fs_set_blk_dev_with_part ret=%d\n", ret);
	if (ret)
		return ret;

	ret = fs_size(path, &size);
	log_warning("qcom-adsp-pas: fs_size path=%s ret=%d size=%llu\n",
			path, ret, (u64)size);
	if (ret)
		return ret;

	/*
	* fs_size() may close the current filesystem context, so select the
	* block device/partition again before fs_read().
	*/
	ret = fs_set_blk_dev_with_part(desc, part);
	log_warning("qcom-adsp-pas: fs_set_blk_dev_with_part before read ret=%d\n",
			ret);
	if (ret)
		return ret;

	buf = memalign(ARCH_DMA_MINALIGN,
			ALIGN(size + 1, ARCH_DMA_MINALIGN));
	if (!buf)
		return -ENOMEM;

	read = 0;
	ret = fs_read(path, (ulong)buf, 0, size, &read);
	log_warning("qcom-adsp-pas: fs_read path=%s ret=%d read=%llu expected=%llu\n",
		    path, ret, (u64)read, (u64)size);
	if (ret || read != size) {
		free(buf);
		return -EIO;
	}

	((u8 *)buf)[size] = 0;
	*bufp = buf;
	*sizep = size;
	return 0;
}

static const char *qpas_basename(const char *path)
{
	const char *slash;

	slash = strrchr(path, '/');

	return slash ? slash + 1 : path;
}

static int qpas_read_firmware(struct qpas_fw *fw, const char *dt_fw_name)
{
	struct blk_desc *desc;
	const char *base;
	char candidate[3][128];
	int part;
	int ret;
	int i;

	ret = qpas_find_modem_partition(&desc, &part);
	if (ret)
		return ret;

	base = qpas_basename(dt_fw_name);
	snprintf(candidate[0], sizeof(candidate[0]), "image/%s", base);
	snprintf(candidate[1], sizeof(candidate[1]), "%s", dt_fw_name);
	snprintf(candidate[2], sizeof(candidate[2]), "%s", base);

	for (i = 0; i < ARRAY_SIZE(candidate); i++) {

		ret = qpas_read_file_exact(desc, part, candidate[i],
					   &fw->data, &fw->size);
		log_warning("qcom-adsp-pas: read %s ret=%d size=%zu\n",
			    candidate[i], ret, ret ? 0 : fw->size);
		if (!ret) {
			fw->desc = desc;
			fw->part = part;
			snprintf(fw->path, sizeof(fw->path), "%s", candidate[i]);
			return 0;
		}
	}

	return ret;
}

static int qpas_read_segment(const struct qpas_fw *fw, unsigned int segment,
			     void *dst, size_t size)
{
	char seg_path[128];
	int len;
	int ret;
	void *buf;
	size_t read_size;

	if (!fw->desc)
		return -EINVAL;

	len = strlen(fw->path);
	if (len < 3 || len >= sizeof(seg_path))
		return -EINVAL;

	snprintf(seg_path, sizeof(seg_path), "%s", fw->path);
	snprintf(seg_path + len - 3, sizeof(seg_path) - len + 3,
		 "b%02u", segment);

	ret = qpas_read_file_exact(fw->desc, fw->part, seg_path, &buf,
				   &read_size);
	log_warning("qcom-adsp-pas: read segment %s ret=%d size=%zu expected=%zu\n",
		    seg_path, ret, ret ? 0 : read_size, size);
	if (ret)
		return ret;

	if (read_size != size) {
		free(buf);
		return -EINVAL;
	}

	memcpy(dst, buf, size);
	free(buf);
	return 0;
}

static bool qpas_mdt_header_valid(const struct qpas_fw *fw)
{
	const Elf32_Ehdr *ehdr;
	size_t phend;
	size_t shend;

	if (fw->size < sizeof(*ehdr))
		return false;

	ehdr = fw->data;
	if (!IS_ELF(*ehdr))
		return false;

	if (ehdr->e_phentsize != sizeof(Elf32_Phdr))
		return false;

	phend = ehdr->e_phoff + (size_t)ehdr->e_phnum * sizeof(Elf32_Phdr);
	if (phend > fw->size)
		return false;

	if (ehdr->e_shentsize || ehdr->e_shnum) {
		if (ehdr->e_shentsize != sizeof(Elf32_Shdr))
			return false;

		shend = ehdr->e_shoff + (size_t)ehdr->e_shnum * sizeof(Elf32_Shdr);
		if (shend > fw->size)
			return false;
	}

	return true;
}

static bool qpas_mdt_phdr_loadable(const Elf32_Phdr *phdr)
{
	if (phdr->p_type != PT_LOAD)
		return false;

	if ((phdr->p_flags & QCOM_MDT_TYPE_MASK) == QCOM_MDT_TYPE_HASH)
		return false;

	return phdr->p_memsz != 0;
}

static bool qpas_mdt_bins_are_split(const struct qpas_fw *fw)
{
	const Elf32_Ehdr *ehdr = fw->data;
	const u8 *data = fw->data;
	const Elf32_Phdr *phdrs = (const void *)(data + ehdr->e_phoff);
	u64 start;
	u64 end;
	int i;

	for (i = 0; i < ehdr->e_phnum; i++) {
		if (!phdrs[i].p_filesz)
			continue;

		start = phdrs[i].p_offset;
		end = start + phdrs[i].p_filesz;
		if (start > fw->size || end > fw->size)
			return true;
	}

	return false;
}

static int qpas_mdt_read_metadata(const struct qpas_fw *fw,
				  void **metap, size_t *metaszp)
{
	const Elf32_Ehdr *ehdr = fw->data;
	const u8 *data = fw->data;
	const Elf32_Phdr *phdrs = (const void *)(data + ehdr->e_phoff);
	unsigned int hash_segment = 0;
	size_t hash_offset;
	size_t hash_size;
	size_t ehdr_size;
	void *metadata;
	int ret;
	int i;

	if (ehdr->e_phnum < 2 || phdrs[0].p_type == PT_LOAD)
		return -EINVAL;

	for (i = 1; i < ehdr->e_phnum; i++) {
		if ((phdrs[i].p_flags & QCOM_MDT_TYPE_MASK) ==
		    QCOM_MDT_TYPE_HASH) {
			hash_segment = i;
			break;
		}
	}

	if (!hash_segment) {
		log_warning("qcom-adsp-pas: no hash segment in %s\n", fw->path);
		return -EINVAL;
	}

	ehdr_size = phdrs[0].p_filesz;
	hash_size = phdrs[hash_segment].p_filesz;
	if (ehdr_size > fw->size)
		return -EINVAL;

	metadata = malloc(ehdr_size + hash_size);
	if (!metadata)
		return -ENOMEM;

	memcpy(metadata, fw->data, ehdr_size);

	if (ehdr_size + hash_size == fw->size) {
		hash_offset = ehdr_size;
		memcpy((u8 *)metadata + ehdr_size, data + hash_offset, hash_size);
	} else if (phdrs[hash_segment].p_offset + hash_size <= fw->size) {
		hash_offset = phdrs[hash_segment].p_offset;
		memcpy((u8 *)metadata + ehdr_size, data + hash_offset, hash_size);
	} else {
		ret = qpas_read_segment(fw, hash_segment,
					(u8 *)metadata + ehdr_size, hash_size);
		if (ret) {
			free(metadata);
			return ret;
		}
	}

	*metap = metadata;
	*metaszp = ehdr_size + hash_size;
	return 0;
}

static int qpas_mdt_init_image(const struct qpas_fw *fw, phys_addr_t mem_phys,
			       void **metadata_ctxp)
{
	const Elf32_Ehdr *ehdr = fw->data;
	const u8 *data = fw->data;
	const Elf32_Phdr *phdrs = (const void *)(data + ehdr->e_phoff);
	phys_addr_t min_addr = (phys_addr_t)-1;
	phys_addr_t max_addr = 0;
	bool relocate = false;
	void *metadata;
	size_t metadata_len;
	int ret;
	int i;

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr *phdr = &phdrs[i];

		if (!qpas_mdt_phdr_loadable(phdr))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);
	}

	ret = qpas_mdt_read_metadata(fw, &metadata, &metadata_len);
	if (ret)
		return ret;

	ret = qcom_scm_pas_init_image(QCOM_ADSP_PAS_ID, metadata,
				      metadata_len, metadata_ctxp);
	free(metadata);
	if (ret)
		return ret;

	if (relocate)
		ret = qcom_scm_pas_mem_setup(QCOM_ADSP_PAS_ID, mem_phys,
					     max_addr - min_addr);

	return ret;
}

static int qpas_mdt_load_segments(const struct qpas_fw *fw, void *mem_region,
				  phys_addr_t mem_phys, size_t mem_size,
				  phys_addr_t *reloc_basep)
{
	const Elf32_Ehdr *ehdr = fw->data;
	const u8 *data = fw->data;
	const Elf32_Phdr *phdrs = (const void *)(data + ehdr->e_phoff);
	phys_addr_t min_addr = (phys_addr_t)-1;
	phys_addr_t mem_reloc;
	bool split;
	bool relocate = false;
	int ret = 0;
	int i;

	split = qpas_mdt_bins_are_split(fw);

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr *phdr = &phdrs[i];

		if (!qpas_mdt_phdr_loadable(phdr))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;
	}

	mem_reloc = relocate ? min_addr : mem_phys;

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr *phdr = &phdrs[i];
		ssize_t offset;
		void *ptr;

		if (!qpas_mdt_phdr_loadable(phdr))
			continue;

		if (phdr->p_filesz > phdr->p_memsz)
			return -EINVAL;

		if (phdr->p_paddr < mem_reloc) {
			log_warning("qcom-adsp-pas: segment %d below reloc base\n",
				    i);
			return -EINVAL;
		}

		offset = phdr->p_paddr - mem_reloc;
		if (offset + phdr->p_memsz > mem_size) {
			log_warning("qcom-adsp-pas: segment %d outside memory range\n",
				    i);
			return -EINVAL;
		}

		ptr = (u8 *)mem_region + offset;
		log_warning("qcom-adsp-pas: load seg=%d dst=%p paddr=%08x filesz=%u memsz=%u split=%d\n",
			    i, ptr, phdr->p_paddr, phdr->p_filesz,
			    phdr->p_memsz, split);

		if (phdr->p_filesz && !split) {
			if (phdr->p_offset + phdr->p_filesz > fw->size)
				return -EINVAL;
			memcpy(ptr, data + phdr->p_offset, phdr->p_filesz);
		} else if (phdr->p_filesz) {
			ret = qpas_read_segment(fw, i, ptr, phdr->p_filesz);
			if (ret)
				return ret;
		}

		if (phdr->p_memsz > phdr->p_filesz)
			memset((u8 *)ptr + phdr->p_filesz, 0,
			       phdr->p_memsz - phdr->p_filesz);

		flush_cache(rounddown((ulong)ptr, ARCH_DMA_MINALIGN),
			    roundup((ulong)ptr + phdr->p_memsz,
				    ARCH_DMA_MINALIGN) -
			    rounddown((ulong)ptr, ARCH_DMA_MINALIGN));
	}

	if (reloc_basep)
		*reloc_basep = mem_reloc;

	return 0;
}

static int qpas_get_memory_region(ofnode node, phys_addr_t *addrp,
				  size_t *sizep, void **vaddrp)
{
	struct ofnode_phandle_args args;
	struct resource res;
	void *vaddr;
	int ret;

	ret = ofnode_parse_phandle_with_args(node, "memory-region",
					     NULL, 0, 0, &args);
	if (ret)
		return ret;

	ret = ofnode_read_resource(args.node, 0, &res);
	if (ret)
		return ret;

	vaddr = map_sysmem(res.start, resource_size(&res));
	if (!vaddr)
		return -ENOMEM;

	log_warning("qcom-adsp-pas: memory-region phys=%llx size=%llx vaddr=%p\n",
		    (unsigned long long)res.start,
		    (unsigned long long)resource_size(&res),
		    vaddr);

	*addrp = res.start;
	*sizep = resource_size(&res);
	*vaddrp = vaddr;
	return 0;
}

static int qcom_adsp_pas_boot_node(struct udevice *dev, ofnode node)
{
	struct qpas_power_domains pds = {};
	struct qpas_fw fw = {};
	const char *fw_name;
	phys_addr_t mem_phys;
	phys_addr_t reloc_base;
	void *mem_region;
	void *metadata_ctx = NULL;
	size_t mem_size;
	int ret;

	if (qpas_booted)
		return 0;

	/* Dump U-Boot memory layout to check for ADSP region overlap */
	log_warning("qcom-adsp-pas: U-Boot mem: ram_base=%llx ram_size=%llx ram_top=%llx\n",
		    (unsigned long long)gd->ram_base,
		    (unsigned long long)gd->ram_size,
		    (unsigned long long)gd->ram_top);
	log_warning("qcom-adsp-pas: U-Boot malloc: base=%llx limit=%x\n",
		    (unsigned long long)gd->malloc_base, gd->malloc_limit);

	if (dev) {
		ret = qpas_enable_clocks(dev);
		if (ret)
			return ret;
	} else {
		log_warning("qcom-adsp-pas: no bound device; skipping clk_get_bulk\n");
	}

	if (dev) {
		ret = qpas_enable_power_domains(dev, &pds);
		if (ret)
			return ret;
	} else {
		log_warning("qcom-adsp-pas: no bound device; skipping power domains\n");
	}

	ret = qpas_get_memory_region(node, &mem_phys, &mem_size, &mem_region);
	if (ret) {
		log_warning("qcom-adsp-pas: memory-region lookup failed ret=%d\n",
			    ret);
		goto out_power_domains;
	}

	fw_name = ofnode_read_string(node, "firmware-name");
	if (!fw_name)
		fw_name = QCOM_ADSP_DEFAULT_FW;

	log_warning("qcom-adsp-pas: boot start firmware=%s mem=%llx size=%zu\n",
		    fw_name, (unsigned long long)mem_phys, mem_size);

	/* Check if ADSP memory region overlaps U-Boot heap */
	if (mem_phys < gd->ram_top && (mem_phys + mem_size) > gd->malloc_base) {
		log_warning("qcom-adsp-pas: WARNING ADSP region [%llx-%llx] overlaps U-Boot memory!\n",
			    (unsigned long long)mem_phys,
			    (unsigned long long)(mem_phys + mem_size - 1));
	}

	/*
	 * Shut down any stale ADSP state left by ABL before loading
	 * firmware. This ensures clean power/cache/SMMU state even if
	 * the DSP wasn't fully booted by the previous stage.
	 */
	log_warning("qcom-adsp-pas: pre-boot shutdown pas_id=%u\n",
		    QCOM_ADSP_PAS_ID);
	qcom_scm_pas_shutdown(QCOM_ADSP_PAS_ID);
	mdelay(100);
	log_warning("qcom-adsp-pas: pre-boot shutdown complete\n");

	ret = qpas_read_firmware(&fw, fw_name);
	if (ret)
		goto out_unmap;

	if (!qpas_mdt_header_valid(&fw)) {
		log_warning("qcom-adsp-pas: invalid MDT header in %s\n", fw.path);
		ret = -EINVAL;
		goto out_free_fw;
	}

	ret = qpas_mdt_init_image(&fw, mem_phys, &metadata_ctx);
	if (ret)
		goto out_free_fw;

	ret = qpas_mdt_load_segments(&fw, mem_region, mem_phys, mem_size,
				     &reloc_base);
	if (ret)
		goto out_free_metadata;

	log_warning("qcom-adsp-pas: load complete, segments=%zu reloc_base=%llx\n",
		    (size_t)((const Elf32_Ehdr *)fw.data)->e_phnum,
		    (unsigned long long)reloc_base);

	if (QCOM_ADSP_SKIP_BOOT) {
		log_warning("qcom-adsp-pas: SKIP_BOOT: bypassing auth_and_reset + SMP2P wait\n");
		qpas_booted = true;
		ret = 0;
		goto out_free_metadata;
	}

	log_warning("qcom-adsp-pas: calling SCM auth_and_reset...\n");
	ret = qcom_scm_pas_auth_and_reset(QCOM_ADSP_PAS_ID);
	log_warning("qcom-adsp-pas: SCM auth_and_reset returned ret=%d\n", ret);
	if (ret)
		goto out_free_metadata;

	/* Fixed delay with heartbeats — NO SMP2P polling.
	 * Use this to determine if the board survives without touching
	 * the ADSP SMEM region, and exactly when it powers off.
	 */
	log_warning("qcom-adsp-pas: heartbeat-only delay (no SMP2P poll)...\n");
	{
		ulong hb_start = get_timer(0);
		int i;
		for (i = 0; i < 50; i++) {
			mdelay(100);
			log_warning("qcom-adsp-pas: heartbeat %d elapsed=%lu ms\n",
				    i, get_timer(hb_start));
		}
	}
	log_warning("qcom-adsp-pas: heartbeat delay complete (5s elapsed)\n");

	/* Now do the real SMP2P poll */
	log_warning("qcom-adsp-pas: starting SMP2P wait\n");
	ret = qpas_wait_for_start(node);
	if (ret) {
		qcom_scm_pas_shutdown(QCOM_ADSP_PAS_ID);
		goto out_free_metadata;
	}

	qpas_booted = true;
	log_warning("qcom-adsp-pas: booted ADSP reloc_base=%llx\n",
		    (unsigned long long)reloc_base);

out_free_metadata:
	free(metadata_ctx);
out_free_fw:
	free(fw.data);
out_unmap:
	unmap_sysmem(mem_region);
out_power_domains:
	if (ret)
		qpas_disable_power_domains(&pds);
	return ret;
}

static int qcom_adsp_pas_probe(struct udevice *dev)
{
	return qcom_adsp_pas_boot_node(dev, dev_ofnode(dev));
}

static const struct udevice_id qcom_adsp_pas_ids[] = {
	{ .compatible = QCOM_ADSP_COMPAT },
	{ }
};

U_BOOT_DRIVER(qcom_adsp_pas) = {
	.name = "qcom_adsp_pas",
	.id = UCLASS_MISC,
	.of_match = qcom_adsp_pas_ids,
	.probe = qcom_adsp_pas_probe,
	.flags = DM_FLAG_DEFAULT_PD_CTRL_OFF,
};

int qcom_adsp_pas_boot(void)
{
	struct udevice *dev;
	ofnode node;
	int ret;

	if (qpas_booted)
		return 0;

	node = ofnode_by_compatible(ofnode_null(), QCOM_ADSP_COMPAT);
	if (!ofnode_valid(node)) {
		log_warning("qcom-adsp-pas: missing %s DT node\n",
			    QCOM_ADSP_COMPAT);
		return -ENOENT;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_MISC, node, &dev);
	if (!ret || qpas_booted)
		return 0;

	log_warning("qcom-adsp-pas: DM device lookup/probe failed ret=%d\n", ret);
	return ret;
}
