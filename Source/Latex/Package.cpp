#include "latexcompleter_config.h"
#include "latexparser/latexparser.h"

#include "Latex/Package.hpp"

CommandDescription extractCommandDef(QString line, QString definition);
CommandDescription extractCommandDefKeyVal(QString line, QString &key);


LatexPackage::LatexPackage() 
	: notFound(false) {
	
	completionWords.clear();
	packageName.clear();
}


QString LatexPackage::makeKey(const QString & cwlFilename,const QString & options){
	return QString("%1#%2")
		.arg(options)
		.arg(cwlFilename);
}


QString LatexPackage::keyToCwlFilename(const QString & key){

	const int i = key.indexOf('#');

	return (i >= 0)
		? key.mid(i + 1)
		: key;
}


// Workaround since there is currently no reliable way 
// to determine the packageName from LatexPackage directly 
// (the attribute with the same name contains the key and sometimes nothing).

QString LatexPackage::keyToPackageName(const QString & key){

	auto name = LatexPackage::keyToCwlFilename(key);

	if(name.endsWith(".cwl"))
		name.remove(name.length() - 4,4);

	if(name.startsWith("class-"))
		name.remove(0,6);
	
	return name;
}


QStringList LatexPackage::keyToOptions(const QString & key){

	const int i = key.indexOf('#');
	
	if(i < 0)
		return QStringList();
	
	auto result = key
		.left(i)
		.split(',');
	
	for(int k = 0;k < result.size();k++){

		QString elem = result.value(k);
		
		if(elem.contains('%')) {
			const int i = elem.indexOf('%');
			elem = elem.left(i);
		}
		
		elem = elem.simplified();
		
		result[k] = elem;
	}
	
	return result;
}


void LatexPackage::unite(LatexPackage &add, bool forCompletion){

	// expensive

	if(forCompletion){
		completionWords.unite(add.completionWords);
		return;
	}
	
	optionCommands.unite(add.optionCommands);
	environmentAliases.unite(add.environmentAliases);
    specialTreatmentCommands.insert(add.specialTreatmentCommands);
    specialDefCommands.insert(add.specialDefCommands);

	// overloaded unit, which does not overwrite well defined CDs with poorly defined ones

	commandDescriptions.unite(add.commandDescriptions);

	// possibleCommands.unite(add.possibleCommands);

	// expensive

	for(const auto & command : add.possibleCommands.keys()){
		
		auto 
			setB = add.possibleCommands[command],
			setA = possibleCommands[command];

		setA.unite(setB);
		possibleCommands[command] = setA;
	}
}


