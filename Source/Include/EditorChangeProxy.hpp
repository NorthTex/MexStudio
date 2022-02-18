#ifndef Header_EditorChangeProxy
#define Header_EditorChangeProxy


#include "Editors.hpp"


class Editors;


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
		Editors * editors;

		bool blocked;

};


#endif
