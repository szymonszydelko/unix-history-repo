/* $Id */
/* $NetBSD: tlsb.c,v 1.10 1998/05/14 00:01:32 thorpej Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Based in part upon a prototype version by Jason Thorpe
 * Copyright (c) 1996 by Jason Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Autoconfiguration and support routines for the TurboLaser System Bus
 * found on AlphaServer 8200 and 8400 systems.
 */

#include "opt_simos.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/malloc.h>

#include <machine/rpb.h>
#include <machine/cpuconf.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>

/* #include "locators.h" */

extern int	cputype;

#define KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))

/*
 * The structure used to attach devices to the TurboLaser.
 */
struct tlsb_device {
	int		td_node;	/* node number */
	u_int16_t	td_dtype;	/* device type */
	u_int8_t	td_swrev;	/* software revision */
	u_int8_t	td_hwrev;	/* hardware revision */
};
#define DEVTOTLSB(dev)	((struct tlsb_device*) device_get_ivars(dev))

struct intr_mapping {
	STAILQ_ENTRY(intr_mapping) queue;
	driver_intr_t*	intr;
	void*		arg;
};

struct tlsb_softc {
	struct bus bus;
	STAILQ_HEAD(, intr_mapping) intr_handlers;
};

static char	*tlsb_node_type_str(u_int32_t);
static void	tlsb_intr(void* frame, u_long vector);

/* There can be only one. */
static int	tlsb_found;
static struct tlsb_softc* tlsb0_softc;

/*
 * Bus handlers.
 */
static bus_print_device_t	tlsb_print_device;
static bus_read_ivar_t		tlsb_read_ivar;
static bus_map_intr_t		tlsb_map_intr;

static bus_ops_t tlsb_bus_ops = {
	tlsb_print_device,
	tlsb_read_ivar,
	null_write_ivar,
	tlsb_map_intr,
};

static void
tlsb_print_device(bus_t bus, device_t dev)
{
	device_t busdev = bus_get_device(bus);
	struct tlsb_device* tdev = DEVTOTLSB(dev);

	printf(" at %s%d node %d",
	       device_get_name(busdev),
	       device_get_unit(busdev),
	       tdev->td_node);
}

static int
tlsb_read_ivar(bus_t bus, device_t dev,
	       int index, u_long* result)
{
	struct tlsb_device* tdev = DEVTOTLSB(dev);

	switch (index) {
	case TLSB_IVAR_NODE:
		*result = tdev->td_node;
		break;

	case TLSB_IVAR_DTYPE:
		*result = tdev->td_dtype;
		break;

	case TLSB_IVAR_SWREV:
		*result = tdev->td_swrev;
		break;

	case TLSB_IVAR_HWREV:
		*result = tdev->td_hwrev;
		break;
	}
	return ENOENT;
}

static int
tlsb_map_intr(bus_t bus, device_t dev, driver_intr_t *intr, void *arg)
{
	struct tlsb_softc* sc = (struct tlsb_softc*) bus;
	struct intr_mapping* i;
	i = malloc(sizeof(struct intr_mapping), M_DEVBUF, M_NOWAIT);
	if (!i)
		return ENOMEM;
	i->intr = intr;
	i->arg = arg;
	STAILQ_INSERT_TAIL(&sc->intr_handlers, i, queue);

	return 0;
}

static driver_probe_t tlsb_bus_probe;
static devclass_t tlsb_devclass;

static driver_t tlsb_bus_driver = {
	"tlsb",
	tlsb_bus_probe,
	bus_generic_attach,
	bus_generic_detach,
	bus_generic_shutdown,
	DRIVER_TYPE_MISC,
	sizeof(struct tlsb_softc),
	NULL,
};

/*
 * At 'probe' time, we add all the devices which we know about to the
 * bus.  The generic attach routine will probe and attach them if they
 * are alive.
 */
