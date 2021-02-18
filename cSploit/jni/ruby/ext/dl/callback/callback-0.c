#include "dl.h"

extern VALUE rb_DLCdeclCallbackAddrs, rb_DLCdeclCallbackProcs;
#ifdef FUNC_STDCALL
extern VALUE rb_DLStdcallCallbackAddrs, rb_DLStdcallCallbackProcs;
#endif
extern ID   rb_dl_cb_call;

static void
FUNC_CDECL(rb_dl_callback_void_0_0_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_0_1_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_0_2_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_0_3_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_0_4_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_1_0_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_1_1_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_1_2_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_1_3_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_1_4_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_2_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_2_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_2_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_2_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_2_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_3_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_3_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_3_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_3_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_3_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_4_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_4_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_4_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_4_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_4_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_5_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_5_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_5_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_5_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_5_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_6_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_6_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_6_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_6_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_6_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_7_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_7_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_7_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_7_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_7_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_8_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_8_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_8_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_8_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_8_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_9_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_9_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_9_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_9_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_9_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_10_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_10_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_10_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_10_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_10_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_11_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_11_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_11_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_11_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_11_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_12_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_12_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_12_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_12_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_12_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_13_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_13_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_13_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_13_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_13_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_14_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_14_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_14_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_14_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_14_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_15_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_15_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_15_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_15_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_15_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_16_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_16_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_16_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_16_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_16_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_17_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_17_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_17_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_17_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_17_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_18_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_18_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_18_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_18_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_18_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_19_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_19_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_19_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_19_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}


