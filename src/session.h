#ifndef Header_Session
#define Header_Session


#include "Bookmarks.hpp"
#include "mostQtHeaders.h"


struct FileInSession {

	QList<int> foldedLines;
	QString fileName;

	int editorGroup;
	int cursorLine;
	int cursorCol;
	int firstLine;

};


Q_DECLARE_METATYPE(FileInSession)


class ConfigManager;


class Session {

	private:

		using Path = const QString &;

	public:

		Session()
			: m_pdfEmbedded(false) {}

		Session(const Session &);

		Session & operator = (const Session &) = default;	// Avoid GCC9 -Wdeprecated-copy warning

		bool load(Path);
		bool save(Path,bool relPaths = true) const;

		const QList<FileInSession> files() const { return m_files; }
		void addFile(FileInSession f);

		void setMasterFile(Path file){
			m_masterFile = file;
		}

		QString masterFile() const {
			return m_masterFile;
		}

		void setCurrentFile(Path file){
			m_currentFile = file;
		}

		QString currentFile() const {
			return m_currentFile;
		}

		void setBookmarks(const QList<Bookmark> &bookmarkList){
			m_bookmarks = bookmarkList;
		}

		QList<Bookmark> bookmarks() const {
			return m_bookmarks;
		}

		void setPDFFile(const QString & pdf){
			m_pdfFile = pdf;
		}

		QString PDFFile() const {
			return m_pdfFile;
		}

		void setPDFEmbedded(bool b){
			m_pdfEmbedded = b;
		}

		bool PDFEmbedded() const {
			return m_pdfEmbedded;
		}

		static QString fileExtension(){
			return m_fileExtension;
		}

		static QString fmtPath(const QDir & dir,Path,bool relPath = true);

	private:

		static QString m_fileExtension;

		QList<FileInSession> m_files;
		QString m_masterFile;
		QString m_currentFile;

		QList<Bookmark> m_bookmarks;

		QString m_pdfFile;
		bool m_pdfEmbedded;

};


#endif
