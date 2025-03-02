#include <curl/curl.h>
#include <json-c/json.h>
#include <fstream>
#include <algorithm>
#include "common.h"
#include "clients/remote_client.h"
#include "clients/github.h"
#include "common.h"
#include "config.h"
#include "lang.h"
#include "util.h"
#include "windows.h"

int GithubClient::Connect(const std::string &url, const std::string &username, const std::string &password)
{
    if (url.find("https://github.com") == std::string::npos)
        return 0;

    this->host_url = "https://api.github.com";
    this->base_path = "/repos" + url.substr(18);
    Util::Rtrim(this->base_path, "/");
    this->base_path += "/releases";
    this->m_download_url = "https://github.com";


    client = new CHTTPClient([](const std::string& log){});
    client->SetBasicAuth(username, password);
    client->InitSession(true, CHTTPClient::SettingsFlag::NO_FLAGS);
    client->SetCertificateFile(CACERT_FILE);

    if (Ping())
        this->connected = true;
    return 1;
}

std::vector<DirEntry> GithubClient::ListDir(const std::string &path)
{
    std::vector<DirEntry> out;
    DirEntry entry;
    Util::SetupPreviousFolder(path, &entry);
    out.push_back(entry);

    if (!ParseReleases())
        return out;

    if (path.compare("/") == 0) // return releases as folders
    {
        for (std::vector<GitRelease>::iterator release = m_releases.begin(); release != m_releases.end();)
        {
            DirEntry entry;
            entry.isDir = true;
            entry.selectable = true;
            entry.file_size = 0;
            snprintf(entry.directory, 512, "%s", "/");
            snprintf(entry.name, 256, "%s", release->name.c_str());
            snprintf(entry.path, 768, "/%s", release->name.c_str());
            snprintf(entry.display_size, 48, "%s", lang_strings[STR_FOLDER]);
            entry.modified = release->modified;
            
            out.push_back(entry);
            release++;
        }
    }
    else // return assets in the releases matching the path
    {
        std::string tag_name = path.substr(1);
        std::map<std::string, GitAsset> assets = m_assets[tag_name];
        for (std::map<std::string, GitAsset>::iterator asset = assets.begin(); asset != assets.end();)
        {
            DirEntry entry;
            memset(&entry, 0, sizeof(DirEntry));
            entry.isDir = false;
            entry.selectable = true;
            snprintf(entry.directory, 512, "%s", path.c_str());
            snprintf(entry.name, 256, "%s", asset->second.name.c_str());
            snprintf(entry.path, 768, "%s/%s", path.c_str(), asset->second.name.c_str());
            entry.file_size = asset->second.size;
            entry.modified = asset->second.modified;
            DirEntry::SetDisplaySize(&entry);

            out.push_back(entry);
            asset++;
        }
    }

    return out;
}

int GithubClient::Size(const std::string &path, int64_t *size)
{
    if (!ParseReleases())
        return 0;

    std::vector<std::string> path_parts = Util::Split(path, "/");
    
    if (path_parts.size() != 2)
    {
        return 0;
    }

    *size = m_assets[path_parts[0]][path_parts[1]].size;

    return 1;
}

bool GithubClient::FileExists(const std::string &path)
{
    uint64_t file_size;
    return Size(path, &file_size);
}

int GithubClient::Head(const std::string &path, void *buffer, uint64_t size)
{
    if (!ParseReleases())
        return 0;

    std::vector<std::string> path_parts = Util::Split(path, "/");
    
    if (path_parts.size() != 2)
    {
        return 0;
    }

    CHTTPClient::HttpResponse res;
    CHTTPClient::HeadersMap headers;

    char range_header[128];
    sprintf(range_header, "bytes=%lu-%lu", 0L, size - 1);
    headers["Range"] = range_header;

    std::string encoded_url = this->m_download_url + CHTTPClient::EncodeUrl(m_assets[path_parts[0]][path_parts[1]].url);
    if (client->Get(encoded_url, headers, res))
    {
        uint64_t len = MIN(size, res.strBody.size());
        memcpy(buffer, res.strBody.data(), len);
        return 1;
    }
    else
    {
        sprintf(this->response, "%s", res.errMessage.c_str());
    }
    return 0;
}

int GithubClient::Get(const std::string &outputfile, const std::string &path, uint64_t offset)
{
    if (!ParseReleases())
        return 0;

    std::vector<std::string> path_parts = Util::Split(path, "/");
    
    if (path_parts.size() != 2)
    {
        return 0;
    }

    long status;
    bytes_transfered = 0;
    prev_tick = Util::GetTick();
    CHTTPClient::HeadersMap headers;

    if (!Size(path, &bytes_to_download))
    {
        sprintf(this->response, "%s", lang_strings[STR_FAIL_DOWNLOAD_MSG]);
        return 0;
    }

    client->SetProgressFnCallback(&bytes_transfered, DownloadProgressCallback);
    std::string encoded_url = this->m_download_url + CHTTPClient::EncodeUrl(m_assets[path_parts[0]][path_parts[1]].url);
    if (client->DownloadFile(outputfile, encoded_url, status))
    {
        return 1;
    }
    else
    {
        sprintf(this->response, "%ld - %s", status, lang_strings[STR_FAIL_DOWNLOAD_MSG]);
    }
    return 0;
}