static void
FUNC_CDECL(rb_dl_callback_void_19_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 0), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_0_0_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_0_1_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_0_2_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_0_3_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_0_4_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_1_0_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_1_1_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_1_2_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_1_3_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_1_4_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_2_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_2_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_2_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_2_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_2_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_3_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_3_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_3_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_3_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_3_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_4_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_4_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_4_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_4_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_4_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_5_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_5_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_5_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_5_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_5_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_6_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_6_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_6_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_6_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_6_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_7_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_7_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_7_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_7_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_7_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_8_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_8_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_8_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_8_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_8_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
{
    VALUE ret, cb, args[8];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_9_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_9_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_9_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_9_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_9_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
{
    VALUE ret, cb, args[9];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_10_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_10_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_10_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_10_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_10_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
{
    VALUE ret, cb, args[10];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_11_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_11_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_11_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_11_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_11_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
{
    VALUE ret, cb, args[11];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_12_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_12_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_12_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_12_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_12_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
{
    VALUE ret, cb, args[12];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_13_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_13_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_13_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_13_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_13_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
{
    VALUE ret, cb, args[13];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_14_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_14_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_14_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_14_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_14_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
{
    VALUE ret, cb, args[14];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_15_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_15_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_15_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_15_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_15_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
{
    VALUE ret, cb, args[15];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_16_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_16_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_16_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_16_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_16_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
{
    VALUE ret, cb, args[16];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_17_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_17_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_17_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_17_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_17_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
{
    VALUE ret, cb, args[17];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_18_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_18_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_18_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_18_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_18_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
{
    VALUE ret, cb, args[18];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_19_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_19_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_19_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_19_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}
#endif


#ifdef FUNC_STDCALL
static void
FUNC_STDCALL(rb_dl_callback_void_19_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
{
    VALUE ret, cb, args[19];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    args[7] = LONG2NUM(stack7);
    args[8] = LONG2NUM(stack8);
    args[9] = LONG2NUM(stack9);
    args[10] = LONG2NUM(stack10);
    args[11] = LONG2NUM(stack11);
    args[12] = LONG2NUM(stack12);
    args[13] = LONG2NUM(stack13);
    args[14] = LONG2NUM(stack14);
    args[15] = LONG2NUM(stack15);
    args[16] = LONG2NUM(stack16);
    args[17] = LONG2NUM(stack17);
    args[18] = LONG2NUM(stack18);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 0), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return ;
}
#endif

void
rb_dl_init_callbacks_0()
{
    rb_ary_push(rb_DLCdeclCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLCdeclCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_void_0_0_cdecl),PTR2NUM(rb_dl_callback_void_1_0_cdecl),PTR2NUM(rb_dl_callback_void_2_0_cdecl),PTR2NUM(rb_dl_callback_void_3_0_cdecl),PTR2NUM(rb_dl_callback_void_4_0_cdecl),PTR2NUM(rb_dl_callback_void_5_0_cdecl),PTR2NUM(rb_dl_callback_void_6_0_cdecl),PTR2NUM(rb_dl_callback_void_7_0_cdecl),PTR2NUM(rb_dl_callback_void_8_0_cdecl),PTR2NUM(rb_dl_callback_void_9_0_cdecl),PTR2NUM(rb_dl_callback_void_10_0_cdecl),PTR2NUM(rb_dl_callback_void_11_0_cdecl),PTR2NUM(rb_dl_callback_void_12_0_cdecl),PTR2NUM(rb_dl_callback_void_13_0_cdecl),PTR2NUM(rb_dl_callback_void_14_0_cdecl),PTR2NUM(rb_dl_callback_void_15_0_cdecl),PTR2NUM(rb_dl_callback_void_16_0_cdecl),PTR2NUM(rb_dl_callback_void_17_0_cdecl),PTR2NUM(rb_dl_callback_void_18_0_cdecl),PTR2NUM(rb_dl_callback_void_19_0_cdecl),PTR2NUM(rb_dl_callback_void_0_1_cdecl),PTR2NUM(rb_dl_callback_void_1_1_cdecl),PTR2NUM(rb_dl_callback_void_2_1_cdecl),PTR2NUM(rb_dl_callback_void_3_1_cdecl),PTR2NUM(rb_dl_callback_void_4_1_cdecl),PTR2NUM(rb_dl_callback_void_5_1_cdecl),PTR2NUM(rb_dl_callback_void_6_1_cdecl),PTR2NUM(rb_dl_callback_void_7_1_cdecl),PTR2NUM(rb_dl_callback_void_8_1_cdecl),PTR2NUM(rb_dl_callback_void_9_1_cdecl),PTR2NUM(rb_dl_callback_void_10_1_cdecl),PTR2NUM(rb_dl_callback_void_11_1_cdecl),PTR2NUM(rb_dl_callback_void_12_1_cdecl),PTR2NUM(rb_dl_callback_void_13_1_cdecl),PTR2NUM(rb_dl_callback_void_14_1_cdecl),PTR2NUM(rb_dl_callback_void_15_1_cdecl),PTR2NUM(rb_dl_callback_void_16_1_cdecl),PTR2NUM(rb_dl_callback_void_17_1_cdecl),PTR2NUM(rb_dl_callback_void_18_1_cdecl),PTR2NUM(rb_dl_callback_void_19_1_cdecl),PTR2NUM(rb_dl_callback_void_0_2_cdecl),PTR2NUM(rb_dl_callback_void_1_2_cdecl),PTR2NUM(rb_dl_callback_void_2_2_cdecl),PTR2NUM(rb_dl_callback_void_3_2_cdecl),PTR2NUM(rb_dl_callback_void_4_2_cdecl),PTR2NUM(rb_dl_callback_void_5_2_cdecl),PTR2NUM(rb_dl_callback_void_6_2_cdecl),PTR2NUM(rb_dl_callback_void_7_2_cdecl),PTR2NUM(rb_dl_callback_void_8_2_cdecl),PTR2NUM(rb_dl_callback_void_9_2_cdecl),PTR2NUM(rb_dl_callback_void_10_2_cdecl),PTR2NUM(rb_dl_callback_void_11_2_cdecl),PTR2NUM(rb_dl_callback_void_12_2_cdecl),PTR2NUM(rb_dl_callback_void_13_2_cdecl),PTR2NUM(rb_dl_callback_void_14_2_cdecl),PTR2NUM(rb_dl_callback_void_15_2_cdecl),PTR2NUM(rb_dl_callback_void_16_2_cdecl),PTR2NUM(rb_dl_callback_void_17_2_cdecl),PTR2NUM(rb_dl_callback_void_18_2_cdecl),PTR2NUM(rb_dl_callback_void_19_2_cdecl),PTR2NUM(rb_dl_callback_void_0_3_cdecl),PTR2NUM(rb_dl_callback_void_1_3_cdecl),PTR2NUM(rb_dl_callback_void_2_3_cdecl),PTR2NUM(rb_dl_callback_void_3_3_cdecl),PTR2NUM(rb_dl_callback_void_4_3_cdecl),PTR2NUM(rb_dl_callback_void_5_3_cdecl),PTR2NUM(rb_dl_callback_void_6_3_cdecl),PTR2NUM(rb_dl_callback_void_7_3_cdecl),PTR2NUM(rb_dl_callback_void_8_3_cdecl),PTR2NUM(rb_dl_callback_void_9_3_cdecl),PTR2NUM(rb_dl_callback_void_10_3_cdecl),PTR2NUM(rb_dl_callback_void_11_3_cdecl),PTR2NUM(rb_dl_callback_void_12_3_cdecl),PTR2NUM(rb_dl_callback_void_13_3_cdecl),PTR2NUM(rb_dl_callback_void_14_3_cdecl),PTR2NUM(rb_dl_callback_void_15_3_cdecl),PTR2NUM(rb_dl_callback_void_16_3_cdecl),PTR2NUM(rb_dl_callback_void_17_3_cdecl),PTR2NUM(rb_dl_callback_void_18_3_cdecl),PTR2NUM(rb_dl_callback_void_19_3_cdecl),PTR2NUM(rb_dl_callback_void_0_4_cdecl),PTR2NUM(rb_dl_callback_void_1_4_cdecl),PTR2NUM(rb_dl_callback_void_2_4_cdecl),PTR2NUM(rb_dl_callback_void_3_4_cdecl),PTR2NUM(rb_dl_callback_void_4_4_cdecl),PTR2NUM(rb_dl_callback_void_5_4_cdecl),PTR2NUM(rb_dl_callback_void_6_4_cdecl),PTR2NUM(rb_dl_callback_void_7_4_cdecl),PTR2NUM(rb_dl_callback_void_8_4_cdecl),PTR2NUM(rb_dl_callback_void_9_4_cdecl),PTR2NUM(rb_dl_callback_void_10_4_cdecl),PTR2NUM(rb_dl_callback_void_11_4_cdecl),PTR2NUM(rb_dl_callback_void_12_4_cdecl),PTR2NUM(rb_dl_callback_void_13_4_cdecl),PTR2NUM(rb_dl_callback_void_14_4_cdecl),PTR2NUM(rb_dl_callback_void_15_4_cdecl),PTR2NUM(rb_dl_callback_void_16_4_cdecl),PTR2NUM(rb_dl_callback_void_17_4_cdecl),PTR2NUM(rb_dl_callback_void_18_4_cdecl),PTR2NUM(rb_dl_callback_void_19_4_cdecl)));
#ifdef FUNC_STDCALL
    rb_ary_push(rb_DLStdcallCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLStdcallCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_void_0_0_stdcall),PTR2NUM(rb_dl_callback_void_1_0_stdcall),PTR2NUM(rb_dl_callback_void_2_0_stdcall),PTR2NUM(rb_dl_callback_void_3_0_stdcall),PTR2NUM(rb_dl_callback_void_4_0_stdcall),PTR2NUM(rb_dl_callback_void_5_0_stdcall),PTR2NUM(rb_dl_callback_void_6_0_stdcall),PTR2NUM(rb_dl_callback_void_7_0_stdcall),PTR2NUM(rb_dl_callback_void_8_0_stdcall),PTR2NUM(rb_dl_callback_void_9_0_stdcall),PTR2NUM(rb_dl_callback_void_10_0_stdcall),PTR2NUM(rb_dl_callback_void_11_0_stdcall),PTR2NUM(rb_dl_callback_void_12_0_stdcall),PTR2NUM(rb_dl_callback_void_13_0_stdcall),PTR2NUM(rb_dl_callback_void_14_0_stdcall),PTR2NUM(rb_dl_callback_void_15_0_stdcall),PTR2NUM(rb_dl_callback_void_16_0_stdcall),PTR2NUM(rb_dl_callback_void_17_0_stdcall),PTR2NUM(rb_dl_callback_void_18_0_stdcall),PTR2NUM(rb_dl_callback_void_19_0_stdcall),PTR2NUM(rb_dl_callback_void_0_1_stdcall),PTR2NUM(rb_dl_callback_void_1_1_stdcall),PTR2NUM(rb_dl_callback_void_2_1_stdcall),PTR2NUM(rb_dl_callback_void_3_1_stdcall),PTR2NUM(rb_dl_callback_void_4_1_stdcall),PTR2NUM(rb_dl_callback_void_5_1_stdcall),PTR2NUM(rb_dl_callback_void_6_1_stdcall),PTR2NUM(rb_dl_callback_void_7_1_stdcall),PTR2NUM(rb_dl_callback_void_8_1_stdcall),PTR2NUM(rb_dl_callback_void_9_1_stdcall),PTR2NUM(rb_dl_callback_void_10_1_stdcall),PTR2NUM(rb_dl_callback_void_11_1_stdcall),PTR2NUM(rb_dl_callback_void_12_1_stdcall),PTR2NUM(rb_dl_callback_void_13_1_stdcall),PTR2NUM(rb_dl_callback_void_14_1_stdcall),PTR2NUM(rb_dl_callback_void_15_1_stdcall),PTR2NUM(rb_dl_callback_void_16_1_stdcall),PTR2NUM(rb_dl_callback_void_17_1_stdcall),PTR2NUM(rb_dl_callback_void_18_1_stdcall),PTR2NUM(rb_dl_callback_void_19_1_stdcall),PTR2NUM(rb_dl_callback_void_0_2_stdcall),PTR2NUM(rb_dl_callback_void_1_2_stdcall),PTR2NUM(rb_dl_callback_void_2_2_stdcall),PTR2NUM(rb_dl_callback_void_3_2_stdcall),PTR2NUM(rb_dl_callback_void_4_2_stdcall),PTR2NUM(rb_dl_callback_void_5_2_stdcall),PTR2NUM(rb_dl_callback_void_6_2_stdcall),PTR2NUM(rb_dl_callback_void_7_2_stdcall),PTR2NUM(rb_dl_callback_void_8_2_stdcall),PTR2NUM(rb_dl_callback_void_9_2_stdcall),PTR2NUM(rb_dl_callback_void_10_2_stdcall),PTR2NUM(rb_dl_callback_void_11_2_stdcall),PTR2NUM(rb_dl_callback_void_12_2_stdcall),PTR2NUM(rb_dl_callback_void_13_2_stdcall),PTR2NUM(rb_dl_callback_void_14_2_stdcall),PTR2NUM(rb_dl_callback_void_15_2_stdcall),PTR2NUM(rb_dl_callback_void_16_2_stdcall),PTR2NUM(rb_dl_callback_void_17_2_stdcall),PTR2NUM(rb_dl_callback_void_18_2_stdcall),PTR2NUM(rb_dl_callback_void_19_2_stdcall),PTR2NUM(rb_dl_callback_void_0_3_stdcall),PTR2NUM(rb_dl_callback_void_1_3_stdcall),PTR2NUM(rb_dl_callback_void_2_3_stdcall),PTR2NUM(rb_dl_callback_void_3_3_stdcall),PTR2NUM(rb_dl_callback_void_4_3_stdcall),PTR2NUM(rb_dl_callback_void_5_3_stdcall),PTR2NUM(rb_dl_callback_void_6_3_stdcall),PTR2NUM(rb_dl_callback_void_7_3_stdcall),PTR2NUM(rb_dl_callback_void_8_3_stdcall),PTR2NUM(rb_dl_callback_void_9_3_stdcall),PTR2NUM(rb_dl_callback_void_10_3_stdcall),PTR2NUM(rb_dl_callback_void_11_3_stdcall),PTR2NUM(rb_dl_callback_void_12_3_stdcall),PTR2NUM(rb_dl_callback_void_13_3_stdcall),PTR2NUM(rb_dl_callback_void_14_3_stdcall),PTR2NUM(rb_dl_callback_void_15_3_stdcall),PTR2NUM(rb_dl_callback_void_16_3_stdcall),PTR2NUM(rb_dl_callback_void_17_3_stdcall),PTR2NUM(rb_dl_callback_void_18_3_stdcall),PTR2NUM(rb_dl_callback_void_19_3_stdcall),PTR2NUM(rb_dl_callback_void_0_4_stdcall),PTR2NUM(rb_dl_callback_void_1_4_stdcall),PTR2NUM(rb_dl_callback_void_2_4_stdcall),PTR2NUM(rb_dl_callback_void_3_4_stdcall),PTR2NUM(rb_dl_callback_void_4_4_stdcall),PTR2NUM(rb_dl_callback_void_5_4_stdcall),PTR2NUM(rb_dl_callback_void_6_4_stdcall),PTR2NUM(rb_dl_callback_void_7_4_stdcall),PTR2NUM(rb_dl_callback_void_8_4_stdcall),PTR2NUM(rb_dl_callback_void_9_4_stdcall),PTR2NUM(rb_dl_callback_void_10_4_stdcall),PTR2NUM(rb_dl_callback_void_11_4_stdcall),PTR2NUM(rb_dl_callback_void_12_4_stdcall),PTR2NUM(rb_dl_callback_void_13_4_stdcall),PTR2NUM(rb_dl_callback_void_14_4_stdcall),PTR2NUM(rb_dl_callback_void_15_4_stdcall),PTR2NUM(rb_dl_callback_void_16_4_stdcall),PTR2NUM(rb_dl_callback_void_17_4_stdcall),PTR2NUM(rb_dl_callback_void_18_4_stdcall),PTR2NUM(rb_dl_callback_void_19_4_stdcall)));
#endif
}
