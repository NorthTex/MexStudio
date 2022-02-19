#ifndef Header_TxsTabWidget
#define Header_TxsTabWidget


#include "Latex/EditorView.hpp"


class TxsTabWidget : public QTabWidget {

	Q_OBJECT

	public:

		explicit TxsTabWidget(QWidget * parent = 0);

		QList<LatexEditorView *> editors() const;

		LatexEditorView * currentEditor() const;
		LatexEditorView * editorAt(QPoint);

		void setCurrentEditor(LatexEditorView *);
		void setActive(bool active);
		void moveTab(int from,int to);

		bool containsEditor(LatexEditorView *) const;
		bool currentEditorViewIsFirst() const;
		bool currentEditorViewIsLast() const;
		bool isEmpty() const;

	signals:

		void editorAboutToChangeByTabClick(LatexEditorView * from,LatexEditorView * to);
		void tabBarContextMenuRequested(QPoint);
		void closeEditorRequested(LatexEditorView *);
		void currentEditorChanged();
		void activationRequested();
		void tabMoved(int from,int to);

	public slots:

		void gotoFirstDocument();
		void gotoNextDocument();
		void gotoPrevDocument();
		void gotoLastDocument();

		// low level public functions

		void insertEditor(LatexEditorView *,int pos = -1,bool asCurrent = true);
		void removeEditor(LatexEditorView *);

	protected:

		LatexEditorView * editorAt(int index);

		void disconnectEditor(LatexEditorView *);
		void connectEditor(LatexEditorView *);
		void updateTab(int index);

	protected slots:

		void updateTabFromSender();

	private slots:

		void currentTabAboutToChange(int from,int to);
		void onTabCloseRequest(int i);

	private:

		bool m_active;
};


Q_DECLARE_METATYPE(TxsTabWidget *)


#endif
