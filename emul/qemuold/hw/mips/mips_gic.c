/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012  MIPS Technologies, Inc.  All rights reserved.
 * Authors: Sanjay Lal <sanjayl@kymasys.com>
 *
 * Copyright (C) 2015 Imagination Technologies
 */

#include "hw/hw.h"
#include "qemu/bitmap.h"
#include "exec/memory.h"
#include "sysemu/sysemu.h"
#include "qom/cpu.h"
#include "exec/address-spaces.h"

#ifdef CONFIG_KVM
#include "sysemu/kvm.h"
#include "kvm_mips.h"
#endif

#ifdef CONFIG_ESESC
#include "linux-user/esesc_qemu.h"
#endif

#include "hw/mips/mips_gic.h"
#include "hw/mips/mips_gcmpregs.h"

/* #define DEBUG */

#ifdef DEBUG
#define DPRINTF(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

#define TIMER_PERIOD 10 /* 10 ns period for 100 Mhz frequency */

/* Support up to 32 VPEs */
#define NUMVPES     32

typedef struct gic_timer_t {
    QEMUTimer *timer;
    uint32_t vp_index;
    struct gic_t *gic;
} gic_timer_t;

struct gic_t {
    CPUMIPSState *env[NUMVPES];
    MemoryRegion gic_mem;
    qemu_irq *irqs;

    /* GCR Registers */
    target_ulong gcr_gic_base;

    /* Shared Section Registers */
    uint32_t gic_gl_config;
    uint32_t gic_gl_intr_pol_reg[8];
    uint32_t gic_gl_intr_trigtype_reg[8];
    uint32_t gic_gl_intr_pending_reg[8];
    uint32_t gic_gl_intr_mask_reg[8];
    uint32_t gic_sh_counterlo;

    uint32_t gic_gl_map_pin[256];

    /* Sparse array, need a better way */
    uint32_t gic_gl_map_vpe[0x7fa];

    /* VPE Local Section Registers */
    /* VPE Other Section Registers, aliased to local,
     * use the other addr to access the correct instance */
    uint32_t gic_vpe_ctl[NUMVPES];
    uint32_t gic_vpe_pend[NUMVPES];
    uint32_t gic_vpe_mask[NUMVPES];
    uint32_t gic_vpe_wd_map[NUMVPES];
    uint32_t gic_vpe_compare_map[NUMVPES];
    uint32_t gic_vpe_timer_map[NUMVPES];
    uint32_t gic_vpe_comparelo[NUMVPES];
    uint32_t gic_vpe_comparehi[NUMVPES];

    uint32_t gic_vpe_other_addr[NUMVPES];

    /* User Mode Visible Section Registers */

    uint32_t num_cpu;
    gic_timer_t *gic_timer;
};

struct gcr_t {
    MemoryRegion gcr_mem;
    gic_t *gic;
};

static inline int gic_get_current_cpu(gic_t *g)
{
    if (g->num_cpu > 1) {
        return current_cpu->cpu_index;
    }
    return 0;
}

/* GIC VPE Local Timer */
static uint32_t gic_vpe_timer_update(gic_t *gic, uint32_t vp_index)
{
    uint64_t now, next;
    uint32_t wait;

    now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    wait = gic->gic_vpe_comparelo[vp_index] - gic->gic_sh_counterlo -
            (uint32_t)(now / TIMER_PERIOD);
    next = now + (uint64_t)wait * TIMER_PERIOD;
    timer_mod(gic->gic_timer[vp_index].timer, next);
    return wait;
}

static void gic_vpe_timer_expire(gic_t *gic, uint32_t vp_index)
{
    uint32_t pin;
    gic_vpe_timer_update(gic, vp_index);
    gic->gic_vpe_pend[vp_index] |= (1 << 1);

    pin = (gic->gic_vpe_compare_map[vp_index] & 0x3F) + 2;
    if (gic->gic_vpe_pend[vp_index] &
            (gic->gic_vpe_mask[vp_index] & (1 << 1))) {
        if (gic->gic_vpe_compare_map[vp_index] & 0x80000000) {
            qemu_irq_raise(gic->env[vp_index]->irq[pin]);
        }
    }
}

