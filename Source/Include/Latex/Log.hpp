#ifndef Header_Latex_Log
#define Header_Latex_Log


#include "mostQtHeaders.h"

#include "Latex/OutputFilter.hpp"


class LatexLogModel: public QAbstractTableModel {

	Q_OBJECT

	private:

		QList<LatexLogEntry> log;
		bool foundType[4];
		int markIDs[4];

		using Index = const QModelIndex &;
		using Orientation = Qt::Orientation;

	public:

		LatexLogModel(QObject * parent = 0);

		int columnCount(Index parent) const;
		int rowCount(Index parent) const;
		QVariant data(Index,int role) const;
		QVariant headerData(int section,Orientation,int role) const;

		int count() const;
		void clear();

		const LatexLogEntry & at(int i);

		void parseLogDocument(QTextDocument *,QString baseFileName);

		bool found(LogType) const;
		int markID(LogType) const;

		int logLineNumberToLogEntryNumber(int logLine) const; //returns the last entry with has a logline number <= logLine, or -1 if none exist
		bool existsReRunWarning() const;

		QStringList getMissingCitations() const;
		QString htmlErrorTable(const QList<int> & errors);
		QString returnString(LogType type);

};


#endif
