#ifndef Header_FileSelector
#define Header_FileSelector


#include "mostQtHeaders.h"


class FileSelector : public QWidget {

	Q_OBJECT

	public:

		explicit FileSelector(QWidget * parent = 0,bool multiselect = false);

		void init(const QStringList & files,int current);
		void setCentered();

	signals:

		void fileChoosen(const QString & name,int duplicate,int lineNr,int column);

	private slots:

		void filterChanged(const QString & newFilter);

	protected:

		virtual bool eventFilter(QObject *,QEvent *);
		virtual void showEvent(QShowEvent *);

	protected slots:

		void emitChoosen();

	private:

		QListWidget * list;
		QLineEdit * filter;

		QStringList rawFiles;

		QList<QPair<QString, int> > currentFiles();

		bool multiselect;

};


#endif