static uint32_t gic_get_sh_count(gic_t *gic)
{
    int i;
    if (gic->gic_gl_config & (1 << 28)) {
        return gic->gic_sh_counterlo;
    } else {
        uint64_t now;
        now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        for (i = 0; i < gic->num_cpu; i++) {
            if (timer_pending(gic->gic_timer[i].timer)
                && timer_expired(gic->gic_timer[i].timer, now)) {
                /* The timer has already expired.  */
                gic_vpe_timer_expire(gic, i);
            }
        }
        return gic->gic_sh_counterlo + (uint32_t)(now / TIMER_PERIOD);
    }
}

static void gic_store_sh_count(gic_t *gic, uint64_t count)
{
    int i;
    DPRINTF("QEMU: gic_store_count %lx\n", count);

    if ((gic->gic_gl_config & 0x10000000) || !gic->gic_timer) {
        gic->gic_sh_counterlo = count;
    } else {
        /* Store new count register */
        gic->gic_sh_counterlo = count -
            (uint32_t)(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) / TIMER_PERIOD);
        /* Update timer timer */
        for (i = 0; i < gic->num_cpu; i++) {
            gic_vpe_timer_update(gic, i);
        }
    }
}

static void gic_store_vpe_compare(gic_t *gic, uint32_t vp_index,
                                  uint64_t compare)
{
    uint32_t wait;
    gic->gic_vpe_comparelo[vp_index] = (uint32_t) compare;
    wait = gic_vpe_timer_update(gic, vp_index);

    DPRINTF("GIC Compare modified (GIC_VPE%d_Compare=0x%x GIC_Counter=0x%x) "
            "- schedule CMP timer interrupt after 0x%x\n",
            vp_index,
            gic->gic_vpe_comparelo[vp_index], gic->gic_sh_counterlo,
            wait);

    gic->gic_vpe_pend[vp_index] &= ~(1 << 1);
    if (gic->gic_vpe_compare_map[vp_index] & 0x80000000) {
        uint32_t irq_num = (gic->gic_vpe_compare_map[vp_index] & 0x3F) + 2;
        qemu_set_irq(gic->env[vp_index]->irq[irq_num], 0);
    }
}

static void gic_vpe_timer_cb(void *opaque)
{
    gic_timer_t *gic_timer = opaque;
    gic_timer->gic->gic_sh_counterlo++;
    gic_vpe_timer_expire(gic_timer->gic, gic_timer->vp_index);
    gic_timer->gic->gic_sh_counterlo--;
}

static void gic_timer_start_count(gic_t *gic)
{
    DPRINTF("QEMU: GIC timer starts count\n");
    gic_store_sh_count(gic, gic->gic_sh_counterlo);
}

static void gic_timer_stop_count(gic_t *gic)
{
    int i;

    DPRINTF("QEMU: GIC timer stops count\n");
    /* Store the current value */
    gic->gic_sh_counterlo +=
        (uint32_t)(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) / TIMER_PERIOD);
    for (i = 0; i < gic->num_cpu; i++) {
        timer_del(gic->gic_timer[i].timer);
    }
}

static void gic_timer_init(gic_t *gic, uint32_t ncpus)
{
    int i;
    gic->gic_timer = (void *) g_malloc0(sizeof(gic_timer_t) * ncpus);
    for (i = 0; i < ncpus; i++) {
        gic->gic_timer[i].gic = gic;
        gic->gic_timer[i].vp_index = i;
        gic->gic_timer[i].timer = timer_new_ns(QEMU_CLOCK_VIRTUAL,
                                               &gic_vpe_timer_cb,
                                               &gic->gic_timer[i]);
    }
    gic_store_sh_count(gic, gic->gic_sh_counterlo);
}

