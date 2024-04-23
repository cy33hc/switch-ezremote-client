#include <fstream>
#include <curl/curl.h>
#include "common.h"
#include "clients/remote_client.h"
#include "clients/baseclient.h"
#include "config.h"
#include "lang.h"
#include "util.h"
#include "windows.h"

BaseClient::BaseClient(){};

BaseClient::~BaseClient()
{
    if (client != nullptr)
        delete client;
};

int BaseClient::DownloadProgressCallback(void* ptr, double dTotalToDownload, double dNowDownloaded, double dTotalToUpload, double dNowUploaded)
{
    CHTTPClient::ProgressFnStruct *progress_data = (CHTTPClient::ProgressFnStruct*) ptr;
    uint64_t *bytes_transfered = (uint64_t *) progress_data->pOwner;
	*bytes_transfered = dNowDownloaded;
    return 0;
}

int BaseClient::UploadProgressCallback(void* ptr, double dTotalToDownload, double dNowDownloaded, double dTotalToUpload, double dNowUploaded)
{
    CHTTPClient::ProgressFnStruct *progress_data = (CHTTPClient::ProgressFnStruct*) ptr;
    uint64_t *bytes_transfered = (uint64_t *) progress_data->pOwner;
    *bytes_transfered = dNowUploaded;
    return 0;
}

int BaseClient::Connect(const std::string &url, const std::string &username, const std::string &password)
{
    this->host_url = url;
    size_t scheme_pos = url.find("://");
    size_t root_pos = url.find("/", scheme_pos + 3);
    if (root_pos != std::string::npos)
    {
        this->host_url = url.substr(0, root_pos);
        this->base_path = url.substr(root_pos);
    }
    client = new CHTTPClient([](const std::string& log){});
    client->SetBasicAuth(username, password);
    client->InitSession(true, CHTTPClient::SettingsFlag::NO_FLAGS);
    client->SetCertificateFile(CACERT_FILE);

    if (Ping())
        this->connected = true;
    return 1;
}

int BaseClient::Mkdir(const std::string &path)
{
    sprintf(this->response, "%s", lang_strings[STR_UNSUPPORTED_OPERATION_MSG]);
    return 0;
}

int BaseClient::Rmdir(const std::string &path, bool recursive)
{
    sprintf(this->response, "%s", lang_strings[STR_UNSUPPORTED_OPERATION_MSG]);
    return 0;
}

int BaseClient::Size(const std::string &path, int64_t *size)
{
    CHTTPClient::HeadersMap headers;
    CHTTPClient::HttpResponse res;

    std::string encoded_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));
    if (client->Head(encoded_url, headers, res))
    {
        if (HTTP_SUCCESS(res.iCode))
        {
            std::string content_length = res.mapHeadersLowercase["content-length"];
            if (content_length.length() > 0)
                *size = atoll(content_length.c_str());
            return 1;
        }
    }
    else
    {
        sprintf(this->response, "%s", res.errMessage.c_str());
    }
    return 0;
}

int BaseClient::Get(const std::string &outputfile, const std::string &path, uint64_t offset)
{
    long status;
    bytes_transfered = 0;
    if (!Size(path, &bytes_to_download))
    {
        sprintf(this->response, "%s", lang_strings[STR_FAIL_DOWNLOAD_MSG]);
        return 0;
    }

    client->SetProgressFnCallback(&bytes_transfered, DownloadProgressCallback);
    std::string encoded_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));
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

int BaseClient::GetRange(const std::string &path, void *buffer, uint64_t size, uint64_t offset)
{
    CHTTPClient::HttpResponse res;
    CHTTPClient::HeadersMap headers;

    char range_header[128];
    sprintf(range_header, "bytes=%llu-%llu", offset, offset + size - 1);
    headers["Range"] = range_header;

    std::string encoded_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));
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

int BaseClient::Put(const std::string &inputfile, const std::string &path, uint64_t offset)
{
    sprintf(this->response, "%s", lang_strings[STR_UNSUPPORTED_OPERATION_MSG]);
    return 0;
}

