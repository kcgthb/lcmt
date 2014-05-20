/*****************************************************************************
 *  Copyright (C) 2010 Lawrence Livermore National Security, LLC.
 *  This module written by Jim Garlick <garlick@llnl.gov>
 *  UCRL-CODE-232438
 *  All Rights Reserved.
 *
 *  This file is part of Lustre Monitoring Tool, version 2.
 *  Authors: H. Wartens, P. Spencer, N. O'Neill, J. Long, J. Garlick
 *  For details, see http://github.com/chaos/lmt.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License (as published by the
 *  Free Software Foundation) version 2, dated June 1991.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA or see
 *  <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#if HAVE_CEREBRO_H
#include <cerebro.h>
#endif
#ifndef CEREBRO_MAX_DATA_STRING_LEN
#define CEREBRO_MAX_DATA_STRING_LEN (63*1024)
#endif

#include "list.h"
#include "hash.h"
#include "error.h"

#include "proc.h"
#include "stat.h"
#include "meminfo.h"

#include "client.h"

#define OPTIONS "m:r:t:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long (ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"metric",          required_argument,  0, 'm'},
    {"proc-root",       required_argument,  0, 'r'},
    {"update-period",   required_argument,  0, 't'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt (ac,av,opt)
#endif

static int _sysstat (pctx_t ctx, char *buf, int len);

static void
usage()
{
    fprintf (stderr,
"Usage: lmtmetric [OPTIONS]\n"
"   -m,--metric NAME            select client|sysstat\n"
"   -r,--proc-root DIR          select proc root other than /proc\n"
"   -t,--update-period SECS     [default: run once]\n"
    );
    exit (1);
}

int
main (int argc, char *argv[])
{
    pctx_t ctx;
    char buf[CEREBRO_MAX_DATA_STRING_LEN];
    char *proc_root = "/proc";
    char *metric = NULL;
    unsigned long update_period = 0;
    int c, n = 0;

    err_init (argv[0]);

    optind = 0;
    opterr = 0;
    while ((c = GETOPT (argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 'm':   /* --metric NAME */
                metric = optarg;
                break;
            case 'r':   /* --proc-root DIR */
                proc_root = optarg;
                break;
            case 't':   /* --update-period SECS */
                update_period = strtoul (optarg, NULL, 10);
                break;
            default:
                usage ();
        }
    }
    if (optind < argc)
        usage();
    if (metric && strcmp (metric, "client") && strcmp (metric, "sysstat"))
        usage();
    if (!metric)
        metric = "sysstat";

    if (!(ctx = proc_create (proc_root)))
        err_exit ("proc_create");

    do {
        errno = 0; 
        if (!strcmp (metric, "sysstat"))
            n = _sysstat (ctx, buf, sizeof (buf));
        else if (!strcmp (metric, "client"))
            n = lmt_client_string (ctx, buf, sizeof (buf));
        if (n < 0 && errno == 0)
            msg ("%s metric information unavailable", metric);
        else if (n < 0)
            err ("%s metric", metric);
        else
            printf ("%s: %s\n", metric, buf);
        if (update_period > 0)
            sleep (update_period);
    } while (update_period > 0);

    proc_destroy (ctx);
    exit (0);
}

static int
_sysstat (pctx_t ctx, char *buf, int len)
{
    static uint64_t cpuusage = 0, cputot = 0;
    double cpupct, mempct;
    uint64_t ktot, kfree;
    int ret = -1;

    if (proc_stat2 (ctx, &cpuusage, &cputot, &cpupct) < 0)
        goto done;
    if (proc_meminfo (ctx, &ktot, &kfree) < 0)
        goto done;
    mempct = ((double)(ktot - kfree) / (double)(ktot)) * 100.0;
    snprintf (buf, len, "cpu_util: %.2f%% mem_util: %.2f%%", cpupct, mempct);
    ret = 0;
done:
    return ret;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

