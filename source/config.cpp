#include <string>
#include <cstring>
#include <map>

#include "config.h"
#include "fs.h"
#include "lang.h"

extern "C"
{
#include "inifile.h"
}

bool swap_xo;
char local_directory[255];
char remote_directory[255];
char app_ver[6];
char last_site[32];
char display_site[32];
char language[128];
int max_edit_file_size;
std::vector<std::string> sites;
std::map<std::string, RemoteSettings> site_settings;
std::set<std::string> text_file_extensions;
std::set<std::string> image_file_extensions;
RemoteSettings *remote_settings;
RemoteClient *remoteclient;
std::vector<std::string> http_servers;
std::vector<std::string> langs;

namespace CONFIG
{

    void LoadConfig()
    {
        if (!FS::FolderExists(DATA_PATH))
        {
            FS::MkDirs(DATA_PATH);
        }

        http_servers = {HTTP_SERVER_APACHE, HTTP_SERVER_MS_IIS, HTTP_SERVER_NGINX, HTTP_SERVER_NPX_SERVE, HTTP_SERVER_RCLONE, HTTP_SERVER_ARCHIVEORG, HTTP_SERVER_MYRIENT, HTTP_SERVER_GITHUB};
        sites = {"Site 1", "Site 2", "Site 3", "Site 4", "Site 5", "Site 6", "Site 7", "Site 8", "Site 9", "Site 10",
                 "Site 11", "Site 12", "Site 13", "Site 14", "Site 15", "Site 16", "Site 17", "Site 18", "Site 19", "Site 20"};
        text_file_extensions = { ".txt", ".ini", ".log", ".json", ".xml", ".html", ".xhtml", ".conf", ".config" };
        image_file_extensions = { ".gif", ".bmp", ".jpg", ".jpeg", ".png", ".webp" };
        langs = { "Default", "Arabic", "Catalan", "Croatian", "Dutch", "English", "Euskera", "French", "Galego", "German", "Greek", 
                  "Hungarian", "Indonesian", "Italiano", "Japanese", "Korean", "Polish", "Portuguese_BR", "Russian", "Romanian", "Ryukyuan", "Spanish", "Turkish",
                  "Simplified Chinese", "Traditional Chinese", "Thai", "Ukrainian","Vietnamese"};

        OpenIniFile(CONFIG_INI_FILE);

        // Load global config
        swap_xo = ReadBool(CONFIG_GLOBAL, CONFIG_SWAP_XO, false);
        WriteBool(CONFIG_GLOBAL, CONFIG_SWAP_XO, swap_xo);

        sprintf(language, "%s", ReadString(CONFIG_GLOBAL, CONFIG_LANGUAGE, ""));
        WriteString(CONFIG_GLOBAL, CONFIG_LANGUAGE, language);

        sprintf(local_directory, "%s", ReadString(CONFIG_GLOBAL, CONFIG_LOCAL_DIRECTORY, "/"));
        WriteString(CONFIG_GLOBAL, CONFIG_LOCAL_DIRECTORY, local_directory);

        sprintf(remote_directory, "%s", ReadString(CONFIG_GLOBAL, CONFIG_REMOTE_DIRECTORY, "/"));
        WriteString(CONFIG_GLOBAL, CONFIG_REMOTE_DIRECTORY, remote_directory);

        max_edit_file_size = ReadInt(CONFIG_GLOBAL, CONFIG_MAX_EDIT_FILE_SIZE, MAX_EDIT_FILE_SIZE);
        WriteInt(CONFIG_GLOBAL, CONFIG_MAX_EDIT_FILE_SIZE, max_edit_file_size);

        for (int i = 0; i < sites.size(); i++)
        {
            RemoteSettings setting;
            sprintf(setting.site_name, "%s", sites[i].c_str());

            sprintf(setting.server, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_SERVER, ""));
            WriteString(sites[i].c_str(), CONFIG_REMOTE_SERVER, setting.server);

            sprintf(setting.username, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_SERVER_USER, ""));
            WriteString(sites[i].c_str(), CONFIG_REMOTE_SERVER_USER, setting.username);

            sprintf(setting.password, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_SERVER_PASSWORD, ""));
            WriteString(sites[i].c_str(), CONFIG_REMOTE_SERVER_PASSWORD, setting.password);

            sprintf(setting.http_server_type, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_HTTP_SERVER_TYPE, HTTP_SERVER_APACHE));
            WriteString(sites[i].c_str(), CONFIG_REMOTE_HTTP_SERVER_TYPE, setting.http_server_type);

            SetClientType(&setting);
            site_settings.insert(std::make_pair(sites[i], setting));
        }

        sprintf(last_site, "%s", ReadString(CONFIG_GLOBAL, CONFIG_LAST_SITE, sites[0].c_str()));
        WriteString(CONFIG_GLOBAL, CONFIG_LAST_SITE, last_site);

        remote_settings = &site_settings[std::string(last_site)];

        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void SaveConfig()
    {
        OpenIniFile(CONFIG_INI_FILE);

        WriteString(last_site, CONFIG_REMOTE_SERVER, remote_settings->server);
        WriteString(last_site, CONFIG_REMOTE_SERVER_USER, remote_settings->username);
        WriteString(last_site, CONFIG_REMOTE_SERVER_PASSWORD, remote_settings->password);
        WriteString(last_site, CONFIG_REMOTE_HTTP_SERVER_TYPE, remote_settings->http_server_type);
        WriteString(CONFIG_GLOBAL, CONFIG_LAST_SITE, last_site);
        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void SaveGlobalConfig()
    {
        OpenIniFile(CONFIG_INI_FILE);

        WriteString(CONFIG_GLOBAL, CONFIG_LANGUAGE, language);

        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void SetClientType(RemoteSettings *setting)
    {
        if (strncmp(setting->server, "smb://", 6) == 0)
        {
            setting->type = CLIENT_TYPE_SMB;
        }
        else if (strncmp(setting->server, "ftp://", 6) == 0)
        {
            setting->type = CLIENT_TYPE_FTP;
        }
        else if (strncmp(setting->server, "webdav://", 9) == 0 || strncmp(setting->server, "webdavs://", 10) == 0)
        {
            setting->type = CLIENT_TYPE_WEBDAV;
        }
        else if (strncmp(setting->server, "http://", 7) == 0 || strncmp(setting->server, "https://", 8) == 0)
        {
            setting->type = CLIENT_TYPE_HTTP_SERVER;
        }
        else if (strncmp(setting->server, "nfs://", 6) == 0)
        {
            setting->type = CLIENT_TYPE_NFS;
        }
        else
        {
            setting->type = CLINET_TYPE_UNKNOWN;
        }
    }
}
