/*-
 * Copyright (c) 2002 Bang Jun-Young
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	from: NetBSD: machfb.c,v 1.23 2005/03/07 21:45:24 martin Exp
 */
/*-
 * Copyright (c) 2005 Marius Strobl <marius@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

/*
 * Driver for ATI Mach64 graphics chips. Some code is derived from the
 * ATI Rage Pro and Derivatives Programmer's Guide.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/consio.h>
#include <sys/eventhandler.h>
#include <sys/fbio.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/resource.h>

#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/openfirm.h>

#include <machine/bus.h>
#include <machine/bus_private.h>
#include <machine/ofw_machdep.h>
#include <machine/resource.h>
#include <machine/sc_machdep.h>

#include <sys/rman.h>

#include <dev/fb/fbreg.h>
#include <dev/fb/gallant12x22.h>
#include <dev/fb/machfbreg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/syscons/syscons.h>

/* #define MACHFB_DEBUG */

#define	MACHFB_DRIVER_NAME	"machfb"

#define	MACH64_REG_OFF		0x7ffc00
#define	MACH64_REG_SIZE		1024

struct machfb_softc {
	video_adapter_t		sc_va;		/* must be first */

	phandle_t		sc_node;	
	uint16_t		sc_chip_id;
	uint8_t			sc_chip_rev;

	int			sc_memrid;
	int			sc_viorid;
	int			sc_vmemrid;
	struct resource		*sc_memres;
	struct resource		*sc_viores;
	struct resource		*sc_vmemres;
	bus_space_tag_t		sc_memt;
	bus_space_tag_t		sc_regt;
	bus_space_tag_t		sc_viot;
	bus_space_tag_t		sc_vmemt;
	bus_space_handle_t	sc_memh;
	bus_space_handle_t	sc_regh;
	bus_space_handle_t	sc_vioh;
	bus_space_handle_t	sc_vmemh;

	int			sc_height;
	int			sc_width;
	int			sc_depth;
	int			sc_xmargin;
	int			sc_ymargin;

	size_t			sc_memsize;
	int			sc_memtype;
	int			sc_mem_freq;
	int			sc_ramdac_freq;
	int			sc_ref_freq;

	int			sc_ref_div;
	int			sc_mclk_post_div;
	int			sc_mclk_fb_div;

	int			sc_dacw;
	uint8_t			sc_cmap_red[256];
	uint8_t			sc_cmap_green[256];
	uint8_t			sc_cmap_blue[256];

	u_char			*sc_font;
	int			sc_cbwidth;
	vm_offset_t		sc_curoff;

	int			sc_draw_cache;
#define	MACHFB_DRAW_CHAR	(1 << 0)
#define	MACHFB_DRAW_FILLRECT	(1 << 1)

	int			sc_flags;
#define	MACHFB_CONSOLE		(1 << 0)
#define	MACHFB_CUREN		(1 << 1)
#define	MACHFB_DSP		(1 << 2)
};

static struct {
	uint16_t	chip_id;
	const char	*name;
	uint32_t	ramdac_freq;
} machfb_info[] = {
	{ ATI_MACH64_CT, "ATI Mach64 CT", 135000 },
	{ ATI_RAGE_PRO_AGP, "ATI 3D Rage Pro (AGP)", 230000 },
	{ ATI_RAGE_PRO_AGP1X, "ATI 3D Rage Pro (AGP 1x)", 230000 },
	{ ATI_RAGE_PRO_PCI_B, "ATI 3D Rage Pro Turbo", 230000 },
	{ ATI_RAGE_XC_PCI66, "ATI Rage XL (PCI66)", 230000 },
	{ ATI_RAGE_XL_AGP, "ATI Rage XL (AGP)", 230000 },
	{ ATI_RAGE_XC_AGP, "ATI Rage XC (AGP)", 230000 },
	{ ATI_RAGE_XL_PCI66, "Rage XL (PCI66)", 230000 },
	{ ATI_RAGE_PRO_PCI_P, "ATI 3D Rage Pro", 230000 },
	{ ATI_RAGE_PRO_PCI_L, "ATI 3D Rage Pro (limited 3D)", 230000 },
	{ ATI_RAGE_XL_PCI, "ATI Rage XL", 230000 },
	{ ATI_RAGE_XC_PCI, "ATI Rage XC", 230000 },
	{ ATI_RAGE_II, "ATI 3D Rage I/II", 135000 },
	{ ATI_RAGE_IIP, "ATI 3D Rage II+", 200000 },
	{ ATI_RAGE_IIC_PCI, "ATI 3D Rage IIC", 230000 },
	{ ATI_RAGE_IIC_AGP_B, "ATI 3D Rage IIC (AGP)", 230000 },
	{ ATI_RAGE_IIC_AGP_P, "ATI 3D Rage IIC (AGP)", 230000 },
	{ ATI_RAGE_LT_PRO_AGP, "ATI 3D Rage LT Pro (AGP 133MHz)", 230000 },
	{ ATI_RAGE_MOB_M3_PCI, "ATI Rage Mobility M3", 230000 },
	{ ATI_RAGE_MOB_M3_AGP, "ATI Rage Mobility M3 (AGP)", 230000 },
	{ ATI_RAGE_LT, "ATI 3D Rage LT", 230000 },
	{ ATI_RAGE_LT_PRO_PCI, "ATI 3D Rage LT Pro", 230000 },
	{ ATI_RAGE_MOBILITY, "ATI Rage Mobility", 230000 },
	{ ATI_RAGE_L_MOBILITY, "ATI Rage L Mobility", 230000 },
	{ ATI_RAGE_LT_PRO, "ATI 3D Rage LT Pro", 230000 },
	{ ATI_RAGE_LT_PRO2, "ATI 3D Rage LT Pro", 230000 },
	{ ATI_RAGE_MOB_M1_PCI, "ATI Rage Mobility M1 (PCI)", 230000 },
	{ ATI_RAGE_L_MOB_M1_PCI, "ATI Rage L Mobility (PCI)", 230000 },
	{ ATI_MACH64_VT, "ATI Mach64 VT", 170000 },
	{ ATI_MACH64_VTB, "ATI Mach64 VTB", 200000 },
	{ ATI_MACH64_VT4, "ATI Mach64 VT4", 230000 }
};

static struct machfb_cmap {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} machfb_default_cmap[16] = {
	{0x00, 0x00, 0x00},	/* black */
	{0x00, 0x00, 0xff},	/* blue */
	{0x00, 0xff, 0x00},	/* green */
	{0x00, 0xc0, 0xc0},	/* cyan */
	{0xff, 0x00, 0x00},	/* red */
	{0xc0, 0x00, 0xc0},	/* magenta */
	{0xc0, 0xc0, 0x00},	/* brown */
	{0xc0, 0xc0, 0xc0},	/* light grey */
	{0x80, 0x80, 0x80},	/* dark grey */
	{0x80, 0x80, 0xff},	/* light blue */
	{0x80, 0xff, 0x80},	/* light green */
	{0x80, 0xff, 0xff},	/* light cyan */
	{0xff, 0x80, 0x80},	/* light red */
	{0xff, 0x80, 0xff},	/* light magenta */
	{0xff, 0xff, 0x80},	/* yellow */
	{0xff, 0xff, 0xff}	/* white */
};

