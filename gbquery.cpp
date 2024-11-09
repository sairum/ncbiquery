#include <QNetworkReply>
#include <QDebug>
#include <QMutex>

#include "esearch.h"
#include "efetch.h"
#include "gbquery.h"



// The normal way to deal with the asyncronous nature of http requests is to
// link the 'finished' SIGNAL of 'QNetworkAcessManager' with a SLOT (usually
// from the class that manages http connections, which in the case is GbQuery,
// and often named 'finished'). This SLOT will process the 'reply' (a
// QNetworkReply type variable) and then deletes it.
//
// connect( manager, &QNetworkAccessManager::finished,
//          this   , &GbQuery::finished );
//
// However, in this program we must use the SIGNAL 'finished' in two different
// circumstances, because queries to NCBI are always a two-stage process.
// First we query the NCBI nucleotide database for retrieving all indexes (GIs
// or GenBank indexes) using a combination of species and gene/marker names
// ('esearch' tool). If the GIs list is not null, we retrieve the records found
// in the previous query using the GIs numbers ('efetch' tool). Both https
// replies are returned as XML formatted results. We cannot easily tell them
// apart without decoding the XML, which is substantially different for each
// case. For 'esearch' the DTD name is a <GBSet>, whereas for 'efetch' it is an
// <eFetchResult>. The best approach is to NOT USE the 'finished' SIGNAL of the
// QNetworkAcessManager, but instead use the same SIGNAL associated to the
// QNetworkReply which is returned from the POST or GET methods of the
// QNetworkAccessManager. So, the SLOT 'searchNCBI' prepares a search query and
// sends it via GET associating the resulting QNetworkReply SIGNAL 'finished'
// with the SLOT 'processESearch'
//
// QNetworkReply *reply = manager->get( esearchRequest );
//
// connect( reply, &QNetworkReply::finished,
//          this , &GbQuery::processESearch );
//
// whilst SLOT 'fetchFromNCBI' prepares a fetch query and sends it via GET,
// associating the resulting QNetworkReply SIGNAL 'finished'  with the SLOT
// 'processEfetch'
//
// QNetworkReply *reply = manager->get( efetchRequest );
//
// connect( reply, &QNetworkReply::finished,
//          this , &GbQuery::processEFetch );
//
// The only problem with this approach is that SLOT 'finished' from
// QNetworkReply does not carry the pointer to a QNetworkReply result as an
// argument, just as the SLOT 'finished' of QNetworkAccessManager do. This
// implies the usage of the method 'sender' to get the QNetworkReply object
// as in
//
// QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );
//
// See
//
// https://stackoverflow.com/questions/53164113/qnetworkaccessmanager-connected-to-2-reply-slots-how-do-i-know-which-reply-belo
//
// Finally, to complicate things further, NCBI searches should be limited to a
// 'reasonable' amount of returned records. After all, it is a public and free
// service and people should use it sparingly. The default is set usually to 20
// records per search but this can be explicitly set in the query to 'esearch'
// through the 'retmax' option. However, a normal search may return a result
// set that includes more than 'retmax' records. In these cases, what the
// 'esearch' tool does is to keep track of a variable that stores where to
// begin retrieving records (or, put in another way, how many records to skip
// before starting retrieving records). This variable called 'retstart' is
// returned in each query to 'esearch'. The 'esearch' result formatted as XML
// provides among other things (e.g. the list of GIs), the total number of
// records matched by the query string in a variable named 'count'. If 'count'
// is larger than the sum of 'retstart' and 'retmax', we increase 'retstart' by
// 'retmax' records and emit the SIGNAL 'search' which carries the new
// 'retstart' value and grabs more GIs from NCBI's 'esearch'.
//
//
//                        SIGNAL MainWindow::search( retstart = 0 )
//                                          |
//                                          V
//                   SLOT GbQuery::searchNCBI( retstart )
//                             ^            |
//     ________________________|            |
//     |                                    V
//     |                     SIGNAL QNetworkReply::finished
//     |                                    |
//     |                                    V
//     |                     SLOT GbQuery::processESearch
//     |                                    |
//     |                                    V
//     |                get count ( = total records matching query )
//     |                get retmax records' ids starting at retstart
//     |                                    |
//     |                                    V
//     |                    _________________________________
//     |                    |                               |
//     |                    |                               V
//     |                    |                   GbQuery::fetchFromNCBI()
//     |                    V                               |
//     |        count > retstart + retmax                   V
//     |          |                   |      SIGNAL QNetworkReply::finished
//     |          |                   |                     |
//     |         Yes                  No                    |
//     |          |                   |                     V
//     |  retstart += retmax          |       SLOT GbQuery::processEfetch
//     |          |                   |                     |
//     |          v                   |                     |
// SIGNAL GbQuery::search( retstart ) |                     |
//                                    |                     |
//                                    V                     V
//                                   STOP                  STOP
//

GbQuery::GbQuery( QObject *parent )
    : QObject( parent )
{
    _manager = new QNetworkAccessManager( this );
    qDebug() << "Constructing GbQuery";
}

GbQuery::~GbQuery()
{
    qDebug() << "Destructing GbQuery";
}

void GbQuery::setQueryParams(const QString organism,
                             const QString marker,
                             const QString key,
                             const ulong retMaxRecords )
{
    _organism   = organism;
    _marker     = marker;
    _apiKey     = key;
    _searchTerm = organism + "[organism]+AND+" + marker;
    // qDebug() << "Search term: " << _searchTerm;
    _retMax     = retMaxRecords;
}

/*****************************************************************************/
/*                                                                           */
/* 'searchNCBI' composes a query to be submited to NCBI's 'esearch' utils    */
/*                                                                           */
/*****************************************************************************/

