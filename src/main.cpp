#include <QtXlsx>
#include <iostream>
#include <sqlite3.h>

#include "bitmaps.h"
#include "engine.h"

int main(int argc, char *argv[])
{
    Bitmaps::init();

    for (Theme t=THEME_NONE; t<=10; ++t)
        std::cout << ThemeBB[t] << std::endl;

    // Check QtXlsx
    QXlsx::Document xlsx;
    xlsx.write("A1","Hello, GenStats!");
    xlsx.saveAs("GenStats.xlsx");

    // Check sqlite3
    DataAccess::open_db();

    //DataAccess::getCmpByID(CREATED,1000,Engine::print_callback);
    //DataAccess::getAllCmp(CREATED, Engine::print_callback);
    //DataAccess::getGS(Engine::print_callback);
    //DataAccess::getCmpByScore(CREATED,Engine::print_callback,130);
    //DataAccess::getCmpByTheme(CREATED,GRIMSHAW,Engine::print_callback);
    //DataAccess::getThemes(GRIMSHAW,Engine::print_callback);
    //DataAccess::getBonuses(GRIMSHAW,Engine::print_callback);
    //DataAccess::getPenalties(GRIMSHAW,Engine::print_callback);
    DataAccess::getPSByTheme(GRIMSHAW,Engine::print_callback);


    if(DataAccess::is_opened())
        DataAccess::close_db();

    return 0;
}
