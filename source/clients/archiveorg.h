#ifndef EZ_ARCHIVEORG_H
#define EZ_ARCHIVEORG_H

#include <string>
#include <vector>
#include "clients/remote_client.h"
#include "clients/baseclient.h"
#include "common.h"

class ArchiveOrgClient : public BaseClient
{
public:
    std::vector<DirEntry> ListDir(const std::string &path);
};

#endif