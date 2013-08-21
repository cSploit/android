/*
 *  $Id: libnet_port_list.c,v 1.10 2004/01/28 19:45:00 mike Exp $
 *
 *  libnet
 *  libnet_port_list.c - transport layer port list chaining code
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
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
 */

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#if (!(_WIN32) || (__CYGWIN__)) 
#include "../include/libnet.h"
#else
#include "../include/win32/libnet.h"
#endif

uint16_t *all_lists;

int
libnet_plist_chain_new(libnet_t *l, libnet_plist_t **plist, char *token_list)
{
    char libnet_plist_legal_tokens[] = "0123456789,- ";
    libnet_plist_t *tmp;
    char *tok;
    int i, j, valid_token, cur_node;
    uint16_t *all_lists_tmp;
    static uint8_t cur_id;

    if (l == NULL)
    { 
        return (-1);
    } 

    if (token_list == NULL)
    {
        return (-1);
    }

    /*
     *  Make sure we have legal tokens.
     */
    for (i = 0; token_list[i]; i++)
    {
        for (j = 0, valid_token = 0; libnet_plist_legal_tokens[j]; j++)
        {
            if (libnet_plist_legal_tokens[j] == token_list[i])
            {
                valid_token = 1;
                break;
            }
        }
        if (!valid_token)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_build_plist_chain: illegal token # %d (%c)\n",
                    i + 1,
                    token_list[i]);
            *plist = NULL;
            return (-1);
        }
    }

    /* head node */
    *plist = malloc(sizeof (libnet_plist_t));

    if (!(*plist))
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_build_plist_chain: malloc %s\n", strerror(errno));
        *plist = NULL;
        return (-1);
    }

    tmp = *plist;
    tmp->node = cur_node = 0;
    tmp->next = NULL;
    tmp->id = cur_id;
    all_lists_tmp = all_lists;
    all_lists = realloc(all_lists_tmp, (sizeof(uint16_t) * (cur_id + 1)));
    if (!all_lists)
    {
        all_lists = all_lists_tmp;
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_build_plist_chain: realloc %s\n", strerror(errno));
        *plist = NULL;
        return(-1);
    }

    all_lists[cur_id++] = 0;

    /*
     *  Using strtok successively proved problematic.  We solve this by
     *  calling it once, then manually extracting the elements from the token.
     *  In the case of bport > eport, we swap them.
     */
    for (i = 0; (tok = strtok(!i ? token_list : NULL, ",")); i = 1, cur_node++)
    {
        /*
         *  The first iteration we will have a head node allocated so we don't
         *  need to malloc().
         */
        if (i)
        {
            tmp->next = malloc(sizeof (libnet_plist_t));
            if (!tmp)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_build_plist_chain: malloc %s\n", strerror(errno));
                /*
                 *  XXX - potential memory leak if other nodes are allocated
                 *  but not freed.
                 */
                *plist = NULL;
                return(-1);
            }
            tmp = tmp->next;
            tmp->node = cur_node;
            tmp->next = NULL;
        }
        tmp->bport = atoi(tok);

        /*
         *  Step past this port number.
         */
        j = 0;
        while (isdigit((int)tok[j]))
        {
            j++;
        }

        /*
         *  If we have a delimiting dash and are NOT at the end of the token
         *  array, we can assume it's the end port, otherwise if we just have
         *  a dash, we consider it int16_thand for `inclusive of all ports up to
         *  65535.  Finally, if we have no dash, we assume this token is a
         *  single port only.
         */
        if (tok[j] == '-')
        {
            tmp->eport = (++j != strlen(tok)) ? atoi(&tok[j]) : 65535;
        }
        else
        {
            tmp->eport = tmp->bport;
        }

        /*
         *  Do we need to swap the values?
         */
        if (tmp->bport > tmp->eport)
        {
            tmp->bport ^= tmp->eport;
            tmp->eport ^= tmp->bport;
            tmp->bport ^= tmp->eport;
        }
    }

    /*
     *  The head node needs to hold the total node count.
     */
    (*plist)->node = cur_node;
    return (1);
}

int
libnet_plist_chain_next_pair(libnet_plist_t *plist, uint16_t *bport,
        uint16_t *eport)
{
    uint16_t *node_cnt;
    uint16_t tmp_cnt;

    node_cnt = &(all_lists[plist->id]);
    if (plist == NULL)
    {
        return (-1);
    }

    /*
     *  We are at the end of the list.
     */
    if (*node_cnt == plist->node)
    {
        *node_cnt = 0;
        *bport = 0;
        *eport = 0;
        return (0);
    }

    for (tmp_cnt = *node_cnt; tmp_cnt; tmp_cnt--, plist = plist->next) ;
    *bport = plist->bport;
    *eport = plist->eport;
    *node_cnt += 1;
    return (1);
}

int
libnet_plist_chain_dump(libnet_plist_t *plist)
{
    if (plist == NULL)
    {
        return (-1);
    }

    for (; plist; plist = plist->next)
    {
        if (plist->bport == plist->eport)
        {
            fprintf(stdout, "%d ", plist->bport);
        }
        else
        {
            fprintf(stdout, "%d-%d ", plist->bport, plist->eport);
        }
    }
    fprintf(stdout, "\n");
    return (1);
}

char *
libnet_plist_chain_dump_string(libnet_plist_t *plist)
{
    char buf[BUFSIZ] = {0};
    int i, j;

    if (plist == NULL)
    {
        return (NULL);
    }

    for (i = 0, j = 0; plist; plist = plist->next)
    {
        if (plist->bport == plist->eport)
        {
            i = snprintf(&buf[j], BUFSIZ, "%d", plist->bport);
        }
        else
        {
            i = snprintf(&buf[j], BUFSIZ, "%d-%d", plist->bport, plist->eport);
        }
        j += i;
        if (plist->next)
        {
            snprintf(&buf[j++], BUFSIZ, ",");
        }
    }
    return (strdup(buf));       /* XXX - reentrancy == no */
}

int
libnet_plist_chain_free(libnet_plist_t *plist)
{
    uint16_t i;
    libnet_plist_t *tmp;

    if (plist == NULL)
    {
        return (-1);
    }

    for (i = plist->node; i; i--)
    {
        tmp = plist;
        plist = plist->next;
        free(tmp);
    }
    plist = NULL;
    return (1);
}

/* EOF */