LatexPackage loadCwlFile(
	const QString fileName,
	LatexCompleterConfig * config,
	QStringList conditions
){

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	CodeSnippetList words;
	LatexPackage package;


	QFile tagsfile("cwl:" + fileName);

	bool skipSection = false;
	
	if(tagsfile.exists() && tagsfile.open(QFile::ReadOnly)){

		QString line;
		QTextStream stream(&tagsfile);
		
		QRegExp rxCom("^(\\\\\\w+\\*?)(\\[.+\\])*\\{(.*)\\}");  // expression for \cmd[opt]{arg} (cmd may be starred, [opt] can appear arbitrary often)
		QRegExp rxCom2("^(\\\\\\w+\\*?)\\[(.+)\\]");            // expression for \cmd[opt]      (cmd may be starred)
		QRegExp rxCom3("^(\\\\\\w+\\*?)");                      // expression for \cmd           (cmd may be starred)
		
		rxCom.setMinimal(true);
		
		QStringList keywords;
		keywords << "text" << "title" << "%<text%>" << "%<title%>";
		
		QStringList specialTreatment;
		specialTreatment << "color";
		
		QString keyvals;
        
		package.containsOptionalSections=false;
		
		while(!stream.atEnd()){

			line = stream.readLine().trimmed();
			
			// end of conditional section

			if(line.startsWith("#endif")){
				skipSection = false;
				continue;
			}

			if(line.startsWith("#ifOption:")){
                package.containsOptionalSections = true;
				QString condition = line.mid(10);
				skipSection = !(conditions.contains(condition) || conditions.contains(condition + "=true"));
				continue;
			}

			// skip conditional sections (if condition is not met)

			if(skipSection)
				continue;

			//include additional cwl file

			if(line.startsWith("#include:")){
	
				QString fn = line.mid(9);
	
				if(
					!fn.isEmpty() &&
					fileName != fn + ".cwl" && !package.requiredPackages.contains(fn + ".cwl")
				)	package.requiredPackages << fn + ".cwl";
			}

			// start reading keyvals

			if(line.startsWith("#keyvals:")){
				keyvals = line.mid(9);
				continue;
			}

			// end of reading keyvals

			if(line.startsWith("#endkeyvals")){
				keyvals.clear();
				continue;
			}

			// read keyval (name stored in "keyvals")

			if(!keyvals.isEmpty() && !line.startsWith("#")){

				QStringList l_cmds=keyvals.split(',');

				QString key;
				CommandDescription cd = extractCommandDefKeyVal(line, key);

				for(const auto & command : l_cmds){

					package.possibleCommands["key%" + command] << line;
					
					if(cd.args > 0){

						if(key.endsWith("="))
							key.chop(1);
					
						package.commandDescriptions.insert(command + "/" + key, cd);
					}
				}

				continue;
			}

			// start reading keyvals

			if(line.startsWith("#repl:")){
				package.possibleCommands["%replace"] << line.mid(6);
				continue;
			}

			// hints for commands usage (e.g. in mathmode only) are separated by #

			if(!line.isEmpty() && !line.startsWith("#") && !line.startsWith(" ")){

				int sep = line.indexOf('#');
				
				QString valid;
				QStringList env;
				QString definition;
				
				bool uncommon = false;
				bool hideFromCompletion = false;
				
				auto type = CodeSnippet::none;
				
				if(sep > -1){
					
					valid = line.mid(sep + 1);
					line = line.left(sep);

					if(valid.startsWith("*")){
						valid = valid.mid(1);
						uncommon = true;
					}
					
					// second time split for specialDef
					
					int sep = valid.indexOf('#');
					
					if(sep > -1){
						definition = valid.mid(sep + 1);
						valid = valid.left(sep);
					}
					
					// normal valid
					
					if(valid.startsWith("/")){
						env = valid.mid(1).split(',');
						valid = "e";
					}
					
					if(valid.contains("\\")){
						int i = valid.indexOf("\\");
						QString zw = valid.mid(i + 1);
						env = zw.split(',');
						valid = valid.left(i);
					}
					
					if(valid.contains('S')){
						hideFromCompletion = true;
						valid.remove('S');
					}
				}

				// parse for spell checkable commands

				int res = rxCom.indexIn(line);
				
				if(keywords.contains(rxCom.cap(3)))
					package.optionCommands << rxCom.cap(1);

				if(specialTreatment.contains(rxCom.cap(3)))
					package.specialTreatmentCommands[rxCom.cap(1)].insert(qMakePair(rxCom.cap(3), 1));

				// for commands which don't have a braces part e.g. \item[text]

				rxCom2.indexIn(line);
				
				// for commands which don't have a options either e.g. \node (asas)

				int res3 = rxCom3.indexIn(line);

				// get commandDefinition

				CommandDescription cd = extractCommandDef(line,valid);

				// bracket command like \left etc

				if(valid.contains('K')){
					cd.bracketCommand=true;
					valid.remove("K");
				}

				QString cmd = rxCom3.cap(1);

				if(cmd == "\\begin"){

					// one insertion of a general \begin-command

					if(!package.commandDescriptions.contains(cmd)){
						CommandDescription cd;
						cd.args = 1;
						cd.argTypes << Token::beginEnv;
						package.commandDescriptions.insert(cmd, cd);
					}

					cmd = rxCom.cap();
				}

				if(package.commandDescriptions.contains(cmd)){

					CommandDescription cd_old = package.commandDescriptions.value(cmd);
					
					if(cd_old.args == cd.args && cd_old.optionalArgs > cd.optionalArgs)
						cd = cd_old;

					if(cd_old.args == cd.args && cd_old.optionalArgs == cd.optionalArgs && cd_old.overlayArgs > cd.overlayArgs)
						cd = cd_old;

					if(cd_old.args < cd.args && cd_old.args > 0){
						cd = cd_old;
						
						#ifndef QT_NO_DEBUG
							qDebug() << "inconsistent command arguments:" << cmd << fileName;
							// commands with different numbers of mandatory arguments are not distinguished by the parser and lead to unreliable results.
							// the lower numer of mandatory arguments is handled only (however not an command with zero arguments)
							// this leads to incomplete handling e.g. for hyperref (which disregards any standards and distinguishes command based on the presence of an optional argument)
						#endif
					}

					if(cd_old.args > cd.args){

						#ifndef QT_NO_DEBUG
							if (cd.args > 0) {
								qDebug() << "inconsistent command arguments:" << cmd << fileName;
								// commands with different numbers of mandatory arguments are not distinguished by the parser and lead to unreliable results.
								// the lower numer of mandatory arguments is handled only (however not an command with zero arguments)
								// this leads to incomplete handling e.g. for hyperref (which disregards any standards and distinguishes command based on the presence of an optional argument)
							}
						#endif
						
						cd = cd_old;
					}

				}

				if(!valid.contains('M')){
					package.commandDescriptions.insert(cmd, cd);
				} else {
					// don't use line for command description
					valid.remove('M');
				}


				// remove newtheorem declaration

				valid.remove('N'); 


				if(keywords.contains(rxCom2.cap(2)))
					package.optionCommands << rxCom2.cap(1);

				// definition command

				if(valid.contains('d')){ 
	
					if(res > -1)
						package.possibleCommands["%definition"] << rxCom.cap(1);
	
					valid.remove('d');
				}

				// include like command

				if(valid.contains('i')){
					
					if(res > -1)
						package.possibleCommands["%include"] << rxCom.cap(1);

					valid.remove('i');
				}

				// label command

				if(valid.contains('l')){
					
					if(res > -1)
						package.possibleCommands["%label"] << rxCom.cap(1);
					
					valid.remove('l');
				}

				// ref command

				if(valid.contains('r')){
					if(res > -1){
						package.possibleCommands["%ref"] << rxCom.cap(1);

                        QRegularExpression re{"{.*?}"};
                        QRegularExpressionMatchIterator it = re.globalMatch(line);
                        QRegularExpressionMatch match;

                        for(int i = 0;i < cd.argTypes.size();++i){

                            match = it.next();

                            if(cd.argTypes[i] == Token::labelRef)
                                break;
                        }

                        if(match.hasMatch())
                            line.replace(match.capturedStart(),match.capturedLength(),"{@l}");
					}

					valid.remove('r');
				}

				if(valid.contains('L')){
					if(valid.contains("L0")){

						valid.remove("L0");
						
						if(res > -1)
							package.possibleCommands["%structure0"] << cmd;
					} else
					if(valid.contains("L1")){

						valid.remove("L1");
						
						if(res > -1)
							package.possibleCommands["%structure1"] << cmd;
					} else
					if (valid.contains("L2")) {
						valid.remove("L2");
						if (res > -1) {
							package.possibleCommands["%structure2"] << cmd;
						}
					} else if (valid.contains("L3")) {
						valid.remove("L3");
						if (res > -1) {
							package.possibleCommands["%structure3"] << cmd;
						}
					} else if (valid.contains("L4")) {
						valid.remove("L4");
						if (res > -1) {
							package.possibleCommands["%structure4"] << cmd;
						}
					} else if (valid.contains("L5")) {
						valid.remove("L5");
						if (res > -1) {
							package.possibleCommands["%structure5"] << cmd;
						}
					} else if (valid.contains("L6")) {
						valid.remove("L6");
						if (res > -1) {
							package.possibleCommands["%structure6"] << cmd;
						}
					} else if (valid.contains("L7")) {
						valid.remove("L7");
						if (res > -1) {
							package.possibleCommands["%structure7"] << cmd;
						}
					} else if (valid.contains("L8")) {
						valid.remove("L8");
						if (res > -1) {
							package.possibleCommands["%structure8"] << cmd;
						}
					} else if (valid.contains("L9")) {
						valid.remove("L9");
						if (res > -1) {
							package.possibleCommands["%structure9"] << cmd;
						}
					}
				}
				if (valid.contains('V')) { // verbatim command
					if (res > -1) {
						package.possibleCommands["%verbatimEnv"] << rxCom.cap(3);
						env << "verbatim";
					}
					valid.remove('V');
				}
				if (valid.contains('s')) { // special def
					if (res > -1) {
						package.specialDefCommands.insert(rxCom.cap(1), definition);
					} else {
						if (res3 > -1)
							package.specialDefCommands.insert(rxCom3.cap(1), definition);
					}
					if (definition.startsWith('%')) {
						if (config)
							config->specialCompletionKeys.insert(definition);
					} else {
						if (definition.length() > 2) {
							QString helper = definition.mid(1, definition.length() - 2);
							if (helper.startsWith('%')) {
								if (config)
									config->specialCompletionKeys.insert(helper);
							}
						}
					}
					valid.remove('s');
				}
				if (valid.contains('c')) { // cite command
                    // replace 'c' to 'C' to maintain cwl compatibility
					valid.remove('c');
                    valid.append('C');
				}
				if (valid.contains('C')) { // cite command
					if (res > -1) {
                        QRegularExpression re{"{.*?}"};
                        QRegularExpressionMatchIterator it = re.globalMatch(line);
                        QRegularExpressionMatch match;
                        for(int i=0;i<cd.argTypes.size();++i){
                            match = it.next();
                            if(cd.argTypes[i]==Token::bibItem)
                                break;
                        }
                        package.possibleCommands["%cite"] << rxCom.cap(1);
                        if (!line.startsWith("\\begin")) // HANDLE begin extra
                            package.possibleCommands["%citeExtendedCommand"] << rxCom.cap(1);
                        line.replace(match.capturedStart(),match.capturedLength(),"{@}");
					}
					valid.remove('C');
				}
				if (valid.contains('g')) { // definition command
					if (res > -1) {
						package.possibleCommands["%graphics"] << rxCom.cap(1);
					}
					valid.remove('g');
				}
				if (valid.contains('u')) { // usepackage command
					if (res > -1) {
						package.possibleCommands["%usepackage"] << rxCom.cap(1);
					}
					valid.remove('u');
				}
				if (valid.contains('b')) { // usepackage command
					if (res > -1) {
						package.possibleCommands["%bibliography"] << rxCom.cap(1);
						package.possibleCommands["%file"] << rxCom.cap(1);
					}
					valid.remove('b');
				}
				if (valid.contains('U')) { // url command
					if (res > -1) {
						package.possibleCommands["%url"] << rxCom.cap(1);
					}
					valid.remove('U');
				}
				if (valid.contains('D')) { // todo command
					if (res > -1) {
						package.possibleCommands["%todo"] << rxCom.cap(1);
					}
					valid.remove('D');
				}
				if (valid.contains('B')) { // color
					package.possibleCommands["%color"] << line;
					hideFromCompletion = true;
				}
				if (valid.contains('L')) { // a length/height
					type = CodeSnippet::length;
					valid.remove('L');
				}
				// normal commands for syntax checking
				// will be extended to distinguish between normal and math commands
				if (valid.isEmpty() || valid.contains('n')) {
					if (res > -1) {
						if (rxCom.cap(1) == "\\begin" || rxCom.cap(1) == "\\end") {
							package.possibleCommands["normal"] << rxCom.cap(1) + "{" + rxCom.cap(3) + "}";
						} else {
							package.possibleCommands["normal"] << rxCom.cap(1);
						}
					} else {
						if (!cmd.isEmpty())
							package.possibleCommands["normal"] << cmd;
						else
							package.possibleCommands["normal"] << line.simplified();
					}
				}
				if (valid.contains('m')) { // math commands
					if (res > -1) {
						if (rxCom.cap(1) == "\\begin" || rxCom.cap(1) == "\\end") {
							package.possibleCommands["math"] << rxCom.cap(1) + "{" + rxCom.cap(3) + "}";
						} else {
							package.possibleCommands["math"] << rxCom.cap(1);
						}
					} else {
						if (!cmd.isEmpty())
							package.possibleCommands["math"] << cmd;
						else
							package.possibleCommands["math"] << line.simplified();
					}
				}
				if (valid.contains('t')) { // tabular commands
					if (res > -1) {
						if (rxCom.cap(1) == "\\begin" || rxCom.cap(1) == "\\end") {
							package.possibleCommands["tabular"] << rxCom.cap(1) + "{" + rxCom.cap(3) + "}";
							package.possibleCommands["array"] << rxCom.cap(1) + "{" + rxCom.cap(3) + "}";
						} else {
							package.possibleCommands["tabular"] << rxCom.cap(1);
							package.possibleCommands["array"] << rxCom.cap(1);
						}
					} else {
						if (cmd.isEmpty())
							cmd = line.simplified();
						package.possibleCommands["tabular"] << cmd;
						package.possibleCommands["array"] << cmd;
					}
				}
				if (valid.contains('T')) { // tabbing support
					if (res == -1) {
						package.possibleCommands["tabbing"] << cmd;
					}
				}
				if (valid.contains('e') && !env.isEmpty()) { // tabbing support
					if (res == -1) {
						foreach (const QString &elem, env)
							package.possibleCommands[elem] << cmd;
					} else {
						QString cmd = rxCom.cap(1);
						QString envName = rxCom.cap(3);
						if (cmd == "\\begin" || cmd == "\\end") {
							cmd += "{" + envName + "}";
						}
						foreach (const QString &elem, env)
							package.possibleCommands[elem] << cmd;
					}
				}
				if (!valid.contains('e') && !env.isEmpty()) { // set env alias
					if (res > -1) {
						if (rxCom.cap(1) == "\\begin") {
							QString envName = rxCom.cap(3);
							if (!envName.isEmpty()) {
								foreach (const QString &elem, env)
									package.environmentAliases.insert(rxCom.cap(3), elem);
							}
						}
					}
				}

				// normal parsing for completer
				// command for spell checking only (auto parser)

				if(hideFromCompletion)
					continue;

				// remove special option classification e.g. %l
				// not n

                line.remove(QRegularExpression("%[a-zA-Z]+"));

				//add placeholders to brackets like () to (%<..%>)

				if(!line.contains("%")){

					const QString brackets = "{}[]()<>";
					int lastOpen = -1, openType = -1;
					
					for(int i = 0;i < line.size();i++){

						int index = brackets.indexOf(line[i]);
						
						if(index >= 0){
							if(index % 2 == 0){
								lastOpen = i;
								openType = index / 2;
							} else {

								if(lastOpen == -1 || openType != index / 2)
									continue;
								
								// ignore empty arguments, feature request 888

								if(i - lastOpen < 2)
									continue;

								//ignore single @ (to be replaced with bibid in completer)

                                if(line.mid(lastOpen + 1, 1) == "@" && line.mid(lastOpen + 2, 1) != "@")
									continue;

								line.insert(lastOpen + 1, "%<");

								i += 2;

								line.insert(i, "%>");

								/*if (lastOpen+2 == i-1) {  // deactivated, feature request 888
								    line.insert(i, QApplication::translate("CodeSnippet", "something"));
								    i+=QApplication::translate("CodeSnippet", "something").length();
								}*/
								lastOpen = -1;
								i += 2;
							}
						}
					}

					if(line.startsWith("\\begin") || line.startsWith("\\end")){

						int i = line.indexOf("%<",0);

						line.replace(i,2,"");

						i = line.indexOf("%>", 0);

						line.replace(i,2,"");

						if(line.endsWith("\\item"))
							line.chop(5);

					}
				}

                CodeSnippet cs = CodeSnippet(line);

                uint hash = qHash(line);

                CodeSnippetList::iterator it = std::lower_bound(words.begin(), words.end(), cs);

                if(it == words.end() || it -> index != hash){

                    CodeSnippetList::iterator it = std::lower_bound(words.begin(), words.end(), cs);
					it = words.insert(it, cs);
					
					int len = line.length();
					
					it -> index = hash;
					it -> snippetLength = len;
					it -> usageCount = uncommon ? -1 : 0;
					it -> type = type;
					
					if(config){

						const auto usage = config -> usage.values(hash);
						
						for(const auto & elem : usage)
							if(elem.first == it -> snippetLength){
								it -> usageCount = elem.second;
								break;
							}
					}
				}
			}
		}
	} else {
		package.packageName = "<notFound>";
		package.notFound = true;
	}

	QApplication::restoreOverrideCursor();
	package.completionWords = words;
	return package;
}