/* GIC Read VPE Local/Other Registers */
static uint64_t gic_read_vpe(gic_t *gic, uint32_t vp_index, hwaddr addr,
                             unsigned size)
{
    switch (addr) {
    case GIC_VPE_CTL_OFS:
        DPRINTF("(GIC_VPE_CTL) -> 0x%016x\n", gic->gic_vpe_ctl[vp_index]);
        return gic->gic_vpe_ctl[vp_index];
    case GIC_VPE_PEND_OFS:
        gic_get_sh_count(gic);
        DPRINTF("(GIC_VPE_PEND) -> 0x%016x\n", gic->gic_vpe_pend[vp_index]);
        return gic->gic_vpe_pend[vp_index];
    case GIC_VPE_MASK_OFS:
        DPRINTF("(GIC_VPE_MASK) -> 0x%016x\n", gic->gic_vpe_mask[vp_index]);
        return gic->gic_vpe_mask[vp_index];
    case GIC_VPE_WD_MAP_OFS:
        return gic->gic_vpe_wd_map[vp_index];
    case GIC_VPE_COMPARE_MAP_OFS:
        return gic->gic_vpe_compare_map[vp_index];
    case GIC_VPE_TIMER_MAP_OFS:
        return gic->gic_vpe_timer_map[vp_index];
    case GIC_VPE_OTHER_ADDR_OFS:
        DPRINTF("(GIC_VPE_OTHER_ADDR) -> 0x%016x\n",
                gic->gic_vpe_other_addr[vp_index]);
        return gic->gic_vpe_other_addr[vp_index];
    case GIC_VPE_IDENT_OFS:
        return vp_index;
    case GIC_VPE_COMPARE_LO_OFS:
        DPRINTF("(GIC_VPE_COMPARELO) -> 0x%016x\n",
                gic->gic_vpe_comparelo[vp_index]);
        return gic->gic_vpe_comparelo[vp_index];
    case GIC_VPE_COMPARE_HI_OFS:
        DPRINTF("(GIC_VPE_COMPAREhi) -> 0x%016x\n",
                gic->gic_vpe_comparehi[vp_index]);
        return gic->gic_vpe_comparehi[vp_index];
    default:
        DPRINTF("Warning *** read %d bytes at GIC offset LOCAL/OTHER 0x%"
                PRIx64 "\n",
                size, addr);
        break;
    }
    return 0;
}

