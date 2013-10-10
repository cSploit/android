/*
    ettercap -- global variables handling module

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ec_globals.c,v 1.13 2004/05/12 15:27:05 alor Exp $
*/

#include <ec.h>
#include <ec_sniff.h>

#define GBL_FREE(x) do{ if (x != NULL) { free(x); x = NULL; } }while(0)


/* global vars */

struct globals *gbls;

/* proto */

void globals_alloc(void);
void globals_free(void);

/*******************************************/

void globals_alloc(void)
{
   
   SAFE_CALLOC(gbls, 1, sizeof(struct globals));
   SAFE_CALLOC(gbls->conf, 1, sizeof(struct ec_conf)); 
   SAFE_CALLOC(gbls->options, 1, sizeof(struct ec_options));         
   SAFE_CALLOC(gbls->stats, 1, sizeof(struct gbl_stats));
   SAFE_CALLOC(gbls->ui, 1, sizeof(struct ui_ops));
   SAFE_CALLOC(gbls->env, 1, sizeof(struct program_env)); 
   SAFE_CALLOC(gbls->pcap, 1, sizeof(struct pcap_env));
   SAFE_CALLOC(gbls->lnet, 1, sizeof(struct lnet_env)); 
   SAFE_CALLOC(gbls->iface, 1, sizeof(struct iface_env));
   SAFE_CALLOC(gbls->bridge, 1, sizeof(struct iface_env));
   SAFE_CALLOC(gbls->sm, 1, sizeof(struct sniffing_method));
   SAFE_CALLOC(gbls->t1, 1, sizeof(struct target_env));
   SAFE_CALLOC(gbls->t2, 1, sizeof(struct target_env));
   SAFE_CALLOC(gbls->filters, 1, sizeof(struct filter_env));

   /* init the struct */
   TAILQ_INIT(&GBL_PROFILES);
   
   return;
}


void globals_free(void)
{
 
   GBL_FREE(gbls->pcap);
   GBL_FREE(gbls->lnet);
   GBL_FREE(gbls->iface);
   GBL_FREE(gbls->bridge);
   GBL_FREE(gbls->sm);
   GBL_FREE(gbls->filters);

   free_ip_list(gbls->t1);
   GBL_FREE(gbls->t1);
   free_ip_list(gbls->t2);
   GBL_FREE(gbls->t2);
   
   GBL_FREE(gbls->env->name);
   GBL_FREE(gbls->env->version);
   GBL_FREE(gbls->env->debug_file);
   GBL_FREE(gbls->env);
   
   GBL_FREE(gbls->options->plugin);
   GBL_FREE(gbls->options->proto);
   GBL_FREE(gbls->options->pcapfile_in);
   GBL_FREE(gbls->options->pcapfile_out);
   GBL_FREE(gbls->options->iface);
   GBL_FREE(gbls->options->iface_bridge);
   GBL_FREE(gbls->options->target1);
   GBL_FREE(gbls->options->target2);
   GBL_FREE(gbls->stats);
   GBL_FREE(gbls->options);
   GBL_FREE(gbls->conf);
   
   GBL_FREE(gbls);
   
   return;
}


/* EOF */

// vim:ts=3:expandtab

