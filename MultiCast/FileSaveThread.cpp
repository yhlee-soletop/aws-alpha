#include "FileSaveThread.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QTextCodec>
#include <QDataStream>
#include <QTcpSocket>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#endif

FileSaveThread::FileSaveThread( const QList<RULE>& rules, const FILE_DATA& df )
	: _rules( rules ), _df( df ), QThread( 0 )
{
	QSettings s( "HimawariCastSimple.ini", QSettings::IniFormat );
	s.setIniCodec( QTextCodec::codecForName("eucKR") );

	_hostAddress = QHostAddress( s.value( "SegmentByPassIP" ).toString() );
	_hostPort = s.value( "SegmentByPassPort" ).toInt();
    _clientAddress = s.value( "BindIP" ).toString();
    _clientPort = s.value( "BindPort" ).toInt();


}

FileSaveThread::~FileSaveThread()
{

}

void FileSaveThread::slotConnected()
{


}

int FileSaveThread::fileid() const
{
	return _df.fileid;
}

void FileSaveThread::run()
{
	if( _df.filesize == _df.receivedbyte )
	{
		qDebug() << _df.filepathname << "[" <<  _df.receivedbyte - _df.filesize << "]";
	}

	else if( _df.filesize > _df.receivedbyte )
	{
#ifdef Q_OS_WIN
		Beep( 1300, 200 );
#else
#endif
		qDebug() << _df.filepathname << "[" <<  _df.receivedbyte - _df.filesize << "]";
		return;
	}
	else
	{
#ifdef Q_OS_WIN
		Beep( 1300, 200 );
#else
#endif
		qDebug() << _df.filepathname << "[" <<  _df.receivedbyte - _df.filesize << "]";
		return;
	}

	const QByteArray& data = _df.data;
	const QString& filename = _df.filename;
	QStringList list = _df.filepathname.split("/" );

	if( list.count() >= 2 )
	{
		for( int i=0; i<_rules.count(); i++ )
		{
			if( list[1] == _rules[i].rule )
			{
				const RULE& rule = _rules[i];
                save( rule, filename, data );
                send( filename, data );
			}
		}
	}
}

void FileSaveThread::save( const RULE& rule, const QString& filename, const QByteArray& data )
{
    QString filepath = convertDir(rule, filename);

	QFileInfo fi( filepath );
	QDir dir = fi.absoluteDir();
	if( dir.exists() == false )
        if( dir.mkpath( fi.absolutePath() ) == false )
            qDebug() << "Can not create direcotry: " << fi.absolutePath();

    QFile file( filepath );

	if( file.open( QIODevice::WriteOnly ) == false )
		qDebug() << "Write Failed: " << filepath;

	if( file.write( data ) == 0 )
        qDebug() << "Write Failed. Not enought space of storage?? " << filepath;

	file.close();
}

#include <sys/types.h>
#include <sys/socket.h>

void FileSaveThread::send( const QString& filename, const QByteArray& data )
{
    if( _df.receivedbyte == _df.filesize && _df.filepathname.contains( "HRIT") ){
        QTcpSocket socket;
        socket.bind( _clientAddress, _clientPort, QAbstractSocket::ReuseAddressHint );
        socket.connectToHost( _hostAddress, _hostPort );
        if( socket.waitForConnected(3000) == false ){
            qWarning() << "Failed to connected" << filename;
            socket.close();
            return;
        }

        int ns = socket.socketDescriptor();
        unsigned int sendbuffersize = 1024*1024*1024;
        setsockopt( ns, SOL_SOCKET, SO_SNDBUF, (char*) &sendbuffersize, sizeof( sendbuffersize) );

        int size = 64;
        char ch = 0x00;
        QByteArray d( size, ch );
        memcpy( d.data(), qPrintable( filename ), filename.length() );
        socket.write( d );
        qint64 sendbytes = socket.write( data );
        socket.flush();
        socket.waitForBytesWritten();
        socket.close();
        qDebug() << "Send done: " << filename << sendbytes;
    }
}

