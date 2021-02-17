LOCAL_PATH := $(call my-dir)

#original path: lib/libncurses.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ncurses\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ncurses/tty/hardscroll.c\
	ncurses/tty/hashmap.c\
	ncurses/base/lib_addch.c\
	ncurses/base/lib_addstr.c\
	ncurses/base/lib_beep.c\
	ncurses/base/lib_bkgd.c\
	ncurses/base/lib_box.c\
	ncurses/base/lib_chgat.c\
	ncurses/base/lib_clear.c\
	ncurses/base/lib_clearok.c\
	ncurses/base/lib_clrbot.c\
	ncurses/base/lib_clreol.c\
	ncurses/base/lib_color.c\
	ncurses/base/lib_colorset.c\
	ncurses/base/lib_delch.c\
	ncurses/base/lib_delwin.c\
	ncurses/base/lib_echo.c\
	ncurses/base/lib_endwin.c\
	ncurses/base/lib_erase.c\
	ncurses/base/lib_flash.c\
	ncurses/lib_gen.c\
	ncurses/base/lib_getch.c\
	ncurses/base/lib_getstr.c\
	ncurses/base/lib_hline.c\
	ncurses/base/lib_immedok.c\
	ncurses/base/lib_inchstr.c\
	ncurses/base/lib_initscr.c\
	ncurses/base/lib_insch.c\
	ncurses/base/lib_insdel.c\
	ncurses/base/lib_insnstr.c\
	ncurses/base/lib_instr.c\
	ncurses/base/lib_isendwin.c\
	ncurses/base/lib_leaveok.c\
	ncurses/base/lib_mouse.c\
	ncurses/base/lib_move.c\
	ncurses/tty/lib_mvcur.c\
	ncurses/base/lib_mvwin.c\
	ncurses/base/lib_newterm.c\
	ncurses/base/lib_newwin.c\
	ncurses/base/lib_nl.c\
	ncurses/base/lib_overlay.c\
	ncurses/base/lib_pad.c\
	ncurses/base/lib_printw.c\
	ncurses/base/lib_redrawln.c\
	ncurses/base/lib_refresh.c\
	ncurses/base/lib_restart.c\
	ncurses/base/lib_scanw.c\
	ncurses/base/lib_screen.c\
	ncurses/base/lib_scroll.c\
	ncurses/base/lib_scrollok.c\
	ncurses/base/lib_scrreg.c\
	ncurses/base/lib_set_term.c\
	ncurses/base/lib_slk.c\
	ncurses/base/lib_slkatr_set.c\
	ncurses/base/lib_slkatrof.c\
	ncurses/base/lib_slkatron.c\
	ncurses/base/lib_slkatrset.c\
	ncurses/base/lib_slkattr.c\
	ncurses/base/lib_slkclear.c\
	ncurses/base/lib_slkcolor.c\
	ncurses/base/lib_slkinit.c\
	ncurses/base/lib_slklab.c\
	ncurses/base/lib_slkrefr.c\
	ncurses/base/lib_slkset.c\
	ncurses/base/lib_slktouch.c\
	ncurses/base/lib_touch.c\
	ncurses/tty/lib_tstp.c\
	ncurses/base/lib_ungetch.c\
	ncurses/tty/lib_vidattr.c\
	ncurses/base/lib_vline.c\
	ncurses/base/lib_wattroff.c\
	ncurses/base/lib_wattron.c\
	ncurses/base/lib_winch.c\
	ncurses/base/lib_window.c\
	ncurses/base/nc_panel.c\
	ncurses/base/safe_sprintf.c\
	ncurses/tty/tty_update.c\
	ncurses/trace/varargs.c\
	ncurses/base/memmove.c\
	ncurses/base/vsscanf.c\
	ncurses/base/lib_freeall.c\
	ncurses/expanded.c\
	ncurses/base/legacy_coding.c\
	ncurses/base/lib_dft_fgbg.c\
	ncurses/tinfo/lib_print.c\
	ncurses/base/resizeterm.c\
	ncurses/tinfo/use_screen.c\
	ncurses/base/use_window.c\
	ncurses/base/wresize.c\
	ncurses/tinfo/access.c\
	ncurses/tinfo/add_tries.c\
	ncurses/tinfo/alloc_ttype.c\
	ncurses/codes.c\
	ncurses/comp_captab.c\
	ncurses/tinfo/comp_error.c\
	ncurses/tinfo/comp_hash.c\
	ncurses/tinfo/db_iterator.c\
	ncurses/tinfo/doalloc.c\
	ncurses/tinfo/entries.c\
	ncurses/fallback.c\
	ncurses/tinfo/free_ttype.c\
	ncurses/tinfo/getenv_num.c\
	ncurses/tinfo/home_terminfo.c\
	ncurses/tinfo/init_keytry.c\
	ncurses/tinfo/lib_acs.c\
	ncurses/tinfo/lib_baudrate.c\
	ncurses/tinfo/lib_cur_term.c\
	ncurses/tinfo/lib_data.c\
	ncurses/tinfo/lib_has_cap.c\
	ncurses/tinfo/lib_kernel.c\
	ncurses/lib_keyname.c\
	ncurses/tinfo/lib_longname.c\
	ncurses/tinfo/lib_napms.c\
	ncurses/tinfo/lib_options.c\
	ncurses/tinfo/lib_raw.c\
	ncurses/tinfo/lib_setup.c\
	ncurses/tinfo/lib_termcap.c\
	ncurses/tinfo/lib_termname.c\
	ncurses/tinfo/lib_tgoto.c\
	ncurses/tinfo/lib_ti.c\
	ncurses/tinfo/lib_tparm.c\
	ncurses/tinfo/lib_tputs.c\
	ncurses/trace/lib_trace.c\
	ncurses/tinfo/lib_ttyflags.c\
	ncurses/tty/lib_twait.c\
	ncurses/tinfo/name_match.c\
	ncurses/names.c\
	ncurses/tinfo/read_entry.c\
	ncurses/tinfo/read_termcap.c\
	ncurses/tinfo/setbuf.c\
	ncurses/tinfo/strings.c\
	ncurses/base/tries.c\
	ncurses/tinfo/trim_sgr0.c\
	ncurses/unctrl.c\
	ncurses/trace/visbuf.c\
	ncurses/tinfo/alloc_entry.c\
	ncurses/tinfo/captoinfo.c\
	ncurses/tinfo/comp_expand.c\
	ncurses/tinfo/comp_parse.c\
	ncurses/tinfo/comp_scan.c\
	ncurses/tinfo/parse_entry.c\
	ncurses/tinfo/write_entry.c\
	ncurses/base/define_key.c\
	ncurses/tinfo/hashed_db.c\
	ncurses/base/key_defined.c\
	ncurses/base/keybound.c\
	ncurses/base/keyok.c\
	ncurses/base/version.c
