/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <ieeefp.h>

fp_except_t
fpsetmask(mask)
	fp_except_t mask;
{
	fp_except_t old;
	fp_except_t new;

	__asm__("st %%fsr,%0" : "=m" (*&old));

	new = old;
	new &= ~(0x1f << 23); 
	new |= ((mask & 0x1f) << 23);

	__asm__("ld %0,%%fsr" : : "m" (*&new));

	return (old >> 23) & 0x1f;
}
