#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"



QString LatexTables::getDef(QDocumentCursor & cur){

	QDocumentCursor c(cur);
	
	int result = findNextToken(c, QStringList(), false, true);
	
	if(result != -2)
		return QString();
	
	QString line = c.line().text();
	QString opt;
	
	int pos = line.indexOf("\\begin");
	
	if(pos > -1){

		QStringList values;
		QList<int> starts;
		
		LatexParser::resolveCommandOptions(line,pos,values,&starts);

		QString env = values.takeFirst();

		pos = starts.takeFirst();
		
		if(!env.startsWith("{"))
			return "";
			
		if(!env.endsWith("}"))
			return "";

		env = env.mid(1);
		env.chop(1);

		// special treatment for tabu/longtabu
		if((env == "tabu" || env == "longtabu") && values.count() < 2){
			
			int startExtra = pos + env.length() + 2;
			int endExtra = line.indexOf("{", startExtra);
			
			if(endExtra >= 0 && endExtra > startExtra){

				QString textHelper = line;
				
				textHelper.remove(startExtra, endExtra - startExtra); // remove to/spread definition
				
				LatexParser::resolveCommandOptions(textHelper, pos, values, &starts);
				
				for(int i = 1;i < starts.count();i++)
					starts[i] += endExtra - startExtra;
			}
		}

		int numberOfOptions = -1;
		
		if(tabularNames.contains(env))
			numberOfOptions = 0;
		
		if(tabularNamesWithOneOption.contains(env))
			numberOfOptions = 1;
		
		if(numberOfOptions >= 0)
			while (!values.isEmpty()) {

				opt = values.takeFirst();
				pos = starts.takeFirst();

				if(opt.startsWith("[") && opt.endsWith("]"))
					continue;
				
				if(numberOfOptions > 0){
					numberOfOptions--;
					continue;
				}

				if(!opt.startsWith("{"))
					return "";
					
				if(!opt.endsWith("}"))
					return "";

				opt = opt.mid(1);
				opt.chop(1);
				cur.moveTo(c.lineNumber(), pos + 1);
				cur.movePosition(opt.length(), QDocumentCursor::NextCharacter, QDocumentCursor::KeepAnchor);
			}
	}

	return opt;
}


QStringList LatexTables::splitColDef(QString def){

	QStringList result;
	
	bool inDef = false;
	bool inAt = false;
	bool inMultiplier = false;
	bool inMultiplied = false;
	bool appendDef = false;
	
	int multiplier = 0;
	
	QString multiplier_str;
	QString before_multiplier_str;
	
	int curl = 0;
	int sqrBracket = 0;
	
	QString col;
	
	for(int i = 0;i < def.length();i++){

		QChar ch = def.at(i);

		if(ch == '*' && curl == 0){
			inMultiplier = true;
			multiplier_str.clear();
			multiplier = 0;
			before_multiplier_str = col;
			continue;
		}

		if(!inMultiplied && !inMultiplier)
			col.append(ch);

		if(ch == '}'){

			curl--;
			
			if(curl == 0){

				if(appendDef){
					appendDef = false;
					result << col;
					col.clear();
				}

				if(inDef)
					inDef = false;
				
				if(inAt)
					inAt = false;
				
				if(inMultiplied){
					
					QStringList helper = splitColDef(multiplier_str);
					
					inMultiplied = false;
					
					int zw = result.size();

					for(int k = 0;k < multiplier;k++)
						result << helper;

					if(!col.isEmpty() && result.size() > zw){
						result[zw] = col + result[zw];
						col.clear();
					}

					before_multiplier_str.clear();
					multiplier = 0;
					multiplier_str.clear();
				}

				if(inMultiplier){

					bool ok;
					multiplier = multiplier_str.toInt(&ok);
					
					if(ok){
						inMultiplied = true;
					} else {
						multiplier = 0;
					}

					multiplier_str.clear();
					inMultiplier = false;
				}
			}
		}

		if((inMultiplier || inMultiplied) && curl > 0)
			multiplier_str.append(ch);

		if(ch == '<')
			inDef = true;
			
		if(ch == '>')
			inDef = true;

		if(ch == '{')
			curl++;
		
		if(inMultiplier && curl == 0){ // multiplier not in braces

			multiplier_str.append(ch);

			bool ok;
			
			multiplier = multiplier_str.toInt(& ok);
			
			if(ok)
				inMultiplied = true;
			else
				multiplier = 0;

			multiplier_str.clear();
			inMultiplier = false;
		}

		if(ch == 's' || ch == 'S'){

		}
		
		if(ch == '[')
			sqrBracket++;
		
		if(ch == ']')
			sqrBracket--;

		if(
			(ch.isLetter() || ch == ']') && 
			! inAt && 
			! inDef && 
			curl == 0 && 
			sqrBracket == 0
		){
			if(
				(ch == 's' || ch == 'S' || ch == 'X') && 
				i + 1 < def.length() && 
				def.at(i + 1) == '['
			) continue;
			
			if((i + 1 < def.length()) && def.at(i + 1) == '{')
				appendDef = true;
			else {
				result << col;
				col.clear();
			}
		}
	}

	if(!result.isEmpty())
		result.last().append(col);

	return result;
}


// removes an @{} sequence form colDef, removes vertical lines '|', removes parameters in curly and square brackets

void LatexTables::simplifyColDefs(QStringList & colDefs){

	for(int i = 0;i < colDefs.count();i++){

		QString colDef = colDefs.at(i);

		colDef.remove('|');
		
		if(colDef.startsWith('>'))
			if(colDef.at(1) == '{'){

				int start = 1;
				int cb = findClosingBracket(colDef, start);

				if(cb >= 0 && colDef.at(cb) == '}' && cb + 1 < colDef.length())
					colDef = colDef.mid(cb + 1);
				else
					colDef = "l"; // fall back
			} else {
				colDef = "l"; // fall back
			}

		if(colDef.startsWith('@'))
			if(colDef.at(1) == '{'){

				int start = 1;
				int cb = findClosingBracket(colDef, start);
				
				if(cb >= 0 && colDef.at(cb) == '}' && cb + 1 < colDef.length())
					colDef = colDef.mid(cb + 1);
				else
					colDef = "l"; // fall back
			} else {
				colDef = "l"; // fall back
			}

		if(colDef.length() >= 2 && colDef.at(1) == '{')
			colDef = colDef.at(0);

		if(colDef.length() >= 2 && colDef.at(1) == '[')
			colDef = colDef.at(0);

		if(colDef.length() >= 2 && colDef.at(1) == '<')
			colDef = colDef.at(0);

		colDefs.replace(i, colDef);
	}
}


QString LatexTables::getSimplifiedDef(QDocumentCursor & cursor){

	QString def = getDef(cursor);
	QStringList l_defs = splitColDef(def);
	
	def = l_defs.join("");
	def.remove('|');
	
	return def;
}