#include "dl.h"

extern VALUE rb_DLCdeclCallbackAddrs, rb_DLCdeclCallbackProcs;
#ifdef FUNC_STDCALL
extern VALUE rb_DLStdcallCallbackAddrs, rb_DLStdcallCallbackProcs;
#endif
extern ID   rb_dl_cb_call;

static double
FUNC_CDECL(rb_dl_callback_double_0_0_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_0_1_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_0_2_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_0_3_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_0_4_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_1_0_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_1_1_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_1_2_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_1_3_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_1_4_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_2_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_2_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_2_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_2_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_2_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_3_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_3_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_3_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_3_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_3_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_4_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_4_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_4_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_4_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_4_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_5_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_5_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_5_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_5_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_5_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_6_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_6_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_6_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_6_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_6_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_7_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_7_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_7_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_7_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_7_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_8_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_8_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_8_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_8_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_8_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_9_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_9_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_9_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_9_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_9_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_10_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_10_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_10_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_10_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_10_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_11_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_11_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_11_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_11_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_11_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_12_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_12_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_12_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_12_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_12_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_13_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_13_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_13_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_13_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_13_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_14_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_14_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_14_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_14_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_14_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_15_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_15_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_15_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_15_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_15_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_16_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_16_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_16_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_16_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_16_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_17_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_17_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_17_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_17_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_17_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_18_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_18_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_18_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_18_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_18_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_19_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_19_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_19_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_19_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}


static double
FUNC_CDECL(rb_dl_callback_double_19_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 8), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_0_0_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_0_1_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_0_2_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_0_3_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_0_4_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_1_0_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_1_1_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_1_2_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_1_3_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_1_4_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_2_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_2_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_2_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_2_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_2_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_3_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_3_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_3_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_3_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_3_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_4_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_4_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_4_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_4_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_4_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_5_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_5_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_5_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_5_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_5_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_6_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_6_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_6_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_6_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_6_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_7_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_7_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_7_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_7_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_7_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_8_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_8_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_8_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_8_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_8_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_9_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_9_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_9_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_9_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_9_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_10_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_10_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_10_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_10_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_10_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_11_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_11_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_11_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_11_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_11_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_12_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_12_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_12_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_12_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_12_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_13_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_13_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_13_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_13_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_13_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_14_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_14_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_14_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_14_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_14_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_15_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_15_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_15_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_15_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_15_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_16_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_16_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_16_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_16_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_16_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_17_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_17_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_17_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_17_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_17_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_18_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_18_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_18_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_18_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_18_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_19_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_19_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_19_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_19_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static double
FUNC_STDCALL(rb_dl_callback_double_19_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 8), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return RFLOAT_VALUE(ret);
}
#endif

