#ifndef Header_ChangeAwareTabBar
#define Header_ChangeAwareTabBar


#include <QTabBar>


class ChangeAwareTabBar : public QTabBar {

	Q_OBJECT

	public:
	signals:

		void middleMouseButtonPressed(int tabNr);
		void currentTabAboutToChange(int from,int to);
		void tabLeftClicked();

	protected:

		virtual void mousePressEvent(QMouseEvent *);
};


#endif
