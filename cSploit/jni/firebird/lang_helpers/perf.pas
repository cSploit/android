(*******************************************************************)
(*                                                                 *)
(* The contents of this file are subject to the Interbase Public   *)
(* License Version 1.0 (the "License"); you may not use this file  *)
(* except in compliance with the License. You may obtain a copy    *)
(* of the License at http://www.Inprise.com/IPL.html                 *)
(*                                                                 *)
(* Software distributed under the License is distributed on an     *)
(* "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express     *)
(* or implied. See the License for the specific language governing *)
(* rights and limitations under the License.                       *)
(*                                                                 *)
(* The Original Code was created by Inprise Corporation *)
(* and its predecessors. Portions created by Inprise Corporation are     *)
(* Copyright (C) Inprise Corporation. *)
(*                                                                 *)
(* All Rights Reserved.                                            *)
(* Contributor(s): ______________________________________.         *)
(*******************************************************************)

(*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		perf.pas
 *	DESCRIPTION:	Peformance tool block
 *)

type
    tms = record
	tms_utime		: integer32;
	tms_stime		: integer32;
	tms_cutime		: integer32;
	tms_cstime		: integer32;
    end;
    perf = record
	perf_fetches		: integer32;
	perf_marks		: integer32;
	perf_reads		: integer32;
	perf_writes		: integer32;
	perf_current_memory	: integer32;
	perf_max_memory		: integer32;
	perf_buffers		: integer32;
	perf_page_size		: integer32;
	perf_elapsed		: integer32;
	perf_times		: tms;
    end;

(* Letter codes controlling printting of statistics:

	!f - fetches
	!m - marks
	!r - reads
	!w - writes
	!e - elapsed time (in seconds)
	!u - user times
	!s - system time
	!p - page size
	!b - number buffers
	!d - delta memory
	!c - current memory
	!x - max memory

*)

(* Entry point definitions *)

procedure perf_get_info (
	in 	handle	: gds__handle; 
	out	block	: perf
	); extern;

procedure perf_format (
	in	before	: perf;
	in	after	: perf;
	in	control	: UNIV string;
	in{out}	buffer	: UNIV string;
	in	buf_len	: integer
	); extern;

procedure perf_report (
	in	before	: perf;
	in	after	: perf;
	in{out}	buffer	: UNIV string;
	in	buf_len	: integer
	); extern;



