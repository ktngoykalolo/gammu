
/* www.irda.org OBEX specs 1.3 */

/* Module connects to F9EC7BC4-953c-11d2-984E-525400DC9E09 UUID and in the
 * future there will required implementing reconnecting. See "ifdef xxxx"
 */

#include <string.h>
#include <time.h>

#include "../../gsmcomon.h"
#include "../../gsmstate.h"
#include "../../misc/coding.h"
#include "../../service/gsmmisc.h"
#include "../../protocol/obex/obex.h"

#ifdef GSM_ENABLE_OBEXGEN

static GSM_Error OBEXGEN_GetNextFileFolder(GSM_StateMachine *s, GSM_File *File, bool start);
static GSM_Error OBEXGEN_GetFilePart	  (GSM_StateMachine *s, GSM_File *File);

static void OBEXGEN_FindNextDir(unsigned char *Path, int *Pos, unsigned char *Return)
{
	unsigned char buff[200];

	buff[0] = 0;
	while(1) {
		if (Path[*Pos] == 0x00) break;
		if (Path[*Pos] == '\\') {
			(*Pos)++;
			break;
		}
		buff[strlen(buff)+1] = 0;
		buff[strlen(buff)]   = Path[(*Pos)];
		(*Pos)++;
	}
	EncodeUnicode(Return,buff,strlen(buff));
}
                                    	
static GSM_Error OBEXGEN_ReplyConnect(GSM_Protocol_Message msg, GSM_StateMachine *s)
{                
	switch (msg.Type) {
	case 0xA0:
		smprintf(s,"Connected OK\n");
		s->Phone.Data.Priv.OBEXGEN.FrameSize = msg.Buffer[2]*256+msg.Buffer[3];
		smprintf(s,"Maximal size of frame is %i 0x%x\n",s->Phone.Data.Priv.OBEXGEN.FrameSize,s->Phone.Data.Priv.OBEXGEN.FrameSize);
		return GE_NONE;
	}
	return GE_UNKNOWNRESPONSE;
}

GSM_Error OBEXGEN_Connect(GSM_StateMachine *s, OBEX_Service service)
{
	int		Current=4;
	unsigned char 	req2[200];
	unsigned char 	req[200] = {
		0x10,			/* Version 1.0 			*/
		0x00,			/* no flags 			*/
		0x20,0x00};		/* 0x2000 max size of packet 	*/

	if (service == s->Phone.Data.Priv.OBEXGEN.Service) return GE_NONE;

	switch (service) {
	case OBEX_None:
		break;
	case OBEX_BrowsingFolders:
		/* Server ID */
		req2[0] = 0xF9; req2[1] = 0xEC; req2[2] = 0x7B;
		req2[3] = 0xC4; req2[4] = 0x95; req2[5] = 0x3C;
		req2[6] = 0x11; req2[7] = 0xD2; req2[8] = 0x98;
		req2[9] = 0x4E; req2[10]= 0x52; req2[11]= 0x54;
		req2[12]= 0x00; req2[13]= 0xDC; req2[14]= 0x9E;
		req2[15]= 0x09;

		/* Target block */
		OBEXAddBlock(req, &Current, 0x46, req2, 16);
	}

#ifndef xxxx
	//disconnect old service
#else
	if (s->Phone.Data.Priv.OBEXGEN.Service != 0) return GE_NONE;
#endif

	s->Phone.Data.Priv.OBEXGEN.Service = service;

	smprintf(s, "Connecting\n");
	return GSM_WaitFor (s, req, Current, 0x80, 2, ID_Initialise);
}


GSM_Error OBEXGEN_Initialise(GSM_StateMachine *s)
{
//	GSM_File 	File;
//	GSM_Error	error = GE_NONE;

	s->Phone.Data.Priv.OBEXGEN.Service = 0;

	strcpy(s->Phone.Data.Model,"obex");

	s->Phone.Data.VerNum 		= 0;
	s->Phone.Data.Version[0] 	= 0;
	s->Phone.Data.Manufacturer[0] 	= 0;

//	File.Used 			= 0;
//	File.ID_FullName[0] 		= 0;
//	File.Buffer			= NULL;
//	while (error == GE_NONE) error = OBEXGEN_GetFilePart(s,&File);

	return GE_NONE;
}

