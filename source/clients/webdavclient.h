#ifndef WEBDAVCLIENT_H
#define WEBDAVCLIENT_H

#include <time.h>
#include <string>
#include <vector>
#include <regex>
#include "webdav/client.hpp"
#include "common.h"
#include "remote_client.h"

#define SMB_CLIENT_MAX_FILENAME_LEN 256

namespace WebDAV
{
	inline std::string GetHttpUrl(std::string url)
	{
		std::string http_url = std::regex_replace(url, std::regex("webdav://"), "http://");
		http_url = std::regex_replace(http_url, std::regex("webdavs://"), "https://");
		return http_url;
	}

	class WebDavClient : public RemoteClient
	{
	public:
		int Connect(const std::string &host, const std::string &user, const std::string &pass);
		int Mkdir(const std::string &path);
		int Rmdir(const std::string &path, bool recursive);
		int Size(const std::string &path, int64_t *size);
		int Get(const std::string &outputfile, const std::string &path, uint64_t offset=0);
		int Put(const std::string &inputfile, const std::string &path, uint64_t offset=0);
		int Rename(const std::string &src, const std::string &dst);
		int Delete(const std::string &path);
		int Copy(const std::string &from, const std::string &to);
		int Move(const std::string &from, const std::string &to);
		bool FileExists(const std::string &path);
		std::vector<DirEntry> ListDir(const std::string &path);
		bool IsConnected();
		bool Ping();
		const char *LastResponse();
		int Quit();
		std::string GetPath(std::string ppath1, std::string ppath2);
		ClientType clientType();
		uint32_t SupportedActions();

	private:
		int _Rmdir(const std::string &path);
		WebDAV::Client *client;
		char response[1024];
		bool connected = false;
	};

}
#endif
