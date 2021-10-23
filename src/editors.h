#ifndef Header_Editors
#define Header_Editors

#include "mostQtHeaders.h"

class TxsTabWidget;
class LatexEditorView;
class EditorChangeProxy;

class Editors : public QWidget {

	Q_OBJECT

	public:

		enum Position { AbsoluteFront , AbsoluteEnd , GroupFront , GroupEnd };

		explicit Editors(QWidget * parent = nullptr);

		void insertEditor(LatexEditorView *,int index,bool asCurrent = true);
		void addTabWidget(TxsTabWidget *);
		void moveEditor(LatexEditorView *,Position pos);
		void addEditor(LatexEditorView *,bool asCurrent = true);

	public slots:

		void requestCloseEditor(LatexEditorView *);
		void removeEditor(LatexEditorView *);

	protected:

		void insertEditor(LatexEditorView *,TxsTabWidget * = nullptr /*current*/,int pos = -1 /*append*/,bool asCurrent = true);
		void removeEditor(LatexEditorView *,TxsTabWidget *);

	public:

		bool containsEditor(LatexEditorView *) const;

		TxsTabWidget * currentTabWidget() const;
		LatexEditorView * currentEditor() const;
		QList<LatexEditorView *> editors();

		void setCurrentEditor(LatexEditorView *,bool setFocus = true);
		void moveToTabGroup(LatexEditorView *,int groupIndex,int targetIndex);

		int tabGroupIndexFromEditor(LatexEditorView *) const;

	signals:

		void editorAboutToChangeByTabClick(LatexEditorView * from,LatexEditorView * to);

		void closeCurrentEditorRequested();
		void currentEditorChanged();
		void listOfEditorsChanged();
		void editorsReordered();

	public slots:

		void closeOtherEditorsFromAction();
		void setCurrentEditorFromAction();
		void setCurrentEditorFromSender();
		void toggleReadOnlyFromAction();
		void closeEditorFromAction();

		bool activatePreviousEditor();
		bool activateNextEditor();

	protected slots:

		bool activateTabWidgetFromSender();
		void changeSplitOrientation();
		void moveAllToOtherTabGroup();
		void moveToOtherTabGroup();

		void onEditorChangeByTabClick(LatexEditorView * from,LatexEditorView * to);
		void tabBarContextMenu(const QPoint &);
		void moveToTabGroup(LatexEditorView *,TxsTabWidget * target,int targetIndex);
		void setCurrentGroup(int index);

	protected:

		TxsTabWidget *tabWidgetFromEditor(LatexEditorView *) const;

	private:

		QList<TxsTabWidget *> tabGroups;
		EditorChangeProxy * changes;
		QSplitter * splitter;

		int currentGroupIndex;

};



class EditorChangeProxy : public QObject {

	Q_OBJECT

	public:

		EditorChangeProxy(Editors *);

		void release();
		bool block();

	signals:

		void currentEditorChanged();
		void listOfEditorsChanged();

	public slots:

		void currentEditorChange();
		void listOfEditorsChange();

	private:

		QList<LatexEditorView *> listOfEditorsAtBlock;
		LatexEditorView * currentEditorAtBlock;
		Editors *editors;

		bool blocked;

};


#endif
