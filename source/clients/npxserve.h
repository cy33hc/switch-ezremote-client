#ifndef NPXSERVE_H
#define NPXSERVE_H

#include <string>
#include <vector>
#include "clients/baseclient.h"
#include "clients/remote_client.h"
#include "common.h"

class NpxServeClient : public BaseClient
{
public:
    std::vector<DirEntry> ListDir(const std::string &path);
};

#endif