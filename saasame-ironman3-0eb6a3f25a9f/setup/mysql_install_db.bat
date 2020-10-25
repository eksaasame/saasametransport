Set MY_DATA=%1\mariadb\data
Set MYSQL_INSTALL_DB_EXE=%1\mariadb\bin\mysql_install_db.exe
 
if exist "%MY_DATA%\my.ini" goto END
if not exist "%MYSQL_INSTALL_DB_EXE%" goto ERROR

%MYSQL_INSTALL_DB_EXE% -d "%MY_DATA%\." -p saasameFTW -i 64k

:ERROR

:END



