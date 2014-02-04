/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _RPCB_PROT_H_RPCGEN
#define	_RPCB_PROT_H_RPCGEN

#include <rpc/rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR   
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
/* from rpcb_prot.x */

/* #pragma ident	"@(#)rpcb_prot.x	1.5	94/04/29 SMI" */

#ifndef _KERNEL


/*
 * The following procedures are supported by the protocol in version 3:
 *
 * RPCBPROC_NULL() returns ()
 * 	takes nothing, returns nothing
 *
 * RPCBPROC_SET(rpcb) returns (bool_t)
 * 	TRUE is success, FALSE is failure.  Registers the tuple
 *	[prog, vers, address, owner, netid].
 *	Finds out owner and netid information on its own.
 *
 * RPCBPROC_UNSET(rpcb) returns (bool_t)
 *	TRUE is success, FALSE is failure.  Un-registers tuple
 *	[prog, vers, netid].  addresses is ignored.
 *	If netid is NULL, unregister all.
 *
 * RPCBPROC_GETADDR(rpcb) returns (string).
 *	0 is failure.  Otherwise returns the universal address where the
 *	triple [prog, vers, netid] is registered.  Ignore address and owner.
 *
 * RPCBPROC_DUMP() RETURNS (rpcblist_ptr)
 *	used to dump the entire rpcbind maps
 *
 * RPCBPROC_CALLIT(rpcb_rmtcallargs)
 * 	RETURNS (rpcb_rmtcallres);
 * 	Calls the procedure on the remote machine.  If it is not registered,
 *	this procedure is quiet; i.e. it does not return error information!!!
 *	This routine only passes null authentication parameters.
 *	It has no interface to xdr routines for RPCBPROC_CALLIT.
 *
 * RPCBPROC_GETTIME() returns (int).
 *	Gets the remote machines time
 *
 * RPCBPROC_UADDR2TADDR(strint) RETURNS (struct netbuf)
 *	Returns the netbuf address from universal address.
 *
 * RPCBPROC_TADDR2UADDR(struct netbuf) RETURNS (string)
 *	Returns the universal address from netbuf address.
 *
 * END OF RPCBIND VERSION 3 PROCEDURES
 */
/*
 * Except for RPCBPROC_CALLIT, the procedures above are carried over to
 * rpcbind version 4.  Those below are added or modified for version 4.
 * NOTE: RPCBPROC_BCAST HAS THE SAME FUNCTIONALITY AND PROCEDURE NUMBER
 * AS RPCBPROC_CALLIT.
 *
 * RPCBPROC_BCAST(rpcb_rmtcallargs)
 * 	RETURNS (rpcb_rmtcallres);
 * 	Calls the procedure on the remote machine.  If it is not registered,
 *	this procedure IS quiet; i.e. it DOES NOT return error information!!!
 *	This routine should be used for broadcasting and nothing else.
 *
 * RPCBPROC_GETVERSADDR(rpcb) returns (string).
 *	0 is failure.  Otherwise returns the universal address where the
 *	triple [prog, vers, netid] is registered.  Ignore address and owner.
 *	Same as RPCBPROC_GETADDR except that if the given version number
 *	is not available, the address is not returned.
 *
 * RPCBPROC_INDIRECT(rpcb_rmtcallargs)
 * 	RETURNS (rpcb_rmtcallres);
 * 	Calls the procedure on the remote machine.  If it is not registered,
 *	this procedure is NOT quiet; i.e. it DOES return error information!!!
 * 	as any normal application would expect.
 *
 * RPCBPROC_GETADDRLIST(rpcb) returns (rpcb_entry_list_ptr).
 *	Same as RPCBPROC_GETADDR except that it returns a list of all the
 *	addresses registered for the combination (prog, vers) (for all
 *	transports).
 *
 * RPCBPROC_GETSTAT(void) returns (rpcb_stat_byvers)
 *	Returns the statistics about the kind of requests received by rpcbind.
 */

/*
 * A mapping of (program, version, network ID) to address
 */

struct rpcb {
	rpcprog_t r_prog;
	rpcvers_t r_vers;
	char *r_netid;
	char *r_addr;
	char *r_owner;
};
typedef struct rpcb rpcb;

typedef rpcb RPCB;