static uint64_t gic_read(void *opaque, hwaddr addr, unsigned size)
{
    int reg;
    gic_t *gic = (gic_t *) opaque;
    uint32_t vp_index = gic_get_current_cpu(gic);
    uint32_t ret = 0;

    DPRINTF("Info read %d bytes at GIC offset 0x%" PRIx64,
            size, addr);

    switch (addr) {
    case GIC_SH_CONFIG_OFS:
        DPRINTF("(GIC_SH_CONFIG) -> 0x%016x\n", gic->gic_gl_config);
        return gic->gic_gl_config;
    case GIC_SH_CONFIG_OFS + 4:
        /* do nothing */
        return 0;
    case GIC_SH_COUNTERLO_OFS:
    {
        ret = gic_get_sh_count(gic);
        DPRINTF("(GIC_SH_COUNTERLO) -> 0x%016x\n", ret);
        return ret;
    }
    case GIC_SH_COUNTERHI_OFS:
        DPRINTF("(Not supported GIC_SH_COUNTERHI) -> 0x%016x\n", 0);
        return 0;
    case GIC_SH_POL_31_0_OFS:
    case GIC_SH_POL_63_32_OFS:
    case GIC_SH_POL_95_64_OFS:
    case GIC_SH_POL_127_96_OFS:
    case GIC_SH_POL_159_128_OFS:
    case GIC_SH_POL_191_160_OFS:
    case GIC_SH_POL_223_192_OFS:
    case GIC_SH_POL_255_224_OFS:
        reg = (addr - GIC_SH_POL_31_0_OFS) / 4;
        ret = gic->gic_gl_intr_pol_reg[reg];
        DPRINTF("(GIC_SH_POL) -> 0x%016x\n", ret);
        return ret;
    case GIC_SH_TRIG_31_0_OFS:
    case GIC_SH_TRIG_63_32_OFS:
    case GIC_SH_TRIG_95_64_OFS:
    case GIC_SH_TRIG_127_96_OFS:
    case GIC_SH_TRIG_159_128_OFS:
    case GIC_SH_TRIG_191_160_OFS:
    case GIC_SH_TRIG_223_192_OFS:
    case GIC_SH_TRIG_255_224_OFS:
        reg = (addr - GIC_SH_TRIG_31_0_OFS) / 4;
        ret = gic->gic_gl_intr_trigtype_reg[reg];
        DPRINTF("(GIC_SH_TRIG) -> 0x%016x\n", ret);
        return ret;
    case GIC_SH_PEND_31_0_OFS:
    case GIC_SH_PEND_63_32_OFS:
    case GIC_SH_PEND_95_64_OFS:
    case GIC_SH_PEND_127_96_OFS:
    case GIC_SH_PEND_159_128_OFS:
    case GIC_SH_PEND_191_160_OFS:
    case GIC_SH_PEND_223_192_OFS:
    case GIC_SH_PEND_255_224_OFS:
        reg = (addr - GIC_SH_PEND_31_0_OFS) / 4;
        ret = gic->gic_gl_intr_pending_reg[reg];
        DPRINTF("(GIC_SH_PEND) -> 0x%016x\n", ret);
        return ret;
    case GIC_SH_MASK_31_0_OFS:
    case GIC_SH_MASK_63_32_OFS:
    case GIC_SH_MASK_95_64_OFS:
    case GIC_SH_MASK_127_96_OFS:
    case GIC_SH_MASK_159_128_OFS:
    case GIC_SH_MASK_191_160_OFS:
    case GIC_SH_MASK_223_192_OFS:
    case GIC_SH_MASK_255_224_OFS:
        reg = (addr - GIC_SH_MASK_31_0_OFS) / 4;
        ret =  gic->gic_gl_intr_mask_reg[reg];
        DPRINTF("(GIC_SH_MASK) -> 0x%016x\n", ret);
        return ret;
    default:
        if (addr < GIC_SH_INTR_MAP_TO_PIN_BASE_OFS) {
            DPRINTF("Warning *** read %d bytes at GIC offset 0x%" PRIx64 "\n",
                    size, addr);
        }
        break;
    }

    /* Global Interrupt Map SrcX to Pin register */
    if (addr >= GIC_SH_INTR_MAP_TO_PIN_BASE_OFS
        && addr <= GIC_SH_MAP_TO_PIN(255)) {
        reg = (addr - GIC_SH_INTR_MAP_TO_PIN_BASE_OFS) / 4;
        ret = gic->gic_gl_map_pin[reg];
        DPRINTF("(GIC) -> 0x%016x\n", ret);
        return ret;
    }

    /* Global Interrupt Map SrcX to VPE register */
    if (addr >= GIC_SH_INTR_MAP_TO_VPE_BASE_OFS
        && addr <= GIC_SH_MAP_TO_VPE_REG_OFF(255, 63)) {
        reg = (addr - GIC_SH_INTR_MAP_TO_VPE_BASE_OFS) / 4;
        ret = gic->gic_gl_map_vpe[reg];
        DPRINTF("(GIC) -> 0x%016x\n", ret);
        return ret;
    }

    /* VPE-Local Register */
    if (addr >= GIC_VPELOCAL_BASE_ADDR && addr < GIC_VPEOTHER_BASE_ADDR) {
        return gic_read_vpe(gic, vp_index, addr - GIC_VPELOCAL_BASE_ADDR, size);
    }

    /* VPE-Other Register */
    if (addr >= GIC_VPEOTHER_BASE_ADDR && addr < GIC_USERMODE_BASE_ADDR) {
        uint32_t other_index = gic->gic_vpe_other_addr[vp_index];
        return gic_read_vpe(gic, other_index, addr - GIC_VPEOTHER_BASE_ADDR,
                            size);
    }

    DPRINTF("GIC unimplemented register %" PRIx64 "\n", addr);
    return 0ULL;
}