static GSM_Error OBEXGEN_ReplyChangePath(GSM_Protocol_Message msg, GSM_StateMachine *s)
{                
	switch (msg.Type) {
	case 0xA0:
		smprintf(s,"Path set OK\n");
		return GE_NONE;
	case 0xA1:
		smprintf(s,"Folder created\n");
		return GE_NONE;
	case 0xC3:
		smprintf(s,"Security error\n");
		return GE_SECURITYERROR;
	}
	return GE_UNKNOWNRESPONSE;
}

static GSM_Error OBEXGEN_ChangePath(GSM_StateMachine *s, char *Name, unsigned char Flag1)
{
	unsigned char 	req[400];
	int		Current = 2;

	/* Flags */
	req[0] = Flag1;
	req[1] = 0x00;

	/* Name block */
	if (Name != NULL && UnicodeLength(Name) != 0) {
		OBEXAddBlock(req, &Current, 0x01, Name, UnicodeLength(Name)*2+2);
	} else {
		OBEXAddBlock(req, &Current, 0x01, NULL, 0);
	}

	/* connection ID block */
	req[Current++] = 0xCB; // ID
	req[Current++] = 0x00; req[Current++] = 0x00;
	req[Current++] = 0x00; req[Current++] = 0x01;

	return GSM_WaitFor (s, req, Current, 0x85, 4, ID_SetPath);
}

static GSM_Error OBEXGEN_ReplyAddFilePart(GSM_Protocol_Message msg, GSM_StateMachine *s)
{
	switch (msg.Type) {
	case 0x90:
		smprintf(s,"Last part of file added OK\n");
		return GE_NONE;
	case 0xA0:
		smprintf(s,"Part of file added OK\n");
		return GE_NONE;
	case 0xC0:
		smprintf(s,"Not understand. Probably not supported\n");
		return GE_NOTSUPPORTED;
	}
	return GE_UNKNOWNRESPONSE;
}

static GSM_Error OBEXGEN_AddFilePart(GSM_StateMachine *s, GSM_File *File, int *Pos)
{
	GSM_Error		error;
	int			j;
	unsigned int		Pos2, Current = 0;
	unsigned char 		req[2000],req2[200];

	s->Phone.Data.File = File;

	if (*Pos == 0) {
		if (!strcmp(File->ID_FullName,"")) {
#ifndef xxxx
			error = OBEXGEN_Connect(s,OBEX_None);
#else
			error = OBEXGEN_Connect(s,OBEX_BrowsingFolders);
#endif
			if (error != GE_NONE) return error;
		} else {
			error = OBEXGEN_Connect(s,OBEX_BrowsingFolders);
			if (error != GE_NONE) return error;

			smprintf(s,"Changing to root\n");
			error = OBEXGEN_ChangePath(s, NULL, 2);
			if (error != GE_NONE) return error;

			Pos2 = 0;
			do {
				OBEXGEN_FindNextDir(File->ID_FullName, &Pos2, req2);
				smprintf(s,"%s %i %i\n",DecodeUnicodeString(req2),Pos2,strlen(File->ID_FullName));
				smprintf(s,"Changing path down\n");
				error=OBEXGEN_ChangePath(s, req2, 2);
				if (error != GE_NONE) return error;
				if (Pos2 == strlen(File->ID_FullName)) break;
			} while (1);
		}

		/* Name block */
		OBEXAddBlock(req, &Current, 0x01, File->Name, UnicodeLength(File->Name)*2+2);

		/* File size block */
		req[Current++] = 0xC3; // ID
		req[Current++] = 0;
		req[Current++] = 0;
		req[Current++] = File->Used / 256;
		req[Current++] = File->Used % 256;
	}

	if (s->Phone.Data.Priv.OBEXGEN.Service == OBEX_BrowsingFolders) {
		/* connection ID block */
		req[Current++] = 0xCB; // ID
		req[Current++] = 0x00; req[Current++] = 0x00;
		req[Current++] = 0x00; req[Current++] = 0x01;
	}

	j = s->Phone.Data.Priv.OBEXGEN.FrameSize - Current - 20;
	if (j > 1000) j = 1000;

	if (File->Used - *Pos < j) {
		j = File->Used - *Pos;
		/* End of file body block */
		OBEXAddBlock(req, &Current, 0x49, File->Buffer+(*Pos), j);
		smprintf(s, "Adding file part %i %i\n",*Pos,j);
		*Pos = *Pos + j;
		error = GSM_WaitFor (s, req, Current, 0x82, 4, ID_AddFile);
		if (error != GE_NONE) return error;
		return GE_EMPTY;
	} else {
		/* File body block */
		OBEXAddBlock(req, &Current, 0x48, File->Buffer+(*Pos), j);
		smprintf(s, "Adding file part %i %i\n",*Pos,j);
		*Pos = *Pos + j;
		error=GSM_WaitFor (s, req, Current, 0x02, 4, ID_AddFile);
	}
	return error;
}