std::map<QString,Token::TokenType> multiTokens {
	{ "%short title" , Token::shorttitle } ,
	{ "%definition"  , Token::definition } ,
	{ "%plain"       , Token::generalArg } ,
	{ "%keyvals"     , Token::keyValArg  } ,
	{ "%ref"         , Token::labelRef   } ,
	{ "%formula"     , Token::formula    } ,
	{ "%labeldef"    , Token::label      } ,
	{ "%title"       , Token::title      } ,
	{ "%l"           , Token::width      } ,
	{ "%text"        , Token::text       } ,
	{ "%todo"        , Token::todo       } ,
	{ "%cmd"         , Token::def        }
};


std::map<QString,Token::TokenType> simpleTokens {
	{ "default"        , Token::optionalArgDefinition } ,
	{ "verbatimSymbol" , Token::verbatimStart         } ,
	{ "options"        , Token::packageoption         } ,
	{ "class"          , Token::documentclass         } ,
	{ "labellist"      , Token::labelRefList          } ,
	{ "args"           , Token::defArgNumber          } ,
	{ "beamertheme"    , Token::beamertheme           } ,
	{ "short title"    , Token::shorttitle            } ,
	{ "def"            , Token::definition            } ,
	{ "definition"     , Token::definition            } ,
	{ "begdef"         , Token::definition            } ,
	{ "enddef"         , Token::definition            } ,
	{ "citekey"        , Token::newBibItem            } ,
	{ "keyvals"        , Token::keyValArg             } ,
	{ "%<options%>"    , Token::keyValArg             } ,
	{ "placement"      , Token::placement             } ,
	{ "position"       , Token::placement             } ,
	{ "imagefile"      , Token::imagefile             } ,
	{ "newlength"      , Token::defWidth              } ,
	{ "key"            , Token::labelRef              } ,
	{ "key1"           , Token::labelRef              } ,
	{ "key2"           , Token::labelRef              } ,
	{ "formula"        , Token::formula               } ,
	{ "package"        , Token::package               } ,
	{ "bib files"      , Token::bibfile               } ,
	{ "bib file"       , Token::bibfile               } ,
	{ "bibid"          , Token::bibItem               } ,
	{ "keylist"        , Token::bibItem               } ,
	{ "cols"           , Token::colDef                } ,
	{ "preamble"       , Token::colDef                } ,
	{ "color"          , Token::color                 } ,
	{ "width"          , Token::width                 } ,
	{ "length"         , Token::width                 } ,
	{ "height"         , Token::width                 } ,
	{ "title"          , Token::title                 } ,
	{ "file"           , Token::file                  } ,
	{ "text"           , Token::text                  } ,
	{ "command"        , Token::def                   } ,
	{ "cmd"            , Token::def                   } ,
};


