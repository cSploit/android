PWD_CURR=	$(shell pwd)
PROJ_ROOT:=	$(PWD_CURR)
FB2_ROOT=	$(PROJ_ROOT)/../../..
GEN_ROOT=	$(FB2_ROOT)/gen
SRC_ROOT=	$(FB2_ROOT)/src
FIREBIRD=	$(GEN_ROOT)/firebird
BIN_ROOT=	$(FIREBIRD)/bin
DB_ROOT=	$(GEN_ROOT)/refDatabases
BUILD_DIR=	$(PROJ_ROOT)/build
FW=			$(BUILD_DIR)/Firebird.framework
VAR=		$(FW)/Versions/Current/Resources/English.lproj/var

DYLD_FRAMEWORK_PATH=$(BUILD_DIR)
export DYLD_FRAMEWORK_PATH


GPRE_BOOT=	$(BUILD_DIR)/gpre_bootstrap
GPRE= $(BUILD_DIR)/gpre
GBAK= $(BUILD_DIR)/gbak
CREATE_DB=	$(BUILD_DIR)/create_db
LOCK_MGR=	$(BUILD_DIR)/gds_lock_mgr
GFIX=		$(BUILD_DIR)/gfix
ISQL=		$(BUILD_DIR)/isql
GSEC=		$(BUILD_DIR)/gsec
QLI=		$(BUILD_DIR)/qli
CHECK_MSGS=	$(BUILD_DIR)/check_msgs
BUILD_MSGS=	$(BUILD_DIR)/build_file
SEC_AUTH=	$(VAR)/auth/security_db.auth
LOCAL_USER_AUTH=	$(VAR)/auth/current_euid.auth

EMPTY_DB=	$(DB_ROOT)/empty.gdb
MSG_DB=		$(DB_ROOT)/msg.gdb
HELP_DB=	$(DB_ROOT)/help.gdb
META_DB=	$(DB_ROOT)/metadata.gdb
ISC_DB=		$(FIREBIRD)/isc4.gdb
ISC_GBAK=	$(FIREBIRD)/isc.gbak
MSG_FILE=	$(FIREBIRD)/interbase.msg
MSG_INDICATOR=	$(GEN_ROOT)/msgs/indicator.msg

PS_FW_FLAG=	$(GEN_ROOT)/firebird/.pseudo_framework_flag
UPG_FW_FLAG=	$(GEN_ROOT)/firebird/.upgrade_framework_flag
FULL_FW_FLAG=	$(GEN_ROOT)/firebird/.full_framework_flag

FB_FW=		$(PROJ_ROOT)/build/Firebird.framework


JRD_EPP_FILES=	blob_filter.cpp dyn.epp dyn_util.epp ini.epp stats.epp \
                dyn_def.epp met.epp dfw.epp dyn_del.epp \
				fun.epp pcmet.epp dpm.epp dyn_mod.epp grant.epp scl.epp
JRD_GEN_FILES= $(JRD_EPP_FILES:%.epp=$(GEN_ROOT)/jrd/%.cpp)

DSQL_EPP_FILES=	array.epp blob.epp metd.epp
DSQL_YACC_FILES=	parse.y
DSQL_GEN_FILES=	$(DSQL_EPP_FILES:%.epp=$(GEN_ROOT)/dsql/%.cpp) \
					$(DSQL_YACC_FILES:%.y=$(GEN_ROOT)/dsql/%.cpp)

GPRE_EPP_FILES=	gpre_meta.epp
GPRE_GEN_FILES= $(GPRE_EPP_FILES:%.epp=$(GEN_ROOT)/gpre/%.cpp)

GBAK_EPP_FILES=	backup.epp restore.epp OdsDetection.epp
GBAK_GEN_FILES= $(GBAK_EPP_FILES:%.epp=$(GEN_ROOT)/burp/%.cpp)

GFIX_EPP_FILES=	alice_meta.epp
GFIX_GEN_FILES= $(GFIX_EPP_FILES:%.epp=$(GEN_ROOT)/alice/%.cpp)

