#include "dl.h"

extern VALUE rb_DLCdeclCallbackAddrs, rb_DLCdeclCallbackProcs;
#ifdef FUNC_STDCALL
extern VALUE rb_DLStdcallCallbackAddrs, rb_DLStdcallCallbackProcs;
#endif
extern ID   rb_dl_cb_call;

static long
FUNC_CDECL(rb_dl_callback_long_0_0_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_0_1_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_0_2_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_0_3_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_0_4_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_1_0_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_1_1_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_1_2_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_1_3_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_1_4_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_2_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_2_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_2_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_2_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_2_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_3_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_3_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_3_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_3_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_3_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_4_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_4_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_4_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_4_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_4_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_5_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_5_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_5_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_5_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_5_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_6_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_6_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_6_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_6_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_6_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_7_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_7_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_7_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_7_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_7_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_8_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_8_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_8_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_8_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_8_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_9_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_9_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_9_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_9_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_9_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_10_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_10_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_10_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_10_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_10_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_11_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_11_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_11_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_11_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_11_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_12_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_12_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_12_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_12_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_12_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_13_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_13_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_13_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_13_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_13_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_14_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_14_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_14_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_14_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_14_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_15_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_15_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_15_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_15_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_15_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_16_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_16_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_16_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_16_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_16_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_17_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_17_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_17_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_17_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_17_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_18_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_18_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_18_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_18_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_18_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_19_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_19_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_19_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_19_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}


static long
FUNC_CDECL(rb_dl_callback_long_19_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 5), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_0_0_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_0_1_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_0_2_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_0_3_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_0_4_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_1_0_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_1_1_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_1_2_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_1_3_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_1_4_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_2_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_2_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_2_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_2_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_2_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_3_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_3_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_3_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_3_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_3_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_4_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_4_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_4_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_4_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_4_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_5_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_5_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_5_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_5_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_5_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_6_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_6_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_6_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_6_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_6_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_7_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_7_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_7_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_7_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_7_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_8_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_8_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_8_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_8_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_8_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_9_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_9_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_9_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_9_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_9_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_10_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_10_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_10_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_10_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_10_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_11_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_11_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_11_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_11_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_11_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_12_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_12_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_12_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_12_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_12_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_13_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_13_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_13_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_13_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_13_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_14_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_14_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_14_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_14_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_14_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_15_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_15_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_15_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_15_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_15_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_16_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_16_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_16_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_16_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_16_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_17_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_17_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_17_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_17_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_17_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_18_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_18_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_18_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_18_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_18_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_19_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_19_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_19_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_19_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}
#endif


#ifdef FUNC_STDCALL
static long
FUNC_STDCALL(rb_dl_callback_long_19_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 5), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return NUM2LONG(ret);
}
#endif

