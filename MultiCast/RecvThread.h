#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include <QThread>
#include <QHostAddress>
#include <QUdpSocket>
#include "DataQueue.h"

class RecvThread : public QThread
{
	Q_OBJECT

public:
	RecvThread( DataQueue<QByteArray*>& out );
	~RecvThread();

	void start( const QString& ip, short port );
	void run();

private slots:
	void processPendingDatagrams();

private:
	DataQueue<QByteArray*>& _out;
	QString _ip;
	short _port;

	QHostAddress m_groupAddress;
	QString m_status;
	QUdpSocket* m_udpSocket;
};

#endif // RECVTHREAD_H
