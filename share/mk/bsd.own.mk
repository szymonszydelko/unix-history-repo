#	$Id: bsd.own.mk,v 1.2 1994/09/16 14:30:21 jkh Exp $

.if !defined(DEBUG_FLAGS)
STRIP?=		-s
.endif

COPY?=		-c


BINOWN?=	bin
BINGRP?=	bin
BINMODE?=	555

MANDIR?=	/usr/share/man/man
MANOWN?=	bin
MANGRP?=	bin
MANMODE?=	444

DOCDIR?=	/usr/share
DOCOWN?=	bin
DOCGRP?=	bin
DOCMODE?=	444

LIBDIR?=	/usr/lib
LINTLIBDIR?=	/usr/libdata/lint
SHLIBDIR?=	${LIBDIR}
LIBOWN?=	bin
LIBGRP?=	bin
LIBMODE?=	444

KMODDIR=	/lkm
KMODOWN?=	bin
KMODGRP?=	bin
KMODMODE?=	555
