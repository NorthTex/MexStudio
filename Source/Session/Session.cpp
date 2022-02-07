#include "Session.hpp"

#include "configmanager.h"
#include "smallUsefulFunctions.h"

QString Session::m_fileExtension = "txss";

Session::Session(const Session & session){

	m_files.append(session.m_files);

	m_pdfEmbedded = session.m_pdfEmbedded;
	m_currentFile = session.m_currentFile;
	m_masterFile = session.m_masterFile;
	m_bookmarks = session.m_bookmarks;
	m_pdfFile = session.m_pdfFile;
}


bool Session::load(const QString & file){

	if(!QFileInfo(file).isReadable())
		return false;
	
	QSettings settings(file,QSettings::IniFormat);
	
	if(!settings.childGroups().contains("Session"))
		return false;
	
	m_files.clear();
	settings.beginGroup("Session");
	
	auto groups = settings.childGroups();
	auto directory = QFileInfo(file).dir();
	
	for(int i = 0;i < 1000;i++){

		if(!groups.contains(QString("File%1").arg(i)))
			break;

		settings.beginGroup(QString("File%1").arg(i));

		const auto filename = settings.value("FileName").toString();

		if(filename.isEmpty())
			continue;

		FileInSession file;
		file.fileName = QDir::cleanPath(directory.filePath(filename));
		file.editorGroup = settings.value("EditorGroup",0).toInt();
		file.cursorLine = settings.value("Line",0).toInt();
		file.cursorCol = settings.value("Col",0).toInt();
		file.firstLine = settings.value("FirstLine",0).toInt();
		file.foldedLines = strToIntList(settings.value("FoldedLines").toString());

		m_files.append(file);
		settings.endGroup();
	}

	m_masterFile = QDir::cleanPath(directory.filePath(settings.value("MasterFile").toString()));
	m_currentFile = QDir::cleanPath(directory.filePath(settings.value("CurrentFile").toString()));
	
	for(const auto & v : settings.value("Bookmarks").value<QList<QVariant>>()){
		auto bookmark = Bookmark::fromStringList(v.toStringList());
		bookmark.path = QDir::cleanPath(directory.filePath(bookmark.path));
		m_bookmarks << bookmark;
	}

	settings.endGroup();
	settings.beginGroup("InternalPDFViewer");
	
	QString pdfFileName(settings.value("File").toString());
	
	m_pdfFile = (pdfFileName.trimmed().isEmpty()) 
		? "" 
		: QDir::cleanPath(directory.filePath(pdfFileName));
	
	m_pdfEmbedded = settings.value("Embedded").toBool();
	
	settings.endGroup();
	
	return true;
}


bool Session::save(const QString & file,bool hasRelativePath) const {
	
	if(!isFileRealWritable(file))
		return false;
	
	QSettings settings(file,QSettings::IniFormat);
	settings.clear();
	settings.beginGroup("Session");

	// increment if format changes are applied later on. 
	// This might be used for version-dependent loading.

	settings.setValue("FileVersion", 1); 
	
	QDir dir = QFileInfo(file).dir();
	
	for(int i = 0; i < m_files.count(); i++){

		const auto & file = m_files[i];

		settings.beginGroup(QString("File%1").arg(i));
		settings.setValue("FileName",fmtPath(dir,file.fileName,hasRelativePath));
		settings.setValue("EditorGroup",file.editorGroup);
		settings.setValue("Line",file.cursorLine);
		settings.setValue("Col",file.cursorCol);
		settings.setValue("FirstLine",file.firstLine);

		// saving as string is not very elegant, but at least human-readable 
		// (QList<int> would result in a byte stream - after adding it as a metatype)

		settings.setValue("FoldedLines",intListToStr(file.foldedLines)); 
		settings.endGroup();
	}

	const auto 
		current = fmtPath(dir,m_currentFile,hasRelativePath),
		master = fmtPath(dir,m_masterFile,hasRelativePath);

	settings.setValue("CurrentFile",current);
	settings.setValue("MasterFile",master);
	
	QList<QVariant> bookmarkList;

	for(auto bookmark : m_bookmarks){

		if(hasRelativePath)
			bookmark.path = fmtPath(dir,bookmark.path,hasRelativePath);

		bookmarkList << bookmark.toStringList();
	}

	settings.setValue("Bookmarks",bookmarkList);
	settings.endGroup();

	settings.beginGroup("InternalPDFViewer");
	settings.setValue("File",fmtPath(dir,m_pdfFile,hasRelativePath));
	settings.setValue("Embedded",m_pdfEmbedded);
	settings.endGroup();

	return true;
}


void Session::addFile(FileInSession file){
	m_files.append(file);
}


QString Session::fmtPath(
	const QDir & directory,
	const QString & file,
	bool hasRelativePath
){
	return (hasRelativePath)
		? directory.relativeFilePath(file)
		: file;
}