LOCAL_MODULE := libncurses

include $(BUILD_STATIC_LIBRARY)


#original path: progs/tic

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

LOCAL_LDFLAGS:= -static  

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections -static

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/progs\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	progs/tic.c\
	progs/dump_entry.c\
	progs/transform.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := tic


include $(BUILD_EXECUTABLE)


#original path: progs/infocmp

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

LOCAL_LDFLAGS:= -static  

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections -static

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/progs\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	progs/infocmp.c\
	progs/dump_entry.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := infocmp


include $(BUILD_EXECUTABLE)


#original path: progs/clear

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

LOCAL_LDFLAGS:= -static  

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections -static

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/progs\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	progs/clear.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := clear


include $(BUILD_EXECUTABLE)


#original path: progs/tabs

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

LOCAL_LDFLAGS:= -static  

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections -static

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/progs\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	progs/tabs.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := tabs


include $(BUILD_EXECUTABLE)


#original path: progs/tput

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

LOCAL_LDFLAGS:= -static  

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections -static

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/progs\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	progs/tput.c\
	progs/transform.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := tput


include $(BUILD_EXECUTABLE)


#original path: progs/tset

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

LOCAL_LDFLAGS:= -static  

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections -static

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/progs\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	progs/tset.c\
	progs/transform.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := tset


include $(BUILD_EXECUTABLE)


#original path: progs/toe

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

LOCAL_LDFLAGS:= -static  

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections -static

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/progs\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	progs/toe.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := toe


include $(BUILD_EXECUTABLE)


#original path: lib/libpanel.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ncurses\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	panel/panel.c\
	panel/p_above.c\
	panel/p_below.c\
	panel/p_bottom.c\
	panel/p_delete.c\
	panel/p_hide.c\
	panel/p_hidden.c\
	panel/p_move.c\
	panel/p_new.c\
	panel/p_replace.c\
	panel/p_show.c\
	panel/p_top.c\
	panel/p_update.c\
	panel/p_user.c\
	panel/p_win.c
LOCAL_SHARED_LIBRARIES:= \
	ncurses
LOCAL_MODULE := libpanel

include $(BUILD_STATIC_LIBRARY)


#original path: lib/libmenu.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ncurses\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	menu/m_attribs.c\
	menu/m_cursor.c\
	menu/m_driver.c\
	menu/m_format.c\
	menu/m_global.c\
	menu/m_hook.c\
	menu/m_item_cur.c\
	menu/m_item_nam.c\
	menu/m_item_new.c\
	menu/m_item_opt.c\
	menu/m_item_top.c\
	menu/m_item_use.c\
	menu/m_item_val.c\
	menu/m_item_vis.c\
	menu/m_items.c\
	menu/m_new.c\
	menu/m_opts.c\
	menu/m_pad.c\
	menu/m_pattern.c\
	menu/m_post.c\
	menu/m_req_name.c\
	menu/m_scale.c\
	menu/m_spacing.c\
	menu/m_sub.c\
	menu/m_userptr.c\
	menu/m_win.c
LOCAL_SHARED_LIBRARIES:= \
	ncurses
LOCAL_MODULE := libmenu

include $(BUILD_STATIC_LIBRARY)


#original path: lib/libform.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNDEBUG 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ncurses\
	$(LOCAL_PATH)/objects\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	form/fld_arg.c\
	form/fld_attr.c\
	form/fld_current.c\
	form/fld_def.c\
	form/fld_dup.c\
	form/fld_ftchoice.c\
	form/fld_ftlink.c\
	form/fld_info.c\
	form/fld_just.c\
	form/fld_link.c\
	form/fld_max.c\
	form/fld_move.c\
	form/fld_newftyp.c\
	form/fld_opts.c\
	form/fld_pad.c\
	form/fld_page.c\
	form/fld_stat.c\
	form/fld_type.c\
	form/fld_user.c\
	form/frm_cursor.c\
	form/frm_data.c\
	form/frm_def.c\
	form/frm_driver.c\
	form/frm_hook.c\
	form/frm_opts.c\
	form/frm_page.c\
	form/frm_post.c\
	form/frm_req_name.c\
	form/frm_scale.c\
	form/frm_sub.c\
	form/frm_user.c\
	form/frm_win.c\
	form/fty_alnum.c\
	form/fty_alpha.c\
	form/fty_enum.c\
	form/fty_generic.c\
	form/fty_int.c\
	form/fty_ipv4.c\
	form/fty_num.c\
	form/fty_regex.c
LOCAL_SHARED_LIBRARIES:= \
	ncurses
LOCAL_MODULE := libform

include $(BUILD_STATIC_LIBRARY)

