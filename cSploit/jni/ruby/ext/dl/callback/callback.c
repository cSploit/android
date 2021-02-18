#include "ruby.h"

VALUE rb_DLCdeclCallbackAddrs, rb_DLCdeclCallbackProcs;
#ifdef FUNC_STDCALL
VALUE rb_DLStdcallCallbackAddrs, rb_DLStdcallCallbackProcs;
#endif
/*static void *cdecl_callbacks[MAX_DLTYPE][MAX_CALLBACK];*/
#ifdef FUNC_STDCALL
/*static void *stdcall_callbacks[MAX_DLTYPE][MAX_CALLBACK];*/
#endif
ID   rb_dl_cb_call;
void rb_dl_init_callbacks_0();
void rb_dl_init_callbacks_1();
void rb_dl_init_callbacks_2();
void rb_dl_init_callbacks_3();
void rb_dl_init_callbacks_4();
void rb_dl_init_callbacks_5();
void rb_dl_init_callbacks_6();
void rb_dl_init_callbacks_7();
void rb_dl_init_callbacks_8();
void
Init_callback(void)
{
    VALUE tmp;
    VALUE rb_mDL = rb_path2class("DL");

    rb_dl_cb_call = rb_intern("call");

    tmp = rb_DLCdeclCallbackProcs = rb_ary_new();
    rb_define_const(rb_mDL, "CdeclCallbackProcs", tmp);

    tmp = rb_DLCdeclCallbackAddrs = rb_ary_new();
    rb_define_const(rb_mDL, "CdeclCallbackAddrs", tmp);

#ifdef FUNC_STDCALL
    tmp = rb_DLStdcallCallbackProcs = rb_ary_new();
    rb_define_const(rb_mDL, "StdcallCallbackProcs", tmp);

    tmp = rb_DLStdcallCallbackAddrs = rb_ary_new();
    rb_define_const(rb_mDL, "StdcallCallbackAddrs", tmp);
#endif

    rb_dl_init_callbacks_0();
    rb_dl_init_callbacks_1();
    rb_dl_init_callbacks_2();
    rb_dl_init_callbacks_3();
    rb_dl_init_callbacks_4();
    rb_dl_init_callbacks_5();
    rb_dl_init_callbacks_6();
    rb_dl_init_callbacks_7();
    rb_dl_init_callbacks_8();
}
