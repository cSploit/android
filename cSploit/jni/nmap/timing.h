
/***************************************************************************
 * timing.h -- Functions related to computing scan timing (such as keeping *
 * track of and adjusting smoothed round trip times, statistical           *
 * deviations, timeout values, etc.  Various user options (such as the     *
 * timing policy (-T)) also play a role in these calculations.             *
 *                                                                         *
 ***********************IMPORTANT NMAP LICENSE TERMS************************
 *                                                                         *
 * The Nmap Security Scanner is (C) 1996-2013 Insecure.Com LLC. Nmap is    *
 * also a registered trademark of Insecure.Com LLC.  This program is free  *
 * software; you may redistribute and/or modify it under the terms of the  *
 * GNU General Public License as published by the Free Software            *
 * Foundation; Version 2 ("GPL"), BUT ONLY WITH ALL OF THE CLARIFICATIONS  *
 * AND EXCEPTIONS DESCRIBED HEREIN.  This guarantees your right to use,    *
 * modify, and redistribute this software under certain conditions.  If    *
 * you wish to embed Nmap technology into proprietary software, we sell    *
 * alternative licenses (contact sales@nmap.com).  Dozens of software      *
 * vendors already license Nmap technology such as host discovery, port    *
 * scanning, OS detection, version detection, and the Nmap Scripting       *
 * Engine.                                                                 *
 *                                                                         *
 * Note that the GPL places important restrictions on "derivative works",  *
 * yet it does not provide a detailed definition of that term.  To avoid   *
 * misunderstandings, we interpret that term as broadly as copyright law   *
 * allows.  For example, we consider an application to constitute a        *
 * derivative work for the purpose of this license if it does any of the   *
 * following with any software or content covered by this license          *
 * ("Covered Software"):                                                   *
 *                                                                         *
 * o Integrates source code from Covered Software.                         *
 *                                                                         *
 * o Reads or includes copyrighted data files, such as Nmap's nmap-os-db   *
 * or nmap-service-probes.                                                 *
 *                                                                         *
 * o Is designed specifically to execute Covered Software and parse the    *
 * results (as opposed to typical shell or execution-menu apps, which will *
 * execute anything you tell them to).                                     *
 *                                                                         *
 * o Includes Covered Software in a proprietary executable installer.  The *
 * installers produced by InstallShield are an example of this.  Including *
 * Nmap with other software in compressed or archival form does not        *
 * trigger this provision, provided appropriate open source decompression  *
 * or de-archiving software is widely available for no charge.  For the    *
 * purposes of this license, an installer is considered to include Covered *
 * Software even if it actually retrieves a copy of Covered Software from  *
 * another source during runtime (such as by downloading it from the       *
 * Internet).                                                              *
 *                                                                         *
 * o Links (statically or dynamically) to a library which does any of the  *
 * above.                                                                  *
 *                                                                         *
 * o Executes a helper program, module, or script to do any of the above.  *
 *                                                                         *
 * This list is not exclusive, but is meant to clarify our interpretation  *
 * of derived works with some common examples.  Other people may interpret *
 * the plain GPL differently, so we consider this a special exception to   *
 * the GPL that we apply to Covered Software.  Works which meet any of     *
 * these conditions must conform to all of the terms of this license,      *
 * particularly including the GPL Section 3 requirements of providing      *
 * source code and allowing free redistribution of the work as a whole.    *
 *                                                                         *
 * As another special exception to the GPL terms, Insecure.Com LLC grants  *
 * permission to link the code of this program with any version of the     *
 * OpenSSL library which is distributed under a license identical to that  *
 * listed in the included docs/licenses/OpenSSL.txt file, and distribute   *
 * linked combinations including the two.                                  *
 *                                                                         *
 * Any redistribution of Covered Software, including any derived works,    *
 * must obey and carry forward all of the terms of this license, including *
 * obeying all GPL rules and restrictions.  For example, source code of    *
 * the whole work must be provided and free redistribution must be         *
 * allowed.  All GPL references to "this License", are to be treated as    *
 * including the terms and conditions of this license text as well.        *
 *                                                                         *
 * Because this license imposes special exceptions to the GPL, Covered     *
 * Work may not be combined (even as part of a larger work) with plain GPL *
 * software.  The terms, conditions, and exceptions of this license must   *
 * be included as well.  This license is incompatible with some other open *
 * source licenses as well.  In some cases we can relicense portions of    *
 * Nmap or grant special permissions to use it in other open source        *
 * software.  Please contact fyodor@nmap.org with any such requests.       *
 * Similarly, we don't incorporate incompatible open source software into  *
 * Covered Software without special permission from the copyright holders. *
 *                                                                         *
 * If you have any questions about the licensing restrictions on using     *
 * Nmap in other works, are happy to help.  As mentioned above, we also    *
 * offer alternative license to integrate Nmap into proprietary            *
 * applications and appliances.  These contracts have been sold to dozens  *
 * of software vendors, and generally include a perpetual license as well  *
 * as providing for priority support and updates.  They also fund the      *
 * continued development of Nmap.  Please email sales@nmap.com for further *
 * information.                                                            *
 *                                                                         *
 * If you have received a written license agreement or contract for        *
 * Covered Software stating terms other than these, you may choose to use  *
 * and redistribute Covered Software under those terms instead of these.   *
 *                                                                         *
 * Source is provided to this software because we believe users have a     *
 * right to know exactly what a program is going to do before they run it. *
 * This also allows you to audit the software for security holes (none     *
 * have been found so far).                                                *
 *                                                                         *
 * Source code also allows you to port Nmap to new platforms, fix bugs,    *
 * and add new features.  You are highly encouraged to send your changes   *
 * to the dev@nmap.org mailing list for possible incorporation into the    *
 * main distribution.  By sending these changes to Fyodor or one of the    *
 * Insecure.Org development mailing lists, or checking them into the Nmap  *
 * source code repository, it is understood (unless you specify otherwise) *
 * that you are offering the Nmap Project (Insecure.Com LLC) the           *
 * unlimited, non-exclusive right to reuse, modify, and relicense the      *
 * code.  Nmap will always be available Open Source, but this is important *
 * because the inability to relicense code has caused devastating problems *
 * for other Free Software projects (such as KDE and NASM).  We also       *
 * occasionally relicense the code to third parties as discussed above.    *
 * If you wish to specify special license conditions of your               *
 * contributions, just say so when you send them.                          *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the Nmap      *
 * license file for more details (it's in a COPYING file included with     *
 * Nmap, and also available from https://svn.nmap.org/nmap/COPYING         *
 *                                                                         *
 ***************************************************************************/