/*
 * A list of mappings
 *
 * Below are two definitions for the rpcblist structure.  This is done because
 * xdr_rpcblist() is specified to take a struct rpcblist **, rather than a
 * struct rpcblist * that rpcgen would produce.  One version of the rpcblist
 * structure (actually called rp__list) is used with rpcgen, and the other is
 * defined only in the header file for compatibility with the specified
 * interface.
 */

struct rp__list {
	rpcb rpcb_map;
	struct rp__list *rpcb_next;
};
typedef struct rp__list rp__list;

typedef rp__list *rpcblist_ptr;

typedef struct rp__list rpcblist;
typedef struct rp__list RPCBLIST;

#ifndef __cplusplus
struct rpcblist {
 RPCB rpcb_map;
 struct rpcblist *rpcb_next;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern bool_t xdr_rpcblist(XDR *, rpcblist**);
#ifdef __cplusplus
}
#endif


/*
 * Arguments of remote calls
 */

struct rpcb_rmtcallargs {
	rpcprog_t prog;
	rpcvers_t vers;
	rpcproc_t proc;
	struct {
		u_int args_len;
		char *args_val;
	} args;
};
typedef struct rpcb_rmtcallargs rpcb_rmtcallargs;

/*
 * Client-side only representation of rpcb_rmtcallargs structure.
 *
 * The routine that XDRs the rpcb_rmtcallargs structure must deal with the
 * opaque arguments in the "args" structure.  xdr_rpcb_rmtcallargs() needs to
 * be passed the XDR routine that knows the args' structure.  This routine
 * doesn't need to go over-the-wire (and it wouldn't make sense anyway) since
 * the application being called already knows the args structure.  So we use a
 * different "XDR" structure on the client side, r_rpcb_rmtcallargs, which
 * includes the args' XDR routine.
 */
struct r_rpcb_rmtcallargs {
 rpcprog_t prog;
 rpcvers_t vers;
 rpcproc_t proc;
 struct {
 u_int args_len;
 char *args_val;
 } args;
 xdrproc_t xdr_args; /* encodes args */
};


/*
 * Results of the remote call
 */

struct rpcb_rmtcallres {
	char *addr;
	struct {
		u_int results_len;
		char *results_val;
	} results;
};
typedef struct rpcb_rmtcallres rpcb_rmtcallres;

/*
 * Client-side only representation of rpcb_rmtcallres structure.
 */
struct r_rpcb_rmtcallres {
 char *addr;
 struct {
 uint32_t results_len;
 char *results_val;
 } results;
 xdrproc_t xdr_res; /* decodes results */
};

/*
 * rpcb_entry contains a merged address of a service on a particular
 * transport, plus associated netconfig information.  A list of rpcb_entrys
 * is returned by RPCBPROC_GETADDRLIST.  See netconfig.h for values used
 * in r_nc_* fields.
 */

struct rpcb_entry {
	char *r_maddr;
	char *r_nc_netid;
	u_int r_nc_semantics;
	char *r_nc_protofmly;
	char *r_nc_proto;
};
typedef struct rpcb_entry rpcb_entry;

/*
 * A list of addresses supported by a service.
 */

struct rpcb_entry_list {
	rpcb_entry rpcb_entry_map;
	struct rpcb_entry_list *rpcb_entry_next;
};
typedef struct rpcb_entry_list rpcb_entry_list;

typedef rpcb_entry_list *rpcb_entry_list_ptr;

/*
 * rpcbind statistics
 */

#define	rpcb_highproc_2 RPCBPROC_CALLIT
#define	rpcb_highproc_3 RPCBPROC_TADDR2UADDR
#define	rpcb_highproc_4 RPCBPROC_GETSTAT
#define	RPCBSTAT_HIGHPROC 13
#define	RPCBVERS_STAT 3
#define	RPCBVERS_4_STAT 2
#define	RPCBVERS_3_STAT 1
#define	RPCBVERS_2_STAT 0

/* Link list of all the stats about getport and getaddr */

struct rpcbs_addrlist {
	rpcprog_t prog;
	rpcvers_t vers;
	int success;
	int failure;
	char *netid;
	struct rpcbs_addrlist *next;
};
typedef struct rpcbs_addrlist rpcbs_addrlist;

/* Link list of all the stats about rmtcall */

