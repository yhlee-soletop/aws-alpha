#include "App.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QDataStream>
#include "FileSaveThread.h"

#define BUFSIZE 102400
#define DATAPACKETSIZE 1427
#define PACKETHEADERSIZE 16
#define FILENAMEOFFSET ( 16*5 + 4 )
#define FILEPATHOFFSET ( 16*12 - 4 )
#define FILESIZEOFFSET ( 16*11 + 4 )
#define IMAGEDATASIZE ( DATAPACKETSIZE - PACKETHEADERSIZE )
#define COUNTEROFFSET 8

App::App(int argc, char **argv)
	: QCoreApplication(argc, argv),
	_recv( _buffer )
{
	// default parameter
	_ip = "239.0.0.1";
	_port = 8001;
}

App::~App()
{

}

void App::start()
{
	_recv.start( _ip, _port );
	QTimer::singleShot( 20, this, SLOT( slotTimeout() ) );

	if( MULTICAST_DEBUG_DUMP_LOAD )
	{
		QFile file("multicast.dump");
		if( file.open( QIODevice::ReadOnly ) )
		{
			QDataStream ds( &file );
			while( file.atEnd() == false )
			{
				int filesize = 0;
				ds >> filesize;

				int aa = file.pos();
				QByteArray data = file.read( filesize );

				if( data.length() == filesize )
				{
					proc( data );
				}
				else
					qDebug() << "not enough data ERROR!!!";

			}
		}
		else
		{
			qDebug() << "file read ERROR!!!";
			::exit( -1001 );
		}
	}
}

void App::slotTimeout()
{
	while( true )
	{
		QByteArray* data = _buffer.dequeue();
		if( data )
		{
			proc( QByteArray( *data ) );
			delete data;
		}
		else
			break;

	}
	QTimer::singleShot( 20, this, SLOT( slotTimeout() ) );
}

void
	App::proc( QByteArray data )
{
	int packetLen = data.length();
	char* buf = data.data();

	if( buf[0] == 0x00 && buf[1] == 0x03  ) // 전송 시작 패킷
		onStartData( data );

	else if( buf[0] == 0x00 && buf[1] == (char)0xff ) // 전송 종료 패킷
		onFinishedData( data );

	else if( buf[0] == 0x00 && buf[1] == 0x01 )  // 데이터 패킷
		onReceviedData( data );

	else
		qDebug() << "unknown packet" << packetLen;
}

void App::save( FILE_DATA& df )
{
	if( df.isSaving == true )
		return;
			
	df.isSaving = true;

	FileSaveThread* pSaveThread = new FileSaveThread( _rules, df );
	connect( pSaveThread, SIGNAL( finished() ), SLOT( slotSaveFinished() ), Qt::QueuedConnection );
	pSaveThread->start();
}

void App::slotSaveFinished()
{
	FileSaveThread* pObj = (FileSaveThread*) sender();
	_dataBuffer.remove( pObj->fileid() ); // 저장 버퍼 삭제
	delete pObj;
}

int App::getCount( const char* buf )
{
	int counter[1] = { 0 };
	char *pCounter = (char*) counter;
	pCounter[0] = buf[0+COUNTEROFFSET];
	pCounter[1] = buf[1+COUNTEROFFSET];
	pCounter[2] = buf[2+COUNTEROFFSET];
	pCounter[3] = buf[3+COUNTEROFFSET];

	return counter[0];
}

int App::getFileSize( const char* buf, int offset )
{
	// 파일 사이즈 
	int filesize[1];
	filesize[0] = 0;

	char* p = (char*)filesize;

	p[0] = buf[ offset ];
	p[1] = buf[ offset + 1 ];
	p[2] = buf[ offset + 2 ];
	p[3] = buf[ offset + 3 ];

	return filesize[0]; 
}

int App::getFileID( const char* buf )
{
	// ID
	int fileid[1];
	fileid[0] = 0;

	char* p2 = (char*)fileid;
	p2[0] = buf[ 4 ];
	p2[1] = buf[ 4 + 1 ];
	p2[2] = buf[ 4 + 2 ];
	p2[3] = buf[ 4 + 3 ];

	return fileid[0];
}