static GSM_Error OBEXGEN_ReplyGetFilePart(GSM_Protocol_Message msg, GSM_StateMachine *s)
{
	int old;

	switch (msg.Type) {
	case 0xA0:
		smprintf(s,"File part received\n");
		s->Phone.Data.Priv.OBEXGEN.FileLastPart = true;
	case 0x90:
		if (msg.Type == 0x90) smprintf(s,"Last file part received\n");
		if (msg.Length < 11) return GE_NONE;
		old = s->Phone.Data.File->Used;
		s->Phone.Data.File->Used += msg.Buffer[1]*256+msg.Buffer[2]-3;
		smprintf(s,"Length of file part: %i\n",
				msg.Buffer[1]*256+msg.Buffer[2]-3);

		s->Phone.Data.File->Buffer = (unsigned char *)realloc(s->Phone.Data.File->Buffer,s->Phone.Data.File->Used);
		memcpy(s->Phone.Data.File->Buffer+old,msg.Buffer+3,s->Phone.Data.File->Used-old);
		return GE_NONE;
	case 0xC3:
		return GE_NOTSUPPORTED;
	case 0xC4:
		smprintf(s,"Not found\n");
		return GE_SECURITYERROR;
	}
	return GE_UNKNOWNRESPONSE;
}

static GSM_Error OBEXGEN_ReplyGetFileInfo(GSM_Protocol_Message msg, GSM_StateMachine *s)
{
	switch (msg.Type) {
	case 0x83:
		smprintf(s,"Not available ?\n");
		return GE_NONE;
	case 0x90:
		smprintf(s,"Last part of file info received\n");
		return GE_NONE;
	}
	return GE_UNKNOWNRESPONSE;
}

