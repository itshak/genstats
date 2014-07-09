#ifndef DATABASE_H
#define	DATABASE_H

#include <cstring>
#include <vector>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <sqlite3.h>

#include "types.h"

namespace DataAccess{

    namespace {
       
        sqlite3 *gen;
        
        int loadOrSaveDb(sqlite3 *pInMemory, const char *zFilename, int isSave)
        {
            int rc;                   /* Function return code */
            sqlite3 *pFile;           /* Database connection opened on zFilename */
            sqlite3_backup *pBackup;  /* Backup object used to copy data */
            sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
            sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */

            rc = sqlite3_open(zFilename, &pFile);
            if( rc==SQLITE_OK )
            {
                pFrom = (isSave ? pInMemory : pFile);
                pTo   = (isSave ? pFile     : pInMemory);

                pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
                if( pBackup ){
                  (void)sqlite3_backup_step(pBackup, -1);
                  (void)sqlite3_backup_finish(pBackup);
                }
                rc = sqlite3_errcode(pTo);
            }

            (void)sqlite3_close(pFile);
            return rc;
          }
    }
   
    void open_db();

    void close_db();

    bool is_opened();
   
   
    //======== For ALL ==================
    void getAllCmp(ComposType ct, sqlite3_callback callback, void* data=NULL);
   
    void getCmpByID(ComposType ct, int ID, sqlite3_callback callback);
   
    void getCmpByTheme(ComposType ct, Theme t, sqlite3_callback callback);

    void getCmpByScore(ComposType ct, sqlite3_callback callback, 
            double score=50.0, void* data=NULL);

    //========= PieceStatistics =====================

    void getPSByTheme(Theme t, sqlite3_callback callback);
      
   
    //========= Generation Statistics =================
   
    void getThemes(Theme t, sqlite3_callback callback);

    void getBonuses(Theme t, sqlite3_callback callback);

    void getPenalties(Theme t, sqlite3_callback callback);

    void getGS(sqlite3_callback callback);
}

#endif	/* DATABASE_H */

