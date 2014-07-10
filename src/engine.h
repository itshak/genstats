#ifndef ENGINE_H
#define	ENGINE_H

#include <cstring>
#include <vector>
#include <iostream>
#include <map>

#include "dataaccess.h"

namespace Engine{

    static int print_callback(void *data, int argc, char **argv, char **azColName)
    {
        for(int i=0; i<argc; i++){
            printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        printf("\n");

        return 0;
    }
}

#endif	/* ENGINE_H */

