/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		que.h
 *	DESCRIPTION:	Que manipulation macros
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef JRD_QUE_H
#define JRD_QUE_H

//
// general purpose queue
//
namespace Jrd {

struct que
{
	struct que* que_forward;
	struct que* que_backward;
};

typedef que* QUE;


inline void QUE_INIT(que& aque)
{
	aque.que_backward = &aque;
	aque.que_forward = &aque;
}


inline void QUE_DELETE(que& node)
{
	node.que_backward->que_forward = node.que_forward;
	node.que_forward->que_backward = node.que_backward;
}

/* QUE_INSERT adds node to queue head. */

inline void QUE_INSERT(que& aque, que& node)
{
	node.que_forward = aque.que_forward;
	node.que_backward = &aque;
	aque.que_forward->que_backward = &node;
	aque.que_forward = &node;
}

/* QUE_APPEND adds node to queue tail. */

inline void QUE_APPEND(que& aque, que& node)
{
	node.que_forward = &aque;
	node.que_backward = aque.que_backward;
	aque.que_backward->que_forward = &node;
	aque.que_backward = &node;
}

inline bool QUE_NOT_EMPTY(const que& aque)
{
	return aque.que_forward != &aque;
}

inline bool QUE_EMPTY(const que& aque)
{
	return aque.que_forward == &aque;
}

} // namespace Jrd


/* QUE_LOOP to visit every node. */

#define QUE_LOOP(que, node) {\
	for (node = (que).que_forward; node != &(que); node = (node)->que_forward)

#define QUE_LOOPA(que, node) {\
	for (node = (que)->que_forward; node != que; node = (node)->que_forward)

// CVC: Not sure how to make these two inline since cch.h includes this file but
// we would need to include here cch.h to get the BufferControl definition.

#define QUE_MOST_RECENTLY_USED(in_use_que) {\
	QUE_DELETE (in_use_que);\
	QUE_INSERT (bcb->bcb_in_use, in_use_que);}

#define QUE_LEAST_RECENTLY_USED(in_use_que) {\
	QUE_DELETE (in_use_que);\
	QUE_APPEND (bcb->bcb_in_use, in_use_que);}


// Self-relative queue BASE should be defined in the source which includes this
#define SRQ_PTR SLONG

#ifndef SRQ_REL_PTR
#define SRQ_REL_PTR(item)           (SRQ_PTR) ((UCHAR*) item - SRQ_BASE)
#endif
#ifndef SRQ_ABS_PTR
#define SRQ_ABS_PTR(item)           (SRQ_BASE + item)
#endif

// CVC: I cannot convert these four to inline functions because SRQ_BASE
// is defined in event.cpp!

#define SRQ_INIT(que)   {que.srq_forward = que.srq_backward = SRQ_REL_PTR (&que);}
#define SRQ_EMPTY(que)  (que.srq_forward == SRQ_REL_PTR (&que))
#define SRQ_NEXT(que)   (srq*) SRQ_ABS_PTR (que.srq_forward)
#define SRQ_PREV(que)	(srq*) SRQ_ABS_PTR (que.srq_backward)

#define SRQ_LOOP(header, que)    for (que = SRQ_NEXT (header);\
	que != &header; que = SRQ_NEXT ((*que)))
#define SRQ_LOOP_BACK(header, que)	for (que = SRQ_PREV (header);\
	que != &header; que = SRQ_PREV ((*que)))

/* Self-relative que block.  Offsets are from the block itself. */

struct srq
{
	SRQ_PTR srq_forward;			/* Forward offset */
	SRQ_PTR srq_backward;			/* Backward offset */
};

typedef srq *SRQ;


#endif /* JRD_QUE_H */