int BaseClient::Rename(const std::string &src, const std::string &dst)
{
    sprintf(this->response, "%s", lang_strings[STR_UNSUPPORTED_OPERATION_MSG]);
    return 0;
}

int BaseClient::Delete(const std::string &path)
{
    sprintf(this->response, "%s", lang_strings[STR_UNSUPPORTED_OPERATION_MSG]);
    return 0;
}

int BaseClient::Copy(const std::string &from, const std::string &to)
{
    sprintf(this->response, "%s", lang_strings[STR_UNSUPPORTED_OPERATION_MSG]);
    return 0;
}

int BaseClient::Move(const std::string &from, const std::string &to)
{
    sprintf(this->response, "%s", lang_strings[STR_UNSUPPORTED_OPERATION_MSG]);
    return 0;
}

int BaseClient::Head(const std::string &path, void *buffer, uint64_t size)
{
    CHTTPClient::HttpResponse res;
    CHTTPClient::HeadersMap headers;

    char range_header[128];
    sprintf(range_header, "bytes=%lu-%lu", 0L, size - 1);
    headers["Range"] = range_header;

    std::string encoded_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path));
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

bool BaseClient::FileExists(const std::string &path)
{
    int64_t file_size;
    return Size(path, &file_size);
}

std::vector<DirEntry> BaseClient::ListDir(const std::string &path)
{
    std::vector<DirEntry> out;
    DirEntry entry;
    Util::SetupPreviousFolder(path, &entry);
    out.push_back(entry);

    return out;
}

std::string BaseClient::GetPath(std::string ppath1, std::string ppath2)
{
    std::string path1 = ppath1;
    std::string path2 = ppath2;
    path1 = Util::Trim(Util::Trim(path1, " "), "/");
    path2 = Util::Trim(Util::Trim(path2, " "), "/");
    path1 = this->base_path + ((this->base_path.length() > 0) ? "/" : "") + path1 + "/" + path2;
    if (path1[0] != '/')
        path1 = "/" + path1;
    return path1;
}

std::string BaseClient::GetFullPath(std::string ppath1)
{
    std::string path1 = ppath1;
    path1 = Util::Trim(Util::Trim(path1, " "), "/");
    path1 = this->base_path + "/" + path1;
    Util::ReplaceAll(path1, "//", "/");
    return path1;
}

bool BaseClient::IsConnected()
{
    return this->connected;
}

bool BaseClient::Ping()
{
    CHTTPClient::HttpResponse res;
    CHTTPClient::HeadersMap headers;

    std::string encoded_url = this->host_url + CHTTPClient::EncodeUrl(GetFullPath("/"));
    if (client->Head(encoded_url, headers, res))
    {
        return true;
    }
    else
    {
        sprintf(this->response, "%s", res.errMessage.c_str());
    }
    return false;
}

const char *BaseClient::LastResponse()
{
    return this->response;
}

int BaseClient::Quit()
{
    if (client != nullptr)
    {
        client->CleanupSession();
        delete client;
        client = nullptr;
    }
    return 1;
}

ClientType BaseClient::clientType()
{
    return CLIENT_TYPE_HTTP_SERVER;
}

uint32_t BaseClient::SupportedActions()
{
    return REMOTE_ACTION_DOWNLOAD | REMOTE_ACTION_INSTALL | REMOTE_ACTION_EXTRACT;
}

std::string BaseClient::Escape(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        char *output = curl_easy_escape(curl, url.c_str(), url.length());
        if (output)
        {
            std::string encoded_url = std::string(output);
            curl_free(output);
            return encoded_url;
        }
        curl_easy_cleanup(curl);
    }
    return "";
}

std::string BaseClient::UnEscape(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        int decode_len;
        char *output = curl_easy_unescape(curl, url.c_str(), url.length(), &decode_len);
        if (output)
        {
            std::string decoded_url = std::string(output, decode_len);
            curl_free(output);
            return decoded_url;
        }
        curl_easy_cleanup(curl);
    }
    return "";
}
