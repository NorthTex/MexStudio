#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"

QSet<QString> LatexTables::tabularNames = QSet<QString>() << "tabular" << "array" << "longtable" << "supertabular" << "tabu" << "longtabu"
                                        << "IEEEeqnarray" << "xtabular" << "xtabular*" << "mpxtabular" << "mpxtabular*";
QSet<QString> LatexTables::tabularNamesWithOneOption = QSet<QString>() << "tabular*" << "tabularx" << "tabulary";
QSet<QString> LatexTables::mathTables = QSet<QString>() << "align" << "align*" << "array" << "matrix" << "matrix*" << "bmatrix" << "bmatrix*"
                                      << "Bmatrix" << "Bmatrix*" << "pmatrix" << "pmatrix*" << "vmatrix" << "vmatrix*"
                                      << "Vmatrix" << "Vmatrix*" << "split" << "multline" << "multline*"
                                      << "gather" << "gather*" << "flalign" << "flalign*" << "alignat" << "alignat*"
                                      << "cases" << "aligned" << "gathered" << "alignedat";







// get column in which the cursor is positioned.
// is determined by number of "&" before last line break (\\)

int LatexTables::getColumn(QDocumentCursor & cur){

	QDocumentCursor c(cur);
    QStringList tokens { "\\\\" , "\\tabularnewline" };
	
	int result = findNextToken(c,tokens,true,true);
	
	if(result == 0)
		c.movePosition(2,QDocumentCursor::NextCharacter,QDocumentCursor::KeepAnchor);

    if(result == 1)
		c.movePosition(15,QDocumentCursor::NextCharacter,QDocumentCursor::KeepAnchor);
    
	if(c.lineNumber() == cur.lineNumber() && c.selectedText().contains(QRegularExpression("^\\s*$"))){

		c.movePosition(1,QDocumentCursor::EndOfLine,QDocumentCursor::KeepAnchor);
		
		QString zw = c.selectedText();
        
		if(zw.contains(QRegularExpression("^\\s*$")))
			return -1;
	}

	c.clearSelection();

	tokens << "\\&" << "&";
	
	int col = 0;

	do {
		result = findNextToken(c,tokens);
		
		if(c.lineNumber() > cur.lineNumber())
			break;
			
		if(c.lineNumber() == cur.lineNumber() && c.columnNumber() > cur.columnNumber())
			break;
        
		if(result == 3)
			col++;

    } while (result > 1);
	
	return col;
}


void LatexTables::executeScript(QString script,LatexEditorView * view){
	scriptengine engine;
	engine.setEditorView(view);
	engine.setScript(script);
	engine.run();
}


// return the index of the first double backslash ("\\\\") after startCol which is on the same
// brace level as startCol (i.e. ignoring double slash within braces)

int LatexTableModel::findRowBreak(const QString & text,int startCol) const {
	
	bool previousIsBackslash = false;
	int braceDepth = 0;
	
	for(int col = startCol;col < text.length();col++){
		switch(text.at(col).toLatin1()){
		case '{':
			braceDepth++;
			break;
		case '}':
			braceDepth--;
			break;
		case '\\':
			if(braceDepth == 0){
				if(previousIsBackslash){
					return col-1;
				} else {
					previousIsBackslash = true;

                    if(text.mid(col,15) == "\\tabularnewline")
                        return col;

					continue;
				}
			}
		}

		previousIsBackslash = false;
	}

	return -1;
}


// input everything between \begin{} and \end{}

void LatexTableModel::setContent(const QString & text){

	int col = 0;
	
	while(col >= 0 && col < text.length()){

		auto ltl = parseNextLine(text,col);

		if(!ltl)
			break;
		
		lines.append(ltl);
	}

	/*	*** alternative more efficient ansatz ***
	inline int skipWhitespace(const QString &line, int pos=0) {while (pos<line.length()) {if (!line.at(pos).isSpace()) return pos; else pos++;}}

	int len = text.length();
	int pos=skipWhitespace(text);
	int start = pos;
	LatexTableLine *ltl = new LatexTableLine(this);
	bool hasMetaContent = false;
	while (pos < len) {
	if (text.at(pos) == '\\') {
	if (pos < len && text.at(pos+1)  == '\\') {
	ltl->setColStr(text.mid(start, pos-start));
	pos+=2;
	start=pos;
	} else {
	QString cmd;
	int end = getCommand(text, cmd, pos);
	if (metaLineCommands.contains(cmd)) {
	 QStringList args;
	 getCommandOptions(text, end, end);
	 hasMetaContent = true;
	}

	}

	}

	pos = skipWhitespace(pos);
	}
	*/
}


QVariant LatexTableModel::data(const QModelIndex & index,int role) const {
	return (role == Qt::DisplayRole)
		? lines.at(index.row()) -> colText(index.column())
		: QVariant();
}


LatexTableLine::LatexTableLine(QObject * parent) 
	: QObject(parent){}

