#include <cstring>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <sqlite3.h>
#include <sstream>
#include <algorithm>

#include "dataaccess.h"

namespace DataAccess{

    void open_db()
    {     
        // return if already opened
        if(gen) {
            fprintf(stderr, "db already opened\n");
            return;
        }

        // create DB in memory
        if(SQLITE_OK == sqlite3_open(":memory:", &gen))
        {
            // load data from file to memory
            if(loadOrSaveDb(gen, "../../data/gen.db3", 0))
            {
                fprintf(stderr, "Can't load data from CREATED db\n");
                return;
            }

        }
        else {
            fprintf(stderr, "Can't open in-memory CREATED db: %s\n", sqlite3_errmsg(gen));
            return;
        }

        std::cout << "db opened" << std::endl;
    }
    
    void close_db()
    {
        // Check if DB exists in memory
        if(gen)
        {
            if( sqlite3_close(gen) )
            {
                fprintf(stderr, "Can't close database in memory: %s\n",
                               sqlite3_errmsg(gen));
                return;
            }
                
            gen=NULL;
        }
        else {
            fprintf(stderr, "db doesn't exists yet in memory\n");
            return;

        }

        std::cout << "db closed" << std::endl;
    }
    
    bool is_opened()
    {
        return gen != NULL;
    }
    
    void getAllCmp(ComposType ct, sqlite3_callback callback, void* data)
    {
        sqlite3 *db = gen;
        const char* sql = ct==DEL_IMP? "SELECT * from DEL_IMP_COMPOS" :
                ct==ADM_IMP? "SELECT * from ADM_IMP_COMPOS AS FIC JOIN \
                               ADM_FULL_LEVEL_STATISTICS AS LS WHERE FIC.ID = LS.COMPOS_ID" :
                ct==FULL_IMP? "SELECT * from FULL_IMP_COMPOS AS FIC JOIN \
                               FULL_LEVEL_STATISTICS AS LS WHERE FIC.ID = LS.COMPOS_ID" :
                              "SELECT * from GEN_COMPOS";
        
        char *zErrMsg = 0;

        if( sqlite3_exec(db, sql, callback, data, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    
    void getCmpByTheme(ComposType ct, Theme t, sqlite3_callback callback)
    {        
        std::stringstream sql;
        
        const char* q = ct==CREATED? "SELECT * from GEN_COMPOS" :
                ct==DEL_IMP? "SELECT * from DEL_IMP_COMPOS" :
                ct==ADM_IMP? "SELECT * from ADM_IMP_COMPOS AS FIC JOIN \
                               ADM_FULL_LEVEL_STATISTICS AS LS WHERE FIC.ID = LS.COMPOS_ID" :
                             "SELECT * from FULL_IMP_COMPOS AS FIC JOIN \
                               FULL_LEVEL_STATISTICS AS LS WHERE FIC.ID = LS.COMPOS_ID";    

        sql << q << "\nwhere THEMES & " << ThemeBB[t] << " = " << ThemeBB[t];
        
        //std::cout << sql.str() << std::endl;


        char *zErrMsg = 0;

        if( sqlite3_exec(gen, sql.str().c_str(), callback, NULL, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);            
        }
    }    

    void getCmpByScore(ComposType ct, sqlite3_callback callback, double score, void* data)
    {        
        std::stringstream sql;
        
        const char* q = ct==CREATED? "SELECT * from GEN_COMPOS" :
                ct==DEL_IMP? "SELECT * from DEL_IMP_COMPOS" :
                ct==ADM_IMP? "SELECT * from ADM_IMP_COMPOS AS FIC JOIN \
                               ADM_FULL_LEVEL_STATISTICS AS LS WHERE FIC.ID = LS.COMPOS_ID" :
                             "SELECT * from FULL_IMP_COMPOS AS FIC JOIN \
                               FULL_LEVEL_STATISTICS AS LS WHERE FIC.ID = LS.COMPOS_ID";    
        
        sql << q << "\nwhere SCORE >= " << score;
        
        char *zErrMsg = 0;

        if( sqlite3_exec(gen, sql.str().c_str(), callback, data, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);            
        }
    }
    
    void getCmpByID(ComposType ct, int ID, sqlite3_callback callback)
    {        
        sqlite3 *db = gen;
        
        std::stringstream sql;
       
        const char* q = ct==DEL_IMP? "SELECT * from DEL_IMP_COMPOS WHERE " :
                ct==ADM_IMP? "SELECT * from ADM_IMP_COMPOS AS FIC JOIN \
                               ADM_LEVEL_STATISTICS AS LS \
                               WHERE FIC.ID = LS.COMPOS_ID AND " :
                ct==FULL_IMP? "SELECT * from FULL_IMP_COMPOS AS FIC JOIN \
                               FULL_LEVEL_STATISTICS AS LS \
                               WHERE FIC.ID = LS.COMPOS_ID AND " :
                              "SELECT * from GEN_COMPOS WHERE ";
        sql << q << "ID = " << ID;
        
        //std::cout << sql.str() << std::endl;
        
        char *zErrMsg = 0;

        if( sqlite3_exec(db, sql.str().c_str(), callback, NULL, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);            
        }
    }
    
    void getPSByTheme(Theme t, sqlite3_callback callback)
    {
        std::stringstream sql;
        
        sql <<  "SELECT * from PIECE_STATISTICS\n \
                where THEME = " << t;
                
        char *zErrMsg = 0;
        const Theme* data =&t;

        if( sqlite3_exec(gen, sql.str().c_str(), callback, (void*)data, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);            
        }
    }
    
    void getThemes(Theme t, sqlite3_callback callback)
    {
        std::stringstream sql;

        sql <<  "SELECT * from GEN_THEMES\n \
                 WHERE THEME_ID = " << t;

        char *zErrMsg = 0;

        if( sqlite3_exec(gen, sql.str().c_str(), callback, NULL, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }

    void getBonuses(Theme t, sqlite3_callback callback)
    {
        std::stringstream sql;

        sql <<  "SELECT * from GEN_BONUSES\n \
                 WHERE THEME_ID = " << t;

        char *zErrMsg = 0;

        if( sqlite3_exec(gen, sql.str().c_str(), callback, NULL, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }

    void getPenalties(Theme t, sqlite3_callback callback)
    {
        std::stringstream sql;

        sql <<  "SELECT * from GEN_PENALTIES\n \
                 WHERE THEME_ID = " << t;

        char *zErrMsg = 0;

        if( sqlite3_exec(gen, sql.str().c_str(), callback, NULL, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }

    void getGS(sqlite3_callback callback)
    {
        std::stringstream sql;

        sql <<  "SELECT * from GEN_STATISTICS";
                
        char *zErrMsg = 0;

        if( sqlite3_exec(gen, sql.str().c_str(), callback, NULL, &zErrMsg) )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);            
        }
    }
}