void
rb_dl_init_callbacks_8()
{
    rb_ary_push(rb_DLCdeclCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLCdeclCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_double_0_0_cdecl),PTR2NUM(rb_dl_callback_double_1_0_cdecl),PTR2NUM(rb_dl_callback_double_2_0_cdecl),PTR2NUM(rb_dl_callback_double_3_0_cdecl),PTR2NUM(rb_dl_callback_double_4_0_cdecl),PTR2NUM(rb_dl_callback_double_5_0_cdecl),PTR2NUM(rb_dl_callback_double_6_0_cdecl),PTR2NUM(rb_dl_callback_double_7_0_cdecl),PTR2NUM(rb_dl_callback_double_8_0_cdecl),PTR2NUM(rb_dl_callback_double_9_0_cdecl),PTR2NUM(rb_dl_callback_double_10_0_cdecl),PTR2NUM(rb_dl_callback_double_11_0_cdecl),PTR2NUM(rb_dl_callback_double_12_0_cdecl),PTR2NUM(rb_dl_callback_double_13_0_cdecl),PTR2NUM(rb_dl_callback_double_14_0_cdecl),PTR2NUM(rb_dl_callback_double_15_0_cdecl),PTR2NUM(rb_dl_callback_double_16_0_cdecl),PTR2NUM(rb_dl_callback_double_17_0_cdecl),PTR2NUM(rb_dl_callback_double_18_0_cdecl),PTR2NUM(rb_dl_callback_double_19_0_cdecl),PTR2NUM(rb_dl_callback_double_0_1_cdecl),PTR2NUM(rb_dl_callback_double_1_1_cdecl),PTR2NUM(rb_dl_callback_double_2_1_cdecl),PTR2NUM(rb_dl_callback_double_3_1_cdecl),PTR2NUM(rb_dl_callback_double_4_1_cdecl),PTR2NUM(rb_dl_callback_double_5_1_cdecl),PTR2NUM(rb_dl_callback_double_6_1_cdecl),PTR2NUM(rb_dl_callback_double_7_1_cdecl),PTR2NUM(rb_dl_callback_double_8_1_cdecl),PTR2NUM(rb_dl_callback_double_9_1_cdecl),PTR2NUM(rb_dl_callback_double_10_1_cdecl),PTR2NUM(rb_dl_callback_double_11_1_cdecl),PTR2NUM(rb_dl_callback_double_12_1_cdecl),PTR2NUM(rb_dl_callback_double_13_1_cdecl),PTR2NUM(rb_dl_callback_double_14_1_cdecl),PTR2NUM(rb_dl_callback_double_15_1_cdecl),PTR2NUM(rb_dl_callback_double_16_1_cdecl),PTR2NUM(rb_dl_callback_double_17_1_cdecl),PTR2NUM(rb_dl_callback_double_18_1_cdecl),PTR2NUM(rb_dl_callback_double_19_1_cdecl),PTR2NUM(rb_dl_callback_double_0_2_cdecl),PTR2NUM(rb_dl_callback_double_1_2_cdecl),PTR2NUM(rb_dl_callback_double_2_2_cdecl),PTR2NUM(rb_dl_callback_double_3_2_cdecl),PTR2NUM(rb_dl_callback_double_4_2_cdecl),PTR2NUM(rb_dl_callback_double_5_2_cdecl),PTR2NUM(rb_dl_callback_double_6_2_cdecl),PTR2NUM(rb_dl_callback_double_7_2_cdecl),PTR2NUM(rb_dl_callback_double_8_2_cdecl),PTR2NUM(rb_dl_callback_double_9_2_cdecl),PTR2NUM(rb_dl_callback_double_10_2_cdecl),PTR2NUM(rb_dl_callback_double_11_2_cdecl),PTR2NUM(rb_dl_callback_double_12_2_cdecl),PTR2NUM(rb_dl_callback_double_13_2_cdecl),PTR2NUM(rb_dl_callback_double_14_2_cdecl),PTR2NUM(rb_dl_callback_double_15_2_cdecl),PTR2NUM(rb_dl_callback_double_16_2_cdecl),PTR2NUM(rb_dl_callback_double_17_2_cdecl),PTR2NUM(rb_dl_callback_double_18_2_cdecl),PTR2NUM(rb_dl_callback_double_19_2_cdecl),PTR2NUM(rb_dl_callback_double_0_3_cdecl),PTR2NUM(rb_dl_callback_double_1_3_cdecl),PTR2NUM(rb_dl_callback_double_2_3_cdecl),PTR2NUM(rb_dl_callback_double_3_3_cdecl),PTR2NUM(rb_dl_callback_double_4_3_cdecl),PTR2NUM(rb_dl_callback_double_5_3_cdecl),PTR2NUM(rb_dl_callback_double_6_3_cdecl),PTR2NUM(rb_dl_callback_double_7_3_cdecl),PTR2NUM(rb_dl_callback_double_8_3_cdecl),PTR2NUM(rb_dl_callback_double_9_3_cdecl),PTR2NUM(rb_dl_callback_double_10_3_cdecl),PTR2NUM(rb_dl_callback_double_11_3_cdecl),PTR2NUM(rb_dl_callback_double_12_3_cdecl),PTR2NUM(rb_dl_callback_double_13_3_cdecl),PTR2NUM(rb_dl_callback_double_14_3_cdecl),PTR2NUM(rb_dl_callback_double_15_3_cdecl),PTR2NUM(rb_dl_callback_double_16_3_cdecl),PTR2NUM(rb_dl_callback_double_17_3_cdecl),PTR2NUM(rb_dl_callback_double_18_3_cdecl),PTR2NUM(rb_dl_callback_double_19_3_cdecl),PTR2NUM(rb_dl_callback_double_0_4_cdecl),PTR2NUM(rb_dl_callback_double_1_4_cdecl),PTR2NUM(rb_dl_callback_double_2_4_cdecl),PTR2NUM(rb_dl_callback_double_3_4_cdecl),PTR2NUM(rb_dl_callback_double_4_4_cdecl),PTR2NUM(rb_dl_callback_double_5_4_cdecl),PTR2NUM(rb_dl_callback_double_6_4_cdecl),PTR2NUM(rb_dl_callback_double_7_4_cdecl),PTR2NUM(rb_dl_callback_double_8_4_cdecl),PTR2NUM(rb_dl_callback_double_9_4_cdecl),PTR2NUM(rb_dl_callback_double_10_4_cdecl),PTR2NUM(rb_dl_callback_double_11_4_cdecl),PTR2NUM(rb_dl_callback_double_12_4_cdecl),PTR2NUM(rb_dl_callback_double_13_4_cdecl),PTR2NUM(rb_dl_callback_double_14_4_cdecl),PTR2NUM(rb_dl_callback_double_15_4_cdecl),PTR2NUM(rb_dl_callback_double_16_4_cdecl),PTR2NUM(rb_dl_callback_double_17_4_cdecl),PTR2NUM(rb_dl_callback_double_18_4_cdecl),PTR2NUM(rb_dl_callback_double_19_4_cdecl)));
