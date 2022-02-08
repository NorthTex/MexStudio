#include "syntaxcheck.h"
#include "SpellerUtility.hpp"
#include "tablemanipulation.h"
#include "latexparser/latexparsing.h"

#include "Latex/Document.hpp"
#include "Latex/EditorViewConfig.hpp"


/*! \class SyntaxCheck
*
*	\brief Asynchrnous thread which checks latex syntax of the text lines
*	
*	It gets the linehandle via a queue, together with a ticket number.
*	The ticket number is increased with every change of the text 
*	of a line, thus it can be determined of the processed handle
*	is still unchanged and can be discarded otherwise.
*	
*	Syntaxinformation are stated via markers on the text.
*	
*	Furthermore environment information, especially
*	tabular information are stored in 'cookies' as
*	they are needed in subsequent lines.
*
*/

/*!
*	\param parent
*/

SyntaxCheck::SyntaxCheck(QObject * parent)
	: SafeThread(parent)
	, mSyntaxChecking(true)
	, syntaxErrorFormat(-1)
	, ltxCommands(nullptr)
	, newLtxCommandsAvailable(false)
	, speller(nullptr)
	, newSpeller(nullptr) {

	mLinesLock.lock();

	stopped = false;

	mLines.clear();
	mLinesEnqueuedCounter.fetchAndStoreOrdered(0);
	mLinesLock.unlock();
}


/*!
*	\brief Set the format for syntax errors.
*	\param format
*/

void SyntaxCheck::setErrFormat(int format){
	syntaxErrorFormat = format;
}


/*!
*	\brief Add line to queue
*	\param dlh linehandle
*	\param previous linehandle of previous line
*	\param stack tokenstack at line start (for handling open arguments of previous commands)
*	\param clearOverlay clear syntaxcheck overlay
*/

void SyntaxCheck::putLine(
	QDocumentLineHandle * lineHandle,
	StackEnvironment previous,
	TokenStack stack,
	bool clearOverlay,
	int hint
){
	REQUIRE(lineHandle);

	SyntaxLine newLine;

	// Impede deletion of handle while in syntax check queue

	lineHandle -> ref();
	lineHandle -> lockForRead();

	newLine.ticket = lineHandle -> getCurrentTicket();

	lineHandle -> unlock();

	newLine.stack = stack;
	newLine.dlh = lineHandle;
	newLine.prevEnv = previous;
	newLine.clearOverlay = clearOverlay;
    newLine.hint = hint;

	mLinesLock.lock();

	mLines.enqueue(newLine);
	mLinesEnqueuedCounter.ref();

	mLinesLock.unlock();

	// Avoid reading of any results before this execution is stopped
	// mResultLock.lock(); not possible under windows

	mLinesAvailable.release();
}


/*!
*	\brief Stop processing syntax checks
*/

void SyntaxCheck::stop(){
	stopped = true;
	mLinesAvailable.release();
}


/*!
*	\brief Actual thread loop
*/

void SyntaxCheck::run(){

	ltxCommands = new LatexParser();

	forever {

		// Wait for enqueued lines

		mLinesAvailable.acquire();

		if(stopped)
			break;

		if(newLtxCommandsAvailable) {

			mLtxCommandLock.lock();

			if(newLtxCommandsAvailable){
				newLtxCommandsAvailable = false;
				* ltxCommands = newLtxCommands;
                speller = newSpeller;
                mReplacementList = newReplacementList;
                mFormatList = newFormatList;
			}

			mLtxCommandLock.unlock();
		}

		// get Linedata

		mLinesLock.lock();
		SyntaxLine newLine = mLines.dequeue();
		mLinesLock.unlock();

		// do syntax check

		newLine.dlh -> lockForRead();
		QString line = newLine.dlh -> text();

		if(newLine.dlh -> hasCookie(QDocumentLine::UNCLOSED_ENVIRONMENT_COOKIE)){
			newLine.dlh -> unlock();
			newLine.dlh -> lockForWrite();
			newLine.dlh -> removeCookie(QDocumentLine::UNCLOSED_ENVIRONMENT_COOKIE);
			//remove possible errors from unclosed envs
		}

		TokenList tokens = newLine.dlh
			-> getCookie(QDocumentLine::LEXER_COOKIE)
			.  value<TokenList>();

		auto commentStart = newLine.dlh
			-> getCookie(QDocumentLine::LEXER_COMMENTSTART_COOKIE)
			.  value<QPair<int,int>>();

		newLine.dlh -> unlock();

		StackEnvironment activeEnv = newLine.prevEnv;
		Ranges newRanges;

		checkLine(line,newRanges,activeEnv,newLine.dlh,tokens,newLine.stack,newLine.ticket,commentStart.first);

		// place results

        if(newLine.clearOverlay){
            
			QList<int> fmtList = {
				syntaxErrorFormat,
				SpellerUtility::spellcheckErrorFormat
			};

            fmtList.append(mFormatList.values());
            newLine.dlh -> clearOverlays(fmtList);
        }

		newLine.dlh -> lockForWrite();

		// discard results if text has been changed meanwhile

		if(newLine.ticket == newLine.dlh -> getCurrentTicket()){

		    newLine.dlh -> setCookie(QDocumentLine::LEXER_COOKIE,QVariant::fromValue<TokenList>(tokens));

			for(const auto & error : newRanges){

                // skip all syntax errors

                if(
					!mSyntaxChecking && 
					error.type != ERR_spelling && 
					error.type != ERR_highlight
				)	continue;

                int format = (error.type == ERR_spelling)
					? SpellerUtility::spellcheckErrorFormat
					: syntaxErrorFormat;

				if(error.type == ERR_highlight)
					format = error.format;

                newLine.dlh -> addOverlayNoLock(QFormatRange(error.range.first,error.range.second,format));
            }

            // add comment hightlight if present

            if(commentStart.first >= 0)
                newLine.dlh -> addOverlayNoLock(QFormatRange(commentStart.first,newLine.dlh -> length() - commentStart.first,mFormatList["comment"]));

			// active envs

			auto oldEnvVar = newLine.dlh -> getCookie(QDocumentLine::STACK_ENVIRONMENT_COOKIE);
			StackEnvironment oldEnv;

			if(oldEnvVar.isValid())
				oldEnv = oldEnvVar.value<StackEnvironment>();

			bool cookieChanged = ! equalEnvStack(oldEnv,activeEnv);

			//if excessCols has changed the subsequent lines need to be rechecked.
            // don't on initial check

			if(cookieChanged){

				QVariant env;
				env.setValue(activeEnv);

				newLine.dlh -> setCookie(QDocumentLine::STACK_ENVIRONMENT_COOKIE,env);
				newLine.dlh -> ref(); // avoid being deleted while in queue

                emit checkNextLine(newLine.dlh,true,newLine.ticket,newLine.hint);
            }
		}

		newLine.dlh -> unlock();
		newLine.dlh -> deref();
	}

	delete ltxCommands;
	ltxCommands = nullptr;
}


