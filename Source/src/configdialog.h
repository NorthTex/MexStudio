/***************************************************************************
 *   copyright       : (C) 2003-2007 by Pascal Brachet                     *
 *   http://www.xm1math.net/texmaker/                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef Header_Config_Dialog
#define Header_Config_Dialog


#include "mostQtHeaders.h"
#include "ui_configdialog.h"
#include "qformat.h"
#include "buildmanager.h"



//TODO: perhaps move each class in its own file?
class ShortcutComboBox: public QComboBox {

	Q_OBJECT

	public:

		ShortcutComboBox(QWidget * parent = nullptr);

	protected:

		virtual void focusInEvent(QFocusEvent *);
		virtual void keyPressEvent(QKeyEvent *);

};


class ShortcutDelegate : public QItemDelegate {

	Q_OBJECT

	public:

		ShortcutDelegate(QObject * parent = nullptr);

		QWidget *createEditor(QWidget * parent,
							  const QStyleOptionViewItem &,
							  const QModelIndex &) const;

		void setEditorData(QWidget * editor,const QModelIndex &) const;
		void setModelData(QWidget * editor,QAbstractItemModel *,
						  const QModelIndex &) const;

		void updateEditorGeometry(QWidget * editor,
								  const QStyleOptionViewItem &,
								  const QModelIndex &) const;

		void drawDisplay(QPainter *,const QStyleOptionViewItem &,const QRect &,const QString & text) const;

		bool isBasicEditorKey(const QModelIndex &) const;

		QTreeWidget * treeWidget;  //tree widget to remove duplicates from, not necessary

		static const QString deleteRowButton;
		static const QString addRowButton;

	public slots:

		void treeWidgetItemClicked(QTreeWidgetItem *,int column);

};


class ComboBoxDelegate : public QItemDelegate {

    public:

        ComboBoxDelegate(QObject * parent = nullptr);

        QWidget *createEditor(QWidget * parent,
                              const QStyleOptionViewItem &,
                              const QModelIndex &) const;

        void setEditorData(QWidget * editor,const QModelIndex &) const;
        void setModelData(QWidget * editor,
                          QAbstractItemModel *,
                          const QModelIndex &) const;

        void updateEditorGeometry(QWidget * editor,
                                  const QStyleOptionViewItem &,
                                  const QModelIndex &) const;

        QStringList defaultItems;
        int activeColumn;

};


class QFormatConfig;


class ConfigDialog : public QDialog {

    Q_OBJECT

    public:

        ConfigDialog(QWidget * parent = nullptr);
        ~ConfigDialog();

        Ui::ConfigDialog ui;

        QMap<QString,QVariant> *replacedIconsOnMenus;
        QMap<QString,QFormat> editorFormats;

        QRadioButton * checkboxInternalPDFViewer;
        QFormatConfig * fmConfig;
        QObject *menuParent;

        QList<QStringList> customizableToolbars;
        QList<QMenu *> allMenus;
        QList<QMenu *> standardToolbarMenus;

        void setBuildManger(BuildManager * buildManager){
            mBuildManager = buildManager;
        }

        bool riddled;

    public slots:

        void changePage(QListWidgetItem * current,QListWidgetItem * previous);

    private slots:

        void comboBoxWithPathEdited(const QString &newText);
        void comboBoxWithPathHighlighted(const QString &newText);
        void browseThesaurus();
        void browseGrammarLTPath();
        void browseGrammarLTJavaPath();
        void browseGrammarWordListsDir();
        void resetLTURL();
        void resetLTArgs();
        void browseDictDir();
        void updateDefaultDictSelection(const QString &dictPaths, const QString &newDefault = QString());
        void browsePathLog();
        void browsePathBib();
        void browsePathImages();
        void browsePathPdf();
        void browsePathCommands();
        void advancedOptionsToggled(bool on);
        void advancedOptionsClicked(bool on);
        void metaFilterChanged(const QString &filter);
        void toolbarChanged(int toolbar);
        void actionsChanged(int actionClass);
        void toToolbarClicked();
        void fromToolbarClicked();
        void checkToolbarMoved();
        void customContextMenu(const QPoint &p);
        void loadOtherIcon();
        void insertSeparator();
        void populatePossibleActions(QTreeWidgetItem *parent, const QMenu *menu, bool keepHierarchy);

        void importDictionary();
        void updateCheckNow();
        void refreshLastUpdateTime();

        void revertClicked();

        void populateComboBoxFont(bool onlyMonospaced);
    private:

        #ifdef INTERNAL_TERMINAL
            void populateTerminalColorSchemes();
            void populateTerminalComboBoxFont(bool onlyMonospaced);
        #endif

    private:

        enum ContentsType { CONTENTS_BASIC , CONTENTS_ADVANCED , CONTENTS_DISABLED };

        QListWidgetItem * createIcon(const QString & caption,const QIcon &,ContentsType = CONTENTS_BASIC);

        bool askRiddle();
        void hideShowAdvancedOptions(QWidget *,bool on);

        static bool metaFilterRecurseWidget(const QString & filter,QWidget *);
        static bool metaFilterRecurseLayout(const QString & filter,QLayout *);

        static int lastUsedPage;
        static QPoint lastSize;

        BuildManager * mBuildManager;
        int oldToolbarIndex;

};


Q_DECLARE_METATYPE(QAction *)


#endif
