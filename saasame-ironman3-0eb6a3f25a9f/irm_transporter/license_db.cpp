#include "license_db.h"

using namespace macho::windows;
using namespace macho;
using namespace saasame::transport;

bool license_db::initial(){
    char *error = 0;
    int rc;
    if (rc = sqlite3_exec(_db, "PRAGMA encoding = \"UTF-8\"", NULL, 0, &error)){
        LOG(LOG_LEVEL_ERROR, L"Can't initialize database: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
    }
    else{
        //if (rc = sqlite3_exec(db, "PRAGMA mmap_size=268435456", NULL, 0, &error)){
        //    fprintf(stderr, "Can't initialize database: %s\n", sqlite3_errmsg(db));
        //    exit(0);
        //}
        rc = sqlite3_exec(_db, "PRAGMA synchronous = OFF", 0, 0, 0); //Improve performance
        rc = sqlite3_exec(_db, "PRAGMA cache_size = 8000", 0, 0, 0); //Increase the cache
        rc = sqlite3_exec(_db, "PRAGMA count_changes = 1", 0, 0, 0); //Returns the number of changes
        rc = sqlite3_exec(_db,
            "CREATE TABLE IF NOT EXISTS \"private_keys\" (\"id\" INTEGER PRIMARY KEY NOT NULL UNIQUE DEFAULT 0, \"key\" VARCHAR(1) NOT NULL UNIQUE DEFAULT \"\", \"comment\" VARCHAR(1) NOT NULL DEFAULT \"\");"
            "CREATE TABLE IF NOT EXISTS \"licenses\" (\"id\" INTEGER PRIMARY KEY NOT NULL UNIQUE DEFAULT 0, \"key\" VARCHAR(1) NOT NULL DEFAULT \"\", \"license\" VARCHAR(1) NOT NULL DEFAULT \"\", \"active\" VARCHAR(1) NOT NULL DEFAULT \"\", \"name\" VARCHAR(1) NOT NULL DEFAULT \"\", \"email\" VARCHAR(1) NOT NULL DEFAULT \"\", \"status\" VARCHAR(1) NOT NULL DEFAULT \"\", \"comment\" VARCHAR(1) NOT NULL DEFAULT \"\" );"
            "CREATE TABLE IF NOT EXISTS \"workloads\" (\"id\" INTEGER PRIMARY KEY NOT NULL UNIQUE DEFAULT 0, \"host\" VARCHAR(1) NOT NULL DEFAULT \"\", \"created\" INTEGER NOT NULL  DEFAULT 0, \"deleted\" INTEGER NOT NULL  DEFAULT 0, \"updated\" INTEGER NOT NULL  DEFAULT 0, \"type\" VARCHAR(1) NOT NULL DEFAULT \"\", \"name\" VARCHAR(1) NOT NULL DEFAULT \"\" , \"comment\" VARCHAR(1) NOT NULL DEFAULT \"\");"
            , NULL, 0, &error);
        if (rc){
            LOG(LOG_LEVEL_ERROR, L"Can't initialize database: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
        }
        else{
            if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO private_keys (id, key, comment) VALUES(?1, ?2, ?3)", -1, &_insert_private_key_stmt, 0))) {
                if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM private_keys WHERE id = 0", -1, &_select_private_key_stmt, 0))) {
                    if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "INSERT INTO licenses (key, license, active, name, email, status, comment) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7)", -1, &_insert_license_stmt, 0))) {
                        if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "UPDATE licenses SET key = ?2, license = ?3, active = ?4, name = ?5, email = ?6, status = ?7, comment = ?8 WHERE id = ?1", -1, &_update_license_stmt, 0))) {
                            if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM licenses", -1, &_select_licenses_stmt, 0))) {
                                if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM licenses WHERE key = ?1 AND status != \"x\"", -1, &_select_license_stmt, 0))) {
                                    if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "INSERT INTO workloads (host, created, deleted, updated, type, name, comment) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7)", -1, &_insert_workload_stmt, 0))) {
                                        if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "UPDATE workloads SET host = ?2, created = ?3, deleted = ?4, updated = ?5, type = ?6, name = ?7, comment = ?8 WHERE id = ?1", -1, &_update_workload_stmt, 0))) {
                                            if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM workloads ORDER BY created", -1, &_select_workloads_stmt, 0))) {
                                                if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM workloads WHERE host = ?1", -1, &_select_workload_stmt, 0))) {
                                                    if (SQLITE_OK == (rc = sqlite3_prepare_v2(_db, "SELECT * FROM workloads WHERE host = ?1 AND created = ?2", -1, &_select_workload_stmt2, 0))) {
                                                        LOG(LOG_LEVEL_DEBUG, L"Initialized database successfully");
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (rc){
                LOG(LOG_LEVEL_ERROR, L"Can't initialize database: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
            }
        }
    }
    return SQLITE_OK == rc;
}

