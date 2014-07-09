#include <QCoreApplication>
#include <QtXlsx>
#include <iostream>
#include <sqlite3.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Check QtXlsx
    QXlsx::Document xlsx;
    xlsx.write("A1","Hello, GenStats!");
    xlsx.saveAs("GenStats.xlsx");

    // Check sqlite3
    sqlite3 *db;

    if( sqlite3_open("test.db", &db) ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
        fprintf(stderr, "Opened database successfully\n");
    }

    sqlite3_close(db);

    return a.exec();
}
