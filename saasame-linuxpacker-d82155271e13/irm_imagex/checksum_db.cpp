#include "checksum_db.h"

using namespace macho::windows;
using namespace macho;

bool checksum_db::initial(){
    char *error = 0;
    int rc;
    //if (rc = sqlite3_exec(db, "PRAGMA encoding = \"UTF-16\"", NULL, 0, &error)){
    //    fprintf(stderr, "Can't initialize database: %s\n", sqlite3_errmsg(db));
    //    exit(0);
    //}
    //if (rc = sqlite3_exec(db, "PRAGMA mmap_size=268435456", NULL, 0, &error)){
    //    fprintf(stderr, "Can't initialize database: %s\n", sqlite3_errmsg(db));
    //    exit(0);
    //}

    rc = sqlite3_exec(_db,
        "CREATE TABLE IF NOT EXISTS \"chechsum\" (\"id\" INTEGER PRIMARY KEY NOT NULL UNIQUE DEFAULT 0, \"bitmap\" VARCHAR(1) NOT NULL DEFAULT \"\", \"md5\" VARCHAR(16) NOT NULL DEFAULT \"\", \"crc\" INTEGER NOT NULL DEFAULT 0, \"validated\" INTEGER NOT NULL DEFAULT 1);" , NULL, 0, &error);
    if (rc){
        LOG(LOG_LEVEL_ERROR, L"Can't initialize database: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)));
    }
    else{
        if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO chechsum (id, bitmap, md5, crc, validated) VALUES(?1, ?2, ?3, ?4, ?5)", -1, &_insert_stmt, 0))) {
            if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM chechsum WHERE id = ?1", -1, &_select_stmt, 0))) {
                LOG(LOG_LEVEL_DEBUG, L"Initialized database successfully");
            }
        }
        if (rc){
            LOG(LOG_LEVEL_ERROR, L"Can't initialize database: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)));
        }
    }
    return SQLITE_OK == rc;
}