/* GIC Write VPE Local/Other Registers */
static void gic_write_vpe(gic_t *gic, uint32_t vp_index, hwaddr addr,
                              uint64_t data, unsigned size)
{
    switch (addr) {
    case GIC_VPE_CTL_OFS:
        gic->gic_vpe_ctl[vp_index] &= ~1;
        gic->gic_vpe_ctl[vp_index] |= data & 1;

        DPRINTF("QEMU: GIC_VPE%d_CTL Write %lx\n", vp_index, data);
        break;
    case GIC_VPE_RMASK_OFS:
        gic->gic_vpe_mask[vp_index] &= ~(data & 0x3f) & 0x3f;

        DPRINTF("QEMU: GIC_VPE%d_RMASK Write data %lx, mask %x\n", vp_index,
                data, gic->gic_vpe_mask[vp_index]);
        break;
    case GIC_VPE_SMASK_OFS:
        gic->gic_vpe_mask[vp_index] |= (data & 0x3f);

        DPRINTF("QEMU: GIC_VPE%d_SMASK Write data %lx, mask %x\n", vp_index,
                data, gic->gic_vpe_mask[vp_index]);
        break;
    case GIC_VPE_WD_MAP_OFS:
        gic->gic_vpe_wd_map[vp_index] = data & 0xE000003F;
        break;
    case GIC_VPE_COMPARE_MAP_OFS:
        gic->gic_vpe_compare_map[vp_index] = data & 0xE000003F;

        DPRINTF("QEMU: GIC_VPE%d_COMPARE_MAP %lx %x\n", vp_index,
                data, gic->gic_vpe_compare_map[vp_index]);
        break;
    case GIC_VPE_TIMER_MAP_OFS:
        gic->gic_vpe_timer_map[vp_index] = data & 0xE000003F;

        DPRINTF("QEMU: GIC Timer MAP %lx %x\n", data,
                gic->gic_vpe_timer_map[vp_index]);
        break;
    case GIC_VPE_OTHER_ADDR_OFS:
        if (data < gic->num_cpu) {
            gic->gic_vpe_other_addr[vp_index] = data;
        }

        DPRINTF("QEMU: GIC other addressing reg WRITE %lx\n", data);
        break;
    case GIC_VPE_OTHER_ADDR_OFS + 4:
        /* do nothing */
        break;
    case GIC_VPE_COMPARE_LO_OFS:
        gic_store_vpe_compare(gic, vp_index, data);
        break;
    case GIC_VPE_COMPARE_HI_OFS:
        /* do nothing */
        break;
    default:
        DPRINTF("Warning *** write %d bytes at GIC offset LOCAL/OTHER "
                "0x%" PRIx64" 0x%08lx\n", size, addr, data);
        break;
    }
}

