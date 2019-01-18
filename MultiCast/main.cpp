// S300D DVB-S2 RECEIVER 패킷 수신 프로그램
// 2015-12-01 전영찬
//
// 수신기가 MULTICAST 하는 패킷을 받아서 전송 포맷 형식의 파일로 저장
// 설정파일의 설정에 따라 수신 파일을 분류하여 저장
// UDP 특성상 패킷 순서가 바뀌는 것을 고려
// UDP 특성상 패킷을 바로 수신하지 않으면 packet loss가 발생

#include <QtCore/QCoreApplication>
#include "App.h"
//#include <QMessageBox>
#include <QSharedMemory>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QDataStream>

//#define SW_VERSION "v1.00.00"
//#define SW_VERSION "v1.01.00" // 수신 데이터는 모두 저장
//#define SW_VERSION "v1.02.00" // Patch for data loss( socket size increased )
//#define SW_VERSION "v1.03.00" // 소켓 바인딩 IP 추가
//#define SW_VERSION "v1.03.01" // 설정 출력 기능 추가
//#define SW_VERSION "v1.03.05" // NIC 바인딩 설정 출력 기능 추가
//#define SW_VERSION "v1.03.06" // directory config added. $yyyy, $MM, $dd, $hh, $mm
//#define SW_VERSION "v1.03.07" // directory config added. log added
//#define SW_VERSION "v1.03.08" // directory config added. log added
//#define SW_VERSION "v1.03.09" // New function: NWP receiving
//#define SW_VERSION "v1.03.10" //
#define SW_VERSION "v1.03.11" // UDP Send Function added.

QSharedMemory shared("MultiCast");  

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

void preventDoubleRun( int argc )
{
    if (!shared.create(512, QSharedMemory::ReadWrite))  
    {  
        qDebug( "Process is running already\r\n" );
        exit(-4);
	}  
}

int main(int argc, char *argv[])
{
	// 로그 기록기 설치
	qInstallMessageHandler(myMessageOutput);

    qDebug( "Version: %s", SW_VERSION );

	/*if( argc != 3 )
	{
        qDebug( "MultiCast r\n" );
        qDebug( "Usage:\r\n" );
        qDebug( "           MultiCast 239.0.0.1 8001\r\n" );
        qDebug( "default argument is used. 239.0.0.1 8001" );
	}*/

    if( QFile( "MultiCast.conf" ).exists() == false )
    {
        qDebug() << "MultiCast.conf file is required.";
        exit(-1);
    }

    bool force = false;
    if( argc > 1 )
        force = QString(argv[1]).toLower() == "-f";

    if( force == false )
        preventDoubleRun( argc );

	App a(argc, argv);
	a.loadConfig();
	a.start();

	a.exec();
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QDir dir;
	if( dir.cd( "log" ) == false )
	{
		dir.mkdir( "log" );
		dir.cd( "log" );
	}

	QString filename = QString( "%1_%2.log" ).arg("MultiCast").arg(QDate::currentDate().toString("yyyyMMdd") );
	QFile file( QFileInfo( dir, filename ).absoluteFilePath() );
	file.open( QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append | QIODevice::Unbuffered );
	QByteArray localMsg = msg.toLocal8Bit();

	switch (type) {
	case QtDebugMsg:
		fprintf(stderr, "[%s][Info] %s\n", qPrintable(QDateTime::currentDateTime().toString("MM/dd hh:mm:ss")), localMsg.constData() );
#ifdef Q_OS_WIN		
		file.write( QString("[%1][Info] %2\r\n").arg( QTime::currentTime().toString("hh:mm:ss") ).arg(msg).toLocal8Bit() );
#else
		file.write( QString("[%1][Info] %2\n").arg( QTime::currentTime().toString("hh:mm:ss") ).arg(msg).toLocal8Bit() );
#endif
		break;
	case QtWarningMsg:
		fprintf(stderr, "[%s][Warning] %s\n", qPrintable(QDateTime::currentDateTime().toString("MM/dd hh:mm:ss")), localMsg.constData() );
#ifdef Q_OS_WIN		
		file.write( QString("[%1][Warning] %2\r\n").arg( QTime::currentTime().toString("hh:mm:ss") ).arg(msg).toLocal8Bit() );
#else
		file.write( QString("[%1][Warning] %2\n").arg( QTime::currentTime().toString("hh:mm:ss") ).arg(msg).toLocal8Bit() );
#endif
		break;
	case QtCriticalMsg:
		fprintf(stderr, "[%s][Critical] %s\n", qPrintable(QDateTime::currentDateTime().toString("MM/dd hh:mm:ss")), localMsg.constData() );
#ifdef Q_OS_WIN		
		file.write( QString("[%1][Critical] %2\r\n").arg( QTime::currentTime().toString("hh:mm:ss") ).arg(msg).toLocal8Bit() );
#else
		file.write( QString("[%1][Critical] %2\n").arg( QTime::currentTime().toString("hh:mm:ss") ).arg(msg).toLocal8Bit() );
#endif
		break;
	}

	file.close();
}
