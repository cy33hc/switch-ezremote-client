
#include <switch.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <un7zip.h>
#include <zlib.h>
#include <archive.h>
#include <archive_entry.h>
#include "clients/remote_client.h"
#include "clients/ftpclient.h"
#include "common.h"
#include "fs.h"
#include "lang.h"
#include "windows.h"
#include "zip_util.h"

#define TRANSFER_SIZE (128 * 1024)

namespace ZipUtil
{
    static char filename_extracted[256];

    void callback_7zip(const char *fileName, unsigned long fileSize, unsigned fileNum, unsigned numFiles)
    {
        sprintf(activity_message, "%s %s: %s", lang_strings[STR_EXTRACTING], filename_extracted, fileName);
    }

    void convertToZipTime(time_t time, tm_zip *tmzip)
    {
        struct tm tm = *localtime(&time);
        tmzip->tm_sec = tm.tm_sec;
        tmzip->tm_min = tm.tm_min;
        tmzip->tm_hour = tm.tm_hour;
        tmzip->tm_mday = tm.tm_mday;
        tmzip->tm_mon = tm.tm_mon + 1;
        tmzip->tm_year = tm.tm_year + 1900;
    }

    int ZipAddFile(zipFile zf, const std::string &path, int filename_start, int level)
    {
        int res;
        // Get file stat
        struct stat file_stat;
        memset(&file_stat, 0, sizeof(file_stat));
        res = stat(path.c_str(), &file_stat);
        if (res < 0)
            return res;

        // Get file local time
        zip_fileinfo zi;
        memset(&zi, 0, sizeof(zip_fileinfo));
        convertToZipTime(file_stat.st_mtim.tv_sec, &zi.tmz_date);

        bytes_transfered = 0;
        bytes_to_download = file_stat.st_size;

        // Large file?
        int use_zip64 = (file_stat.st_size >= 0xFFFFFFFF);

        // Open new file in zip
        res = zipOpenNewFileInZip3_64(zf, path.substr(filename_start).c_str(), &zi,
                                      NULL, 0, NULL, 0, NULL,
                                      (level != 0) ? Z_DEFLATED : 0,
                                      level, 0,
                                      -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                      NULL, 0, use_zip64);
        if (res < 0)
            return res;

        // Open file to add
        FILE *fd = FS::OpenRead(path);
        if (fd == NULL)
        {
            zipCloseFileInZip(zf);
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
                zipCloseFileInZip(zf);
                return read;
            }

            if (read == 0)
                break;

            int written = zipWriteInFileInZip(zf, buf, read);
            if (written < 0)
            {
                free(buf);
                FS::Close(fd);
                zipCloseFileInZip(zf);
                return written;
            }

            seek += written;
            bytes_transfered += read;
        }

        free(buf);
        FS::Close(fd);
        zipCloseFileInZip(zf);

