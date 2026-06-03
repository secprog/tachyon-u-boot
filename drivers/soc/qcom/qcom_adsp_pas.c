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
#include <mailbox.h>
#include <mailbox-uclass.h>
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
#include <asm/io.h>
#include <asm/cache.h>
#include <soc/qcom/cmd-db.h>
#include <soc/qcom/qcom_adsp_pas.h>
#include <soc/qcom/qcom_aoss_qmp.h>
#include <soc/qcom/rpmh.h>
#include <u-boot/crc.h>
#include <lmb.h>
#include <linux/arm-smccc.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

#define QCOM_ADSP_COMPAT			"qcom,sc7280-adsp-pas"
#define QCOM_ADSP_DEFAULT_FW			"adsp.mdt"
#define QCOM_ADSP_PAS_ID			1
#define QCOM_ADSP_CRASH_REASON_SMEM		423
#define QCOM_ADSP_ERR_LOG_SMEM			611
#define QCOM_ADSP_ERR_CRASH_LOG_ADSP_SMEM	11
#define QCOM_CDSP_COMPAT			"qcom,sc7280-cdsp-pas"
#define QCOM_CDSP_DEFAULT_FW			"cdsp.mdt"
#define QCOM_CDSP_PAS_ID			18
#define QCOM_CDSP_CRASH_REASON_SMEM		601
#define QCOM_MODEM_PARTITION			"modem_a"
#define QCOM_ADSP_START_TIMEOUT_MS		5000
#define QPAS_MAX_POWER_DOMAINS			4
#define QPAS_SMP2P_MAX_ENTRY			16
#define QPAS_SMP2P_MAX_ENTRY_NAME		16
#define QPAS_SMP2P_MAGIC			0x504d5324
#define QPAS_SMP2P_VERSION			1
#define SMEM_HOST_APPS				0

#define QPAS_AOP_LOAD_STATE_SETTLE_MS		500

#define SMP2P_FEATURE_SSR_ACK			0x01
#define SMP2P_MAX_VERSION			2

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

#define QPAS_PIL_RELOC_COMPAT			"qcom,pil-reloc-info"
#define QPAS_PIL_RELOC_NAME_LEN		8
#define QPAS_PIL_RELOC_ENTRY_SIZE		(QPAS_PIL_RELOC_NAME_LEN + sizeof(u64) + sizeof(u32))
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

struct qpas_proc {
	const char *name;
	const char *compat;
	const char *default_fw;
	const char *load_state;
	const char *pil_image;
	u32 pas_id;
	u32 crash_reason_smem;
	bool booted;
	bool failed;
};

struct qpas_power_domains {
	struct power_domain pd[QPAS_MAX_POWER_DOMAINS];
	int count;
	int enabled;
};

struct qpas_clocks {
	struct clk_bulk bulk;
	bool valid;
	bool enabled;
};

struct qpas_bcm_aux {
	__le32 unit;
	__le16 width;
	u8 vcd;
	u8 reserved;
} __packed;

struct qpas_bcm_vote {
	const char *name;
	u32 addr;
	u32 unit;
	u16 width;
	u8 vcd;
	u32 vote_x;
	u32 vote_y;
};

struct qpas_smp2p_info {
	ofnode node;
	ofnode inbound;
	u32 local_pid;
	u32 remote_pid;
	u32 inbound_item;
	u32 outbound_item;
	u32 fatal_bit;
	u32 ready_bit;
	u32 handover_bit;
	u32 stop_ack_bit;
	u32 shutdown_ack_bit;
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
} __packed;

static void qpas_flush_shared_range(const void *ptr, size_t size)
{
	ulong start = rounddown((ulong)ptr, ARCH_DMA_MINALIGN);
	ulong end = roundup((ulong)ptr + size, ARCH_DMA_MINALIGN);

	flush_dcache_range(start, end);
	dsb();
}

static struct qpas_proc qpas_adsp_proc = {
	.name = "ADSP",
	.compat = QCOM_ADSP_COMPAT,
	.default_fw = QCOM_ADSP_DEFAULT_FW,
	.load_state = "adsp",
	.pil_image = "adsp",
	.pas_id = QCOM_ADSP_PAS_ID,
	.crash_reason_smem = QCOM_ADSP_CRASH_REASON_SMEM,
};

static struct qpas_proc qpas_cdsp_proc = {
	.name = "CDSP",
	.compat = QCOM_CDSP_COMPAT,
	.default_fw = QCOM_CDSP_DEFAULT_FW,
	.load_state = "cdsp",
	.pil_image = "cdsp",
	.pas_id = QCOM_CDSP_PAS_ID,
	.crash_reason_smem = QCOM_CDSP_CRASH_REASON_SMEM,
};

static int qpas_enable_clocks(struct udevice *dev, struct qpas_clocks *clks)
{
	int ret;

	ret = clk_get_bulk(dev, &clks->bulk);
	if (ret) {
		log_warning("qcom-adsp-pas: clk_get_bulk ret=%d; XO may be unmanaged in U-Boot\n",
			    ret);
		if (dev_read_prop(dev, "clocks", NULL))
			return ret;

		return 0;
	}

	clks->valid = true;

	ret = clk_enable_bulk(&clks->bulk);
	log_warning("qcom-adsp-pas: clk_enable_bulk ret=%d count=%d\n",
		    ret, clks->bulk.count);
	if (ret)
		return ret;

	clks->enabled = true;
	return 0;
}

static void qpas_disable_clocks(struct qpas_clocks *clks)
{
	if (clks->enabled)
		clk_disable_bulk(&clks->bulk);
	if (clks->valid)
		clk_release_bulk(&clks->bulk);
}

static int qpas_bcm_get(const char *name, struct qpas_bcm_vote *vote)
{
	const struct qpas_bcm_aux *aux;
	enum cmd_db_hw_type type;
	size_t aux_len = 0;

	vote->name = name;
	vote->addr = cmd_db_read_addr(name);
	if (!vote->addr) {
		log_warning("qcom-adsp-pas: BCM %s missing cmd-db addr\n",
			    name);
		return -ENOENT;
	}

	type = cmd_db_read_slave_id(name);
	if (type != CMD_DB_HW_BCM) {
		log_warning("qcom-adsp-pas: BCM %s cmd-db type=%d expected=%d\n",
			    name, type, CMD_DB_HW_BCM);
		return -EINVAL;
	}

	aux = cmd_db_read_aux_data(name, &aux_len);
	if (IS_ERR_OR_NULL(aux) || aux_len < sizeof(*aux)) {
		log_warning("qcom-adsp-pas: BCM %s missing aux data len=%zu\n",
			    name, aux_len);
		return -EINVAL;
	}

	vote->vcd = aux->vcd;
	vote->unit = le32_to_cpu(aux->unit);
	vote->width = le16_to_cpu(aux->width);
	log_warning("qcom-adsp-pas: BCM %s addr=%#x unit=%u width=%u vcd=%u\n",
		    name, vote->addr, vote->unit, vote->width, vote->vcd);

	return 0;
}

static u64 qpas_div_round_up_u64(u64 dividend, u64 divisor)
{
	return (dividend + divisor - 1) / divisor;
}

static u32 qpas_bcm_vote_field(u64 bw, u32 unit)
{
	u64 val;

	if (!bw || !unit)
		return 0;

	val = qpas_div_round_up_u64(bw, unit);
	if (val > BCM_TCS_CMD_VOTE_MASK)
		return BCM_TCS_CMD_VOTE_MASK;

	return val;
}

static u64 qpas_icb_scale_bw(u64 bw, u64 dividend, u64 divisor)
{
	if (!bw || !dividend || !divisor)
		return 0;

	return qpas_div_round_up_u64(bw * dividend, divisor);
}

