//The one way ZomBerry logs (through ZomberryBase.Log): each line goes to ZomBerry's log file (or the script log), and on
//the server, lines of the modules below also go to the in-game admin log (ZBerryActivityLog), which keeps them searchable
class ZomberryLogger {
	static const string MODULE_ADMIN = "ZomBerryAdmin"; //Admin actions
	static const string MODULE_BANS = "ZomBerryBans"; //Kicks, bans, refused joins
	static const string MODULE_AUTH = "ZomBerryAuth"; //Setup token, versions
	static const string MODULE_KILLS = "ZomBerryKills"; //Player deaths

	private string file_path;
	private bool defaultIO = true;

	void Log(string module, string txt) {
		Write(module, txt);
		if (GetGame() && GetGame().IsServer()) GetZBerryActivityLog().Record(module, txt);
	}

	//The file, or the script log when there's none (or it can't be written)
	private void Write(string module, string txt) {
		if (defaultIO) {
			Print(GetDate() + " [" + module + "]: " + txt);
			return;
		}

		FileHandle logFile = OpenFile(file_path, FileMode.APPEND);
		if (logFile != 0) {
			FPrintln(logFile, GetDate() + " [" + module + "]: " + txt);
			CloseFile(logFile);
		} else {
			defaultIO = true;
			Write("ZomBerryLogger", "WARN: !!! Falling back to scripts.txt, can't write to " + file_path);
			Write(module, txt);
		}
	}

	//True while Log() ends up in the script log anyway
	bool IsUsingScriptLog() {
		return defaultIO;
	}

	void DebugLog(int lvl, string module, string txt) {
		if (ZomberryBase.GetConfig().GetDebugLvl() >= lvl) {
			Log(module, txt);
		}
	}

	void SwitchToCustomIO() {
		string tempPath = "";
		string realProfiles = ZomberryBase.GetConfig().GetProfilesPath();
		if (!defaultIO) return;

		if (defaultIO) {
			tempPath = "$profile:ZomBerry\\";
			if (realProfiles != "") Log("ZomBerryLogger", "INFO: Will try to create ZomBerry log file in profile directory: " + realProfiles + "\\ZomBerry\\");
				else Log("ZomBerryLogger", "INFO: Will try to create ZomBerry log file in profile directory: " + tempPath);
			SetDir(tempPath);
		}
		if (defaultIO) {
			tempPath = "$profile:";
			if (realProfiles != "") Log("ZomBerryLogger", "INFO: Will try to create ZomBerry log file in profile root: " + realProfiles);
				else Log("ZomBerryLogger", "INFO: Will try to create ZomBerry log file in profile root: " + tempPath);
			SetDir(tempPath);
		}
		if (defaultIO) Log("ZomBerryLogger", "WARN: All attempts to use custom log file failed, using script.log");
	}

	private void SetDir(string fPath) {
		string tPath = fPath + "ZomBerry_" + GetDate(true) + ".log";
		if (!FileExist(fPath)) MakeDirectory(fPath);

		FileHandle logFile = OpenFile(tPath, FileMode.APPEND);
		if (logFile != 0) {
			Log("ZomBerryLogger", "INFO: Switching to logfile: " + tPath);
			defaultIO = false;
			file_path = tPath;
			FPrintln(logFile, "---------------------------------------------");
			FPrintln(logFile, "ZomBerry " + ZBerryVersion.GetFullVersion() + " log started at " + GetDate());
			FPrintln(logFile, "");
			CloseFile(logFile);
		} else {
			if (FileExist(tPath)) {
				Log("ZomBerryLogger", "INFO: Can't write to file " + tPath);
			} else {
				Log("ZomBerryLogger", "INFO: Can't create file " + tPath + " (files per directory limit exceeded?)");
			}
		}
	}

	static private string GetDate(bool fileFriendly = false) {
		int year, month, day, hour, minute, second;

		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		string date = day.ToStringLen(2) + "." + month.ToStringLen(2) + "." + year.ToStringLen(4) + " " + hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2);
		if (fileFriendly) {
			date.Replace(" ", "_");
			date.Replace(".", "-");
			date.Replace(":", "-");
		}

		return date;
	}
};