/*!
*	\brief Get error description for syntax error in line 'dlh' at column 'pos'
*	\param dlh linehandle
*	\param pos column
*	\param previous environment stack at start of line
*	\param stack tokenstack at start of line
*	\return error description
*/

QString SyntaxCheck::getErrorAt(
	QDocumentLineHandle * dlh,
	int pos,
	StackEnvironment previous,
	TokenStack stack
){
	// do syntax check
	QString line = dlh -> text();
	QStack<Environment> activeEnv = previous;

	TokenList tokens = dlh
		-> getCookieLocked(QDocumentLine::LEXER_COOKIE)
		.  value<TokenList>();

	auto commentStart = dlh
		-> getCookieLocked(QDocumentLine::LEXER_COMMENTSTART_COOKIE)
		.  value<QPair<int,int>>();

	Ranges newRanges;

	checkLine(line,newRanges,activeEnv,dlh,tokens,stack,dlh -> getCurrentTicket(),commentStart.first);

	// add Error for unclosed env

	auto var = dlh -> getCookieLocked(QDocumentLine::UNCLOSED_ENVIRONMENT_COOKIE);

	if(var.isValid()){

		activeEnv = var.value<StackEnvironment>();

		Q_ASSERT_X(activeEnv.size() == 1,"SyntaxCheck","Cookie error");

		Environment env = activeEnv.top();

		QString command = "\\begin{" + env.name + "}";
		int index = line.lastIndexOf(command);

		if(index >= 0){
			Error elem;
			elem.range = QPair<int,int>(index,command.length());
			elem.type = ERR_EnvNotClosed;
			newRanges.append(elem);
		}
	}

	// find Error at Position

	ErrorType result = ERR_none;

	for(const auto & error : newRanges){

		const auto range = error.range;

		if(range.second + range.first < pos)
			continue;

		if(range.first > pos)
			break;

		result = error.type;
	}

	// filter out accidental highlight detection (test only)

    if(result == ERR_highlight)
        result = ERR_none;

	// now generate Error message
	// indices have to match ErrorType
	// mock message for arbitrary highlight. Will not be shown.

	const auto messages = QStringList()
		<< tr("no error")
		<< tr("unrecognized environment")
		<< tr("unrecognized command")
		<< tr("unrecognized math command")
		<< tr("unrecognized tabular command")
		<< tr("tabular command outside tabular env")
		<< tr("math command outside math env")
		<< tr("tabbing command outside tabbing env")
		<< tr("more cols in tabular than specified")
		<< tr("cols in tabular missing")
		<< tr("\\\\ missing")
		<< tr("closing environment which has not been opened")
		<< tr("environment not closed")
		<< tr("unrecognized key in key option")
		<< tr("unrecognized value in key option")
		<< tr("command outside suitable env")
		<< tr("spelling")
		<< "highlight";

	Q_ASSERT(messages.length() == ERR_MAX);

	return messages.value(int(result),tr("unknown"));
}


/*!
*	\brief Set latex commands which are referenced for syntax checking
*	\param cmds
*/

void SyntaxCheck::setLtxCommands(const LatexParser & commands){

	if(stopped)
		return;

	mLtxCommandLock.lock();
	newLtxCommandsAvailable = true;
	newLtxCommands = commands;
    mLtxCommandLock.unlock();
}


/*!
*	\brief set new spellchecker engine (language)
*	\param su new spell checker
*/