void
rb_dl_init_callbacks_5()
{
    rb_ary_push(rb_DLCdeclCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLCdeclCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_long_0_0_cdecl),PTR2NUM(rb_dl_callback_long_1_0_cdecl),PTR2NUM(rb_dl_callback_long_2_0_cdecl),PTR2NUM(rb_dl_callback_long_3_0_cdecl),PTR2NUM(rb_dl_callback_long_4_0_cdecl),PTR2NUM(rb_dl_callback_long_5_0_cdecl),PTR2NUM(rb_dl_callback_long_6_0_cdecl),PTR2NUM(rb_dl_callback_long_7_0_cdecl),PTR2NUM(rb_dl_callback_long_8_0_cdecl),PTR2NUM(rb_dl_callback_long_9_0_cdecl),PTR2NUM(rb_dl_callback_long_10_0_cdecl),PTR2NUM(rb_dl_callback_long_11_0_cdecl),PTR2NUM(rb_dl_callback_long_12_0_cdecl),PTR2NUM(rb_dl_callback_long_13_0_cdecl),PTR2NUM(rb_dl_callback_long_14_0_cdecl),PTR2NUM(rb_dl_callback_long_15_0_cdecl),PTR2NUM(rb_dl_callback_long_16_0_cdecl),PTR2NUM(rb_dl_callback_long_17_0_cdecl),PTR2NUM(rb_dl_callback_long_18_0_cdecl),PTR2NUM(rb_dl_callback_long_19_0_cdecl),PTR2NUM(rb_dl_callback_long_0_1_cdecl),PTR2NUM(rb_dl_callback_long_1_1_cdecl),PTR2NUM(rb_dl_callback_long_2_1_cdecl),PTR2NUM(rb_dl_callback_long_3_1_cdecl),PTR2NUM(rb_dl_callback_long_4_1_cdecl),PTR2NUM(rb_dl_callback_long_5_1_cdecl),PTR2NUM(rb_dl_callback_long_6_1_cdecl),PTR2NUM(rb_dl_callback_long_7_1_cdecl),PTR2NUM(rb_dl_callback_long_8_1_cdecl),PTR2NUM(rb_dl_callback_long_9_1_cdecl),PTR2NUM(rb_dl_callback_long_10_1_cdecl),PTR2NUM(rb_dl_callback_long_11_1_cdecl),PTR2NUM(rb_dl_callback_long_12_1_cdecl),PTR2NUM(rb_dl_callback_long_13_1_cdecl),PTR2NUM(rb_dl_callback_long_14_1_cdecl),PTR2NUM(rb_dl_callback_long_15_1_cdecl),PTR2NUM(rb_dl_callback_long_16_1_cdecl),PTR2NUM(rb_dl_callback_long_17_1_cdecl),PTR2NUM(rb_dl_callback_long_18_1_cdecl),PTR2NUM(rb_dl_callback_long_19_1_cdecl),PTR2NUM(rb_dl_callback_long_0_2_cdecl),PTR2NUM(rb_dl_callback_long_1_2_cdecl),PTR2NUM(rb_dl_callback_long_2_2_cdecl),PTR2NUM(rb_dl_callback_long_3_2_cdecl),PTR2NUM(rb_dl_callback_long_4_2_cdecl),PTR2NUM(rb_dl_callback_long_5_2_cdecl),PTR2NUM(rb_dl_callback_long_6_2_cdecl),PTR2NUM(rb_dl_callback_long_7_2_cdecl),PTR2NUM(rb_dl_callback_long_8_2_cdecl),PTR2NUM(rb_dl_callback_long_9_2_cdecl),PTR2NUM(rb_dl_callback_long_10_2_cdecl),PTR2NUM(rb_dl_callback_long_11_2_cdecl),PTR2NUM(rb_dl_callback_long_12_2_cdecl),PTR2NUM(rb_dl_callback_long_13_2_cdecl),PTR2NUM(rb_dl_callback_long_14_2_cdecl),PTR2NUM(rb_dl_callback_long_15_2_cdecl),PTR2NUM(rb_dl_callback_long_16_2_cdecl),PTR2NUM(rb_dl_callback_long_17_2_cdecl),PTR2NUM(rb_dl_callback_long_18_2_cdecl),PTR2NUM(rb_dl_callback_long_19_2_cdecl),PTR2NUM(rb_dl_callback_long_0_3_cdecl),PTR2NUM(rb_dl_callback_long_1_3_cdecl),PTR2NUM(rb_dl_callback_long_2_3_cdecl),PTR2NUM(rb_dl_callback_long_3_3_cdecl),PTR2NUM(rb_dl_callback_long_4_3_cdecl),PTR2NUM(rb_dl_callback_long_5_3_cdecl),PTR2NUM(rb_dl_callback_long_6_3_cdecl),PTR2NUM(rb_dl_callback_long_7_3_cdecl),PTR2NUM(rb_dl_callback_long_8_3_cdecl),PTR2NUM(rb_dl_callback_long_9_3_cdecl),PTR2NUM(rb_dl_callback_long_10_3_cdecl),PTR2NUM(rb_dl_callback_long_11_3_cdecl),PTR2NUM(rb_dl_callback_long_12_3_cdecl),PTR2NUM(rb_dl_callback_long_13_3_cdecl),PTR2NUM(rb_dl_callback_long_14_3_cdecl),PTR2NUM(rb_dl_callback_long_15_3_cdecl),PTR2NUM(rb_dl_callback_long_16_3_cdecl),PTR2NUM(rb_dl_callback_long_17_3_cdecl),PTR2NUM(rb_dl_callback_long_18_3_cdecl),PTR2NUM(rb_dl_callback_long_19_3_cdecl),PTR2NUM(rb_dl_callback_long_0_4_cdecl),PTR2NUM(rb_dl_callback_long_1_4_cdecl),PTR2NUM(rb_dl_callback_long_2_4_cdecl),PTR2NUM(rb_dl_callback_long_3_4_cdecl),PTR2NUM(rb_dl_callback_long_4_4_cdecl),PTR2NUM(rb_dl_callback_long_5_4_cdecl),PTR2NUM(rb_dl_callback_long_6_4_cdecl),PTR2NUM(rb_dl_callback_long_7_4_cdecl),PTR2NUM(rb_dl_callback_long_8_4_cdecl),PTR2NUM(rb_dl_callback_long_9_4_cdecl),PTR2NUM(rb_dl_callback_long_10_4_cdecl),PTR2NUM(rb_dl_callback_long_11_4_cdecl),PTR2NUM(rb_dl_callback_long_12_4_cdecl),PTR2NUM(rb_dl_callback_long_13_4_cdecl),PTR2NUM(rb_dl_callback_long_14_4_cdecl),PTR2NUM(rb_dl_callback_long_15_4_cdecl),PTR2NUM(rb_dl_callback_long_16_4_cdecl),PTR2NUM(rb_dl_callback_long_17_4_cdecl),PTR2NUM(rb_dl_callback_long_18_4_cdecl),PTR2NUM(rb_dl_callback_long_19_4_cdecl)));