static int qpas_cdsp_proxy_vote(struct udevice *rpmh_dev, bool enable)
{
	const char * const names[] = { "CO3", "CN1" };
	struct qpas_bcm_vote votes[ARRAY_SIZE(names)];
	struct tcs_cmd cmds[ARRAY_SIZE(names)];
	int ret;
	int i;
	int j;

	if (!rpmh_dev)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		ret = qpas_bcm_get(names[i], &votes[i]);
		if (ret)
			return ret;
	}

	/*
	 * Tachyon's XBL reports SocKodiakLAA.  Kodiak PilProxyVoteLib votes
	 * only MASTER_CDSP_PROC -> SLAVE_CLK_CTL with ICB TYPE_3
	 * ib=100 MB/s, ab=100 MB/s.
	 *
	 * Kodiak ICB route:
	 *   master qxm_nsp       -> BCM CO3, width 32, agg ports 2
	 *   slave  qhs_clk_ctl   -> BCM CN1, width 4,  agg ports 1
	 *
	 * Qualcomm's ICB aggregation scales by bcm_width/node_width, then
	 * encodes ceil(ib_or_ab / bcm_unit) into the BCM vote fields.
	 */
	for (i = 0; i < ARRAY_SIZE(votes); i++) {
		if (!strcmp(votes[i].name, "CO3")) {
			u64 ib = qpas_icb_scale_bw(100000000ULL,
						   votes[i].width, 32);
			u64 ab = qpas_icb_scale_bw(100000000ULL,
						   votes[i].width, 32 * 2);

			votes[i].vote_y = qpas_bcm_vote_field(ib,
							      votes[i].unit);
			votes[i].vote_x = qpas_bcm_vote_field(ab,
							      votes[i].unit);
		} else if (!strcmp(votes[i].name, "CN1")) {
			u64 ib = qpas_icb_scale_bw(100000000ULL,
						   votes[i].width, 4);
			u64 ab = qpas_icb_scale_bw(100000000ULL,
						   votes[i].width, 4);

			votes[i].vote_y = qpas_bcm_vote_field(ib,
							      votes[i].unit);
			votes[i].vote_x = qpas_bcm_vote_field(ab,
							      votes[i].unit);
		}
	}

	for (i = 0; i < ARRAY_SIZE(votes) - 1; i++) {
		for (j = i + 1; j < ARRAY_SIZE(votes); j++) {
			if (votes[j].vcd < votes[i].vcd) {
				struct qpas_bcm_vote tmp = votes[i];

				votes[i] = votes[j];
				votes[j] = tmp;
			}
		}
	}

	for (i = 0; i < ARRAY_SIZE(votes); i++) {
		bool commit = i == ARRAY_SIZE(votes) - 1 ||
			      votes[i].vcd != votes[i + 1].vcd;
		u32 vote_x = enable ? votes[i].vote_x : 0;
		u32 vote_y = enable ? votes[i].vote_y : 0;
		bool valid = enable && (vote_x || vote_y);

		cmds[i].addr = votes[i].addr;
		cmds[i].data = BCM_TCS_CMD(commit, valid, vote_x, vote_y);
		cmds[i].wait = commit;

		log_warning("qcom-adsp-pas: CDSP proxy %s BCM %s vcd=%u commit=%d x=%#x y=%#x data=%#x\n",
			    enable ? "vote" : "unvote", votes[i].name,
			    votes[i].vcd, commit, vote_x, vote_y, cmds[i].data);
	}

	ret = rpmh_write(rpmh_dev, RPMH_ACTIVE_ONLY_STATE, cmds,
			 ARRAY_SIZE(cmds));
	log_warning("qcom-adsp-pas: CDSP proxy %s ret=%d\n",
		    enable ? "vote" : "unvote", ret);

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

		/*
		 * Linux sets INT_MAX performance state before
		 * pm_runtime_get_sync() for proxy PDs (LCX, LMX).
		 * Do the same here: vote max corner first, then
		 * enable the domain.  -ENOSYS is silently ignored.
		 */
		ret = power_domain_set_performance_state(&pds->pd[i],
							 (unsigned int)-1);
		if (ret && ret != -ENOSYS)
			log_warning("qcom-adsp-pas: perf_state index=%d ret=%d\n",
				    i, ret);

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
		power_domain_set_performance_state(&pds->pd[pds->enabled], 0);
		power_domain_off(&pds->pd[pds->enabled]);
	}

	return ret;
}

static void qpas_disable_power_domains(struct qpas_power_domains *pds)
{
	int ret;

	while (pds->enabled > 0) {
		pds->enabled--;
		power_domain_set_performance_state(&pds->pd[pds->enabled], 0);
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
	info->handover_bit = U32_MAX;
	info->stop_ack_bit = U32_MAX;
	info->shutdown_ack_bit = U32_MAX;

	for (i = 0; i < count; i++) {
		ret = ofnode_read_string_index(node, "interrupt-names", i,
					       &name);
		if (ret)
			return ret;

		if (strcmp(name, "fatal") && strcmp(name, "ready") &&
		    strcmp(name, "handover") && strcmp(name, "stop-ack") &&
		    strcmp(name, "shutdown-ack"))
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
		else if (!strcmp(name, "ready"))
			info->ready_bit = args.args[0];
		else if (!strcmp(name, "handover"))
			info->handover_bit = args.args[0];
		else if (!strcmp(name, "stop-ack"))
			info->stop_ack_bit = args.args[0];
		else
			info->shutdown_ack_bit = args.args[0];
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
	u32 local_pid;
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

		ret = ofnode_read_u32(smp2p, "qcom,local-pid", &local_pid);
		if (ret)
			return ret;

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
			info->local_pid = local_pid;
			info->inbound_item = smem[1];
			info->outbound_item = smem[0];
			strncpy(info->entry_name, entry_name,
				sizeof(info->entry_name));
			info->entry_name[sizeof(info->entry_name) - 1] = '\0';

			ret = qpas_find_smp2p_irq_bits(node, info);
			if (ret)
				return ret;

			log_warning("qcom-adsp-pas: SMP2P local_pid=%u remote_pid=%u item=%u entry='%s' fatal=%u ready=%u handover=%u stop_ack=%u shutdown_ack=%u\n",
				    info->local_pid, info->remote_pid,
				    info->inbound_item, info->entry_name,
				    info->fatal_bit, info->ready_bit,
				    info->handover_bit,
				    info->stop_ack_bit,
				    info->shutdown_ack_bit);
			return 0;
		}
	}

	return -ENOENT;
}

/*
 * qpas_smp2p_kick() - Linux qcom_smp2p_kick() equivalent.
 *
 * Tries mboxes from the SMP2P node first; falls back to qcom,ipc
 * (parses syscon phandle+offset+bit, writes BIT(bit) directly).
 * Returns 0 if any kick mechanism succeeded; -ENODEV if no usable
 * mechanism was found.  Caller treats -ENODEV as fatal.
 */