Token::TokenType tokenTypeFromCwlArg(QString arg,QString definition){
	
	int i = arg.indexOf('%');
	
	// type from suffix

	if(i >= 0){

		const auto suffix = arg.mid(i);

		if(multiTokens.contains(suffix))
			return multiTokens.at(suffix);

		if(suffix == "%special"){

			arg.chop(8);

			auto parser = LatexParser::getInstancePtr();

			if(parser){

				auto arguments = parser -> mapSpecialArgs;

				if(!arguments.values().contains("%" + arg)){
					const int count = arguments.count();
					arguments.insert(count,"%" + arg);
					return Token::TokenType(Token::specialArg + count);
				}
			}

			return Token::specialArg;
		}

		if(suffix == "%envname" && definition.contains('N'))
			return Token::newTheorem;
	}


	if(simpleTokens.contains(arg))
		return simpleTokens.at(arg);

	if(arg == "key" && definition.contains('l'))
		return Token::label;
	
	if(arg == "envname" && definition.contains('N'))
		return Token::newTheorem;
		
	if(arg == "environment name" && definition.contains('N'))
		return Token::newTheorem;

	if(arg == "label" && definition.contains('r'))
		return Token::labelRef;
		
	if(arg == "%<label%>" && definition.contains('r'))
		return Token::labelRef; 

	if(arg == "label" && definition.contains('l'))
		return Token::label;
	
	if(arg == "%<label%>" && definition.contains('l'))
		return Token::label;
	
	if(arg.contains("overlay specification"))
		return Token::overlay;

	if(arg.contains("URL"))
		return Token::url;

	if(arg.contains("keys"))
		return Token::keyValArg;

	return Token::generalArg;
}


