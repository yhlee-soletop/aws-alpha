// S300D DVB-S2 RECEIVER ��Ŷ ���� ���α׷�
// 2015-12-01 ������
//
// ���űⰡ MULTICAST �ϴ� ��Ŷ�� �޾Ƽ� ���� ���� ������ ���Ϸ� ����
// ���������� ������ ���� ���� ������ �з��Ͽ� ����
// UDP Ư���� ��Ŷ ������ �ٲ�� ���� ���
// UDP Ư���� ��Ŷ�� �ٷ� �������� ������ packet loss�� �߻�

#include <QtCore/QCoreApplication>
#include "App.h"
//#include <QMessageBox>
#include <QSharedMemory>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QDataStream>

//#define SW_VERSION "v1.00.00"
//#define SW_VERSION "v1.01.00" // ���� �����ʹ� ��� ����
//#define SW_VERSION "v1.02.00" // Patch for data loss( socket size increased )
//#define SW_VERSION "v1.03.00" // ���� ���ε� IP �߰�
//#define SW_VERSION "v1.03.01" // ���� ��� ��� �߰�
//#define SW_VERSION "v1.03.05" // NIC ���ε� ���� ��� ��� �߰�
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
	// �α� ��ϱ� ��ġ
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
