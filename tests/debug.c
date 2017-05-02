#include <gammu.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

GSM_StateMachine *s;

#ifdef WIN32
# define NONE_LOG ".\\none.log"
char debug_filename[] = ".\\gammu-debug-test.log";
#else
# define NONE_LOG "/dev/null"
char debug_filename[] = "./gammu-debug-test.log";
#endif

NORETURN void fail(int error_code)
{
	unlink(debug_filename);
	exit(error_code);
}

void check_log(FILE * f, gboolean match, const char *test_name)
{
	char buff[100];
	char test_message[] = "T3ST M3S5AG3";
	char cleaner[] = "XXXXXXXXXXXXXXXXX";
	size_t result;
	int cmp;

	rewind(f);
	GSM_LogError(s, test_message, ERR_MOREMEMORY);
	rewind(f);
	result = fread(buff, 1, sizeof(test_message), f);
	if (match && result != sizeof(test_message)) {
		printf("%s: Read failed (%ld)!\n", test_name, (long)result);
		fail(10);
	}
	if (!match && result != sizeof(test_message)) {
		goto done;
	}
	cmp = strncmp(test_message, buff, sizeof(test_message) - 1);
	if (match && cmp != 0) {
		printf("%s: Match failed!\n", test_name);
		fail(11);
	}
	if (!match && result == 0) {
		printf("%s: Matched but should not!\n", test_name);
		fail(12);
	}
done:
	rewind(f);
	result = fwrite(cleaner, 1, sizeof(cleaner), f);
	test_result(result == sizeof(cleaner));
	rewind(f);
}

void Log_Function(const char *text, void *data UNUSED)
{
	printf("msg: %s", text);
}

int main(int argc UNUSED, char **argv UNUSED)
{
	FILE *debug_file;
	GSM_Debug_Info *di_sm, *di_global;
	GSM_Error error;

	/* Allocates state machine */
	s = GSM_AllocStateMachine();
	test_result(s != NULL);

	/* Get debug handles */
	di_sm = GSM_GetDebug(s);
	di_global = GSM_GetGlobalDebug();

	/*
	 * Test 1 - setting debug level.
	 */
	test_result(GSM_SetDebugLevel("NONSENSE", di_sm) == FALSE);
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	test_result(GSM_SetDebugLevel("textall", di_sm) == TRUE);
	test_result(GSM_SetDebugLevel("textall", di_global) == TRUE);

	/*
	 * Test 2 - global /dev/null, local tempfile, do not use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_global);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_global)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm)");
	check_log(debug_file, TRUE, "2. global_file=NULL, sm_file=TEMP, use_global=FALSE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 3 - global /dev/null, local tempfile, use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_global);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_global)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm)");
	check_log(debug_file, FALSE, "3. global_file=NULL, sm_file=TEMP, use_global=TRUE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 4 - global tempfile, local /dev/null, use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_sm);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_sm)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global)");
	check_log(debug_file, TRUE, "4. global_file=TEMP, sm_file=NULL, use_global=TRUE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 5 - global tempfile, local /dev/null, do not use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_sm);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_sm)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global)");
	check_log(debug_file, FALSE, "5. global_file=TEMP, sm_file=NULL, use_global=FALSE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 6 - global /dev/null, local tempfile, do not use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_global);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_global)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm)");
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	check_log(debug_file, TRUE, "6. global_file=NULL, sm_file=TEMP, use_global=FALSE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 7 - global /dev/null, local tempfile, use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_global);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_global)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_sm)");
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	check_log(debug_file, FALSE, "7. global_file=NULL, sm_file=TEMP, use_global=TRUE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 8 - global tempfile, local /dev/null, use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_sm);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_sm)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global)");
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	check_log(debug_file, TRUE, "8. global_file=TEMP, sm_file=NULL, use_global=TRUE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 9 - global tempfile, local /dev/null, do not use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_sm);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_sm)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global)");
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	check_log(debug_file, FALSE, "9. global_file=TEMP, sm_file=NULL, use_global=FALSE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 10 - functional logging, do not use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_sm);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_sm)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global)");
	error = GSM_SetDebugFunction(Log_Function, NULL, di_sm);
	gammu_test_result(error, "GSM_SetDebugFunction(Log_Function, NULL, di_sm)");
	test_result(GSM_SetDebugGlobal(FALSE, di_sm) == TRUE);
	check_log(debug_file, FALSE, "10. global_file=TEMP, sm_file=function, use_global=FALSE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/*
	 * Test 11 - functional logging, use global
	 */
	debug_file = fopen(debug_filename, "w+");
	test_result(debug_file != NULL);
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	error = GSM_SetDebugFile(NONE_LOG, di_global);
	gammu_test_result(error, "GSM_SetDebugFile(NONE_LOG, di_global)");
	error = GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(debug_file, TRUE, di_global)");
	error = GSM_SetDebugFunction(Log_Function, NULL, di_global);
	gammu_test_result(error, "GSM_SetDebugFunction(Log_Function, NULL, di_global)");
	test_result(GSM_SetDebugGlobal(TRUE, di_sm) == TRUE);
	check_log(debug_file, FALSE, "11. global_file=function, sm_file=NULL, use_global=TRUE");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_sm)");
	error = GSM_SetDebugFileDescriptor(NULL, FALSE, di_global);
	gammu_test_result(error, "GSM_SetDebugFileDescriptor(NULL, FALSE, di_global)");

	/* Free state machine */
	GSM_FreeStateMachine(s);
	fail(0);
	return 0;
}

/* Editor configuration
 * vim: noexpandtab sw=8 ts=8 sts=8 tw=72:
 */
