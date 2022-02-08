#ifndef Header_Latex_Log_Widget
#define Header_Latex_Log_Widget


#include "LogEditor.hpp"
#include <QSortFilterProxyModel>
#include "mostQtHeaders.h"

#include "Latex/Log.hpp"


class LatexLogWidget : public QWidget {

	Q_OBJECT

	public:

		explicit LatexLogWidget(QWidget * parent = 0);


		LatexLogModel * getLogModel(){
			return logModel;
		}

		bool childHasFocus() const {
			return log -> hasFocus() || errorTable -> hasFocus();
		}

		bool loadLogFile(const QString & logname,const QString & compiledFileName,QTextCodec * fallback);
		bool logEntryNumberValid(int logEntryNumber);
		bool logPresent();

		void selectLogEntry(int logEntryNumber);
		void resetLog();
		void copy();

		QList<QAction *> displayActions();

	signals:

		void logEntryActivated(int);
		void logLoaded();
		void logResetted();

	private slots:

		void clickedOnLogModelIndex(const QModelIndex &);
		void setWidgetVisibleFromAction(bool visible);
		void gotoLogEntry(int logEntryNumber);
		void filterChanged(bool);
		void gotoLogLine(int logLine);
		void setInfo(const QString &message);

		void copyAllMessagesWithLineNumbers();
		void copyMessage();
		void copyAllMessages();

	private:

		QSortFilterProxyModel * proxyModel;
		LatexLogModel * logModel;

		QTableView * errorTable;
		LogEditor * log;
		QLabel * infoLabel;

		QAction
			* displayTableAction,
			* displayLogAction,
			* filterErrorAction,
			* filterWarningAction,
			* filterBadBoxAction;

		bool logpresent;
};


#endif