struct rpcbs_rmtcalllist {
	rpcprog_t prog;
	rpcvers_t vers;
	rpcproc_t proc;
	int success;
	int failure;
	int indirect;
	char *netid;
	struct rpcbs_rmtcalllist *next;
};
typedef struct rpcbs_rmtcalllist rpcbs_rmtcalllist;

typedef int rpcbs_proc[RPCBSTAT_HIGHPROC];

typedef rpcbs_addrlist *rpcbs_addrlist_ptr;

typedef rpcbs_rmtcalllist *rpcbs_rmtcalllist_ptr;

struct rpcb_stat {
	rpcbs_proc info;
	int setinfo;
	int unsetinfo;
	rpcbs_addrlist_ptr addrinfo;
	rpcbs_rmtcalllist_ptr rmtinfo;
};
typedef struct rpcb_stat rpcb_stat;

/*
 * One rpcb_stat structure is returned for each version of rpcbind
 * being monitored.
 */

typedef rpcb_stat rpcb_stat_byvers[RPCBVERS_STAT];

/*
 * We don't define netbuf in RPCL, since it would contain structure member
 * names that would conflict with the definition of struct netbuf in
 * <tiuser.h>.  Instead we merely declare the XDR routine xdr_netbuf() here,
 * and implement it ourselves in rpc/rpcb_prot.c.
 */
#ifdef __cplusplus
extern "C" bool_t xdr_netbuf(XDR *, struct netbuf *);

#else /* __STDC__ */
extern bool_t xdr_netbuf(XDR *, struct netbuf *);

#endif

#define RPCBVERS_3 RPCBVERS
#define RPCBVERS_4 RPCBVERS4

#else /* ndef _KERNEL */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * A mapping of (program, version, network ID) to address
 */
struct rpcb {
 rpcprog_t r_prog; /* program number */
 rpcvers_t r_vers; /* version number */
 char *r_netid; /* network id */
 char *r_addr; /* universal address */
 char *r_owner; /* owner of the mapping */
};
typedef struct rpcb RPCB;

/*
 * A list of mappings
 */
struct rpcblist {
 RPCB rpcb_map;
 struct rpcblist *rpcb_next;
};
typedef struct rpcblist RPCBLIST;
typedef struct rpcblist *rpcblist_ptr;

/*
 * Remote calls arguments
 */
struct rpcb_rmtcallargs {
 rpcprog_t prog; /* program number */
 rpcvers_t vers; /* version number */
 rpcproc_t proc; /* procedure number */
 uint32_t arglen; /* arg len */
 caddr_t args_ptr; /* argument */
 xdrproc_t xdr_args; /* XDR routine for argument */
};
typedef struct rpcb_rmtcallargs rpcb_rmtcallargs;

/*
 * Remote calls results
 */
struct rpcb_rmtcallres {
 char *addr_ptr; /* remote universal address */
 uint32_t resultslen; /* results length */
 caddr_t results_ptr; /* results */
 xdrproc_t xdr_results; /* XDR routine for result */
};
typedef struct rpcb_rmtcallres rpcb_rmtcallres;

struct rpcb_entry {
 char *r_maddr;
 char *r_nc_netid;
 unsigned int r_nc_semantics;
 char *r_nc_protofmly;
 char *r_nc_proto;
};
typedef struct rpcb_entry rpcb_entry;

/*
 * A list of addresses supported by a service.
 */

struct rpcb_entry_list {
 rpcb_entry rpcb_entry_map;
 struct rpcb_entry_list *rpcb_entry_next;
};
typedef struct rpcb_entry_list rpcb_entry_list;

typedef rpcb_entry_list *rpcb_entry_list_ptr;

/*
 * rpcbind statistics
 */

#define rpcb_highproc_2 RPCBPROC_CALLIT
#define rpcb_highproc_3 RPCBPROC_TADDR2UADDR
#define rpcb_highproc_4 RPCBPROC_GETSTAT
#define RPCBSTAT_HIGHPROC 13
#define RPCBVERS_STAT 3
#define RPCBVERS_4_STAT 2
#define RPCBVERS_3_STAT 1
#define RPCBVERS_2_STAT 0

/* Link list of all the stats about getport and getaddr */