static int qpas_smp2p_kick(const struct qpas_smp2p_info *info)
{
	struct ofnode_phandle_args mbox_args;
	struct udevice *ipcc_dev;
	struct mbox_chan mbox;
	int ret;

	if (!ofnode_valid(info->node))
		return -ENODEV;

	/* Linux qcom_smp2p_kick() begins with wmb() */
	dmb();

	/* Try SMP2P node's own mboxes */
	ret = ofnode_parse_phandle_with_args(info->node, "mboxes",
					     "#mbox-cells", 0, 0, &mbox_args);
	if (!ret && ofnode_valid(mbox_args.node) && mbox_args.args_count >= 2) {
		ret = uclass_get_device_by_ofnode(UCLASS_MAILBOX,
						  mbox_args.node, &ipcc_dev);
		if (!ret) {
			const struct mbox_ops *ops = ipcc_dev->driver->ops;

			mbox.dev = ipcc_dev;
			mbox.con_priv = NULL;
			if (ops->of_xlate)
				ret = ops->of_xlate(&mbox, &mbox_args);
			else
				mbox.id = mbox_args.args[0];
			if (!ret && ops->request)
				ret = ops->request(&mbox);
			if (!ret) {
				ret = mbox_send(&mbox, NULL);
				log_warning("qcom-adsp-pas: SMP2P kick ret=%d\n",
					    ret);
				return 0;
			}
		}
	}

	/* Fallback: qcom,ipc (Linux smp2p_parse_ipc).
	 * ipc_data[0]=syscon_phandle, ipc_data[1]=offset, ipc_data[2]=bit.
	 * Linux writes exactly BIT(bit), not read-modify-write.
	 */
	{
		struct ofnode_phandle_args ipc_args;
		u32 ipc_data[3];

		ret = ofnode_parse_phandle_with_args(info->node, "qcom,ipc",
						     NULL, 0, 0, &ipc_args);
		if (!ret) {
			ret = ofnode_read_u32_array(info->node, "qcom,ipc",
						    ipc_data, 3);
			if (!ret) {
				struct resource res;

				ret = ofnode_read_resource(ipc_args.node, 0,
							   &res);
				if (!ret) {
					void __iomem *base = map_sysmem(
						res.start,
						resource_size(&res));

					writel(BIT(ipc_data[2]),
					       base + ipc_data[1]);
					log_warning("qcom-adsp-pas: SMP2P kick via qcom,ipc off=%x bit=%u\n",
						    ipc_data[1],
						    ipc_data[2]);
					return 0;
				}
			}
		}
	}

	log_warning("qcom-adsp-pas: SMP2P kick no usable mechanism\n");
	return -ENODEV;
}

/*
 * qpas_smp2p_init() - Linux qcom_smp2p_alloc_outbound_item() equivalent.
 *
 * Follows Linux's two-phase publish/kick sequence:
 *   Phase 1 (header only): clear, write magic/local_pid/remote_pid/
 *     total_entries/valid_entries=0/features, dmb(), version=1, kick.
 *   Phase 2 (entries): populate every outbound child entry from DT,
 *     bump valid_entries, kick again.
 *
 * Linux does this in two probe stages (probe -> for_each_child).
 * We do both upfront since ADSP isn't running yet, but preserve
 * the two-kick order for protocol fidelity.
 *
 * Does NOT allocate item 429 - ADSP firmware creates its own outbound
 * item (per-processor ownership model).
 */
static int qpas_smp2p_init(struct udevice *smem, ofnode node,
			   struct qpas_smp2p_info *info)
{
	const char *entry_name;
	struct qpas_smp2p_smem_item *out;
	ofnode child;
	u16 valid = 0;
	int ret;

	ret = qpas_find_smp2p(node, info);
	if (ret) {
		log_warning("qcom-adsp-pas: SMP2P discovery ret=%d\n", ret);
		return ret;
	}

	/*
	 * Allocate outbound item (APPS -> ADSP) in the ADSP-private
	 * partition.  Linux: qcom_smem_alloc(remote_pid, smem_id, size).
	 */
	ret = smem_alloc(smem, info->remote_pid, info->outbound_item,
			 sizeof(*out));
	if (ret && ret != -EEXIST) {
		log_warning("qcom-adsp-pas: smem_alloc out item=%u ret=%d\n",
			    info->outbound_item, ret);
		return ret;
	}

	out = smem_get(smem, info->remote_pid, info->outbound_item, NULL);
	if (IS_ERR_OR_NULL(out)) {
		ret = PTR_ERR(out);
		log_warning("qcom-adsp-pas: smem_get out item=%u ret=%d\n",
			    info->outbound_item, ret);
		return ret ?: -EIO;
	}

	/*
	 * Phase 1: header -> dmb -> version -> kick (Linux order).
	 * Linux: memset(out, 0, sizeof(*out)); magic=...; features=...;
	 * wmb(); version=...; qcom_smp2p_kick().
	 */
	memset(out, 0, sizeof(*out));
	out->magic = cpu_to_le32(QPAS_SMP2P_MAGIC);
	out->local_pid = cpu_to_le16(info->local_pid);
	out->remote_pid = cpu_to_le16(info->remote_pid);
	out->total_entries = cpu_to_le16(QPAS_SMP2P_MAX_ENTRY);
	out->valid_entries = cpu_to_le16(0);
	memset(out->features, 0, sizeof(out->features));
	out->features[0] = SMP2P_FEATURE_SSR_ACK;

	dmb();
	out->version = QPAS_SMP2P_VERSION;	/* Linux 6.8: version must be 1 */
	qpas_flush_shared_range(out, sizeof(*out));

	log_warning("qcom-adsp-pas: SMP2P header done item=%u magic=%08x ver=%u local=%u remote=%u feat=%02x\n",
		    info->outbound_item,
		    le32_to_cpu(out->magic), out->version,
		    le16_to_cpu(out->local_pid), le16_to_cpu(out->remote_pid),
		    out->features[0]);

	ret = qpas_smp2p_kick(info);
	if (ret) {
		log_warning("qcom-adsp-pas: SMP2P phase-1 kick failed ret=%d\n",
			    ret);
		return ret;
	}

	/*
	 * Phase 2: publish every outbound child entry under the SMP2P node.
	 * Linux's qcom_smp2p_probe() walks all children and treats nodes
	 * without interrupt-controller as outbound qcom_smem_state entries.
	 * Tachyon/QCM6490 adds rdbg/sleepstate entries on top of the base
	 * master-kernel entry, so publishing only qcom,smem-states is not
	 * Linux parity.
	 */
	ofnode_for_each_subnode(child, info->node) {
		if (ofnode_read_bool(child, "interrupt-controller"))
			continue;

		entry_name = ofnode_read_string(child, "qcom,entry-name");
		if (!entry_name)
			continue;

		if (valid >= QPAS_SMP2P_MAX_ENTRY) {
			log_warning("qcom-adsp-pas: SMP2P too many outbound entries, dropping '%s'\n",
				    entry_name);
			break;
		}

		strlcpy((char *)out->entries[valid].name, entry_name,
			QPAS_SMP2P_MAX_ENTRY_NAME);
		out->entries[valid].value = cpu_to_le32(0);
		valid++;
		out->valid_entries = cpu_to_le16(valid);
		log_warning("qcom-adsp-pas: SMP2P out entry[%u]='%s'\n",
			    valid - 1, entry_name);
	}

	if (!valid) {
		log_warning("qcom-adsp-pas: SMP2P no outbound entries found\n");
		return -EINVAL;
	}

	qpas_flush_shared_range(out, sizeof(*out));

	ret = qpas_smp2p_kick(info);
	if (ret) {
		log_warning("qcom-adsp-pas: SMP2P phase-2 kick failed ret=%d\n",
			    ret);
		return ret;
	}

	return 0;
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

	/*
	 * Linux reads inbound items using remote_pid (e.g. host=2 for
	 * ADSP). Try remote_pid first, then SMEM_HOST_APPS fallback.
	 * Log both distinctly so we can tell which path resolves (if any).
	 */
	item = smem_get(smem, info->remote_pid, info->inbound_item, &size);
	if (IS_ERR_OR_NULL(item)) {
		log_warning("qcom-adsp-pas: inbound remote_pid=%u item=%u ptr=%p size=%zu\n",
			    info->remote_pid, info->inbound_item, item, size);
		size = 0;
		item = smem_get(smem, SMEM_HOST_APPS,
				info->inbound_item, &size);
		log_warning("qcom-adsp-pas: inbound APPS/global item=%u ptr=%p size=%zu\n",
			    info->inbound_item, item, size);
	} else {
		log_warning("qcom-adsp-pas: inbound remote_pid=%u item=%u ptr=%p size=%zu OK\n",
			    info->remote_pid, info->inbound_item, item, size);
	}
	if (IS_ERR_OR_NULL(item))
		return -EAGAIN;

