#include <QCoreApplication>
#include <QDebug>

#include "gbquery.h"

int main(int argc, char *argv[])
{


    QString organism    {""};
    QString marker      {"COI"};
    QString key         {""};

    QCoreApplication a(argc, argv);
    if( argc > 1)
    {
        organism = argv[1];

        // Remove any excessive white speces if present and then replace them
        // by '+' character to be used in URLs
        organism = organism.simplified();
        organism.replace(" ", "+");

        if ( argc > 2 )
        {
            marker = argv[2];
            marker = marker.simplified();
            if( marker.contains(" ") )
            {
                marker = "COI";
                qDebug() << "provide a single marker/gene name!";
            }
        }
        if (argc > 3)
        {
            key = argv[3];
        }

        ulong maxRecords  {20};

        // Check if organism name has spaces and replace them by '+'



        GbQuery *ncbiquery = new GbQuery( &a );

        GbQuery::connect( ncbiquery, &GbQuery::quit,
                          &a, &QCoreApplication::quit );

        GbQuery::connect( ncbiquery, &GbQuery::search,
                          ncbiquery, &GbQuery::searchNCBI );

        ncbiquery->setQueryParams( organism, marker , key, maxRecords );

        emit ncbiquery->search( 0 );

        return a.exec();
    }
    else
    {
        a.quit();
        qDebug() << "usage:\n\tncbi_query <species name> [marker] [api key]";
        qDebug() << "\nUse double quotes if species' name includes spaces such as in \"Munna minuta\"."
                 << "You can ommit the marker/gene name (COI is the default) and the NCBI's API Key.";
    }
}