struct rpcbs_addrlist {
 rpcprog_t prog;
 rpcvers_t vers;
 int success;
 int failure;
 char *netid;
 struct rpcbs_addrlist *next;
};
typedef struct rpcbs_addrlist rpcbs_addrlist;

/* Link list of all the stats about rmtcall */

struct rpcbs_rmtcalllist {
 rpcprog_t prog;
 rpcvers_t vers;
 rpcproc_t proc;
 int success;
 int failure;
 int indirect;
 char *netid;
 struct rpcbs_rmtcalllist *next;
};
typedef struct rpcbs_rmtcalllist rpcbs_rmtcalllist;

typedef int rpcbs_proc[RPCBSTAT_HIGHPROC];

typedef rpcbs_addrlist *rpcbs_addrlist_ptr;

typedef rpcbs_rmtcalllist *rpcbs_rmtcalllist_ptr;

struct rpcb_stat {
 rpcbs_proc info;
 int setinfo;
 int unsetinfo;
 rpcbs_addrlist_ptr addrinfo;
 rpcbs_rmtcalllist_ptr rmtinfo;
};
typedef struct rpcb_stat rpcb_stat;

/*
 * One rpcb_stat structure is returned for each version of rpcbind
 * being monitored.
 */

typedef rpcb_stat rpcb_stat_byvers[RPCBVERS_STAT];

#ifdef __cplusplus
}
#endif

#endif /* ndef _KERNEL */

#define _PATH_RPCBINDSOCK "/var/run/rpcbind.sock"

#define	RPCBPROG ((unsigned long)(100000))
#define	RPCBVERS ((unsigned long)(3))

extern  void rpcbprog_3(struct svc_req *rqstp, SVCXPRT *transp);
#define	RPCBPROC_SET ((unsigned long)(1))
extern  bool_t * rpcbproc_set_3(RPCB *, CLIENT *);
extern  bool_t * rpcbproc_set_3_svc(RPCB *, struct svc_req *);
#define	RPCBPROC_UNSET ((unsigned long)(2))
extern  bool_t * rpcbproc_unset_3(RPCB *, CLIENT *);
extern  bool_t * rpcbproc_unset_3_svc(RPCB *, struct svc_req *);
#define	RPCBPROC_GETADDR ((unsigned long)(3))
extern  char ** rpcbproc_getaddr_3(RPCB *, CLIENT *);
extern  char ** rpcbproc_getaddr_3_svc(RPCB *, struct svc_req *);
#define	RPCBPROC_DUMP ((unsigned long)(4))
extern  rpcblist_ptr * rpcbproc_dump_3(void *, CLIENT *);
extern  rpcblist_ptr * rpcbproc_dump_3_svc(void *, struct svc_req *);
#define	RPCBPROC_CALLIT ((unsigned long)(5))
extern  rpcb_rmtcallres * rpcbproc_callit_3(rpcb_rmtcallargs *, CLIENT *);
extern  rpcb_rmtcallres * rpcbproc_callit_3_svc(rpcb_rmtcallargs *, struct svc_req *);
#define	RPCBPROC_GETTIME ((unsigned long)(6))
extern  u_int * rpcbproc_gettime_3(void *, CLIENT *);
extern  u_int * rpcbproc_gettime_3_svc(void *, struct svc_req *);
#define	RPCBPROC_UADDR2TADDR ((unsigned long)(7))
extern  struct netbuf * rpcbproc_uaddr2taddr_3(char **, CLIENT *);
extern  struct netbuf * rpcbproc_uaddr2taddr_3_svc(char **, struct svc_req *);
#define	RPCBPROC_TADDR2UADDR ((unsigned long)(8))
extern  char ** rpcbproc_taddr2uaddr_3(struct netbuf *, CLIENT *);
extern  char ** rpcbproc_taddr2uaddr_3_svc(struct netbuf *, struct svc_req *);
extern int rpcbprog_3_freeresult(SVCXPRT *, xdrproc_t, caddr_t);
#define	RPCBVERS4 ((unsigned long)(4))

