#ifndef EFETCH_H
#define EFETCH_H

#include <QByteArray>
#include <QString>

class Efetch
{
    bool                _error          {false};
    QString             _errorMessage   {"No error parsing XML source"};

    bool    parseXML( const QByteArray );

public:
    Efetch( const QByteArray );
    ~Efetch();
    bool            hasError();
    QString         errorMessage();
};

#endif // EFETCH_H

