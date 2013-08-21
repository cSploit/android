/*
 *  $Id: libnet_cq.c,v 1.11 2004/01/28 19:45:00 mike Exp $
 *
 *  libnet
 *  libnet_cq.c - context queue management routines
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  Copyright (c) 2002 Frédéric Raynal <pappy@security-labs.org>
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

/* private function prototypes */
static libnet_cq_t *libnet_cq_find_internal(libnet_t *);
static int libnet_cq_dup_check(libnet_t *, char *);
static libnet_cq_t *libnet_cq_find_by_label_internal(char *label);

/* global context queue */
static libnet_cq_t *l_cq = NULL;
static libnet_cqd_t l_cqd = {0, CQ_LOCK_UNLOCKED, NULL};


static int
set_cq_lock(uint x) 
{
    if (check_cq_lock(x))
    {
        return (0);
    }

    l_cqd.cq_lock |= x;
    return (1);
}

static int
clear_cq_lock(uint x) 
{
    if (!check_cq_lock(x))
    {
        return (0);
    }

    l_cqd.cq_lock &= ~x;
    return (1);
}

int 
libnet_cq_add(libnet_t *l, char *label)
{
    libnet_cq_t *new;

    if (l == NULL) 
    {
        return (-1);
    }

    /* check for write lock on the context queue */
    if (cq_is_wlocked()) 
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't add, context queue is write locked\n", __func__);
        return (-1);
    }
  
    /* ensure there is a label */
    if (label == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s(): empty label\n",
                __func__);
        return (-1);
    }

    /* check to see if we're the first node on the list */
    if (l_cq == NULL)
    {
        l_cq = (libnet_cq_t *)malloc(sizeof (libnet_cq_t));
        if (l_cq == NULL)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): can't malloc initial context queue: %s\n",
                    __func__, strerror(errno));
            return (-1);
        }

        l_cq->context = l;

        /* label the context with the user specified string */
        strncpy(l->label, label, LIBNET_LABEL_SIZE);
        l->label[LIBNET_LABEL_SIZE] = '\0';

        l_cq->next = NULL;
        l_cq->prev = NULL;

        /* track the number of nodes in the context queue */
        l_cqd.node = 1;

        return (1);
    }

    /* check to see if the cq we're about to add is already in the list */
    if (libnet_cq_dup_check(l, label)) 
    {
        /* error message set in libnet_cq_dup_check() */
        return (-1);
    }

    new = (libnet_cq_t *)malloc(sizeof (libnet_cq_t));
    if (l_cq == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't malloc new context queue: %s\n",
                __func__, strerror(errno));
        return (-1);
    }

    new->context = l;

    /* label the context with the user specified string */
    strncpy(l->label, label, LIBNET_LABEL_SIZE);
    l->label[LIBNET_LABEL_SIZE] = '\0';

    new->next = l_cq;
    new->prev = NULL;

    l_cq->prev = new;
    l_cq = new;

    /* track the number of nodes in the context queue */
    l_cqd.node++;

    return (1); 
}

libnet_t *
libnet_cq_remove(libnet_t *l) 
{
    libnet_cq_t *p;
    libnet_t *ret;

    if (l_cq == NULL) 
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't remove from empty context queue\n", __func__);
        return (NULL);
    }

    if (l == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't remove empty libnet context\n", __func__);
        return(NULL);
    }

    /* check for write lock on the cq */
    if (cq_is_wlocked()) 
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't remove, context queue is write locked\n",
                __func__);
        return (NULL);
    }
  
    if ((p = libnet_cq_find_internal(l)) == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): context not present in context queue\n", __func__);
        return (NULL);
    }

    if (p->prev) 
    {
        p->prev->next = p->next;
    }
    else
    {
        l_cq = p->next;
    }
    if (p->next)
    {
        p->next->prev = p->prev;
    }

    ret = p->context;
    free(p);

    /* track the number of nodes in the cq */
    l_cqd.node--;
  
    return (ret);
}

libnet_t *
libnet_cq_remove_by_label(char *label) 
{
    libnet_cq_t *p;
    libnet_t *ret;

    if ((p = libnet_cq_find_by_label_internal(label)) == NULL)
    {
        /* no context to write an error message */
        return (NULL);
    }

    if (cq_is_wlocked()) 
    {
        /* now we have a context, but the user can't see it */
        return (NULL);
    }

    if (p->prev) 
    {
        p->prev->next = p->next;
    }
    else
    {
        l_cq = p->next;
    }
    if (p->next)
    {
        p->next->prev = p->prev;
    }

    ret = p->context;
    free(p);

    /* track the number of nodes in the cq */
    l_cqd.node--;
  
    return (ret);
}

libnet_cq_t *
libnet_cq_find_internal(libnet_t *l) 
{
    libnet_cq_t *p;

    for (p = l_cq; p; p = p->next)
    {
        if (p->context == l)
        {
            return (p);
        }
    }
    return (NULL);
}

int
libnet_cq_dup_check(libnet_t *l, char *label)
{
    libnet_cq_t *p;

    for (p = l_cq; p; p = p->next)
    {
        if (p->context == l)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): context already in context queue\n", __func__);
            return (1);
        }
        if (strncmp(p->context->label, label, LIBNET_LABEL_SIZE) == 0)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): duplicate label %s\n", __func__, label);
            return (1);
        }
    }
    /* no duplicate */
    return (0);
}

libnet_cq_t *
libnet_cq_find_by_label_internal(char *label) 
{
    libnet_cq_t *p;
  
    if (label == NULL)
    {
        return (NULL);
    }

    for (p = l_cq; p; p = p->next)
    {
        if (!strncmp(p->context->label, label, LIBNET_LABEL_SIZE))
        {
            return (p);
        }
    }
    return (NULL);
}

libnet_t *
libnet_cq_find_by_label(char *label)
{
    libnet_cq_t *p;
  
    p = libnet_cq_find_by_label_internal(label);
    return (p ? p->context : NULL);
}

const char *
libnet_cq_getlabel(libnet_t *l)
{
    return (l->label);
}

void
libnet_cq_destroy() 
{
    libnet_cq_t *p = l_cq;
    libnet_cq_t *tmp;

    while (p)
    {
        tmp = p;
        p = p->next;
        libnet_destroy(tmp->context);
        free(tmp);
    }
}

libnet_t *
libnet_cq_head()
{
    if (l_cq == NULL) 
    {
        return (NULL);
    }

    if (!set_cq_lock(CQ_LOCK_WRITE)) 
    {
        return (NULL);
    }

    l_cqd.current = l_cq;
    return (l_cqd.current->context);
}

int
libnet_cq_last()
{
    if (l_cqd.current)
    {
        return (1);
    }
    else
    {
        return (0);
    }
}

libnet_t *
libnet_cq_next()
{
    if (l_cqd.current == NULL)
    {
        return (NULL);
    }

    l_cqd.current = l_cqd.current->next;
    return (l_cqd.current ? l_cqd.current->context : NULL);
}

uint32_t
libnet_cq_size()
{
    return (l_cqd.node);
}

uint32_t
libnet_cq_end_loop()
{
    if (! clear_cq_lock(CQ_LOCK_WRITE))
    {
        return (0);
    }
    l_cqd.current = l_cq;
    return (1);
}