void SyntaxCheck::setSpeller(SpellerUtility * speller){

	if(stopped)
		return;

	mLtxCommandLock.lock();
    newLtxCommandsAvailable = true;
    newSpeller = speller;
    mLtxCommandLock.unlock();
}


/*!
 *	\brief enable showing of Syntax errors
 *	
 *	Since the syntax checker is also used for 
 *	asynchronous syntax highligting/spell 
 *	checking, it will not be disabled any more.
 *
 *	Only syntax error will not be shown any more.
 *	
 * \param enable
 */

void SyntaxCheck::enableSyntaxCheck(const bool enable){

	if(stopped)
		return;

	mSyntaxChecking=enable;
}


/*!
 * \brief set character/text replacementList for spell checking
 * \param replacementList Map for characater/text replacement prior to spellchecking words. E.g. "u -> ü when german is activated
 */

void SyntaxCheck::setReplacementList(QMap<QString,QString> replacements){

	if(stopped)
		return;

	mLtxCommandLock.lock();
    newLtxCommandsAvailable = true;
    newReplacementList = replacements;
    mLtxCommandLock.unlock();
}


void SyntaxCheck::setFormats(QMap<QString,int> formats){

	if(stopped)
		return;

    mLtxCommandLock.lock();
    newLtxCommandsAvailable = true;
    newFormatList = formats;
    mLtxCommandLock.unlock();
}


#ifndef NO_TESTS


/*!
*	\brief Wait for syntax checker to finish processing.
*	\details Wait for syntax checker to finish processing. 
*	This method should be used only in self-tests because
*	in some rare cases it could return too early before 
*	the syntax checker queue is fully processsed.
*/

void SyntaxCheck::waitForQueueProcess(void){

	int linesBefore , linesAfter;

	/*
	 * The logic in the following loop is not perfect because it could terminate the loop too early if it takes more
	 * than 10ms between the call to mLinesAvailable.acquire() and the following call to mLinesAvailable.release().
	 * Implementing the check properly requires bi-directional communication with the worker thread with commands to
	 * pause/unpause the worker thread which complicates the code too much just to handle testing.
	 */

	linesBefore = mLinesEnqueuedCounter.fetchAndAddOrdered(0);

	while(true){

		for(int i = 0;i < 2;++i){

			// Process queued checkNextLine events
			QCoreApplication::processEvents(QEventLoop::AllEvents,1000);

			// Deferred delete must be processed explicitly. Using 0 for event_type does not work.
			QCoreApplication::sendPostedEvents(Q_NULLPTR,QEvent::DeferredDelete);

			// Give the checkNextLine signal handler time to queue the next line
			wait(5);
		}

		linesAfter = mLinesEnqueuedCounter.fetchAndAddOrdered(0);

		if((linesBefore == linesAfter) && !mLinesAvailable.available())
			break;

		linesBefore = linesAfter;
	}
}

#endif


/*!
*	\brief check if top-most environment in 'envs' is `name`
*	\param name environment name which is checked
*	\param envs stack of environments
*	\param id check for `id` of the environment, <0 means check is disabled
*	\return environment id or 0
*/

int SyntaxCheck::topEnv(
	const QString & name,
	const StackEnvironment & environments,
	const int id
){
	if(environments.isEmpty())
		return 0;

	auto environment = environments.top();

	if(environment.name == name)
		if (id < 0 || environment.id == id)
			return environment.id;

	const auto aliases = ltxCommands -> environmentAliases;

	if(id >= 0)
		return 0;

	if(!aliases.contains(environment.name))
		return 0;

	QStringList altEnvs = aliases.values(environment.name);

	if(altEnvs.contains(name))
		return environment.id;

	return 0;
}


/*!
*	\brief check if the environment stack contains a environment with name `name`
*	\param parser reference to LatexParser. It is used to access environment aliases, e.g. equation is also a math environment
*	\param name name of the checked environment
*	\param envs stack of environements
*	\param id if >=0 check if the env has the given id.
*	\return environment id of  found env otherwise 0
*/

int SyntaxCheck::containsEnv(
	const LatexParser & parser,
	const QString & name,
	const StackEnvironment & environments,
	const int id
){
	const auto aliases = parser.environmentAliases;

	for(int i = environments.size() - 1;i > -1;--i){

		const auto environment = environments.at(i);
		const auto envname = environment.name;
		const auto envid = environment.id;


		if(envname == name){

			if(id < 0)
				return envid;

			if(envid == id)
				return envid;
		}

		if(id >= 0)
			continue;

		if(aliases.contains(envname)){

			auto alternatives = aliases.values(envname);

			for(const auto & alternative : alternatives)
				if(alternative == name)
					return envid;
		}
	}

	return 0;
}


/*!
*	\brief check if the command is valid in the environment stack
*	\param cmd name of command
*	\param envs environment stack
*	\return is valid
*/

bool SyntaxCheck::checkCommand(const QString & command,const StackEnvironment & enviroments){

	const auto commands = ltxCommands -> possibleCommands;
	const auto aliases = ltxCommands -> environmentAliases;

	for(const auto environment : enviroments){

		const auto name = environment.name;

		if(
			commands.contains(name) &&
			commands.value(name).contains(command)
		)	return true;

		if(aliases.contains(name))
			for(const auto & alternative : aliases.values(name))
				if(
					commands.contains(alternative) &&
					commands.value(alternative).contains(command)
				)	return true;
	}

	return false;
}


