#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ***********************IMPORTANT NMAP LICENSE TERMS************************
# *                                                                         *
# * The Nmap Security Scanner is (C) 1996-2013 Insecure.Com LLC. Nmap is    *
# * also a registered trademark of Insecure.Com LLC.  This program is free  *
# * software; you may redistribute and/or modify it under the terms of the  *
# * GNU General Public License as published by the Free Software            *
# * Foundation; Version 2 ("GPL"), BUT ONLY WITH ALL OF THE CLARIFICATIONS  *
# * AND EXCEPTIONS DESCRIBED HEREIN.  This guarantees your right to use,    *
# * modify, and redistribute this software under certain conditions.  If    *
# * you wish to embed Nmap technology into proprietary software, we sell    *
# * alternative licenses (contact sales@nmap.com).  Dozens of software      *
# * vendors already license Nmap technology such as host discovery, port    *
# * scanning, OS detection, version detection, and the Nmap Scripting       *
# * Engine.                                                                 *
# *                                                                         *
# * Note that the GPL places important restrictions on "derivative works",  *
# * yet it does not provide a detailed definition of that term.  To avoid   *
# * misunderstandings, we interpret that term as broadly as copyright law   *
# * allows.  For example, we consider an application to constitute a        *
# * derivative work for the purpose of this license if it does any of the   *
# * following with any software or content covered by this license          *
# * ("Covered Software"):                                                   *
# *                                                                         *
# * o Integrates source code from Covered Software.                         *
# *                                                                         *
# * o Reads or includes copyrighted data files, such as Nmap's nmap-os-db   *
# * or nmap-service-probes.                                                 *
# *                                                                         *
# * o Is designed specifically to execute Covered Software and parse the    *
# * results (as opposed to typical shell or execution-menu apps, which will *
# * execute anything you tell them to).                                     *
# *                                                                         *
# * o Includes Covered Software in a proprietary executable installer.  The *
# * installers produced by InstallShield are an example of this.  Including *
# * Nmap with other software in compressed or archival form does not        *
# * trigger this provision, provided appropriate open source decompression  *
# * or de-archiving software is widely available for no charge.  For the    *
# * purposes of this license, an installer is considered to include Covered *
# * Software even if it actually retrieves a copy of Covered Software from  *
# * another source during runtime (such as by downloading it from the       *
# * Internet).                                                              *
# *                                                                         *
# * o Links (statically or dynamically) to a library which does any of the  *
# * above.                                                                  *
# *                                                                         *
# * o Executes a helper program, module, or script to do any of the above.  *
# *                                                                         *
# * This list is not exclusive, but is meant to clarify our interpretation  *
# * of derived works with some common examples.  Other people may interpret *
# * the plain GPL differently, so we consider this a special exception to   *
# * the GPL that we apply to Covered Software.  Works which meet any of     *
# * these conditions must conform to all of the terms of this license,      *
# * particularly including the GPL Section 3 requirements of providing      *
# * source code and allowing free redistribution of the work as a whole.    *
# *                                                                         *
# * As another special exception to the GPL terms, Insecure.Com LLC grants  *
# * permission to link the code of this program with any version of the     *
# * OpenSSL library which is distributed under a license identical to that  *
# * listed in the included docs/licenses/OpenSSL.txt file, and distribute   *
# * linked combinations including the two.                                  *
# *                                                                         *
# * Any redistribution of Covered Software, including any derived works,    *
# * must obey and carry forward all of the terms of this license, including *
# * obeying all GPL rules and restrictions.  For example, source code of    *
# * the whole work must be provided and free redistribution must be         *
# * allowed.  All GPL references to "this License", are to be treated as    *
# * including the terms and conditions of this license text as well.        *
# *                                                                         *
# * Because this license imposes special exceptions to the GPL, Covered     *
# * Work may not be combined (even as part of a larger work) with plain GPL *
# * software.  The terms, conditions, and exceptions of this license must   *
# * be included as well.  This license is incompatible with some other open *
# * source licenses as well.  In some cases we can relicense portions of    *
# * Nmap or grant special permissions to use it in other open source        *
# * software.  Please contact fyodor@nmap.org with any such requests.       *
# * Similarly, we don't incorporate incompatible open source software into  *
# * Covered Software without special permission from the copyright holders. *
# *                                                                         *
# * If you have any questions about the licensing restrictions on using     *
# * Nmap in other works, are happy to help.  As mentioned above, we also    *
# * offer alternative license to integrate Nmap into proprietary            *
# * applications and appliances.  These contracts have been sold to dozens  *
# * of software vendors, and generally include a perpetual license as well  *
# * as providing for priority support and updates.  They also fund the      *
# * continued development of Nmap.  Please email sales@nmap.com for further *
# * information.                                                            *
# *                                                                         *
# * If you have received a written license agreement or contract for        *
# * Covered Software stating terms other than these, you may choose to use  *
# * and redistribute Covered Software under those terms instead of these.   *
# *                                                                         *
# * Source is provided to this software because we believe users have a     *
# * right to know exactly what a program is going to do before they run it. *
# * This also allows you to audit the software for security holes (none     *
# * have been found so far).                                                *
# *                                                                         *
# * Source code also allows you to port Nmap to new platforms, fix bugs,    *
# * and add new features.  You are highly encouraged to send your changes   *
# * to the dev@nmap.org mailing list for possible incorporation into the    *
# * main distribution.  By sending these changes to Fyodor or one of the    *
# * Insecure.Org development mailing lists, or checking them into the Nmap  *
# * source code repository, it is understood (unless you specify otherwise) *
# * that you are offering the Nmap Project (Insecure.Com LLC) the           *
# * unlimited, non-exclusive right to reuse, modify, and relicense the      *
# * code.  Nmap will always be available Open Source, but this is important *
# * because the inability to relicense code has caused devastating problems *
# * for other Free Software projects (such as KDE and NASM).  We also       *
# * occasionally relicense the code to third parties as discussed above.    *
# * If you wish to specify special license conditions of your               *
# * contributions, just say so when you send them.                          *
# *                                                                         *
# * This program is distributed in the hope that it will be useful, but     *
# * WITHOUT ANY WARRANTY; without even the implied warranty of              *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the Nmap      *
# * license file for more details (it's in a COPYING file included with     *
# * Nmap, and also available from https://svn.nmap.org/nmap/COPYING         *
# *                                                                         *
# ***************************************************************************/