extern  void rpcbprog_4(struct svc_req *rqstp, SVCXPRT *transp);
extern  bool_t * rpcbproc_set_4(RPCB *, CLIENT *);
extern  bool_t * rpcbproc_set_4_svc(RPCB *, struct svc_req *);
extern  bool_t * rpcbproc_unset_4(RPCB *, CLIENT *);
extern  bool_t * rpcbproc_unset_4_svc(RPCB *, struct svc_req *);
extern  char ** rpcbproc_getaddr_4(RPCB *, CLIENT *);
extern  char ** rpcbproc_getaddr_4_svc(RPCB *, struct svc_req *);
extern  rpcblist_ptr * rpcbproc_dump_4(void *, CLIENT *);
extern  rpcblist_ptr * rpcbproc_dump_4_svc(void *, struct svc_req *);
#define	RPCBPROC_BCAST ((unsigned long)(RPCBPROC_CALLIT))
extern  rpcb_rmtcallres * rpcbproc_bcast_4(rpcb_rmtcallargs *, CLIENT *);
extern  rpcb_rmtcallres * rpcbproc_bcast_4_svc(rpcb_rmtcallargs *, struct svc_req *);
extern  u_int * rpcbproc_gettime_4(void *, CLIENT *);
extern  u_int * rpcbproc_gettime_4_svc(void *, struct svc_req *);
extern  struct netbuf * rpcbproc_uaddr2taddr_4(char **, CLIENT *);
extern  struct netbuf * rpcbproc_uaddr2taddr_4_svc(char **, struct svc_req *);
extern  char ** rpcbproc_taddr2uaddr_4(struct netbuf *, CLIENT *);
extern  char ** rpcbproc_taddr2uaddr_4_svc(struct netbuf *, struct svc_req *);
#define	RPCBPROC_GETVERSADDR ((unsigned long)(9))
extern  char ** rpcbproc_getversaddr_4(RPCB *, CLIENT *);
extern  char ** rpcbproc_getversaddr_4_svc(RPCB *, struct svc_req *);
#define	RPCBPROC_INDIRECT ((unsigned long)(10))
extern  rpcb_rmtcallres * rpcbproc_indirect_4(rpcb_rmtcallargs *, CLIENT *);
extern  rpcb_rmtcallres * rpcbproc_indirect_4_svc(rpcb_rmtcallargs *, struct svc_req *);
#define	RPCBPROC_GETADDRLIST ((unsigned long)(11))
extern  rpcb_entry_list_ptr * rpcbproc_getaddrlist_4(RPCB *, CLIENT *);
extern  rpcb_entry_list_ptr * rpcbproc_getaddrlist_4_svc(RPCB *, struct svc_req *);
#define	RPCBPROC_GETSTAT ((unsigned long)(12))
extern  rpcb_stat * rpcbproc_getstat_4(void *, CLIENT *);
extern  rpcb_stat * rpcbproc_getstat_4_svc(void *, struct svc_req *);
extern int rpcbprog_4_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

/* the xdr functions */
extern  bool_t xdr_rpcb(XDR *, RPCB *);
#ifndef _KERNEL
extern  bool_t xdr_rp__list(XDR *, rp__list*);
#endif
extern  bool_t xdr_rpcblist_ptr(XDR *, rpcblist_ptr*);
extern  bool_t xdr_rpcb_rmtcallargs(XDR *, rpcb_rmtcallargs*);
extern  bool_t xdr_rpcb_rmtcallres(XDR *, rpcb_rmtcallres*);
extern  bool_t xdr_rpcb_entry(XDR *, rpcb_entry*);
extern  bool_t xdr_rpcb_entry_list(XDR *, rpcb_entry_list*);
extern  bool_t xdr_rpcb_entry_list_ptr(XDR *, rpcb_entry_list_ptr*);
extern  bool_t xdr_rpcbs_addrlist(XDR *, rpcbs_addrlist*);
extern  bool_t xdr_rpcbs_rmtcalllist(XDR *, rpcbs_rmtcalllist*);
extern  bool_t xdr_rpcbs_proc(XDR *, rpcbs_proc);
extern  bool_t xdr_rpcbs_addrlist_ptr(XDR *, rpcbs_addrlist_ptr*);
extern  bool_t xdr_rpcbs_rmtcalllist_ptr(XDR *, rpcbs_rmtcalllist_ptr*);
extern  bool_t xdr_rpcb_stat(XDR *, rpcb_stat*);
extern  bool_t xdr_rpcb_stat_byvers(XDR *, rpcb_stat_byvers);

#ifdef __cplusplus
}
#endif

#endif /* !_RPCB_PROT_H_RPCGEN */
