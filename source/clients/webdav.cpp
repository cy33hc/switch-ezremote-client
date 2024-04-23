#include <fstream>
#include <regex>
#include "common.h"
#include "clients/remote_client.h"
#include "clients/webdav.h"
#include "pugixml/pugiext.hpp"
#include "fs.h"
#include "lang.h"
#include "util.h"
#include "windows.h"

static const char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

std::string WebDAVClient::GetHttpUrl(std::string url)
{
    std::string http_url = std::regex_replace(url, std::regex("webdav://"), "http://");
    http_url = std::regex_replace(http_url, std::regex("webdavs://"), "https://");
    return http_url;
}

int WebDAVClient::Connect(const std::string &host, const std::string &user, const std::string &pass)
{
    std::string url = GetHttpUrl(host);
    return BaseClient::Connect(url, user, pass);
}

bool WebDAVClient::PropFind(const std::string &path, int depth, CHTTPClient::HttpResponse &res)
{
    CHTTPClient::HeadersMap headers;
    headers["Accept"] = "*/*";
    headers["Depth"] = std::to_string(depth);
    std::string encoded_path = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));

    return client->CustomRequest("PROPFIND", encoded_path, headers, res);
}

std::vector<DirEntry> WebDAVClient::ListDir(const std::string &path)
{
    CHTTPClient::HttpResponse res;
    std::vector<DirEntry> out;
    DirEntry entry;
    Util::SetupPreviousFolder(path, &entry);
    out.push_back(entry);

    if (PropFind(path, 1, res))
    {
        pugi::xml_document document;
        document.load_buffer(res.strBody.data(), res.strBody.size());
        auto multistatus = document.select_node("*[local-name()='multistatus']").node();
        auto responses = multistatus.select_nodes("*[local-name()='response']");
        for (auto response : responses)
        {
            pugi::xml_node href = response.node().select_node("*[local-name()='href']").node();
            std::string resource_path = CHTTPClient::DecodeUrl(href.first_child().value(), true);

            auto target_path_without_sep = GetFullPath(path);
            if (!target_path_without_sep.empty() && target_path_without_sep.back() == '/')
                target_path_without_sep.resize(target_path_without_sep.length() - 1);
            auto resource_path_without_sep = resource_path.erase(resource_path.find_last_not_of('/') + 1);
            size_t pos = resource_path_without_sep.find(this->host_url);
            if (pos != std::string::npos)
                resource_path_without_sep.erase(pos, this->host_url.length());

            if (resource_path_without_sep == target_path_without_sep)
                continue;

            pos = resource_path_without_sep.find_last_of('/');
            auto name = resource_path_without_sep.substr(pos + 1);
            auto propstat = response.node().select_node("*[local-name()='propstat']").node();
            auto prop = propstat.select_node("*[local-name()='prop']").node();
            std::string creation_date = prop.select_node("*[local-name()='creationdate']").node().first_child().value();
            std::string content_length = prop.select_node("*[local-name()='getcontentlength']").node().first_child().value();
            std::string m_date = prop.select_node("*[local-name()='getlastmodified']").node().first_child().value();
            std::string resource_type = prop.select_node("*[local-name()='resourcetype']").node().first_child().name();

            DirEntry entry;
            memset(&entry, 0, sizeof(entry));
            entry.selectable = true;
            sprintf(entry.directory, "%s", path.c_str());
            sprintf(entry.name, "%s", name.c_str());

            if (path.length() == 1 and path[0] == '/')
            {
                sprintf(entry.path, "%s%s", path.c_str(), name.c_str());
            }
            else
            {
                sprintf(entry.path, "%s/%s", path.c_str(), name.c_str());
            }

            entry.isDir = resource_type.find("collection") != std::string::npos;
            entry.file_size = 0;
            if (!entry.isDir)
            {
                entry.file_size = atoll(content_length.c_str());
                DirEntry::SetDisplaySize(&entry);
            }
            else
            {
                sprintf(entry.display_size, "%s", lang_strings[STR_FOLDER]);
            }

            char modified_date[32];
            char *p_char = NULL;
            sprintf(modified_date, "%s", m_date.c_str());
            p_char = strchr(modified_date, ' ');
            if (p_char)
            {
                char month[5];
                sscanf(p_char, "%d %s %d %d:%d:%d", &entry.modified.day, month, &entry.modified.year, &entry.modified.hours, &entry.modified.minutes, &entry.modified.seconds);
                for (int k = 0; k < 12; k++)
                {
                    if (strcmp(month, months[k]) == 0)
                    {
                        entry.modified.month = k + 1;
                        break;
                    }
                }
            }
            out.push_back(entry);
        }
    }
    else
    {
        sprintf(this->response, "%s", res.errMessage.c_str());
        return out;
    }

    return out;
}

int WebDAVClient::Put(const std::string &inputfile, const std::string &path, uint64_t offset)
{
    size_t bytes_remaining = FS::GetSize(inputfile);
    bytes_transfered = 0;

    client->SetProgressFnCallback(&bytes_transfered, UploadProgressCallback);
    std::string encode_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));
    long status;

    if (client->UploadFile(inputfile, encode_url, status))
    {
        if (HTTP_SUCCESS(status))
        {
            return 1;
        }
    }

    return 0;
}

int WebDAVClient::Mkdir(const std::string &path)
{
    CHTTPClient::HeadersMap headers;
    CHTTPClient::HttpResponse res;

    headers["Accept"] =  "*/*";
    headers["Connection"] = "Keep-Alive";
    std::string encode_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));

    if (client->CustomRequest("MKCOL", encode_url, headers, res))
    {
        if (HTTP_SUCCESS(res.iCode))
            return 1;
    }

    return 0;
}

int WebDAVClient::Rmdir(const std::string &path, bool recursive)
{
    return Delete(path);
}

int WebDAVClient::Rename(const std::string &src, const std::string &dst)
{
    return Move(src, dst);
}

int WebDAVClient::Delete(const std::string &path)
{
    CHTTPClient::HeadersMap headers;
    CHTTPClient::HttpResponse res;

    headers["Accept"] =  "*/*";
    headers["Connection"] = "Keep-Alive";
    std::string encode_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));

    if (client->CustomRequest("DELETE", encode_url, headers, res))
    {
        if (HTTP_SUCCESS(res.iCode))
            return 1;
    }
    
    return 0;

}

int WebDAVClient::Copy(const std::string &from, const std::string &to)
{
    CHTTPClient::HeadersMap headers;
    CHTTPClient::HttpResponse res;

    headers["Accept"] =  "*/*";
    headers["Destination"] = GetFullPath(to);
    std::string encode_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(from));

    if (client->CustomRequest("COPY", encode_url, headers, res))
    {
        if (HTTP_SUCCESS(res.iCode))
            return 1;
    }

    return 0;
}

int WebDAVClient::Move(const std::string &from, const std::string &to)
{
    CHTTPClient::HeadersMap headers;
    CHTTPClient::HttpResponse res;

    headers["Accept"] =  "*/*";
    headers["Destination"] = GetFullPath(to);
    std::string encode_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(from));

    if (client->CustomRequest("MOVE", encode_url, headers, res))
    {
        if (HTTP_SUCCESS(res.iCode))
            return 1;
    }

    return 0;
}

ClientType WebDAVClient::clientType()
{
    return CLIENT_TYPE_WEBDAV;
}

uint32_t WebDAVClient::SupportedActions()
{
    return REMOTE_ACTION_ALL;
}