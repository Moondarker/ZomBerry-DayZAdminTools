# ZomBerry Admin Tools
ZomBerry is still in early testing stage, please report about any issues!
Don't forget to mention game version in issue description

## Usage
### Installation:
* Make sure that you have installed [RPCFramework](https://github.com/Jacob-Mango/DayZ-RPCFramework) by Jacob_Mango, Arkensor and Kegan Hollern
* Download latest release [here](https://github.com/Moondarker/ZomBerry-DayZAdminTools/releases)
* Unzip the file into the root of your game and server directory
* Add RPCFramework and ZomBerry to your game and server launch options. For example:  
```-mod=RPCFramework;ZomBerry;```

On server:
* Copy file ```admins.cfg``` from ```ZomBerry\Config``` to server profile dir (alternatives: default dir, custom dir (passed through  
```-zbryDir=path_to_dir``` launch option), mission dir (you might need to modify your mission ```init.c``` and add line   ```g_Game.SetMissionPath(path);``` in ```CreateCustomMission``` method)
* Add your BIGUID into ```admins.cfg``` file (use new line for each admin entry)

On client:
* Copy file ```ZomBerry.cfg``` from ```ZomBerry\Config``` to ```C:\Users\[Your Username]\Documents\DayZ``` dir and change menuKey (if you wish to)


### For developers:
ZomBerry was created to be as simple and lightweight for users and developers as possible. Right now, there are two main functions available:

**AddCategory** requires two arguments: 
* string Category name
* int color code (basically, colors are usual HEX color codes but with first byte used for alpha channel. Example: 0xFF00FF00 is opaque green color)
```java
GetZomberryCmdAPI().AddCategory("MyMod", COLOR_RED);
```

**AddCommand** requires four arguments: 
* string displayName - name shown in functions list
* string functionName - name of function which will be called
* Class instance - instance of the Class which the function resides
* string categoryName - name of category for function to be placed
```java
GetZomberryCmdAPI().AddCommand("Induce sneeze", "MyModSneezeTarget", this, "MyMod");
```
Your function will be called with four arguments:
* string functionName (explains itself)
* int adminId - session id of admin who executed this function 
* int targetId - session id of targeted player
* vector cursor - position where admin's cursor intersects with ground
Note: you may get PlayerBase of player's character using ```ZBGetPlayerById(int playerId)``` function

## Planned features and updates:
* Make commands list completely server-side, so it could be supplemented directly from mission/serverside-only addons
* Major UI tweaks and code review
* New functions in default kit
