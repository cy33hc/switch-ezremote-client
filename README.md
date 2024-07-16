# ezRemote Client

ezRemote Client is a File Manager application that allows you to connect the Switch to remote FTP, SMB, WebDAV servers to transfer  and manage files. The interface is inspired by Filezilla client which provides a commander like GUI.

![Preview](/screenshot.jpg)

## Features
 - Transfer files back and forth between Switch and FTP/SMB/WebDAV/HTTP(Apache,Ngnix,IIS,RClone,NPXServe) server
 - File management function include cut/copy/paste/edit/rename/delete/new folder/file for files on Switch SD Card
 - Extract zip, rar, 7zip, tar, tar.gz and tar.bz2 from Local and Remote Servers
 - Create Zip file in local
 - Simple Text Editor for editing text files
 - Image viewer for jpg, png, bmp

## Installation
 - Copy ezremote-client.nro to the "/switch" folder on sdcard
 - Install [ezremote-client.nsp](https://github.com/cy33hc/switch-ezremote-client/releases/download/1.00/ezremote-client.nsp)

## Known Issues
 - Occasional crash in applet mode. Avoid running in applet mode and install the NSP forwarder.

## Usage
To distinguish between FTP, SMB, WebDAV, the URL must be prefix with **ftp://**, **smb://**, **webdav://**, **webdavs://**, **http://**, **https://**

 - The url format for FTP is
   ```
   ftp://hostname[:port]

     - hostname can be the textual hostname or an IP address. hostname is required
     - port is optional and defaults to 21(ftp) if not provided
   ```

 - The url format for SMB is
   ```
   smb://hostname[:port]/sharename

     - hostname can be the textual hostname or an IP address. hostname is required
     - port is optional and defaults to 445 if not provided
     - sharename is required
   ```

 - The url format for WebDAV is
   ```
   webdav://hostname[:port]/[url_path]
   webdavs://hostname[:port]/[url_path]

     - hostname can be the textual hostname or an IP address. hostname is required
     - port is optional and defaults to 80(webdav) and 443(webdavs) if not provided
     - url_path is optional based on your WebDAV hosting requiremets
   ```
  
 - The url format for HTTP is
   ```
   http://hostname[:port]/[url_path]
   https://hostname[:port]/[url_path]

     - hostname can be the textual hostname or an IP address. hostname is required
     - port is optional and defaults to 80(http) and 443(https) if not provided
     - url_path is optional based on your WebDAV hosting requiremets
   ```

- For Internet Archive repos download URLs
  - Only supports parsing of the download URL (ie the URL where you see a list of files). Example
    |      |           |  |
    |----------|-----------|---|
    | ![archive_org_screen1](https://github.com/user-attachments/assets/b129b6cf-b938-4d7c-a3fa-61e1c633c1f6) | ![archive_org_screen2](https://github.com/user-attachments/assets/646106d1-e60b-4b35-b153-3475182df968)| ![image](https://github.com/user-attachments/assets/cad94de8-a694-4ef5-92a8-b87468e30adb) |
    
Tested with following WebDAV server:
 - **(Recommeded)** [RClone](https://rclone.org/) - For hosting your own WebDAV server. You can use RClone WebDAV server as proxy to 70+ public file hosting services (Eg. Google Drive, OneDrive, Mega, dropbox, NextCloud etc..)
 - [Dufs](https://github.com/sigoden/dufs) - For hosting your own WebDAV server.
 - [SFTPgo](https://github.com/drakkan/sftpgo) - For local hosted WebDAV server. Can also be used as a webdav frontend for Cloud Storage like AWS S3, Azure Blob or Google Storage.

## Controls
```
X - Menu (after a file is selected)
A - Select Button/TextBox
B - Un-Select the file list to navigate to other widgets
Y - Mark file(s)/folder(s) for Delete/Rename/Upload/Download
R1 - Navigate to the Remote list of files
L1 - Navigate to the Local list of files
ZL - Go up a directory from current directory
+ - Exit Application
```

## Multi Language Support
The appplication support following languages.

The following languages are auto detected.
```
Dutch
English
French
German
Italiano
Japanese
Korean
Polish
Portuguese_BR
Russian
Spanish
Simplified Chinese
Traditional Chinese
```

The following aren't standard languages supported by the PS4, therefore requires a config file update.
```
Arabic
Catalan
Croatian
Euskera
Galego
Greek
Hungarian
Indonesian
Romanian
Ryukyuan
Thai
Turkish
```
User must modify the file **/switch/ezremote-client/config.ini** and update the **language** setting with the **exact** values from the list above.

**HELP:** There are no language translations for the following languages, therefore not support yet. Please help expand the list by submitting translation for the following languages. If you would like to help, please download this [Template](https://github.com/cy33hc/switch-ezremote-client/blob/master/lang/English.ini), make your changes and submit an issue with the file attached.
```
Finnish
Swedish
Danish
Norwegian
Czech
Vietnamese
```
or any other language that you have a traslation for.