bool App::loadConfig()
{
	QFile file( "MultiCast.conf");
	if( file.open( QIODevice::ReadOnly ) == false )
		return false;

	QList<QByteArray> ipline = file.readLine().trimmed().split('=');
	QList<QByteArray> portline = file.readLine().trimmed().split('=');

	if( ipline.count() != 2 )
	{
		qDebug() << "MultiCast.conf error, first line must be ip=???.???.???.???";
		::exit(-100);
	}

	if( ipline.count() != 2 )
	{
		qDebug() << "MultiCast.conf error, second line must be port=????";
		::exit(-101);
	}

	_ip = ipline[1];
	_port = portline[1].toShort();

	while( file.atEnd() == false )
	{
		QList<QByteArray> items = file.readLine().trimmed().split(',');
		if( items.count() < 2 )
			continue;

		RULE rule;
		rule.rule = items[0];
		rule.dir = items[1];
		rule.createDir = items[2].toLower() == QString("yes") ? true : false;

		rule.dir.replace( "\\", "/" );
		if( !rule.dir.endsWith( "/" ) )
			rule.dir += "/";

		QDir dir( rule.dir );
		if( dir.exists() == false )
			dir.mkdir( rule.dir );

		_rules += rule;
	}

	file.close();
	return true;
}

void App::onStartData( const QByteArray& data )
{
	int packetLen = data.length();
	const char* buf = data.data();

	QString filename;
	QString filepathname;
	int filesize = 0;
	int fileid = 0;

	// MANAM 패킷이라면
	if( packetLen == 236 )
	{
		filename = QString( buf + FILENAMEOFFSET );
		filepathname = QString( buf + FILEPATHOFFSET - 2 );
		filesize = getFileSize( buf, FILESIZEOFFSET - 2 );
		fileid = getFileID( buf );
	}

	else if( packetLen == 215 ) // NWP
	{
		filename = QString( buf + 0x54 );
		filepathname = QString( buf + 0xAC );
		filesize = getFileSize( buf, 0xA4 );
		fileid = getFileID( buf );
	}

	else // 이미지 패킷 처리. HRIT( len: 242 ), LRIT( len: 239 ) 이지만 offset 위치가 같다.
	{
		filename = QString( buf + FILENAMEOFFSET );
		filepathname = QString( buf + FILEPATHOFFSET );
		filesize = getFileSize( buf, FILESIZEOFFSET );
		fileid = getFileID( buf );
	}

	filepathname.replace("\\","/");

	if( _dataBuffer.contains( fileid ) == false )
	{
		FILE_DATA filedata;
		filedata.fileid = fileid;
		filedata.filename = filename;
		filedata.filepathname = filepathname;
		filedata.filesize = filesize;
		filedata.data.resize( filesize );
		_dataBuffer.insert( fileid, filedata );
	}
}

void App::onReceviedData( const QByteArray& data )
{
	int packetLen = data.length();
	const char* buf = data.constData();

	int fileID = getFileID( buf );

	if( _dataBuffer.contains( fileID ) )
	{
		FILE_DATA& df = _dataBuffer[ fileID ];
		int ncounter = getCount( buf );

		//qDebug() << fileID << ncounter;
		// 카운터의 구간에 복사한다. - 패킷 도착 순서가 바뀔 수 있으므로
		if( ncounter * IMAGEDATASIZE <= ( df.data.length() - IMAGEDATASIZE ) )
		{
			char* pDest = df.data.data() + ncounter * IMAGEDATASIZE;
			const char* pSrc = buf + PACKETHEADERSIZE;
			memcpy( pDest, pSrc, IMAGEDATASIZE );
			df.receivedbyte += IMAGEDATASIZE;
		}
	}
}

void App::onFinishedData( const QByteArray& data )
{
	int packetLen = data.length();
	const char* buf = data.data();

	int fileID = getFileID( buf );

	if( _dataBuffer.contains( fileID ) )
	{ 
		FILE_DATA& df = _dataBuffer[ fileID ];
		save( df );
	}
}
