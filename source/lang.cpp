#include <switch.h>
#include "string.h"
#include "stdio.h"
#include "config.h"
#include "util.h"
#include "lang.h"
#include "fs.h"

char lang_identifiers[LANG_STRINGS_NUM][LANG_ID_SIZE] = {
	FOREACH_STR(GET_STRING)};

// This is properly populated so that emulator won't crash if an user launches it without language INI files.
char lang_strings[LANG_STRINGS_NUM][LANG_STR_SIZE] = {
	"Connection Settings",																	// STR_CONNECTION_SETTINGS
	"Site",																					// STR_SITE
	"Local",																				// STR_LOCAL
	"Remote",																				// STR_REMOTE
	"Messages",																				// STR_MESSAGES
	"Update Software",																		// STR_UPDATE_SOFTWARE
	"Connect",																				// STR_CONNECT
	"Disconnect",																			// STR_DISCONNECT
	"Search",																				// STR_SEARCH
	"Refresh",																				// STR_REFRESH
	"Server",																				// STR_SERVER
	"Username",																				// STR_USERNAME
	"Password",																				// STR_PASSWORD
	"Port",																					// STR_PORT
	"Pasv",																					// STR_PASV
	"Directory",																			// STR_DIRECTORY
	"Filter",																				// STR_FILTER
	"Yes",																					// STR_YES
	"No",																					// STR_NO
	"Cancel",																				// STR_CANCEL
	"Continue",																				// STR_CONTINUE
	"Close",																				// STR_CLOSE
	"Folder",																				// STR_FOLDER
	"File",																					// STR_FILE
	"Type",																					// STR_TYPE
	"Name",																					// STR_NAME
	"Size",																					// STR_SIZE
	"Date",																					// STR_DATE
	"New Folder",																			// STR_NEW_FOLDER
	"Rename",																				// STR_RENAME
	"Delete",																				// STR_DELETE
	"Upload",																				// STR_UPLOAD
	"Download",																				// STR_DOWNLOAD
	"Select All",																			// STR_SELECT_ALL
	"Clear All",																			// STR_CLEAR_ALL
	"Uploading",																			// STR_UPLOADING
	"Downloading",																			// STR_DOWNLOADING
	"Overwrite",																			// STR_OVERWRITE
	"Don't Overwrite",																		// STR_DONT_OVERWRITE
	"Ask for Confirmation",																	// STR_ASK_FOR_CONFIRM
	"Don't Ask for Confirmation",															// STR_DONT_ASK_CONFIRM
	"Always use this option and don't ask again",											// STR_ALLWAYS_USE_OPTION
	"Actions",																				// STR_ACTIONS
	"Confirm",																				// STR_CONFIRM
	"Overwrite Options",																	// STR_OVERWRITE_OPTIONS
	"Properties",																			// STR_PROPERTIES
	"Progress",																				// STR_PROGRESS
	"Updates",																				// STR_UPDATES
	"Are you sure you want to delete this file(s)/folder(s)?",								// STR_DEL_CONFIRM_MSG
	"Canceling. Waiting for last action to complete",										// STR_CANCEL_ACTION_MSG
	"Failed to upload file",																// STR_FAIL_UPLOAD_MSG
	"Failed to download file",																// STR_FAIL_DOWNLOAD_MSG
	"Failed to read contents of directory or folder does not exist.",						// STR_FAIL_READ_LOCAL_DIR_MSG
	"426 Connection closed.",																// STR_CONNECTION_CLOSE_ERR_MSG
	"426 Remote Server has terminated the connection.",										// STR_REMOTE_TERM_CONN_MSG
	"300 Failed Login. Please check your username or password.",							// STR_FAIL_LOGIN_MSG
	"426 Failed. Connection timeout.",														// STR_FAIL_TIMEOUT_MSG
	"Failed to delete directory",															// STR_FAIL_DEL_DIR_MSG
	"Deleting",																				// STR_DELETING
	"Failed to delete file",																// STR_FAIL_DEL_FILE_MSG
	"Deleted",																				// STR_DELETED
	"Link",																					// STR_LINK
	"Share",																				// STR_SHARE
	"310 Failed",																			// STR_FAILED
	"310 Failed to create file on local",													// STR_FAIL_CREATE_LOCAL_FILE_MSG
	"Install",																				// STR_INSTALL
	"Installing",																			// STR_INSTALLING
	"Success",																				// STR_INSTALL_SUCCESS
	"Failed",																				// STR_INSTALL_FAILED
	"Skipped",																				// STR_INSTALL_SKIPPED
	"Checking connection to remote HTTP Server",											// STR_CHECK_HTTP_MSG
	"Failed connecting to HTTP Server",														// STR_FAILED_HTTP_CHECK
	"Remote is not a HTTP Server",															// STR_REMOTE_NOT_HTTP
	"Package not in the /data or /mnt/usbX folder",											// STR_INSTALL_FROM_DATA_MSG
	"Package is already installed",															// STR_ALREADY_INSTALLED_MSG
	"Install from URL",																		// STR_INSTALL_FROM_URL
	"Could not read package header info",													// STR_CANNOT_READ_PKG_HDR_MSG
	"Favorite URLs",																		// STR_FAVORITE_URLS
	"Slot",																					// STR_SLOT
	"Edit",																					// STR_EDIT
	"One Time Url",																			// STR_ONETIME_URL
	"Not a valid Package",																	// STR_NOT_A_VALID_PACKAGE
	"Waiting for Package to finish installing",												// STR_WAIT_FOR_INSTALL_MSG
	"Failed to install pkg file. Please delete the tmp pkg manually",						// STR_FAIL_INSTALL_TMP_PKG_MSG
	"Auto delete temporary downloaded pkg file after install",								// STR_AUTO_DELETE_TMP_PKG
	"Protocol not supported",																// STR_PROTOCOL_NOT_SUPPORTED
	"Could not resolve hostname",															// STR_COULD_NOT_RESOLVE_HOST
	"Extract",																				// STR_EXTRACT
	"Extracting",																			// STR_EXTRACTING
	"Failed to extract",																	// STR_FAILED_TO_EXTRACT
	"Extract Location",																		// STR_EXTRACT_LOCATION
	"Compress",																				// STR_COMPRESS
	"Zip Filename",																			// STR_ZIP_FILE_PATH
	"Compressing",																			// STR_COMPRESSING
	"Error occured while creating zip",														// STR_ERROR_CREATE_ZIP
	"Unsupported compressed file format",													// STR_UNSUPPORTED_FILE_FORMAT
	"Cut",																					// STR_CUT
	"Copy",																					// STR_COPY
	"Paste",																				// STR_PASTE
	"Moving",																				// STR_MOVING
	"Copying",																				// STR_COPYING
	"Failed to move file",																	// STR_FAIL_MOVE_MSG
	"Failed to copy file",																	// STR_FAIL_COPY_MSG
	"Cannot move parent directory to sub subdirectory",										// STR_CANT_MOVE_TO_SUBDIR_MSG
	"Cannot copy parent directory to sub subdirectory",										// STR_CANT_COPY_TO_SUBDIR_MSG
	"Operation not supported",																// STR_UNSUPPORTED_OPERATION_MSG
	"Http Port",																			// STR_HTTP_PORT
	"The content has already been installed. Do you want to continue installing",			// STR_REINSTALL_CONFIRM_MSG
	"Remote package installation is not supported for protected servers.",					// STR_REMOTE_NOT_SUPPORT_MSG
	"Remote HTTP Server not reachable.",													// STR_CANNOT_CONNECT_REMOTE_MSG
	"Remote Package Install not possible. Would you like to download package and install?", // STR_DOWNLOAD_INSTALL_MSG
	"Checking remote server for Remote Package Install.",									// STR_CHECKING_REMOTE_SERVER_MSG
	"Files",																				// STR_FILES
	"Editor",																				// STR_EDITOR
	"Save",																					// STR_SAVE
	"Cannot edit files bigger than",														// STR_MAX_EDIT_FILE_SIZE_MSG
	"Delete Selected Line",																	// STR_DELETE_LINE
	"Insert Below Selected Line",															// STR_INSERT_LINE
	"Modified",																				// STR_MODIFIED
	"Failed to obtain an access token from",												// STR_FAIL_GET_TOKEN_MSG
	"Login Success. You may close the browser and return to the application",				// STR_GET_TOKEN_SUCCESS_MSG
	"New File",																				// STR_NEW_FILE
	"Settings",																				// STR_SETTINGS
	"Global",																				// STR_GLOBAL
	"Copy selected line",																	// STR_COPY_LINE
	"Paste into selected line",																// STR_PASTE_LINE
	"Show hidden files",																	// STR_SHOW_HIDDEN_FILES
	"Set Default Folder",																	// STR_SET_DEFAULT_DIRECTORY
	"has being set as default direcotry",													// STR_SET_DEFAULT_DIRECTORY_MSG
	"NFS export path missing in URL",														// STR_NFS_EXP_PATH_MISSING_MSG
	"Failed to init NFS context",															// STR_FAIL_INIT_NFS_CONTEXT
	"Failed to mount NFS share",															// STR_FAIL_MOUNT_NFS_MSG
	"View Image",																			// STR_VIEW_IMAGE
	"Language",                                                                             // STR_LANGUAGE
};

