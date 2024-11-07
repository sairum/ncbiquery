#include <QXmlStreamReader>
#include <QDebug>

#include "esearch.h"

bool Esearch::parseXML(const QByteArray http_response )
{
    QString _elementname    {""};
    bool    _xmlerror       {false};

    QXmlStreamReader    _xml( QString::fromUtf8( http_response ) );

    while ( !_xml.atEnd() && !_xml.hasError() )
    {
        // Read the next XML element. Elements can be of type <StartDocument>
        // ( and <EndDocument>), <DTD>, <StartElement> (and <EndElement>), or
        // <Characters>.

        _xml.readNext();

        // The element name returned by xml.name() after a call to the function
        // xml.readNext()is a UTF16 encoded string. It is wise to transform it
        // into a standard QString for subsequent comparisons with literals

        _elementname = _xml.name().toString();

        // We should break out of the loop when a <TranslationSet>, a
        // <TranslationStack>, or a <QueryTranslation> top elements are found.
        // Some inner elements (such as <Count>) are repeated inside these top
        // level elements and their value may have a differnt meaning. Within
        // <TranslationStack>, the <Count> element may refer to all occurrences
        // of the queried organism (without taking into account the marker) or
        // may refer to all occurrences of the marker (for all organisms)!
        // We are only interested in <Count>, <RetMax>, <RetStart>, and <Id>
        // (the latter inside <IdList>).

        if( _elementname ==  "TranslationStack" ||
            _elementname ==  "TranslationSet"   ||
            _elementname ==  "QueryTranslation" ) _xml.skipCurrentElement();

        if( _xml.isStartElement() )
        {
            if( _elementname == "Id" )
            {
                _idList.append( _xml.readElementText().toULong() );
            }
            else if( _elementname == "Count" )
            {
                _count = _xml.readElementText().toULong();
            }
            else if( _elementname == "RetMax" )
            {
                _retmax = _xml.readElementText().toULong();
            }
            else if( _elementname == "RetStart" )
            {
                _retstart = _xml.readElementText().toULong();
            }
        }

    }
    if ( _xml.hasError() )
    {
        _errorMessage = "XML parse error: " + _xml.errorString();
        _xmlerror = true;
    }
    return _xmlerror;
}

Esearch::Esearch( const QByteArray http_response )
{
    qDebug() << "Constructing Esearch";
    _error = parseXML( http_response );
}

Esearch::~Esearch()
{
    qDebug() << "Destructing Esearch";
}

ulong Esearch::count()
{
    return  _count;
}

ulong Esearch::retMax()
{
    return  _retmax;
}

ulong Esearch::retStart()
{
    return  _retstart;
}

bool Esearch::hasError()
{
    return _error;
}

QList<ulong> Esearch::idList()
{
    return  _idList;
}

QString Esearch::errorMessage()
{
    return _errorMessage;
}