static u_char machfb_mouse_pointer_bits[64][8] = {
	{ 0x00, 0x00, },	/* ............ */
	{ 0x80, 0x00, },	/* *........... */
	{ 0xc0, 0x00, },	/* **.......... */
	{ 0xe0, 0x00, },	/* ***......... */
	{ 0xf0, 0x00, },	/* ****........ */
	{ 0xf8, 0x00, },	/* *****....... */
	{ 0xfc, 0x00, },	/* ******...... */
	{ 0xfe, 0x00, },	/* *******..... */
	{ 0xff, 0x00, },	/* ********.... */
	{ 0xff, 0x80, },	/* *********... */
	{ 0xfc, 0xc0, },	/* ******..**.. */
	{ 0xdc, 0x00, },	/* **.***...... */
	{ 0x8e, 0x00, },	/* *...***..... */
	{ 0x0e, 0x00, },	/* ....***..... */
	{ 0x07, 0x00, },	/* .....***.... */
	{ 0x04, 0x00, },	/* .....*...... */
	{ 0x00, 0x00, },	/* ............ */
	{ 0x00, 0x00, },	/* ............ */
	{ 0x00, 0x00, },	/* ............ */
	{ 0x00, 0x00, },	/* ............ */
	{ 0x00, 0x00, },	/* ............ */
	{ 0x00, 0x00, },	/* ............ */
};

/*
 * Lookup table to perform a bit-swap of the mouse pointer bits,
 * map set bits to CUR_CLR0 and unset bits to transparent.
 */
static u_char machfb_mouse_pointer_lut[] = {
	0xaa, 0x2a, 0x8a, 0x0a, 0xa2, 0x22, 0x82, 0x02,
	0xa8, 0x28, 0x88, 0x08, 0xa0, 0x20, 0x80, 0x00
};

static char *machfb_memtype_names[] = {
	"(N/A)", "DRAM", "EDO DRAM", "EDO DRAM", "SDRAM", "SGRAM", "WRAM",
	"(unknown type)"
};

static struct machfb_softc machfb_softc;
static struct bus_space_tag machfb_bst_store[1];

static device_probe_t machfb_pci_probe;
static device_attach_t machfb_pci_attach;
static device_detach_t machfb_pci_detach;

static device_method_t machfb_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		machfb_pci_probe),
	DEVMETHOD(device_attach,	machfb_pci_attach),
	DEVMETHOD(device_detach,	machfb_pci_detach),

	{ 0, 0 }
};

static driver_t machfb_pci_driver = {
	MACHFB_DRIVER_NAME,
	machfb_methods,
	sizeof(struct machfb_softc),
};

static devclass_t machfb_devclass;

DRIVER_MODULE(machfb, pci, machfb_pci_driver, machfb_devclass, 0, 0);
MODULE_DEPEND(machfb, pci, 1, 1, 1);

static void machfb_cursor_enable(struct machfb_softc *, int);
static int machfb_cursor_install(struct machfb_softc *);
static int machfb_get_memsize(struct machfb_softc *);
static void machfb_reset_engine(struct machfb_softc *);
static void machfb_init_engine(struct machfb_softc *);
#if 0
static void machfb_adjust_frame(struct machfb_softc *, int, int);
#endif
static void machfb_putpalreg(struct machfb_softc *, uint8_t, uint8_t, uint8_t,
    uint8_t);
static void machfb_shutdown_final(void *v);
static void machfb_shutdown_reset(void *v);

static int machfb_configure(int flags);

static vi_probe_t machfb_probe;
static vi_init_t machfb_init;
static vi_get_info_t machfb_get_info;
static vi_query_mode_t machfb_query_mode;
static vi_set_mode_t machfb_set_mode;
static vi_save_font_t machfb_save_font;
static vi_load_font_t machfb_load_font;
static vi_show_font_t machfb_show_font;
static vi_save_palette_t machfb_save_palette;
static vi_load_palette_t machfb_load_palette;
static vi_set_border_t machfb_set_border;
static vi_save_state_t machfb_save_state;
static vi_load_state_t machfb_load_state;
static vi_set_win_org_t machfb_set_win_org;
static vi_read_hw_cursor_t machfb_read_hw_cursor;
static vi_set_hw_cursor_t machfb_set_hw_cursor;
static vi_set_hw_cursor_shape_t machfb_set_hw_cursor_shape;
static vi_blank_display_t machfb_blank_display;
static vi_mmap_t machfb_mmap;
static vi_ioctl_t machfb_ioctl;
static vi_clear_t machfb_clear;
static vi_fill_rect_t machfb_fill_rect;
static vi_bitblt_t machfb_bitblt;
static vi_diag_t machfb_diag;
static vi_save_cursor_palette_t machfb_save_cursor_palette;
static vi_load_cursor_palette_t machfb_load_cursor_palette;
static vi_copy_t machfb_copy;
static vi_putp_t machfb_putp;
static vi_putc_t machfb_putc;
static vi_puts_t machfb_puts;
static vi_putm_t machfb_putm;

static video_switch_t machfbvidsw = {
	.probe			= machfb_probe,
	.init			= machfb_init,
	.get_info		= machfb_get_info,
	.query_mode		= machfb_query_mode,
	.set_mode		= machfb_set_mode,
	.save_font		= machfb_save_font,
	.load_font		= machfb_load_font,
	.show_font		= machfb_show_font,
	.save_palette		= machfb_save_palette,
	.load_palette		= machfb_load_palette,
	.set_border		= machfb_set_border,
	.save_state		= machfb_save_state,
	.load_state		= machfb_load_state,
	.set_win_org		= machfb_set_win_org,
	.read_hw_cursor		= machfb_read_hw_cursor,
	.set_hw_cursor		= machfb_set_hw_cursor,
	.set_hw_cursor_shape	= machfb_set_hw_cursor_shape,
	.blank_display		= machfb_blank_display,
	.mmap			= machfb_mmap,
	.ioctl			= machfb_ioctl,
	.clear			= machfb_clear,
	.fill_rect		= machfb_fill_rect,
	.bitblt			= machfb_bitblt,
	NULL,
	NULL,
	.diag			= machfb_diag,
	.save_cursor_palette	= machfb_save_cursor_palette,
	.load_cursor_palette	= machfb_load_cursor_palette,
	.copy			= machfb_copy,
	.putp			= machfb_putp,
	.putc			= machfb_putc,
	.puts			= machfb_puts,
	.putm			= machfb_putm
};