static GSM_Error OBEXGEN_PrivGetFilePart(GSM_StateMachine *s, GSM_File *File, bool FolderList)
{
	unsigned int 		Current = 0, Pos;
	GSM_Error		error;
	unsigned char 		req[2000], req2[200];

	s->Phone.Data.File 	= File;
	File->ReadOnly 		= false;
	File->Protected 	= false;
	File->Hidden		= false;
	File->System		= false;

	if (File->Used == 0x00) {
		if (FolderList) {
			/* Type block */
			strcpy(req2,"x-obex/folder-listing");
			OBEXAddBlock(req, &Current, 0x42, req2, strlen(req2)+1);
	
			/* Name block */
			if (UnicodeLength(File->Name) == 0x00) {
				OBEXAddBlock(req, &Current, 0x01, NULL, 0);
			} else {
				CopyUnicodeString(req2,File->Name);
				OBEXAddBlock(req, &Current, 0x01, req2, UnicodeLength(req2)*2+2);
			}
		} else {
			File->Folder = false;

			if (File->ID_FullName[0] == 0x00) {
#ifndef xxxx
				error = OBEXGEN_Connect(s,OBEX_None);
#else
				error = OBEXGEN_Connect(s,OBEX_BrowsingFolders);
#endif
				if (error != GE_NONE) return error;

				EncodeUnicode(File->Name,"one",3);
	
				strcpy(req2,"x-obex/capability");
//				strcpy(req2,"x-obex/object-profile");
				/* Type block */
				OBEXAddBlock(req, &Current, 0x42, req2, strlen(req2)+1);
			} else {
				error = OBEXGEN_Connect(s,OBEX_BrowsingFolders);
				if (error != GE_NONE) return error;

				smprintf(s,"Changing to root\n");
				error = OBEXGEN_ChangePath(s, NULL, 2);
				if (error != GE_NONE) return error;

				Pos = 0;
				do {
					OBEXGEN_FindNextDir(File->ID_FullName, &Pos, req2);
					smprintf(s,"%s %i %i\n",DecodeUnicodeString(req2),Pos,strlen(File->ID_FullName));
					if (Pos == strlen(File->ID_FullName)) break;
					smprintf(s,"Changing path down\n");
					error=OBEXGEN_ChangePath(s, req2, 2);
					if (error != GE_NONE) return error;
				} while (1);

				CopyUnicodeString(File->Name,req2);

				s->Phone.Data.File = File;
	
				Current = 0;
				/* Name block */
				OBEXAddBlock(req, &Current, 0x01, req2, UnicodeLength(req2)*2+2);
			}
		}
	}

	if (s->Phone.Data.Priv.OBEXGEN.Service == OBEX_BrowsingFolders) {
		/* connection ID block */
		req[Current++] = 0xCB; // ID
		req[Current++] = 0x00; req[Current++] = 0x00;
		req[Current++] = 0x00; req[Current++] = 0x01;
	}

	smprintf(s, "Getting file info from filesystem\n");
	error=GSM_WaitFor (s, req, Current, 0x03, 4, ID_GetFileInfo);
	if (error != GE_NONE) return error;

	s->Phone.Data.Priv.OBEXGEN.FileLastPart = false;

	while (!s->Phone.Data.Priv.OBEXGEN.FileLastPart) {
		Current = 0;
 	    	if (s->Phone.Data.Priv.OBEXGEN.Service == OBEX_BrowsingFolders) {
			/* connection ID block */
			req[Current++] = 0xCB; // ID
			req[Current++] = 0x00; req[Current++] = 0x00;
			req[Current++] = 0x00; req[Current++] = 0x01;
		}
		smprintf(s, "Getting file part from filesystem\n");
		error=GSM_WaitFor (s, req, Current, 0x83, 4, ID_GetFile);
		if (error != GE_NONE) return error;
	}
	return GE_EMPTY;
}

static GSM_Error OBEXGEN_GetFilePart(GSM_StateMachine *s, GSM_File *File)
{
	return OBEXGEN_PrivGetFilePart(s,File,false);
}

