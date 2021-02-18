#include "dl.h"

extern VALUE rb_DLCdeclCallbackAddrs, rb_DLCdeclCallbackProcs;
#ifdef FUNC_STDCALL
extern VALUE rb_DLStdcallCallbackAddrs, rb_DLStdcallCallbackProcs;
#endif
extern ID   rb_dl_cb_call;

static float
FUNC_CDECL(rb_dl_callback_float_0_0_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_0_1_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_0_2_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_0_3_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_0_4_cdecl)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_1_0_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_1_1_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_1_2_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_1_3_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_1_4_cdecl)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_2_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_2_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_2_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_2_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_2_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_3_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_3_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_3_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_3_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_3_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_4_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_4_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_4_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_4_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_4_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_5_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_5_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_5_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_5_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_5_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_6_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_6_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_6_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_6_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_6_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_7_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_7_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_7_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_7_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_7_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_8_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_8_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_8_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_8_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_8_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_9_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_9_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_9_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_9_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_9_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_10_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_10_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_10_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_10_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_10_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_11_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_11_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_11_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_11_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_11_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_12_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_12_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_12_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_12_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_12_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_13_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_13_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_13_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_13_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_13_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_14_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_14_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_14_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_14_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_14_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_15_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_15_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_15_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_15_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_15_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_16_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_16_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_16_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_16_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_16_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_17_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_17_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_17_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_17_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_17_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_18_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_18_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_18_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_18_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_18_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_19_0_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_19_1_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_19_2_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_19_3_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}


static float
FUNC_CDECL(rb_dl_callback_float_19_4_cdecl)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLCdeclCallbackProcs, 7), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_0_0_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 0);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_0_1_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 20);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_0_2_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 40);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_0_3_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 60);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_0_4_stdcall)()
{
    VALUE ret, cb;

    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 80);
    ret = rb_funcall2(cb, rb_dl_cb_call, 0, NULL);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_1_0_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 1);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_1_1_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 21);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_1_2_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 41);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_1_3_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 61);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_1_4_stdcall)(DLSTACK_TYPE stack0)
{
    VALUE ret, cb, args[1];
    args[0] = LONG2NUM(stack0);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 81);
    ret = rb_funcall2(cb, rb_dl_cb_call, 1, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_2_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 2);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_2_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 22);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_2_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 42);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_2_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 62);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_2_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1)
{
    VALUE ret, cb, args[2];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 82);
    ret = rb_funcall2(cb, rb_dl_cb_call, 2, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_3_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 3);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_3_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 23);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_3_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 43);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_3_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 63);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_3_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2)
{
    VALUE ret, cb, args[3];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 83);
    ret = rb_funcall2(cb, rb_dl_cb_call, 3, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_4_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 4);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_4_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 24);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_4_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 44);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_4_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 64);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_4_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3)
{
    VALUE ret, cb, args[4];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 84);
    ret = rb_funcall2(cb, rb_dl_cb_call, 4, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_5_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 5);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_5_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 25);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_5_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 45);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_5_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 65);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_5_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4)
{
    VALUE ret, cb, args[5];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 85);
    ret = rb_funcall2(cb, rb_dl_cb_call, 5, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_6_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 6);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_6_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 26);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_6_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 46);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_6_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 66);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_6_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5)
{
    VALUE ret, cb, args[6];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 86);
    ret = rb_funcall2(cb, rb_dl_cb_call, 6, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_7_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 7);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_7_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 27);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_7_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 47);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_7_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 67);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_7_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6)
{
    VALUE ret, cb, args[7];
    args[0] = LONG2NUM(stack0);
    args[1] = LONG2NUM(stack1);
    args[2] = LONG2NUM(stack2);
    args[3] = LONG2NUM(stack3);
    args[4] = LONG2NUM(stack4);
    args[5] = LONG2NUM(stack5);
    args[6] = LONG2NUM(stack6);
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 87);
    ret = rb_funcall2(cb, rb_dl_cb_call, 7, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_8_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 8);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_8_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 28);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_8_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 48);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_8_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 68);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_8_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 88);
    ret = rb_funcall2(cb, rb_dl_cb_call, 8, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_9_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 9);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_9_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 29);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_9_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 49);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_9_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 69);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_9_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 89);
    ret = rb_funcall2(cb, rb_dl_cb_call, 9, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_10_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 10);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_10_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 30);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_10_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 50);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_10_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 70);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_10_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 90);
    ret = rb_funcall2(cb, rb_dl_cb_call, 10, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_11_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 11);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_11_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 31);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_11_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 51);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_11_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 71);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_11_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 91);
    ret = rb_funcall2(cb, rb_dl_cb_call, 11, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_12_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 12);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_12_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 32);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_12_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 52);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_12_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 72);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_12_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 92);
    ret = rb_funcall2(cb, rb_dl_cb_call, 12, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_13_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 13);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_13_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 33);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_13_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 53);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_13_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 73);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_13_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 93);
    ret = rb_funcall2(cb, rb_dl_cb_call, 13, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_14_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 14);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_14_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 34);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_14_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 54);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_14_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 74);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_14_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 94);
    ret = rb_funcall2(cb, rb_dl_cb_call, 14, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_15_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 15);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_15_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 35);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_15_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 55);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_15_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 75);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_15_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 95);
    ret = rb_funcall2(cb, rb_dl_cb_call, 15, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_16_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 16);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_16_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 36);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_16_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 56);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_16_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 76);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_16_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 96);
    ret = rb_funcall2(cb, rb_dl_cb_call, 16, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_17_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 17);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_17_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 37);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_17_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 57);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_17_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 77);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_17_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 97);
    ret = rb_funcall2(cb, rb_dl_cb_call, 17, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_18_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 18);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_18_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 38);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_18_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 58);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_18_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 78);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_18_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 98);
    ret = rb_funcall2(cb, rb_dl_cb_call, 18, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_19_0_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 19);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_19_1_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 39);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_19_2_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 59);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_19_3_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 79);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif


#ifdef FUNC_STDCALL
static float
FUNC_STDCALL(rb_dl_callback_float_19_4_stdcall)(DLSTACK_TYPE stack0, DLSTACK_TYPE stack1, DLSTACK_TYPE stack2, DLSTACK_TYPE stack3, DLSTACK_TYPE stack4, DLSTACK_TYPE stack5, DLSTACK_TYPE stack6, DLSTACK_TYPE stack7, DLSTACK_TYPE stack8, DLSTACK_TYPE stack9, DLSTACK_TYPE stack10, DLSTACK_TYPE stack11, DLSTACK_TYPE stack12, DLSTACK_TYPE stack13, DLSTACK_TYPE stack14, DLSTACK_TYPE stack15, DLSTACK_TYPE stack16, DLSTACK_TYPE stack17, DLSTACK_TYPE stack18)
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
    cb = rb_ary_entry(rb_ary_entry(rb_DLStdcallCallbackProcs, 7), 99);
    ret = rb_funcall2(cb, rb_dl_cb_call, 19, args);
    return (float)RFLOAT_VALUE(ret);
}
#endif

void
rb_dl_init_callbacks_7()
{
    rb_ary_push(rb_DLCdeclCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLCdeclCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_float_0_0_cdecl),PTR2NUM(rb_dl_callback_float_1_0_cdecl),PTR2NUM(rb_dl_callback_float_2_0_cdecl),PTR2NUM(rb_dl_callback_float_3_0_cdecl),PTR2NUM(rb_dl_callback_float_4_0_cdecl),PTR2NUM(rb_dl_callback_float_5_0_cdecl),PTR2NUM(rb_dl_callback_float_6_0_cdecl),PTR2NUM(rb_dl_callback_float_7_0_cdecl),PTR2NUM(rb_dl_callback_float_8_0_cdecl),PTR2NUM(rb_dl_callback_float_9_0_cdecl),PTR2NUM(rb_dl_callback_float_10_0_cdecl),PTR2NUM(rb_dl_callback_float_11_0_cdecl),PTR2NUM(rb_dl_callback_float_12_0_cdecl),PTR2NUM(rb_dl_callback_float_13_0_cdecl),PTR2NUM(rb_dl_callback_float_14_0_cdecl),PTR2NUM(rb_dl_callback_float_15_0_cdecl),PTR2NUM(rb_dl_callback_float_16_0_cdecl),PTR2NUM(rb_dl_callback_float_17_0_cdecl),PTR2NUM(rb_dl_callback_float_18_0_cdecl),PTR2NUM(rb_dl_callback_float_19_0_cdecl),PTR2NUM(rb_dl_callback_float_0_1_cdecl),PTR2NUM(rb_dl_callback_float_1_1_cdecl),PTR2NUM(rb_dl_callback_float_2_1_cdecl),PTR2NUM(rb_dl_callback_float_3_1_cdecl),PTR2NUM(rb_dl_callback_float_4_1_cdecl),PTR2NUM(rb_dl_callback_float_5_1_cdecl),PTR2NUM(rb_dl_callback_float_6_1_cdecl),PTR2NUM(rb_dl_callback_float_7_1_cdecl),PTR2NUM(rb_dl_callback_float_8_1_cdecl),PTR2NUM(rb_dl_callback_float_9_1_cdecl),PTR2NUM(rb_dl_callback_float_10_1_cdecl),PTR2NUM(rb_dl_callback_float_11_1_cdecl),PTR2NUM(rb_dl_callback_float_12_1_cdecl),PTR2NUM(rb_dl_callback_float_13_1_cdecl),PTR2NUM(rb_dl_callback_float_14_1_cdecl),PTR2NUM(rb_dl_callback_float_15_1_cdecl),PTR2NUM(rb_dl_callback_float_16_1_cdecl),PTR2NUM(rb_dl_callback_float_17_1_cdecl),PTR2NUM(rb_dl_callback_float_18_1_cdecl),PTR2NUM(rb_dl_callback_float_19_1_cdecl),PTR2NUM(rb_dl_callback_float_0_2_cdecl),PTR2NUM(rb_dl_callback_float_1_2_cdecl),PTR2NUM(rb_dl_callback_float_2_2_cdecl),PTR2NUM(rb_dl_callback_float_3_2_cdecl),PTR2NUM(rb_dl_callback_float_4_2_cdecl),PTR2NUM(rb_dl_callback_float_5_2_cdecl),PTR2NUM(rb_dl_callback_float_6_2_cdecl),PTR2NUM(rb_dl_callback_float_7_2_cdecl),PTR2NUM(rb_dl_callback_float_8_2_cdecl),PTR2NUM(rb_dl_callback_float_9_2_cdecl),PTR2NUM(rb_dl_callback_float_10_2_cdecl),PTR2NUM(rb_dl_callback_float_11_2_cdecl),PTR2NUM(rb_dl_callback_float_12_2_cdecl),PTR2NUM(rb_dl_callback_float_13_2_cdecl),PTR2NUM(rb_dl_callback_float_14_2_cdecl),PTR2NUM(rb_dl_callback_float_15_2_cdecl),PTR2NUM(rb_dl_callback_float_16_2_cdecl),PTR2NUM(rb_dl_callback_float_17_2_cdecl),PTR2NUM(rb_dl_callback_float_18_2_cdecl),PTR2NUM(rb_dl_callback_float_19_2_cdecl),PTR2NUM(rb_dl_callback_float_0_3_cdecl),PTR2NUM(rb_dl_callback_float_1_3_cdecl),PTR2NUM(rb_dl_callback_float_2_3_cdecl),PTR2NUM(rb_dl_callback_float_3_3_cdecl),PTR2NUM(rb_dl_callback_float_4_3_cdecl),PTR2NUM(rb_dl_callback_float_5_3_cdecl),PTR2NUM(rb_dl_callback_float_6_3_cdecl),PTR2NUM(rb_dl_callback_float_7_3_cdecl),PTR2NUM(rb_dl_callback_float_8_3_cdecl),PTR2NUM(rb_dl_callback_float_9_3_cdecl),PTR2NUM(rb_dl_callback_float_10_3_cdecl),PTR2NUM(rb_dl_callback_float_11_3_cdecl),PTR2NUM(rb_dl_callback_float_12_3_cdecl),PTR2NUM(rb_dl_callback_float_13_3_cdecl),PTR2NUM(rb_dl_callback_float_14_3_cdecl),PTR2NUM(rb_dl_callback_float_15_3_cdecl),PTR2NUM(rb_dl_callback_float_16_3_cdecl),PTR2NUM(rb_dl_callback_float_17_3_cdecl),PTR2NUM(rb_dl_callback_float_18_3_cdecl),PTR2NUM(rb_dl_callback_float_19_3_cdecl),PTR2NUM(rb_dl_callback_float_0_4_cdecl),PTR2NUM(rb_dl_callback_float_1_4_cdecl),PTR2NUM(rb_dl_callback_float_2_4_cdecl),PTR2NUM(rb_dl_callback_float_3_4_cdecl),PTR2NUM(rb_dl_callback_float_4_4_cdecl),PTR2NUM(rb_dl_callback_float_5_4_cdecl),PTR2NUM(rb_dl_callback_float_6_4_cdecl),PTR2NUM(rb_dl_callback_float_7_4_cdecl),PTR2NUM(rb_dl_callback_float_8_4_cdecl),PTR2NUM(rb_dl_callback_float_9_4_cdecl),PTR2NUM(rb_dl_callback_float_10_4_cdecl),PTR2NUM(rb_dl_callback_float_11_4_cdecl),PTR2NUM(rb_dl_callback_float_12_4_cdecl),PTR2NUM(rb_dl_callback_float_13_4_cdecl),PTR2NUM(rb_dl_callback_float_14_4_cdecl),PTR2NUM(rb_dl_callback_float_15_4_cdecl),PTR2NUM(rb_dl_callback_float_16_4_cdecl),PTR2NUM(rb_dl_callback_float_17_4_cdecl),PTR2NUM(rb_dl_callback_float_18_4_cdecl),PTR2NUM(rb_dl_callback_float_19_4_cdecl)));