VIDEO_DRIVER(machfb, machfbvidsw, machfb_configure);

extern sc_rndr_sw_t txtrndrsw;
RENDERER(machfb, 0, txtrndrsw, gfb_set);

RENDERER_MODULE(machfb, gfb_set);

/*
 * Inline functions for getting access to register aperture.
 */
static inline uint32_t regr(struct machfb_softc *, uint32_t);
static inline uint8_t regrb(struct machfb_softc *, uint32_t);
static inline void regw(struct machfb_softc *, uint32_t, uint32_t);
static inline void regwb(struct machfb_softc *, uint32_t, uint8_t);
static inline void regwb_pll(struct machfb_softc *, uint32_t, uint8_t);

static inline uint32_t
regr(struct machfb_softc *sc, uint32_t index)
{

	return bus_space_read_4(sc->sc_regt, sc->sc_regh, index);
}

static inline uint8_t
regrb(struct machfb_softc *sc, uint32_t index)
{

	return bus_space_read_1(sc->sc_regt, sc->sc_regh, index);
}

static inline void
regw(struct machfb_softc *sc, uint32_t index, uint32_t data)
{

	bus_space_write_4(sc->sc_regt, sc->sc_regh, index, data);
	bus_space_barrier(sc->sc_regt, sc->sc_regh, index, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline void
regwb(struct machfb_softc *sc, uint32_t index, uint8_t data)
{

	bus_space_write_1(sc->sc_regt, sc->sc_regh, index, data);
	bus_space_barrier(sc->sc_regt, sc->sc_regh, index, 1,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline void
regwb_pll(struct machfb_softc *sc, uint32_t index, uint8_t data)
{

	regwb(sc, CLOCK_CNTL + 1, (index << 2) | PLL_WR_EN);
	regwb(sc, CLOCK_CNTL + 2, data);
	regwb(sc, CLOCK_CNTL + 1, (index << 2) & ~PLL_WR_EN);
}

static inline void
wait_for_fifo(struct machfb_softc *sc, uint8_t v)
{

	while ((regr(sc, FIFO_STAT) & 0xffff) > (0x8000 >> v))
		;
}

static inline void
wait_for_idle(struct machfb_softc *sc)
{

	wait_for_fifo(sc, 16);
	while ((regr(sc, GUI_STAT) & 1) != 0)
		;
}

/*
 * video driver interface
 */
static int
machfb_configure(int flags)
{
	struct machfb_softc *sc;
	phandle_t chosen, output;
	ihandle_t stdout;
	bus_addr_t addr;
	uint32_t id;
	int i, space;

	sc = &machfb_softc;

	if ((chosen = OF_finddevice("/chosen")) == -1)	/* Quis contra nos? */
		return (0);
	if (OF_getprop(chosen, "stdout", &stdout, sizeof(stdout)) == -1)
		return (0);
	if ((output = OF_instance_to_package(stdout)) == -1)
		return (0);
	if ((OF_getprop(output, "vendor-id", &id, sizeof(id)) == -1) ||
	    id != ATI_VENDOR)
		return (0);
	if (OF_getprop(output, "device-id", &id, sizeof(id)) == -1)
		return (0);
	for (i = 0; i < sizeof(machfb_info) / sizeof(machfb_info[0]); i++) {
		if (id == machfb_info[i].chip_id) {
			/*
			 * When being called a second time, i.e. during
			 * sc_probe_unit(), just return at this point.
			 * Note that the polarity of the VIO_PROBE_ONLY
			 * flag is somewhat non-intuitive.
			 */
			if (!(flags & VIO_PROBE_ONLY))
				goto found;
			sc->sc_flags = MACHFB_CONSOLE;
			sc->sc_node = output;
			sc->sc_chip_id = id;
			break;
		}
	}
	if (!(sc->sc_flags & MACHFB_CONSOLE))
		return (0);

	if (OF_getprop(output, "revision-id", &sc->sc_chip_rev,
	    sizeof(sc->sc_chip_rev)) == -1)
		return (0);
	if (OF_decode_addr(output, 0, &space, &addr) != 0)
		return (0);
	sc->sc_memt = &machfb_bst_store[0];
	sc->sc_memh = sparc64_fake_bustag(space, addr, sc->sc_memt);
	sc->sc_regt = sc->sc_memt;
	bus_space_subregion(sc->sc_regt, sc->sc_memh, MACH64_REG_OFF,
	    MACH64_REG_SIZE, &sc->sc_regh);

	if (machfb_init(0, &sc->sc_va, 0) < 0)
		 return (0);

 found:
	/* Return number of found adapters. */
	return (1);
}

static int
machfb_probe(int unit, video_adapter_t **adpp, void *arg, int flags)
{

	return (0);
}

static int
machfb_init(int unit, video_adapter_t *adp, int flags)
{
	struct machfb_softc *sc;
	phandle_t options;
	video_info_t *vi;
	char buf[32];
	int i, j;

	sc = (struct machfb_softc *)adp;
	vi = &adp->va_info;

	if ((regr(sc, CONFIG_CHIP_ID) & 0xffff) != sc->sc_chip_id)
		return (ENXIO);

	sc->sc_ramdac_freq = 0;
	for (i = 0; i < sizeof(machfb_info) / sizeof(machfb_info[0]); i++) {
		if (sc->sc_chip_id == machfb_info[i].chip_id) {
			sc->sc_ramdac_freq = machfb_info[i].ramdac_freq;
			break;
		}
	}
	if (sc->sc_ramdac_freq == 0)
		return (ENXIO);
	if (sc->sc_chip_id == ATI_RAGE_II && sc->sc_chip_rev & 0x07)
		sc->sc_ramdac_freq = 170000;

	vid_init_struct(adp, MACHFB_DRIVER_NAME, -1, unit);

 	if (OF_getprop(sc->sc_node, "height", &sc->sc_height,
	    sizeof(sc->sc_height)) == -1)
		return (ENXIO);
	if (OF_getprop(sc->sc_node, "width", &sc->sc_width,
	    sizeof(sc->sc_width)) == -1)
		return (ENXIO);
	if (OF_getprop(sc->sc_node, "depth", &sc->sc_depth,
	    sizeof(sc->sc_depth)) == -1)
		return (ENXIO);
	if ((options = OF_finddevice("/options")) == -1)
		return (ENXIO);
	if (OF_getprop(options, "screen-#rows", buf, sizeof(buf)) == -1)
		return (ENXIO);
	vi->vi_height = strtol(buf, NULL, 10);
	if (OF_getprop(options, "screen-#columns", buf, sizeof(buf)) == -1)
		return (ENXIO);
	vi->vi_width = strtol(buf, NULL, 10);
	vi->vi_cwidth = 12;
	vi->vi_cheight = 22;
	vi->vi_flags = V_INFO_COLOR;
	vi->vi_mem_model = V_INFO_MM_OTHER;

	sc->sc_draw_cache = -1;
	sc->sc_font = gallant12x22_data;
	sc->sc_cbwidth = howmany(vi->vi_cwidth, 8);	/* width in bytes */
	sc->sc_xmargin = (sc->sc_width - (vi->vi_width * vi->vi_cwidth)) / 2;
	sc->sc_ymargin = (sc->sc_height - (vi->vi_height * vi->vi_cheight)) / 2;

	if (sc->sc_chip_id != ATI_MACH64_CT &&
	    !((sc->sc_chip_id == ATI_MACH64_VT ||
	    sc->sc_chip_id == ATI_RAGE_II) &&
	    (sc->sc_chip_rev & 0x07) == 0))
		sc->sc_flags |= MACHFB_DSP;

	sc->sc_memsize = machfb_get_memsize(sc);
	if (sc->sc_memsize == 8192)
		/* The last page is used as register aperture. */
		sc->sc_memsize -= 4;
	sc->sc_memtype = regr(sc, CONFIG_STAT0) & 0x07;

	if ((sc->sc_chip_id >= ATI_RAGE_XC_PCI66 &&
	    sc->sc_chip_id <= ATI_RAGE_XL_PCI66) ||
	    (sc->sc_chip_id >= ATI_RAGE_XL_PCI &&
	    sc->sc_chip_id <= ATI_RAGE_XC_PCI))
		sc->sc_ref_freq = 29498;
	else
		sc->sc_ref_freq = 14318;

	regwb(sc, CLOCK_CNTL + 1, PLL_REF_DIV << 2);
	sc->sc_ref_div = regrb(sc, CLOCK_CNTL + 2);
	regwb(sc, CLOCK_CNTL + 1, MCLK_FB_DIV << 2);
	sc->sc_mclk_fb_div = regrb(sc, CLOCK_CNTL + 2);
	sc->sc_mem_freq = (2 * sc->sc_ref_freq * sc->sc_mclk_fb_div) /
	    (sc->sc_ref_div * 2);
	sc->sc_mclk_post_div = (sc->sc_mclk_fb_div * 2 * sc->sc_ref_freq) /
	    (sc->sc_mem_freq * sc->sc_ref_div);

	machfb_init_engine(sc);
#if 0
	mach64_adjust_frame(0, 0);
#endif

	sc->sc_dacw = -1;
	for (i = 0; i < 16; i++)
		for (j = 0; j < 16; j++)
			machfb_putpalreg(sc, (i * 16) + j,
			    machfb_default_cmap[j].red,
			    machfb_default_cmap[j].green,
			    machfb_default_cmap[j].blue);

	machfb_blank_display(adp, V_DISPLAY_BLANK);

	adp->va_flags |= V_ADP_COLOR | V_ADP_BORDER | V_ADP_INITIALIZED;
	if (vid_register(adp) < 0)
		return (ENXIO);
	adp->va_flags |= V_ADP_REGISTERED;

	return (0);
}

static int
machfb_get_info(video_adapter_t *adp, int mode, video_info_t *info)
{

	bcopy(&adp->va_info, info, sizeof(*info));

	return (0);
}

static int
machfb_query_mode(video_adapter_t *adp, video_info_t *info)
{

	return (ENODEV);
}

static int
machfb_set_mode(video_adapter_t *adp, int mode)
{

	return (ENODEV);
}

static int
machfb_save_font(video_adapter_t *adp, int page, int size, u_char *data,
    int c, int count)
{

	return (ENODEV);
}

static int
machfb_load_font(video_adapter_t *adp, int page, int size, u_char *data,
    int c, int count)
{

	return (ENODEV);
}

static int
machfb_show_font(video_adapter_t *adp, int page)
{

	return (ENODEV);
}

static int
machfb_save_palette(video_adapter_t *adp, u_char *palette)
{

	return (ENODEV);
}

static int
machfb_load_palette(video_adapter_t *adp, u_char *palette)
{

	return (ENODEV);
}

static int
machfb_set_border(video_adapter_t *adp, int border)
{
	struct machfb_softc *sc;

	sc = (struct machfb_softc *)adp;

	machfb_fill_rect(adp, border, 0, 0, sc->sc_width, sc->sc_ymargin);
	machfb_fill_rect(adp, border, 0, sc->sc_height - sc->sc_ymargin,
	    sc->sc_width, sc->sc_ymargin);
	machfb_fill_rect(adp, border, 0, 0, sc->sc_xmargin, sc->sc_height);
	machfb_fill_rect(adp, border, sc->sc_width - sc->sc_xmargin, 0,
	    sc->sc_xmargin, sc->sc_height);

	return (0);
}

static int
machfb_save_state(video_adapter_t *adp, void *p, size_t size)
{

	return (ENODEV);
}

static int
machfb_load_state(video_adapter_t *adp, void *p)
{

	return (ENODEV);
}

static int
machfb_set_win_org(video_adapter_t *adp, off_t offset)
{

	return (ENODEV);
}

static int
machfb_read_hw_cursor(video_adapter_t *adp, int *col, int *row)
{

	*col = 0;
	*row = 0;

	return (0);
}

static int
machfb_set_hw_cursor(video_adapter_t *adp, int col, int row)
{

	return (ENODEV);
}

static int
machfb_set_hw_cursor_shape(video_adapter_t *adp, int base, int height,
    int celsize, int blink)
{

	return (ENODEV);
}

static int
machfb_blank_display(video_adapter_t *adp, int mode)
{
	struct machfb_softc *sc;

	sc = (struct machfb_softc *)adp;

	if (mode == V_DISPLAY_ON || mode == V_DISPLAY_BLANK)
		regw(sc, CRTC_GEN_CNTL, (regr(sc, CRTC_GEN_CNTL) | CRTC_EN |
		    CRTC_EXT_DISP_EN) & ~CRTC_DISPLAY_DIS);
	else
		regw(sc, CRTC_GEN_CNTL, regr(sc, CRTC_GEN_CNTL) & ~CRTC_EN);
	if (mode == V_DISPLAY_BLANK)
		machfb_fill_rect(adp, (SC_NORM_ATTR >> 4) & 0xf, 0, 0,
		    sc->sc_width, sc->sc_height);

	return (0);
}

static int
machfb_mmap(video_adapter_t *adp, vm_offset_t offset, vm_paddr_t *paddr,
    int prot)
{
	struct machfb_softc *sc;

	sc = (struct machfb_softc *)adp;

	if (adp->va_io_base != 0 && offset >= adp->va_io_base &&
	    offset < adp->va_io_base + adp->va_io_size) {
		*paddr = sc->sc_vioh + offset - adp->va_io_size;
		return (0);
	}

	if (adp->va_mem_base != 0 && offset >= adp->va_mem_base &&
	    offset < adp->va_mem_base + adp->va_mem_size) {
	    	*paddr = sc->sc_vmemh + offset - adp->va_mem_base;
		return (0);
	}

	if (offset >= adp->va_registers &&
	    offset < adp->va_registers + adp->va_registers_size) {
	    	*paddr = sc->sc_memh + offset - adp->va_registers;
		return (0);
	}

	/* 'regular' framebuffer mmap()ing */
	if (offset < adp->va_window_size) {
		*paddr = adp->va_window + offset;
		return (0);
	}

	return (EINVAL);
}

static int
machfb_ioctl(video_adapter_t *adp, u_long cmd, caddr_t data)
{
	struct machfb_softc *sc;
	struct fbcursor *fbc;
	struct fbtype *fb;

	sc = (struct machfb_softc *)adp;

	switch (cmd) {
	case FBIOGTYPE:
		fb = (struct fbtype *)data;
		fb->fb_type = FBTYPE_PCIMISC;
		fb->fb_height = sc->sc_height;
		fb->fb_width = sc->sc_width;
		fb->fb_depth = sc->sc_depth;
		if (sc->sc_depth <= 1 || sc->sc_depth > 8)
			fb->fb_cmsize = 0;
		else
			fb->fb_cmsize = 1 << sc->sc_depth;
		fb->fb_size = adp->va_buffer_size;
		break;
	case FBIOSCURSOR:
		fbc = (struct fbcursor *)data;
		if (fbc->set & FB_CUR_SETCUR) {
			if (fbc->enable == 0) {
				machfb_cursor_enable(sc, 0);
				sc->sc_flags &= ~MACHFB_CUREN;
			} else
				return (ENODEV);
		}
		break;
	default:
		return (fb_commonioctl(adp, cmd, data));
	}

	return (0);
}

static int
machfb_clear(video_adapter_t *adp)
{

	return (ENODEV);
}

static int
machfb_fill_rect(video_adapter_t *adp, int val, int x, int y, int cx, int cy)
{
	struct machfb_softc *sc;

	sc = (struct machfb_softc *)adp;

	if (sc->sc_draw_cache != MACHFB_DRAW_FILLRECT) {
		wait_for_fifo(sc, 7);
		regw(sc, DP_WRITE_MASK, 0xff);
		regw(sc, DP_PIX_WIDTH, DST_8BPP | SRC_8BPP | HOST_8BPP);
		regw(sc, DP_SRC, FRGD_SRC_FRGD_CLR);
		regw(sc, DP_MIX, MIX_SRC << 16);
		regw(sc, CLR_CMP_CNTL, 0);	/* no transparency */
		regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);
		regw(sc, DST_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM);
		sc->sc_draw_cache = MACHFB_DRAW_FILLRECT;
	}
	wait_for_fifo(sc, 5);
	regw(sc, DP_FRGD_CLR, val);
	regw(sc, SRC_Y_X, (x << 16) | y);
	regw(sc, SRC_WIDTH1, cx);
	regw(sc, DST_Y_X, (x << 16) | y);
	regw(sc, DST_HEIGHT_WIDTH, (cx << 16) | cy);

	return (0);
}

static int
machfb_bitblt(video_adapter_t *adp, ...)
{

	return (ENODEV);
}

static int
machfb_diag(video_adapter_t *adp, int level)
{
	video_info_t info;

	fb_dump_adp_info(adp->va_name, adp, level);
	machfb_get_info(adp, 0, &info);
	fb_dump_mode_info(adp->va_name, adp, &info, level);

	return (0);
}

static int
machfb_save_cursor_palette(video_adapter_t *adp, u_char *palette)
{

	return (ENODEV);
}

static int
machfb_load_cursor_palette(video_adapter_t *adp, u_char *palette)
{

	return (ENODEV);
}

static int
machfb_copy(video_adapter_t *adp, vm_offset_t src, vm_offset_t dst, int n)
{

	return (ENODEV);
}

static int
machfb_putp(video_adapter_t *adp, vm_offset_t off, uint32_t p, uint32_t a,
    int size, int bpp, int bit_ltor, int byte_ltor)
{

	return (ENODEV);
}

static int
machfb_putc(video_adapter_t *adp, vm_offset_t off, uint8_t c, uint8_t a)
{
	struct machfb_softc *sc;
	uint8_t *p;
	int i;

	sc = (struct machfb_softc *)adp;

	if (sc->sc_draw_cache != MACHFB_DRAW_CHAR) {
		wait_for_fifo(sc, 8);
		regw(sc, DP_WRITE_MASK, 0xff);	/* XXX only good for 8 bit */
		regw(sc, DP_PIX_WIDTH, DST_8BPP | SRC_1BPP | HOST_1BPP);
		regw(sc, DP_SRC, MONO_SRC_HOST | BKGD_SRC_BKGD_CLR |
		    FRGD_SRC_FRGD_CLR);
		regw(sc, DP_MIX ,((MIX_SRC & 0xffff) << 16) | MIX_SRC);
		regw(sc, CLR_CMP_CNTL, 0);	/* no transparency */
		regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);
		regw(sc, DST_CNTL, DST_Y_TOP_TO_BOTTOM | DST_X_LEFT_TO_RIGHT);
		regw(sc, HOST_CNTL, HOST_BYTE_ALIGN);
		sc->sc_draw_cache = MACHFB_DRAW_CHAR;
	}
	p = sc->sc_font + (c * adp->va_info.vi_cheight * sc->sc_cbwidth);
	wait_for_fifo(sc, 6 + (adp->va_info.vi_cheight / sc->sc_cbwidth));
	regw(sc, DP_BKGD_CLR, (a >> 4) & 0xf);
	regw(sc, DP_FRGD_CLR, a & 0xf);
	regw(sc, SRC_Y_X, 0);
	regw(sc, SRC_WIDTH1, adp->va_info.vi_cwidth);
	regw(sc, DST_Y_X, ((((off % adp->va_info.vi_width) *
	    adp->va_info.vi_cwidth) + sc->sc_xmargin) << 16) |
	    (((off / adp->va_info.vi_width) * adp->va_info.vi_cheight) +
	    sc->sc_ymargin));
	regw(sc, DST_HEIGHT_WIDTH, (adp->va_info.vi_cwidth << 16) |
	    adp->va_info.vi_cheight);
	for (i = 0; i < adp->va_info.vi_cheight * sc->sc_cbwidth; i += 4)
		regw(sc, HOST_DATA0 + i, (p[i + 3] << 24 | p[i + 2] << 16 |
		    p[i + 1] << 8 | p[i]));

	return (0);
}

static int
machfb_puts(video_adapter_t *adp, vm_offset_t off, uint16_t *s, int len)
{
	struct machfb_softc *sc;
	int blanks, i, x1, x2, y1, y2;
	uint8_t a, c, color1, color2;

	sc = (struct machfb_softc *)adp;

#define	MACHFB_BLANK	machfb_fill_rect(adp, color1, x1, y1,		\
			    blanks * adp->va_info.vi_cwidth,		\
			    adp->va_info.vi_cheight)

	blanks = color1 = x1 = y1 = 0;
	for (i = 0; i < len; i++) {
		/*
		 * Accelerate continuous blanks by drawing a respective
		 * rectangle instead. Drawing a rectangle of any size
		 * takes about the same number of operations as drawing
		 * a single character.
		 */
		c = s[i] & 0xff;
		a = (s[i] & 0xff00) >> 8;
		if (c == 0x00 || c == 0x20 || c == 0xdb || c == 0xff) {
			color2 = (a >> (c == 0xdb ? 0 : 4) & 0xf);
	    		x2 = (((off + i) % adp->va_info.vi_width) *
			    adp->va_info.vi_cwidth) + sc->sc_xmargin;
			y2 = (((off + i) / adp->va_info.vi_width) *
			    adp->va_info.vi_cheight) + sc->sc_ymargin;
			if (blanks == 0) {
				color1 = color2;
				x1 = x2;
				y1 = y2;
				blanks++;
			} else if (color1 != color2 || y1 != y2) {
				MACHFB_BLANK;
				color1 = color2;
				x1 = x2;
				y1 = y2;
				blanks = 1;
			} else
				blanks++;
		} else {
			if (blanks != 0) {
				MACHFB_BLANK;
				blanks = 0;
			}
			(*vidsw[adp->va_index]->putc)(adp, off + i, c, a);
		}
	}
	if (blanks != 0)
		MACHFB_BLANK;

#undef MACHFB_BLANK

	return (0);
}

static int
machfb_putm(video_adapter_t *adp, int x, int y, uint8_t *pixel_image,
    uint32_t pixel_mask, int size)
{
	struct machfb_softc *sc;
	int error;

	sc = (struct machfb_softc *)adp;

	if ((!(sc->sc_flags & MACHFB_CUREN)) &&
	    (error = machfb_cursor_install(sc)) < 0)
	    	return (error);
	else {
		/*
		 * The hardware cursor always must be disabled when
		 * fiddling with its bits otherwise some artifacts
		 * may appear on the screen.
		 */
		machfb_cursor_enable(sc, 0);
	}

	regw(sc, CUR_HORZ_VERT_OFF, 0);
	if ((regr(sc, GEN_TEST_CNTL) & CRTC_DBL_SCAN_EN) != 0)
		y <<= 1;
	regw(sc, CUR_HORZ_VERT_POSN, ((y + sc->sc_ymargin) << 16) |
	    (x + sc->sc_xmargin));
	machfb_cursor_enable(sc, 1);
	sc->sc_flags |= MACHFB_CUREN;

	return (0);
}

/*
 * PCI bus interface
 */
static int
machfb_pci_probe(device_t dev)
{
	int i;

	if (pci_get_class(dev) != PCIC_DISPLAY ||
	    pci_get_subclass(dev) != PCIS_DISPLAY_VGA)
		return (ENXIO);

	for (i = 0; i < sizeof(machfb_info) / sizeof(machfb_info[0]); i++) {
		if (pci_get_device(dev) == machfb_info[i].chip_id) {
			device_set_desc(dev, machfb_info[i].name);
			return (BUS_PROBE_DEFAULT);
		}
	}
	
	return (ENXIO);
}

static int
machfb_pci_attach(device_t dev)
{
	struct machfb_softc *sc;
	video_adapter_t *adp;
	video_switch_t *sw;
	phandle_t node;
	uint32_t *p32, saved_value;
	uint8_t *p;
	int error, i;

	node = ofw_bus_get_node(dev);
	if ((sc = (struct machfb_softc *)vid_get_adapter(vid_find_adapter(
	    MACHFB_DRIVER_NAME, 0))) != NULL && sc->sc_node == node) {
	    	device_printf(dev, "console\n");
	    	device_set_softc(dev, sc);
	} else {
		sc = device_get_softc(dev);
		bzero(sc, sizeof(struct machfb_softc));

		sc->sc_node = node;
		sc->sc_chip_id = pci_get_device(dev);
		sc->sc_chip_rev = pci_get_revid(dev);
	}
	adp = &sc->sc_va;
		
	/*
	 * Regardless whether we are the console and already allocated
	 * resources in machfb_configure() or not we have to allocate
	 * them here (again) in order for rman_get_virtual() to work.
	 */

	/* Enable memory and IO access. */
	pci_write_config(dev, PCIR_COMMAND,
	    pci_read_config(dev, PCIR_COMMAND, 2) | PCIM_CMD_PORTEN |
	    PCIM_CMD_MEMEN, 2);
	
	sc->sc_memrid = PCIR_BAR(0);
	if ((sc->sc_memres = bus_alloc_resource_any(dev, SYS_RES_MEMORY,
	    &sc->sc_memrid, RF_ACTIVE)) == NULL) {
		device_printf(dev, "cannot allocate memory resources\n");
		return (ENXIO);
	}
	sc->sc_memt = rman_get_bustag(sc->sc_memres);
	sc->sc_memh = rman_get_bushandle(sc->sc_memres);
	adp->va_registers = rman_get_start(sc->sc_memres);
	adp->va_registers_size = rman_get_size(sc->sc_memres);
	sc->sc_regt = sc->sc_memt;
	bus_space_subregion(sc->sc_regt, sc->sc_memh, MACH64_REG_OFF,
	    MACH64_REG_SIZE, &sc->sc_regh);
    	adp->va_buffer = (vm_offset_t)rman_get_virtual(sc->sc_memres);
	adp->va_buffer_size = rman_get_size(sc->sc_memres);

	/*
	 * Depending on the firmware version the VGA I/O and/or memory
	 * resources of the Mach64 chips come up disabled. We generally
	 * enable them above (pci(4) actually already did this unless
	 * pci_enable_io_modes is not set) but this doesn't necessarily
	 * mean that we get valid ones. Invalid resources seem to have
	 * in common that they start at address 0. We don't allocate
	 * them in this case in order to avoid warnings in apb(4) and
	 * crashes when using these invalid resources. Xorg is aware
	 * of this and doesn't use the VGA resources in this case (but
	 * demands them if they are valid).
	 */
	sc->sc_viorid = PCIR_BAR(1);
	if (bus_get_resource_start(dev, SYS_RES_IOPORT, sc->sc_viorid) != 0) {
		if ((sc->sc_viores = bus_alloc_resource_any(dev,
		    SYS_RES_IOPORT, &sc->sc_viorid, RF_ACTIVE)) == NULL) {
			device_printf(dev,
			    "cannot allocate VGA I/O resources\n");
			error = ENXIO;
			goto fail_memres;
		}
		sc->sc_viot = rman_get_bustag(sc->sc_viores);
		sc->sc_vioh = rman_get_bushandle(sc->sc_viores);
		adp->va_io_base = rman_get_start(sc->sc_viores);
		adp->va_io_size = rman_get_size(sc->sc_viores);
	}

	sc->sc_vmemrid = PCIR_BAR(2);
	if (bus_get_resource_start(dev, SYS_RES_MEMORY, sc->sc_vmemrid) != 0) {
		if ((sc->sc_vmemres = bus_alloc_resource_any(dev,
		    SYS_RES_MEMORY, &sc->sc_vmemrid, RF_ACTIVE)) == NULL) {
			device_printf(dev,
			    "cannot allocate VGA memory resources\n");
			error = ENXIO;
			goto fail_viores;
		}
		sc->sc_vmemt = rman_get_bustag(sc->sc_vmemres);
		sc->sc_vmemh = rman_get_bushandle(sc->sc_vmemres);
		adp->va_mem_base = rman_get_start(sc->sc_vmemres);
		adp->va_mem_size = rman_get_size(sc->sc_vmemres);
	}

	device_printf(dev,
	    "%d MB aperture at 0x%08x, %d KB registers at 0x%08x\n",
	    (u_int)(adp->va_buffer_size / (1024 * 1024)),
	    (u_int)adp->va_buffer, MACH64_REG_SIZE / 1024,
	    (u_int)sc->sc_regh);

	if (!(sc->sc_flags & MACHFB_CONSOLE)) {
		if ((sw = vid_get_switch(MACHFB_DRIVER_NAME)) == NULL) {
			device_printf(dev, "cannot get video switch\n");
			error = ENODEV;
			goto fail_vmemres;
		}
		/*
		 * During device configuration we don't necessarily probe
		 * the adapter which is the console first so we can't use
		 * the device unit number for the video adapter unit. The
		 * worst case would be that we use the video adapter unit
		 * 0 twice. As it doesn't really matter which unit number
		 * the corresponding video adapter has just use the next
		 * unused one.
		 */
		for (i = 0; i < devclass_get_maxunit(machfb_devclass); i++)
			if (vid_find_adapter(MACHFB_DRIVER_NAME, i) < 0)
				break;
		if ((error = sw->init(i, adp, 0)) != 0) {
			device_printf(dev, "cannot initialize adapter\n");
		    	goto fail_vmemres;
		}
	}

	device_printf(dev,
	    "%ld KB %s %d.%d MHz, maximum RAMDAC clock %d MHz, %sDSP\n",
	    (u_long)sc->sc_memsize, machfb_memtype_names[sc->sc_memtype],
	    sc->sc_mem_freq / 1000, sc->sc_mem_freq % 1000,
	    sc->sc_ramdac_freq / 1000,
	    (sc->sc_flags & MACHFB_DSP) ? "" : "no ");
	device_printf(dev, "resolution %dx%d at %d bpp\n",
	    sc->sc_width, sc->sc_height, sc->sc_depth);

	/*
	 * Test whether the aperture is byte swapped or not, set
	 * va_window and va_window_size as appropriate.
	 */
	p32 = (uint32_t *)adp->va_buffer;
	saved_value = *p32;
	p = (uint8_t *)adp->va_buffer;
	*p32 = 0x12345678;
	if (!(p[0] == 0x12 && p[1] == 0x34 && p[2] == 0x56 && p[3] == 0x78)) {
		adp->va_window = adp->va_buffer + 0x800000;
		adp->va_window_size = adp->va_buffer_size - 0x800000;
	} else {
		adp->va_window = adp->va_buffer;
		adp->va_window_size = adp->va_buffer_size;
	}
	*p32 = saved_value;
	adp->va_window_gran = adp->va_window_size;

	/*
	 * Allocate one page for the mouse pointer image at the end of
	 * the little endian aperture, right before the memory mapped
	 * registers that might also reside there. Must be done after
	 * sc_memsize was set and possibly adjusted to account for the
	 * memory mapped registers.
	 */
	sc->sc_curoff = (sc->sc_memsize * 1024) - PAGE_SIZE;
	sc->sc_memsize -= PAGE_SIZE / 1024;
	machfb_cursor_enable(sc, 0);
	/* Initialize with an all transparent image. */
	memset((void *)(adp->va_buffer + sc->sc_curoff), 0xaa, PAGE_SIZE);

	/*
	 * For cosmetics register a handler that turns off the mouse
	 * pointer on halt. Register a second handler that turns off
	 * the CRTC when resetting, otherwise the OFW boot command
	 * issued by cpu_reset() just doesn't work.
	 */
	EVENTHANDLER_REGISTER(shutdown_final, machfb_shutdown_final, sc,
	    SHUTDOWN_PRI_DEFAULT);
	EVENTHANDLER_REGISTER(shutdown_reset, machfb_shutdown_reset, sc,
	    SHUTDOWN_PRI_DEFAULT);

	return (0);

 fail_vmemres:
 	if (sc->sc_vmemres != NULL)
	 	bus_release_resource(dev, SYS_RES_MEMORY, sc->sc_vmemrid,
		    sc->sc_vmemres);
 fail_viores:
 	if (sc->sc_viores != NULL)
	 	bus_release_resource(dev, SYS_RES_IOPORT, sc->sc_viorid,
		    sc->sc_viores);
 fail_memres:
	bus_release_resource(dev, SYS_RES_MEMORY, sc->sc_memrid, sc->sc_memres);

	return (error);
}

static int
machfb_pci_detach(device_t dev)
{

	return (EINVAL);
}

/*
 * internal functions
 */
static void
machfb_cursor_enable(struct machfb_softc *sc, int onoff)
{

	if (onoff)
		regw(sc, GEN_TEST_CNTL,
		    regr(sc, GEN_TEST_CNTL) | HWCURSOR_ENABLE);
	else
		regw(sc, GEN_TEST_CNTL,
		    regr(sc, GEN_TEST_CNTL) &~ HWCURSOR_ENABLE);
}

static int
machfb_cursor_install(struct machfb_softc *sc)
{
	uint16_t *p;
	uint8_t fg;
	int i, j;

	if (sc->sc_curoff == 0)
		return (ENODEV);

	machfb_cursor_enable(sc, 0);
	regw(sc, CUR_OFFSET, sc->sc_curoff >> 3);
	fg = SC_NORM_ATTR & 0xf;
	regw(sc, CUR_CLR0, sc->sc_cmap_red[fg] << 24 |
	    sc->sc_cmap_green[fg] << 16 | sc->sc_cmap_blue[fg] << 8 | fg);
	p = (uint16_t *)(sc->sc_va.va_buffer + sc->sc_curoff);
	for (i = 0; i < 64; i++)
		for (j = 0; j < 8; j++)
			*(p++) = machfb_mouse_pointer_lut[
			    machfb_mouse_pointer_bits[i][j] >> 4] |
			    machfb_mouse_pointer_lut[
			    machfb_mouse_pointer_bits[i][j] & 0x0f] << 8;

	return (0);
}

static int
machfb_get_memsize(struct machfb_softc *sc)
{
	int tmp, memsize;
	int mem_tab[] = {
		512, 1024, 2048, 4096, 6144, 8192, 12288, 16384
	};

	tmp = regr(sc, MEM_CNTL);
#ifdef MACHFB_DEBUG
	printf("memcntl=0x%08x\n", tmp);
#endif
	if (sc->sc_flags & MACHFB_DSP) {
		tmp &= 0x0000000f;
		if (tmp < 8)
			memsize = (tmp + 1) * 512;
		else if (tmp < 12)
			memsize = (tmp - 3) * 1024;
		else
			memsize = (tmp - 7) * 2048;
	} else
		memsize = mem_tab[tmp & 0x07];

	return (memsize);
}

static void
machfb_reset_engine(struct machfb_softc *sc)
{

	/* Reset engine.*/
	regw(sc, GEN_TEST_CNTL, regr(sc, GEN_TEST_CNTL) & ~GUI_ENGINE_ENABLE);

	/* Enable engine. */
	regw(sc, GEN_TEST_CNTL, regr(sc, GEN_TEST_CNTL) | GUI_ENGINE_ENABLE);

	/*
	 * Ensure engine is not locked up by clearing any FIFO or
	 * host errors.
	 */
	regw(sc, BUS_CNTL, regr(sc, BUS_CNTL) | BUS_HOST_ERR_ACK |
	    BUS_FIFO_ERR_ACK);
}

static void
machfb_init_engine(struct machfb_softc *sc)
{
	uint32_t pitch_value;

	pitch_value = sc->sc_width;

	if (sc->sc_depth == 24)
		pitch_value *= 3;

	machfb_reset_engine(sc);

	wait_for_fifo(sc, 14);

	regw(sc, CONTEXT_MASK, 0xffffffff);

	regw(sc, DST_OFF_PITCH, (pitch_value / 8) << 22);

	regw(sc, DST_Y_X, 0);
	regw(sc, DST_HEIGHT, 0);
	regw(sc, DST_BRES_ERR, 0);
	regw(sc, DST_BRES_INC, 0);
	regw(sc, DST_BRES_DEC, 0);

	regw(sc, DST_CNTL, DST_LAST_PEL | DST_X_LEFT_TO_RIGHT |
	    DST_Y_TOP_TO_BOTTOM);

	regw(sc, SRC_OFF_PITCH, (pitch_value / 8) << 22);

	regw(sc, SRC_Y_X, 0);
	regw(sc, SRC_HEIGHT1_WIDTH1, 1);
	regw(sc, SRC_Y_X_START, 0);
	regw(sc, SRC_HEIGHT2_WIDTH2, 1);

	regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);

	wait_for_fifo(sc, 13);
	regw(sc, HOST_CNTL, 0);

	regw(sc, PAT_REG0, 0);
	regw(sc, PAT_REG1, 0);
	regw(sc, PAT_CNTL, 0);

	regw(sc, SC_LEFT, 0);
	regw(sc, SC_TOP, 0);
	regw(sc, SC_BOTTOM, sc->sc_height - 1);
	regw(sc, SC_RIGHT, pitch_value - 1);

	regw(sc, DP_BKGD_CLR, 0);
	regw(sc, DP_FRGD_CLR, 0xffffffff);
	regw(sc, DP_WRITE_MASK, 0xffffffff);
	regw(sc, DP_MIX, (MIX_SRC << 16) | MIX_DST);

	regw(sc, DP_SRC, FRGD_SRC_FRGD_CLR);

	wait_for_fifo(sc, 3);
	regw(sc, CLR_CMP_CLR, 0);
	regw(sc, CLR_CMP_MASK, 0xffffffff);
	regw(sc, CLR_CMP_CNTL, 0);

	wait_for_fifo(sc, 2);
	switch (sc->sc_depth) {
	case 8:
		regw(sc, DP_PIX_WIDTH, HOST_8BPP | SRC_8BPP | DST_8BPP);
		regw(sc, DP_CHAIN_MASK, DP_CHAIN_8BPP);
		regw(sc, DAC_CNTL, regr(sc, DAC_CNTL) | DAC_8BIT_EN);
		break;
#if 0
	case 32:
		regw(sc, DP_PIX_WIDTH, HOST_32BPP | SRC_32BPP | DST_32BPP);
		regw(sc, DP_CHAIN_MASK, DP_CHAIN_32BPP);
		regw(sc, DAC_CNTL, regr(sc, DAC_CNTL) | DAC_8BIT_EN);
		break;
#endif
	}

	wait_for_fifo(sc, 5);
	regw(sc, CRTC_INT_CNTL, regr(sc, CRTC_INT_CNTL) & ~0x20);
	regw(sc, GUI_TRAJ_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM);

	wait_for_idle(sc);
}