license_db::ptr license_db::open(std::string path, std::string password){
    license_db::ptr _db(new license_db());
    if (_db->_open(path, password))
        return _db;
    return NULL;
}

bool license_db::_open(std::string path, std::string password){
    macho::windows::auto_lock lock(_cs);
    close();      
    return (SQLITE_OK == sqlite3_open_v2(path.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
        && (password.empty() || SQLITE_OK == sqlite3_key(_db, password.c_str(), password.length()))
        && initial();
}

bool license_db::save(std::string path, std::string password){
    macho::windows::auto_lock lock(_cs);
    int rc;                   /* Function return code */
    sqlite3 *pFile;           /* Database connection opened on zFilename */
    sqlite3_backup *pBackup;  /* Backup object used to copy data */
    sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
    sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */
    boost::filesystem::remove(path);
    /* Open the database file identified by zFilename. Exit early if this fails
    ** for any reason. */
    rc = sqlite3_open_v2(path.c_str(), &pFile, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc == SQLITE_OK && !password.empty())
        rc = sqlite3_key(pFile, password.c_str(), password.length());
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
    sqlite3_close_v2(pFile);
    return SQLITE_OK == rc;
}

void license_db::close(){
    macho::windows::auto_lock lock(_cs);
    if (_select_private_key_stmt){ sqlite3_finalize(_select_private_key_stmt); _select_private_key_stmt = NULL; }
    if (_insert_private_key_stmt){ sqlite3_finalize(_insert_private_key_stmt); _insert_private_key_stmt = NULL; }
    if (_select_licenses_stmt){ sqlite3_finalize(_select_licenses_stmt); _select_licenses_stmt = NULL; }
    if (_select_license_stmt){ sqlite3_finalize(_select_license_stmt); _select_license_stmt = NULL; }
    if (_insert_license_stmt){ sqlite3_finalize(_insert_license_stmt); _insert_license_stmt = NULL; }
    if (_update_license_stmt){ sqlite3_finalize(_update_license_stmt); _update_license_stmt = NULL; }
    if (_select_workloads_stmt){ sqlite3_finalize(_select_workloads_stmt); _select_workloads_stmt = NULL; }
    if (_select_workload_stmt){ sqlite3_finalize(_select_workload_stmt); _select_workload_stmt = NULL; }
    if (_select_workload_stmt2){ sqlite3_finalize(_select_workload_stmt2); _select_workload_stmt2 = NULL; }
    if (_insert_workload_stmt){ sqlite3_finalize(_insert_workload_stmt); _insert_workload_stmt = NULL; }
    if (_update_workload_stmt){ sqlite3_finalize(_update_workload_stmt); _update_workload_stmt = NULL; }
    if (_db) { sqlite3_close_v2(_db); _db = NULL; }
}

private_key::ptr license_db::get_private_key(){
    macho::windows::auto_lock lock(_cs);
    int rc;
    private_key::ptr result;
    while (1) {
        rc = sqlite3_step(_select_private_key_stmt);
        if (rc == SQLITE_ROW) {
            result = private_key::ptr(new private_key());
            result->index = sqlite3_column_int64(_select_private_key_stmt, 0);
            result->key = (char*)sqlite3_column_text(_select_private_key_stmt, 1);
            result->comment = (char*)sqlite3_column_text(_select_private_key_stmt, 2);
            break;
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else{
            break;
        }
    }
    sqlite3_reset(_select_private_key_stmt);
    return result;
}

bool license_db::put_private_key(const private_key& key){
    macho::windows::auto_lock lock(_cs);
    bool ret = false;
    sqlite3_bind_int64(_insert_private_key_stmt, 1, key.index);
    sqlite3_bind_text(_insert_private_key_stmt, 2, key.key.c_str(), key.key.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_private_key_stmt, 3, key.comment.c_str(), key.comment.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_insert_private_key_stmt);
    if (rc == SQLITE_ROW){
        ret = true;
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"ERROR inserting data: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
    }
    sqlite3_reset(_insert_private_key_stmt);
    return ret;
}

license::vtr license_db::get_licenses(){
    macho::windows::auto_lock lock(_cs);
    license::vtr results;
    while (1) {
        int rc = sqlite3_step(_select_licenses_stmt);
        if (rc == SQLITE_ROW) {
            license::ptr result = license::ptr(new license());
            result->index = sqlite3_column_int64(_select_licenses_stmt, 0);
            result->key = (char*)sqlite3_column_text(_select_licenses_stmt, 1);
            result->lic = (char*)sqlite3_column_text(_select_licenses_stmt, 2);
            result->active = (char*)sqlite3_column_text(_select_licenses_stmt, 3);
            result->name = (char*)sqlite3_column_text(_select_licenses_stmt, 4);
            result->email = (char*)sqlite3_column_text(_select_licenses_stmt, 5);
            result->status = (char*)sqlite3_column_text(_select_licenses_stmt, 6);
            result->comment = (char*)sqlite3_column_text(_select_licenses_stmt, 7);
            results.push_back(result);
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else{
            break;
        }
    }
    sqlite3_reset(_select_licenses_stmt);
    return results;
}

bool license_db::insert_license(license &lic){
    macho::windows::auto_lock lock(_cs);
    bool ret = false;
    sqlite3_bind_text(_insert_license_stmt, 1, lic.key.c_str(), lic.key.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_license_stmt, 2, lic.lic.c_str(), lic.lic.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_license_stmt, 3, lic.active.c_str(), lic.active.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_license_stmt, 4, lic.name.c_str(), lic.name.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_license_stmt, 5, lic.email.c_str(), lic.email.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_license_stmt, 6, lic.status.c_str(), lic.status.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_license_stmt, 7, lic.comment.c_str(), lic.comment.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_insert_license_stmt);
    if (rc == SQLITE_ROW){
        lic.index = sqlite3_last_insert_rowid(_db);
        ret = true;
    }
    else if (rc == SQLITE_DONE) {
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"ERROR inserting data: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
    }
    sqlite3_reset(_insert_license_stmt);
    return ret;
}

license::ptr license_db::get_license(const std::string key){
    macho::windows::auto_lock lock(_cs);
    int rc;
    license::ptr result;
    sqlite3_bind_text(_select_license_stmt, 1, key.c_str(), key.length(), SQLITE_STATIC);
    while (1) {
        rc = sqlite3_step(_select_license_stmt);
        if (rc == SQLITE_ROW) {
            result = license::ptr(new license());
            result->index = sqlite3_column_int64(_select_license_stmt, 0);
            result->key = (char*)sqlite3_column_text(_select_license_stmt, 1);
            result->lic = (char*)sqlite3_column_text(_select_license_stmt, 2);
            result->active = (char*)sqlite3_column_text(_select_license_stmt, 3);
            result->name = (char*)sqlite3_column_text(_select_license_stmt, 4);
            result->email = (char*)sqlite3_column_text(_select_license_stmt, 5);
            result->status = (char*)sqlite3_column_text(_select_license_stmt, 6);
            result->comment = (char*)sqlite3_column_text(_select_license_stmt, 7);
            break;
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else{
            break;
        }
    }
    sqlite3_reset(_select_license_stmt);
    return result;
}

bool license_db::update_license(const license &lic){
    macho::windows::auto_lock lock(_cs);
    bool ret = false;
    sqlite3_bind_int64(_update_license_stmt, 1, lic.index);
    sqlite3_bind_text(_update_license_stmt, 2, lic.key.c_str(), lic.key.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_license_stmt, 3, lic.lic.c_str(), lic.lic.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_license_stmt, 4, lic.active.c_str(), lic.active.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_license_stmt, 5, lic.name.c_str(), lic.name.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_license_stmt, 6, lic.email.c_str(), lic.email.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_license_stmt, 7, lic.status.c_str(), lic.status.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_license_stmt, 8, lic.comment.c_str(), lic.comment.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_update_license_stmt);
    if (rc == SQLITE_ROW){
        ret = true;
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"ERROR update data: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
    }
    sqlite3_reset(_update_license_stmt);
    return ret;
}

workload::map license_db::get_workloads_map(){
    workload::map results;
    workload::vtr workloads = get_workloads();
    foreach(workload::ptr w, workloads){
        results[w->host].push_back(w);
    }
    return results;
}

workload::vtr license_db::get_workloads(){
    macho::windows::auto_lock lock(_cs);
    workload::vtr results;
    while (1) {
        int rc = sqlite3_step(_select_workloads_stmt);
        if (rc == SQLITE_ROW) {
            workload::ptr result = workload::ptr(new workload());
            result->index = sqlite3_column_int64(_select_workloads_stmt, 0);
            result->host = (char*)sqlite3_column_text(_select_workloads_stmt, 1);
            result->created = sqlite3_column_int64(_select_workloads_stmt, 2);
            result->deleted = sqlite3_column_int64(_select_workloads_stmt, 3);
            result->updated = sqlite3_column_int64(_select_workloads_stmt, 4);
            result->type = (char*)sqlite3_column_text(_select_workloads_stmt, 5);
            result->name = (char*)sqlite3_column_text(_select_workloads_stmt, 6);
            result->comment = (char*)sqlite3_column_text(_select_workloads_stmt, 7);
            results.push_back(result);
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else{
            break;
        }
    }
    sqlite3_reset(_select_workloads_stmt);
    return results;
}

bool license_db::insert_workload(workload &host){
    macho::windows::auto_lock lock(_cs);
    bool ret = false;
    sqlite3_bind_text(_insert_workload_stmt, 1, host.host.c_str(), host.host.length(), SQLITE_STATIC);
    sqlite3_bind_int64(_insert_workload_stmt, 2, host.created);
    sqlite3_bind_int64(_insert_workload_stmt, 3, host.deleted);
    sqlite3_bind_int64(_insert_workload_stmt, 4, host.updated);
    sqlite3_bind_text(_insert_workload_stmt, 5, host.type.c_str(), host.type.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_workload_stmt, 6, host.name.c_str(), host.name.length(), SQLITE_STATIC);
    sqlite3_bind_text(_insert_workload_stmt, 7, host.comment.c_str(), host.comment.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_insert_workload_stmt);
    if (rc == SQLITE_ROW){
        host.index = sqlite3_last_insert_rowid(_db);
        ret = true;
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"ERROR inserting data: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
    }
    
    sqlite3_reset(_insert_workload_stmt);
    return ret;
}

bool license_db::update_workload(const workload &host){
    macho::windows::auto_lock lock(_cs);
    bool ret = false;
    sqlite3_bind_int64(_update_workload_stmt, 1, host.index);
    sqlite3_bind_text(_update_workload_stmt, 2, host.host.c_str(), host.host.length(), SQLITE_STATIC);
    sqlite3_bind_int64(_update_workload_stmt, 3, host.created);
    sqlite3_bind_int64(_update_workload_stmt, 4, host.deleted);
    sqlite3_bind_int64(_update_workload_stmt, 5, host.updated);
    sqlite3_bind_text(_update_workload_stmt, 6, host.type.c_str(), host.type.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_workload_stmt, 7, host.name.c_str(), host.name.length(), SQLITE_STATIC);
    sqlite3_bind_text(_update_workload_stmt, 8, host.comment.c_str(), host.comment.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_update_workload_stmt);
    if (rc == SQLITE_ROW){
        ret = true;
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"ERROR inserting data: %s", macho::stringutils::convert_ansi_to_unicode(sqlite3_errmsg(_db)).c_str());
    }
    sqlite3_reset(_update_workload_stmt);
    return ret;
}

workload::vtr license_db::get_workload(const std::string host){
    macho::windows::auto_lock lock(_cs);
    int rc;
    workload::vtr results;
    sqlite3_bind_text(_select_workload_stmt, 1, host.c_str(), host.length(), SQLITE_STATIC);
    while (1) {
        rc = sqlite3_step(_select_workload_stmt);
        if (rc == SQLITE_ROW) {
            workload::ptr result = workload::ptr(new workload());
            result->index = sqlite3_column_int64(_select_workload_stmt, 0);
            result->host = (char*)sqlite3_column_text(_select_workload_stmt, 1);
            result->created = sqlite3_column_int64(_select_workload_stmt, 2);
            result->deleted = sqlite3_column_int64(_select_workload_stmt, 3);
            result->updated = sqlite3_column_int64(_select_workload_stmt, 4);
            result->type = (char*)sqlite3_column_text(_select_workload_stmt, 5);
            result->name = (char*)sqlite3_column_text(_select_workload_stmt, 6);
            result->comment = (char*)sqlite3_column_text(_select_workload_stmt, 7);
            results.push_back(result);
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else{
            break;
        }
    }
    sqlite3_reset(_select_workload_stmt);
    return results;
}

workload::ptr license_db::get_workload(const std::string host, const std::time_t created){
    macho::windows::auto_lock lock(_cs);
    int rc;
    workload::ptr result;
    sqlite3_bind_text(_select_workload_stmt2, 1, host.c_str(), host.length(), SQLITE_STATIC);
    sqlite3_bind_int64(_select_workload_stmt2, 2, created);
    while (1) {
        rc = sqlite3_step(_select_workload_stmt2);
        if (rc == SQLITE_ROW) {
            result = workload::ptr(new workload());
            result->index = sqlite3_column_int64(_select_workload_stmt2, 0);
            result->host = (char*)sqlite3_column_text(_select_workload_stmt2, 1);
            result->created = sqlite3_column_int64(_select_workload_stmt2, 2);
            result->deleted = sqlite3_column_int64(_select_workload_stmt2, 3);
            result->updated = sqlite3_column_int64(_select_workload_stmt2, 4);
            result->type = (char*)sqlite3_column_text(_select_workload_stmt2, 5);
            result->name = (char*)sqlite3_column_text(_select_workload_stmt2, 6);
            result->comment = (char*)sqlite3_column_text(_select_workload_stmt2, 7);
            break;
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else{
            break;
        }
    }
    sqlite3_reset(_select_workload_stmt2);
    return result;
}
