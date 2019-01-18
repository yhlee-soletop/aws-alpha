#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

//
// Only support pointer type
//

template <class Type>
class DataQueue
{
public:
	DataQueue();
	DataQueue( const QString& strMsgName );
	~DataQueue();

public:
	const QString&		queueName() const;
	void				enqueue( Type pItem );
	Type				dequeue();
	uint				count();
	void				clear();
	void				setMaxCount( unsigned int uCount );
	void				setWaitTime( int nMilesec );
	bool				isFull();
	bool				wait( unsigned long time );
	void				waitUntilCanEnqueue( bool* bRunFlag, unsigned long time = 100 );
	void				waitUntilCanEnqueue( bool* bRunFlag1, bool* bRunFlag2, unsigned long time = 100 );
	void				wake();

private:
	QQueue<Type>		_queue;
	QMutex				_mutex;
	QWaitCondition		_waitCon;
	uint				_uCount;
	uint				_nWaitMileSec;
};

template <class Type>
DataQueue<Type>::DataQueue()
{
	_uCount			= 100000;
	_nWaitMileSec	= 5;
}

template <class Type>
DataQueue<Type>::DataQueue( const QString& strMsgName )
{
	_uCount			= 100000;
	_nWaitMileSec	= 5;
}

template <class Type>
DataQueue<Type>::~DataQueue()
{
	QMutexLocker locker( &_mutex );
	
	while( _queue.isEmpty() == false )
	{
		delete _queue.takeFirst();
	}
}

template <class Type> 
const QString&
DataQueue<Type>::queueName() const
{
        return QString(); //_strMsgName;
}

template <class Type> 
void 
DataQueue<Type>::enqueue( Type pItem )
{
	QMutexLocker locker( &_mutex );

	if(  _queue.count() > _uCount )
	{
		_waitCon.wait( &_mutex );
	}

	_queue.enqueue( pItem );
	_waitCon.wakeAll();
}

template <class Type> 
Type
DataQueue<Type>::dequeue()
{
	QMutexLocker locker( &_mutex );
	Type		 data;

	if( _queue.isEmpty() )
	{
		if( !_waitCon.wait( &_mutex, _nWaitMileSec ) )
		{
			_waitCon.wakeAll();
			return NULL;
		}
	}

	data = _queue.dequeue();

	_waitCon.wakeAll();

	return data;
}

template <class Type>
uint
DataQueue<Type>::count()
{
	QMutexLocker	locker( &_mutex );

	return _queue.count();
}

template <class Type>
void
DataQueue<Type>::clear()
{
	QMutexLocker	locker( &_mutex );

	while( _queue.isEmpty() == false )
	{
		delete _queue.takeFirst();
	}
}

template <class Type> 
void
DataQueue<Type>::setMaxCount( unsigned int nMaxCount )
{
	_uCount = nMaxCount;
}

template <class Type> 
void
DataQueue<Type>::setWaitTime( int nMilesec )
{
	_nWaitMileSec = nMilesec;
}

template <class Type> 
bool
DataQueue<Type>::isFull()
{
	QMutexLocker	locker( &_mutex );

	if( ( _uCount + 1 ) == _queue.count() )
	{
		return true;
	}

	return false;
}

template <class Type> 
bool			
DataQueue<Type>::wait( unsigned long time )
{
	QMutexLocker locker( &_mutex );

	return _waitCon.wait( &_mutex, time );
}

template <class Type> 
void				
DataQueue<Type>::waitUntilCanEnqueue( bool* bRunFlag, unsigned long time )
{
	while ( isFull() == true )
	{
		if ( *bRunFlag == false )
			return;

		if ( wait( time ) == true )
		{
			return;
		}
	}
}

template <class Type> 
void				
DataQueue<Type>::waitUntilCanEnqueue( bool* bRunFlag1, bool* bRunFlag2, unsigned long time )
{
	while ( isFull() == true )
	{
		if ( *bRunFlag1 == false || *bRunFlag2 == false )
			return;

		if ( wait( time ) == true )
		{
			return;
		}
	}
}

template <class Type> 
void				
DataQueue<Type>::wake()
{
	_waitCon.wakeAll();
}

#endif // MSGQUEUE_H
