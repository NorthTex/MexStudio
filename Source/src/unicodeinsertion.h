#ifndef Header_UnicodeInsertion
#define Header_UnicodeInsertion

#include "mostQtHeaders.h"


class QLineEditWithMetaText: public QLineEdit {

	Q_OBJECT

	public:


		QLineEditWithMetaText(QWidget * parent);
		void setMetaText(const QString & text);

	protected:

		QString metaText;
		virtual void paintEvent ( QPaintEvent * event );

};


class UnicodeInsertion : public QWidget {

	Q_OBJECT

	public:

		UnicodeInsertion(QWidget * parent,uint defCharCode = 0);

	private:

        QLineEditWithMetaText * edit;
        QTableWidget *  table;
        uint defaultCharCode;

        void setTableText(int row,int column,const QString & text);

    protected:

        virtual void keyPressEvent(QKeyEvent * event);

    private slots:

        void editChanged(const QString & text);
        void tableCellClicked(int row,int column);
        void tableCellDoubleClicked(int row,int column);

    public:
    signals:

        void insertCharacter(const QString & character);

};

#endif
