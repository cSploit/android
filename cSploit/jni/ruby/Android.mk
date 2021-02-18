LOCAL_PATH := $(call my-dir)

#original path: libruby-static.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fvisibility=hidden -DRUBY_EXPORT 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)\
        $(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
        $(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
        dln.c\
        encoding.c\
        version.c\
        array.c\
        bignum.c\
        class.c\
        compar.c\
        complex.c\
        dir.c\
        dln_find.c\
        enum.c\
        enumerator.c\
        error.c\
        eval.c\
        load.c\
        proc.c\
        file.c\
        gc.c\
        hash.c\
        inits.c\
        io.c\
        marshal.c\
        math.c\
        node.c\
        numeric.c\
        object.c\
        pack.c\
        parse.c\
        process.c\
        random.c\
        range.c\
        rational.c\
        re.c\
        regcomp.c\
        regenc.c\
        regerror.c\
        regexec.c\
        regparse.c\
        regsyntax.c\
        ruby.c\
        safe.c\
        signal.c\
        sprintf.c\
        st.c\
        strftime.c\
        string.c\
        struct.c\
        time.c\
        transcode.c\
        util.c\
        variable.c\
        compile.c\
        debug.c\
        iseq.c\
        vm.c\
        vm_dump.c\
        thread.c\
        cont.c\
        enc/ascii.c\
        enc/us_ascii.c\
        enc/unicode.c\
        enc/utf_8.c\
        newline.c\
        missing/memcmp.c\
        missing/crypt.c\
        missing/isinf.c\
        missing/setproctitle.c\
        addr2line.c\
        prelude.c\
        dmyext.c
LOCAL_MODULE := libruby-static

include $(BUILD_STATIC_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/encdb.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/encdb.c
LOCAL_MODULE := RUBY_enc_encdb

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/big5.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/big5.c
LOCAL_MODULE := RUBY_enc_big5

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/cp949.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/cp949.c
LOCAL_MODULE := RUBY_enc_cp949

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/emacs_mule.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/emacs_mule.c
LOCAL_MODULE := RUBY_enc_emacs_mule

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/euc_jp.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/euc_jp.c
LOCAL_MODULE := RUBY_enc_euc_jp

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/euc_kr.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/euc_kr.c
LOCAL_MODULE := RUBY_enc_euc_kr

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/euc_tw.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/euc_tw.c
LOCAL_MODULE := RUBY_enc_euc_tw

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/gb2312.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/gb2312.c
LOCAL_MODULE := RUBY_enc_gb2312

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/gb18030.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/gb18030.c
LOCAL_MODULE := RUBY_enc_gb18030

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/gbk.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/gbk.c
LOCAL_MODULE := RUBY_enc_gbk

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_1.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_1.c
LOCAL_MODULE := RUBY_enc_iso_8859_1

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_2.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_2.c
LOCAL_MODULE := RUBY_enc_iso_8859_2

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_3.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_3.c
LOCAL_MODULE := RUBY_enc_iso_8859_3

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_4.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_4.c
LOCAL_MODULE := RUBY_enc_iso_8859_4

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_5.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_5.c
LOCAL_MODULE := RUBY_enc_iso_8859_5

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_6.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_6.c
LOCAL_MODULE := RUBY_enc_iso_8859_6

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_7.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_7.c
LOCAL_MODULE := RUBY_enc_iso_8859_7

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_8.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_8.c
LOCAL_MODULE := RUBY_enc_iso_8859_8

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_9.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_9.c
LOCAL_MODULE := RUBY_enc_iso_8859_9

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_10.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_10.c
LOCAL_MODULE := RUBY_enc_iso_8859_10

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_11.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_11.c
LOCAL_MODULE := RUBY_enc_iso_8859_11

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_13.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_13.c
LOCAL_MODULE := RUBY_enc_iso_8859_13

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_14.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_14.c
LOCAL_MODULE := RUBY_enc_iso_8859_14

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_15.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_15.c
LOCAL_MODULE := RUBY_enc_iso_8859_15

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/iso_8859_16.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/iso_8859_16.c
LOCAL_MODULE := RUBY_enc_iso_8859_16

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/koi8_r.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/koi8_r.c
LOCAL_MODULE := RUBY_enc_koi8_r

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/koi8_u.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/koi8_u.c
LOCAL_MODULE := RUBY_enc_koi8_u

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/shift_jis.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/shift_jis.c
LOCAL_MODULE := RUBY_enc_shift_jis

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/utf_16be.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/utf_16be.c
LOCAL_MODULE := RUBY_enc_utf_16be

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/utf_16le.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/utf_16le.c
LOCAL_MODULE := RUBY_enc_utf_16le

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/utf_32be.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/utf_32be.c
LOCAL_MODULE := RUBY_enc_utf_32be

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/utf_32le.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/utf_32le.c
LOCAL_MODULE := RUBY_enc_utf_32le

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/windows_1251.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/windows_1251.c
LOCAL_MODULE := RUBY_enc_windows_1251

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/transdb.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/transdb.c
LOCAL_MODULE := RUBY_enc_trans_transdb

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/big5.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/big5.c
LOCAL_MODULE := RUBY_enc_trans_big5

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/chinese.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/chinese.c
LOCAL_MODULE := RUBY_enc_trans_chinese

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/emoji.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/emoji.c
LOCAL_MODULE := RUBY_enc_trans_emoji

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/emoji_iso2022_kddi.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/emoji_iso2022_kddi.c
LOCAL_MODULE := RUBY_enc_trans_emoji_iso2022_kddi

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/emoji_sjis_docomo.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/emoji_sjis_docomo.c
LOCAL_MODULE := RUBY_enc_trans_emoji_sjis_docomo

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/emoji_sjis_kddi.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/emoji_sjis_kddi.c
LOCAL_MODULE := RUBY_enc_trans_emoji_sjis_kddi

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/emoji_sjis_softbank.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/emoji_sjis_softbank.c
LOCAL_MODULE := RUBY_enc_trans_emoji_sjis_softbank

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/escape.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/escape.c
LOCAL_MODULE := RUBY_enc_trans_escape

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/gb18030.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/gb18030.c
LOCAL_MODULE := RUBY_enc_trans_gb18030

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/gbk.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/gbk.c
LOCAL_MODULE := RUBY_enc_trans_gbk

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/iso2022.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/iso2022.c
LOCAL_MODULE := RUBY_enc_trans_iso2022

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/japanese.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/japanese.c
LOCAL_MODULE := RUBY_enc_trans_japanese

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/japanese_euc.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/japanese_euc.c
LOCAL_MODULE := RUBY_enc_trans_japanese_euc

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/japanese_sjis.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/japanese_sjis.c
LOCAL_MODULE := RUBY_enc_trans_japanese_sjis

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/korean.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/korean.c
LOCAL_MODULE := RUBY_enc_trans_korean

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/single_byte.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/single_byte.c
LOCAL_MODULE := RUBY_enc_trans_single_byte

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/utf8_mac.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/utf8_mac.c
LOCAL_MODULE := RUBY_enc_trans_utf8_mac

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/enc/trans/utf_16_32.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DONIG_ENC_REGISTER=rb_enc_register 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	enc/trans/utf_16_32.c
LOCAL_MODULE := RUBY_enc_trans_utf_16_32

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/bigdecimal.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/bigdecimal\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/bigdecimal/bigdecimal.c
LOCAL_MODULE := RUBY_bigdecimal

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/continuation.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/continuation\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/continuation/continuation.c
LOCAL_MODULE := RUBY_continuation

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/coverage.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/coverage\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	ruby
LOCAL_SRC_FILES:= \
	ext/coverage/coverage.c
LOCAL_MODULE := RUBY_coverage

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/curses.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/curses\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	external/ncurses/include
LOCAL_SRC_FILES:= \
	ext/curses/curses.c
LOCAL_STATIC_LIBRARIES:= \
	libncurses\
	libtinfo
LOCAL_MODULE := RUBY_curses

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/date_core.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/date\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/date/date_parse.c\
	ext/date/date_strptime.c\
	ext/date/date_core.c\
	ext/date/date_strftime.c
LOCAL_MODULE := RUBY_date_core

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/digest.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/digest\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/digest/digest.c
LOCAL_MODULE := RUBY_digest

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/digest/bubblebabble.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/digest/bubblebabble\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/ext/digest
LOCAL_SRC_FILES:= \
	ext/digest/bubblebabble/bubblebabble.c
LOCAL_MODULE := RUBY_digest_bubblebabble

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/digest/md5.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/digest/md5\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/ext/digest\
	external/openssl/include
LOCAL_SRC_FILES:= \
	ext/digest/md5/md5init.c\
	ext/digest/md5/md5ossl.c
LOCAL_SHARED_LIBRARIES:= \
	libcrypto
LOCAL_MODULE := RUBY_digest_md5

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/digest/rmd160.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/digest/rmd160\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/ext/digest\
	external/openssl/include
LOCAL_SRC_FILES:= \
	ext/digest/rmd160/rmd160init.c\
	ext/digest/rmd160/rmd160ossl.c

# RMD160 missing on certain devices ( API >= 21 )
LOCAL_STATIC_LIBRARIES:= libcrypto_static

LOCAL_MODULE := RUBY_digest_rmd160

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/digest/sha1.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/digest/sha1\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/ext/digest\
	external/openssl/include
LOCAL_SRC_FILES:= \
	ext/digest/sha1/sha1init.c\
	ext/digest/sha1/sha1ossl.c
LOCAL_SHARED_LIBRARIES:= \
	libcrypto
LOCAL_MODULE := RUBY_digest_sha1

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/digest/sha2.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/digest/sha2\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/ext/digest\
	external/openssl/include
LOCAL_SRC_FILES:= \
	ext/digest/sha2/sha2init.c\
	ext/digest/sha2/sha2ossl.c
LOCAL_SHARED_LIBRARIES:= \
	libcrypto
LOCAL_MODULE := RUBY_digest_sha2

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/dl.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" -fno-defer-pop \
-fno-omit-frame-pointer 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/dl\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/dl/handle.c\
	ext/dl/cptr.c\
	ext/dl/dl.c\
	ext/dl/cfunc.c
LOCAL_MODULE := RUBY_dl

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/dl/callback.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/dl/callback\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/ext/dl
LOCAL_SRC_FILES:= \
	ext/dl/callback/callback.c\
	ext/dl/callback/callback-0.c\
	ext/dl/callback/callback-1.c\
	ext/dl/callback/callback-2.c\
	ext/dl/callback/callback-3.c\
	ext/dl/callback/callback-4.c\
	ext/dl/callback/callback-5.c\
	ext/dl/callback/callback-6.c\
	ext/dl/callback/callback-7.c\
	ext/dl/callback/callback-8.c
LOCAL_MODULE := RUBY_dl_callback

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/etc.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/etc\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/etc/etc.c
LOCAL_MODULE := RUBY_etc

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/fcntl.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/fcntl\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/fcntl/fcntl.c
LOCAL_MODULE := RUBY_fcntl

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/fiber.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/fiber\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/fiber/fiber.c
LOCAL_MODULE := RUBY_fiber

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/iconv.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/iconv\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	external/libiconv/include
LOCAL_SRC_FILES:= \
	ext/iconv/iconv.c
LOCAL_STATIC_LIBRARIES:= \
	libiconv
LOCAL_MODULE := RUBY_iconv

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/io/console.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/io/console\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/io/console/console.c
LOCAL_MODULE := RUBY_io_console

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/io/nonblock.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/io/nonblock\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/io/nonblock/nonblock.c
LOCAL_MODULE := RUBY_io_nonblock

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/io/wait.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/io/wait\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/io/wait/wait.c
LOCAL_MODULE := RUBY_io_wait

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/json/ext/generator.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/json/generator\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/json/generator/generator.c
LOCAL_MODULE := RUBY_json_ext_generator

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/json/ext/parser.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/json/parser\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/json/parser/parser.c
LOCAL_MODULE := RUBY_json_ext_parser

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/mathn/complex.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/mathn/complex\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/mathn/complex/complex.c
LOCAL_MODULE := RUBY_mathn_complex

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/mathn/rational.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/mathn/rational\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/mathn/rational/rational.c
LOCAL_MODULE := RUBY_mathn_rational

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/nkf.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/nkf\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/nkf/nkf.c
LOCAL_MODULE := RUBY_nkf

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/objspace.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/objspace\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	ruby
LOCAL_SRC_FILES:= \
	ext/objspace/objspace.c
LOCAL_MODULE := RUBY_objspace

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/openssl.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/openssl\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	external/openssl/include
LOCAL_SRC_FILES:= \
	ext/openssl/openssl_missing.c\
	ext/openssl/ossl_x509crl.c\
	ext/openssl/ossl_config.c\
	ext/openssl/ossl_x509req.c\
	ext/openssl/ossl_cipher.c\
	ext/openssl/ossl_asn1.c\
	ext/openssl/ossl_ssl_session.c\
	ext/openssl/ossl_x509.c\
	ext/openssl/ossl.c\
	ext/openssl/ossl_pkey_dsa.c\
	ext/openssl/ossl_pkcs7.c\
	ext/openssl/ossl_x509ext.c\
	ext/openssl/ossl_bn.c\
	ext/openssl/ossl_pkey_ec.c\
	ext/openssl/ossl_bio.c\
	ext/openssl/ossl_pkcs5.c\
	ext/openssl/ossl_rand.c\
	ext/openssl/ossl_pkey.c\
	ext/openssl/ossl_x509name.c\
	ext/openssl/ossl_x509store.c\
	ext/openssl/ossl_ocsp.c\
	ext/openssl/ossl_hmac.c\
	ext/openssl/ossl_digest.c\
	ext/openssl/ossl_ssl.c\
	ext/openssl/ossl_x509attr.c\
	ext/openssl/ossl_pkey_dh.c\
	ext/openssl/ossl_pkey_rsa.c\
	ext/openssl/ossl_pkcs12.c\
	ext/openssl/ossl_x509cert.c\
	ext/openssl/ossl_x509revoked.c\
	ext/openssl/ossl_ns_spki.c\
	ext/openssl/ossl_engine.c
LOCAL_SHARED_LIBRARIES:= \
	libssl\
	libcrypto
LOCAL_MODULE := RUBY_openssl

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/pathname.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/pathname\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/pathname/pathname.c
LOCAL_MODULE := RUBY_pathname

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/psych.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/psych\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	external/libyaml/include
LOCAL_SRC_FILES:= \
	ext/psych/to_ruby.c\
	ext/psych/emitter.c\
	ext/psych/psych.c\
	ext/psych/parser.c\
	ext/psych/yaml_tree.c
LOCAL_STATIC_LIBRARIES:= \
	libyaml
LOCAL_MODULE := RUBY_psych

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/pty.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/pty\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/pty/pty.c
LOCAL_MODULE := RUBY_pty

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/racc/cparse.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/racc/cparse\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/racc/cparse/cparse.c
LOCAL_MODULE := RUBY_racc_cparse

include $(BUILD_SHARED_LIBRARY)

#original path: .ext/arm-linux-androideabi/readline.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/ext/readline\
        $(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
        $(LOCAL_PATH)/include\
        external/libreadline\
        external/ncurses/include
LOCAL_SRC_FILES:= \
        ext/readline/readline.c
LOCAL_STATIC_LIBRARIES:= \
        libreadline\
        libncurses
LOCAL_MODULE := RUBY_readline

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/ripper.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/ripper\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	ruby
LOCAL_SRC_FILES:= \
	ext/ripper/ripper.c
LOCAL_MODULE := RUBY_ripper

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/sdbm.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/sdbm\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/sdbm/init.c\
	ext/sdbm/_sdbm.c
LOCAL_MODULE := RUBY_sdbm

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/socket.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/socket\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include\
	ruby
LOCAL_SRC_FILES:= \
	ext/socket/init.c\
	ext/socket/constants.c\
	ext/socket/basicsocket.c\
	ext/socket/socket.c\
	ext/socket/ipsocket.c\
	ext/socket/tcpsocket.c\
	ext/socket/tcpserver.c\
	ext/socket/sockssocket.c\
	ext/socket/udpsocket.c\
	ext/socket/unixsocket.c\
	ext/socket/unixserver.c\
	ext/socket/option.c\
	ext/socket/ancdata.c\
	ext/socket/raddrinfo.c
LOCAL_MODULE := RUBY_socket

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/stringio.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/stringio\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/stringio/stringio.c
LOCAL_MODULE := RUBY_stringio

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/strscan.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/strscan\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/strscan/strscan.c
LOCAL_MODULE := RUBY_strscan

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/syck.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/syck\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/syck/yaml2byte.c\
	ext/syck/token.c\
	ext/syck/handler.c\
	ext/syck/emitter.c\
	ext/syck/rubyext.c\
	ext/syck/syck.c\
	ext/syck/node.c\
	ext/syck/implicit.c\
	ext/syck/bytecode.c\
	ext/syck/gram.c
LOCAL_MODULE := RUBY_syck

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/syslog.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/syslog\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/syslog/syslog.c
LOCAL_MODULE := RUBY_syslog

include $(BUILD_SHARED_LIBRARY)


#original path: .ext/arm-linux-androideabi/zlib.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DRUBY_EXTCONF_H=\"extconf.h\" 

LOCAL_LDFLAGS:= -shared  -rdynamic  -Wl,-export-dynamic 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ext/zlib\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	ext/zlib/zlib.c
LOCAL_SHARED_LIBRARIES:= \
	libz
LOCAL_MODULE := RUBY_zlib

include $(BUILD_SHARED_LIBRARY)


#original path: ruby

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fvisibility=hidden -DRUBY_EXPORT 

LOCAL_LDFLAGS:= -rdynamic  -Wl,-export-dynamic 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--no-gc-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/.ext/include/arm-linux-androideabi\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	main.c
LOCAL_STATIC_LIBRARIES:= \
	libruby-static
LOCAL_SHARED_LIBRARIES:= \
	libdl
LOCAL_MODULE := ruby


include $(BUILD_EXECUTABLE)

