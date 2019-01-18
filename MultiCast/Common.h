#ifndef COMMON_H
#define COMMON_H

struct RULE
{
	QString	rule;
	QString	dir;
	bool createDir;
};

struct FILE_DATA
{
	bool isSaving;
	int filesize;
	int fileid;
	int receivedbyte;
	QByteArray data;
	QString filename;
	QString filepathname;

	FILE_DATA()
	{
		isSaving = false;
		filesize = 0;
		fileid = 0;
		receivedbyte = 0;
	}
};

#define MULTICAST_DEBUG_DUMP_WRITE false
#define MULTICAST_DEBUG_DUMP_LOAD false

#endif