void GbQuery::searchNCBI( ulong startAtRecord )
{
    QNetworkRequest request;
    QUrl            url;

    // Compose the request URL with its individual components. The search term
    // (species/genus and gene marker) is in variable '_searchTerm'

    url.setScheme( _scheme );
    url.setHost( _host );
    url.setPath( _searchPath );
    QString query = "db=nuccore&term=" + _searchTerm +
                    "&retmax=" + QString::number( _retMax );

    if( startAtRecord > 0 )
    {
        query += "&retstart=" +  QString::number( startAtRecord );
    }

    // Set the API Key if it exists
    if( _apiKey != "" )
    {
        query += "&api_key=" + _apiKey;
    }

    url.setQuery( query );
    request.setUrl( url );

    qDebug() << url.toString();

    request.setRawHeader( "Content-Type",
                          "text/html,application/xhtml+xml,application/xml" );

    // submit the request to NCBI's eutils!

    QNetworkReply *reply = _manager->get( request );

    connect( reply, &QNetworkReply::finished,
             this,  &GbQuery::processESearch );
}

/*****************************************************************************/
/*                                                                           */
/* 'fetchFromNCBI' composes a query to be submited to NCBI's 'efetch' utils  */
/*                                                                           */
/*****************************************************************************/

void GbQuery::fetchFromNCBI()
{
    QNetworkRequest request;
    QUrl url;

    // Transform the list of GIs into a string

    QStringList gis;
    gis.reserve( _giList.size() );
    for ( const auto &i: _giList )
    {
         gis.append( QString::number( i ) );
    }

    // Turn the GIs list into a comma separated list without spaces

    QString reqList = QStringLiteral("%1").arg(gis.join(','));

    // Clear the list GIs for eventual new searches

    _giList.clear();

    // Compose the request URL with its individual components

    url.setScheme( _scheme );
    url.setHost( _host );
    url.setPath( _fetchPath );
    QString query = "db=nuccore&id=" + reqList;
    query += "&rettype=gb&retmode=xml";
    query += "&retmax=" + QString::number( _retMax );

    // Set the API Key if exists
    if( _apiKey != "" )
    {
        query += "&api_key=" + _apiKey;
    }

    url.setQuery( query );
    request.setUrl( url );

    qDebug() << url.toString();

    request.setRawHeader( "Content-Type",
                          "text/html,application/xhtml+xml,application/xml" );

    // submit the request to NCBI's eutils!

    QNetworkReply *reply = _manager->get( request );

    connect( reply, &QNetworkReply::finished,
             this,  &GbQuery::processEFetch );

}

/*****************************************************************************/
/*                                                                           */
/* The SLOT 'processESearch' is triggered by a SIGNAL from QNetworkReply     */
/* when the search terminates. The connection is established in 'searchNCBI' */
/* It's function is to process the XML received containing the list of GIs   */
/* found for the search query (taxon/gene) submitted to 'esearch. If no      */
/* errors have occurred it should call the function 'fetchFromNCBI' to       */
/* retrieve the records corresponding to the GI list obtained.               */
/*                                                                           */
/*****************************************************************************/

void GbQuery::processESearch()
{
    int             count     {0};
    int             retmax    {0};
    int             retstart  {0};

    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );

    // Mark reply for later deletion

    reply->deleteLater();

    // Check if any error has occurred. If not move on otherwise stop
    // processing this request

    if( reply->error() == QNetworkReply::NoError )
    {
        QByteArray bts = reply->readAll();

        Esearch p( bts );

        if( !p.hasError() )
        {

            count      = p.count();
            retmax     = p.retMax();
            retstart   = p.retStart();

            _giList     = p.idList();

            // Update number of expected records
            setCount( count );

            // qDebug() << "Count:    " << count;
            // qDebug() << "RetMax:   " << retmax;
            // qDebug() << "RetStart: " << retstart;
            // qDebug() << "List of IDs";
            // for( long id : _giList ) qDebug() << id;

            fetchFromNCBI();

            if( retstart + retmax < count )
            {
                retstart += retmax;
                emit search( retstart );
            }
            else
            {
                _canQuit = true;
            }

        } else
        {
            qDebug() << p.errorMessage();
        }
    }
}

/*****************************************************************************/
/*                                                                           */
/* 'processEFetch' is a SLOT linked to the 'finished' SIGNAL of a reply that */
/* is emitted after a NCBI 'efetch' network query. It reads and parses a XML */
/* stream with the data from the fetched NCBI records.                       */
/*                                                                           */
/*****************************************************************************/

void GbQuery::processEFetch()
{

    ulong   records  {0};

    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );

    // Mark the reply for deletion later

    reply->deleteLater();

    // Check if any error has occurred. If not move on otherwise stop
    // processing this request

    if( reply->error() == QNetworkReply::NoError )
    {
        QByteArray bts = reply->readAll();

        Efetch e( bts );

        if( e.hasError() )
        {
            qDebug() << e.errorMessage();
        }

        records = e.fetchedRecords();

        // Update records fetched
        setFetchedRecords( records );
    }
    else
    {
        qDebug() << reply->error();
    }

    if( _canQuit )
    {
        //emit quit();
    }
}

/*****************************************************************************/
/*                                                                           */
/* 'setCount' sets the total number of records expected after the first      */
/* successful query to NCBI. This number is not expected to change during    */
/* subsequent calls to NCBI for the same query.                              */
/*                                                                           */
/*****************************************************************************/

void GbQuery::setCount( ulong count )
{
    if( _count == 0 )
    {
        _count = count;
    }
}

void GbQuery::setFetchedRecords( ulong records )
{
    QMutex mutex;
    mutex.lock();
    _recordsFetched += records;
    mutex.unlock();
    if( _recordsFetched == _count )
    {
        emit quit();
    }
}