static int
tlsb_bus_probe(bus_t parent, device_t dev)
{
	struct tlsb_softc* sc = device_get_softc(dev);
	struct tlsb_device *tdev;
	u_int32_t tldev;
	u_int8_t vid;
	int node;
	device_t subdev;

	device_set_desc(dev, "TurboLaser bus");
	bus_init(&sc->bus, dev, &tlsb_bus_ops);
	STAILQ_INIT(&sc->intr_handlers);
	tlsb0_softc = sc;

	set_iointr(tlsb_intr);

	printf("Probing for devices on the TurboLaser bus:\n");

	tlsb_found = 1;

	/*
	 * Attempt to find all devices on the bus, including
	 * CPUs, memory modules, and I/O modules.
	 */

	/*
	 * Sigh. I would like to just start off nicely,
	 * but I need to treat I/O modules differently-
	 * The highest priority I/O node has to be in
	 * node #8, and I want to find it *first*, since
	 * it will have the primary disks (most likely)
	 * on it.
	 */
	/*
	 * XXX dfr: I don't see why I need to do this
	 */
	for (node = 0; node <= TLSB_NODE_MAX; ++node) {
		/*
		 * Check for invalid address.  This may not really
		 * be necessary, but what the heck...
		 */
		if (badaddr(TLSB_NODE_REG_ADDR(node, TLDEV), sizeof(u_int32_t)))
			continue;
		tldev = TLSB_GET_NODEREG(node, TLDEV);
#ifdef SIMOS
		if (node != 0 && node != 8)
			continue;
#endif
		if (tldev == 0) {
			/* Nothing at this node. */
			continue;
		}
#if 0
		if (TLDEV_ISIOPORT(tldev))
			continue;	/* not interested right now */
#endif

		tdev = (struct tlsb_device*)
			malloc(sizeof(struct tlsb_device),
			       M_DEVBUF, M_NOWAIT);

		if (!tdev)
			continue;

		tdev->td_node = node;
#ifdef SIMOS
		if (node == 0)
			tdev->td_dtype = TLDEV_DTYPE_SCPU4;
		else if (node == 8)
			tdev->td_dtype = TLDEV_DTYPE_KFTIA;
#else
		tdev->td_dtype = TLDEV_DTYPE(tldev);
#endif
		tdev->td_swrev = TLDEV_SWREV(tldev);
		tdev->td_hwrev = TLDEV_HWREV(tldev);

		subdev = bus_add_device(&sc->bus, NULL, -1, tdev);
		device_set_desc(subdev, tlsb_node_type_str(tdev->td_dtype));

		/*
		 * Deal with hooking CPU instances to TurboLaser nodes.
		 */
		if (TLDEV_ISCPU(tldev)) {
			printf("%s%d node %d: %s",
			       device_get_name(dev), device_get_unit(dev),
			       node, tlsb_node_type_str(tldev));

			/*
			 * Hook in the first CPU unit.
			 */
			vid = (TLSB_GET_NODEREG(node, TLVID) &
			    TLVID_VIDA_MASK) >> TLVID_VIDA_SHIFT;
			printf(", VID %d\n", vid);
			TLSB_PUT_NODEREG(node, TLCPUMASK, (1<<vid));
		}
	}

	return 0;
}

static void
tlsb_intr(void* frame, u_long vector)
{
	struct tlsb_softc* sc = tlsb0_softc;
	struct intr_mapping* i;

	/*
	 * XXX for SimOS, broadcast the interrupt.  A real implementation
	 * will decode the vector to extract node and host etc.
	 */
	for (i = STAILQ_FIRST(&sc->intr_handlers);
	     i; i = STAILQ_NEXT(i, queue))
		i->intr(i->arg);
}

DRIVER_MODULE(tlsb, root, tlsb_bus_driver, tlsb_devclass, 0, 0);

static char *
tlsb_node_type_str(u_int32_t dtype)
{
	static char	tlsb_line[64];

	switch (dtype & TLDEV_DTYPE_MASK) {
	case TLDEV_DTYPE_KFTHA:
		return ("KFTHA I/O interface");

	case TLDEV_DTYPE_KFTIA:
		return ("KFTIA I/O interface");

	case TLDEV_DTYPE_MS7CC:
		return ("MS7CC Memory Module");

	case TLDEV_DTYPE_SCPU4:
		return ("Single CPU, 4MB cache");

	case TLDEV_DTYPE_SCPU16:
		return ("Single CPU, 16MB cache");

	case TLDEV_DTYPE_DCPU4:
		return ("Dual CPU, 4MB cache");

	case TLDEV_DTYPE_DCPU16:
		return ("Dual CPU, 16MB cache");

	default:
		bzero(tlsb_line, sizeof(tlsb_line));
		sprintf(tlsb_line, "unknown, dtype 0x%x", dtype);
		return (tlsb_line);
	}
	/* NOTREACHED */
}