#ifdef FUNC_STDCALL
    rb_ary_push(rb_DLStdcallCallbackProcs, rb_ary_new3(100,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil,Qnil));
    rb_ary_push(rb_DLStdcallCallbackAddrs, rb_ary_new3(100,PTR2NUM(rb_dl_callback_float_0_0_stdcall),PTR2NUM(rb_dl_callback_float_1_0_stdcall),PTR2NUM(rb_dl_callback_float_2_0_stdcall),PTR2NUM(rb_dl_callback_float_3_0_stdcall),PTR2NUM(rb_dl_callback_float_4_0_stdcall),PTR2NUM(rb_dl_callback_float_5_0_stdcall),PTR2NUM(rb_dl_callback_float_6_0_stdcall),PTR2NUM(rb_dl_callback_float_7_0_stdcall),PTR2NUM(rb_dl_callback_float_8_0_stdcall),PTR2NUM(rb_dl_callback_float_9_0_stdcall),PTR2NUM(rb_dl_callback_float_10_0_stdcall),PTR2NUM(rb_dl_callback_float_11_0_stdcall),PTR2NUM(rb_dl_callback_float_12_0_stdcall),PTR2NUM(rb_dl_callback_float_13_0_stdcall),PTR2NUM(rb_dl_callback_float_14_0_stdcall),PTR2NUM(rb_dl_callback_float_15_0_stdcall),PTR2NUM(rb_dl_callback_float_16_0_stdcall),PTR2NUM(rb_dl_callback_float_17_0_stdcall),PTR2NUM(rb_dl_callback_float_18_0_stdcall),PTR2NUM(rb_dl_callback_float_19_0_stdcall),PTR2NUM(rb_dl_callback_float_0_1_stdcall),PTR2NUM(rb_dl_callback_float_1_1_stdcall),PTR2NUM(rb_dl_callback_float_2_1_stdcall),PTR2NUM(rb_dl_callback_float_3_1_stdcall),PTR2NUM(rb_dl_callback_float_4_1_stdcall),PTR2NUM(rb_dl_callback_float_5_1_stdcall),PTR2NUM(rb_dl_callback_float_6_1_stdcall),PTR2NUM(rb_dl_callback_float_7_1_stdcall),PTR2NUM(rb_dl_callback_float_8_1_stdcall),PTR2NUM(rb_dl_callback_float_9_1_stdcall),PTR2NUM(rb_dl_callback_float_10_1_stdcall),PTR2NUM(rb_dl_callback_float_11_1_stdcall),PTR2NUM(rb_dl_callback_float_12_1_stdcall),PTR2NUM(rb_dl_callback_float_13_1_stdcall),PTR2NUM(rb_dl_callback_float_14_1_stdcall),PTR2NUM(rb_dl_callback_float_15_1_stdcall),PTR2NUM(rb_dl_callback_float_16_1_stdcall),PTR2NUM(rb_dl_callback_float_17_1_stdcall),PTR2NUM(rb_dl_callback_float_18_1_stdcall),PTR2NUM(rb_dl_callback_float_19_1_stdcall),PTR2NUM(rb_dl_callback_float_0_2_stdcall),PTR2NUM(rb_dl_callback_float_1_2_stdcall),PTR2NUM(rb_dl_callback_float_2_2_stdcall),PTR2NUM(rb_dl_callback_float_3_2_stdcall),PTR2NUM(rb_dl_callback_float_4_2_stdcall),PTR2NUM(rb_dl_callback_float_5_2_stdcall),PTR2NUM(rb_dl_callback_float_6_2_stdcall),PTR2NUM(rb_dl_callback_float_7_2_stdcall),PTR2NUM(rb_dl_callback_float_8_2_stdcall),PTR2NUM(rb_dl_callback_float_9_2_stdcall),PTR2NUM(rb_dl_callback_float_10_2_stdcall),PTR2NUM(rb_dl_callback_float_11_2_stdcall),PTR2NUM(rb_dl_callback_float_12_2_stdcall),PTR2NUM(rb_dl_callback_float_13_2_stdcall),PTR2NUM(rb_dl_callback_float_14_2_stdcall),PTR2NUM(rb_dl_callback_float_15_2_stdcall),PTR2NUM(rb_dl_callback_float_16_2_stdcall),PTR2NUM(rb_dl_callback_float_17_2_stdcall),PTR2NUM(rb_dl_callback_float_18_2_stdcall),PTR2NUM(rb_dl_callback_float_19_2_stdcall),PTR2NUM(rb_dl_callback_float_0_3_stdcall),PTR2NUM(rb_dl_callback_float_1_3_stdcall),PTR2NUM(rb_dl_callback_float_2_3_stdcall),PTR2NUM(rb_dl_callback_float_3_3_stdcall),PTR2NUM(rb_dl_callback_float_4_3_stdcall),PTR2NUM(rb_dl_callback_float_5_3_stdcall),PTR2NUM(rb_dl_callback_float_6_3_stdcall),PTR2NUM(rb_dl_callback_float_7_3_stdcall),PTR2NUM(rb_dl_callback_float_8_3_stdcall),PTR2NUM(rb_dl_callback_float_9_3_stdcall),PTR2NUM(rb_dl_callback_float_10_3_stdcall),PTR2NUM(rb_dl_callback_float_11_3_stdcall),PTR2NUM(rb_dl_callback_float_12_3_stdcall),PTR2NUM(rb_dl_callback_float_13_3_stdcall),PTR2NUM(rb_dl_callback_float_14_3_stdcall),PTR2NUM(rb_dl_callback_float_15_3_stdcall),PTR2NUM(rb_dl_callback_float_16_3_stdcall),PTR2NUM(rb_dl_callback_float_17_3_stdcall),PTR2NUM(rb_dl_callback_float_18_3_stdcall),PTR2NUM(rb_dl_callback_float_19_3_stdcall),PTR2NUM(rb_dl_callback_float_0_4_stdcall),PTR2NUM(rb_dl_callback_float_1_4_stdcall),PTR2NUM(rb_dl_callback_float_2_4_stdcall),PTR2NUM(rb_dl_callback_float_3_4_stdcall),PTR2NUM(rb_dl_callback_float_4_4_stdcall),PTR2NUM(rb_dl_callback_float_5_4_stdcall),PTR2NUM(rb_dl_callback_float_6_4_stdcall),PTR2NUM(rb_dl_callback_float_7_4_stdcall),PTR2NUM(rb_dl_callback_float_8_4_stdcall),PTR2NUM(rb_dl_callback_float_9_4_stdcall),PTR2NUM(rb_dl_callback_float_10_4_stdcall),PTR2NUM(rb_dl_callback_float_11_4_stdcall),PTR2NUM(rb_dl_callback_float_12_4_stdcall),PTR2NUM(rb_dl_callback_float_13_4_stdcall),PTR2NUM(rb_dl_callback_float_14_4_stdcall),PTR2NUM(rb_dl_callback_float_15_4_stdcall),PTR2NUM(rb_dl_callback_float_16_4_stdcall),PTR2NUM(rb_dl_callback_float_17_4_stdcall),PTR2NUM(rb_dl_callback_float_18_4_stdcall),PTR2NUM(rb_dl_callback_float_19_4_stdcall)));
#endif
}
