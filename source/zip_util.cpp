
#include <switch.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include "clients/remote_client.h"
#include "clients/ftpclient.h"
#include "common.h"
#include "fs.h"
#include "lang.h"
#include "windows.h"
#include "zip_util.h"
#include "util.h"

#define TRANSFER_SIZE (128 * 1024)

namespace ZipUtil
{
    int ZipAddFile(struct archive *a, const std::string &path, int filename_start)
    {
        struct archive_entry *entry;
        int res;

        // Get file stat
        struct stat file_stat;
        memset(&file_stat, 0, sizeof(file_stat));
        res = stat(path.c_str(), &file_stat);
        if (res < 0)
            return res;

        // Get file local time
        entry = archive_entry_new();

        bytes_transfered = 0;
        prev_tick = Util::GetTick();
        bytes_to_download = file_stat.st_size;

        archive_entry_set_pathname(entry, path.substr(filename_start).c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_size(entry, file_stat.st_size);
        archive_entry_set_perm(entry, 0644);
        archive_write_header(a, entry);

        // Open file to add
        FILE *fd = FS::OpenRead(path);
        if (fd == NULL)
        {
            archive_entry_free(entry);
            return 0;
        }

        // Add file to zip
        void *buf = malloc(TRANSFER_SIZE);
        uint64_t seek = 0;

        while (1)
        {
            int read = FS::Read(fd, buf, TRANSFER_SIZE);
            if (read < 0)
            {
                free(buf);
                FS::Close(fd);
                archive_entry_free(entry);
                return read;
            }

            if (read == 0)
                break;

            int written = archive_write_data(a, buf, read);
            if (written < 0)
            {
                free(buf);
                FS::Close(fd);
                archive_entry_free(entry);
                return written;
            }

            seek += written;
            bytes_transfered += read;
        }

        free(buf);
        FS::Close(fd);
        archive_entry_free(entry);

        return 1;
    }

    int ZipAddFolder(struct archive *a, const std::string &path, int filename_start)
    {
        if (filename_start > path.length())
        {
            return 1;
        }

        struct archive_entry *entry;
        int res;

        // Get file stat
        struct stat file_stat;
        memset(&file_stat, 0, sizeof(file_stat));
        res = stat(path.c_str(), &file_stat);
        if (res < 0)
            return res;

        entry = archive_entry_new();

        // Open new file in zip
        std::string folder = path.substr(filename_start);
        if (folder[folder.length() - 1] != '/')
            folder = folder + "/";

        archive_entry_set_pathname(entry, folder.c_str());
        archive_entry_set_filetype(entry, AE_IFDIR);
        archive_entry_set_perm(entry, 0755);
        archive_write_header(a, entry);

        archive_entry_free(entry);
        return 1;
    }

    int ZipAddPath(struct archive *a, const std::string &path, int filename_start)
    {
        DIR *dfd = opendir(path.c_str());
        if (dfd != NULL)
        {
            int ret = ZipAddFolder(a, path, filename_start);
            if (ret <= 0)
                return ret;

            struct dirent *dirent;
            do
            {
                dirent = readdir(dfd);
                if (stop_activity)
                    return 1;
                if (dirent != NULL && strcmp(dirent->d_name, ".") != 0 && strcmp(dirent->d_name, "..") != 0)
                {
                    int new_path_length = path.length() + strlen(dirent->d_name) + 2;
                    char *new_path = (char *)malloc(new_path_length);
                    snprintf(new_path, new_path_length, "%s%s%s", path.c_str(), FS::hasEndSlash(path.c_str()) ? "" : "/", dirent->d_name);

                    int ret = 0;

                    if (dirent->d_type & DT_DIR)
                    {
                        ret = ZipAddPath(a, new_path, filename_start);
                    }
                    else
                    {
                        sprintf(activity_message, "%s %s", lang_strings[STR_COMPRESSING], new_path);
                        ret = ZipAddFile(a, new_path, filename_start);
                    }

                    free(new_path);

                    // Some folders are protected and return 0x80010001. Bypass them
                    if (ret <= 0)
                    {
                        closedir(dfd);
                        return ret;
                    }
                }
            } while (dirent != NULL);

            closedir(dfd);
        }
        else
        {
            sprintf(activity_message, "%s %s", lang_strings[STR_COMPRESSING], path.c_str());
            return ZipAddFile(a, path, filename_start);
        }

        return 1;
    }

    /* duplicate a path name, possibly converting to lower case */
    static char *pathdup(const char *path)
    {
        char *str;
        size_t i, len;

        if (path == NULL || path[0] == '\0')
            return (NULL);

        len = strlen(path);
        while (len && path[len - 1] == '/')
            len--;
        if ((str = (char *)malloc(len + 1)) == NULL)
        {
            errno = ENOMEM;
        }
        memcpy(str, path, len);

        str[len] = '\0';

        return (str);
    }

    /* concatenate two path names */
    static char *pathcat(const char *prefix, const char *path)
    {
        char *str;
        size_t prelen, len;

        prelen = prefix ? strlen(prefix) + 1 : 0;
        len = strlen(path) + 1;
        if ((str = (char *)malloc(prelen + len)) == NULL)
        {
            errno = ENOMEM;
        }
        if (prefix)
        {
            memcpy(str, prefix, prelen); /* includes zero */
            str[prelen - 1] = '/';       /* splat zero */
        }
        memcpy(str + prelen, path, len); /* includes zero */

        return (str);
    }

    /*
     * Extract a directory.
     */
    void extract_dir(struct archive *a, struct archive_entry *e, const std::string &path)
    {
        int mode;

        if (path[0] == '\0')
            return;

        FS::MkDirs(path);
        archive_read_data_skip(a);
    }

    /*
     * Extract to a file descriptor
     */
    int extract2fd(struct archive *a, const std::string &pathname, void *fd)
    {
        uint32_t write_len;
        uint32_t current_progress = 0;
        ssize_t len = 0;
        unsigned char *buffer = (unsigned char *)malloc(ARCHIVE_TRANSFER_SIZE);

        /* loop over file contents and write to fd */
        for (int n = 0;; n++)
        {
            len = archive_read_data(a, buffer, ARCHIVE_TRANSFER_SIZE);

            if (len == 0)
            {
                free(buffer);
                return 1;
            }

            if (len < 0)
            {
                sprintf(status_message, "error archive_read_data('%s')", pathname.c_str());
                free(buffer);
                return 0;
            }
            current_progress += len;
            bytes_transfered = current_progress;

            write_len = FS::Write(fd, buffer, len);
            if (write_len != len)
            {
                sprintf(status_message, "error write('%s')", pathname.c_str());
                free(buffer);
                return 0;
            }
        }

        free(buffer);
        return 1;
    }

    /*
     * Extract a regular file.
     */
    void extract_file(struct archive *a, struct archive_entry *e, const std::string &path)
    {
        struct stat sb;
        void *fd;
        const char *linkname;

        /* look for existing file of same name */
    recheck:
        if (lstat(path.c_str(), &sb) == 0)
        {
            (void)unlink(path.c_str());
        }

        /* process symlinks */
        linkname = archive_entry_symlink(e);
        if (linkname != NULL)
        {
            /* set access and modification time */
            return;
        }

        bytes_to_download = archive_entry_size(e);
        if ((fd = FS::Create(path.c_str())) == nullptr)
        {
            sprintf(status_message, "error open('%s')", path.c_str());
            return;
        }

        extract2fd(a, path, fd);

        /* set access and modification time */
        FS::Close(fd);
    }

    void extract(struct archive *a, struct archive_entry *e, const std::string &base_dir)
    {
        char *pathname, *realpathname;
        mode_t filetype;

        if ((pathname = pathdup(archive_entry_pathname(e))) == NULL)
        {
            archive_read_data_skip(a);
            return;
        }
        filetype = archive_entry_filetype(e);

        /* sanity checks */
        if (pathname[0] == '/' ||
            strncmp(pathname, "../", 3) == 0 ||
            strstr(pathname, "/../") != NULL)
        {
            archive_read_data_skip(a);
            free(pathname);
            return;
        }

        /* I don't think this can happen in a zipfile.. */
        if (!S_ISDIR(filetype) && !S_ISREG(filetype) && !S_ISLNK(filetype))
        {
            archive_read_data_skip(a);
            free(pathname);
            return;
        }

        realpathname = pathcat(base_dir.c_str(), pathname);

        /* ensure that parent directory exists */
        FS::MkDirs(realpathname, true);

        if (S_ISDIR(filetype))
            extract_dir(a, e, realpathname);
        else
        {
            snprintf(activity_message, 255, "%s: %s", lang_strings[STR_EXTRACTING], pathname);
            bytes_transfered = 0;
            prev_tick = Util::GetTick();

            extract_file(a, e, realpathname);
        }

        free(realpathname);
        free(pathname);
    }

    static RemoteArchiveData *OpenRemoteArchive(const std::string &file, RemoteClient *client)
    {
        RemoteArchiveData *data;

        data = (RemoteArchiveData *)malloc(sizeof(RemoteArchiveData));
        memset(data, 0, sizeof(RemoteArchiveData));

        data->offset = 0;
        client->Size(file, &data->size);
        data->client = client;
        data->path = file;
        if (client->SupportedActions() & REMOTE_ACTION_RAW_READ)
        {
            data->fp = client->Open(file, O_RDONLY);
        }

        if (client->clientType() == CLIENT_TYPE_FTP)
        {
            FtpClient *_client = (FtpClient *)client;
            data->ftp_xfer_callbak = _client->GetCallbackXferFunction();
            _client->SetCallbackXferFunction(nullptr);
        }
        return data;
    }

    static ssize_t ReadRemoteArchive(struct archive *a, void *client_data, const void **buff)
    {
        ssize_t to_read;
        int ret;
        RemoteArchiveData *data;

        data = (RemoteArchiveData *)client_data;
        *buff = data->buf;

        to_read = data->size - data->offset;
        if (to_read == 0)
            return 0;

        to_read = MIN(to_read, ARCHIVE_TRANSFER_SIZE);
        if (data->client->SupportedActions() & REMOTE_ACTION_RAW_READ)
            ret = data->client->GetRange(data->fp, data->buf, to_read, data->offset);
        else
            ret = data->client->GetRange(data->path, data->buf, to_read, data->offset);

        if (ret == 0)
            return -1;
        data->offset = data->offset + to_read;

        return to_read;
    }

    static int CloseRemoteArchive(struct archive *a, void *client_data)
    {
        if (client_data != nullptr)
        {
            RemoteArchiveData *data;
            data = (RemoteArchiveData *)client_data;
            if (data->client->clientType() == CLIENT_TYPE_FTP)
            {
                FtpClient *_client = (FtpClient *)data->client;
                _client->SetCallbackXferFunction(data->ftp_xfer_callbak);
            }
            if (data->client->SupportedActions() & REMOTE_ACTION_RAW_READ)
                data->client->Close(data->fp);

            free(client_data);
        }
        return 0;
    }

    int64_t SeekRemoteArchive(struct archive *, void *client_data, int64_t offset, int whence)
    {
        RemoteArchiveData *data = (RemoteArchiveData *)client_data;

        if (whence == SEEK_SET)
        {
            data->offset = offset;
        }
        else if (whence == SEEK_CUR)
        {
            data->offset = data->offset + offset - 1;
        }
        else if (whence == SEEK_END)
        {
            data->offset = data->size - 1;
        }
        else
            return ARCHIVE_FATAL;

        return data->offset;
    }

    int64_t SkipRemoteArchive(struct archive *, void *client_data, int64_t request)
    {
        RemoteArchiveData *data = (RemoteArchiveData *)client_data;

        data->offset = data->offset + request - 1;

        return request;
    }

    /*
     * Main loop: open the zipfile, iterate over its contents and decide what
     * to do with each entry.
     */
    int Extract(const DirEntry &file, const std::string &basepath, RemoteClient *client)
    {
        struct archive *a;
        struct archive_entry *e;
        RemoteArchiveData *client_data = nullptr;
        int ret;

        if ((a = archive_read_new()) == NULL)
        {
            sprintf(status_message, "%s", "archive_read_new failed");
            return 0;
        }

        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);

        if (client == nullptr)
        {
            ret = archive_read_open_filename(a, file.path, ARCHIVE_TRANSFER_SIZE);
            if (ret < ARCHIVE_OK)
            {
                sprintf(status_message, "%s", "archive_read_open_filename failed");
                return 0;
            }
        }
        else
        {
            client_data = OpenRemoteArchive(file.path, client);
            if (client_data == nullptr)
            {
                sprintf(status_message, "%s", "archive_read_open_filename failed");
                return 0;
            }

            ret = archive_read_set_seek_callback(a, SeekRemoteArchive);
            if (ret < ARCHIVE_OK)
            {
                sprintf(status_message, "archive_read_set_seek_callback failed - %s", archive_error_string(a));
                return 0;
            }

            ret = archive_read_open2(a, client_data, NULL, ReadRemoteArchive, SkipRemoteArchive, CloseRemoteArchive);
            if (ret < ARCHIVE_OK)
            {
                sprintf(status_message, "archive_read_open failed - %s", archive_error_string(a));
                return 0;
            }
        }

        for (;;)
        {
            if (stop_activity)
                break;

            ret = archive_read_next_header(a, &e);
            if (ret < ARCHIVE_OK)
            {
                sprintf(status_message, "%s", "archive_read_next_header failed");
                archive_read_free(a);
                return 0;
            }

            if (ret == ARCHIVE_EOF)
                break;

            extract(a, e, basepath);
        }

        archive_read_free(a);

        return 1;
    }

}