static GSM_Error OBEXGEN_GetNextFileFolder(GSM_StateMachine *s, GSM_File *File, bool start)
{
	GSM_Phone_OBEXGENData	*Priv = &s->Phone.Data.Priv.OBEXGEN;
	GSM_Error		error;
	unsigned char		Line[500],Line2[500],*name,*size;
	int			Pos,i,j,num,pos2,Current,z;

	if (start) {
		Priv->Files[0].Folder		= true;
		Priv->Files[0].Level		= 1;
		Priv->Files[0].Name[0]		= 0;
		Priv->Files[0].Name[1]		= 0;
		Priv->Files[0].ID_FullName[0]	= 0;
		Priv->Files[0].ID_FullName[1]	= 0;

		Priv->FilesLocationsUsed 	= 1;
		Priv->FilesLocationsCurrent 	= 0;
		Priv->FileLev			= 1;
		
		error = OBEXGEN_Connect(s,OBEX_BrowsingFolders);
		if (error != GE_NONE) return error;

		smprintf(s,"Changing to root\n");
		error = OBEXGEN_ChangePath(s, NULL, 2);
		if (error != GE_NONE) return error;

		Current = 0;
	}

	while (1) {
		if (Priv->FilesLocationsCurrent == Priv->FilesLocationsUsed) {
			dprintf("Last file\n");
			return GE_EMPTY;
		}

		strcpy(File->ID_FullName,Priv->Files[Priv->FilesLocationsCurrent].ID_FullName);
		File->Level	= Priv->Files[Priv->FilesLocationsCurrent].Level;
		File->Folder	= Priv->Files[Priv->FilesLocationsCurrent].Folder;
		CopyUnicodeString(File->Name,Priv->Files[Priv->FilesLocationsCurrent].Name);
		Priv->FilesLocationsCurrent++;

		if (File->Folder) {
			if (File->Level < Priv->FileLev) {
				for (i=0;i<File->Level;i++) {
					smprintf(s,"Changing path up\n");
					error=OBEXGEN_ChangePath(s, NULL, 2);
					if (error != GE_NONE) return error;
				}
			}
	
			smprintf(s,"Level %i %i\n",File->Level,Priv->FileLev);
	
			File->Buffer 	= NULL;		
			File->Used 	= 0;
			OBEXGEN_PrivGetFilePart(s, File,true);

			num = 0;
			Pos = 0;
			while (1) {
				MyGetLine(File->Buffer, &Pos, Line);
				if (strlen(Line) == 0) break;
				name = strstr(Line,"folder name=\"");
				if (name != NULL) {
					name += 13;
					j = 0;
					while(1) {
						if (name[j] == '"') break;
						j++;
					}
					name[j] = 0;
	
					if (strcmp(name,".")) num++;
				}
				name = strstr(Line,"file name=\"");
				if (name != NULL) num++;
			}
			if (num != 0) {
				i = Priv->FilesLocationsUsed-1;
				while (1) {
					if (i==Priv->FilesLocationsCurrent-1) break;
					memcpy(&Priv->Files[i+num],&Priv->Files[i],sizeof(GSM_File));
					i--;
				}
			}

			Pos 	= 0;
			pos2 	= 0;
			while (1) {
				MyGetLine(File->Buffer, &Pos, Line);
				if (strlen(Line) == 0) break;
				strcpy(Line2,Line);
				name = strstr(Line2,"folder name=\"");
				if (name != NULL) {
					name += 13;
					j = 0;
					while(1) {
						if (name[j] == '"') break;
						j++;
					}
					name[j] = 0;
					if (strcmp(name,".")) {
						dprintf("copying folder %s to %i parent %i\n",name,Priv->FilesLocationsCurrent+pos2,Priv->FilesLocationsCurrent);
						strcpy(Priv->Files[Priv->FilesLocationsCurrent+pos2].ID_FullName,File->ID_FullName);
						if (strlen(File->ID_FullName) != 0) strcat(Priv->Files[Priv->FilesLocationsCurrent+pos2].ID_FullName,"\\");
						strcat(Priv->Files[Priv->FilesLocationsCurrent+pos2].ID_FullName,name);
						Priv->Files[Priv->FilesLocationsCurrent+pos2].Level  = File->Level+1;
						Priv->Files[Priv->FilesLocationsCurrent+pos2].Folder = true;
						EncodeUnicode(Priv->Files[Priv->FilesLocationsCurrent+pos2].Name,name,strlen(name));
						Priv->FilesLocationsUsed++;
						pos2++;
					}
				}
				strcpy(Line2,Line);
				name = strstr(Line2,"file name=\"");
				if (name != NULL) {
					name += 11;
					j = 0;
					while(1) {
						if (name[j] == '"') break;
						j++;
					}
					name[j] = 0;
					dprintf("copying file %s to %i\n",name,Priv->FilesLocationsCurrent+pos2);
					Priv->Files[Priv->FilesLocationsCurrent+pos2].Level	= File->Level+1;
					Priv->Files[Priv->FilesLocationsCurrent+pos2].Folder 	= false;
					strcpy(Priv->Files[Priv->FilesLocationsCurrent+pos2].ID_FullName,File->ID_FullName);
					if (strlen(File->ID_FullName) != 0) strcat(Priv->Files[Priv->FilesLocationsCurrent+pos2].ID_FullName,"\\");
					strcat(Priv->Files[Priv->FilesLocationsCurrent+pos2].ID_FullName,name);
					EncodeUnicode(Priv->Files[Priv->FilesLocationsCurrent+pos2].Name,name,strlen(name));

					Priv->Files[Priv->FilesLocationsCurrent+pos2].Used 	= 0;
					strcpy(Line2,Line);
					size = strstr(Line2,"size=\"");
					if (size != NULL) Priv->Files[Priv->FilesLocationsCurrent+pos2].Used = atoi(size+6);

					Priv->Files[Priv->FilesLocationsCurrent+pos2].ModifiedEmpty = true;
					strcpy(Line2,Line);
					size = strstr(Line2,"modified=\"");
					if (size != NULL) {
						Priv->Files[Priv->FilesLocationsCurrent+pos2].ModifiedEmpty = false;
						ReadVCALDateTime(size+10, &Priv->Files[Priv->FilesLocationsCurrent+pos2].Modified);
					}
					Priv->FilesLocationsUsed++;
					pos2++;
				}
			}

			z = Priv->FilesLocationsCurrent;
			if (z != 1) {
				while (1) {
					if (z == Priv->FilesLocationsUsed) break;
					if (Priv->Files[z].Folder) {
						if (Priv->Files[z].Level > File->Level) {
							smprintf(s,"Changing path down\n");
							error=OBEXGEN_ChangePath(s, File->Name, 2);
							if (error != GE_NONE) return error;
						}
						break;
					} 
					z++;
				}
			}

			Priv->FileLev = File->Level;
			free(File->Buffer);
		} else {
			File->Used 	    	= Priv->Files[Priv->FilesLocationsCurrent-1].Used;
			File->ModifiedEmpty 	= Priv->Files[Priv->FilesLocationsCurrent-1].ModifiedEmpty;
			if (!File->ModifiedEmpty) {
				memcpy(&File->Modified,&Priv->Files[Priv->FilesLocationsCurrent-1].Modified,sizeof(GSM_DateTime));
			}
			File->ReadOnly 		= false;
			File->Protected 	= false;
			File->Hidden		= false;
			File->System		= false;

		}
		return GE_NONE;
	}
}

