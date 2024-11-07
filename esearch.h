#ifndef ESEARCH_H
#define ESEARCH_H

#include <QByteArray>
#include <QString>
#include <QList>

class Esearch
{
    ulong               _count          {0};
    ulong               _retmax         {0};
    ulong               _retstart       {0};
    bool                _error          {false};
    QList<ulong>        _idList;
    QString             _errorMessage   {"No error parsing XML source"};

    bool                parseXML( const QByteArray );

public:
    Esearch( QByteArray );
    ~Esearch();
    ulong           count();
    ulong           retMax();
    ulong           retStart();
    bool            hasError();
    QList<ulong>    idList();
    QString         errorMessage();
};

#endif // ESEARCH_H
