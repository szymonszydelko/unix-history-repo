/*
 * Copyright (c) 2000, 2001 Andrew Gallatin & Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Portions of this file were obtained from Compaq intellectual
 * property which was made available under the following copyright:
 *
 * *****************************************************************
 * *                                                               *
 * *    Copyright Compaq Computer Corporation, 2000                *
 * *                                                               *
 * *   Permission to use, copy, modify, distribute, and sell       *
 * *   this software and its documentation for any purpose is      *
 * *   hereby granted without fee, provided that the above         *
 * *   copyright notice appear in all copies and that both         *
 * *   that copyright notice and this permission notice appear     *
 * *   in supporting documentation, and that the name of           *
 * *   Compaq Computer Corporation not be used in advertising      *
 * *   or publicity pertaining to distribution of the software     *
 * *   without specific, written prior permission.  Compaq         *
 * *   makes no representations about the suitability of this      *
 * *   software for any purpose.  It is provided "AS IS"           *
 * *   without express or implied warranty.                        *
 * *                                                               *
 * *****************************************************************
 *
 * $FreeBSD$
 */

/*
 * T2 CBUS to PCI bridge
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/rman.h>

#include <alpha/pci/t2reg.h>
#include <alpha/pci/t2var.h>
#include <alpha/pci/pcibus.h>
#include <alpha/isa/isavar.h>
#include <machine/intr.h>
#include <machine/resource.h>
#include <machine/intrcnt.h>
#include <machine/cpuconf.h>
#include <machine/swiz.h>
#include <machine/sgmap.h>

#include <vm/vm.h>
#include <vm/vm_page.h>

#define KV(pa)			ALPHA_PHYS_TO_K0SEG(pa + sable_lynx_base)

vm_offset_t	sable_lynx_base = 0UL;

volatile t2_csr_t *t2_csr[2];
static int pci_int_type[2];

static devclass_t	t2_devclass;
static device_t		t2_0;		/* XXX only one for now */

struct t2_softc {
	int		junk;
};

#define T2_SOFTC(dev)	(struct t2_softc*) device_get_softc(dev)

static alpha_chipset_inb_t	t2_inb;
static alpha_chipset_inw_t	t2_inw;
static alpha_chipset_inl_t	t2_inl;
static alpha_chipset_outb_t	t2_outb;
static alpha_chipset_outw_t	t2_outw;
static alpha_chipset_outl_t	t2_outl;
static alpha_chipset_readb_t	t2_readb;
static alpha_chipset_readw_t	t2_readw;
static alpha_chipset_readl_t	t2_readl;
static alpha_chipset_writeb_t	t2_writeb;
static alpha_chipset_writew_t	t2_writew;
static alpha_chipset_writel_t	t2_writel;
static alpha_chipset_maxdevs_t	t2_maxdevs;
static alpha_chipset_cfgreadb_t	t2_cfgreadb;
static alpha_chipset_cfgreadw_t	t2_cfgreadw;
static alpha_chipset_cfgreadl_t	t2_cfgreadl;
static alpha_chipset_cfgwriteb_t t2_cfgwriteb;
static alpha_chipset_cfgwritew_t t2_cfgwritew;
static alpha_chipset_cfgwritel_t t2_cfgwritel;
static alpha_chipset_addrcvt_t   t2_cvt_dense;
static alpha_chipset_read_hae_t	t2_read_hae;
static alpha_chipset_write_hae_t t2_write_hae;

static alpha_chipset_t t2_chipset = {
	t2_inb,
	t2_inw,
	t2_inl,
	t2_outb,
	t2_outw,
	t2_outl,
	t2_readb,
	t2_readw,
	t2_readl,
	t2_writeb,
	t2_writew,
	t2_writel,
	t2_maxdevs,
	t2_cfgreadb,
	t2_cfgreadw,
	t2_cfgreadl,
	t2_cfgwriteb,
	t2_cfgwritew,
	t2_cfgwritel,
	t2_cvt_dense,
	NULL,
	t2_read_hae,
	t2_write_hae,
};

static u_int8_t
t2_inb(u_int32_t port)
{
	alpha_mb();
	return SPARSE_READ_BYTE(KV(T2_PCI_SIO), port);
}

static u_int16_t
t2_inw(u_int32_t port)
{
	alpha_mb();
	return SPARSE_READ_WORD(KV(T2_PCI_SIO), port);
}

static u_int32_t
t2_inl(u_int32_t port)
{
	alpha_mb();
	return SPARSE_READ_LONG(KV(T2_PCI_SIO), port);
}