static void gic_write(void *opaque, hwaddr addr, uint64_t data, unsigned size)
{
    int reg, intr;
    gic_t *gic = (gic_t *) opaque;
    uint32_t vp_index = gic_get_current_cpu(gic);

    switch (addr) {
    case GIC_SH_CONFIG_OFS:
    {
        uint32_t pre = gic->gic_gl_config;
        gic->gic_gl_config = (gic->gic_gl_config & 0xEFFFFFFF) |
                             (data & 0x10000000);
        if (pre != gic->gic_gl_config) {
            if ((gic->gic_gl_config & 0x10000000)) {
                DPRINTF("Info GIC_SH_CONFIG.COUNTSTOP modified STOPPING\n");
                gic_timer_stop_count(gic);
            }
            if (!(gic->gic_gl_config & 0x10000000)) {
                DPRINTF("Info GIC_SH_CONFIG.COUNTSTOP modified STARTING\n");
                gic_timer_start_count(gic);
            }
        }
    }
        break;
    case GIC_SH_CONFIG_OFS + 4:
        /* do nothing */
        break;
    case GIC_SH_COUNTERLO_OFS:
        if (gic->gic_gl_config & 0x10000000) {
            gic_store_sh_count(gic, data);
        }
        break;
    case GIC_SH_COUNTERHI_OFS:
        /* do nothing */
        break;
    case GIC_SH_POL_31_0_OFS:
    case GIC_SH_POL_63_32_OFS:
    case GIC_SH_POL_95_64_OFS:
    case GIC_SH_POL_127_96_OFS:
    case GIC_SH_POL_159_128_OFS:
    case GIC_SH_POL_191_160_OFS:
    case GIC_SH_POL_223_192_OFS:
    case GIC_SH_POL_255_224_OFS:
        reg = (addr - GIC_SH_POL_31_0_OFS) / 4;
        gic->gic_gl_intr_pol_reg[reg] = data;
        break;
    case GIC_SH_TRIG_31_0_OFS:
    case GIC_SH_TRIG_63_32_OFS:
    case GIC_SH_TRIG_95_64_OFS:
    case GIC_SH_TRIG_127_96_OFS:
    case GIC_SH_TRIG_159_128_OFS:
    case GIC_SH_TRIG_191_160_OFS:
    case GIC_SH_TRIG_223_192_OFS:
    case GIC_SH_TRIG_255_224_OFS:
        reg = (addr - GIC_SH_TRIG_31_0_OFS) / 4;
        gic->gic_gl_intr_trigtype_reg[reg] = data;
        break;
    case GIC_SH_RMASK_31_0_OFS:
    case GIC_SH_RMASK_63_32_OFS:
    case GIC_SH_RMASK_95_64_OFS:
    case GIC_SH_RMASK_127_96_OFS:
    case GIC_SH_RMASK_159_128_OFS:
    case GIC_SH_RMASK_191_160_OFS:
    case GIC_SH_RMASK_223_192_OFS:
    case GIC_SH_RMASK_255_224_OFS:
        reg = (addr - GIC_SH_RMASK_31_0_OFS) / 4;
        gic->gic_gl_intr_mask_reg[reg] &= ~data;
        break;
    case GIC_SH_WEDGE_OFS:
        DPRINTF("addr: %#" PRIx64 ", data: %#" PRIx64 ", size: %#x\n", addr,
               data, size);
        /* Figure out which VPE/HW Interrupt this maps to */
        intr = data & 0x7FFFFFFF;
        /* Mask/Enabled Checks */
        if (data & 0x80000000) {
            qemu_set_irq(gic->irqs[intr], 1);
        } else {
            qemu_set_irq(gic->irqs[intr], 0);
        }
        break;
    case GIC_SH_PEND_31_0_OFS:
    case GIC_SH_PEND_63_32_OFS:
    case GIC_SH_PEND_95_64_OFS:
    case GIC_SH_PEND_127_96_OFS:
    case GIC_SH_PEND_159_128_OFS:
    case GIC_SH_PEND_191_160_OFS:
    case GIC_SH_PEND_223_192_OFS:
    case GIC_SH_PEND_255_224_OFS:
        break;

    case GIC_SH_SMASK_31_0_OFS:
    case GIC_SH_SMASK_63_32_OFS:
    case GIC_SH_SMASK_95_64_OFS:
    case GIC_SH_SMASK_127_96_OFS:
    case GIC_SH_SMASK_159_128_OFS:
    case GIC_SH_SMASK_191_160_OFS:
    case GIC_SH_SMASK_223_192_OFS:
    case GIC_SH_SMASK_255_224_OFS:
        reg = (addr - GIC_SH_SMASK_31_0_OFS) / 4;
        gic->gic_gl_intr_mask_reg[reg] |= data;
        break;

    default:
        if (addr < GIC_SH_INTR_MAP_TO_PIN_BASE_OFS) {
            DPRINTF("Warning *** write %d bytes at GIC offset 0x%" PRIx64
                    " 0x%08lx\n",
                    size, addr, data);
        }
        break;
    }

    /* Other cases */
    if (addr >= GIC_SH_INTR_MAP_TO_PIN_BASE_OFS
        && addr <= GIC_SH_MAP_TO_PIN(255)) {
        reg = (addr - GIC_SH_INTR_MAP_TO_PIN_BASE_OFS) / 4;
        gic->gic_gl_map_pin[reg] = data;
    }
    if (addr >= GIC_SH_INTR_MAP_TO_VPE_BASE_OFS
        && addr <= GIC_SH_MAP_TO_VPE_REG_OFF(255, 63)) {
        reg = (addr - GIC_SH_INTR_MAP_TO_VPE_BASE_OFS) / 4;
        gic->gic_gl_map_vpe[reg] = data;
    }

    /* VPE-Local Register */
    if (addr >= GIC_VPELOCAL_BASE_ADDR && addr < GIC_VPEOTHER_BASE_ADDR) {
        gic_write_vpe(gic, vp_index, addr - GIC_VPELOCAL_BASE_ADDR,
                      data, size);
    }

    /* VPE-Other Register */
    if (addr >= GIC_VPEOTHER_BASE_ADDR && addr < GIC_USERMODE_BASE_ADDR) {
        uint32_t other_index = gic->gic_vpe_other_addr[vp_index];
        gic_write_vpe(gic, other_index, addr - GIC_VPEOTHER_BASE_ADDR,
                      data, size);
    }
}

