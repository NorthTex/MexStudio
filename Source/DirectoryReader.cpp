#include "directoryreader.h"


directoryReader::directoryReader(QObject * parent) 
	: SafeThread(parent) {}


void directoryReader::readDirectory(QString path){

	QSet<QString> paths;

	auto files = QDir(path).entryInfoList();
	
	for(auto & file : files)
		paths.insert(file.fileName() + (file.isDir() ? "/" : ""));

	emit directoryLoaded(path,paths);
}