bool needs_extended_font = false;

namespace Lang
{
	void SetTranslation(SetLanguage lang_code)
	{
		char langFile[LANG_STR_SIZE * 2];
		char identifier[LANG_ID_SIZE], buffer[LANG_STR_SIZE];

		std::string lang = std::string(language);
		lang = Util::Trim(lang, " ");
		if (lang.size() > 0)
		{
			sprintf(langFile, "romfs:/lang/%s.ini", lang.c_str());
		}
		else
		{
			switch (lang_code)
			{
			case 0:
				sprintf(langFile, "romfs:/lang/Japanese.ini");
				break;
			case 2:
			case 13:
				sprintf(langFile, "romfs:/lang/French.ini");
				break;
			case 3:
				sprintf(langFile, "romfs:/lang/German.ini");
				break;
			case 4:
				sprintf(langFile, "romfs:/lang/Italiano.ini");
				break;
			case 5:
			case 14:
				sprintf(langFile, "romfs:/lang/Spanish.ini");
				break;
			case 6:
			case 15:
				sprintf(langFile, "romfs:/lang/Simplified Chinese.ini");
				break;
			case 7:
				sprintf(langFile, "romfs:/lang/Korean.ini");
				break;
			case 8:
				sprintf(langFile, "romfs:/lang/Dutch.ini");
				break;
			case 9:
			case 17:
				sprintf(langFile, "romfs:/lang/Portuguese_BR.ini");
				break;
			case 10:
				sprintf(langFile, "romfs:/lang/Russian.ini");
				break;
			case 11:
			case 16:
				sprintf(langFile, "romfs:/lang/Traditional Chinese.ini");
				break;
			default:
				sprintf(langFile, "romfs:/lang/English.ini");
				break;
			}
		}

		FILE *config = fopen(langFile, "r");
		if (config)
		{
			while (EOF != fscanf(config, "%[^=]=%[^\n]\n", identifier, buffer))
			{
				for (int i = 0; i < LANG_STRINGS_NUM; i++)
				{
					if (strcmp(lang_identifiers[i], identifier) == 0)
					{
						char *newline = nullptr, *p = buffer;
						while (newline = strstr(p, "\\n"))
						{
							newline[0] = '\n';
							int len = strlen(&newline[2]);
							memmove(&newline[1], &newline[2], len);
							newline[len + 1] = 0;
							p++;
						}
						strcpy(lang_strings[i], buffer);
					}
				}
			}
			fclose(config);
		}

		char buf[12];
		int num;
		sscanf(last_site, "%[^ ] %d", buf, &num);
		sprintf(display_site, "%s %d", lang_strings[STR_SITE], num);
	}
}