QString FileSaveThread::convertDir( const RULE& rule, const QString& filename )
{
    QString path = QFileInfo( rule.dir, filename ).absoluteFilePath();

    if( (rule.rule == "HRIT_in" && filename.startsWith("IMG_")) || (rule.rule == "LRIT" && filename.startsWith("IMG_")) )
    {
        QString yyyy = filename.mid( 12, 4 );
        QString MM = filename.mid( 16, 2 );
        QString dd = filename.mid( 18, 2 );
        QString hh = filename.mid( 20, 2 );
        QString mm = filename.mid( 22, 2 );

        path.replace( "$yyyy", yyyy );
        path.replace( "$MM", MM );
        path.replace( "$dd", dd );
        path.replace( "$hh", hh );
        path.replace( "$mm", mm );

        //qDebug() << "convertDir" << rule.rule << filename << path;
    }

    else if( rule.rule == "SATAID_Obs" && filename.startsWith("essential_") )
    {
        QString yyyy = filename.mid( 12, 4 );
        QString MM = filename.mid( 16, 2 );
        QString dd = filename.mid( 18, 2 );
        QString hh = filename.mid( 20, 2 );
        QString mm = filename.mid( 22, 2 );

        path.replace( "$yyyy", yyyy );
        path.replace( "$MM", MM );
        path.replace( "$dd", dd );
        path.replace( "$hh", hh );
        path.replace( "$mm", mm );
    }
	else if( rule.rule == "SATAID_Image" && filename.startsWith("GS") )
	{
		//GS170504.Z06.bz2
		QString yyyy = "20" + filename.mid( 2, 2 );
        QString MM = filename.mid( 4, 2 );
        QString dd = filename.mid( 6, 2 );
        QString hh = filename.mid( 10, 2 );
        QString mm = "00";

        path.replace( "$yyyy", yyyy );
        path.replace( "$MM", MM );
        path.replace( "$dd", dd );
        path.replace( "$hh", hh );
        path.replace( "$mm", mm );
	}
	else
		qDebug() << "Can't classify: " << rule.rule << filename;

    return path;
}

/*
void FileSaveThread::send( const QString& filename, const QByteArray& data )
{

    int size = 64;
    char ch = 0x00;
    QByteArray d( size, ch );
    memcpy( d.data(), qPrintable( filename ), filename.length() );

    sendHeaderData( filename, data.size() );
    sendHRITData( filename, data );
    sendEOFData( filename );
}*/

void FileSaveThread::sendHeaderData( const QString& filename, int filesize )
{
    /*
	QByteArray header;

	QDataStream ds( &header, QIODevice::WriteOnly );
	ds.setByteOrder( QDataStream::BigEndian );
    ds.writeRawData( "GS30", 4);
    ds << (quint8) 1;

    int size = 64;
    char ch = 0x00;
    QByteArray d( size, ch );
    memcpy( d.data(), qPrintable( filename ), filename.length() );
	
    QUdpSocket socket;
    socket.writeDatagram( header+d, _hostAddress, _hostPort );
    */
}

void FileSaveThread::sendHRITData( const QString& filename, const QByteArray& data )
{
    /*
    QByteArray h;
    h.resize( 5 );
    h[0] = 'G';
    h[1] = 'S';
    h[2] = '3';
    h[3] = '0';
    h[4] = 2;
    int size = 64;
    char ch = 0x00;
    QByteArray d( size, ch );
    memcpy( d.data(), qPrintable( filename ), filename.length() );

	QUdpSocket socket;
	int filepos = 0;
	while( filepos < data.length() )
	{
		int pos = data.length() - filepos;
        if( pos > 512 )
            pos = 512;
        QByteArray datagram = h+d+QByteArray(filepos+data.data(), pos);
        qint64 sendLen = socket.writeDatagram( datagram , datagram.length() , _hostAddress, _hostPort );
        if( datagram.length() != sendLen )
            qWarning() << "Data Loss";

        filepos += pos;
       // msleep(5);
	}
    */
}

void FileSaveThread::sendEOFData( const QString& filename )
{
    /*
	QByteArray tail;

	QDataStream ds( &tail, QIODevice::WriteOnly );
	ds.setByteOrder( QDataStream::BigEndian );
    ds.writeRawData( "GS30", 4);
    ds << (quint8) 3;

    int size = 64;
    char ch = 0x00;
    QByteArray d( size, ch );
    memcpy( d.data(), qPrintable( filename ), filename.length() );

	QUdpSocket socket;
    socket.writeDatagram( tail+d, _hostAddress, _hostPort );
    */
}
