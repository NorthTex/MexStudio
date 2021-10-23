#ifndef Header_TitlePanel
#define Header_TitlePanel

#include "mostQtHeaders.h"

class TitledPanelPage;

Q_DECLARE_METATYPE(TitledPanelPage *)


class TitledPanelPage : public QObject {

	Q_OBJECT

	friend class TitledPanel;

	public:

		TitledPanelPage(QWidget * widget,const QString & id,const QString & title,const QIcon & icon = QIcon());
		~TitledPanelPage();

		void addToolbarAction(QAction * act);
		void addToolbarActions(const QList<QAction *> & actions);

		inline QString id() const;
		QWidget * widget();
		bool visible() const;

		static TitledPanelPage * fromId(const QString & id);
		static void updatePageTitle(const QString & id,const QString & newTitle);

	signals:

		void titleChanged(QString);
		void iconChanged(QIcon);

	public slots:

		void setTitle(const QString & title);
		void setIcon(const QIcon & icon);

	private:

		static QHash<QString, TitledPanelPage *> allPages;

		QString m_id;
		QString m_title;
		QIcon m_icon;
		QWidget * m_widget;

		QAction * m_visibleAction;
		QAction * m_selectAction;
		QList<QAction *> * m_toolbarActions;

};



class TitledPanel : public QFrame {

	Q_OBJECT

	public:

		explicit TitledPanel(QWidget * parent = 0);

		enum PageSelectorStyle { ComboSelector, TabSelector };

		void appendPage(TitledPanelPage * page,bool guiUpdate = true);
		void removePage(TitledPanelPage * page,bool guiUpdate = true);
		int pageCount() const;
		TitledPanelPage * pageFromId(const QString & id);

		void setHiddenPageIds(const QStringList & hidden);
		QStringList hiddenPageIds() const;

		TitledPanelPage * currentPage() const;
		QString currentPageId() const;
		void showPage(const QString & id);
		void setCurrentPage(const QString & id);
		QAction * toggleViewAction() const;

		void setSelectorStyle(PageSelectorStyle style);

	signals:

		void widgetContextMenuRequested(QWidget * widget,const QPoint & globalPosition);
		void pageChanged(const QString & id);

	public slots:

		virtual void setVisible(bool visible);
		void updateTopbar();

	private slots:

		void viewToggled(bool visible) { setVisible(visible); }

		void updatePageSelector(TitledPanelPage * page = 0);
		void onPageTitleChange();
		void onPageIconChange();

		void setActivePageFromAction();
		void setActivePageFromComboBox(int index);
		void setActivePageFromTabBar(int index);
		void togglePageVisibleFromAction(bool on);
		//void updateToolbarForResizing(CollapseState clState);
		void customContextMenuRequested(const QPoint &localPosition);

	protected:

		QAction * mToggleViewAction;
		QAction * closeAction;

	private:

		QStringList mHiddenPageIds;
		QActionGroup * pageSelectActions;    // only for visible widgets
		QList<QWidget *> widgets;

		// visual elements
		PageSelectorStyle selectorStyle;
		QVBoxLayout * vLayout; // containing title bar and contents
		QToolBar * topbar;
		QLabel * lbTopbarLabel;
		QComboBox * cbTopbarSelector;
		QTabBar * tbTopbarSelector;

		QStackedWidget * stack;

		QList<TitledPanelPage *> pages;

};


#endif