static GSM_Error OBEXGEN_DeleteFile(GSM_StateMachine *s, unsigned char *ID)
{
	GSM_Error		error;
	unsigned int		Current = 0, Pos;
	unsigned char		req[200],req2[200];

	error = OBEXGEN_Connect(s,OBEX_BrowsingFolders);
	if (error != GE_NONE) return error;

	smprintf(s,"Changing to root\n");
	error = OBEXGEN_ChangePath(s, NULL, 2);
	if (error != GE_NONE) return error;

	Pos = 0;
	do {
		OBEXGEN_FindNextDir(ID, &Pos, req2);
		smprintf(s,"%s %i %i\n",DecodeUnicodeString(req2),Pos,strlen(ID));
		if (Pos == strlen(ID)) break;
		smprintf(s,"Changing path down\n");
		error=OBEXGEN_ChangePath(s, req2, 2);
		if (error != GE_NONE) return error;
	} while (1);

	/* Name block */
	OBEXAddBlock(req, &Current, 0x01, req2, UnicodeLength(req2)*2+2);

	/* connection ID block */
	req[Current++] = 0xCB; // ID
	req[Current++] = 0x00; req[Current++] = 0x00;
	req[Current++] = 0x00; req[Current++] = 0x01;

	return GSM_WaitFor (s, req, Current, 0x82, 4, ID_AddFile);
}

static GSM_Error OBEXGEN_AddFolder(GSM_StateMachine *s, GSM_File *File)
{
	GSM_Error		error;
	unsigned char		req2[200];
	unsigned int		Pos;

	error = OBEXGEN_Connect(s,OBEX_BrowsingFolders);
	if (error != GE_NONE) return error;

	smprintf(s,"Changing to root\n");
	error = OBEXGEN_ChangePath(s, NULL, 2);
	if (error != GE_NONE) return error;

	Pos = 0;
	do {
		OBEXGEN_FindNextDir(File->ID_FullName, &Pos, req2);
		smprintf(s,"%s %i %i\n",DecodeUnicodeString(req2),Pos,strlen(File->ID_FullName));
		smprintf(s,"Changing path down\n");
		error=OBEXGEN_ChangePath(s, req2, 2);
		if (error != GE_NONE) return error;
		if (Pos == strlen(File->ID_FullName)) break;
	} while (1);

	smprintf(s,"Adding directory\n");
	return OBEXGEN_ChangePath(s, File->Name, 0);
}