#if 0
static void
machfb_adjust_frame(struct machfb_softc *sc, int x, int y)
{
	int offset;

	offset = ((x + y * sc->sc_width) * (sc->sc_depth >> 3)) >> 3;

	regw(sc, CRTC_OFF_PITCH, (regr(sc, CRTC_OFF_PITCH) & 0xfff00000) |
	     offset);
}
#endif

static void
machfb_putpalreg(struct machfb_softc *sc, uint8_t index, uint8_t r, uint8_t g,
    uint8_t b)
{

	sc->sc_cmap_red[index] = r;
	sc->sc_cmap_green[index] = g;
	sc->sc_cmap_blue[index] = b;
	/*
	 * Writing the DAC index takes a while, in theory we can poll some
	 * register to see when it's ready - but we better avoid writing it
	 * unnecessarily.
	 */
	if (index != sc->sc_dacw) {
		regwb(sc, DAC_MASK, 0xff);
		regwb(sc, DAC_WINDEX, index);
	}
	sc->sc_dacw = index + 1;
	regwb(sc, DAC_DATA, r);
	regwb(sc, DAC_DATA, g);
	regwb(sc, DAC_DATA, b);
}

static void
machfb_shutdown_final(void *v)
{
	struct machfb_softc *sc = v;

	machfb_cursor_enable(sc, 0);
}

static void
machfb_shutdown_reset(void *v)
{
	struct machfb_softc *sc = v;

	machfb_blank_display(&sc->sc_va, V_DISPLAY_STAND_BY);
}
