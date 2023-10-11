# Switch SMB Client

Simple SMB client for the Switch. Allows you to transfer files between the Switch and your Windows Shares, Linux SMB Shares and NAS SMB shares

![Preview](/screenshot.jpg)

## Installation
Copy the **switch-smb-client.nro** in to the folder **/switch/switch-smb-client** of the SD card. Install the forwarder NSP **switch-smb-client.nsp**.

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
The appplication support following languages

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

The following aren't standard languages supported by the Switch, therefore requires a config file update.
```
Arabic
Catalan
Croatian
Euskera
Galego
Greek
Hungarian
Indonesian
Ryukyuan
Thai
Turkish
```
User must modify the file **/switch/switch-smb-client/config.ini** located in the switch hard drive and update the **language** setting to with the **exact** values from the list above.

**HELP:** There are no language translations for the following languages, therefore not support yet. Please help expand the list by submitting translation for the following languages. If you would like to help, please download this [Template](https://github.com/cy33hc/switch-smb-client/blob/master/lang/English.ini), make your changes and submit an issue with the file attached.
```
Finnish
Swedish
Danish
Norwegian
Czech
Romanian
Vietnamese
```
or any other language that you have a traslation for.

## Building
Before build the app, you need to build the dependencies first.
Clone the following Git repos and build them in order

openssl - https://github.com/cy33hc/ps4-openssl/tree/OpenSSL_1_1_1-switch

libsmb2 - https://github.com/cy33hc/libsmb2/tree/switch