static void gic_reset(void *opaque)
{
    int i;
    gic_t *gic = (gic_t *) opaque;

    /* Rest value is map to pin */
    for (i = 0; i < 256; i++) {
        gic->gic_gl_map_pin[i] = GIC_MAP_TO_PIN_MSK;
    }

    gic->gic_sh_counterlo = 0;

    gic->gic_gl_config = 0x180f0000 | gic->num_cpu;
}

static void gic_set_irq(void *opaque, int n_IRQ, int level)
{
    int vpe = -1, pin = -1, i;
    gic_t *gic = (gic_t *) opaque;

    pin = gic->gic_gl_map_pin[n_IRQ] & 0x7;

    for (i = 0; i < NUMVPES; i++) {
        vpe = gic->gic_gl_map_vpe[(GIC_SH_MAP_TO_VPE_REG_OFF(n_IRQ, i) -
                                   GIC_SH_INTR_MAP_TO_VPE_BASE_OFS) / 4];
        if (vpe & GIC_SH_MAP_TO_VPE_REG_BIT(i)) {
            vpe = i;
            break;
        }
    }

    if (pin >= 0 && vpe >= 0) {
        int offset;
        DPRINTF("[%s] INTR %d maps to PIN %d on VPE %d\n",
                (level ? "ASSERT" : "DEASSERT"), n_IRQ, pin, vpe);
        /* Set the Global PEND register */
        offset = GIC_INTR_OFS(n_IRQ) / 4;
        if (level) {
            gic->gic_gl_intr_pending_reg[offset] |= (1 << GIC_INTR_BIT(n_IRQ));
        } else {
            gic->gic_gl_intr_pending_reg[offset] &= ~(1 << GIC_INTR_BIT(n_IRQ));
        }

#ifdef CONFIG_KVM
        if (kvm_enabled())  {
            kvm_mips_set_ipi_interrupt(gic->env[vpe], pin + 2, level);
        }
#endif
        qemu_set_irq(gic->env[vpe]->irq[pin+2], level);
    }
}

/* Read GCR registers */
static uint64_t gcr_read(void *opaque, hwaddr addr, unsigned size)
{
    gic_t *gic = (gic_t *) opaque;

    DPRINTF("Info read %d bytes at GCR offset 0x%" PRIx64 " (GCR) -> ",
            size, addr);

    switch (addr) {
    case GCMP_GCB_GC_OFS:
        /* Set PCORES to 0 */
        DPRINTF("0x%016x\n", 0);
        return 0;
    case GCMP_GCB_GCMPB_OFS:
        DPRINTF("GCMP_BASE_ADDR: %016llx\n", GCMP_BASE_ADDR);
        return GCMP_BASE_ADDR;
    case GCMP_GCB_GCMPREV_OFS:
        DPRINTF("0x%016x\n", 0x800);
        return 0x800;
    case GCMP_GCB_GICBA_OFS:
        DPRINTF("0x" TARGET_FMT_lx "\n", gic->gcr_gic_base);
        return gic->gcr_gic_base;
    case GCMP_GCB_GICST_OFS:
        /* FIXME indicates a connection between GIC and CM */
        DPRINTF("0x%016x\n", GCMP_GCB_GICST_EX_MSK);
        return GCMP_GCB_GICST_EX_MSK;
    case GCMP_GCB_CPCST_OFS:
        DPRINTF("0x%016x\n", 0);
        return 0;

    case GCMP_CLCB_OFS + GCMP_CCB_CFG_OFS:
        /* Set PVP to # cores - 1 */
        DPRINTF("0x%016x\n", smp_cpus - 1);
        return smp_cpus - 1;
    case GCMP_CLCB_OFS + GCMP_CCB_OTHER_OFS:
        DPRINTF("0x%016x\n", 0);
        return 0;
    default:
        DPRINTF("Warning *** unimplemented GCR read at offset 0x%" PRIx64 "\n",
                addr);
        return 0;
    }
    return 0ULL;
}