static void
t2_outb(u_int32_t port, u_int8_t data)
{
	SPARSE_WRITE_BYTE(KV(T2_PCI_SIO), port, data);
	alpha_wmb();
}

static void
t2_outw(u_int32_t port, u_int16_t data)
{
	SPARSE_WRITE_WORD(KV(T2_PCI_SIO), port, data);
	alpha_wmb();
}

static void
t2_outl(u_int32_t port, u_int32_t data)
{
	SPARSE_WRITE_LONG(KV(T2_PCI_SIO), port, data);
	alpha_wmb();
}

static u_int32_t	t2_hae_mem[2];

#define REG1 (1UL << 24)

static __inline  void
t2_set_hae_mem(void *arg, u_int32_t *pa)
{
	int s; 
	u_int32_t msb;
	int hose;

	hose = (long)arg;

	if(*pa >= REG1){
		msb = *pa & 0xf8000000;
		*pa -= msb;
		msb >>= 27;	/* t2 puts high bits in the bottom of the register */
		s = splhigh();
		if (msb != t2_hae_mem[hose]) {
			t2_hae_mem[hose] = msb;
			t2_csr[hose]->hae0_1 = t2_hae_mem[hose];
			alpha_mb();
			t2_hae_mem[hose] = t2_csr[hose]->hae0_1;
		}
		splx(s);
	}
}
static u_int64_t
t2_read_hae(void)
{
	return t2_hae_mem[0] << 27;
}

static void
t2_write_hae(u_int64_t hae)
{
	u_int32_t pa = hae;
	t2_set_hae_mem(0, &pa);
}

static u_int8_t
t2_readb(u_int32_t pa)
{
	alpha_mb();
	t2_set_hae_mem(0, &pa);
	return SPARSE_READ_BYTE(KV(T2_PCI_SPARSE), pa);
}

static u_int16_t
t2_readw(u_int32_t pa)
{
	alpha_mb();
	t2_set_hae_mem(0, &pa);
	return SPARSE_READ_WORD(KV(T2_PCI_SPARSE), pa);
}

static u_int32_t
t2_readl(u_int32_t pa)
{
	alpha_mb();
	t2_set_hae_mem(0, &pa);
	return SPARSE_READ_LONG(KV(T2_PCI_SPARSE), pa);
}

static void
t2_writeb(u_int32_t pa, u_int8_t data)
{
	t2_set_hae_mem(0, &pa);
	SPARSE_WRITE_BYTE(KV(T2_PCI_SPARSE), pa, data);
	alpha_wmb();
}

static void
t2_writew(u_int32_t pa, u_int16_t data)
{
	t2_set_hae_mem(0, &pa);
	SPARSE_WRITE_WORD(KV(T2_PCI_SPARSE), pa, data);
	alpha_wmb();
}

static void
t2_writel(u_int32_t pa, u_int32_t data)
{
	t2_set_hae_mem(0, &pa);
	SPARSE_WRITE_LONG(KV(T2_PCI_SPARSE), pa, data);
	alpha_wmb();
}

static int
t2_maxdevs(u_int b)
{
	return 9;
}



/* XXX config space access? */

static vm_offset_t
t2_cvt_dense(vm_offset_t addr)
{
	addr &= 0xffffffffUL;
	return (addr | T2_PCI_DENSE);
	
}


#define T2_CFGOFF(b, s, f, r) \
	((b) ? (((b) << 16) | ((s) << 11) | ((f) << 8) | (r)) \
	 : ((1 << ((s) + 11)) | ((f) << 8) | (r)))

#define T2_TYPE1_SETUP(b,s,old_hae3) if((b)) {			\
        do {							\
		(s) = splhigh();				\
		(old_hae3) = REGVAL(T2_HAE0_3);			\
		alpha_mb();					\
		REGVAL(T2_HAE0_3) = (old_hae3) | (1<<30);	\
		alpha_mb();					\
        } while(0);						\
}

#define T2_TYPE1_TEARDOWN(b,s,old_hae3) if((b)) {	\
        do {						\
		alpha_mb();				\
		REGVAL(T2_HAE0_3) = (old_hae3);		\
		alpha_mb();				\
		splx((s));				\
        } while(0);					\
}

