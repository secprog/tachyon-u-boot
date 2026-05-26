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
#include <dm/uclass.h>
#include <elf.h>
#include <errno.h>
#include <fs.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <memalign.h>
#include <part.h>
#include <string.h>
#include <soc/qcom/qcom_adsp_pas.h>
#include <linux/arm-smccc.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/sizes.h>

#define QCOM_ADSP_PAS_ID			1
#define QCOM_ADSP_COMPAT			"qcom,sc7280-adsp-pas"
#define QCOM_ADSP_DEFAULT_FW			"adsp.mdt"
#define QCOM_MODEM_PARTITION			"modem_a"

#define QCOM_MDT_TYPE_MASK			(7 << 24)
#define QCOM_MDT_TYPE_HASH			(2 << 24)
#define QCOM_MDT_RELOCATABLE			BIT(27)

#define QCOM_SCM_MAX_ARGS			10
#define QCOM_SCM_MAX_RETS			3
#define QCOM_SCM_SVC_PIL			0x02
#define QCOM_SCM_PIL_PAS_INIT_IMAGE		0x01
#define QCOM_SCM_PIL_PAS_MEM_SETUP		0x02
#define QCOM_SCM_PIL_PAS_AUTH_AND_RESET		0x05
#define QCOM_SCM_V2_EBUSY			-12
#define QCOM_SCM_ENOMEM			-5
#define QCOM_SCM_EOPNOTSUPP			-4
#define QCOM_SCM_EINVAL_ADDR			-3
#define QCOM_SCM_EINVAL_ARG			-2
#define QCOM_SCM_ERROR				-1
#define QCOM_SCM_INTERRUPTED			1
#define QCOM_SCM_EBUSY_WAIT_MS			30
#define QCOM_SCM_EBUSY_MAX_RETRY		20

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
	char path[128];
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

static int qpas_find_modem_partition(struct blk_desc **descp, int *partp)
{
	struct disk_partition info;
	struct udevice *dev;
	struct blk_desc *desc;
	int part;

	uclass_foreach_dev_probe(UCLASS_BLK, dev) {
		desc = dev_get_uclass_plat(dev);
		if (!desc || desc->part_type == PART_TYPE_UNKNOWN)
			continue;

		part = part_get_info_by_name(desc, QCOM_MODEM_PARTITION, &info);
		if (part >= 0) {
			*descp = desc;
			*partp = part;
			log_warning("qcom-adsp-pas: found %s on blk devnum=%d part=%d\n",
				    QCOM_MODEM_PARTITION, desc->devnum, part);
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

	ret = fs_set_blk_dev_with_part(desc, part);
	if (ret)
		return ret;

	ret = fs_size(path, &size);
	if (ret)
		return ret;

	buf = memalign(ARCH_DMA_MINALIGN,
		       ALIGN(size + 1, ARCH_DMA_MINALIGN));
	if (!buf)
		return -ENOMEM;

	ret = fs_read(path, (ulong)buf, 0, size, &read);
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
			snprintf(fw->path, sizeof(fw->path), "%s", candidate[i]);
			return 0;
		}
	}

	return ret;
}

static int qpas_read_segment(const char *fw_path, unsigned int segment,
			     void *dst, size_t size)
{
	struct blk_desc *desc;
	char seg_path[128];
	int part;
	int len;
	int ret;
	void *buf;
	size_t read_size;

	ret = qpas_find_modem_partition(&desc, &part);
	if (ret)
		return ret;

	len = strlen(fw_path);
	if (len < 3 || len >= sizeof(seg_path))
		return -EINVAL;

	snprintf(seg_path, sizeof(seg_path), "%s", fw_path);
	snprintf(seg_path + len - 3, sizeof(seg_path) - len + 3,
		 "b%02u", segment);

	ret = qpas_read_file_exact(desc, part, seg_path, &buf, &read_size);
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
		ret = qpas_read_segment(fw->path, hash_segment,
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
			ret = qpas_read_segment(fw->path, i, ptr,
						phdr->p_filesz);
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

	*addrp = res.start;
	*sizep = resource_size(&res);
	*vaddrp = vaddr;
	return 0;
}

static int qcom_adsp_pas_boot_node(struct udevice *dev, ofnode node)
{
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

	if (dev) {
		ret = qpas_enable_clocks(dev);
		if (ret)
			return ret;
	} else {
		log_warning("qcom-adsp-pas: no bound device; skipping clk_get_bulk\n");
	}

	ret = qpas_get_memory_region(node, &mem_phys, &mem_size, &mem_region);
	if (ret) {
		log_warning("qcom-adsp-pas: memory-region lookup failed ret=%d\n",
			    ret);
		return ret;
	}

	fw_name = ofnode_read_string(node, "firmware-name");
	if (!fw_name)
		fw_name = QCOM_ADSP_DEFAULT_FW;

	log_warning("qcom-adsp-pas: boot start firmware=%s mem=%llx size=%zu\n",
		    fw_name, (unsigned long long)mem_phys, mem_size);

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

	ret = qcom_scm_pas_auth_and_reset(QCOM_ADSP_PAS_ID);
	if (ret)
		goto out_free_metadata;

	qpas_booted = true;
	log_warning("qcom-adsp-pas: booted ADSP reloc_base=%llx\n",
		    (unsigned long long)reloc_base);

out_free_metadata:
	free(metadata_ctx);
out_free_fw:
	free(fw.data);
out_unmap:
	unmap_sysmem(mem_region);
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
