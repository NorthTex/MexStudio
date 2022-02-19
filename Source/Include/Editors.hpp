#ifndef Header_Editors
#define Header_Editors


#include "Latex/EditorView.hpp"
#include "TxsTabWidget.hpp"
#include "EditorChangeProxy.hpp"


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

		void removeEditor(LatexEditorView *,TxsTabWidget *);
		
		void insertEditor(LatexEditorView *,
			TxsTabWidget * = nullptr , // Current
			int pos = -1 , // Append
			bool asCurrent = true
		);

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


#endif