ISQL_EPP_FILES=	extract.epp isql.epp show.epp
ISQL_GEN_FILES= $(ISQL_EPP_FILES:%.epp=$(GEN_ROOT)/isql/%.cpp)

UTILITIES_EPP_FILES= dba.epp
UTILITIES_GEN_FILES= $(UTILITIES_EPP_FILES:%.epp=$(GEN_ROOT)/utilities/%.cpp)

SECURITY_EPP_FILES=	security.epp
SECURITY_GEN_FILES=	$(SECURITY_EPP_FILES:%.epp=$(GEN_ROOT)/utilities/%.cpp)

MSG_EPP_FILES=	build_file.epp change_msgs.epp check_msgs.epp enter_msgs.epp load.epp modify_msgs.epp
MSG_GEN_FILES=	$(MSG_EPP_FILES:%.epp=$(GEN_ROOT)/msgs/%.cpp)

QLI_EPP_FILES=	help.epp meta.epp proc.epp show.epp
QLI_GEN_FILES=	$(QLI_EPP_FILES:%.epp=$(GEN_ROOT)/qli/%.cpp)

GPRE_FLAGS= -r -m -z -n

all:

$(GEN_ROOT)/jrd/dyn_def.cpp : $(SRC_ROOT)/jrd/dyn_def.epp \
								$(SRC_ROOT)/jrd/dyn_def.sed
	$(GPRE_BOOT) $(GPRE_FLAGS) $< $(GEN_ROOT)/jrd/dyn_deffoo.cpp
	sed -f $(SRC_ROOT)/jrd/dyn_def.sed $(GEN_ROOT)/jrd/dyn_deffoo.cpp > $@
	rm $(GEN_ROOT)/jrd/dyn_deffoo.cpp

$(GEN_ROOT)/dsql/y.tab.c:    $(SRC_ROOT)/dsql/parse.y
	$(YACC) -l $(YFLAGS) -o $@ $<
$(GEN_ROOT)/dsql/parse.cpp:	$(SRC_ROOT)/dsql/parse.sed \
							$(GEN_ROOT)/dsql/y.tab.c
	sed -f $< $(GEN_ROOT)/dsql/y.tab.c > $@


$(GEN_ROOT)/jrd/%.cpp: $(SRC_ROOT)/jrd/%.epp $(GPRE_BOOT)
	$(GPRE_BOOT) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/dsql/%.cpp: $(SRC_ROOT)/dsql/%.epp $(GPRE_BOOT)
	$(GPRE_BOOT) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/gpre/%.cpp: $(SRC_ROOT)/gpre/%.epp $(GPRE_BOOT)
	$(GPRE_BOOT) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/burp/%.cpp: $(SRC_ROOT)/burp/%.epp $(GPRE)
	$(GPRE) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/alice/%.cpp: $(SRC_ROOT)/alice/%.epp $(GPRE)
	$(GPRE) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/isql/%.cpp: $(SRC_ROOT)/isql/%.epp $(GPRE)
	$(GPRE) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/utilities/%.cpp: $(SRC_ROOT)/utilities/%.epp $(GPRE)
	$(GPRE) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/msgs/%.cpp: $(SRC_ROOT)/msgs/%.epp $(GPRE)
	$(GPRE) $(GPRE_FLAGS) $< $@

$(GEN_ROOT)/qli/%.cpp: $(SRC_ROOT)/qli/%.epp $(GPRE)
	$(GPRE) $(GPRE_FLAGS) $< $@

gds_lock_mgr: $(LOCK_MGR)
$(LOCK_MGR): $(PROJ_ROOT)/build/gds_lock_mgr
	cp $< $@