/*!
*	\brief compare two environment stacks
*	\param env1
*	\param env2
*	\return are equal
*/

bool SyntaxCheck::equalEnvStack(StackEnvironment a,StackEnvironment b){

	if(a.isEmpty() || b.isEmpty())
		return a.isEmpty() && b.isEmpty();

	if(a.size() != b.size())
		return false;

	for(int i = 0; i < a.size();i++)
		if(a.value(i) != b.value(i))
			return false;

	return true;
}


/*!
* \brief mark environment start
*
* This function is used to mark unclosed environment,
* i.e. environments which are unclosed at the end of the text
* \param env used environment
*/

void SyntaxCheck::markUnclosedEnv(Environment environment){

	auto handle = environment.dlh;

	if(!handle)
		return;

	handle -> lockForWrite();

	if(handle -> getCurrentTicket() == environment.ticket){

		QString line = handle -> text();
		line = ltxCommands -> cutComment(line);

		QString command = "\\begin{" + environment.name + "}";

		int index = line.lastIndexOf(command);

		if(index >= 0){

			Error error;
			error.range = QPair<int,int>(index,command.length());
			error.type = ERR_EnvNotClosed;

		    int format = (error.type == ERR_spelling)
				? SpellerUtility::spellcheckErrorFormat
				: syntaxErrorFormat;

			if(error.type == ERR_highlight)
				format = error.format;

			handle -> addOverlayNoLock(QFormatRange(error.range.first,error.range.second,format));

			QVariant var_env;
			StackEnvironment activeEnv;

			activeEnv.append(environment);
			var_env.setValue(activeEnv);

			// ERR_EnvNotClosed;

			handle -> setCookie(QDocumentLine::UNCLOSED_ENVIRONMENT_COOKIE,var_env); 
		}
	}

	handle -> unlock();
}


/*!
*	\brief Check if the tokenstack contains a definition-token
*	\param stack tokenstack
*	\return contains a definition
*/

#include <algorithm>

bool SyntaxCheck::stackContainsDefinition(const TokenStack & stack) const {
	return std::any_of(stack.begin(),stack.end(),[](auto & token){
		return token.subtype == Token::definition;
	});
}


/*!
*	\brief check one line
*
*	Checks one line. Context information needs to be given by newRanges,activeEnv,dlh and ticket.
*	This method is obsolete as the new system relies on tokens.
*	
*	\param line text of line as string
*	\param newRanges will return the result as ranges
*	\param activeEnv environment context
*	\param dlh linehandle
*	\param tl tokenlist of line
*	\param stack token stack at start of line
*	\param ticket ticket number for current processed line
*/

