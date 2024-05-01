#ifndef ZIP_UTIL_H
#define ZIP_UTIL_H

#include <string.h>
#include <stdlib.h>
#include <archive.h>
#include "clients/remote_client.h"
#include "common.h"
#include "fs.h"

#define ARCHIVE_TRANSFER_SIZE 1048576

static uint8_t MAGIC_ZIP_1[4] = {0x50, 0x4B, 0x03, 0x04};
static uint8_t MAGIC_ZIP_2[4] = {0x50, 0x4B, 0x05, 0x06};
static uint8_t MAGIC_ZIP_3[4] = {0x50, 0x4B, 0x07, 0x08};

static uint8_t MAGIC_7Z_1[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};

static uint8_t MAGIC_RAR_1[7] = {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00};
static uint8_t MAGIC_RAR_2[8] = {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00};

enum CompressFileType {
    COMPRESS_FILE_TYPE_7Z,
    COMPRESS_FILE_TYPE_ZIP,
    COMPRESS_FILE_TYPE_RAR,
    COMPRESS_FILE_TYPE_UNKNOWN
};

struct RemoteArchiveData
{
    void *fp;
    std::string path;
    uint64_t size;
    uint64_t offset;
    uint8_t buf[ARCHIVE_TRANSFER_SIZE];
    int buf_ref;
    FtpCallbackXfer ftp_xfer_callbak;
    RemoteClient *client;
};

namespace ZipUtil
{
    int ZipAddPath(struct archive *a, const std::string &path, int filename_start);
    int Extract(const DirEntry &file, const std::string &dir, RemoteClient *client = nullptr);
}
#endif