        return 1;
    }

    int ZipAddFolder(zipFile zf, const std::string &path, int filename_start, int level)
    {
        int res;

        // Get file stat
        struct stat file_stat;
        memset(&file_stat, 0, sizeof(file_stat));
        res = stat(path.c_str(), &file_stat);
        if (res < 0)
            return res;

        // Get file local time
        zip_fileinfo zi;
        memset(&zi, 0, sizeof(zip_fileinfo));
        convertToZipTime(file_stat.st_mtim.tv_sec, &zi.tmz_date);

        // Open new file in zip
        std::string folder = path.substr(filename_start);
        if (folder[folder.length() - 1] != '/')
            folder = folder + "/";

        res = zipOpenNewFileInZip3_64(zf, folder.c_str(), &zi,
                                      NULL, 0, NULL, 0, NULL,
                                      (level != 0) ? Z_DEFLATED : 0,
                                      level, 0,
                                      -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                      NULL, 0, 0);

        if (res < 0)
            return res;

        zipCloseFileInZip(zf);
        return 1;
    }

    int ZipAddPath(zipFile zf, const std::string &path, int filename_start, int level)
    {
        DIR *dfd = opendir(path.c_str());
        if (dfd != NULL)
        {
            int ret = ZipAddFolder(zf, path, filename_start, level);
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
                        ret = ZipAddPath(zf, new_path, filename_start, level);
                    }
                    else
                    {
                        sprintf(activity_message, "%s %s", lang_strings[STR_COMPRESSING], new_path);
                        ret = ZipAddFile(zf, new_path, filename_start, level);
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
            return ZipAddFile(zf, path, filename_start, level);
        }

        return 1;
    }

    CompressFileType getCompressFileType(const std::string &file)
    {
        char buf[8];

        memset(buf, 0, 8);
        int ret = FS::Head(file, buf, 8);
        if (ret == 0)
            return COMPRESS_FILE_TYPE_UNKNOWN;

        if (strncmp(buf, (const char *)MAGIC_7Z_1, 6) == 0)
            return COMPRESS_FILE_TYPE_7Z;
        else if (strncmp(buf, (const char *)MAGIC_ZIP_1, 4) == 0 || strncmp(buf, (const char *)MAGIC_ZIP_2, 4) == 0 || strncmp(buf, (const char *)MAGIC_ZIP_3, 4) == 0)
            return COMPRESS_FILE_TYPE_ZIP;

        return COMPRESS_FILE_TYPE_UNKNOWN;
    }

    int ExtractZip(const DirEntry &file, const std::string &dir)
    {
        file_transfering = true;
        unz_global_info global_info;
        unz_file_info file_info;
        unzFile zipfile = unzOpen(file.path);
        std::string dest_dir = std::string(dir);
        if (dest_dir[dest_dir.length() - 1] != '/')
        {
            dest_dir = dest_dir + "/";
        }
        if (zipfile == NULL)
        {
            return 0;
        }
        unzGetGlobalInfo(zipfile, &global_info);
        unzGoToFirstFile(zipfile);
        uint64_t curr_extracted_bytes = 0;
        uint64_t curr_file_bytes = 0;
        int num_files = global_info.number_entry;
        char fname[512];
        char ext_fname[512];
        char read_buffer[TRANSFER_SIZE];

        for (int zip_idx = 0; zip_idx < num_files; ++zip_idx)
        {
            if (stop_activity)
                break;
            unzGetCurrentFileInfo(zipfile, &file_info, fname, 512, NULL, 0, NULL, 0);
            sprintf(ext_fname, "%s%s", dest_dir.c_str(), fname);
            const size_t filename_length = strlen(ext_fname);
            bytes_transfered = 0;
            bytes_to_download = file_info.uncompressed_size;
            if (ext_fname[filename_length - 1] != '/')
            {
                snprintf(activity_message, 255, "%s %s: %s", lang_strings[STR_EXTRACTING], file.name, fname);
                curr_file_bytes = 0;
                unzOpenCurrentFile(zipfile);
                FS::MkDirs(ext_fname, true);
                FILE *f = fopen(ext_fname, "wb");
                while (curr_file_bytes < file_info.uncompressed_size)
                {
                    int rbytes = unzReadCurrentFile(zipfile, read_buffer, TRANSFER_SIZE);
                    if (rbytes > 0)
                    {
                        fwrite(read_buffer, 1, rbytes, f);
                        curr_extracted_bytes += rbytes;
                        curr_file_bytes += rbytes;
                        bytes_transfered = curr_file_bytes;
                    }
                }
                fclose(f);
                unzCloseCurrentFile(zipfile);
            }
            else
            {
                FS::MkDirs(ext_fname, true);
            }
            if ((zip_idx + 1) < num_files)
            {
                unzGoToNextFile(zipfile);
            }
        }
        unzClose(zipfile);
        return 1;
    }

    int Extract7Zip(const DirEntry &file, const std::string &dir)
    {
        file_transfering = false;
        FS::MkDirs(dir, true);
        sprintf(filename_extracted, "%s", file.name);
        int res = Extract7zFileEx(file.path, dir.c_str(), callback_7zip, DEFAULT_IN_BUF_SIZE);
        return res == 0;
    }

/*     int Extract(const DirEntry &file, const std::string &dir)
    {
        CompressFileType fileType = getCompressFileType(file.path);

        if (fileType == COMPRESS_FILE_TYPE_ZIP)
            return ExtractZip(file, dir);
        else if (fileType == COMPRESS_FILE_TYPE_7Z)
            return Extract7Zip(file, dir);
        else
        {
            sprintf(status_message, "%s - %s", file.name, lang_strings[STR_UNSUPPORTED_FILE_FORMAT]);
            return -1;
        }
        return 1;
    }

 */    /* duplicate a path name, possibly converting to lower case */
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

    int64_t	SkipRemoteArchive(struct archive *, void *client_data, int64_t request)
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