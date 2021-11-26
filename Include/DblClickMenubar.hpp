#ifndef Header_DblClickMenuBar
#define Header_DblClickMenuBar


#include <QMenuBar>


class DblClickMenuBar : public QMenuBar {

	Q_OBJECT

	protected:

		virtual void mouseDoubleClickEvent(QMouseEvent *);

	public:

		explicit DblClickMenuBar(QWidget * parent = nullptr);

	signals:

		void doubleClicked(); // emitted when double clicking empty part of menu bar

};


#endif
