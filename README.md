# ezRemote Client

ezRemote Client is a File Manager application that allows you to connect the Switch to remote FTP, SMB, WebDAV servers to transfer  and manage files. The interface is inspired by Filezilla client which provides a commander like GUI.

![Preview](/screenshot.jpg)

## Usage
To distinguish between FTP, SMB, WebDAV, the URL must be prefix with **ftp://**, **smb://**, **webdav://**, **webdavs://**

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
  
Tested with following WebDAV server:
 - **(Recommeded)** [RClone](https://rclone.org/) - For hosting your own WebDAV server. You can use RClone WebDAV server as proxy to 70+ public file hosting services (Eg. Google Drive, OneDrive, Mega, dropbox, NextCloud etc..)
 - [Dufs](https://github.com/sigoden/dufs) - For hosting your own WebDAV server.
 - [SFTPgo](https://github.com/drakkan/sftpgo) - For local hosted WebDAV server. Can also be used as a webdav frontend for Cloud Storage like AWS S3, Azure Blob or Google Storage.

## Features Native Application##
 - Transfer files back and forth between Switch and FTP/SMB/WebDAV server
 - File management function include cut/copy/paste/rename/delete/new folder/file for files on Switch SD Card

## Controls
```
X - Menu (after a file is selected)
A - Select Button/TextBox
B - Un-Select the file list to navigate to other widgets
Y - Mark file(s)/folder(s) for Delete/Rename/Upload/Download
R1 - Navigate to the Remote list of files
L1 - Navigate to the Local list of files
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
User must modify the file **ux0:/data/RMTCLI001/config.ini** and update the **language** setting with the **exact** values from the list above.

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