#define SWIZ_CFGREAD(b, s, f, r, width, type)					\
	type val = ~0;								\
	int ipl = 0;								\
	u_int32_t old_hae3 = 0;							\
	vm_offset_t off = T2_CFGOFF(b, s, f, r);				\
	vm_offset_t kv = SPARSE_##width##_ADDRESS(KV(T2_PCI_CONF), off);	\
	alpha_mb();								\
	T2_TYPE1_SETUP(b,ipl,old_hae3);						\
	if (!badaddr((caddr_t)kv, sizeof(type))) {				\
		val = SPARSE_##width##_EXTRACT(off, SPARSE_READ(kv));		\
	}									\
        T2_TYPE1_TEARDOWN(b,ipl,old_hae3);					\
	return val;							

#define SWIZ_CFGWRITE(b, s, f, r, data, width, type)				\
	int ipl = 0;								\
	u_int32_t old_hae3 = 0;							\
	vm_offset_t off = T2_CFGOFF(b, s, f, r);				\
	vm_offset_t kv = SPARSE_##width##_ADDRESS(KV(T2_PCI_CONF), off);	\
	alpha_mb();								\
	T2_TYPE1_SETUP(b,ipl,old_hae3);						\
	if (!badaddr((caddr_t)kv, sizeof(type))) {				\
                SPARSE_WRITE(kv, SPARSE_##width##_INSERT(off, data));		\
		alpha_wmb();							\
	}									\
        T2_TYPE1_TEARDOWN(b,ipl,old_hae3);					\
	return;							

static u_int8_t
t2_cfgreadb(u_int h, u_int b, u_int s, u_int f, u_int r)
{
	SWIZ_CFGREAD(b, s, f, r, BYTE, u_int8_t);
}

static u_int16_t
t2_cfgreadw(u_int h, u_int b, u_int s, u_int f, u_int r)
{
	SWIZ_CFGREAD(b, s, f, r, WORD, u_int16_t);
}

static u_int32_t
t2_cfgreadl(u_int h, u_int b, u_int s, u_int f, u_int r)
{
	SWIZ_CFGREAD(b, s, f, r, LONG, u_int32_t);
}

static void
t2_cfgwriteb(u_int h, u_int b, u_int s, u_int f, u_int r, u_int8_t data)
{
	SWIZ_CFGWRITE(b, s, f, r, data, BYTE, u_int8_t);
}

static void
t2_cfgwritew(u_int h, u_int b, u_int s, u_int f, u_int r, u_int16_t data)
{
	SWIZ_CFGWRITE(b, s, f, r, data, WORD, u_int16_t);
}

static void
t2_cfgwritel(u_int h, u_int b, u_int s, u_int f, u_int r, u_int32_t data)
{
	SWIZ_CFGWRITE(b, s, f, r, data, LONG, u_int32_t);
}

static int t2_probe(device_t dev);
static int t2_attach(device_t dev);
static int t2_setup_intr(device_t dev, device_t child,
			    struct resource *irq, int flags,
			    void *intr, void *arg, void **cookiep);
static int t2_teardown_intr(device_t dev, device_t child,
			    struct resource *irq, void *cookie);
static void
t2_dispatch_intr(void *frame, unsigned long vector);
static void
t2_machine_check(unsigned long mces, struct trapframe *framep, 
		 unsigned long vector, unsigned long param);


static device_method_t t2_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,			t2_probe),
	DEVMETHOD(device_attach,		t2_attach),

	/* Bus interface */
	DEVMETHOD(bus_alloc_resource,		pci_alloc_resource),
	DEVMETHOD(bus_release_resource,		pci_release_resource),
	DEVMETHOD(bus_activate_resource, 	pci_activate_resource),
	DEVMETHOD(bus_deactivate_resource,	pci_deactivate_resource),
	DEVMETHOD(bus_setup_intr,		t2_setup_intr),
	DEVMETHOD(bus_teardown_intr,		t2_teardown_intr),

	{ 0, 0 }
};

static driver_t t2_driver = {
	"t2",
	t2_methods,
	sizeof(struct t2_softc),
};


#define T2_SGMAP_BASE		(8*1024*1024)
#define T2_SGMAP_SIZE		(8*1024*1024)

static void
t2_sgmap_invalidate(void)
{
	u_int64_t val;

	alpha_mb();
	val = REGVAL64(T2_IOCSR);
	val |= T2_IOCSRL_ITLB;
	REGVAL64(T2_IOCSR) = val;
	alpha_mb();
	alpha_mb();
	val = REGVAL64(T2_IOCSR);
	val &= ~T2_IOCSRL_ITLB;
	REGVAL64(T2_IOCSR) = val;
	alpha_mb();
	alpha_mb();
}