	if (size < sizeof(*item))
		return -EINVAL;

	magic = le32_to_cpu(item->magic);
	if (magic != QPAS_SMP2P_MAGIC || !item->version ||
	    item->version > SMP2P_MAX_VERSION)
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

static int qpas_find_interrupt_index(ofnode node, const char *needle)
{
	const char *name;
	int count;
	int ret;
	int i;

	count = ofnode_read_string_count(node, "interrupt-names");
	if (count < 0)
		return count;

	for (i = 0; i < count; i++) {
		ret = ofnode_read_string_index(node, "interrupt-names", i,
					       &name);
		if (ret)
			return ret;

		if (!strcmp(name, needle))
			return i;
	}

	return -ENOENT;
}

static void qpas_log_q6v5_irq_resources(ofnode node)
{
	static const char * const names[] = {
		"wdog",
		"fatal",
		"ready",
		"handover",
		"stop-ack",
		"shutdown-ack",
	};
	struct ofnode_phandle_args args;
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		ret = qpas_find_interrupt_index(node, names[i]);
		if (ret < 0) {
			log_warning("qcom-adsp-pas: q6v5 irq '%s' missing ret=%d\n",
				    names[i], ret);
			continue;
		}

		ret = ofnode_parse_phandle_with_args(node,
						     "interrupts-extended",
						     "#interrupt-cells", 0,
						     ret, &args);
		if (ret) {
			log_warning("qcom-adsp-pas: q6v5 irq '%s' parse ret=%d\n",
				    names[i], ret);
			continue;
		}

		log_warning("qcom-adsp-pas: q6v5 irq '%s' provider=%s cells=%d arg0=%u arg1=%u\n",
			    names[i], ofnode_get_name(args.node),
			    args.args_count,
			    args.args_count > 0 ? args.args[0] : 0,
			    args.args_count > 1 ? args.args[1] : 0);
	}

	log_warning("qcom-adsp-pas: q6v5 prepare IRQ parity: Linux enables handover IRQ here; U-Boot has no Qualcomm PDC/SMP2P IRQ backend, using SMEM/SMP2P polling\n");
}

static int qcom_scm_pas_shutdown(u32 pas_id);

static void qpas_log_pre_release_state(struct qpas_proc *proc, ofnode node)
{
	struct qpas_smp2p_info info = {};
	struct udevice *smem;
	size_t cr_sz = 0;
	char *cr;
	u32 value = 0;
	int ret;

	ret = qpas_find_smp2p(node, &info);
	if (ret) {
		log_warning("qcom-adsp-pas: pre-release SMP2P discovery ret=%d\n",
			    ret);
		return;
	}

	ret = uclass_first_device_err(UCLASS_SMEM, &smem);
	if (ret) {
		log_warning("qcom-adsp-pas: pre-release SMEM lookup ret=%d\n",
			    ret);
		return;
	}

	ret = qpas_smp2p_read_entry(smem, &info, &value);
	log_warning("qcom-adsp-pas: pre-release SMP2P read ret=%d value=%08x\n",
		    ret, value);

	cr = smem_get(smem, SMEM_HOST_APPS, proc->crash_reason_smem, &cr_sz);
	if (IS_ERR_OR_NULL(cr) || !cr_sz) {
		log_warning("qcom-adsp-pas: pre-release crash reason empty\n");
		return;
	}

	log_warning("qcom-adsp-pas: pre-release crash reason='%.*s'\n",
		    (int)min(cr_sz, (size_t)96), cr);
}

static bool qpas_log_crash_reason(struct udevice *smem, struct qpas_proc *proc,
				  ulong elapsed)
{
	size_t cr_size = 0;
	char *reason;

	reason = smem_get(smem, SMEM_HOST_APPS, proc->crash_reason_smem,
			  &cr_size);
	if (IS_ERR_OR_NULL(reason) || !cr_size)
		return false;

	reason[cr_size - 1] = '\0';
	if (!reason[0] || !strcmp(reason, "empty"))
		return false;

	log_warning("qcom-adsp-pas: %s crash reason at %lu ms: '%s'\n",
		    proc->name, elapsed, reason);
	return true;
}

static bool qpas_ascii_has_payload(const char *buf, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++) {
		if (buf[i] >= 0x20 && buf[i] <= 0x7e)
			return true;
	}

	return false;
}

static void qpas_log_smem_ascii_item(struct udevice *smem, u32 host, u32 item,
				     const char *label, size_t max_len)
{
	char line[257];
	const char *buf;
	size_t size = 0;
	size_t len;
	size_t i;

	buf = smem_get(smem, host, item, &size);
	if (IS_ERR_OR_NULL(buf)) {
		log_warning("qcom-adsp-pas: %s host=%u item=%u unavailable ptr=%p size=%zu\n",
			    label, host, item, buf, size);
		return;
	}

	len = min(size, min(max_len, sizeof(line) - 1));
	if (!len || !qpas_ascii_has_payload(buf, len)) {
		log_warning("qcom-adsp-pas: %s host=%u item=%u size=%zu empty/non-ascii\n",
			    label, host, item, size);
		return;
	}

	for (i = 0; i < len; i++) {
		char c = buf[i];

		line[i] = (c >= 0x20 && c <= 0x7e) ? c : ' ';
	}
	line[len] = '\0';

	log_warning("qcom-adsp-pas: %s host=%u item=%u size=%zu text='%s'\n",
		    label, host, item, size, line);
}

static void qpas_log_adsp_error_smem(struct udevice *smem,
				     const struct qpas_smp2p_info *info)
{
	qpas_log_smem_ascii_item(smem, info->remote_pid,
				 QCOM_ADSP_ERR_LOG_SMEM,
				 "ADSP ERR_LOG_SMEM_ITEM", 256);
	qpas_log_smem_ascii_item(smem, SMEM_HOST_APPS,
				 QCOM_ADSP_ERR_LOG_SMEM,
				 "ADSP ERR_LOG_SMEM_ITEM fallback", 256);
	qpas_log_smem_ascii_item(smem, SMEM_HOST_APPS,
				 QCOM_ADSP_ERR_CRASH_LOG_ADSP_SMEM,
				 "ADSP legacy crash-log", 256);
}

/*
 * qpas_wait_for_start() - poll ADSP SMP2P state every 20 ms.
 *
 * Mirrors Linux's Q6V5 IRQ-driven wait: watches fatal, ready, handover,
 * stop-ack, and shutdown-ack bits.  On handover (Linux qcom_pas_handover),
 * releases proxy clocks and power domains and continues waiting for ready.
 * On fatal, shuts down ADSP via PAS.
 *
 * @node:     remoteproc DT node
 * @clks:     proxy clocks to release on handover (may be NULL)
 * @pds:      proxy power domains to release on handover (may be NULL)
 * @return:   0 on ready, -EIO on fatal, -ETIMEDOUT on timeout
 */
static int qpas_wait_for_start(struct qpas_proc *proc, ofnode node,
			       struct qpas_clocks *clks,
			       struct qpas_power_domains *pds)
{
	struct qpas_smp2p_info info = {};
	struct udevice *smem;
	ulong start;
	u32 value;
	bool handover_seen = false;
	bool pending_logged = false;
	ulong last_trace = 0;
	ulong last_crash_check = 0;
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

	log_warning("qcom-adsp-pas: waiting %s SMP2P ready (remote_pid=%u) timeout=%d ms\n",
		    proc->name, info.remote_pid, QCOM_ADSP_START_TIMEOUT_MS);