bool checksum_db::open(std::string path){
    macho::windows::auto_lock lock(_cs);
    close();      
    return (SQLITE_OK == sqlite3_open_v2(path.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) && initial();
}

bool checksum_db::load(std::string path){
    macho::windows::auto_lock lock(_cs);
    int rc;                   /* Function return code */
    sqlite3 *pFile;           /* Database connection opened on zFilename */
    sqlite3_backup *pBackup;  /* Backup object used to copy data */
    sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
    sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */

    /* Open the database file identified by zFilename. Exit early if this fails
    ** for any reason. */
    rc = sqlite3_open(path.c_str(), &pFile);
    if (rc == SQLITE_OK){

        /* If this is a 'load' operation (isSave==0), then data is copied
        ** from the database file just opened to database pInMemory.
        ** Otherwise, if this is a 'save' operation (isSave==1), then data
        ** is copied from pInMemory to pFile.  Set the variables pFrom and
        ** pTo accordingly. */
        pFrom = pFile;
        pTo = _db;

        /* Set up the backup procedure to copy from the "main" database of
        ** connection pFile to the main database of connection pInMemory.
        ** If something goes wrong, pBackup will be set to NULL and an error
        ** code and  message left in connection pTo.
        **
        ** If the backup object is successfully created, call backup_step()
        ** to copy data from pFile to pInMemory. Then call backup_finish()
        ** to release resources associated with the pBackup object.  If an
        ** error occurred, then  an error code and message will be left in
        ** connection pTo. If no error occurred, then the error code belonging
        ** to pTo is set to SQLITE_OK.
        */
        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
        if (pBackup){
            (void)sqlite3_backup_step(pBackup, -1);
            (void)sqlite3_backup_finish(pBackup);
        }
        rc = sqlite3_errcode(pTo);
    }

    /* Close the database connection opened on database file zFilename
    ** and return the result of this function. */
    (void)sqlite3_close(pFile);
    return SQLITE_OK == rc;
}

bool checksum_db::save(std::string path){
    macho::windows::auto_lock lock(_cs);
    int rc;                   /* Function return code */
    sqlite3 *pFile;           /* Database connection opened on zFilename */
    sqlite3_backup *pBackup;  /* Backup object used to copy data */
    sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
    sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */
    boost::filesystem::remove(path);
    /* Open the database file identified by zFilename. Exit early if this fails
    ** for any reason. */
    rc = sqlite3_open(path.c_str(), &pFile);
    if (rc == SQLITE_OK){

        /* If this is a 'load' operation (isSave==0), then data is copied
        ** from the database file just opened to database pInMemory.
        ** Otherwise, if this is a 'save' operation (isSave==1), then data
        ** is copied from pInMemory to pFile.  Set the variables pFrom and
        ** pTo accordingly. */
        pFrom = _db;
        pTo = pFile;

        /* Set up the backup procedure to copy from the "main" database of
        ** connection pFile to the main database of connection pInMemory.
        ** If something goes wrong, pBackup will be set to NULL and an error
        ** code and  message left in connection pTo.
        **
        ** If the backup object is successfully created, call backup_step()
        ** to copy data from pFile to pInMemory. Then call backup_finish()
        ** to release resources associated with the pBackup object.  If an
        ** error occurred, then  an error code and message will be left in
        ** connection pTo. If no error occurred, then the error code belonging
        ** to pTo is set to SQLITE_OK.
        */
        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
        if (pBackup){
            (void)sqlite3_backup_step(pBackup, -1);
            (void)sqlite3_backup_finish(pBackup);
        }
        rc = sqlite3_errcode(pTo);
    }

    /* Close the database connection opened on database file zFilename
    ** and return the result of this function. */
    (void)sqlite3_close(pFile);
    return SQLITE_OK == rc;
}

void checksum_db::close(){
    macho::windows::auto_lock lock(_cs);
    if (_select_stmt){ sqlite3_finalize(_select_stmt); _select_stmt = NULL; }
    if (_insert_stmt){ sqlite3_finalize(_insert_stmt); _insert_stmt = NULL; }
    if (_db) { sqlite3_close_v2(_db); _db = NULL; }
}

bool checksum_db::put(const chechsum &blk){
    macho::windows::auto_lock lock(_cs);
    bool result = false;
    sqlite3_bind_int(_insert_stmt, 1, blk.index);
    sqlite3_bind_text(_insert_stmt, 2, blk.bitmap.c_str(), blk.bitmap.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_stmt, 3, blk.md5.c_str(), blk.md5.length(), SQLITE_STATIC);
    sqlite3_bind_int(_insert_stmt, 4, blk.crc);
    sqlite3_bind_int(_insert_stmt, 5, blk.validated ? 1 : 0);
    int rc = sqlite3_step(_insert_stmt);
    if (rc != SQLITE_DONE) {
        LOG(LOG_LEVEL_ERROR, L"ERROR inserting data: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)));
    }
    else{
        result = true;
    }
    sqlite3_reset(_insert_stmt);
    return result;
}

chechsum::ptr checksum_db::get(int index){
    macho::windows::auto_lock lock(_cs);
    int rc;
    chechsum::ptr result;
    sqlite3_bind_int(_select_stmt, 1, index);
    while (1) {
        rc = sqlite3_step(_select_stmt);
        if (rc == SQLITE_ROW) {
            result = chechsum::ptr(new chechsum());
            result->index = sqlite3_column_int(_select_stmt, 0);
            result->bitmap = (char*)sqlite3_column_text(_select_stmt, 1);
            result->md5 = (char*)sqlite3_column_text(_select_stmt, 2);
            result->crc= sqlite3_column_int(_select_stmt, 3);
            result->validated = sqlite3_column_int(_select_stmt, 4) > 0 ? true : false;
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else{
            break;
        }
    }
    sqlite3_reset(_select_stmt);
    return result;
}

chechsum::vtr checksum_db::get_all(){
    macho::windows::auto_lock lock(_cs);
    chechsum::vtr results;
    sqlite3_stmt *select_all_stmt;
    int rc;
    if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM chechsum", -1, &select_all_stmt, 0))) {
        while (1) {
            rc = sqlite3_step(select_all_stmt);
            if (rc == SQLITE_ROW) {
                chechsum::ptr result = chechsum::ptr(new chechsum());
                result->index = sqlite3_column_int(select_all_stmt, 0);
                result->bitmap = (char*)sqlite3_column_text(_select_stmt, 1);
                result->md5 = (char*)sqlite3_column_text(_select_stmt, 2);
                result->crc = sqlite3_column_int(_select_stmt, 3);
                result->validated = sqlite3_column_int(_select_stmt, 4) > 0 ? true : false;
                results.push_back(result);
            }
            else if (rc == SQLITE_DONE) {
                break;
            }
            else{
                break;
            }
        }
        sqlite3_finalize(select_all_stmt);
    }
    return results;
}

bool checksum_db::merge(std::string path){
    macho::windows::auto_lock lock(_cs);
    char *error = 0;
    int rc;
    std::string merge = boost::str(boost::format("ATTACH DATABASE '%1%' AS mergedb; INSERT OR REPLACE INTO chechsum SELECT * FROM mergedb.chechsum; DETACH DATABASE mergedb;") % path);
    rc = sqlite3_exec(_db, merge.c_str(), NULL, 0, &error);
    if (rc){
        LOG(LOG_LEVEL_ERROR, L"Can't merge database: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)));
    }
    return SQLITE_OK == rc;
}