static void
t2_sgmap_map(void *arg, vm_offset_t ba, vm_offset_t pa)
{
	u_int64_t *sgtable = arg;
	int index = alpha_btop(ba - T2_SGMAP_BASE);

	if (pa) {
		if (pa > (1L<<32))
			panic("t2_sgmap_map: can't map address 0x%lx", pa);
		sgtable[index] = ((pa >> 13) << 1) | 1;
	} else {
		sgtable[index] = 0;
	}
	alpha_mb();
	t2_sgmap_invalidate();
}


static void
t2_init_sgmap(int h)
{
	void *sgtable;

	/*
	 * First setup Window 2 to map 8Mb to 16Mb with an
	 * sgmap. Allocate the map aligned to a 32 boundary.
	 *
	 *  bits 31..20 of WBASE represent the pci start address
	 *  (in units of 1Mb), and bits 11..0 represent the pci
	 *  end address
	 */
	t2_csr[h]->wbase2 = T2_WSIZE_8M|T2_WINDOW_ENABLE|T2_WINDOW_SG
	                     | ((T2_SGMAP_BASE >> 20) << 20)
	                     | ((T2_SGMAP_BASE + T2_SGMAP_SIZE) >> 20);
	t2_csr[h]->wmask2 = T2_WMASK_8M;
	alpha_mb();

	sgtable = contigmalloc(8192, M_DEVBUF, M_NOWAIT,
			       0, (1L<<34),
			       32*1024, (1L<<34));
	if (!sgtable)
		panic("t2_init_sgmap: can't allocate page table");

	t2_csr[h]->tbase2 =
	    (pmap_kextract((vm_offset_t) sgtable) >> T2_TBASE_SHIFT);

	chipset.sgmap = sgmap_map_create(T2_SGMAP_BASE,
					 T2_SGMAP_BASE + T2_SGMAP_SIZE,
					 t2_sgmap_map, sgtable);
}

static void
t2_csr_init(int h)
{
	/* 
	 * initialize the DMA windows
	 */
	t2_csr[h]->wbase1 = T2_WSIZE_1G|T2_WINDOW_ENABLE|T2_WINDOW_DIRECT|0x7ff;
	t2_csr[h]->wmask1 = T2_WMASK_1G;
	t2_csr[h]->tbase1 = 0x0;

	t2_csr[h]->wbase2 = 0x0;

	/* 
	 *  enable the PCI "Hole" for ISA devices which use memory in
	 *  the 512k - 1MB range
	 */
	t2_csr[h]->hbase = 1 << 13;
	t2_init_sgmap(0);

	/* initialize the HAEs */
	t2_csr[h]->hae0_1 = 0x0;
	alpha_mb();
	t2_csr[h]->hae0_2 = 0x0;
	alpha_mb();
	t2_csr[h]->hae0_3 = 0x0;
	alpha_mb();

}

/*
 * Perform basic chipset init/fixup.  Called by various early
 * consumers to ensure that the system will work before the 
 * bus methods are invoked.
 *
 */

void
t2_init()
{
	static int initted = 0;

	if (initted) return;
	initted = 1;

	chipset = t2_chipset;
}

static int
t2_probe(device_t dev)
{
	int h, t2_num_hoses = 1;
	device_t child;

	if (t2_0)
		return ENXIO;
	t2_0 = dev;
	device_set_desc(dev, "T2 Core Logic chipset"); 
	t2_csr[0] = (t2_csr_t *)
	    ALPHA_PHYS_TO_K0SEG(sable_lynx_base + PCI0_BASE);
	t2_csr[1] = (t2_csr_t *)
	    ALPHA_PHYS_TO_K0SEG(sable_lynx_base + PCI1_BASE);

	/* Look at the rev of the chip.  If the high bit is set in the
	 * rev field then we have either a T3 or a T4 chip, so use the
	 * new interrupt structure.  If it is clear, then we have a T2
	 * so use the old way */

	platform.mcheck_handler = t2_machine_check;

	if (((t2_csr[0]->iocsr) >> 35) & 1)
		pci_int_type[0] = 1;
	 else 
		pci_int_type[0] = 0;

	device_printf(dev, "using interrupt type %d on pci bus 0\n", 
	    pci_int_type[0]);

	if (!badaddr((void *)&t2_csr[1]->tlbbr, sizeof(long))) {
		pci_int_type[1] = 1; /* PCI1 always uses the new scheme */
		/* Clear any errors that the BADADDR probe may have caused */
		t2_csr[1]->cerr1 |= t2_csr[1]->cerr1;
		t2_csr[1]->pcierr1 |= t2_csr[1]->pcierr1;
		device_printf(dev, "found EXT_IO!!!!!\n");
		/* t2_num_hoses = 2; XXX not ready for this yet */
	}

	pci_init_resources();

	for (h = 0; h < t2_num_hoses; h++)
		t2_csr_init(h);

	
	child = device_add_child(dev, "pcib", 0);
	device_set_ivars(child, 0);

	return 0;
}

