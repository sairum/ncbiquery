#include <QXmlStreamReader>
#include <QStringList>
#include <QDebug>

#include "efetch.h"

bool Efetch::parseXML(const QByteArray http_response )
{
    QString _elementname    {""};
    bool    _xmlerror       {false};

    QXmlStreamReader    _xml( QString::fromUtf8( http_response ) );

    // We retreive the full Genbank record in XML format because, apart from
    // the sequence itself and the accession number, there are many attributes
    // that may be of interest for the sequence list, namely the voucher
    // (isolate) label, to relate different sequencesof the same individuals,
    // the country where the specimen was sampled, latitude/longitude (if
    // available), etc. All these attributes are in XML elements of the form
    //
    // <GBQualifier>
    //    <GBQualifier_name>organism</GBQualifier_name>
    //    <GBQualifier_value>Idotea pelagica</GBQualifier_value>
    // </GBQualifier>
    //
    // The problem with these "qualifiers" is that their name is not part of
    // the tag, such as <GBSeq_sequence> (the tag for a sequence). For example,
    // the name of the organism is on a <GBQualifier> which has a subtype
    // <GBQualifier_name> with value "organism". The name of the organism
    // itself is on the other subtype <GBQualifier_value>. Whenever we read a
    // <GBQualifier_name> we store it on QString 'qualname'. Then, within the
    // while loop we advance for the next element with a call to
    // 'xml.readNextStartElement()'. If the XML is well formed the next element
    // should be a <GBQualifier_value>, so we store its value on the respective
    // variable.
    //
    // The other elements of interest are
    //
    // <GBSeq_sequence> and <GBSeq_accession-version>
    //
    // but their values can be read directly because their name is clearly
    // depicted on the tag name, not as the value of a subtype!


    while ( !_xml.atEnd() && !_xml.hasError() )
    {
        _xml.readNext();

        // The element name returned by xml.name() after a call to the function
        // xml.readNext() is a UTF16 encoded QStringView. It is better to
        // transform it into a standard QString for subsequent comparisons with
        // string literals

        _elementname = _xml.name().toString();

        // Check if we are reading a Start Element

        if( _xml.isStartElement() )
        {
            // qDebug() << _elementname.toStdString();

            if( _elementname == "GBSeq" )
            {
                // Every record in a <GBset> (Genbank XML response) is included
                // in a <GBSeq></GBSeq> pair of tags. For each new record, we
                // should clear the respective attribute fields

                // qDebug() << "\n--------------";

                _records++;
            }
            else if ( _elementname == "GBSeq_sequence" )
            {
                // This element represents a true sequence
                QString sequence = _xml.readElementText();
                // qDebug() << "Sequence: " << sequence.toStdString();
            }
            else if ( _elementname == "GBSeq_accession-version")
            {
                // This element holds the accession number + version
                // of the sequence in <GBSeq_sequence>
                QString accession = _xml.readElementText();
                // qDebug() << "Accession: " << accession.toStdString();
            }
            else if( _elementname == "GBSeqid" )
            {
                // There are two <GBSeqid> tags by record enclosed inside a
                // <GBSeq_other-seqids> tag. One has content of the form
                // "gb|KU530525.1|", that is, its an accession number. We
                // already got it from <GBSeq_accession-version>. The other is
                // of form: "gi|1040737823" and is the only field where we can
                // get the UID (or GID) of the record. So check if the begining
                // of string  is "gi" and split it using "|"

                QString qualname = _xml.readElementText();
                QStringList list = qualname.split('|');
                QString gi {""};
                if( list[0] == "gi" )
                {
                    gi = list[1];
                    qDebug() << "GI: " << gi.toStdString();
                }
            }
            else if( _elementname == "GBQualifier_name" )
            {
                // A <GBQualifier> tag has always two subtags: one called
                // <GBQualifier_name> and the other named <GBQualifier_value>,
                // by that order. So, if we hit a <GBQualifier_name> we should
                // check its value by reading the next element. First read the
                // <GBQualifier_name> inner text.

                QString qualname = _xml.readElementText();

                // Read the next element which should be a <GBQualifier_value>
                // and grab its value

                _xml.readNextStartElement(); // Skip the close tag element

                QString qualvalue = _xml.readElementText();

                if( qualname == "organism" )
                {
                    QString organism = qualvalue;
                    // qDebug() << "Organism: " << organism.toStdString();
                }
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

Efetch::Efetch( const QByteArray http_response )
{
    qDebug() << "Constructing EFetch";
    _error = parseXML( http_response );
}

Efetch::~Efetch()
{
    qDebug() << "Destructing EFetch";
}

bool Efetch::hasError()
{
    return _error;
}

QString Efetch::errorMessage()
{
    return _errorMessage;
}

ulong Efetch::fetchedRecords()
{
    return _records;
}
