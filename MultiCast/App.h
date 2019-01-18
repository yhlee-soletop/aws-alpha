#ifndef APP_H
#define APP_H

#include <QCoreApplication>
#include <QHash>
#include "RecvThread.h"
#include "DataQueue.h"
#include "Common.h"

class App : public QCoreApplication
{
	Q_OBJECT

public:
	App(int argc, char **argv);
	~App();

	bool loadConfig();
	void start();

private:
	void proc( QByteArray data );
	void onStartData( const QByteArray& data );
	void onReceviedData( const QByteArray& data );
	void onFinishedData( const QByteArray& data );

	int getFileID( const char* buf );
	int getFileSize( const char* buf, int offset );
	int getCount( const char* buf );
    void save( FILE_DATA& df );

private slots:
	void slotTimeout();
	void slotSaveFinished();

private:
	QHash< int, FILE_DATA > _dataBuffer;	
	QString	_ip;
	short		_port;

	QList<RULE> _rules;
	RecvThread	_recv;		// UDP 수신 스레드
	DataQueue<QByteArray*>	_buffer;
};

#endif // APP_H