/*!
\brief extract command defintion from cwl line

\a line contains a command with arguments.
The argument names have special meanings which are recognized by the function and used accordingly.

argument name | description
----------|----------
\em text or ends with \em \%text| The spellchecker will operate inside this argument (by default arguments are not spellchecked).
\em title or <em> short title</em>| The spellchecker will operate inside this argument (by default arguments are not spellchecked). Furthermore the argument will be set in bold text (like in section)
\em bibid or \em keylists| If used in a command classified as "C". See the classifier description below.
\em cmd,\em command or ends with \em \%cmd| defintion for command, e.g. \\newcommand{cmd}. This "cmd" will considered to have no arguments and convey no functionality.
\em def or \em definition| actual defintion for command, e.g. \\newcommand{cmd}{definition}. This "definition" will ignored for syntax check.
\em args| number of arguments for command, e.g. \\newcommand{cmd}[args]{definition}.
\em package|package name, e.g. \\usepackage{package}
\em citekey|definition of new citation key name, e.g. \\bibitem{citekey}
\em title or <em> short title</em>|section name, e.g. \\section{title}
\em color|color name, e.g. \\textcolor{color}
\em width,\em length,\em height or ends with \em \%l|width or length option e.g. \\abc{size\%l}
\em cols or \em preamble|columns defintion in tabular,etc. , e.g. \\begin{tabular}{cols}
\em file|file name
\em URL|URL
\em options|package options, e.g. \\usepackage[options]
\em imagefile|file name of an image
\em key|label/ref key
\em label with option #r or key ending with \em \%ref|ref key
\em label with option #l or key ending with \em \%labeldef|defines a label
\em labellist|list of labels as employed by cleveref
<em>bib file</em> or <em>bib files</em>|bibliography file
\em class|document class
\em placement or \em position|position of env
\em beamertheme|beamer theme, e.g. \\usebeamertheme{beamertheme}
\em keys,\em keyvals or \em \%<options\%>|key/value list
\em envname|environment name for \\newtheorem, e.g. \\newtheorem{envname}#N (classification N needs to be present !)
\em ends with %plain|ignore a special meaning of the key
\em contains "overlay specification" | beamer overlay specification, usually within <>

 * \param line command definition until '#'
 * \param definition context information right of '#'
 * \return command definition
 */