void SyntaxCheck::checkLine(
	const QString & line,
	Ranges & newRanges,
	StackEnvironment & activeEnv,
	QDocumentLineHandle * dlh,
	TokenList & tokens,
	TokenStack stack,
	int ticket,
	int commentStart
){
    // special treatment for empty lines with $/$$ math environmens
    // latex treats them as error, so do we

    if(
		tokens.length() == 0 && 
		line.simplified().isEmpty() && 
		! activeEnv.isEmpty() && 
		activeEnv.top().name == "math"
	)
        if(activeEnv.top().origName == "$" || activeEnv.top().origName == "$$")
            Environment env = activeEnv.pop();

    // check command-words

	for(int i = 0;i < tokens.length();i++){

		auto & token = tokens[i];

		const auto error = [ & ](auto type,int format = 0){
			newRanges.append(Error {
				{ token.start , token.length },
				type , format
			});
		};

		// ignore commands in definition arguments e.g. \newcommand{cmd}{definition}

		if(stackContainsDefinition(stack)){

			auto top = stack.top();

			if(top.dlh == token.dlh)
				if(token.start < top.start + top.length)
					continue;
			else
				if(token.type != Token::closeBrace)
					continue;

			stack.pop();
		}


		// don't check command definitions

		if(token.subtype == Token::definition){

			switch(token.type){
			case Token::braces:
			case Token::openBrace:
				stack.push(token);
			}

			continue;
		}


		// don't check command definitions

        if(token.type == Token::verbatim){

	        // highlight

			error(ERR_highlight,mFormatList["verbatim"]);

            continue;
        }

		if(
			token.type == Token::punctuation || 
			token.type == Token::symbol
		){

			QString word = line.mid(token.start,token.length);

			auto forbiddenSymbols = QStringList()
				<< "^"
				<< "_";

            if(
				forbiddenSymbols.contains(word) && 
				! containsEnv(* ltxCommands,"math",activeEnv) && 
				token.subtype != Token::formula
			) error(ERR_MathCommandOutsideMath);
		}


        // math highlighting of formula

        if(token.subtype == Token::formula){

            // highlight

			const auto format = (token.type == Token::command)
				? "#math" 
				: "math";

			error(ERR_highlight,mFormatList[format]);

            // newRanges.append(error);
        }


        // spell checking

        if(speller -> inlineSpellChecking && token.type == Token::word && (
			token.subtype == Token::text ||
			token.subtype == Token::title ||
			token.subtype == Token::shorttitle ||
			token.subtype == Token::todo ||
			token.subtype == Token::none
			)  && token.length >= 3 && speller
		){
            int tkLength = token.length;
            QString word = token.getText();

            if(i + 1 < tokens.length()){

				//check if next token is . or -

				Token tk1 = tokens.at(i + 1);

				if(
					tk1.type == Token::punctuation && 
					tk1.start == (token.start + token.length) && 
					! word.endsWith('"')
				){
					QString add = tk1.getText();

					if(add == '.' || add == '-'){
                        word += add;
                        i++;
                        tkLength += tk1.length;
                    }

                    if(add == "'" && i + 2 < tokens.length()){

                        Token tk2 = tokens.at(i + 2);

                        if(tk2.type == Token::word && tk2.start == (tk1.start + tk1.length)){
                            add += tk2.getText();
                            word += add;
                            i += 2;
                            tkLength += tk1.length + tk2.length;
                        }
                    }
                }
            }

			//	Remove special chars

            word = latexToPlainWordwithReplacementList(word,mReplacementList);

            if(speller -> hideNonTextSpellingErrors && (
				containsEnv(* ltxCommands,"math",activeEnv) ||
				containsEnv(* ltxCommands,"picture",activeEnv)
				) && token.subtype != Token::text
			){
                word.clear();
                token.ignoreSpelling = true;
            } else {

			    token.ignoreSpelling = false;

			    if(containsEnv(* ltxCommands,"math",activeEnv)){

                    // in math env, highlight as math-text !

					error(ERR_highlight,mFormatList["#mathText"]);
                }
            }

            if(!word.isEmpty() && !speller->check(word)){

				// word ended with '-', without that letter, word is correct (e.g. set-up / german hypehantion)

                if(word.endsWith('-') && speller -> check(word.left(word.length() - 1)))
                    continue;

				// don't take point into misspelled word

				if(word.endsWith('.'))
                    tkLength--;

				error(ERR_spelling);
            }
        }

		if(token.type == Token::commandUnknown){

			QString word = line.mid(token.start,token.length);

			// ignore commands containg @

			if(word.contains('@'))
				continue;

			if(ltxCommands -> mathStartCommands.contains(word) && (
				activeEnv.isEmpty() ||
				activeEnv.top().name != "math")
			){
				Environment environment;
				environment.name = "math";
				environment.origName = word;
				environment.id = 1; // to be changed
				environment.dlh = dlh;
				environment.ticket = ticket;
				environment.level = token.level;
                environment.startingColumn = token.start + token.length;
				activeEnv.push(environment);

                // highlight delimiter

				error(ERR_highlight,mFormatList["&math"]);

                continue;
			}

			if(
				ltxCommands -> mathStopCommands.contains(word) &&
				! activeEnv.isEmpty() &&
				activeEnv.top().name == "math"
			){
				int i = ltxCommands -> mathStopCommands.indexOf(word);
				QString txt = ltxCommands -> mathStartCommands.value(i);

				if(activeEnv.top().origName == txt){

                    Environment environment = activeEnv.pop();

					{
						Error error;
						error.type = ERR_highlight;
						error.format = mFormatList["math"];
						error.range = (dlh == environment.dlh)
							? QPair<int,int>(environment.startingColumn,token.start - environment.startingColumn)
							: QPair<int,int>(0,token.start);

						newRanges.prepend(error);
					}

                    // highlight delimiter

					error(ERR_highlight,mFormatList["&math"]);
				}

				// ignore mismatching mathstop commands

				continue;
			}

			if(
				word == "\\\\" &&
				topEnv("tabular",activeEnv) != 0 &&
				token.level == activeEnv.top().level
			){
				if(activeEnv.top().excessCol < (activeEnv.top().id - 1))
					error(ERR_tooLittleCols);

				if(activeEnv.top().excessCol >= (activeEnv.top().id))
					error(ERR_tooManyCols);

				activeEnv.top().excessCol = 0;

				continue;
			}

            // command highlighing
            // this looks slow
            // TODO: optimize !

			for(const auto & environment : activeEnv){

				// ignore "normal" env

                if(!environment.dlh)
                    continue;

				// ignore "document" env

				if(environment.name == "document")
                    continue;

				for(const auto & format : mFormatList.keys())
                    if(format.at(0) == '#'){

                        auto aliases = ltxCommands -> environmentAliases.values(environment.name);
                        aliases << environment.name;

						if(aliases.contains(format.mid(1)))
							error(ERR_highlight,mFormatList.value(format));
                    }
            }

			if(ltxCommands -> possibleCommands["user"].contains(word))
				continue;

			if(ltxCommands -> customCommands.contains(word))
				continue;

			if(!checkCommand(word,activeEnv)){
				error(ERR_unrecognizedCommand);
				continue;
			}
		}

		if(token.type == Token::env){

			QString env = line.mid(token.start,token.length);

			// corresponds \end{env}

			if(!activeEnv.isEmpty()){

				Environment tp = activeEnv.top();
			
				if(tp.name == env){

                    Environment closingEnv = activeEnv.pop();
					
					if(tp.name == "tabular" || ltxCommands -> environmentAliases.values(tp.name).contains("tabular")){

						// correct length of col error if it exists
						
						if(!newRanges.isEmpty()){
							
							auto & error = newRanges.last();

							if(error.type == ERR_tooManyCols && error.range.first + error.range.second > token.start)
								error.range.second = token.start - error.range.first;
						}

						// get new cols
                        //cols = containsEnv(*ltxCommands, "tabular", activeEnv);
					}

                    // handle higlighting
                    
					auto altEnvs = ltxCommands -> environmentAliases.values(env);
                    
					altEnvs << env;
                    
					for(const auto & key : mFormatList.keys()){

                        if(altEnvs.contains(key)){
                        
						    Error elem;
                        
						    int start = (closingEnv.dlh == dlh) ? closingEnv.startingColumn : 0;
                            int end = token.start - 1;
                        
						    if(i > 1){
                                auto token = tokens.at(i - 2);

                                if(token.type == Token::command && line.mid(token.start,token.length) == "\\end")
                                    end = token.start;
                            }

                            // trick to avoid coloring of end
                            
							if(!newRanges.isEmpty() && newRanges.last().type == ERR_highlight)
                                if(i > 1){

									// skip over brace

                                    Token token = tokens.at(i - 2); 

                                    if(token.type == Token::command && line.mid(token.start,token.length) == "\\end"){

                                        //previous token is end
                                        // see whether it was colored with *-keyword i.e. #math or #picture
                                        
                                        // yes, remove !

                                        if(newRanges.last().range == QPair<int,int>(token.start,token.length))
                                            newRanges.removeLast();
                                    }
                                }

                            elem.range = QPair<int,int>(start,end);
                            elem.type = ERR_highlight;
                            elem.format = mFormatList.value(key);
                            newRanges.append(elem);
                        }
                    }
				} else {
					error(ERR_closingUnopendEnv);
				}
			} else {
				error(ERR_closingUnopendEnv);
			}
		}

		if(token.type == Token::beginEnv){

			auto name = line.mid(token.start, token.length);
			
			// corresponds \begin{env}
			Environment environment;
			environment.name = name;
			environment.id = 1; //needs correction
			environment.excessCol = 0;
			environment.dlh = dlh;
			environment.startingColumn = token.start + token.length + 1; // after closing brace
			environment.ticket = ticket;
			environment.level = token.level - 1; // tk is the argument, not the command, hence -1
			
			if(
				name == "tabular" || 
				ltxCommands -> environmentAliases.values(name).contains("tabular")
			){
				// tabular env opened
				// get cols !!!!
				QString option;
			
				// special treatment as the env is rather not latex standard

				if(name == "tabu" || name == "longtabu"){ 
					for(int k = i + 1;k < tokens.length();k++){

						Token elem = tokens.at(k);
						
						if(elem.level < token.level - 1)
							break;
						
						if(elem.level > token.level)
							continue;
						
						// take the first mandatory argument at the correct level -> TODO: put colDef also for tabu correctly in lexer

						if(elem.type == Token::braces){ 

							// strip {}
							option = line.mid(elem.start + 1,elem.length - 2);

							// first argument only !
							break;
						}
					}
				} else {
					if(name == "tikztimingtable"){
						// is always 2 columns
						option = "ll"; 
					} else {
						for(int k = i + 1;k < tokens.length();k++){

							Token elem = tokens.at(k);
							
							if(elem.level < token.level)
								break;
							
							if(elem.level > token.level)
								continue;
							
							if(elem.subtype == Token::colDef) {
								// strip {}
								option = line.mid(elem.start + 1, elem.length - 2);
								break;
							}
						}
					}
				}

				auto translationMap = ltxCommands -> possibleCommands.value("%columntypes");
				QStringList res = LatexTables::splitColDef(option);
				QStringList res2;

                for(const auto & elem : res){
					
					bool add = true;
                    
					for(const auto & translation : translationMap)
						if(translation.left(1) == elem && add){
							res2 << LatexTables::splitColDef(translation.mid(1));
							add = false;
						}

					if(add)
						res2 << elem;
				}

                int cols = res2.count();
                environment.id = cols;
			}

			activeEnv.push(environment);
		}


		if(token.type == Token::command){

			QString word = line.mid(token.start, token.length);
			
			if(!token.optionalCommandName.isEmpty())
				word = token.optionalCommandName;

			Token tkEnvName;

			if(
				word == "\\begin" || 
				word == "\\end"
			){
				// check complete expression e.g. \begin{something}
				
				if(tokens.length() > i + 1 && tokens.at(i + 1).type == Token::braces){
					tkEnvName = tokens.at(i + 1);
					word = word + line.mid(tkEnvName.start, tkEnvName.length);
				}
			}

            // special treatment for & in math
            
			if(word == "&" && containsEnv(* ltxCommands,"math",activeEnv)){
				error(ERR_highlight,mFormatList.value("align-ampersand"));
                continue;
            }

			if(
				ltxCommands -> mathStartCommands.contains(word) && 
				(activeEnv.isEmpty() || activeEnv.top().name != "math")
			){
				Environment env;
				env.name = "math";
				env.origName = word;
				env.id = 1; // to be changed
				env.dlh = dlh;
				env.ticket = ticket;
				env.level = token.level;
				env.startingColumn = token.start + token.length;
				activeEnv.push(env);

                // highlight delimiter
                
				error(ERR_highlight,mFormatList["&math"]);

				continue;
			}

			if(
				ltxCommands -> mathStopCommands.contains(word) && 
				! activeEnv.isEmpty() && 
				activeEnv.top().name == "math"
			){
				int i = ltxCommands -> mathStopCommands.indexOf(word);
				QString txt = ltxCommands -> mathStartCommands.value(i);

				if(activeEnv.top().origName == txt){

                    Environment env = activeEnv.pop();

                    Error elem;
                    elem.type = ERR_highlight;
                    elem.format=mFormatList["math"];
					elem.range = (dlh == env.dlh)
						? QPair<int,int>(env.startingColumn,token.start - env.startingColumn)
						: QPair<int,int>(0,token.start);

                    newRanges.prepend(elem);

                    // highlight delimiter

					error(ERR_highlight,mFormatList["&math"]);
				}
				
				// ignore mismatching mathstop commands
				
				continue;
			}

			//tabular checking

			if(topEnv("tabular",activeEnv) != 0){
				if(word == "&"){
				
					activeEnv.top().excessCol++;
				
					if(activeEnv.top().excessCol >= activeEnv.top().id)
						error(ERR_tooManyCols);
                    else
						error(ERR_highlight,mFormatList.value("align-ampersand"));

					continue;
				}

				if(
					word == "\\\\" || 
					word == "\\tabularnewline"
				){
					if(activeEnv.top().excessCol < (activeEnv.top().id - 1))
						error(ERR_tooLittleCols);

					if(activeEnv.top().excessCol >= (activeEnv.top().id))
						error(ERR_tooManyCols);

					activeEnv.top().excessCol = 0;
					
					continue;
				}

				if(word == "\\multicolumn"){

					QRegExp rxMultiColumn("\\\\multicolumn\\{(\\d+)\\}\\{.+\\}\\{.+\\}");
					rxMultiColumn.setMinimal(true);
					
					int res = rxMultiColumn.indexIn(line, token.start);
					
					if(res > -1){

						// multicoulmn before &
						
						bool ok;
						
						int c = rxMultiColumn.cap(1).toInt(&ok);
						
						if(ok)
							activeEnv.top().excessCol += c - 1;
					}

					if(activeEnv.top().excessCol >= activeEnv.top().id)
						error(ERR_tooManyCols);

					continue;
				}

			}

            // command highlighing
            // this looks slow
            // TODO: optimize !

            for(const auto & env : activeEnv){

				//ignore "normal" env

                if(!env.dlh)
                    continue; 
                
				//ignore "document" env

				if(env.name == "document")
                    continue; 
                
				for(const auto & key : mFormatList.keys())
                    if(key.at(0)=='#'){
                        
						auto altEnvs = ltxCommands -> environmentAliases.values(env.name);
                        altEnvs << env.name;
                        
						if(altEnvs.contains(key.mid(1)))
							error(ERR_highlight,mFormatList.value(key));
                    }
            }

			if(
				ltxCommands -> possibleCommands["user"].contains(word) || 
				ltxCommands -> customCommands.contains(word)
			)	continue;

			if(!checkCommand(word, activeEnv)){

				Error elem;
				
				if(tkEnvName.type == Token::braces){
					Token tkEnvName = tokens.at(i+1);
					elem.range = QPair<int, int>(tkEnvName.innerStart(), tkEnvName.innerLength());
					elem.type = ERR_unrecognizedEnvironment;
				} else {
					elem.range = QPair<int, int>(token.start, token.length);
					elem.type = ERR_unrecognizedCommand;
				}


				if(ltxCommands -> possibleCommands["math"].contains(word))
					elem.type = ERR_MathCommandOutsideMath;

				if(ltxCommands -> possibleCommands["tabular"].contains(word))
					elem.type = ERR_TabularCommandOutsideTab;
				
				if(ltxCommands -> possibleCommands["tabbing"].contains(word))
					elem.type = ERR_TabbingCommandOutside;
				
				if(elem.type== ERR_unrecognizedEnvironment){

					// try to find command in unspecified envs
					
					auto keys = ltxCommands -> possibleCommands.keys();
					keys.removeAll("math");
					keys.removeAll("tabular");
					keys.removeAll("tabbing");
					keys.removeAll("normal");

					for(auto key : keys){

						if(key.contains("%"))
							continue;
						
						if(ltxCommands -> possibleCommands[key].contains(word)){
							elem.type = ERR_commandOutsideEnv;
							break;
						}
					}
				}

				if(
					elem.type != ERR_MathCommandOutsideMath || 
					token.subtype!=Token::formula
				)	newRanges.append(elem);
			}
		}

		if(token.type == Token::specialArg){
			
			QString value = line.mid(token.start,token.length);
			QString special = ltxCommands -> mapSpecialArgs.value(int(token.type - Token::specialArg));
			
			if(!ltxCommands -> possibleCommands[special].contains(value))
				error(ERR_unrecognizedKey);
		}

		if(token.type == Token::keyVal_key){

			// special treatment for key val checking
			
			QString command = token.optionalCommandName;
			QString value = line.mid(token.start, token.length);

			// search stored keyvals

			QString elem;
            
            foreach(elem,ltxCommands -> possibleCommands.keys()){

				if(elem.startsWith("key%") && elem.mid(4) == command)
					break;
				
				if(
					elem.startsWith("key%") && 
					elem.mid(4, command.length()) == command && 
					elem.mid(4 + command.length(), 1) == "/" && 
					! elem.endsWith("#c")
				){
					// special treatment for distinguishing \command[keyvals]{test}
					// where argument needs to equal test (used in yathesis.cwl)
					// now find mandatory argument
				
					QString subcommand;
				
					for(int k = i + 1; k < tokens.length(); k++) {
						
						Token tk_elem = tokens.at(k);
						
						if(tk_elem.level > token.level)
							continue;
						
						if(tk_elem.level < token.level)
							break;
						
						if(tk_elem.type == Token::braces){
						
							subcommand = line.mid(tk_elem.start + 1, tk_elem.length - 2);
						
							if(elem == "key%" + command + "/" + subcommand)
								break;
							
							subcommand.clear();
						}
					}

					if(subcommand.isEmpty())
						elem.clear();
					else
						elem = "key%" + command + "/" + subcommand;
					
					break;
				}

				elem.clear();
			}

			if(!elem.isEmpty()){

				QStringList lst = ltxCommands -> possibleCommands[elem].values();
				QStringList::iterator iterator;
				QStringList toAppend;
				
				for(iterator = lst.begin();iterator != lst.end();++iterator){

					int i = iterator -> indexOf("#");
					
					if(i > -1)
						* iterator = iterator -> left(i);

					i = iterator -> indexOf("=");
					
					if(i > -1)
						* iterator = iterator -> left(i);
					
					if(iterator -> startsWith("%"))
						toAppend << ltxCommands -> possibleCommands[* iterator].values();
				}

				lst << toAppend;
				
				if(!lst.contains(value))
					error(ERR_unrecognizedKey);
			}
		}

		if(token.subtype == Token::keyVal_val){

			//figure out keyval
			
			QString word = line.mid(token.start,token.length);
			
			// first get command
			
			Token cmd = Parsing::getCommandTokenFromToken(tokens,token);
			QString command = line.mid(cmd.start,cmd.length);
			
			// figure out key

			QString key;
			
			for(int k = i - 1; k >= 0; k--){
			
				Token elem = tokens.at(k);
			
				if(elem.level == token.level - 1){
					
					if(elem.type == Token::keyVal_key)
						key = line.mid(elem.start, elem.length);
					
					break;
				}
			}

			// find if values are defined
			QString elem;
            
            foreach(elem,ltxCommands -> possibleCommands.keys()){

				if(elem.startsWith("key%") && elem.mid(4) == command)
					break;
				
				if(
					elem.startsWith("key%") && 
					elem.mid(4,command.length()) == command && 
					elem.mid(4 + command.length(),1) == "/" && 
					! elem.endsWith("#c")
				){
					// special treatment for distinguishing \command[keyvals]{test} 
					// where argument needs to equal test (used in yathesis.cwl)
					// now find mandatory argument

					QString subcommand;
				
					for(int k = i + 1;k < tokens.length();k++){

						Token tk_elem = tokens.at(k);
						
						if(tk_elem.level > token.level - 2)
							continue;
						
						if(tk_elem.level < token.level - 2)
							break;
						
						if(tk_elem.type == Token::braces){
						
							subcommand = line.mid(tk_elem.start + 1, tk_elem.length - 2);
						
							if(elem == "key%" + command + "/" + subcommand)
								break;

							subcommand.clear();
						}
					}

					if(!subcommand.isEmpty())
						elem = "key%" + command + "/" + subcommand;
					
					break;
				}

				elem.clear();
			}

			if(!elem.isEmpty()){

				// check whether keys is valid
				QStringList lst = ltxCommands->possibleCommands[elem].values();
				QStringList::iterator iterator;
				QString options;
				
				for(iterator = lst.begin();iterator != lst.end();++iterator){

					int i = iterator->indexOf("#");
					options.clear();
					
					if(i > -1){
						options = iterator -> mid(i + 1);
						* iterator = iterator -> left(i);
					}

					if(iterator -> endsWith("="))
						iterator -> chop(1);

					if(* iterator == key)
						break;
				}

				if(iterator != lst.end() && !options.isEmpty()){

					// ignore type keys, like width#L

					if(options.startsWith('#'))
						continue;

                    if(options.startsWith('%')){
                        if(!ltxCommands -> possibleCommands[options].contains(word))
							error(ERR_unrecognizedKeyValues);
                    } else {

                        QStringList l = options.split(',');

						if(!l.contains(word))
							error(ERR_unrecognizedKeyValues);
                    }
				}
			}
		}
	}

	if(!activeEnv.isEmpty()){

	    //check active env for env highlighting (math,verbatim)

	    for(const auto & environment : activeEnv){

            auto aliases = ltxCommands -> environmentAliases.values(environment.name);

			aliases << environment.name;

			for(const auto & format : mFormatList.keys())
				if(aliases.contains(format)){
                    Error error;

					int start = 0;

					if(environment.dlh == dlh)
						start = environment.startingColumn;

					const auto end = (commentStart) >= 0
						? commentStart - start
						: line.length() - start;

                    error.range = QPair<int,int>(start,end);
                    error.type = ERR_highlight;
                    error.format = mFormatList.value(format);
                    newRanges.prepend(error);

					// draw this first and then other on top (e.g. keyword highlighting) !
                }
        }
    }
}