intl_lib: $(VAR)/intl/gdsintl
$(VAR)/intl/gdsintl: build/gdsintl
	cp $< $@
	chmod a+x $(VAR)/intl/*

sec_auth: $(SEC_AUTH)
$(SEC_AUTH): $(PROJ_ROOT)/build/security_db.auth.bundle/Contents/MacOS/security_db.auth
	mkdir -p $(VAR)/auth
	-cp $< $@
	-chmod a+rx $@

local_user_auth: $(LOCAL_USER_AUTH)
$(LOCAL_USER_AUTH): $(PROJ_ROOT)/build/current_euid.auth.bundle/Contents/MacOS/current_euid.auth
	mkdir -p $(VAR)/auth
	-cp $< $@
	-chmod a+rx $@

firebird_boot.dylib: ../../gen/firebird/lib/firebird_boot.dylib
../../gen/firebird/lib/firebird_boot.dylib: build/firebird_boot.dylib
	rm -f $@
	ln -s ../../../macosx_build/firebird_test/$< $@
	rm -f $(FB_FW)/Versions/Current/Firebird2
	ln -s ../../../lib/firebird_boot.dylib $(FB_FW)/Versions/Current/Firebird2

jrd_preprocess_clean:
	rm -f $(JRD_GEN_FILES)
jrd_preprocess_: GPRE_FLAGS=-n -z -gds_cxx -raw -ids
jrd_preprocess_: $(JRD_GEN_FILES)
jrd_preprocess_%:

gpre_preprocess_clean:
	rm -f $(GPRE_GEN_FILES)
gpre_preprocess_: GPRE_FLAGS=-lang_internal -r -m -z -n
gpre_preprocess_: $(GPRE_GEN_FILES)
gpre_preprocess_%:

dsql_preprocess_clean:
	rm -f $(DSQL_GEN_FILES) $(GEN_ROOT)/dsql/y.tab.c
dsql_preprocess_: GPRE_FLAGS=-lang_internal -r -m -z -n
dsql_preprocess_: $(DSQL_GEN_FILES)
dsql_preprocess_%:

burp_preprocess: $(GBAK_GEN_FILES)
burp_preprocess_clean:
	rm -f $(GBAK_GEN_FILES)
burp_preprocess_:
	./gpre_wrapper.sh burp_preprocess burp
burp_preprocess_%:

msg_preprocess: $(MSG_GEN_FILES)
msg_preprocess_clean:
	rm -f $(MSG_GEN_FILES)
msg_preprocess_:
	./gpre_wrapper.sh msg_preprocess msgs
msg_preprocess_%:

messages_clean:
	rm -f $(MSG_FILE) $(MSG_INDICATOR)
messages_:
	make -f $(PROJ_ROOT)/Helpers.make -C $(FB2_ROOT)/gen/msgs PWD_CURR=$(PWD_CURR) messages
messages: update_msg_indicator msg_file
msg_file:	$(MSG_INDICATOR)
	$(BUILD_MSGS) -d master_msg_db
	cp interbase.msg $(MSG_FILE)
update_msg_indicator: $(MSG_DB)
	$(CHECK_MSGS) -d master_msg_db

alice_preprocess_clean:
	rm -f $(ALICE_GEN_FILES)
alice_preprocess_: $(ALICE_GEN_FILES)
alice_preprocess_%:

gfix_preprocess_clean:
	rm -f $(GFIX_GEN_FILES)
gfix_preprocess: $(GFIX_GEN_FILES)
gfix_preprocess_%:
gfix_preprocess_:
	./gpre_wrapper.sh gfix_preprocess alice

qli_preprocess_clean:
	rm -f $(QLI_GEN_FILES)
qli_preprocess: $(QLI_GEN_FILES)
qli_preprocess_%:
qli_preprocess_:
	./gpre_wrapper.sh qli_preprocess qli

security_preprocess_clean:
	rm -f $(SECURITY_GEN_FILES)
security_preprocess: $(SECURITY_GEN_FILES)
security_preprocess_%:
security_preprocess_:
	./gpre_wrapper.sh security_preprocess utilities

utilities_preprocess_clean:
	rm -f $(UTILITIES_GEN_FILES)
utilities_preprocess_: $(UTILITIES_GEN_FILES)
utilities_preprocess_%:

isql_preprocess_clean:
	rm -f $(ISQL_GEN_FILES)
isql_preprocess_:
	./gpre_wrapper.sh isql_preprocess isql
isql_preprocess: $(ISQL_GEN_FILES)
isql_preprocess_%:

empty_db_clean:
	rm -f $(EMPTY_DB)
empty_db_:	$(EMPTY_DB)
$(EMPTY_DB):
	rm -f $(EMPTY_DB)
	$(CREATE_DB) $(EMPTY_DB)
	ln -fs $(EMPTY_DB) $(GEN_ROOT)/burp/yachts.lnk
	ln -fs $(EMPTY_DB) $(GEN_ROOT)/alice/yachts.lnk
	ln -fs $(EMPTY_DB) $(GEN_ROOT)/isql/yachts.lnk
	ln -fs $(EMPTY_DB) $(GEN_ROOT)/utilities/yachts.lnk
empty_db_%:

$(FULL_FW_FLAG):
	touch $(FULL_FW_FLAG)
$(UPG_FW_FLAG):
	touch $(UPG_FW_FLAG)

#upgrade_fw_:	$(UPG_FW_FLAG)
#$(UPG_FW_FLAG): $(PS_FW_FLAG) $(FULL_FW_FLAG)
#	rm -f $(FB_FW)/Versions/A/Firebird2
#	ln -s ../../../lib/firebird.dylib $(FB_FW)/Versions/Current/Firebird2
#	touch $(UPG_FW_FLAG)
#upgrade_fw_clean:
#upgrade_fw_install:

darwin_pseudo_fw_: $(PS_FW_FLAG)
$(PS_FW_FLAG): $(FULL_FW_FLAG) $(UPG_FW_FLAG)
	$(MAKE) -C $(SRC_ROOT) darwin_pseudo_fw
	touch $(PS_FW_FLAG)
darwin_pseudo_fw_clean:

build_dbs_: $(MSG_DB) $(HELP_DB) $(META_DB)
build_dbs_clean:
	rm -f $(MSG_DB) $(HELP_DB) $(META_DB)
build_dbs_%:

$(MSG_DB): $(SRC_ROOT)/msgs/msg.gbak
	$(GBAK) -MODE read_only -R $(SRC_ROOT)/msgs/msg.gbak $@
	ln -fs $(MSG_DB) $(GEN_ROOT)/msgs/msg.gdb
	ln -fs $(MSG_DB) $(GEN_ROOT)/msgs/master_msg_db

$(HELP_DB): $(SRC_ROOT)/misc/help.gbak
	$(GBAK) -MODE read_only -R $(SRC_ROOT)/misc/help.gbak $@
	ln -fs $(HELP_DB) $(GEN_ROOT)/qli/help.gdb

$(META_DB): $(SRC_ROOT)/misc/metadata.gbak
	$(GBAK) -MODE read_only -R $(SRC_ROOT)/misc/metadata.gbak $@
	ln -fs $(META_DB) $(GEN_ROOT)/qli/yachts.lnk

isc4.gdb_: $(ISC_DB) sysdba_user
$(ISC_DB) : $(SRC_ROOT)/utilities/isc4.sql $(SRC_ROOT)/utilities/isc4.gdl
	( cd $(FIREBIRD); $(ISQL) -z -i $(SRC_ROOT)/utilities/isc4.sql)
	-ln -sf $(ISC_DB) $(GEN_ROOT)/utilities/isc4.gdb

isc4.gdb_clean:
	rm -f $(ISC_DB) $(GEN_ROOT)/utilities/isc4.gdb
isc4.gdb_%:

sysdba_user_:
	make -C $(FIREBIRD) -f $(PROJ_ROOT)/Helpers.make PWD_CURR=$(PWD_CURR) sysdba_user
sysdba_user_clean:
sysdba_user:
	-$(GSEC) -da $(ISC_DB) -delete SYSDBA
	$(GSEC) -da $(ISC_DB) -add SYSDBA -pw masterkey
	$(GBAK) -z $(ISC_DB) $(ISC_GBAK)


message_file_:
	$(MAKE) -C $(FB2_ROOT)/src/msgs GPRE_CURRENT=$(GPRE) msgs
message_file_clean:
	rm -f $(FB2_ROOT)/gen/firebird/interbase.msg

squeky_:
squeky_install:
squeky_clean:
	rm -rf $(FIREBIRD)/lib/* $(FIREBIRD)/Firebird2.framework $(FIREBIRD)/.* $GEN_ROOT)/jrd/.* $(GEN_ROOT)/utilities/.* $(FIREBIRD)/bin/*

autoconf_:	$(FB2_ROOT)/config.status
$(FB2_ROOT)/config.status: $(FB2_ROOT)/configure
	(cd $(FB2_ROOT); ./configure)
autoconf_clean:
	rm -f $(FB2_ROOT)/config.cache $(FB2_ROOT)/config.log $(FB2_ROOT)/config.status $(FB2_ROOT)/src/include/gen/autoconfig.h

fb_fw_var:	$(VAR)
$(VAR):
	mkdir -p $(VAR)
	mkdir -p $(VAR)/intl
	mkdir -p $(VAR)/help
	mkdir -p $(VAR)/auth
	mkdir -p $(VAR)/UDF
	ln -s $(GEN_ROOT)/firebird/interbase.msg $(VAR)/interbase.msg
	ln -s ../../../../../.. $(VAR)/bin
	ln -s $(GEN_ROOT)/firebird/isc4.gdb $(VAR)/isc4.gdb

fw_files_clean:
fw_files_:
	rm -rf $(VAR)
	mkdir -p $(VAR)/UDF
	mkdir -p $(VAR)/intl
	mkdir -p $(VAR)/help
	mkdir -p $(VAR)/auth
	mkdir -p $(FB_FW)/Resources/bin
	cp $(FIREBIRD)/interbase.msg $(VAR)/interbase.msg
	-cp $(GPRE) $(GBAK) $(ISQL) $(QLI) $(GSEC) $(GFIX) $(FB_FW)/Resources/bin
	cp $(FIREBIRD)/isc.gbak $(VAR)
	cp build/gdsintl $(VAR)/intl
	chmod a+x $(VAR)/intl/*
	-cp build/local_user.bundle/Contents/MacOS/local_user $(LOCAL_USER_AUTH)
	-cp $(SRC_ROOT)/install/arch-specific/darwin/services.isc $(VAR)
	ln -s ../../bin $(VAR)/bin

headers_:
	-mkdir -p $(FB_FW)/Versions/A/Headers
	echo "#ifndef IBASE_H" > $(FW)/Headers/ibase.h
	cat $(SRC_ROOT)/include/fb_types.h $(SRC_ROOT)/jrd/sqlda_pub.h $(SRC_ROOT)/jrd/dsc_pub.h  $(SRC_ROOT)/jrd/ibase.h $(SRC_ROOT)/jrd/inf_pub.h $(SRC_ROOT)/include/gen/iberror.h $(SRC_ROOT)/jrd/blr.h | grep -v "#include" >> $(FW)/Headers/ibase.h
	echo "#endif /*IBASE_H*/" >> $(FW)/Headers/ibase.h
headers_clean:

installer_clean:
	rm -f build/firebird.tar.gz
installer_:
	mkdir -p build/installer_tmp/firebird
	rm -f build/firebird.tar.gz
	rm -f $(VAR)/isc_init* $(VAR)/isc_lock* $(VAR)/isc_event* $(VAR)/interbase.log
	tar -cf build/installer_tmp/firebird/firebird.tar -C build Firebird.framework
	-cp $(SRC_ROOT)/install/arch-specific/darwin/install build/installer_tmp/firebird
	tar -czf build/firebird.tar.gz -C build/installer_tmp firebird
	rm -rf build/installer_tmp
