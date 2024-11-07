#include <QCoreApplication>
#include <QDebug>

#include "gbquery.h"

int main(int argc, char *argv[])
{


    QString organism {""};
    QString marker   {""};

    QCoreApplication a(argc, argv);
    if( argc == 3 )
    {
        organism = argv[1];
        marker = argv[2];
        ulong maxRecords  {20};

        GbQuery *ncbiquery = new GbQuery( &a );

        GbQuery::connect( ncbiquery, &GbQuery::quit,
                          &a, &QCoreApplication::quit );

        GbQuery::connect( ncbiquery, &GbQuery::search,
                          ncbiquery, &GbQuery::searchNCBI );

        ncbiquery->setQueryParams( organism, marker , maxRecords );

        emit ncbiquery->search( 0 );

        return a.exec();
    }
    else
    {
        a.quit();
        qDebug() << "usage:\n\tncbi_query <species> <marker>";
        qDebug() << "\nUse double quotes if species' name includes spaces such as in \"Munna minuta\"";
    }
}
