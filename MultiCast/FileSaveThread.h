#ifndef FILESAVETHREAD_H
#define FILESAVETHREAD_H

#include <QThread>
#include <QHostAddress>
#include "Common.h"

class FileSaveThread : public QThread
{
    Q_OBJECT

public:
    FileSaveThread( const QList<RULE>& rules, const FILE_DATA& df );
    ~FileSaveThread();

    int fileid() const;

private:
    void run();
    void save( const RULE& rule, const QString& filename, const QByteArray& data );
    QString convertDir( const RULE& rule, const QString& filename );
    void send( const QString& filename, const QByteArray& data );
    void sendHeaderData( const QString& filename, int filesize );
    void sendHRITData( const QString& filename, const QByteArray& data );
    void sendEOFData( const QString& filename );

private slots:
    void slotConnected();

private:
    const QList<RULE>& _rules;
    const FILE_DATA& _df;

    QHostAddress _hostAddress, _clientAddress;
    short	_hostPort, _clientPort;
};

#endif // FILESAVETHREAD_H