/* Write GCR registers */
static void gcr_write(void *opaque, hwaddr addr, uint64_t data, unsigned size)
{
    gcr_t *gcr = (gcr_t *) opaque;
    gic_t *gic = gcr->gic;

    switch (addr) {
    case GCMP_GCB_GICBA_OFS:
        DPRINTF("Info write %d bytes at GCR offset %" PRIx64 " <- 0x%016lx\n",
                size, addr, data);
        gic->gcr_gic_base = data & ~1;
        if (data & 1) {
            memory_region_del_subregion(get_system_memory(),
                                        &gic->gic_mem);
            memory_region_add_subregion(get_system_memory(),
                                        gic->gcr_gic_base,
                                        &gic->gic_mem);
            DPRINTF("init gic base addr %lx " TARGET_FMT_lx "\n",
                    data, gic->gcr_gic_base);
            gic_store_sh_count(gic, gic->gic_sh_counterlo);
        }
        break;
    default:
        DPRINTF("Warning *** unimplemented GCR write at offset 0x%" PRIx64 "\n",
                addr);
        break;
    }
}

static const MemoryRegionOps gic_ops = {
    .read = gic_read,
    .write = gic_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const MemoryRegionOps gcr_ops = {
    .read = gcr_read,
    .write = gcr_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

/* Initialise GCR (Just enough to support GIC) */
gcr_t *gcr_init(uint32_t ncpus, CPUState *cs, MemoryRegion * address_space,
                qemu_irq **gic_irqs)
{
    gcr_t *gcr;
    gcr = (gcr_t *) g_malloc0(sizeof(gcr_t));

    if (!gcr) {
        fprintf(stderr, "Not enough memory %s\n", __func__);
        return 0;
    }

    /* Register GCR regions */
    memory_region_init_io(&gcr->gcr_mem, NULL, &gcr_ops, gcr, "GCR",
                          GCMP_ADDRSPACE_SZ);
    /* The MIPS default location for the GCR_BASE address is 0x1FBF_8. */
    memory_region_add_subregion(address_space, GCMP_BASE_ADDR, &gcr->gcr_mem);

    /* initialising GIC */
    gcr->gic = (gic_t *) gic_init(ncpus, cs, address_space);
    *gic_irqs = gcr->gic->irqs;
    return gcr;
}

/* Initialise GIC */
gic_t *gic_init(uint32_t ncpus, CPUState *cs, MemoryRegion *address_space)
{
    gic_t *gic;
    uint32_t i;

    if (ncpus > NUMVPES) {
        fprintf(stderr, "Unable to initialise GIC - ncpus %d > NUMVPES!",
                ncpus);
        return NULL;
    }

    gic = (gic_t *) g_malloc0(sizeof(gic_t));
    gic->num_cpu = ncpus;

    /* Register the CPU env for all cpus with the GIC */
    for (i = 0; i < ncpus; i++) {
        if (cs != NULL) {
            gic->env[i] = cs->env_ptr;
            cs = CPU_NEXT(cs);
#ifdef CONFIG_ESESC
            if (cs)
              cs->fid = QEMUReader_resumeThread(cs->host_tid, UINT32_MAX);
#endif
        } else {
            fprintf(stderr, "Unable to initialize GIC - CPUState for "
                    "CPU #%d not valid!", i);
            return NULL;
        }
    }

    /* Register GIC regions */
    memory_region_init_io(&gic->gic_mem, NULL, &gic_ops, gic, "GIC",
                          GIC_ADDRSPACE_SZ);
    memory_region_add_subregion(address_space, GIC_BASE_ADDR, &gic->gic_mem);
    qemu_register_reset(gic_reset, gic);

    gic->irqs = qemu_allocate_irqs(gic_set_irq, gic, GIC_NUM_INTRS);
    gic_timer_init(gic, ncpus);
    return gic;
}