static GSM_Reply_Function OBEXGENReplyFunctions[] = {
	/* CONTINUE block */
	{OBEXGEN_ReplyAddFilePart,	"\x90",0x00,0x00,ID_AddFile			},
	{OBEXGEN_ReplyGetFilePart,	"\x90",0x00,0x00,ID_GetFile			},
	{OBEXGEN_ReplyGetFileInfo,	"\x90",0x00,0x00,ID_GetFileInfo			},

	/* OK block */
	{OBEXGEN_ReplyChangePath,	"\xA0",0x00,0x00,ID_SetPath			},
	{OBEXGEN_ReplyConnect,		"\xA0",0x00,0x00,ID_Initialise			},
	{OBEXGEN_ReplyAddFilePart,	"\xA0",0x00,0x00,ID_AddFile			},
	{OBEXGEN_ReplyGetFilePart,	"\xA0",0x00,0x00,ID_GetFile			},

	/* FOLDER CREATED block */
	{OBEXGEN_ReplyChangePath,	"\xA1",0x00,0x00,ID_SetPath			},

	/* NOT UNDERSTAND block */
	{OBEXGEN_ReplyAddFilePart,	"\xC0",0x00,0x00,ID_AddFile			},

	/* FORBIDDEN block */
	{OBEXGEN_ReplyChangePath,	"\xC3",0x00,0x00,ID_SetPath			},
	{OBEXGEN_ReplyGetFilePart,	"\xC3",0x00,0x00,ID_GetFile			},

	/* NOT FOUND block */
	{OBEXGEN_ReplyGetFilePart,	"\xC4",0x00,0x00,ID_GetFile			},

	{NULL,				"\x00",0x00,0x00,ID_None			}
};