import gtk

from types import StringTypes
from zenmapGUI.higwidgets.higboxes import HIGVBox
from zenmapGUI.Icons import get_os_icon
import zenmapCore.I18N


def treemodel_get_addrs_for_sort(model, iter):
    host = model.get_value(iter, 0)
    return host.get_addrs_for_sort()


# Used to sort hosts by address.
def cmp_treemodel_addr(model, iter_a, iter_b):
    addrs_a = treemodel_get_addrs_for_sort(model, iter_a)
    addrs_b = treemodel_get_addrs_for_sort(model, iter_b)
    return cmp(addrs_a, addrs_b)


class ScanHostsView(HIGVBox, object):
    HOST_MODE, SERVICE_MODE = range(2)

    def __init__(self, scan_interface):
        HIGVBox.__init__(self)

        self._scan_interface = scan_interface
        self._create_widgets()
        self._connect_widgets()
        self._pack_widgets()
        self._set_scrolled()
        self._set_host_list()
        self._set_service_list()

        self._pack_expand_fill(self.main_vbox)

        self.mode = None

        # Default mode is host mode
        self.host_mode(self.host_mode_button)

        self.host_view.show_all()
        self.service_view.show_all()

    def _create_widgets(self):
        # Mode buttons
        self.host_mode_button = gtk.ToggleButton(_("Hosts"))
        self.service_mode_button = gtk.ToggleButton(_("Services"))
        self.buttons_box = gtk.HBox()

        # Main window vbox
        self.main_vbox = HIGVBox()

        # Host list
        self.host_list = gtk.ListStore(object, str, str)
        self.host_list.set_sort_func(1000, cmp_treemodel_addr)
        self.host_list.set_sort_column_id(1000, gtk.SORT_ASCENDING)
        self.host_view = gtk.TreeView(self.host_list)
        self.pic_column = gtk.TreeViewColumn(_('OS'))
        self.host_column = gtk.TreeViewColumn(_('Host'))
        self.os_cell = gtk.CellRendererPixbuf()
        self.host_cell = gtk.CellRendererText()

        # Service list
        self.service_list = gtk.ListStore(str)
        self.service_list.set_sort_column_id(0, gtk.SORT_ASCENDING)
        self.service_view = gtk.TreeView(self.service_list)
        self.service_column = gtk.TreeViewColumn(_('Service'))
        self.service_cell = gtk.CellRendererText()

        self.scrolled = gtk.ScrolledWindow()

    def _pack_widgets(self):
        self.main_vbox.set_spacing(0)
        self.main_vbox.set_border_width(0)
        self.main_vbox._pack_noexpand_nofill(self.buttons_box)
        self.main_vbox._pack_expand_fill(self.scrolled)

        self.host_mode_button.set_active(True)

        self.buttons_box.set_border_width(5)
        self.buttons_box.pack_start(self.host_mode_button)
        self.buttons_box.pack_start(self.service_mode_button)

    def _connect_widgets(self):
        self.host_mode_button.connect("toggled", self.host_mode)
        self.service_mode_button.connect("toggled", self.service_mode)

    def host_mode(self, widget):
        self._remove_scrolled_child()
        if widget.get_active():
            self.mode = self.HOST_MODE
            self.service_mode_button.set_active(False)
            self.scrolled.add(self.host_view)
        else:
            self.service_mode_button.set_active(True)

    def service_mode(self, widget):
        self._remove_scrolled_child()
        if widget.get_active():
            self.mode = self.SERVICE_MODE
            self.host_mode_button.set_active(False)
            self.scrolled.add(self.service_view)
        else:
            self.host_mode_button.set_active(True)

    def _remove_scrolled_child(self):
        try:
            child = self.scrolled.get_child()
            self.scrolled.remove(child)
        except:
            pass

    def _set_scrolled(self):
        self.scrolled.set_border_width(5)
        self.scrolled.set_size_request(150, -1)
        self.scrolled.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)

    def _set_service_list(self):
        self.service_view.set_enable_search(True)
        self.service_view.set_search_column(0)

        selection = self.service_view.get_selection()
        selection.set_mode(gtk.SELECTION_MULTIPLE)
        self.service_view.append_column(self.service_column)

        self.service_column.set_resizable(True)
        self.service_column.set_sort_column_id(0)
        self.service_column.set_reorderable(True)
        self.service_column.pack_start(self.service_cell, True)
        self.service_column.set_attributes(self.service_cell, text=0)

    def _set_host_list(self):
        self.host_view.set_enable_search(True)
        self.host_view.set_search_column(1)

        selection = self.host_view.get_selection()
        selection.set_mode(gtk.SELECTION_MULTIPLE)

        self.host_view.append_column(self.pic_column)
        self.host_view.append_column(self.host_column)

        self.host_column.set_resizable(True)
        self.pic_column.set_resizable(True)

        self.host_column.set_sort_column_id(1000)
        self.pic_column.set_sort_column_id(1)

        self.host_column.set_reorderable(True)
        self.pic_column.set_reorderable(True)

        self.pic_column.pack_start(self.os_cell, True)
        self.host_column.pack_start(self.host_cell, True)

        self.pic_column.set_min_width(35)
        self.pic_column.set_attributes(self.os_cell, stock_id=1)
        self.host_column.set_attributes(self.host_cell, text=2)

    def mass_update(self, hosts):
        """Update the internal ListStores to reflect the hosts and services
        passed in. Hosts that have not changed are left alone."""
        hosts = set(hosts)
        services = set()
        for h in hosts:
            services.update([s["service_name"] for s in h.services])

        # Disable sorting while elements are added. See the PyGTK FAQ 13.43,
        # "Are there tips for improving performance when adding many rows to a
        # Treeview?"
        sort_column_id = self.host_list.get_sort_column_id()
        self.host_list.set_default_sort_func(lambda *args: -1)
        self.host_list.set_sort_column_id(-1, gtk.SORT_ASCENDING)
        self.host_view.freeze_child_notify()
        self.host_view.set_model(None)

        it = self.host_list.get_iter_first()
        # Remove any of our ListStore hosts that aren't in the list passed in.
        while it:
            host = self.host_list.get_value(it, 0)
            if host in hosts:
                hosts.remove(host)
                self.host_list.set(it, 1, get_os_icon(host))
                it = self.host_list.iter_next(it)
            else:
                if not self.host_list.remove(it):
                    it = None
        # Add any remaining hosts into our ListStore.
        for host in hosts:
            self.add_host(host)

        # Reenable sorting.
        if sort_column_id != (None, None):
            self.host_list.set_sort_column_id(*sort_column_id)
        self.host_view.set_model(self.host_list)
        self.host_view.thaw_child_notify()

        it = self.service_list.get_iter_first()
        # Remove any of our ListStore services that aren't in the list passed
        # in.
        while it:
            service_name = self.service_list.get_value(it, 0)
            if service_name in services:
                services.remove(service_name)
                it = self.service_list.iter_next(it)
            else:
                if not self.service_list.remove(it):
                    it = None
        # Add any remaining services into our ListStore.
        for service_name in services:
            self.add_service(service_name)

    def add_host(self, host):
        self.host_list.append([host, get_os_icon(host), host.get_hostname()])

    def add_service(self, service):
        self.service_list.append([service])

if __name__ == "__main__":
    w = gtk.Window()
    h = ScanHostsView()
    w.add(h)
    w.show_all()
    gtk.main()
