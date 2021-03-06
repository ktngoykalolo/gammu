/* Test for decoding SMS on Nokia 6510 driver */

#include <gammu.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#include "../libgammu/protocol/protocol.h"	/* Needed for GSM_Protocol_Message */
#include "../libgammu/gsmstate.h"	/* Needed for state machine internals */

#include "../helper/message-display.h"

unsigned char data[] = {
	0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x01, 0xD7, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54,
	0x00, 0x63, 0x00, 0x68, 0x00, 0x69, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x0B, 0xD0, 0xD4, 0x31, 0x3A, 0x2D, 0x7E, 0x03, 0x00, 0x00, 0x90, 0x50, 0x81, 0x22, 0x10,
	0x23, 0x80, 0x35, 0xC8, 0xB2, 0x5C, 0xCF, 0x4E, 0x8F, 0xD1, 0xA0, 0x6B, 0x9A, 0xCD, 0x5E, 0xBF,
	0xDB, 0xED, 0xB2, 0x1B, 0x24, 0x2E, 0xA7, 0x41, 0x49, 0xB4, 0xBC, 0xDC, 0x06, 0x51, 0xC7, 0xE8,
	0xB4, 0xF8, 0x0D, 0x6A, 0xBE, 0xC5, 0x69, 0xB6, 0xB9, 0xEE, 0x5E, 0xB7, 0xA8, 0x61, 0x79, 0xDA,
	0xEC, 0x02, 0x01, 0x00, 0xE2, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x01,
	0x00, 0x0B, 0x00, 0x01, 0x00, 0x0F, 0x00, 0x02, 0x00, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x09, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x01, 0x00, 0x2A, 0x00, 0x01, 0x00,
	0x23, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x01, 0x00, 0x03, 0x00, 0x6C, 0x00, 0x48,
	0x00, 0x65, 0x00, 0x72, 0x00, 0x7A, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x63, 0x00, 0x68, 0x00, 0x20,
	0x00, 0x57, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x6B, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x6D,
	0x00, 0x65, 0x00, 0x6E, 0x00, 0x20, 0x00, 0x62, 0x00, 0x65, 0x00, 0x69, 0x00, 0x20, 0x00, 0x49,
	0x00, 0x68, 0x00, 0x72, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x20, 0x00, 0x54, 0x00, 0x63, 0x00, 0x68,
	0x00, 0x69, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x20, 0x00, 0x4D, 0x00, 0x6F, 0x00, 0x62, 0x00, 0x69,
	0x00, 0x6C, 0x00, 0x66, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x6B, 0x00, 0x2D, 0x00, 0x54, 0x00, 0x61,
	0x00, 0x72, 0x00, 0x69, 0x00, 0x66, 0x00, 0x2E, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x2B, 0x34, 0x39,
	0x31, 0x37, 0x36, 0x30, 0x30, 0x30, 0x30, 0x34, 0x34, 0x33, 0x00, 0x04, 0x00, 0x01, 0x00, 0x2B,
	0x00, 0x0E, 0x00, 0x54, 0x00, 0x63, 0x00, 0x68, 0x00, 0x69, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x00,
	0x07, 0x00, 0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x12, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x24,
	0x00, 0x01, 0x00, 0x22, 0x00, 0x01, 0x00
};

const char smsc[] = "+491760000443";

/* This is not part of API! */
extern GSM_Error N6510_DecodeFilesystemSMS(GSM_StateMachine * s, GSM_MultiSMSMessage * sms, GSM_File * FFF, int location);

int main(int argc UNUSED, char **argv UNUSED)
{
	GSM_Debug_Info *debug_info;
	GSM_StateMachine *s;
	GSM_File file;
	GSM_Error error;
	GSM_MultiSMSMessage sms;

	debug_info = GSM_GetGlobalDebug();
	GSM_SetDebugFileDescriptor(stderr, FALSE, debug_info);
	GSM_SetDebugLevel("textall", debug_info);

	/* Allocates state machine */
	s = GSM_AllocStateMachine();
	test_result(s != NULL);

	debug_info = GSM_GetDebug(s);
	GSM_SetDebugGlobal(TRUE, debug_info);

	/* Init file */
	file.Buffer = malloc(sizeof(data));
	memcpy(file.Buffer, data, sizeof(data));
	file.Used = sizeof(data);
	file.ID_FullName[0] = 0;
	file.ID_FullName[1] = 0;
	GSM_GetCurrentDateTime(&(file.Modified));

	/* Parse it */
	error = N6510_DecodeFilesystemSMS(s, &sms, &file, 0);

	/* Display message */
	DisplayMultiSMSInfo(&sms, FALSE, TRUE, NULL, NULL);
	DisplayMultiSMSInfo(&sms, TRUE, TRUE, NULL, NULL);

	/* Free state machine */
	GSM_FreeStateMachine(s);

	/* Check expected text */
	test_result(strcmp(smsc, DecodeUnicodeString(sms.SMS[0].SMSC.Number)) == 0);

	gammu_test_result(error, "N6510_DecodeFilesystemSMS");

	return 0;
}

/* Editor configuration
 * vim: noexpandtab sw=8 ts=8 sts=8 tw=72:
 */
