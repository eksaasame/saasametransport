========================================================================
    STATIC LIBRARY : sqlite Project Overview
========================================================================

AppWizard has created this sqlite library project for you.

No source files were created as part of your project.


sqlite.vcxproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

sqlite.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

/////////////////////////////////////////////////////////////////////////////
Other notes:

http://love.junzimu.com/archives/510

int result = 0;
sqlite3 *db = NULL;
 
result = sqlite3_open("c:\test.db", &db);
result = sqlite3_key(db, "abcd", 3); //使用密碼，第一次為設置密碼
										//result=sqlite3_rekey(db,NULL,0); //清空密碼
result = sqlite3_exec(db, "PRAGMA synchronous = OFF", 0, 0, 0);    //提高性能
result = sqlite3_exec(db, "PRAGMA cache_size = 8000", 0, 0, 0); //加大緩存
result = sqlite3_exec(db, "PRAGMA count_changes = 1", 0, 0, 0); //返回改變記錄數
result = sqlite3_exec(db, "PRAGMA case_sensitive_like = 1", 0, 0, 0); //支援中文LIKE查詢
 
result = sqlite3_exec(db, "CREATE TABLE [MyTable] ([ID] INTEGER PRIMARY KEY NOT NULL,[MyText] TEXT NULL)", 0, 0, 0);
result = sqlite3_exec(db, "INSERT INTO MyTable (MyText) VALUES ('測試!')", 0, 0, 0);
 
result = sqlite3_close(db);


/////////////////////////////////////////////////////////////////////////////
