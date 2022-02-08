#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"

static QSet<QString> environmentsRequiringTrailingLineBreak = QSet<QString>() << "supertabular";



// check whether the cursor is inside a table environemnt

bool LatexTables::inTableEnv(QDocumentCursor & cur){

	QDocumentCursor c(cur);
	
	int result = findNextToken(c,QStringList(),false,true);
	
	if(result != -2)
		return false;
	
	if(c.lineNumber() == cur.lineNumber())
		return false;
	
	QString line = c.line().text();
	
	int pos = line.indexOf("\\begin");
	
	if(pos < 0)
		return false;
		
	QStringList values;
	LatexParser::resolveCommandOptions(line,pos,values);
	
	QString env = values.takeFirst();
	
	if(!env.startsWith("{"))
	
	if(!env.endsWith("}"))
		return -1;
	
	env = env.mid(1);
	env.chop(1);
	
	if(tabularNames.contains(env) || tabularNamesWithOneOption.contains(env)){

		int result = findNextToken(c, QStringList());
		
		if(result != -2)
			return false;
		
		if(c.lineNumber() > cur.lineNumber())
			return true;
	}

	return false;
}


void LatexTables::generateTableFromTemplate(LatexEditorView * edView,QString templateFileName,QString def,QList<QStringList> table,QString env,QString width){
	
	//read in js template which generates the tabular code
	
	QFile file(templateFileName);
	
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	
	QTextStream stream(&file);
	QString templateText;
	templateText = stream.readAll();
	
	//env
	QString envDef = "var env=\"" + env + "\"\n";
	
	//tabular column definition
	QString templateDef = "var def=\"" + def + "\"\n";
    
	// width def (only tabu at the moment)
    QString widthDef= "var widthDef=\""+width+ "\"\n";
    widthDef.replace("\\", "\\\\");
	
	//tabular column definition, split
	QString templateDefSplit = "var defSplit=[";
	QStringList lst = splitColDef(def);
	
	templateDefSplit += "\"" + lst.join("\",\"") + "\"";
	templateDefSplit += "]\n";
	
	//tabular content as js array
	QString tableDef = "var tab=[\n";
	
	for(int i = 0;i < table.size();i++){
		
		QStringList lst = table.at(i);
		QStringList::iterator it;
        
		for(it = lst.begin();it != lst.end();++it){
			QString str = * it;
			str.replace("\\","\\\\");
			str.replace("\"","\\\"");
			str.replace("\n","\\n");
			str.replace("\t","\\t");
			* it = str;
		}
	
		tableDef += "[\"" + lst.join("\",\"") + "\"]";
	
		if(i < table.size() - 1)
			tableDef += ",\n";
	}

	tableDef += "]\n";
	
	//join js parts
	templateText.prepend(tableDef);
    templateText.prepend(widthDef);
	templateText.prepend(envDef);
	templateText.prepend(templateDefSplit);
	templateText.prepend(templateDef);
	
	//generate tabular in editor
	executeScript(templateText, edView);
}


QString LatexTables::getTableText(QDocumentCursor & cursor){

	int result = findNextToken(cursor,QStringList(),false,true);
	
	if(result != -2)
		return "";
	
	QString line = cursor.line().text();
	
	int i = line.indexOf("\\begin");
	
	if(i >= 0)
		cursor.setColumnNumber(i);
	
	result = findNextToken(cursor,QStringList(),true,false);
	
	if(result != -2)
		return "";

	line = cursor.line().text();

	QRegExp rx("\\\\end\\{.*\\}");

	i = rx.indexIn(line);

	if(i >= 0)
		cursor.setColumnNumber(i + rx.cap(0).length(),QDocumentCursor::KeepAnchor);
	
	return cursor.selectedText();
}


void LatexTables::alignTableCols(QDocumentCursor & cur){

	QString text = getTableText(cur);

	if(!cur.hasSelection())
		return;
	
	QString indentation = cur.selectionStart().line().indentation();


	// split off \begin and \end parts

	int index = text.indexOf("\\begin{") + 6;
	int cellsStart;
	
	auto args = getCommandOptions(text,index,& cellsStart);
	
	if(args.count() < 1)
		return;
	
	QString tableType = args.at(0).value;
	

	// special treatment for tabu/longtabu

	if((tableType == "tabu" || tableType == "longtabu") && args.count() == 1){ //special treatment for tabu to linewidth etc.
		
		int startExtra = cellsStart;
		int endExtra = text.indexOf("{", startExtra);
		
		if(endExtra >= 0 && endExtra > startExtra){
			QString textHelper = text;
			textHelper.remove(startExtra, endExtra - startExtra); // remove to/spread definition
			args = getCommandOptions(textHelper, index, &cellsStart);
			cellsStart += endExtra - startExtra;
		}
	}


	auto alignment = [ & ]() -> QString {

		if(args.count() < 3 && mathTables.contains(tableType))
			return "l";

		if(args.count() < 1 && tabularNames.contains(tableType))
			return args.at(1).value;

		if(tabularNamesWithOneOption.contains(tableType))
			return (args.count() < 3) ? "" : args.at(2).value;
			
		return "";
	}();


	int cellsEnd = text.indexOf("\\end{" + tableType);
	
	if(cellsEnd < 0)
		return;
	
	QString beginPart = text.left(cellsStart);
	QString endPart = text.mid(cellsEnd);

	LatexTableModel ltm;
	
	ltm.setContent(text.mid(cellsStart,cellsEnd - cellsStart));

	QStringList l_defs = splitColDef(alignment);
	simplifyColDefs(l_defs);
	
	bool forceNewline = environmentsRequiringTrailingLineBreak.contains(tableType);
    
	QString tab = "\t";
    
	auto cfg = ConfigManager::getInstance();
    
	if(cfg)
        if(cfg -> getOption("Editor/Indent with Spaces").toBool()){
            int tabs = cfg -> getOption("Editor/TabStop",4).toInt();
            tab.fill(' ',tabs);
        }

    QStringList content = ltm.getAlignedLines(l_defs,tab,forceNewline);

	QString result = beginPart + '\n';

	for(int i = 0;i < content.count();i++)
		result.append(indentation + content.at(i) + '\n');

	result.append(indentation + endPart);
	cur.replaceSelectedText(result);
}


LatexTableModel::LatexTableModel(QObject * parent) 
	: QAbstractTableModel(parent)
	, metaLineCommandPos(0){

	auto cfg = ConfigManager::getInstance();

	if(!cfg)
		return;

	metaLineCommands = cfg -> getOption("TableAutoformat/Special Commands")
		.toString()
		.split(',');

	// make sure there are no empty commands due to double or trailing comma

	metaLineCommands.removeAll("");  

	metaLineCommandPos = cfg -> getOption("TableAutoformat/Special Command Position")
		.toInt();
}
