

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"

extern int TestCallbackSetterAndGetterSuiteSetup(CuSuite *suite);
extern int TestCallbackSetterAndGetterSuiteTeardown(CuSuite *suite);
extern int TestCallbackSetterAndGetterTestSetup(CuTest *ct);
extern int TestCallbackSetterAndGetterTestTeardown(CuTest *ct);
extern int TestEnvCallbacks(CuTest *ct);
extern int TestDbCallbacks(CuTest *ct);
extern int TestChannelSuiteSetup(CuSuite *suite);
extern int TestChannelSuiteTeardown(CuSuite *suite);
extern int TestChannelTestSetup(CuTest *test);
extern int TestChannelTestTeardown(CuTest *test);
extern int TestChannelFeature(CuTest *ct);
extern int TestDbHotBackupSuiteSetup(CuSuite *suite);
extern int TestDbHotBackupSuiteTeardown(CuSuite *suite);
extern int TestDbHotBackupTestSetup(CuTest *ct);
extern int TestDbHotBackupTestTeardown(CuTest *ct);
extern int TestBackupSimpleEnvNoCallback(CuTest *ct);
extern int TestBackupSimpleEnvWithCallback(CuTest *ct);
extern int TestBackupSimpleEnvWithConfig(CuTest *ct);
extern int TestBackupPartitionDB(CuTest *ct);
extern int TestBackupMultiDataDir(CuTest *ct);
extern int TestBackupSetLogDir(CuTest *ct);
extern int TestBackupQueueDB(CuTest *ct);
extern int TestBackupHeapDB(CuTest *ct);
extern int TestDbTuner(CuTest *ct);
extern int TestNoEncryptedDb(CuTest *ct);
extern int TestEncryptedDbFlag(CuTest *ct);
extern int TestEncryptedDb(CuTest *ct);
extern int TestEncryptedDbFlagAndDb(CuTest *ct);
extern int TestEnvWithNoEncryption(CuTest *ct);
extern int TestEnvWithEncryptedDbFlag(CuTest *ct);
extern int TestEnvWithEncryptedDb(CuTest *ct);
extern int TestEnvWithEncryptedDbFlagAndDb(CuTest *ct);
extern int TestEncyptedEnv(CuTest *ct);
extern int TestEncyptedEnvWithEncyptedDbFlag(CuTest *ct);
extern int TestEncyptedEnvWithEncyptedDb(CuTest *ct);
extern int TestEncyptedEnvWithEncryptedDbFlagAndDb(CuTest *ct);
extern int TestEnvConfigSuiteSetup(CuSuite *ct);
extern int TestEnvConfigSuiteTeardown(CuSuite *ct);
extern int TestEnvConfigTestSetup(CuTest *ct);
extern int TestEnvConfigTestTeardown(CuTest *ct);
extern int TestSetTxMax(CuTest *ct);
extern int TestSetLogMax(CuTest *ct);
extern int TestSetLogBufferSize(CuTest *ct);
extern int TestSetLogRegionSize(CuTest *ct);
extern int TestGetLockConflicts(CuTest *ct);
extern int TestSetLockDetect(CuTest *ct);
extern int TestLockMaxLocks(CuTest *ct);
extern int TestLockMaxLockers(CuTest *ct);
extern int TestSetLockMaxObjects(CuTest *ct);
extern int TestSetLockTimeout(CuTest *ct);
extern int TestSetTransactionTimeout(CuTest *ct);
extern int TestSetCachesize(CuTest *ct);
extern int TestSetThreadCount(CuTest *ct); /* SKIP */
extern int TestKeyExistErrorReturn(CuTest *ct);
extern int TestMutexAlignment(CuTest *ct);
extern int TestPartialSuiteSetup(CuSuite *ct);
extern int TestPartialSuiteTeardown(CuSuite *ct);
extern int TestPartialTestSetup(CuTest *ct);
extern int TestPartialTestTeardown(CuTest *ct);
extern int TestDbPartialGet(CuTest *ct);
extern int TestDbPartialPGet(CuTest *ct);
extern int TestCursorPartialGet(CuTest *ct);
extern int TestCursorPartialPGet(CuTest *ct);
extern int TestPartitionSuiteSetup(CuSuite *suite);
extern int TestPartitionSuiteTeardown(CuSuite *suite);
extern int TestPartitionTestSetup(CuTest *ct);
extern int TestPartitionTestTeardown(CuTest *ct);
extern int TestPartOneKeyNoData(CuTest *ct);
extern int TestPartTwoKeyNoData(CuTest *ct);
extern int TestPartDuplicatedKey(CuTest *ct);
extern int TestPartUnsortedKey(CuTest *ct);
extern int TestPartNumber(CuTest *ct);
extern int TestPartKeyCallBothSet(CuTest *ct);
extern int TestPartKeyCallNeitherSet(CuTest *ct);
extern int TestPreOpenSetterAndGetterSuiteSetup(CuSuite *suite);
extern int TestPreOpenSetterAndGetterSuiteTeardown(CuSuite *suite);
extern int TestPreOpenSetterAndGetterTestSetup(CuTest *ct);
extern int TestPreOpenSetterAndGetterTestTeardown(CuTest *ct);
extern int TestEnvPreOpenSetterAndGetter(CuTest *ct);
extern int TestDbPreOpenSetterAndGetter(CuTest *ct);
extern int TestMpoolFilePreOpenSetterAndGetter(CuTest *ct);
extern int TestSequencePreOpenSetterAndGetter(CuTest *ct);
extern int TestQueue(CuTest *ct);

int RunCallbackSetterAndGetterTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestCallbackSetterAndGetter",
	    TestCallbackSetterAndGetterSuiteSetup, 
	    TestCallbackSetterAndGetterSuiteTeardown);
	int count;

	SUITE_ADD_TEST(suite, TestEnvCallbacks,
	    TestCallbackSetterAndGetterTestSetup, 
	    TestCallbackSetterAndGetterTestTeardown);
	SUITE_ADD_TEST(suite, TestDbCallbacks,
	    TestCallbackSetterAndGetterTestSetup, 
	    TestCallbackSetterAndGetterTestTeardown);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunChannelTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestChannel",
	    TestChannelSuiteSetup, TestChannelSuiteTeardown);
	int count;

	SUITE_ADD_TEST(suite, TestChannelFeature,
	    TestChannelTestSetup, TestChannelTestTeardown);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunDbHotBackupTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestDbHotBackup",
	    TestDbHotBackupSuiteSetup, TestDbHotBackupSuiteTeardown);
	int count;

	SUITE_ADD_TEST(suite, TestBackupSimpleEnvNoCallback,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);
	SUITE_ADD_TEST(suite, TestBackupSimpleEnvWithCallback,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);
	SUITE_ADD_TEST(suite, TestBackupSimpleEnvWithConfig,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);
	SUITE_ADD_TEST(suite, TestBackupPartitionDB,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);
	SUITE_ADD_TEST(suite, TestBackupMultiDataDir,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);
	SUITE_ADD_TEST(suite, TestBackupSetLogDir,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);
	SUITE_ADD_TEST(suite, TestBackupQueueDB,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);
	SUITE_ADD_TEST(suite, TestBackupHeapDB,
	    TestDbHotBackupTestSetup, TestDbHotBackupTestTeardown);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunDbTunerTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestDbTuner",
	    NULL, NULL);
	int count;

	SUITE_ADD_TEST(suite, TestDbTuner,
	    NULL, NULL);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunEncryptionTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestEncryption",
	    NULL, NULL);
	int count;

	SUITE_ADD_TEST(suite, TestNoEncryptedDb,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEncryptedDbFlag,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEncryptedDb,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEncryptedDbFlagAndDb,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEnvWithNoEncryption,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEnvWithEncryptedDbFlag,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEnvWithEncryptedDb,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEnvWithEncryptedDbFlagAndDb,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEncyptedEnv,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEncyptedEnvWithEncyptedDbFlag,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEncyptedEnvWithEncyptedDb,
	    NULL, NULL);
	SUITE_ADD_TEST(suite, TestEncyptedEnvWithEncryptedDbFlagAndDb,
	    NULL, NULL);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunEnvConfigTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestEnvConfig",
	    TestEnvConfigSuiteSetup, TestEnvConfigSuiteTeardown);
	int count;

	SUITE_ADD_TEST(suite, TestSetTxMax,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetLogMax,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetLogBufferSize,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetLogRegionSize,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestGetLockConflicts,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetLockDetect,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestLockMaxLocks,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestLockMaxLockers,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetLockMaxObjects,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetLockTimeout,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetTransactionTimeout,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);
	SUITE_ADD_TEST(suite, TestSetCachesize,
	    TestEnvConfigTestSetup, TestEnvConfigTestTeardown);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunEnvMethodTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestEnvMethod",
	    NULL, NULL);
	int count;

	SUITE_ADD_TEST(suite, TestSetThreadCount,
	    NULL, NULL); /* SKIP */

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunKeyExistErrorReturnTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestKeyExistErrorReturn",
	    NULL, NULL);
	int count;

	SUITE_ADD_TEST(suite, TestKeyExistErrorReturn,
	    NULL, NULL);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunMutexAlignmentTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestMutexAlignment",
	    NULL, NULL);
	int count;

	SUITE_ADD_TEST(suite, TestMutexAlignment,
	    NULL, NULL);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunPartialTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestPartial",
	    TestPartialSuiteSetup, TestPartialSuiteTeardown);
	int count;

	SUITE_ADD_TEST(suite, TestDbPartialGet,
	    TestPartialTestSetup, TestPartialTestTeardown);
	SUITE_ADD_TEST(suite, TestDbPartialPGet,
	    TestPartialTestSetup, TestPartialTestTeardown);
	SUITE_ADD_TEST(suite, TestCursorPartialGet,
	    TestPartialTestSetup, TestPartialTestTeardown);
	SUITE_ADD_TEST(suite, TestCursorPartialPGet,
	    TestPartialTestSetup, TestPartialTestTeardown);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunPartitionTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestPartition",
	    TestPartitionSuiteSetup, TestPartitionSuiteTeardown);
	int count;

	SUITE_ADD_TEST(suite, TestPartOneKeyNoData,
	    TestPartitionTestSetup, TestPartitionTestTeardown);
	SUITE_ADD_TEST(suite, TestPartTwoKeyNoData,
	    TestPartitionTestSetup, TestPartitionTestTeardown);
	SUITE_ADD_TEST(suite, TestPartDuplicatedKey,
	    TestPartitionTestSetup, TestPartitionTestTeardown);
	SUITE_ADD_TEST(suite, TestPartUnsortedKey,
	    TestPartitionTestSetup, TestPartitionTestTeardown);
	SUITE_ADD_TEST(suite, TestPartNumber,
	    TestPartitionTestSetup, TestPartitionTestTeardown);
	SUITE_ADD_TEST(suite, TestPartKeyCallBothSet,
	    TestPartitionTestSetup, TestPartitionTestTeardown);
	SUITE_ADD_TEST(suite, TestPartKeyCallNeitherSet,
	    TestPartitionTestSetup, TestPartitionTestTeardown);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunPreOpenSetterAndGetterTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestPreOpenSetterAndGetter",
	    TestPreOpenSetterAndGetterSuiteSetup, 
	    TestPreOpenSetterAndGetterSuiteTeardown);
	int count;

	SUITE_ADD_TEST(suite, TestEnvPreOpenSetterAndGetter,
	    TestPreOpenSetterAndGetterTestSetup, 
	    TestPreOpenSetterAndGetterTestTeardown);
	SUITE_ADD_TEST(suite, TestDbPreOpenSetterAndGetter,
	    TestPreOpenSetterAndGetterTestSetup, 
	    TestPreOpenSetterAndGetterTestTeardown);
	SUITE_ADD_TEST(suite, TestMpoolFilePreOpenSetterAndGetter,
	    TestPreOpenSetterAndGetterTestSetup, 
	    TestPreOpenSetterAndGetterTestTeardown);
	SUITE_ADD_TEST(suite, TestSequencePreOpenSetterAndGetter,
	    TestPreOpenSetterAndGetterTestSetup, 
	    TestPreOpenSetterAndGetterTestTeardown);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

int RunQueueTests(CuString *output)
{
	CuSuite *suite = CuSuiteNew("TestQueue",
	    NULL, NULL);
	int count;

	SUITE_ADD_TEST(suite, TestQueue,
	    NULL, NULL);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}

TestSuite g_suites[] = {
	{ "TestCallbackSetterAndGetter", RunCallbackSetterAndGetterTests },
	{ "TestChannel", RunChannelTests },
	{ "TestDbHotBackup", RunDbHotBackupTests },
	{ "TestDbTuner", RunDbTunerTests },
	{ "TestEncryption", RunEncryptionTests },
	{ "TestEnvConfig", RunEnvConfigTests },
	{ "TestEnvMethod", RunEnvMethodTests },
	{ "TestKeyExistErrorReturn", RunKeyExistErrorReturnTests },
	{ "TestMutexAlignment", RunMutexAlignmentTests },
	{ "TestPartial", RunPartialTests },
	{ "TestPartition", RunPartitionTests },
	{ "TestPreOpenSetterAndGetter", RunPreOpenSetterAndGetterTests },
	{ "TestQueue", RunQueueTests },
	{ "", NULL },
};

