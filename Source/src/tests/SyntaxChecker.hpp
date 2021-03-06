#ifndef Test_SyntaxChecker
#define Test_SyntaxChecker
#ifndef QT_NO_DEBUG
#include "mostQtHeaders.h"

#include "Latex/EditorView.hpp"

class SyntaxCheckTest: public QObject{
	Q_OBJECT
	public:
		SyntaxCheckTest(LatexEditorView* editor);
	private:
		LatexEditorView *edView;
	private slots:
		void checktabular_data();
		void checktabular();
        void checkkeyval_data();
        void checkkeyval();
        void checkArguments_data();
        void checkArguments();
};

#endif
#endif