static int
t2_attach(device_t dev)
{
	t2_init();

	set_iointr(t2_dispatch_intr);
	platform.isa_setup_intr = t2_setup_intr;
	platform.isa_teardown_intr = t2_teardown_intr;

	snprintf(chipset_type, sizeof(chipset_type), "t2");

	bus_generic_attach(dev);

	return 0;
}
/*
 * magical mystery table partly obtained from Linux
 * at least some of their values for PCI masks
 * were incorrect, and I've filled in my own extrapolations
 * XXX this needs more testers 
 */

unsigned long t2_shadow_mask = -1L;
static const char irq_to_mask[40] = {
	-1,  6, -1,  8, 15, 12,  7,  9,		/* ISA 0-7  */
	-1, 16, 17, 18,  3, -1, 21, 22, 	/* ISA 8-15 */
	-1, -1, -1, -1, -1, -1, -1, -1,		/* ?? EISA XXX */
	-1, -1, -1, -1, -1, -1, -1, -1,		/* ?? EISA XXX */
	 0,  1,  2,  3,  4,  5,  6,  7		/* PCI 0-7 XXX */
};

static void
t2_8259_disable_mask(int mask)
{
	t2_shadow_mask |= (1UL << mask);

	if (mask <= 7)
		outb(SLAVE0_ICU, t2_shadow_mask);
	else if (mask <= 15)
		outb(SLAVE1_ICU, t2_shadow_mask >> 8);
	else 
		outb(SLAVE2_ICU, t2_shadow_mask >> 16);
}

static void
t2_8259_enable_mask(int mask)
{
	t2_shadow_mask &= ~(1UL << mask);

	if (mask <= 7)
		outb(SLAVE0_ICU, t2_shadow_mask);
	else if (mask <= 15)
		outb(SLAVE1_ICU, t2_shadow_mask >> 8);
	else 
		outb(SLAVE2_ICU, t2_shadow_mask >> 16);
}


static void 
t2_eoi( int vector)
{
	int irq, hose;

	hose = (vector >= 0xC00);
	irq = (vector - 0x800) >> 4;

	if (pci_int_type[hose]) {

		/* New interrupt scheme.  Both PCI0 and PCI1 can use
		 * the same handler.  Dispatching interrupts with the
		 * IC IC chip is easy.  We simply write the vector
		 * address  register (var) on the T3/T4 (offset
		 * 0x480) with the IRQ  level (0 - 63) of what came in.  */
		t2_csr[hose]->var = (u_long) irq;
		alpha_mb();
		alpha_mb();
	} else {
		switch (irq) {
		case 0 ... 7:
			outb(SLAVE0_ICU-1, (0xe0 | (irq)));
			outb(MASTER_ICU-1, (0xe0 | 1));
			break;
		case 8 ... 15:
			outb(SLAVE1_ICU-1, (0xe0 | (irq - 8)));
			outb(MASTER_ICU-1, (0xe0 | 3));
			break;
		case 16 ... 24:
			outb(SLAVE2_ICU-1, (0xe0 | (irq - 16)));
			outb(MASTER_ICU-1, (0xe0 | 4));
			break;
		}	
	}
}

static void
t2_enable_vec(int vector)
{
	int irq, hose;
	u_long IC_mask, scratch;

	hose = (vector >= 0xC00);
	irq = (vector - 0x800) >> 4;

	if (pci_int_type[hose]) {

		/* Write the air register on the T3/T4 with the
		 * address of the IC IC masks register (offset 0x40) */
		t2_csr[hose]->air = 0x40;
		alpha_mb();
		scratch = t2_csr[hose]->air;	
		alpha_mb();
		IC_mask = t2_csr[hose]->dir;
		IC_mask &= ~(1L << ( (u_long) irq));
		t2_csr[hose]->dir = IC_mask;	
		alpha_mb();
		alpha_mb();
		/*
		 * EOI the interrupt we just enabled.
		 */
		t2_eoi(vector);
	} else {
		/* Old style 8259 (Gack!!!) interrupts */
		t2_8259_enable_mask(irq);
	}
}