	start = get_timer(0);
	do {
		if (!last_crash_check || get_timer(last_crash_check) >= 100) {
			last_crash_check = get_timer(0);
			/*
			 * ADSP seeds SMEM with
			 * "SFR Init: wdog or kernel error suspected." during
			 * normal early init.  Linux does not treat crash reason
			 * content as a start failure condition; it waits for the
			 * SMP2P ready/fatal bits.  Keep this as diagnostic-only
			 * unless a real fatal bit or timeout occurs.
			 */
			qpas_log_crash_reason(smem, proc, get_timer(start));
		}

		ret = qpas_smp2p_read_entry(smem, &info, &value);
		if (ret == -EAGAIN) {
			if (!pending_logged) {
				log_warning("qcom-adsp-pas: waiting for SMP2P entry '%s' (elapsed=%lu ms)\n",
					    info.entry_name, get_timer(start));
				pending_logged = true;
			}
			mdelay(20);
			continue;
		}
		if (ret) {
			log_warning("qcom-adsp-pas: SMP2P read ret=%d\n", ret);
			return ret;
		}

		/* Trace all bits every 20ms once the entry is readable */
		if (get_timer(last_trace) >= 20 || !last_trace) {
			log_warning("qcom-adsp-pas: t=%lu smp2p=%08x fatal=%d ready=%d handover=%d stop=%d shutdown=%d\n",
				    get_timer(start), value,
				    !!(value & BIT(info.fatal_bit)),
				    !!(value & BIT(info.ready_bit)),
				    info.handover_bit != U32_MAX ?
					    !!(value & BIT(info.handover_bit)) : -1,
				    info.stop_ack_bit != U32_MAX ?
					    !!(value & BIT(info.stop_ack_bit)) : -1,
				    info.shutdown_ack_bit != U32_MAX ?
					    !!(value & BIT(info.shutdown_ack_bit)) : -1);
			last_trace = get_timer(0);
		}

		/* Fatal: ADSP crashed - shutdown and report */
		if (value & BIT(info.fatal_bit)) {
			log_warning("qcom-adsp-pas: %s fatal at %lu ms value=%08x\n",
				    proc->name, get_timer(start), value);

			qpas_log_crash_reason(smem, proc, get_timer(start));
			if (proc == &qpas_adsp_proc)
				qpas_log_adsp_error_smem(smem, &info);
			qcom_scm_pas_shutdown(proc->pas_id);
			proc->failed = true;
			return -EIO;
		}

		/*
		 * Handover: ADSP has taken ownership of proxy resources.
		 * Linux qcom_pas_handover() releases XO/aggre2 clocks and
		 * drops proxy PD votes.  Do the same here, once.
		 */
		if (!handover_seen && info.handover_bit != U32_MAX &&
		    (value & BIT(info.handover_bit))) {
			log_warning("qcom-adsp-pas: %s handover at %lu ms, releasing proxy resources\n",
				    proc->name, get_timer(start));
			handover_seen = true;
			if (clks)
				qpas_disable_clocks(clks);
			if (pds)
				qpas_disable_power_domains(pds);
		}

		/* Ready: ADSP booted successfully */
		if (value & BIT(info.ready_bit)) {
			log_warning("qcom-adsp-pas: %s ready at %lu ms value=%08x\n",
				    proc->name, get_timer(start), value);
			return 0;
		}

		mdelay(20);
	} while (get_timer(start) < QCOM_ADSP_START_TIMEOUT_MS);

	log_warning("qcom-adsp-pas: %s SMP2P ready timeout after %lu ms\n",
		    proc->name, get_timer(start));

	qpas_log_crash_reason(smem, proc, get_timer(start));
	if (proc == &qpas_adsp_proc)
		qpas_log_adsp_error_smem(smem, &info);
	qcom_scm_pas_shutdown(proc->pas_id);
	proc->failed = true;

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

static size_t qpas_metadata_aligned;

struct qpas_metadata_ctx {
	void *vaddr;
	phys_addr_t phys;
	size_t size;
	bool lmb;
};

static struct qpas_metadata_ctx qpas_metadata;

static int qpas_alloc_low_metadata(size_t size)
{
	phys_addr_t addr;
	size_t aligned;
	void *vaddr;

	memset(&qpas_metadata, 0, sizeof(qpas_metadata));

	aligned = ALIGN(size, SZ_4K);
	qpas_metadata_aligned = aligned;

	/* Force allocation strictly below 4 GiB */
	addr = lmb_alloc_base(aligned, SZ_4K, 0x100000000ULL, LMB_NONE);
	if (!addr) {
		log_warning("qcom-adsp-pas: LMB low metadata alloc failed below 4G size=%zu\n",
			    aligned);
		return -ENOMEM;
	}

	if (addr + aligned > 0x100000000ULL) {
		log_warning("qcom-adsp-pas: LMB metadata not below 4G addr=%llx size=%zu\n",
			    (unsigned long long)addr, aligned);
		lmb_free(addr, aligned);
		return -ENOMEM;
	}

	vaddr = map_sysmem(addr, aligned);
	if (!vaddr) {
		lmb_free(addr, aligned);
		return -ENOMEM;
	}

	/*
	 * Linux uses dma_alloc_coherent() for PAS metadata; make this
	 * buffer non-cacheable before copying into it.
	 */
	mmu_set_region_dcache_behaviour(addr, aligned, DCACHE_OFF);

	qpas_metadata.vaddr = vaddr;
	qpas_metadata.phys = addr;
	qpas_metadata.size = aligned;
	qpas_metadata.lmb = true;

	return 0;
}

static void qpas_free_low_metadata(void)
{
	if (!qpas_metadata.vaddr)
		return;

	/* Restore cacheable mapping before freeing */
	mmu_set_region_dcache_behaviour(qpas_metadata.phys,
					qpas_metadata.size,
					DCACHE_DEFAULT_OPTION);

	unmap_sysmem(qpas_metadata.vaddr);

	if (qpas_metadata.lmb)
		lmb_free(qpas_metadata.phys, qpas_metadata.size);

	memset(&qpas_metadata, 0, sizeof(qpas_metadata));
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
	int ret;

	ret = qpas_alloc_low_metadata(size);
	if (ret)
		return ret;

	memset(qpas_metadata.vaddr, 0, qpas_metadata.size);
	memcpy(qpas_metadata.vaddr, metadata, size);

	/*
	 * Flush is harmless on device memory but keeps the pattern
	 * consistent with the ADSP carveout and segment loader.
	 */
	flush_cache(rounddown((ulong)qpas_metadata.vaddr, ARCH_DMA_MINALIGN),
		    roundup((ulong)qpas_metadata.vaddr + qpas_metadata.size,
			    ARCH_DMA_MINALIGN) -
		    rounddown((ulong)qpas_metadata.vaddr,
			      ARCH_DMA_MINALIGN));

	desc.args[1] = qpas_metadata.phys;

	log_warning("qcom-adsp-pas: SCM PAS_INIT_IMAGE svc=%x cmd=%x arginfo=%x pas_id=%u metadata_phys=%llx metadata_size=%zu below4g=%d noncache=1 lmb=1\n",
		    QCOM_SCM_SVC_PIL,
		    QCOM_SCM_PIL_PAS_INIT_IMAGE,
		    desc.arginfo,
		    pas_id,
		    (unsigned long long)qpas_metadata.phys, size,
		    qpas_metadata.phys + qpas_metadata.size <= 0x100000000ULL);

	ret = qcom_scm_call(&desc, &res);

	log_warning("qcom-adsp-pas: SCM PAS_INIT_IMAGE ret=%d result0=%llu result1=%llu result2=%llu\n",
		    ret, res.result[0], res.result[1], res.result[2]);

	if (ret || res.result[0]) {
		log_warning("qcom-adsp-pas: init_image ret=%d scm_result=%llu\n",
			    ret, res.result[0]);
		qpas_free_low_metadata();
		return ret ? ret : (int)res.result[0];
	}

	*ctxp = &qpas_metadata;
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

	log_warning("qcom-adsp-pas: SCM PAS_MEM_SETUP svc=%x cmd=%x arginfo=%x pas_id=%u addr=%llx size=%llx\n",
		    QCOM_SCM_SVC_PIL,
		    QCOM_SCM_PIL_PAS_MEM_SETUP,
		    desc.arginfo,
		    pas_id,
		    (unsigned long long)addr,
		    (unsigned long long)size);

	ret = qcom_scm_call(&desc, &res);

	log_warning("qcom-adsp-pas: SCM PAS_MEM_SETUP ret=%d result0=%llu result1=%llu result2=%llu\n",
		    ret, res.result[0], res.result[1], res.result[2]);

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

	log_warning("qcom-adsp-pas: SCM PAS_AUTH_AND_RESET svc=%x cmd=%x arginfo=%x pas_id=%u\n",
		    QCOM_SCM_SVC_PIL,
		    QCOM_SCM_PIL_PAS_AUTH_AND_RESET,
		    desc.arginfo,
		    pas_id);

	ret = qcom_scm_call(&desc, &res);

	log_warning("qcom-adsp-pas: SCM PAS_AUTH_AND_RESET ret=%d result0=%llu result1=%llu result2=%llu\n",
		    ret, res.result[0], res.result[1], res.result[2]);

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

static int qpas_mdt_init_image(struct qpas_proc *proc, const struct qpas_fw *fw,
			       phys_addr_t mem_phys, void **metadata_ctxp)
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

	log_warning("qcom-adsp-pas: MDT hdr phnum=%u entry=%08x\n",
		    ehdr->e_phnum, ehdr->e_entry);

	for (i = 0; i < ehdr->e_phnum; i++) {
		const Elf32_Phdr *phdr = &phdrs[i];

		log_warning("qcom-adsp-pas: phdr[%d] type=%08x flags=%08x off=%08x paddr=%08x filesz=%08x memsz=%08x valid=%d reloc=%d\n",
			    i,
			    le32_to_cpu(phdr->p_type),
			    le32_to_cpu(phdr->p_flags),
			    le32_to_cpu(phdr->p_offset),
			    le32_to_cpu(phdr->p_paddr),
			    le32_to_cpu(phdr->p_filesz),
			    le32_to_cpu(phdr->p_memsz),
			    qpas_mdt_phdr_loadable(phdr),
			    !!(le32_to_cpu(phdr->p_flags) & QCOM_MDT_RELOCATABLE));

		if (!qpas_mdt_phdr_loadable(phdr))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);
	}