GSM_Phone_Functions OBEXGENPhone = {
	"obex",
	OBEXGENReplyFunctions,
	OBEXGEN_Initialise,
	NONEFUNCTION,			/*	Terminate 		*/
	GSM_DispatchMessage,
	NONEFUNCTION,
	NONEFUNCTION,
	NOTIMPLEMENTED,			/*	GetIMEI			*/
	NOTIMPLEMENTED,			/*	GetDateTime		*/
	NOTIMPLEMENTED,			/*	GetAlarm		*/
	NOTIMPLEMENTED,			/*	GetMemory		*/
	NOTIMPLEMENTED,			/*	GetMemoryStatus		*/
	NOTIMPLEMENTED,			/*	GetSMSC			*/
	NOTIMPLEMENTED,			/*	GetSMSMessage		*/
	NOTIMPLEMENTED,			/*	GetSMSFolders		*/
	NONEFUNCTION,
	NOTIMPLEMENTED,			/*	GetNextSMSMessage	*/
	NOTIMPLEMENTED,			/*	GetSMSStatus		*/
	NOTIMPLEMENTED,			/*	SetIncomingSMS		*/
	NOTIMPLEMENTED,			/*	GetNetworkInfo		*/
	NOTIMPLEMENTED,			/*	Reset			*/
	NOTIMPLEMENTED,			/*	DialVoice		*/
	NOTIMPLEMENTED,			/*	AnswerCall		*/
	NOTIMPLEMENTED,			/*	CancelCall		*/
	NOTIMPLEMENTED,			/*	GetRingtone		*/
	NOTIMPLEMENTED,			/*	GetWAPBookmark		*/
	NOTIMPLEMENTED,			/*	GetBitmap		*/
	NOTIMPLEMENTED,			/*	SetRingtone		*/
	NOTIMPLEMENTED,			/*	SaveSMSMessage		*/
	NOTIMPLEMENTED,			/*	SendSMSMessage		*/
	NOTIMPLEMENTED,			/*	SetDateTime		*/
	NOTIMPLEMENTED,			/*	SetAlarm		*/
	NOTIMPLEMENTED,			/*	SetBitmap		*/
	NOTIMPLEMENTED,			/* 	SetMemory 		*/
	NOTIMPLEMENTED,			/* 	DeleteSMS 		*/
	NOTIMPLEMENTED,			/* 	SetWAPBookmark 		*/
	NOTIMPLEMENTED, 		/* 	DeleteWAPBookmark 	*/
	NOTIMPLEMENTED,			/* 	GetWAPSettings 		*/
	NOTIMPLEMENTED,			/* 	SetIncomingCB		*/
	NOTIMPLEMENTED,			/*	SetSMSC			*/
	NOTIMPLEMENTED,			/*	GetManufactureMonth	*/
	NOTIMPLEMENTED,			/*	GetProductCode		*/
	NOTIMPLEMENTED,			/*	GetOriginalIMEI		*/
	NOTIMPLEMENTED,			/*	GetHardware		*/
	NOTIMPLEMENTED,			/*	GetPPM			*/
	NOTIMPLEMENTED,			/*	PressKey		*/
	NOTIMPLEMENTED,			/*	GetToDo			*/
	NOTIMPLEMENTED,			/*	DeleteAllToDo		*/
	NOTIMPLEMENTED,			/*	SetToDo			*/
	NOTIMPLEMENTED,			/*	GetToDoStatus		*/
	NOTIMPLEMENTED,			/*	PlayTone		*/
	NOTIMPLEMENTED,			/*	EnterSecurityCode	*/
	NOTIMPLEMENTED,			/*	GetSecurityStatus	*/
	NOTIMPLEMENTED, 		/*	GetProfile		*/
	NOTIMPLEMENTED,			/*	GetRingtonesInfo	*/
	NOTIMPLEMENTED,			/* 	SetWAPSettings 		*/
	NOTIMPLEMENTED,			/*	GetSpeedDial		*/
	NOTIMPLEMENTED,			/*	SetSpeedDial		*/
	NOTIMPLEMENTED,			/*	ResetPhoneSettings	*/
	NOTIMPLEMENTED,			/*	SendDTMF		*/
	NOTIMPLEMENTED,			/*	GetDisplayStatus	*/
	NOTIMPLEMENTED,			/*	SetAutoNetworkLogin	*/
	NOTIMPLEMENTED, 		/*	SetProfile		*/
	NOTIMPLEMENTED,			/*	GetSIMIMSI		*/
	NOTIMPLEMENTED,			/*	SetIncomingCall		*/
    	NOTIMPLEMENTED,			/*	GetNextCalendar		*/
	NOTIMPLEMENTED,   		/*	DelCalendar		*/
	NOTIMPLEMENTED,       		/*	AddCalendar		*/
	NOTIMPLEMENTED,			/*	GetBatteryCharge	*/
	NOTIMPLEMENTED,			/*	GetSignalQuality	*/
	NOTIMPLEMENTED,     		/*  	GetCategory 		*/
        NOTIMPLEMENTED,      		/*  	GetCategoryStatus 	*/	
    	NOTIMPLEMENTED,			/*  	GetFMStation        	*/
    	NOTIMPLEMENTED,			/*  	SetFMStation        	*/
    	NOTIMPLEMENTED,			/*  	ClearFMStations       	*/
	NOTIMPLEMENTED,			/*  	SetIncomingUSSD		*/
	NOTIMPLEMENTED,			/* 	DeleteUserRingtones	*/
	NOTIMPLEMENTED,			/* 	ShowStartInfo		*/
	OBEXGEN_GetNextFileFolder,
	OBEXGEN_GetFilePart,
	OBEXGEN_AddFilePart,
	NOTIMPLEMENTED, 		/* 	GetFileSystemStatus	*/
	OBEXGEN_DeleteFile,
	OBEXGEN_AddFolder,
	NOTIMPLEMENTED,			/* 	GetMMSSettings		*/
 	NOTIMPLEMENTED,			/* 	SetMMSSettings		*/
 	NOTIMPLEMENTED,			/* 	HoldCall 		*/
 	NOTIMPLEMENTED,			/* 	UnholdCall 		*/
 	NOTIMPLEMENTED,			/* 	ConferenceCall 		*/
 	NOTIMPLEMENTED,			/* 	SplitCall		*/
 	NOTIMPLEMENTED,			/* 	TransferCall		*/
 	NOTIMPLEMENTED,			/* 	SwitchCall		*/
 	NOTIMPLEMENTED,			/* 	GetCallDivert		*/
 	NOTIMPLEMENTED,			/* 	SetCallDivert		*/
 	NOTIMPLEMENTED,			/* 	CancelAllDiverts	*/
 	NOTIMPLEMENTED,			/* 	AddSMSFolder		*/
 	NOTIMPLEMENTED,			/* 	DeleteSMSFolder		*/
	NOTIMPLEMENTED,			/* 	GetGPRSAccessPoint	*/
	NOTIMPLEMENTED,			/* 	SetGPRSAccessPoint	*/
	NOTSUPPORTED,			/* 	GetLocale		*/
	NOTSUPPORTED,			/* 	SetLocale		*/
	NOTSUPPORTED,			/* 	GetCalendarSettings	*/
	NOTSUPPORTED			/* 	SetCalendarSettings	*/
};

#endif

/* How should editor hadle tabs in this file? Add editor commands here.
 * vim: noexpandtab sw=8 ts=8 sts=8:
 */
