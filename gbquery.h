#ifndef GBQUERY_H
#define GBQUERY_H


#include <QNetworkAccessManager>
#include <QString>
#include <QList>
#include <QObject>

class GbQuery : public QObject
{
    Q_OBJECT

public:
    explicit        GbQuery( QObject * parent = nullptr );
    ~GbQuery();
    void            setQueryParams( const QString, const QString, const ulong );

signals:
    void            search( ulong );
    void            quit();

public slots:
    void            searchNCBI( ulong startAtRecord = 0 );

private:
    QString         _organism       {""};
    QString         _marker         {""};
    QString         _scheme         {"https"};
    QString         _host           {"eutils.ncbi.nlm.nih.gov"};
    QString         _searchPath     {"/entrez/eutils/esearch.fcgi"};
    QString         _fetchPath      {"/entrez/eutils/efetch.fcgi"};
    QString         _searchTerm     {""};
    ulong           _retMax         {20};

    bool            _canQuit        {false};

    QList<ulong>    _giList;

    QNetworkAccessManager           *_manager;

    void            fetchFromNCBI();

private slots:
    void            processESearch();
    void            processEFetch();
};

#endif // GBQUERY_H