#ifdef notyet
static void
t2_disable_vec(int vector)
{
	int hose, irq;
	u_long scratch, IC_mask;

	hose = (vector >= 0xC00);
	irq =  (vector - 0x800) >> 4;

	if (pci_int_type[hose]) {

		/* Write the air register on the T3/T4 wioth the
		 * address of the IC IC masks register (offset 0x40) */

		t2_csr[hose]->air = 0x40;
		alpha_mb();
		scratch = t2_csr[hose]->air;	
		alpha_mb();
		/*
		 * Read the dir register to fetch the mask data, 'or' in the
		 * new disable bit, and write the data back.
		 */
		IC_mask = t2_csr[hose]->dir;
		IC_mask |= (1L << ( (u_long) irq));
		/* Set the disable bit */
		t2_csr[hose]->dir = IC_mask;	
		alpha_mb();
		alpha_mb();
	} else {
		/* Old style 8259 (Gack!!!) interrupts */
		t2_8259_disable_mask(irq);
	}
}
#endif /*notyet*/

static int
t2_setup_intr(device_t dev, device_t child,
	       struct resource *irq, int flags,
	       void *intr, void *arg, void **cookiep)
{
	int error, vector, stdio_irq;
	const char *name;
	device_t bus, parent;

	name = device_get_nameunit(dev);
	stdio_irq = irq->r_start;
	if (strncmp(name, "eisa", 4) == 0) {
		if ((stdio_irq != 6 ) && (stdio_irq != 3 )) {
			stdio_irq = 
			    T2_EISA_IRQ_TO_STDIO_IRQ(stdio_irq);
		}
	} else if ((strncmp(name, "isa", 3)) == 0) {
		stdio_irq = irq_to_mask[stdio_irq];
	}

	parent = dev;
	do {
		bus = parent;
		parent = device_get_parent(bus);
	} while (parent && strncmp("t2", device_get_nameunit(parent), 2));

	if (parent && (device_get_unit(bus) != 0))
		vector = STDIO_PCI1_IRQ_TO_SCB_VECTOR(stdio_irq);
	else	
		vector = STDIO_PCI0_IRQ_TO_SCB_VECTOR(stdio_irq);

	error = rman_activate_resource(irq);
	if (error)
		return error;

	error = alpha_setup_intr(vector, intr, arg, cookiep,
	    &intrcnt[irq->r_start]);
	    
	if (error)
		return error;

	/* Enable interrupt */
	t2_enable_vec(vector);
	
	if (bootverbose != 0) 
		device_printf(child, 
		    "interrupting at T2 irq %d (stdio irq %d)\n",
		      (int) irq->r_start, stdio_irq);
	return 0;
}

static int
t2_teardown_intr(device_t dev, device_t child,
	       struct resource *irq, void *cookie)
{
	int mask;
	
	mask = irq_to_mask[irq->r_start];

	/* Disable interrupt */
	
	/* 
	 *  XXX this is totally broken! 
	 *  we don't have enough info to figure out where the interrupt 
	 *  came from if hose != 0 and pci_int_type[hose] != 0
	 *  We should probably carry around the vector someplace --
	 *  that would be enough to figure out the hose and the stdio irq
	 */

	t2_shadow_mask |= (1UL << mask);

	if (mask <= 7)
		outb(SLAVE0_ICU, t2_shadow_mask);
	else if (mask <= 15)
		outb(SLAVE1_ICU, t2_shadow_mask >> 8);
	else 
		outb(SLAVE2_ICU, t2_shadow_mask >> 16);

	alpha_teardown_intr(cookie);
	return rman_deactivate_resource(irq);
}


static void
t2_dispatch_intr(void *frame, unsigned long vector)
{
	alpha_dispatch_intr(frame, vector);
	t2_eoi(vector);
}

static void
t2_machine_check(unsigned long mces, struct trapframe *framep, 
    unsigned long vector, unsigned long param)
{
	int expected;

	expected = mc_expected;
	machine_check(mces, framep, vector, param);
	/* for some reason the alpha_pal_wrmces() doesn't clear all
	   pending machine checks & we may take another */
	mc_expected = expected;
}

DRIVER_MODULE(t2, root, t2_driver, t2_devclass, 0, 0);
