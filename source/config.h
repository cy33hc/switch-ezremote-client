#ifndef LAUNCHER_CONFIG_H
#define LAUNCHER_CONFIG_H

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include "clients/remote_client.h"

#define APP_ID "ezremote-client"
#define DATA_PATH "/switch/" APP_ID
#define CONFIG_INI_FILE DATA_PATH "/config.ini"
#define TMP_EDITOR_FILE DATA_PATH "/tmp_editor.txt"

#define CONFIG_GLOBAL "Global"

#define CONFIG_DEFAULT_STYLE_NAME "Default"
#define CONFIG_SWAP_XO "swap_xo"
#define CONFIG_MAX_EDIT_FILE_SIZE "max_edit_file_size"

#define CONFIG_REMOTE_SERVER "remote_server"
#define CONFIG_REMOTE_SERVER_USER "remote_server_user"
#define CONFIG_REMOTE_SERVER_PASSWORD "remote_server_password"

#define CONFIG_LAST_SITE "last_site"

#define CONFIG_LOCAL_DIRECTORY "local_directory"
#define CONFIG_REMOTE_DIRECTORY "remote_directory"

#define CONFIG_LANGUAGE "language"
#define MAX_EDIT_FILE_SIZE 32768

struct RemoteSettings
{
    char site_name[32];
    char server[256];
    char username[33];
    char password[128];
    ClientType type;
};

extern bool swap_xo;
extern std::vector<std::string> sites;
extern std::map<std::string, RemoteSettings> site_settings;
extern std::set<std::string> text_file_extensions;
extern char local_directory[255];
extern char remote_directory[255];
extern char app_ver[6];
extern char last_site[32];
extern char display_site[32];
extern char language[128];
extern RemoteSettings *remote_settings;
extern RemoteClient *remoteclient;
extern int max_edit_file_size;

namespace CONFIG
{
    void LoadConfig();
    void SaveConfig();
    void SetClientType(RemoteSettings *settings);
}
#endif