#ifdef FUNC_STDCALL
    rb_ary_push(rb_DLStdcallCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLStdcallCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_long_0_0_stdcall),PTR2NUM(rb_dl_callback_long_1_0_stdcall),PTR2NUM(rb_dl_callback_long_2_0_stdcall),PTR2NUM(rb_dl_callback_long_3_0_stdcall),PTR2NUM(rb_dl_callback_long_4_0_stdcall),PTR2NUM(rb_dl_callback_long_5_0_stdcall),PTR2NUM(rb_dl_callback_long_6_0_stdcall),PTR2NUM(rb_dl_callback_long_7_0_stdcall),PTR2NUM(rb_dl_callback_long_8_0_stdcall),PTR2NUM(rb_dl_callback_long_9_0_stdcall),PTR2NUM(rb_dl_callback_long_10_0_stdcall),PTR2NUM(rb_dl_callback_long_11_0_stdcall),PTR2NUM(rb_dl_callback_long_12_0_stdcall),PTR2NUM(rb_dl_callback_long_13_0_stdcall),PTR2NUM(rb_dl_callback_long_14_0_stdcall),PTR2NUM(rb_dl_callback_long_15_0_stdcall),PTR2NUM(rb_dl_callback_long_16_0_stdcall),PTR2NUM(rb_dl_callback_long_17_0_stdcall),PTR2NUM(rb_dl_callback_long_18_0_stdcall),PTR2NUM(rb_dl_callback_long_19_0_stdcall),PTR2NUM(rb_dl_callback_long_0_1_stdcall),PTR2NUM(rb_dl_callback_long_1_1_stdcall),PTR2NUM(rb_dl_callback_long_2_1_stdcall),PTR2NUM(rb_dl_callback_long_3_1_stdcall),PTR2NUM(rb_dl_callback_long_4_1_stdcall),PTR2NUM(rb_dl_callback_long_5_1_stdcall),PTR2NUM(rb_dl_callback_long_6_1_stdcall),PTR2NUM(rb_dl_callback_long_7_1_stdcall),PTR2NUM(rb_dl_callback_long_8_1_stdcall),PTR2NUM(rb_dl_callback_long_9_1_stdcall),PTR2NUM(rb_dl_callback_long_10_1_stdcall),PTR2NUM(rb_dl_callback_long_11_1_stdcall),PTR2NUM(rb_dl_callback_long_12_1_stdcall),PTR2NUM(rb_dl_callback_long_13_1_stdcall),PTR2NUM(rb_dl_callback_long_14_1_stdcall),PTR2NUM(rb_dl_callback_long_15_1_stdcall),PTR2NUM(rb_dl_callback_long_16_1_stdcall),PTR2NUM(rb_dl_callback_long_17_1_stdcall),PTR2NUM(rb_dl_callback_long_18_1_stdcall),PTR2NUM(rb_dl_callback_long_19_1_stdcall),PTR2NUM(rb_dl_callback_long_0_2_stdcall),PTR2NUM(rb_dl_callback_long_1_2_stdcall),PTR2NUM(rb_dl_callback_long_2_2_stdcall),PTR2NUM(rb_dl_callback_long_3_2_stdcall),PTR2NUM(rb_dl_callback_long_4_2_stdcall),PTR2NUM(rb_dl_callback_long_5_2_stdcall),PTR2NUM(rb_dl_callback_long_6_2_stdcall),PTR2NUM(rb_dl_callback_long_7_2_stdcall),PTR2NUM(rb_dl_callback_long_8_2_stdcall),PTR2NUM(rb_dl_callback_long_9_2_stdcall),PTR2NUM(rb_dl_callback_long_10_2_stdcall),PTR2NUM(rb_dl_callback_long_11_2_stdcall),PTR2NUM(rb_dl_callback_long_12_2_stdcall),PTR2NUM(rb_dl_callback_long_13_2_stdcall),PTR2NUM(rb_dl_callback_long_14_2_stdcall),PTR2NUM(rb_dl_callback_long_15_2_stdcall),PTR2NUM(rb_dl_callback_long_16_2_stdcall),PTR2NUM(rb_dl_callback_long_17_2_stdcall),PTR2NUM(rb_dl_callback_long_18_2_stdcall),PTR2NUM(rb_dl_callback_long_19_2_stdcall),PTR2NUM(rb_dl_callback_long_0_3_stdcall),PTR2NUM(rb_dl_callback_long_1_3_stdcall),PTR2NUM(rb_dl_callback_long_2_3_stdcall),PTR2NUM(rb_dl_callback_long_3_3_stdcall),PTR2NUM(rb_dl_callback_long_4_3_stdcall),PTR2NUM(rb_dl_callback_long_5_3_stdcall),PTR2NUM(rb_dl_callback_long_6_3_stdcall),PTR2NUM(rb_dl_callback_long_7_3_stdcall),PTR2NUM(rb_dl_callback_long_8_3_stdcall),PTR2NUM(rb_dl_callback_long_9_3_stdcall),PTR2NUM(rb_dl_callback_long_10_3_stdcall),PTR2NUM(rb_dl_callback_long_11_3_stdcall),PTR2NUM(rb_dl_callback_long_12_3_stdcall),PTR2NUM(rb_dl_callback_long_13_3_stdcall),PTR2NUM(rb_dl_callback_long_14_3_stdcall),PTR2NUM(rb_dl_callback_long_15_3_stdcall),PTR2NUM(rb_dl_callback_long_16_3_stdcall),PTR2NUM(rb_dl_callback_long_17_3_stdcall),PTR2NUM(rb_dl_callback_long_18_3_stdcall),PTR2NUM(rb_dl_callback_long_19_3_stdcall),PTR2NUM(rb_dl_callback_long_0_4_stdcall),PTR2NUM(rb_dl_callback_long_1_4_stdcall),PTR2NUM(rb_dl_callback_long_2_4_stdcall),PTR2NUM(rb_dl_callback_long_3_4_stdcall),PTR2NUM(rb_dl_callback_long_4_4_stdcall),PTR2NUM(rb_dl_callback_long_5_4_stdcall),PTR2NUM(rb_dl_callback_long_6_4_stdcall),PTR2NUM(rb_dl_callback_long_7_4_stdcall),PTR2NUM(rb_dl_callback_long_8_4_stdcall),PTR2NUM(rb_dl_callback_long_9_4_stdcall),PTR2NUM(rb_dl_callback_long_10_4_stdcall),PTR2NUM(rb_dl_callback_long_11_4_stdcall),PTR2NUM(rb_dl_callback_long_12_4_stdcall),PTR2NUM(rb_dl_callback_long_13_4_stdcall),PTR2NUM(rb_dl_callback_long_14_4_stdcall),PTR2NUM(rb_dl_callback_long_15_4_stdcall),PTR2NUM(rb_dl_callback_long_16_4_stdcall),PTR2NUM(rb_dl_callback_long_17_4_stdcall),PTR2NUM(rb_dl_callback_long_18_4_stdcall),PTR2NUM(rb_dl_callback_long_19_4_stdcall)));
#endif
}