/* $Id: timing.h 32717 2014-02-12 20:25:51Z dmiller $ */

#ifndef NMAP_TIMING_H
#define NMAP_TIMING_H

#include "nmap.h"
#include "global_structures.h"

/* Call this function on a newly allocated struct timeout_info to
   initialize the values appropriately */
void initialize_timeout_info(struct timeout_info *to);

/* Same as adjust_timeouts(), except this one allows you to specify
 the receive time too (which could be because it was received a while
 back or it could be for efficiency because the caller already knows
 the current time */
void adjust_timeouts2(const struct timeval *sent,
                      const struct timeval *received,
                      struct timeout_info *to);

/* Adjust our timeout values based on the time the latest probe took for a
   response.  We update our RTT averages, etc. */
void adjust_timeouts(struct timeval sent, struct timeout_info *to);

#define DEFAULT_CURRENT_RATE_HISTORY 5.0

/* Sleeps if necessary to ensure that it isn't called twice within less
   time than o.send_delay.  If it is passed a non-null tv, the POST-SLEEP
   time is recorded in it */
void enforce_scan_delay(struct timeval *tv);

/* This class measures current and lifetime average rates for some quantity. */
class RateMeter {
  public:
    RateMeter(double current_rate_history = DEFAULT_CURRENT_RATE_HISTORY);

    void start(const struct timeval *now = NULL);
    void stop(const struct timeval *now = NULL);
    void update(double amount, const struct timeval *now = NULL);
    double getOverallRate(const struct timeval *now = NULL) const;
    double getCurrentRate(const struct timeval *now = NULL, bool update = true);
    double getTotal(void) const;
    double elapsedTime(const struct timeval *now = NULL) const;

  private:
    /* How many seconds to look back when calculating the "current" rates. */
    double current_rate_history;

    /* When this meter started recording. */
    struct timeval start_tv;
    /* When this meter stopped recording. */
    struct timeval stop_tv;
    /* The last time the current sample rates were updated. */
    struct timeval last_update_tv;

    double total;
    double current_rate;

    static bool isSet(const struct timeval *tv);
};

/* A specialization of RateMeter that measures packet and byte rates. */
class PacketRateMeter {
  public:
    PacketRateMeter(double current_rate_history = DEFAULT_CURRENT_RATE_HISTORY);

    void start(const struct timeval *now = NULL);
    void stop(const struct timeval *now = NULL);
    void update(u32 len, const struct timeval *now = NULL);
    double getOverallPacketRate(const struct timeval *now = NULL) const;
    double getCurrentPacketRate(const struct timeval *now = NULL, bool update = true);
    double getOverallByteRate(const struct timeval *now = NULL) const;
    double getCurrentByteRate(const struct timeval *now = NULL, bool update =true);
    unsigned long long getNumPackets(void) const;
    unsigned long long getNumBytes(void) const;

  private:
    RateMeter packet_rate_meter;
    RateMeter byte_rate_meter;
};

class ScanProgressMeter {
 public:
  /* A COPY of stypestr is made and saved for when stats are printed */
  ScanProgressMeter(const char *stypestr);
  ~ScanProgressMeter();
/* Decides whether a timing report is likely to even be
   printed.  There are stringent limitations on how often they are
   printed, as well as the verbosity level that must exist.  So you
   might as well check this before spending much time computing
   progress info.  now can be NULL if caller doesn't have the current
   time handy.  Just because this function returns true does not mean
   that the next printStatsIfNecessary will always print something.
   It depends on whether time estimates have changed, which this func
   doesn't even know about. */
  bool mayBePrinted(const struct timeval *now);

/* Prints an estimate of when this scan will complete.  It only does
   so if mayBePrinted() is true, and it seems reasonable to do so
   because the estimate has changed significantly.  Returns whether
   or not a line was printed.*/
  bool printStatsIfNecessary(double perc_done, const struct timeval *now);

  /* Prints an estimate of when this scan will complete. */
  bool printStats(double perc_done, const struct timeval *now);

  /* Prints that this task is complete. */
  bool endTask(const struct timeval *now, const char *additional_info) { return beginOrEndTask(now, additional_info, false); }

  struct timeval begin; /* When this ScanProgressMeter was instantiated */
 private:
  struct timeval last_print_test; /* Last time printStatsIfNecessary was called */
  struct timeval last_print; /* The most recent time the ETC was printed */
  char *scantypestr;
  struct timeval last_est; /* The latest PRINTED estimate */

  bool beginOrEndTask(const struct timeval *now, const char *additional_info, bool beginning);
};

#endif /* NMAP_TIMING_H */