	log_warning("qcom-adsp-pas: MDT summary relocate=%d min_addr=%08llx max_addr=%08llx mem_phys=%llx mem_setup_size=%llx\n",
		    relocate,
		    (unsigned long long)min_addr,
		    (unsigned long long)max_addr,
		    (unsigned long long)mem_phys,
		    (unsigned long long)(max_addr - min_addr));

	ret = qpas_mdt_read_metadata(fw, &metadata, &metadata_len);
	if (ret)
		return ret;

	ret = qcom_scm_pas_init_image(proc->pas_id, metadata,
				      metadata_len, metadata_ctxp);
	free(metadata);
	if (ret)
		return ret;

	if (relocate) {
		ret = qcom_scm_pas_mem_setup(proc->pas_id, mem_phys,
					     max_addr - min_addr);
	} else {
		log_warning("qcom-adsp-pas: SCM PAS_MEM_SETUP skipped relocate=%d\n",
			    relocate);
	}

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

/*
 * PIL relocation info helpers
 *
 * Linux-parity: store the ADSP firmware base/size into the IMEM
 * pil-reloc-info region so post-mortem debugging tools can locate
 * remoteproc images.  Entry layout:
 *   - 8-byte textual image name (QPAS_PIL_RELOC_NAME_LEN)
 *   - u64 little-endian base address
 *   - u32 little-endian size
 */

static bool qpas_pil_reloc_inited;

static ofnode qpas_find_compatible_recursive(ofnode parent,
					     const char *compat)
{
	ofnode child;

	if (ofnode_device_is_compatible(parent, compat))
		return parent;

	ofnode_for_each_subnode(child, parent) {
		ofnode found;

		found = qpas_find_compatible_recursive(child, compat);
		if (ofnode_valid(found))
			return found;
	}

	return ofnode_null();
}

static ofnode qpas_find_pil_reloc_node(void)
{
	return qpas_find_compatible_recursive(ofnode_root(),
					      QPAS_PIL_RELOC_COMPAT);
}

static int qpas_pil_info_store(const char *image, phys_addr_t base,
			       size_t size)
{
	ofnode node;
	struct resource res;
	void __iomem *vaddr;
	u8 name[QPAS_PIL_RELOC_NAME_LEN];
	size_t region_size;
	size_t num_entries;
	int slot = -1;
	int ret;
	int i;

	node = qpas_find_pil_reloc_node();
	if (!ofnode_valid(node)) {
		log_warning("qcom-adsp-pas: PIL reloc node not found, skipping\n");
		return 0;
	}

	ret = ofnode_read_resource(node, 0, &res);
	if (ret) {
		log_warning("qcom-adsp-pas: PIL reloc resource ret=%d\n", ret);
		return ret;
	}

	region_size = resource_size(&res);
	if (region_size < QPAS_PIL_RELOC_ENTRY_SIZE) {
		log_warning("qcom-adsp-pas: PIL reloc region too small size=%zu\n",
			    region_size);
		return -ENOSPC;
	}

	vaddr = map_sysmem(res.start, region_size);
	if (!vaddr) {
		log_warning("qcom-adsp-pas: PIL reloc map failed phys=%llx\n",
			    (unsigned long long)res.start);
		return -ENOMEM;
	}

	/*
	 * Linux qcom_pil_info_init() clears the region once on first
	 * access (memset_io).  Without this, stale boot data or
	 * previous crash info can defeat empty-slot detection.
	 */
	if (!qpas_pil_reloc_inited) {
		memset((void *)vaddr, 0, region_size);
		flush_cache(rounddown((ulong)vaddr, ARCH_DMA_MINALIGN),
			    roundup((ulong)vaddr + region_size,
				    ARCH_DMA_MINALIGN) -
			    rounddown((ulong)vaddr, ARCH_DMA_MINALIGN));
		qpas_pil_reloc_inited = true;
	}

	memset(name, 0, sizeof(name));
	strncpy((char *)name, image, QPAS_PIL_RELOC_NAME_LEN);

	num_entries = region_size / QPAS_PIL_RELOC_ENTRY_SIZE;

	for (i = 0; i < (int)num_entries; i++) {
		u8 *entry = (u8 *)vaddr + (i * QPAS_PIL_RELOC_ENTRY_SIZE);

		/*
		 * Linux checks only the first byte of the name field:
		 * a zero byte means the entry is unused.
		 */
		if (!entry[0]) {
			slot = i;
			break;
		}

		if (memcmp(entry, name, QPAS_PIL_RELOC_NAME_LEN) == 0) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		log_warning("qcom-adsp-pas: PIL reloc no free slot\n");
		unmap_sysmem(vaddr);
		return -ENOSPC;
	}

	{
		u8 *entry = (u8 *)vaddr + (slot * QPAS_PIL_RELOC_ENTRY_SIZE);

		/* Write name */
		memcpy(entry, name, QPAS_PIL_RELOC_NAME_LEN);

		/* Write base (little-endian u64 via two 32-bit writes) */
		writel((u32)base,
		       (void __iomem *)(entry + QPAS_PIL_RELOC_NAME_LEN));
		writel((u32)((u64)base >> 32),
		       (void __iomem *)(entry + QPAS_PIL_RELOC_NAME_LEN + 4));

		/* Write size (little-endian u32) */
		writel((u32)size,
		       (void __iomem *)(entry + QPAS_PIL_RELOC_NAME_LEN + 8));
	}

	flush_cache(rounddown((ulong)vaddr + slot * QPAS_PIL_RELOC_ENTRY_SIZE,
			      ARCH_DMA_MINALIGN),
		    roundup((ulong)vaddr + (slot + 1) * QPAS_PIL_RELOC_ENTRY_SIZE,
			    ARCH_DMA_MINALIGN) -
		    rounddown((ulong)vaddr + slot * QPAS_PIL_RELOC_ENTRY_SIZE,
			      ARCH_DMA_MINALIGN));

	unmap_sysmem(vaddr);

	log_warning("qcom-adsp-pas: PIL info image=%s base=%llx size=%zu slot=%d region=%llx+%zx\n",
		    image, (unsigned long long)base, size, slot,
		    (unsigned long long)res.start, region_size);

	return 0;
}

static int qcom_q6v5_pas_boot_node(struct qpas_proc *proc, struct udevice *dev,
				   ofnode node)
{
	struct qpas_power_domains pds = {};
	struct qpas_clocks clks = {};
	struct udevice *qmp_dev;
	struct qpas_fw fw = {};
	bool qmp_on = false;
	bool cdsp_proxy_on = false;
	const char *fw_name;
	phys_addr_t mem_phys;
	phys_addr_t reloc_base;
	void *mem_region;
	void *metadata_ctx = NULL;
	size_t mem_size;
	int ret;

	if (proc->booted)
		return 0;

	if (proc->failed) {
		log_warning("qcom-adsp-pas: previous %s boot failed, refusing retry\n",
			    proc->name);
		return -EIO;
	}

	/* Dump U-Boot memory layout to check for ADSP region overlap */
	log_warning("qcom-adsp-pas: U-Boot mem: ram_base=%llx ram_size=%llx ram_top=%llx\n",
		    (unsigned long long)gd->ram_base,
		    (unsigned long long)gd->ram_size,
		    (unsigned long long)gd->ram_top);
	log_warning("qcom-adsp-pas: U-Boot malloc: base=%llx limit=%x\n",
		    (unsigned long long)gd->malloc_base, gd->malloc_limit);

	/* Runtime DT check - U-Boot may fix up the DT at boot */
	log_warning("qcom-adsp-pas: dt has iommus=%d interconnects=%d\n",
		    ofnode_read_bool(node, "iommus"),
		    ofnode_read_bool(node, "interconnects"));

	/*
	 * Linux qcom_q6v5_pas start order:
	 *   1. qcom_q6v5_prepare()   - QMP load_state on
	 *   2. proxy power domains on (with INT_MAX perf state)
	 *   3. XO / aggre2 clocks on
	 *   4. qcom_mdt_pas_load()
	 *   5. qcom_scm_pas_auth_and_reset()
	 *   6. qcom_q6v5_wait_for_start()
	 *
	 * We follow the same order here.  On SC7280/QCM6490 the DT has
	 * qcom,qmp; failure to resolve/probe it is fatal.
	 */

	/*
	 * Step 0: SMP2P init.
	 *
	 * Linux's qcom_smp2p driver probes independently and creates the APSS
	 * outbound "master-kernel" SMEM item before remoteproc start. Do this
	 * before qcom_q6v5_prepare()/QMP load_state, not just before PAS auth.
	 */
	{
		struct qpas_smp2p_info smp2p_info = {};
		struct udevice *smem_dev;

		ret = uclass_first_device_err(UCLASS_SMEM, &smem_dev);
		if (ret) {
			log_warning("qcom-adsp-pas: SMEM lookup for SMP2P ret=%d\n",
				    ret);
			return ret;
		}

		ret = qpas_smp2p_init(smem_dev, node, &smp2p_info);
		log_warning("qcom-adsp-pas: SMP2P init ret=%d\n", ret);
		if (ret)
			return ret;
	}

	/*
	 * Linux qcom_q6v5_init() requests the remoteproc IRQs at probe time,
	 * and qcom_q6v5_prepare() enables handover IRQ before release. U-Boot
	 * has no Qualcomm PDC/SMP2P IRQ controller backend here, so record the
	 * same DT resources and rely on the SMEM/SMP2P polling path below for
	 * ready/fatal/handover/stop observation.
	 */
	qpas_log_q6v5_irq_resources(node);

	/* Step 1: QMP load_state on (Linux qcom_q6v5_prepare) */
	ret = qcom_aoss_qmp_get_by_node(node, &qmp_dev);
	if (ret) {
		log_warning("qcom-adsp-pas: QMP phandle lookup failed ret=%d (fatal)\n",
			    ret);
		return ret;
	}

	ret = qcom_aoss_qmp_load_state(qmp_dev, proc->load_state, true);
	log_warning("qcom-adsp-pas: QMP load_state %s on ret=%d\n",
		    proc->load_state, ret);
	if (ret) {
		return ret;
	}
	qmp_on = true;
	log_warning("qcom-adsp-pas: %s AOP QMP descriptor is firmware-side; settling load_state for %u ms\n",
		    proc->name, QPAS_AOP_LOAD_STATE_SETTLE_MS);
	mdelay(QPAS_AOP_LOAD_STATE_SETTLE_MS);

	/* Step 2: proxy power domains on */
	if (dev) {
		ret = qpas_enable_power_domains(dev, &pds);
		if (ret)
			goto out_qmp_off;
	} else {
		log_warning("qcom-adsp-pas: no bound device; skipping power domains\n");
	}

	/* Step 2b: CDSP Kodiak PIL proxy vote */
	if (proc == &qpas_cdsp_proc && ofnode_read_bool(node, "interconnects")) {
		struct udevice *rpmh_dev = pds.count ? pds.pd[0].dev : NULL;

		ret = qpas_cdsp_proxy_vote(rpmh_dev, true);
		if (ret)
			goto out_cdsp_proxy;
		cdsp_proxy_on = true;
	}

	/* Step 3: clocks on (XO, aggre2) */
	if (dev) {
		ret = qpas_enable_clocks(dev, &clks);
		if (ret)
			goto out_clocks;
	} else {
		log_warning("qcom-adsp-pas: no bound device; skipping clk_get_bulk\n");
	}

	/* Step 4: memory + firmware load */
	ret = qpas_get_memory_region(node, &mem_phys, &mem_size, &mem_region);
	if (ret) {
		log_warning("qcom-adsp-pas: memory-region lookup failed ret=%d\n",
			    ret);
		goto out_clocks;
	}

	fw_name = ofnode_read_string(node, "firmware-name");
	if (!fw_name)
		fw_name = proc->default_fw;

	log_warning("qcom-adsp-pas: boot start firmware=%s mem=%llx size=%zu\n",
		    fw_name, (unsigned long long)mem_phys, mem_size);

	/* Check if ADSP memory region overlaps U-Boot heap */
	if (mem_phys < gd->ram_top && (mem_phys + mem_size) > gd->malloc_base) {
		log_warning("qcom-adsp-pas: WARNING ADSP region [%llx-%llx] overlaps U-Boot memory!\n",
			    (unsigned long long)mem_phys,
			    (unsigned long long)(mem_phys + mem_size - 1));
	}

	/*
	 * Linux maps the ADSP carveout with devm_ioremap_resource_wc()
	 * (Normal Non-Cacheable Write-Combine).  Use DCACHE_WRITEBACK
	 * (Normal cacheable) with explicit flush+DSB to achieve the same
	 * end result: all writes reach physical memory before the ADSP
	 * starts executing.  DCACHE_OFF (Device-nGnRnE) is avoided
	 * because cache maintenance ops on Device memory have undefined
	 * behaviour on ARMv8, and Device memory write ordering can cause
	 * unexpected bus-level write decomposition.
	 */
	log_warning("qcom-adsp-pas: mapping %s region as write-back cacheable\n",
		    proc->name);
	mmu_set_region_dcache_behaviour(mem_phys, mem_size, DCACHE_WRITEBACK);

	/*
	 * Reserve the ADSP carveout in LMB so U-Boot does not allocate
	 * this range for the kernel, FDT, initramfs, or other purposes.
	 * Linux parity: the reserved-memory/adsp@86700000 node has no-map,
	 * which the kernel respects, but U-Boot's LMB must also know.
	 */
	lmb_reserve(mem_phys, mem_size, LMB_NONE);

	ret = qpas_read_firmware(&fw, fw_name);
	if (ret)
		goto out_unmap;

	if (!qpas_mdt_header_valid(&fw)) {
		log_warning("qcom-adsp-pas: invalid MDT header in %s\n", fw.path);
		ret = -EINVAL;
		goto out_free_fw;
	}

	ret = qpas_mdt_init_image(proc, &fw, mem_phys, &metadata_ctx);
	if (ret)
		goto out_free_fw;

	ret = qpas_mdt_load_segments(&fw, mem_region, mem_phys, mem_size,
				     &reloc_base);
	if (ret)
		goto out_free_metadata;

	log_warning("qcom-adsp-pas: load complete, segments=%zu reloc_base=%llx\n",
		    (size_t)((const Elf32_Ehdr *)fw.data)->e_phnum,
		    (unsigned long long)reloc_base);

	/*
	 * CRC the loaded segments to verify memory integrity with
	 * DCACHE_OFF mapping.  CRC only the used portion of the
	 * region (Linux: devm_ioremap_resource_wc produces WC, not
	 * cacheable).  If the CRC is stable and ADSP still fails,
	 * caching is ruled out.
	 */
	log_warning("qcom-adsp-pas: %s crc_after_load=%08x\n",
		    proc->load_state, crc32(0, mem_region, mem_size));

	/*
	 * PIL relocation info store (Linux parity: qcom_pil_info_store)
	 *
	 * Stores ADSP firmware base/size into IMEM pil-reloc-info region
	 * so post-mortem debugging tools can locate remoteproc images.
	 * Linux calls this between qcom_mdt_pas_load() and
	 * qcom_scm_pas_auth_and_reset().  Not fatal on failure.
	 */
	ret = qpas_pil_info_store(proc->pil_image, mem_phys, mem_size);
	log_warning("qcom-adsp-pas: PIL info store ret=%d\n", ret);
	if (ret)
		log_warning("qcom-adsp-pas: continuing despite PIL info failure\n");

	if (QCOM_ADSP_SKIP_BOOT) {
		log_warning("qcom-adsp-pas: SKIP_BOOT: bypassing auth_and_reset + SMP2P wait\n");
		proc->booted = true;
		ret = 0;
		goto out_free_metadata;
	}

	/*
	 * CRC the region again right before auth_and_reset, after PIL info
	 * store. If crc_before_auth != crc_after_load, something wrote to the
	 * ADSP region.
	 */
	log_warning("qcom-adsp-pas: %s crc_before_flush=%08x\n",
		    proc->load_state, crc32(0, mem_region, mem_size));

	/*
	 * Push all firmware data from dcaches to physical memory.
	 * DCACHE_WRITEBACK mapping means the per-segment flush_cache()
	 * calls clean each segment.  Do one final range flush of the
	 * entire carveout followed by a DSB to guarantee every write is
	 * globally observable before auth_and_reset.
	 */
	flush_dcache_range((ulong)mem_region,
			   (ulong)mem_region + mem_size);
	dsb();

	log_warning("qcom-adsp-pas: %s crc_before_auth=%08x\n",
		    proc->load_state, crc32(0, mem_region, mem_size));

	qpas_log_pre_release_state(proc, node);
	log_warning("qcom-adsp-pas: %s pre-release: not reading AOP-private QMP descriptor from APSS\n",
		    proc->name);

	/* Step 5: SCM auth_and_reset */
	log_warning("qcom-adsp-pas: preparing to boot %s\n", proc->name);
	ret = qcom_scm_pas_auth_and_reset(proc->pas_id);
	log_warning("qcom-adsp-pas: SCM auth_and_reset returned ret=%d\n", ret);
	if (ret)
		goto out_free_metadata;

	/* Step 6: poll SMP2P for ready/fatal/handover */
	ret = qpas_wait_for_start(proc, node, &clks, &pds);
	if (!ret)
		proc->booted = true;
	/* qpas_wait_for_start already shuts down on fatal/timeout. */

out_free_metadata:
	/*
	 * qcom_scm_pas_init_image allocates low-4GB non-cacheable
	 * metadata via LMB.  Free it via the helper (restores
	 * cacheable mapping, unmaps, frees LMB reservation).
	 */
	if (metadata_ctx)
		qpas_free_low_metadata();
out_free_fw:
	free(fw.data);
out_unmap:
	unmap_sysmem(mem_region);
out_clocks:
	if (ret)
		qpas_disable_clocks(&clks);
out_cdsp_proxy:
	if (ret && cdsp_proxy_on)
		qpas_cdsp_proxy_vote(pds.count ? pds.pd[0].dev : NULL, false);
out_power_domains:
	if (ret)
		qpas_disable_power_domains(&pds);
out_qmp_off:
	if (ret && qmp_on) {
		int qmp_off_ret;

		qmp_off_ret = qcom_aoss_qmp_load_state(qmp_dev,
						       proc->load_state, false);
		log_warning("qcom-adsp-pas: QMP load_state %s off ret=%d\n",
			    proc->load_state, qmp_off_ret);
	}
	return ret;
}

static int qcom_adsp_pas_probe(struct udevice *dev)
{
	struct qpas_proc *proc = (struct qpas_proc *)dev_get_driver_data(dev);

	return qcom_q6v5_pas_boot_node(proc, dev, dev_ofnode(dev));
}

static const struct udevice_id qcom_adsp_pas_ids[] = {
	{ .compatible = QCOM_ADSP_COMPAT, .data = (ulong)&qpas_adsp_proc },
	{ .compatible = QCOM_CDSP_COMPAT, .data = (ulong)&qpas_cdsp_proc },
	{ }
};

U_BOOT_DRIVER(qcom_adsp_pas) = {
	.name = "qcom_adsp_pas",
	.id = UCLASS_MISC,
	.of_match = qcom_adsp_pas_ids,
	.probe = qcom_adsp_pas_probe,
	.flags = DM_FLAG_DEFAULT_PD_CTRL_OFF,
};

static int qcom_q6v5_pas_boot(struct qpas_proc *proc)
{
	struct udevice *dev;
	ofnode node;
	int ret;

	if (proc->booted)
		return 0;

	if (proc->failed) {
		log_warning("qcom-adsp-pas: previous %s boot failed, refusing retry\n",
			    proc->name);
		return -EIO;
	}

	node = ofnode_by_compatible(ofnode_null(), proc->compat);
	if (!ofnode_valid(node)) {
		log_warning("qcom-adsp-pas: missing %s DT node\n",
			    proc->compat);
		return -ENOENT;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_MISC, node, &dev);
	if (!ret || proc->booted)
		return 0;

	if (proc->failed && ret == -EIO)
		return ret;

	log_warning("qcom-adsp-pas: DM device lookup/probe failed ret=%d\n", ret);
	return ret;
}

int qcom_adsp_pas_boot(void)
{
	return qcom_q6v5_pas_boot(&qpas_adsp_proc);
}

int qcom_cdsp_pas_boot(void)
{
	return qcom_q6v5_pas_boot(&qpas_cdsp_proc);
}