#ifdef FUNC_STDCALL
    rb_ary_push(rb_DLStdcallCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLStdcallCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_double_0_0_stdcall),PTR2NUM(rb_dl_callback_double_1_0_stdcall),PTR2NUM(rb_dl_callback_double_2_0_stdcall),PTR2NUM(rb_dl_callback_double_3_0_stdcall),PTR2NUM(rb_dl_callback_double_4_0_stdcall),PTR2NUM(rb_dl_callback_double_5_0_stdcall),PTR2NUM(rb_dl_callback_double_6_0_stdcall),PTR2NUM(rb_dl_callback_double_7_0_stdcall),PTR2NUM(rb_dl_callback_double_8_0_stdcall),PTR2NUM(rb_dl_callback_double_9_0_stdcall),PTR2NUM(rb_dl_callback_double_10_0_stdcall),PTR2NUM(rb_dl_callback_double_11_0_stdcall),PTR2NUM(rb_dl_callback_double_12_0_stdcall),PTR2NUM(rb_dl_callback_double_13_0_stdcall),PTR2NUM(rb_dl_callback_double_14_0_stdcall),PTR2NUM(rb_dl_callback_double_15_0_stdcall),PTR2NUM(rb_dl_callback_double_16_0_stdcall),PTR2NUM(rb_dl_callback_double_17_0_stdcall),PTR2NUM(rb_dl_callback_double_18_0_stdcall),PTR2NUM(rb_dl_callback_double_19_0_stdcall),PTR2NUM(rb_dl_callback_double_0_1_stdcall),PTR2NUM(rb_dl_callback_double_1_1_stdcall),PTR2NUM(rb_dl_callback_double_2_1_stdcall),PTR2NUM(rb_dl_callback_double_3_1_stdcall),PTR2NUM(rb_dl_callback_double_4_1_stdcall),PTR2NUM(rb_dl_callback_double_5_1_stdcall),PTR2NUM(rb_dl_callback_double_6_1_stdcall),PTR2NUM(rb_dl_callback_double_7_1_stdcall),PTR2NUM(rb_dl_callback_double_8_1_stdcall),PTR2NUM(rb_dl_callback_double_9_1_stdcall),PTR2NUM(rb_dl_callback_double_10_1_stdcall),PTR2NUM(rb_dl_callback_double_11_1_stdcall),PTR2NUM(rb_dl_callback_double_12_1_stdcall),PTR2NUM(rb_dl_callback_double_13_1_stdcall),PTR2NUM(rb_dl_callback_double_14_1_stdcall),PTR2NUM(rb_dl_callback_double_15_1_stdcall),PTR2NUM(rb_dl_callback_double_16_1_stdcall),PTR2NUM(rb_dl_callback_double_17_1_stdcall),PTR2NUM(rb_dl_callback_double_18_1_stdcall),PTR2NUM(rb_dl_callback_double_19_1_stdcall),PTR2NUM(rb_dl_callback_double_0_2_stdcall),PTR2NUM(rb_dl_callback_double_1_2_stdcall),PTR2NUM(rb_dl_callback_double_2_2_stdcall),PTR2NUM(rb_dl_callback_double_3_2_stdcall),PTR2NUM(rb_dl_callback_double_4_2_stdcall),PTR2NUM(rb_dl_callback_double_5_2_stdcall),PTR2NUM(rb_dl_callback_double_6_2_stdcall),PTR2NUM(rb_dl_callback_double_7_2_stdcall),PTR2NUM(rb_dl_callback_double_8_2_stdcall),PTR2NUM(rb_dl_callback_double_9_2_stdcall),PTR2NUM(rb_dl_callback_double_10_2_stdcall),PTR2NUM(rb_dl_callback_double_11_2_stdcall),PTR2NUM(rb_dl_callback_double_12_2_stdcall),PTR2NUM(rb_dl_callback_double_13_2_stdcall),PTR2NUM(rb_dl_callback_double_14_2_stdcall),PTR2NUM(rb_dl_callback_double_15_2_stdcall),PTR2NUM(rb_dl_callback_double_16_2_stdcall),PTR2NUM(rb_dl_callback_double_17_2_stdcall),PTR2NUM(rb_dl_callback_double_18_2_stdcall),PTR2NUM(rb_dl_callback_double_19_2_stdcall),PTR2NUM(rb_dl_callback_double_0_3_stdcall),PTR2NUM(rb_dl_callback_double_1_3_stdcall),PTR2NUM(rb_dl_callback_double_2_3_stdcall),PTR2NUM(rb_dl_callback_double_3_3_stdcall),PTR2NUM(rb_dl_callback_double_4_3_stdcall),PTR2NUM(rb_dl_callback_double_5_3_stdcall),PTR2NUM(rb_dl_callback_double_6_3_stdcall),PTR2NUM(rb_dl_callback_double_7_3_stdcall),PTR2NUM(rb_dl_callback_double_8_3_stdcall),PTR2NUM(rb_dl_callback_double_9_3_stdcall),PTR2NUM(rb_dl_callback_double_10_3_stdcall),PTR2NUM(rb_dl_callback_double_11_3_stdcall),PTR2NUM(rb_dl_callback_double_12_3_stdcall),PTR2NUM(rb_dl_callback_double_13_3_stdcall),PTR2NUM(rb_dl_callback_double_14_3_stdcall),PTR2NUM(rb_dl_callback_double_15_3_stdcall),PTR2NUM(rb_dl_callback_double_16_3_stdcall),PTR2NUM(rb_dl_callback_double_17_3_stdcall),PTR2NUM(rb_dl_callback_double_18_3_stdcall),PTR2NUM(rb_dl_callback_double_19_3_stdcall),PTR2NUM(rb_dl_callback_double_0_4_stdcall),PTR2NUM(rb_dl_callback_double_1_4_stdcall),PTR2NUM(rb_dl_callback_double_2_4_stdcall),PTR2NUM(rb_dl_callback_double_3_4_stdcall),PTR2NUM(rb_dl_callback_double_4_4_stdcall),PTR2NUM(rb_dl_callback_double_5_4_stdcall),PTR2NUM(rb_dl_callback_double_6_4_stdcall),PTR2NUM(rb_dl_callback_double_7_4_stdcall),PTR2NUM(rb_dl_callback_double_8_4_stdcall),PTR2NUM(rb_dl_callback_double_9_4_stdcall),PTR2NUM(rb_dl_callback_double_10_4_stdcall),PTR2NUM(rb_dl_callback_double_11_4_stdcall),PTR2NUM(rb_dl_callback_double_12_4_stdcall),PTR2NUM(rb_dl_callback_double_13_4_stdcall),PTR2NUM(rb_dl_callback_double_14_4_stdcall),PTR2NUM(rb_dl_callback_double_15_4_stdcall),PTR2NUM(rb_dl_callback_double_16_4_stdcall),PTR2NUM(rb_dl_callback_double_17_4_stdcall),PTR2NUM(rb_dl_callback_double_18_4_stdcall),PTR2NUM(rb_dl_callback_double_19_4_stdcall)));
#endif
}