int GithubClient::GetRange(const std::string &path, void *buffer, uint64_t size, uint64_t offset)
{
    if (!ParseReleases())
        return 0;

    std::vector<std::string> path_parts = Util::Split(path, "/");
    
    if (path_parts.size() != 2)
    {
        return 0;
    }

    CHTTPClient::HttpResponse res;
    CHTTPClient::HeadersMap headers;

    char range_header[128];
    sprintf(range_header, "bytes=%lu-%lu", offset, offset + size - 1);
    headers["Range"] = range_header;

    std::string encoded_url = this->m_download_url + CHTTPClient::EncodeUrl(m_assets[path_parts[0]][path_parts[1]].url);
    if (client->Get(encoded_url, headers, res))
    {
        uint64_t len = MIN(size, res.strBody.size());
        memcpy(buffer, res.strBody.data(), len);
        return 1;
    }
    else
    {
        sprintf(this->response, "%s", res.errMessage.c_str());
    }
    return 0;
}

bool GithubClient::ParseReleases()
{
    CHTTPClient::HeadersMap headers;
    CHTTPClient::HttpResponse res;

    if (!releases_parsed)
    {
        std::string encoded_url = this->host_url + this->base_path + "?per_page=100&page=1";
        client->SetProgressFnCallback(&bytes_transfered, DownloadProgressCallback);
        if (client->Get(encoded_url, headers, res))
        {
            if (HTTP_SUCCESS(res.iCode))
            {
                json_object *jobj = json_tokener_parse(res.strBody.data());
                struct array_list *areleases = json_object_get_array(jobj);

                for (size_t release_idx = 0; release_idx < areleases->length; ++release_idx)
                {
                    GitRelease release_entry;

                    json_object *release = (json_object *)array_list_get_idx(areleases, release_idx);
                    release_entry.name = std::string(json_object_get_string(json_object_object_get(release, "tag_name")));
                    std::string date_time = std::string(json_object_get_string(json_object_object_get(release, "published_at")));

                    auto date_time_array = Util::Split(date_time, "T");
                    auto date_array = Util::Split(date_time_array[0], "-");
                    auto time_array = Util::Split(date_time_array[1], ":");
                    release_entry.modified.year = std::atoi(date_array[0].c_str());
                    release_entry.modified.month = std::atoi(date_array[1].c_str());
                    release_entry.modified.day = std::atoi(date_array[2].c_str());
                    release_entry.modified.hours = std::atoi(time_array[0].c_str());
                    release_entry.modified.minutes = std::atoi(time_array[1].c_str());
                    release_entry.modified.seconds = std::atoi(time_array[2].substr(0,2).c_str());

                    json_object *obj_assets = json_object_object_get(release, "assets");
                    if (json_object_get_type(obj_assets) == json_type_array)
                    {
                        struct array_list *aassets = json_object_get_array(obj_assets);
                        std::map<std::string, GitAsset> assets;

                        for (size_t asset_idx = 0; asset_idx < aassets->length; ++asset_idx)
                        {
                            GitAsset asset_entry;

                            json_object *asset = (json_object *)array_list_get_idx(aassets, asset_idx);
                            asset_entry.name = std::string(json_object_get_string(json_object_object_get(asset, "name")));
                            asset_entry.size = json_object_get_int64(json_object_object_get(asset, "size"));
                            std::string date_time = std::string(json_object_get_string(json_object_object_get(asset, "updated_at")));
                            asset_entry.url = std::string(json_object_get_string(json_object_object_get(asset, "browser_download_url")));
                            Util::ReplaceAll(asset_entry.url, "https://github.com", "");

                            auto date_time_array = Util::Split(date_time, "T");
                            auto date_array = Util::Split(date_time_array[0], "-");
                            auto time_array = Util::Split(date_time_array[1], ":");
                            asset_entry.modified.year = std::atoi(date_array[0].c_str());
                            asset_entry.modified.month = std::atoi(date_array[1].c_str());
                            asset_entry.modified.day = std::atoi(date_array[2].c_str());
                            asset_entry.modified.hours = std::atoi(time_array[0].c_str());
                            asset_entry.modified.minutes = std::atoi(time_array[1].c_str());
                            asset_entry.modified.seconds = std::atoi(time_array[2].substr(0,2).c_str());

                            assets.insert(std::make_pair(asset_entry.name, asset_entry));
                        }

                        m_assets.insert(std::make_pair(release_entry.name, assets));
                    }

                    m_releases.push_back(release_entry);
                }

                releases_parsed = true;
                return 1;
            }
        }
        return 0;
    }

    return 1;
}