const QRegExp rxCom("^(\\\\\\w+\\*?)");

const QString 
		specialChars = "{[(<",
		specialChars2 = "}])>";

CommandDescription extractCommandDef(QString line,QString definition){

	int i = rxCom.indexIn(line);
	
	QString command = rxCom.cap();
	
	line = line.mid(command.length());

	if(line.isEmpty())
		return CommandDescription();
	
	auto c = line.at(0);
	int loop = 1;

	CommandDescription description;


	while(specialChars.contains(c)){
		
		int j = specialChars.indexOf(c);

		QChar closingChar = specialChars2.at(j);
		
		i = line.indexOf(closingChar);
		
		QString arg = line.mid(1, i - 1);
		
		// assume that unknown argument is not a text

		auto type = Token::generalArg;
		
		if(loop == 1 && command == "\\begin"){
			type = Token::beginEnv;
		} else
		if(loop == 1 && command == "\\end"){
			type = Token::env;
		} else {
			type = tokenTypeFromCwlArg(arg,definition);
		}

		// ignore empty arguments

		if(!arg.isEmpty()){
			switch(j){
			case 0:
				description.args = description.args + 1;
				description.argTypes.append(type);
				break;
			case 1:
				description.optionalArgs = description.optionalArgs + 1;
				description.optTypes.append(type);
				break;
			case 2:
				description.bracketArgs = description.bracketArgs + 1;
				description.bracketTypes.append(type);
				break;
			case 3:
				description.overlayArgs = description.overlayArgs + 1;
				description.overlayTypes.append(type);
				break;
			default:
				break;
			}
		}

		if(i < 0)
			break;
		
		line = line.mid(i + 1);
		
		if(line.isEmpty())
			break;
		
		c = line.at(0);
		
		loop++;
	}

	return description;
}


CommandDescription extractCommandDefKeyVal(QString line,QString & key){

	int i = line.indexOf("#");
	
	if(i < 0)
		return CommandDescription();
	
	key = line.left(i);
	auto values = line.mid(i + 1);

	
	CommandDescription description;

	if(values == "#L"){
		description.args = 1;
		description.argTypes << Token::width;
	}
	
	if(values == "#l"){
		description.args = 1;
		description.argTypes << Token::label;
	}
	
	return description;
}
