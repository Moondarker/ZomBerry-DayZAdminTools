class ZomberryLogger {
	private string file_path;
	private bool defaultIO = true;

	void ZomberryLogger() {
		FindFP();
	}

	void Log(string module, string txt) {
		if (defaultIO) {
			Print(GetDate() + " [" + module + "]: " + txt);
		} else {
			FileHandle logFile = OpenFile(file_path, FileMode.APPEND);
			if (logFile != 0) {
				FPrintln(logFile, GetDate() + " [" + module + "]: " + txt);
				CloseFile(logFile);
			} else {
				defaultIO = true;
				Log("ZomBerryLogger", "WARN: !!! Falling back to scripts.txt, can't write to " + file_path);
				Log(module, txt);
			}
		}
	}

	void DebugLog(int lvl, string module, string txt) {
		if (ZomberryBase.GetConfig().GetDebugLvl() >= lvl) {
			Log(module, txt);
		}
	}

	private void FindFP() {
		string temp_path = "";
		if (GetCLIParam("zbryDir", temp_path)) {
			Log("ZomBerryLogger", "INFO: Will try to create log file ZB_log.txt in custom directory: " + temp_path);
			SetDir(temp_path);
		}
		if (defaultIO) {
			temp_path = "$profile:ZomBerry\\";
			Log("ZomBerryLogger", "INFO: Will try to create log file ZB_log.txt in profile directory: " + temp_path);
			SetDir(temp_path);
		}
		if (defaultIO) {
			temp_path = "$profile:";
			Log("ZomBerryLogger", "INFO: Will try to create log file ZB_log.txt in profile root: " + temp_path);
			SetDir(temp_path);
		}
		Log("ZomBerryLogger", "WARN: All attempts to use custom log file failed, using script.log");
	}

	private void SetDir(string fPath) {
		string tPath = fPath + "ZB_log.txt";
		if (!FileExist(fPath)) MakeDirectory(fPath);

		FileHandle logFile = OpenFile(tPath, FileMode.APPEND);
		if (logFile != 0) {
			Log("ZomBerryLogger", "INFO: Switching to logfile: " + tPath);
			defaultIO = false;
			file_path = tPath;
			CloseFile(logFile);
		} else {
			if (FileExist(tPath)) {
				Log("ZomBerryLogger", "INFO: Can't write to file " + tPath);
			} else {
				Log("ZomBerryLogger", "INFO: Can't create file " + tPath + " (try creating one manually)");
			}
		}
	}

	static private string GetDate() {
		int year, month, day, hour, minute, second;

		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		string date = day.ToStringLen(2) + "." + month.ToStringLen(2) + "." + year.ToStringLen(4) + " " + hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2);

		return date;
	}
};
