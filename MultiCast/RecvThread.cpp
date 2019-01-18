#include "RecvThread.h"
#include <QDateTime>
#include <QFile>
#include <QNetworkInterface>
#include "Common.h"
#define BUFSIZE 10240

RecvThread::RecvThread( DataQueue<QByteArray*>& out )
	: _out ( out )
{
	if( MULTICAST_DEBUG_DUMP_WRITE )
	{
		if( QFile("multicast.dump").exists() )
		{
			if( QFile("multicast.dump").remove() == false )
				qDebug() << "delete ERROR multicast.dump";
		}
	}

}

RecvThread::~RecvThread()
{

}

void
RecvThread::start( const QString& ip, short port )
{
	_ip = ip;
	_port = port;
	qDebug() << "Setting IP: " << ip;
	qDebug() << "Setting port: " << port;

	// Initialize the multicast address
	m_groupAddress = QHostAddress("239.0.0.1");

	/**
	* Initialize the 'status' property with some data, it will
	* be shown in the UI until the first datagram arrives.
	*/
	m_status = tr("Listening for multicasted messages");

	// Create a new UDP socket and bind it against port 45454
	m_udpSocket = new QUdpSocket(this);
	//m_udpSocket->bind(_port, QUdpSocket::ShareAddress);
	
    if( m_udpSocket->bind( QHostAddress::AnyIPv4, _port) == false )
    {
        qDebug() << QString("Binding Failed");
        return;
    }

	//m_udpSocket->bind( QHostAddress(_ip), _port );
	
	m_udpSocket->setReadBufferSize( 8192 * 1000 );
	m_udpSocket->setSocketOption( QAbstractSocket::ReceiveBufferSizeSocketOption, 8192 * 1000 );

	QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();

	// loop for checking NIC all interfaces.
	for( int i=0; i<list.count(); i++ )
	{
		QList<QNetworkAddressEntry> addrEntryList = list[i].addressEntries();

		// loop for checking which NIC contains the IP
		for( int j=0; j<addrEntryList.count(); j++ )
		{
			if( addrEntryList[j].ip().toString() == _ip )
			{
				//QNetworkInterface selectedInterface = QNetworkInterface::QNetworkInterface( name );
				// Tell the UDP socket which multicast group it should join
				if( m_udpSocket->joinMulticastGroup(m_groupAddress, list[i]) == true )
				{
					qDebug() << QString("joined %1 group on %2").arg(m_groupAddress.toString()).arg(list[i].humanReadableName());
					/**
					* Create signal/slot connection to invoke processPendingDatagrams() whenever
					* a new multicast datagram is received by the socket.
					*/
					connect(m_udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
					

					return;
				}
				else
				{
					qDebug() << "NIC found. " << list[i].humanReadableName();
					qDebug() << "but error when joinMulticastGroup!";
					exit( -1002 );
				}

			}
		}
	}

	qDebug() << QString( "Error!! No NIC found for given IP: %1" ).arg(_ip);
	exit( -1001 );
}

void RecvThread::processPendingDatagrams()
{
	if( MULTICAST_DEBUG_DUMP_LOAD )
	{
		return;
	}

		//qDebug() << "read" << m_udpSocket->readBufferSize();
		//qDebug() << "recv" << m_udpSocket->socketOption( QAbstractSocket::ReceiveBufferSizeSocketOption );
		

	// Now read all available datagrams from the socket
    while (m_udpSocket->hasPendingDatagrams())
    {
		// Create a temporary buffer ...
		QByteArray datagram;

		// ... with the size of the received datagram ...
		datagram.resize(m_udpSocket->pendingDatagramSize());

		// ... and copy over the received datagram into that buffer.
		m_udpSocket->readDatagram(datagram.data(), datagram.size());

		// Update the 'status' property with the content of the received datagram
		m_status = tr("%1").arg(datagram.data());

		if( datagram.size() > 0 )
			_out.enqueue( new QByteArray( datagram ) );

		else
			qDebug() << m_status;

		if( MULTICAST_DEBUG_DUMP_WRITE )
		{
			QFile file("multicast.dump");
			if( file.open( QIODevice::WriteOnly | QIODevice::Append ) == false )
			{
				qDebug() << "dump data write ERROR!!!";
				::exit( -1000 );
			}

			QDataStream ds( &file );
			ds << datagram.length();
			file.write( datagram );
			file.close();
		}
	}
}

void RecvThread::run()
{
}
