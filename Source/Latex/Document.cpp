#include "latexdocument.h"
#include "latexeditorview.h"
#include "qdocument.h"
#include "qformatscheme.h"
#include "qlanguagedefinition.h"
#include "qdocumentline.h"
#include "qdocumentline_p.h"
#include "qdocumentcursor.h"
#include "qeditor.h"
#include "latexcompleter.h"
#include "latexcompleter_config.h"
#include "configmanagerinterface.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparsing.h"
#include <QtConcurrent>



FileNamePair::FileNamePair(const QString & rel,const QString & abs)
	: relative(rel)
	, absolute(abs) {}

UserCommandPair::UserCommandPair(const QString & name,const CodeSnippet & snippet)
	: name(name)
	, snippet(snippet) {}


// languages for LaTeX syntax checking (exact name from qnfa file)

const QSet<QString> LatexDocument::LATEX_LIKE_LANGUAGES = QSet<QString>() 
	<< "(La)TeX" 
	<< "Pweave" 
	<< "Sweave" 
	<< "TeX dtx file";

	
/*! \brief constructor
 * sets up structure for structure view
 * starts the syntax checker in a separate thread
 */

LatexDocument::LatexDocument(QObject *parent)
	: QDocument(parent)
	, remeberAutoReload(false)
	, mayHaveDiffMarkers(false)
	, edView(nullptr)
	, mAppendixLine(nullptr)
	, mBeyondEnd(nullptr){
	
	magicCommentList = new StructureEntry(this,StructureEntry::SE_OVERVIEW);
	baseStructure = new StructureEntry(this,StructureEntry::SE_DOCUMENT_ROOT);
	bibTeXList = new StructureEntry(this,StructureEntry::SE_OVERVIEW);
	labelList = new StructureEntry(this,StructureEntry::SE_OVERVIEW);
	blockList = new StructureEntry(this,StructureEntry::SE_OVERVIEW);
	todoList = new StructureEntry(this,StructureEntry::SE_OVERVIEW);

	magicCommentList -> title = tr("MAGIC_COMMENTS");
	bibTeXList -> title = tr("BIBLIOGRAPHY");
	labelList -> title = tr("LABELS");
	blockList -> title = tr("BLOCKS");
	todoList -> title = tr("TODO");

	mMentionedBibTeXFiles.clear();
	mUserCommandList.clear();
	mLabelItem.clear();
	mBibItem.clear();

	masterDocument = nullptr;
	this -> parent = nullptr;

	unclosedEnv.id = -1;
	syntaxChecking = true;

	lp = LatexParser::getInstance();

	SynChecker.setLtxCommands(LatexParser::getInstance());

    updateSettings();

	SynChecker.start();

    connect(& SynChecker,
		SIGNAL(checkNextLine(QDocumentLineHandle *,bool,int,int)),
		SLOT(checkNextLine(QDocumentLineHandle*,bool,int,int)),
		Qt::QueuedConnection);
}


LatexDocument::~LatexDocument(){

	SynChecker.stop();
	SynChecker.wait();
	
	if(!magicCommentList -> parent)
		delete magicCommentList;
	
	if(!labelList -> parent)
		delete labelList;
	
	if(!todoList -> parent)
		delete todoList;
	
	if(!bibTeXList -> parent)
		delete bibTeXList;
	
	if(!blockList -> parent)
		delete blockList;

	for(auto handle : mLineSnapshot)
		handle -> deref();

	mLineSnapshot.clear();

	delete baseStructure;
}


void LatexDocument::setFileName(const QString & name){
	this -> setFileNameInternal(name);
	this -> edView = nullptr;
}


void LatexDocument::setEditorView(LatexEditorView * view){

	this -> setFileNameInternal(view -> editor -> fileName(),view -> editor -> fileInfo());
	this -> edView = view;

	if(baseStructure){
		baseStructure -> title = fileName;
		emit updateElement(baseStructure);
	}
}


/// Set the values of this->fileName and this->this->fileInfo
/// Note: QFileInfo is cached, so the performance cost to recreate
/// QFileInfo (instead of copying it from edView) should be very small.

void LatexDocument::setFileNameInternal(const QString & name){
	setFileNameInternal(name,QFileInfo(name));
}


/// Set the values of this->fileName and this->this->fileInfo

void LatexDocument::setFileNameInternal(const QString & name,const QFileInfo & pairedFileInfo){
	
	Q_ASSERT(fileName.isEmpty() || pairedFileInfo.isAbsolute());
	
    this -> fileInfo = pairedFileInfo;
	this -> fileName = name;
}


LatexEditorView * LatexDocument::getEditorView() const {
	return this -> edView;
}


QString LatexDocument::getFileName() const {
	return fileName;
}


bool LatexDocument::isHidden() {
	return parent -> hiddenDocuments.contains(this);
}


QFileInfo LatexDocument::getFileInfo() const {
	return fileInfo;
}


QMultiHash<QDocumentLineHandle *,FileNamePair> & LatexDocument::mentionedBibTeXFiles(){
	return mMentionedBibTeXFiles;
}


const QMultiHash<QDocumentLineHandle *,FileNamePair> & LatexDocument::mentionedBibTeXFiles() const {
	return mMentionedBibTeXFiles;
}


QStringList LatexDocument::listOfMentionedBibTeXFiles() const {

	QStringList paths;
    
	for(const auto & file : mMentionedBibTeXFiles)
		paths << file.absolute;

	return paths;
}


/*! select a complete section with the text
 * this method is called from structureview via contex menu
 *
 */

QDocumentSelection LatexDocument::sectionSelection(StructureEntry * section){

	QDocumentSelection result = { -1, -1, -1, -1};

	if(section -> type != StructureEntry::SE_SECTION)
		return result;
	
	int startLine = section -> getRealLineNumber();

	// find next section or higher
	StructureEntry * parent;

	int index;
	
	do {
		parent = section->parent;
		if (parent) {
			index = section->getRealParentRow();
			section = parent;
		} else index = -1;
	} while ((index >= 0) && (index >= parent->children.count() - 1) && (parent->type == StructureEntry::SE_SECTION));

	int endingLine = -1;
	if (index >= 0 && index < parent->children.count() - 1) {
		endingLine = parent->children.at(index + 1)->getRealLineNumber();
	} else {
		// no ending section but end of document
		endingLine = findLineContaining("\\end{document}", startLine, Qt::CaseInsensitive);
		if (endingLine < 0) endingLine = lines();
	}

	result.startLine = startLine;
	result.endLine = endingLine;
	result.end = 0;
	result.start = 0;
	return result;
}


class LatexStructureMerger {
	
	protected:
	
		LatexDocument * document;
		QVector<StructureEntry *> parent_level;
	
	public:
		
		LatexStructureMerger (LatexDocument * document,int maxDepth)
			: document(document)
			, parent_level(maxDepth) {}

	protected:
	
		void updateParentVector(StructureEntry *);
		void moveToAppropiatePositionWithSignal(StructureEntry *);
};


class LatexStructureMergerMerge: public LatexStructureMerger {

	private:

		const int linenr;
		const int count;

		QList<StructureEntry *> * flatStructure;

	private:

		void mergeStructure(StructureEntry *);
		void mergeChildren(StructureEntry *,int start = 0);
	
	public:
        
		LatexStructureMergerMerge(LatexDocument * document,int maxDepth,int linenr,int count)
			: LatexStructureMerger(document,maxDepth)
			, linenr(linenr)
			, count(count)
			, flatStructure(nullptr) {}

	void operator () (QList<StructureEntry *> & flatStructure){

		this -> flatStructure = & flatStructure;
		
		parent_level.fill(document -> baseStructure);
		mergeStructure(document -> baseStructure);
	}
};


/*! clear all internal data
 * preparation for rebuilding structure or for first parsing
 */

void LatexDocument::initClearStructure(){

	mMentionedBibTeXFiles.clear();
	mUserCommandList.clear();
	mLabelItem.clear();
	mBibItem.clear();
	mRefItem.clear();

	mAppendixLine = nullptr;
	mBeyondEnd = nullptr;

	emit structureUpdated(this,nullptr);

	const int CATCOUNT = 5;
	
	StructureEntry * categories[CATCOUNT] = { 
		magicCommentList , labelList , todoList , bibTeXList , blockList };
	
	for(int i = 0;i < CATCOUNT;i++)
		if(categories[i] -> parent == baseStructure){

			removeElementWithSignal(categories[i]);
			
			for(auto entry : categories[i] -> children)
				delete entry;

			categories[i] -> children.clear();
		}

	for(int i = 0;i < baseStructure -> children.length();i++){
		auto entry = baseStructure -> children[i];
		removeElementWithSignal(entry);
		delete entry;
	}

	baseStructure -> title = fileName;
}


/*! rebuild structure view completely
 *  /note very expensive call
 */

void LatexDocument::updateStructure(){

	initClearStructure();
	patchStructure(0,-1);

	emit structureLost(this);
}


/*! Removes a deleted line from the structure view
*/

void LatexDocument::patchStructureRemoval(QDocumentLineHandle * handle,int hint){

	if(!baseStructure)
		return;
	
	bool
		bibTeXFilesNeedsUpdate = false,
		completerNeedsUpdate = false,
		updateSyntaxCheck = false;

	if(mLabelItem.contains(handle)){

		auto labels = mLabelItem.values(handle);
		
		completerNeedsUpdate = true;
		
		mLabelItem.remove(handle);
		
		for(const auto & label : labels)
			updateRefsLabels(label.name);
	}

	mRefItem.remove(handle);

	if(mMentionedBibTeXFiles.remove(handle))
		bibTeXFilesNeedsUpdate = true;
	
	if(mBibItem.contains(handle)){
		mBibItem.remove(handle);
		bibTeXFilesNeedsUpdate = true;
	}

	auto commands = mUserCommandList.values(handle);
	
	for(auto command : commands){

		QString word = command.snippet.word;
		
		if(word.length() == 1){
		    for(auto cmd : ltxCommands.possibleCommands["%columntypes"])
				if(cmd.left(1) == word){
			    	ltxCommands.possibleCommands["%columntypes"].remove(cmd);
			    	break;
				}
		} else {
		    
			int index = word.indexOf('{');
		    
			if(index >= 0)
				word = word.left(index);
		    
			ltxCommands.possibleCommands["user"].remove(word);
		}

		updateSyntaxCheck = true;
	}

	mUserCommandList.remove(handle);

	auto removeIncludes = mIncludedFilesList.values(handle);

	if(mIncludedFilesList.remove(handle) > 0){
		parent -> removeDocs(removeIncludes);
		parent -> updateMasterSlaveRelations(this);
	}

	QStringList removedUsepackages;
	removedUsepackages << mUsepackageList.values(handle);
	mUsepackageList.remove(handle);

	if(handle == mAppendixLine){
		updateContext(mAppendixLine,nullptr,StructureEntry::InAppendix);
		mAppendixLine = nullptr;
	}

	int linenr = indexOf(handle,hint);

	if(linenr == -1)
		linenr = lines();

	// check if line contains bookmark
    
	if(edView){

        int id = edView->hasBookmark(handle);

        if(id > -2){
            emit bookmarkRemoved(handle);
            edView -> removeBookmark(handle,id);
        }
    }

    auto categories = QList<StructureEntry *>() 
		<< magicCommentList 
		<< labelList 
		<< todoList 
		<< blockList 
		<< bibTeXList;
	
	for(auto category : categories){

		int l = 0;
		
		QMutableListIterator<StructureEntry *> iter(category -> children);
		
		while(iter.hasNext()){
			
			auto entry = iter.next();
			
			if (handle == entry -> getLineHandle()){
				emit removeElement(entry,l);
				iter.remove();
				emit removeElementFinished();
				delete entry;
			} else {
				l++;
			}
		}
	}

	QList<StructureEntry *> tmp;

    if(!baseStructure -> children.isEmpty()){ // merge is not the fastest, even when there is nothing to merge
        LatexStructureMergerMerge(this,LatexParser::getInstance().structureDepth(),linenr,1)(tmp);
    }

	// rehighlight current cursor position

	StructureEntry * newSection = nullptr;
	
	if(edView){
	
		int i = edView -> editor -> cursor().lineNumber();
	
		if(i >= 0)
			newSection = findSectionForLine(i);
	}

	emit structureUpdated(this,newSection);

	if(bibTeXFilesNeedsUpdate)
		emit updateBibTeXFiles();

	if(completerNeedsUpdate || bibTeXFilesNeedsUpdate)
		emit updateCompleter();

	if(!removedUsepackages.isEmpty() || updateSyntaxCheck)
		updateCompletionFiles(updateSyntaxCheck);
}


// workaround to prevent false command recognition in definitions:
// Example: In \newcommand{\seeref}[1]{\ref{(see #1)}} the argument of \ref is not actually a label.
// Using this function we detect this case.
// TODO: a more general solution should make this dependent on if the command is inside a definition.
// However this requires a restructuring of the patchStructure. It would also allow categorizing
// the redefined command, e.g. as "%ref"

const QRegExp validDefinitionArgument { "#[0-9]" };

inline bool isDefinitionArgument(const QString & argument){
	return validDefinitionArgument.containedIn(argument);
}


/*!
 * \brief parse lines to update syntactical and structure information
 *
 * updates structure informationen from the changed lines only
 * parses the lines to gather syntactical information on the latex content
 * e.g. find labels/references, new command definitions etc.
 * the syntax parsing has been largely changed to the token system which is tranlated here for faster information extraction \see Tokens
 * \param linenr first line to check
 * \param count number of lines to check (-1: all)
 * \param recheck method has been called a second time to handle profound syntax changes from first call (like newly loaded packages). This allows to avoid some costly operations on the second call.
 * \return true means a second run is suggested as packages are loadeed which change the outcome
 *         e.g. definition of specialDef command, but packages are load at the end of this method.
 */

bool LatexDocument::patchStructure(int linenr,int count,bool recheck){

	/* true means a second run is suggested as packages are loadeed which change the outcome
	 * e.g. definition of specialDef command, but packages are load at the end of this method.
	 */

	if(!parent -> patchEnabled())
		return false;

	if(!baseStructure)
		return false;

	static QRegExp rxMagicTexComment("^%\\ ?!T[eE]X");
	static QRegExp rxMagicBibComment("^%\\ ?!BIB");

	bool reRunSuggested = false;
	bool recheckLabels = true;

	if(count < 0){
		count = lineCount();
		recheckLabels = false;
	}

	emit toBeChanged();

	bool completerNeedsUpdate = false;
	bool bibTeXFilesNeedsUpdate = false;
	bool bibItemsChanged = false;

	auto oldLine = mAppendixLine; // to detect a change in appendix position
	auto oldLineBeyond = mBeyondEnd; // to detect a change in end document position

	// get remainder
	TokenStack remainder;
	
	int lineNrStart = linenr;
	int newCount = count;

	if(linenr > 0){
		
		auto previous = line(linenr - 1).handle();
	
		remainder = previous -> getCookieLocked(QDocumentLine::LEXER_REMAINDER_COOKIE).value<TokenStack>();
	
		if(!remainder.isEmpty() && remainder.top().subtype != Token::none){
			
			auto lineHandle = remainder.top().dlh;
			lineNrStart = lineHandle -> document() -> indexOf(lineHandle);
			
			if(linenr - lineNrStart > 10) // limit search depth
				lineNrStart = linenr;
		}
	}

	bool updateSyntaxCheck = false;
	
	QList<StructureEntry *> flatStructure;

	// usepackage list
	QStringList removedUsepackages;
	QStringList addedUsepackages;
	QStringList removedUserCommands, addedUserCommands;
	QStringList lstFilesToLoad;

	//first pass: lex
    TokenStack oldRemainder;
    CommandStack oldCommandStack;
	
	if(!recheck){
		QList<QDocumentLineHandle *> l_dlh;
	
		for(int i = linenr;i < linenr + count;i++)
			l_dlh << line(i).handle();
    
	    QtConcurrent::blockingMap(l_dlh,Parsing::simpleLexLatexLine);
	}

	auto lastHandle = line(linenr - 1).handle();
	
	if(lastHandle){
		oldRemainder = lastHandle -> getCookieLocked(QDocumentLine::LEXER_REMAINDER_COOKIE).value<TokenStack>();
		oldCommandStack = lastHandle -> getCookieLocked(QDocumentLine::LEXER_COMMANDSTACK_COOKIE).value<CommandStack>();
	}

    int stoppedAtLine = -1;
    
	for(int i = linenr;i < lineCount() && i < linenr + count;i++){
       
	    if(line(i).text() == "\\begin{document}"){
            if(linenr == 0 && count == lineCount() && !recheck){
                stoppedAtLine=i;
                break; // do recheck quickly as usepackages probably need to be loaded
            }
        }

        bool remainderChanged = Parsing::latexDetermineContexts2(line(i).handle(),oldRemainder,oldCommandStack,lp);
		
		if(remainderChanged && i + 1 == linenr + count && i + 1 < lineCount()) // remainder changed in last line which is to be checked
			count++; // check also next line ...
	}

	if(linenr >= lineNrStart)
		newCount = linenr + count - lineNrStart;

	// Note: We cannot re-use the structure elements in the updated area because if there are multiple same-type elements on the same line
	// and the user has changed their order, re-using these elements would not update their order and this would break updates of any
	// QPersistentModelIndex'es that point to these elements in the structure tree view. That is why we remove all the structure elements
	// within the updated area and then just add anew any structure elements that we find in the updated area.

	QList<StructureEntry *> removedMagicComments;
	int posMagicComment = findStructureParentPos(magicCommentList -> children,removedMagicComments,lineNrStart,newCount);

	QList<StructureEntry *> removedLabels;
	int posLabel = findStructureParentPos(labelList -> children,removedLabels,lineNrStart,newCount);

	QList<StructureEntry *> removedTodo;
	int posTodo = findStructureParentPos(todoList -> children,removedTodo,lineNrStart,newCount);

	QList<StructureEntry *> removedBlock;
	int posBlock = findStructureParentPos(blockList -> children,removedBlock,lineNrStart,newCount);

	QList<StructureEntry *> removedBibTeX;
	int posBibTeX = findStructureParentPos(bibTeXList -> children,removedBibTeX,lineNrStart,newCount);

	bool isLatexLike = languageIsLatexLike();

	//updateSubsequentRemaindersLatex(this,linenr,count,lp);
	// force command from all line of which the actual line maybe subsequent lines (multiline commands)
	
	for(int i = lineNrStart; i < linenr + count; i++) {
		//update bookmarks
		if (edView && edView->hasBookmark(i, -1)) {
			emit bookmarkLineUpdated(i);
		}

		if (!isLatexLike) continue;

		QString curLine = line(i).text();
		QDocumentLineHandle *dlh = line(i).handle();
		if (!dlh)
			continue; //non-existing line ...

		// remove command,bibtex,labels at from this line
		QList<UserCommandPair> commands = mUserCommandList.values(dlh);
		foreach (UserCommandPair cmd, commands) {
			QString elem = cmd.snippet.word;
			if(elem.length()==1){
			    for (auto i:ltxCommands.possibleCommands["%columntypes"]) {
				if(i.left(1)==elem){
				    ltxCommands.possibleCommands["%columntypes"].remove(i);
				    break;
				}
			    }
			}else{
			    int i = elem.indexOf("{");
			    if (i >= 0) elem = elem.left(i);
			    ltxCommands.possibleCommands["user"].remove(elem);
			}
			if(cmd.snippet.type==CodeSnippet::userConstruct)
				continue;
			removedUserCommands << elem;
			//updateSyntaxCheck=true;
		}
		if (mLabelItem.contains(dlh)) {
			QList<ReferencePair> labels = mLabelItem.values(dlh);
			completerNeedsUpdate = true;
			mLabelItem.remove(dlh);
			foreach (const ReferencePair &rp, labels)
				updateRefsLabels(rp.name);
		}
		mRefItem.remove(dlh);
		QStringList removedIncludes = mIncludedFilesList.values(dlh);
		mIncludedFilesList.remove(dlh);

		if (mUserCommandList.remove(dlh) > 0) completerNeedsUpdate = true;
		if (mBibItem.remove(dlh))
			bibTeXFilesNeedsUpdate = true;

		removedUsepackages << mUsepackageList.values(dlh);
		if (mUsepackageList.remove(dlh) > 0) completerNeedsUpdate = true;

		//remove old bibs files from hash, but keeps a temporary copy
		QStringList oldBibs;
		while (mMentionedBibTeXFiles.contains(dlh)) {
			QMultiHash<QDocumentLineHandle *, FileNamePair>::iterator it = mMentionedBibTeXFiles.find(dlh);
			Q_ASSERT(it.key() == dlh);
			Q_ASSERT(it != mMentionedBibTeXFiles.end());
			if (it == mMentionedBibTeXFiles.end()) break;
			oldBibs.append(it.value().relative);
			mMentionedBibTeXFiles.erase(it);
		}

        // handle special comments (TODO, MAGIC comments)
        // comment detection moved to lexer as formats are not yet generated here (e.g. on first load)
        QPair<int,int> commentStart = dlh->getCookieLocked(QDocumentLine::LEXER_COMMENTSTART_COOKIE).value<QPair<int,int> >();
        int col = commentStart.first;
		if (col >= 0) {
            // all
            //// TODO marker
            QString text = curLine.mid(col);
			QString regularExpression=ConfigManagerInterface::getInstance()->getOption("Editor/todo comment regExp").toString();
			QRegExp rx(regularExpression);
			if (rx.indexIn(text)==0) {  // other todos like \todo are handled by the tokenizer below.
				StructureEntry *newTodo = new StructureEntry(this, StructureEntry::SE_TODO);
				newTodo->title = text.mid(1).trimmed();
				newTodo->setLine(line(i).handle(), i);
				insertElementWithSignal(todoList, posTodo++, newTodo);
                // save comment type into cookie
                commentStart.second=Token::todoComment;
                dlh->setCookie(QDocumentLine::LEXER_COMMENTSTART_COOKIE, QVariant::fromValue<QPair<int,int> >(commentStart));
			}
            //// parameter comment
            if (curLine.startsWith("%&")) {
                int start = curLine.indexOf("-job-name=");
                if (start >= 0) {
                    int end = start + 10; // += "-job-name=".length;
                    if (end < curLine.length() && curLine[end] == '"') {
                        // quoted filename
                        end = curLine.indexOf('"', end + 1);
                        if (end >= 0) {
                            end += 1;  // include closing quotation mark
                            addMagicComment(curLine.mid(start, end - start), i, posMagicComment++);
                        }
                    } else {
                        end = curLine.indexOf(' ', end + 1);
                        if (end >= 0) {
                            addMagicComment(curLine.mid(start, end - start), i, posMagicComment++);
                        } else {
                            addMagicComment(curLine.mid(start), i, posMagicComment++);
                        }
                    }
                }
                commentStart.second=Token::magicComment;
                dlh->setCookie(QDocumentLine::LEXER_COMMENTSTART_COOKIE, QVariant::fromValue<QPair<int,int> >(commentStart));
            }
            //// magic comment
            if (rxMagicTexComment.indexIn(text) == 0) {
                addMagicComment(text.mid(rxMagicTexComment.matchedLength()).trimmed(), i, posMagicComment++);
                commentStart.second=Token::magicComment;
                dlh->setCookie(QDocumentLine::LEXER_COMMENTSTART_COOKIE, QVariant::fromValue<QPair<int,int> >(commentStart));
            } else if (rxMagicBibComment.indexIn(text) == 0) {
                // workaround to also support "% !BIB program = biber" syntax used by TeXShop and TeXWorks
                text = text.mid(rxMagicBibComment.matchedLength()).trimmed();
                QString name;
                QString val;
                splitMagicComment(text, name, val);
                if ((name == "TS-program" || name == "program") && (val == "biber" || val == "bibtex" || val == "bibtex8")) {
                    addMagicComment(QString("TXS-program:bibliography = txs:///%1").arg(val), i, posMagicComment++);
                    commentStart.second=Token::magicComment;
                    dlh->setCookie(QDocumentLine::LEXER_COMMENTSTART_COOKIE, QVariant::fromValue<QPair<int,int> >(commentStart));
                }
            }
		}

		// check also in command argument, als references might be put there as well...
		//// Appendix keyword

		if(curLine == "\\appendix"){
			oldLine = mAppendixLine;
			mAppendixLine = line(i).handle();
		}

		if(line(i).handle() == mAppendixLine && curLine != "\\appendix"){
			oldLine = mAppendixLine;
			mAppendixLine = nullptr;
		}

        /// \begin{document}
        /// break patchStructure at begin{document} since added usepackages need to be loaded and then the text needs to be checked
        /// only useful when loading a complete new text.
        
		if(
			curLine == "\\begin{document}" &&
			count == lineCount() && 
            linenr == 0 && 
			!recheck
		){
			if(!addedUsepackages.isEmpty())
				break; // do recheck quickly as usepackages probably need to be loaded
			else
				// oops, complete tokenlist needed !
				// redo on time
				for(int i = stoppedAtLine;i < lineCount();i++)
					Parsing::latexDetermineContexts2(line(i).handle(),oldRemainder,oldCommandStack,lp);
		}

		/// \end{document} keyword
		/// don't add section in structure view after passing \end{document} , this command must not contains spaces nor any additions in the same line
		if (curLine == "\\end{document}") {
			oldLineBeyond = mBeyondEnd;
			mBeyondEnd = line(i).handle();
		}
		if (line(i).handle() == mBeyondEnd && curLine != "\\end{document}") {
			oldLineBeyond = mBeyondEnd;
			mBeyondEnd = nullptr;
		}

        TokenList tl = dlh->getCookieLocked(QDocumentLine::LEXER_COOKIE).value<TokenList >();

		for (int j = 0; j < tl.length(); j++) {
			Token tk = tl.at(j);
			// break at comment start
			if (tk.type == Token::comment)
				break;
			// work special args
			////Ref
			//for reference counting (can be placed in command options as well ...
			if (tk.type == Token::labelRef || tk.type == Token::labelRefList) {
				ReferencePair elem;
				elem.name = tk.getText();
				elem.start = tk.start;
				mRefItem.insert(line(i).handle(), elem);
			}

			//// label ////
			if (tk.type == Token::label && tk.length > 0) {
				ReferencePair elem;
				elem.name = tk.getText();
				elem.start = tk.start;
				mLabelItem.insert(line(i).handle(), elem);
				completerNeedsUpdate = true;
				StructureEntry *newLabel = new StructureEntry(this, StructureEntry::SE_LABEL);
				newLabel->title = elem.name;
				newLabel->setLine(line(i).handle(), i);
				insertElementWithSignal(labelList, posLabel++, newLabel);
			}
			//// newtheorem ////
			if (tk.type == Token::newTheorem && tk.length > 0) {
				completerNeedsUpdate = true;
				QStringList lst;
				QString firstArg = tk.getText();
				lst << "\\begin{" + firstArg + "}" << "\\end{" + firstArg + "}";
				foreach (const QString &elem, lst) {
					mUserCommandList.insert(line(i).handle(), UserCommandPair(firstArg, elem));
					ltxCommands.possibleCommands["user"].insert(elem);
					if (!removedUserCommands.removeAll(elem)) {
						addedUserCommands << elem;
					}
				}
				continue;
			}
			/// bibitem ///
			if (tk.type == Token::newBibItem && tk.length > 0) {
				ReferencePair elem;
				elem.name = tk.getText();
				elem.start = tk.start;
				mBibItem.insert(line(i).handle(), elem);
				bibItemsChanged = true;
				continue;
			}
			/// todo ///
			if (tk.subtype == Token::todo && (tk.type == Token::braces || tk.type == Token::openBrace)) {
				StructureEntry *newTodo = new StructureEntry(this, StructureEntry::SE_TODO);
				newTodo->title = tk.getInnerText();
				newTodo->setLine(line(i).handle(), i);
				insertElementWithSignal(todoList, posTodo++, newTodo);
			}

			// work on general commands
			if (tk.type != Token::command && tk.type != Token::commandUnknown)
				continue; // not a command
			Token tkCmd;
			TokenList args;
			QString cmd;
			int cmdStart = Parsing::findCommandWithArgsFromTL(tl, tkCmd, args, j, parent->showCommentedElementsInStructure);
			if (cmdStart < 0) break;
			cmdStart=tkCmd.start; // from here, cmdStart is line column position of command
			cmd = curLine.mid(tkCmd.start, tkCmd.length);

            QString firstArg = Parsing::getArg(args, dlh, 0, ArgumentList::Mandatory,true,i);

			//// newcommand ////
			if (lp.possibleCommands["%definition"].contains(cmd) || ltxCommands.possibleCommands["%definition"].contains(cmd)) {
				completerNeedsUpdate = true;
				//Tokens cmdName;
				QString cmdName = Parsing::getArg(args, Token::def);
				cmdName.replace("@","@@"); // special treatment for commandnames containing @
				bool isDefWidth = true;
				if (cmdName.isEmpty())
					cmdName = Parsing::getArg(args, Token::defWidth);
				else
					isDefWidth = false;
				//int optionCount = Parsing::getArg(args, dlh, 0, ArgumentList::Optional).toInt(); // results in 0 if there is no optional argument or conversion fails
				int optionCount = Parsing::getArg(args, Token::defArgNumber).toInt(); // results in 0 if there is no optional argument or conversion fails
				if (optionCount > 9 || optionCount < 0) optionCount = 0; // limit number of options
				bool def = !Parsing::getArg(args, Token::optionalArgDefinition).isEmpty();

				ltxCommands.possibleCommands["user"].insert(cmdName);

				if (!removedUserCommands.removeAll(cmdName)) {
					addedUserCommands << cmdName;
				}
				QString cmdNameWithoutArgs = cmdName;
				QString cmdNameWithoutOptional = cmdName;
				for (int j = 0; j < optionCount; j++) {
					if (j == 0) {
						if (!def){
							cmdName.append("{%<arg1%|%>}");
							cmdNameWithoutOptional.append("{%<arg1%|%>}");
						} else
							cmdName.append("[%<opt. arg1%|%>]");
						} else {
							cmdName.append(QString("{%<arg%1%>}").arg(j + 1));
							cmdNameWithoutOptional.append(QString("{%<arg%1%>}").arg(j + 1));
						}
				}
				CodeSnippet cs(cmdName);
				cs.index = qHash(cmdName);
				cs.snippetLength = cmdName.length();
				if (isDefWidth)
					cs.type = CodeSnippet::length;
				mUserCommandList.insert(line(i).handle(), UserCommandPair(cmdNameWithoutArgs, cs));
				if(def){ // optional argument, add version without that argument as well
					CodeSnippet cs(cmdNameWithoutOptional);
					cs.index = qHash(cmdNameWithoutOptional);
					cs.snippetLength = cmdNameWithoutOptional.length();
					if (isDefWidth)
						cs.type = CodeSnippet::length;
					mUserCommandList.insert(line(i).handle(), UserCommandPair(cmdNameWithoutArgs, cs));
				}
				// remove obsolete Overlays (maybe this can be refined
				//updateSyntaxCheck=true;
				continue;
			}
			// special treatment \def
			if (cmd == "\\def" || cmd == "\\gdef" || cmd == "\\edef" || cmd == "\\xdef") {
				QString remainder = curLine.mid(cmdStart + cmd.length());
				completerNeedsUpdate = true;
				QRegExp rx("(\\\\\\w+)\\s*([^{%]*)");
				if (rx.indexIn(remainder) > -1) {
					QString name = rx.cap(1);
					QString nameWithoutArgs = name;
					QString optionStr = rx.cap(2);
					//qDebug()<< name << ":"<< optionStr;
					ltxCommands.possibleCommands["user"].insert(name);
					if (!removedUserCommands.removeAll(name)) addedUserCommands << name;
					optionStr = optionStr.trimmed();
					if (optionStr.length()) {
						int lastArg = optionStr[optionStr.length() - 1].toLatin1() - '0';
						if (optionStr.length() == lastArg * 2) { //#1#2#3...
							for (int j = 1; j <= lastArg; j++)
								if (j == 1) name.append("{%<arg1%|%>}");
								else name.append(QString("{%<arg%1%>}").arg(j));
						} else {
							QStringList args = optionStr.split('#'); //#1;#2#3:#4 => ["",1;,2,3:,4]
							bool hadSeparator = true;
							for (int i = 1; i < args.length(); i++) {
								if (args[i].length() == 0) continue; //invalid
								bool hasSeparator = (args[i].length() != 1); //only single digit variables allowed. last arg also needs a sep
								if (!hadSeparator || !hasSeparator)
                                    args[i] = QString("{%<arg") + args[i][0] + QString("%>}") + args[i].mid(1);
								else
                                    args[i] = QString("%<arg") + args[i][0] + QString("%>") + args[i].mid(1); //no need to use {} for arguments that are separated anyways
								hadSeparator  = hasSeparator;
							}
							name.append(args.join(""));
						}
					}
					mUserCommandList.insert(line(i).handle(), UserCommandPair(nameWithoutArgs, name));
					// remove obsolete Overlays (maybe this can be refined
					//updateSyntaxCheck=true;
				}
				continue;
			}
			if (cmd == "\\newcolumntype") {
				if(firstArg.length()==1){ // only single letter definitions are allowed/handled
					QString secondArg = Parsing::getArg(args, dlh, 1, ArgumentList::Mandatory);
					ltxCommands.possibleCommands["%columntypes"].insert(firstArg+secondArg);
					if (!removedUserCommands.removeAll(firstArg)) {
						addedUserCommands << firstArg;
					}
					mUserCommandList.insert(line(i).handle(), UserCommandPair(QString(), firstArg));
					continue;
				}
			}

			//// newenvironment ////
			static const QStringList envTokens = QStringList() << "\\newenvironment" << "\\renewenvironment";
			if (envTokens.contains(cmd)) {
				completerNeedsUpdate = true;
				TokenList argsButFirst = args;
				if(argsButFirst.isEmpty())
					continue; // no arguments present
				argsButFirst.removeFirst();
				int optionCount = Parsing::getArg(argsButFirst, dlh, 0, ArgumentList::Optional).toInt(); // results in 0 if there is no optional argument or conversion fails
				if (optionCount > 9 || optionCount < 0) optionCount = 0; // limit number of options
				mUserCommandList.insert(line(i).handle(), UserCommandPair(firstArg, "\\end{" + firstArg + "}"));
				QStringList lst;
				lst << "\\begin{" + firstArg + "}" << "\\end{" + firstArg + "}";
				foreach (const QString &elem, lst) {
					ltxCommands.possibleCommands["user"].insert(elem);
					if (!removedUserCommands.removeAll(elem)) {
						addedUserCommands << elem;
					}
				}
				bool hasDefaultArg = !Parsing::getArg(argsButFirst, dlh, 1, ArgumentList::Optional).isNull();
				int mandatoryOptionCount = hasDefaultArg ? optionCount - 1 : optionCount;
				QString mandatoryArgString;
				for (int j = 0; j < mandatoryOptionCount; j++) {
					if (j == 0) mandatoryArgString.append("{%<1%>}");
					else mandatoryArgString.append(QString("{%<%1%>}").arg(j + 1));
				}
				mUserCommandList.insert(line(i).handle(), UserCommandPair(firstArg, "\\begin{" + firstArg + "}" + mandatoryArgString));
				if (hasDefaultArg) {
					mUserCommandList.insert(line(i).handle(), UserCommandPair(firstArg, "\\begin{" + firstArg + "}" + "[%<opt%>]" + mandatoryArgString));
				}
				continue;
			}
			//// newcounter ////
			if (cmd == "\\newcounter") {
				completerNeedsUpdate = true;
				QStringList lst;
				lst << "\\the" + firstArg ;
				foreach (const QString &elem, lst) {
					mUserCommandList.insert(line(i).handle(), UserCommandPair(elem, elem));
					ltxCommands.possibleCommands["user"].insert(elem);
					if (!removedUserCommands.removeAll(elem)) {
						addedUserCommands << elem;
					}
				}
				continue;
			}
			//// newif ////
			if (cmd == "\\newif") {
				// \newif\ifmycondition also defines \myconditiontrue and \myconditionfalse
				completerNeedsUpdate = true;
				QStringList lst;
				lst << firstArg
					<< "\\" + firstArg.mid(3) + "false"
					<< "\\" + firstArg.mid(3) + "true";
				foreach (const QString &elem, lst) {
					mUserCommandList.insert(line(i).handle(), UserCommandPair(elem, elem));
					ltxCommands.possibleCommands["user"].insert(elem);
					if (!removedUserCommands.removeAll(elem)) {
						addedUserCommands << elem;
					}
				}
				continue;
			}
			/// specialDefinition ///
			/// e.g. definecolor
			if (ltxCommands.specialDefCommands.contains(cmd)) {
				if (!args.isEmpty() ) {
					completerNeedsUpdate = true;
					QString definition = ltxCommands.specialDefCommands.value(cmd);
					Token::TokenType type = Token::braces;
					if (definition.startsWith('(')) {
						definition.chop(1);
						definition = definition.mid(1);
						type = Token::bracket;
					}
					if (definition.startsWith('[')) {
						definition.chop(1);
						definition = definition.mid(1);
						type = Token::squareBracket;
					}

					foreach (Token mTk, args) {
						if (mTk.type != type)
							continue;
						QString elem = mTk.getText();
						elem = elem.mid(1, elem.length() - 2); // strip braces
						mUserCommandList.insert(line(i).handle(), UserCommandPair(QString(), definition + "%" + elem));
						if (!removedUserCommands.removeAll(elem)) {
							addedUserCommands << elem;
						}
						break;
					}
				}
			}

			///usepackage
			if (lp.possibleCommands["%usepackage"].contains(cmd)) {
				completerNeedsUpdate = true;
				QStringList packagesHelper = firstArg.split(",");

				if (cmd.endsWith("theme")) { // special treatment for  \usetheme
					QString preambel = cmd;
					preambel.remove(0, 4);
					preambel.prepend("beamer");
                    packagesHelper.replaceInStrings(QRegularExpression("^"), preambel);
				}

				QString firstOptArg = Parsing::getArg(args, dlh, 0, ArgumentList::Optional);
				if (cmd == "\\documentclass") {
					//special treatment for documentclass, especially for the class options
					// at the moment a change here soes not automatically lead to an update of corresponding definitions, here babel
					mClassOptions = firstOptArg;
				}

				if (firstArg == "babel") {
					//special treatment for babel
					if (firstOptArg.isEmpty()) {
						firstOptArg = mClassOptions;
					}
					if (!firstOptArg.isEmpty()) {
						packagesHelper << firstOptArg.split(",");
					}
				}

				QStringList packages;
				foreach (QString elem, packagesHelper) {
					elem = elem.simplified();
					if (lp.packageAliases.contains(elem))
						packages << lp.packageAliases.values(elem);
					else
						packages << elem;
				}

				foreach (const QString &elem, packages) {
					if (!removedUsepackages.removeAll(firstOptArg + "#" + elem))
						addedUsepackages << firstOptArg + "#" + elem;
                    mUsepackageList.insert(dlh, firstOptArg + "#" + elem); // hand on option of usepackages for conditional cwl load ..., force load if option is changed
				}
				continue;
			}
			//// bibliography ////
			if (lp.possibleCommands["%bibliography"].contains(cmd)) {
				QStringList additionalBibPaths = ConfigManagerInterface::getInstance()->getOption("Files/Bib Paths").toString().split(getPathListSeparator());
                QStringList bibs = firstArg.split(',', Qt::SkipEmptyParts);
				//add new bibs and set bibTeXFilesNeedsUpdate if there was any change
				foreach (const QString &elem, bibs) { //latex doesn't seem to allow any spaces in file names
					mMentionedBibTeXFiles.insert(line(i).handle(), FileNamePair(elem, getAbsoluteFilePath(elem, "bib", additionalBibPaths)));
					if (oldBibs.removeAll(elem) == 0)
						bibTeXFilesNeedsUpdate = true;
				}
				//write bib tex in tree
				foreach (const QString &bibFile, bibs) {
					StructureEntry *newFile = new StructureEntry(this, StructureEntry::SE_BIBTEX);
					newFile->title = bibFile;
					newFile->setLine(line(i).handle(), i);
					insertElementWithSignal(bibTeXList, posBibTeX++, newFile);
				}
				continue;
			}

			//// beamer blocks ////

			if (cmd == "\\begin" && firstArg == "block") {
				StructureEntry *newBlock = new StructureEntry(this, StructureEntry::SE_BLOCK);
                newBlock->title = Parsing::getArg(args, dlh, 1, ArgumentList::Mandatory,true,i);
				newBlock->setLine(line(i).handle(), i);
				insertElementWithSignal(blockList, posBlock++, newBlock);
				continue;
			}

			//// include,input,import ////
			if (lp.possibleCommands["%include"].contains(cmd) && !isDefinitionArgument(firstArg)) {
				StructureEntry *newInclude = new StructureEntry(this, StructureEntry::SE_INCLUDE);
				newInclude->level = parent && !parent->indentIncludesInStructure ? 0 : lp.structureDepth() - 1;
				firstArg = removeQuote(firstArg);
				newInclude->title = firstArg;
                QString name=firstArg;
                name.replace("\\string~",QDir::homePath());
                QString fname = findFileName(name);
				removedIncludes.removeAll(fname);
				mIncludedFilesList.insert(line(i).handle(), fname);
				LatexDocument *dc = parent->findDocumentFromName(fname);
				if (dc) {
					childDocs.insert(dc);
					dc->setMasterDocument(this, recheckLabels);
				} else {
					lstFilesToLoad << fname;
					//parent->addDocToLoad(fname);
				}

				newInclude->valid = !fname.isEmpty();
				newInclude->setLine(line(i).handle(), i);
				newInclude->columnNumber = cmdStart;
				flatStructure << newInclude;
				updateSyntaxCheck = true;
				continue;
			}

			if (lp.possibleCommands["%import"].contains(cmd) && !isDefinitionArgument(firstArg)) {
				StructureEntry *newInclude = new StructureEntry(this, StructureEntry::SE_INCLUDE);
				newInclude->level = parent && !parent->indentIncludesInStructure ? 0 : lp.structureDepth() - 1;
				QDir dir(firstArg);
                QFileInfo fi(dir, Parsing::getArg(args, dlh, 1, ArgumentList::Mandatory,true,i));
				QString file = fi.filePath();
				newInclude->title = file;
				QString fname = findFileName(file);
				removedIncludes.removeAll(fname);
				mIncludedFilesList.insert(line(i).handle(), fname);
				LatexDocument *dc = parent->findDocumentFromName(fname);
				if (dc) {
					childDocs.insert(dc);
					dc->setMasterDocument(this, recheckLabels);
				} else {
					lstFilesToLoad << fname;
					//parent->addDocToLoad(fname);
				}

				newInclude->valid = !fname.isEmpty();
				newInclude->setLine(line(i).handle(), i);
				newInclude->columnNumber = cmdStart;
				flatStructure << newInclude;
				updateSyntaxCheck = true;
				continue;
			}

			//// all sections ////
			if (cmd.endsWith("*"))
				cmd = cmd.left(cmd.length() - 1);
			int level = lp.structureCommandLevel(cmd);
			if(level<0 && cmd=="\\begin"){
				// special treatment for \begin{frame}{title}
				level=lp.structureCommandLevel(cmd+"{"+firstArg+"}");
			}
			if (level > -1 && !firstArg.isEmpty() && tkCmd.subtype == Token::none) {
				StructureEntry *newSection = new StructureEntry(this, StructureEntry::SE_SECTION);
				if (mAppendixLine && indexOf(mAppendixLine) < i) newSection->setContext(StructureEntry::InAppendix);
				if (mBeyondEnd && indexOf(mBeyondEnd) < i) newSection->setContext(StructureEntry::BeyondEnd);
				//QString firstOptArg = Parsing::getArg(args, dlh, 0, ArgumentList::Optional);
				QString firstOptArg = Parsing::getArg(args, Token::shorttitle);
				if (!firstOptArg.isEmpty() && firstOptArg != "[]") // workaround, actually getArg should return "" for "[]"
					firstArg = firstOptArg;
				if(cmd=="\\begin"){
					// special treatment for \begin{frame}{title}
                    firstArg = Parsing::getArg(args, dlh, 1, ArgumentList::MandatoryWithBraces,false,i);
					if(firstArg.isEmpty()){
						// empty frame title, maybe \frametitle is used ?
						delete newSection;
						continue;
					}
				}
				newSection->title = latexToText(firstArg).trimmed();
				newSection->level = level;
				newSection->setLine(line(i).handle(), i);
				newSection->columnNumber = cmdStart;
				flatStructure << newSection;
				continue;
			}
			/// auto user command for \symbol_...
			if(j+2<tl.length()){
				Token tk2=tl.at(j+1);
				if(tk2.getText()=="_"){
					QString txt=cmd+"_";
					tk2=tl.at(j+2);
					txt.append(tk2.getText());
					if(tk2.type==Token::command && j+3<tl.length()){
						Token tk3=tl.at(j+3);
						if(tk3.level==tk2.level && tk.subtype!=Token::none)
							txt.append(tk3.getText());
					}
					CodeSnippet cs(txt);
					cs.type=CodeSnippet::userConstruct;
					mUserCommandList.insert(line(i).handle(), UserCommandPair(QString(), cs));
				}
			}
			/// auto user commands of \mathcmd{one arg} e.g. \mathsf{abc} or \overbrace{abc}
			if(j+2<tl.length() && !firstArg.isEmpty() && lp.possibleCommands["math"].contains(cmd) ){
				if (lp.commandDefs.contains(cmd)) {
					CommandDescription cd = lp.commandDefs.value(cmd);
					if(cd.args==1 && cd.bracketArgs==0 && cd.optionalArgs==0){
						QString txt=cmd+"{"+firstArg+"}";
						CodeSnippet cs(txt);
						cs.type=CodeSnippet::userConstruct;
						mUserCommandList.insert(line(i).handle(), UserCommandPair(QString(), cs));
					}
				}
			}

		} // while(findCommandWithArgs())

		if (!oldBibs.isEmpty())
			bibTeXFilesNeedsUpdate = true; //file name removed

		if (!removedIncludes.isEmpty()) {
			parent->removeDocs(removedIncludes);
			parent->updateMasterSlaveRelations(this);
		}
	}//for each line handle
	
	
	const auto removeElement = [ & ](auto entry){
		removeElementWithSignal(entry);
		delete(entry);
	};

	const auto removables = { 
		removedTodo , removedBibTeX , 
		removedBlock , removedLabels , 
		removedMagicComments 
	};

	for(auto elements : removables)
		for(auto element : elements)
			removeElement(element);


	StructureEntry * newSection = nullptr;

    // always generate complete structure, also for hidden, as needed for globalTOC
    LatexStructureMergerMerge(this,lp.structureDepth(),lineNrStart,newCount)(flatStructure);

    const auto categories = QList<StructureEntry *>() 
		<< magicCommentList 
		<< blockList 
		<< labelList 
		<< todoList 
		<< bibTeXList;

    for(int i = categories.size() - 1;i >= 0;i--){

        auto category = categories[i];
        
		if(category -> children.isEmpty() == (category -> parent == nullptr))
			continue;
        
		if(category -> children.isEmpty())
			removeElementWithSignal(category);
        else 
			insertElementWithSignal(baseStructure,0,category);
    }

    //update appendix change
    if(oldLine != mAppendixLine)
        updateContext(oldLine,mAppendixLine,StructureEntry::InAppendix);
    
	//update end document change
    if(oldLineBeyond != mBeyondEnd)
        updateContext(oldLineBeyond,mBeyondEnd,StructureEntry::BeyondEnd);

    // rehighlight current cursor position
    if(edView){

        int line = edView -> editor -> cursor().lineNumber();
        
		if(line >= 0)
            newSection = findSectionForLine(line);
    }

	emit structureUpdated(this,newSection);

	bool updateLtxCommands = false;

	if(
		! addedUsepackages.isEmpty() || 
		! removedUsepackages.isEmpty() || 
		! addedUserCommands.isEmpty() || 
		! removedUserCommands.isEmpty()
	){
		bool forceUpdate = 
			! addedUserCommands.isEmpty() || 
			! removedUserCommands.isEmpty();
	
		reRunSuggested = (count > 1) && (
			! addedUsepackages.isEmpty() || 
			! removedUsepackages.isEmpty()
		);
    
	    // don't patch single lines if the whole text needs to be rechecked anyways
        updateLtxCommands = updateCompletionFiles(forceUpdate,false,true,reRunSuggested);
	}

	if(bibTeXFilesNeedsUpdate)
		emit updateBibTeXFiles();

	// force update on citation overlays
	
	if(bibItemsChanged || bibTeXFilesNeedsUpdate){

		parent -> updateBibFiles(bibTeXFilesNeedsUpdate);
		
		// needs probably done asynchronously as bibteFiles needs to be loaded first ...
		for(auto document : getListOfDocs())
			if(document -> edView)
				document -> edView ->updateCitationFormats();
	}

	if(completerNeedsUpdate || bibTeXFilesNeedsUpdate)
		emit updateCompleter();
	
	if((!recheck && updateSyntaxCheck) || updateLtxCommands)
	    this -> updateLtxCommands(true);

    //update view (unless patchStructure is run again anyway)
    if(edView && !reRunSuggested)
		edView -> documentContentChanged(linenr,count);

	#ifndef QT_NO_DEBUG

		if(!isHidden())
			checkForLeak();
	
	#endif

	for(auto file : lstFilesToLoad)
		parent -> addDocToLoad(file);

	if(reRunSuggested && !recheck)
		patchStructure(0,-1,true); // expensive solution for handling changed packages (and hence command definitions)

	if(!recheck)
		reCheckSyntax(lineNrStart,newCount);

	return reRunSuggested;
}


#ifndef QT_NO_DEBUG

	void LatexDocument::checkForLeak(){

		StructureEntryIterator iter(baseStructure);
		QSet<StructureEntry *> zw = StructureContent;
		
		while(iter.hasNext())
			zw.remove(iter.next());

		// filter top level elements
		QMutableSetIterator<StructureEntry *> i(zw);
		
		while(i.hasNext())
			if(i.next() -> type == StructureEntry::SE_OVERVIEW)
				i.remove();

		if(zw.count() > 0)
			qDebug("Memory leak in structure");
	}

#endif

StructureEntry * LatexDocument::findSectionForLine(int currentLine){

	StructureEntryIterator iter(baseStructure);
    StructureEntry * newSection = nullptr;

	while(/*iter.hasNext()*/true){
        
		StructureEntry * curSection = nullptr;
		
		while(iter.hasNext()){

			curSection = iter.next();
			
			if(curSection -> type == StructureEntry::SE_SECTION)
				break;
		}
        
		if(curSection == nullptr)
			break;
		
		if(curSection -> type != StructureEntry::SE_SECTION)
			break;

		// curSection is after newSection where the cursor is

		if(curSection -> getRealLineNumber() > currentLine)
			break;
		
		newSection = curSection;
	}

    if(newSection && newSection -> getRealLineNumber() > currentLine)
		return nullptr;

	return newSection;
}

void LatexDocument::setTemporaryFileName(const QString & fileName){
	temporaryFileName = fileName;
}

QString LatexDocument::getTemporaryFileName() const {
	return temporaryFileName;
}

QString LatexDocument::getFileNameOrTemporaryFileName() const {
	return (fileName.isEmpty())
		? temporaryFileName
		: fileName;
}

QFileInfo LatexDocument::getTemporaryFileInfo() const {
	return QFileInfo(temporaryFileName);
}

int LatexDocument::countLabels(const QString & name){

	int count = 0;
	
	for(const auto document : getListOfDocs()){
		auto items = document -> labelItems();
		count += items.count(name);
	}
	
	return count;
}

int LatexDocument::countRefs(const QString & name){

	int count = 0;
	
	for(const auto document : getListOfDocs()){
		auto items = document -> refItems();
		count += items.count(name);
	}
	
	return count;
}

bool LatexDocument::bibIdValid(const QString & name){

	if(!findFileFromBibId(name).isEmpty())
		return true;

	for(const auto document : getListOfDocs())
		if(document -> bibItems().contains(name))
			return true;

	return false;
}

bool LatexDocument::isBibItem(const QString & name){
	
	for(const auto document : getListOfDocs())
		if(document -> bibItems().contains(name))
			return true;

	return false;
}

QString LatexDocument::findFileFromBibId(const QString & bibId){

	const auto & bibtexfiles = parent -> bibTeXFiles;

	for(const auto document : getListOfDocs())
		for(auto file : document -> listOfMentionedBibTeXFiles())
			if(bibtexfiles.value(file).ids.contains(bibId))
				return file;
	
	return "";
}

QMultiHash<QDocumentLineHandle *,int> LatexDocument::getBibItems(const QString & name){
    
	QMultiHash<QDocumentLineHandle *, int> result;
	
	for(const auto document : getListOfDocs()){

		const auto end = document -> mBibItem.constEnd();
		auto it = document -> mBibItem.constBegin();
		
		for(;it != end;++it){

			auto reference = it.value();
			
			if(reference.name == name && document -> indexOf(it.key()) >= 0)
				result.insert(it.key(),reference.start);
		}
	}
	return result;
}

QMultiHash<QDocumentLineHandle *,int> LatexDocument::getLabels(const QString &name){

    QMultiHash<QDocumentLineHandle *,int> result;
	
	for(const auto document : getListOfDocs()){

		const auto end = document -> mLabelItem.constEnd();
		auto it = document -> mLabelItem.constBegin();
		
		for(;it != end;++it){
			
			auto reference = it.value();

			if(reference.name == name && document -> indexOf(it.key()) >= 0)
				result.insert(it.key(),reference.start);
		}
	}

	return result;
}

QDocumentLineHandle * LatexDocument::findCommandDefinition(const QString & name){

	for(const auto document : getListOfDocs()){

		const auto end = document -> mUserCommandList.constEnd(); 
		auto it = document -> mUserCommandList.constBegin();
		
		for(;it != end;++it)
			if(it.value().name == name && document -> indexOf(it.key()) >= 0)
				return it.key();
	}

	return nullptr;
}

QDocumentLineHandle * LatexDocument::findUsePackage(const QString & name){

	for(const auto document : getListOfDocs()){

		const auto end = document -> mUsepackageList.constEnd();
		auto it = document -> mUsepackageList.constBegin();
		
		for(;it != end;++it)
			if(LatexPackage::keyToPackageName(it.value()) == name && document -> indexOf(it.key()) >= 0)
				return it.key();
	}

	return nullptr;
}


QMultiHash<QDocumentLineHandle *,int> LatexDocument::getRefs(const QString & name){
    
	QMultiHash<QDocumentLineHandle *,int> result;
	
	for(const auto document : getListOfDocs()){

		const auto end = document -> mRefItem.constEnd();
		auto it = document -> mRefItem.constBegin();
		
		for(;it != end;++it){

			auto reference = it.value();
			
			if(reference.name == name && document -> indexOf(it.key()) >= 0)
				result.insert(it.key(),reference.start);
		}
	}

	return result;
}


/*!
 * replace all given items by newName
 * an optional QDocumentCursor may be passed in, if the operation should be
 * part of a larger editBlock of that cursor.
 */

void LatexDocument::replaceItems(QMultiHash<QDocumentLineHandle *,ReferencePair> items,const QString & newName,QDocumentCursor * cursor){
	
	auto cur = cursor;
	
	if(!cursor){
		cur = new QDocumentCursor(this);
		cur -> beginEditBlock();
	}
	
	QMultiHash<QDocumentLineHandle *, ReferencePair>::const_iterator it;
	
	int 
		oldLineNr = -1,
		offset = 0;
	
	for(it = items.constBegin();it != items.constEnd();++it){
		
		auto lineHandle = it.key();
		auto reference = it.value();
		
		int lineNo = indexOf(lineHandle);

		if(oldLineNr != lineNo)
			offset = 0;

		if(lineNo >= 0){
			cur -> setLineNumber(lineNo);
			cur -> setColumnNumber(reference.start + offset);
			cur -> movePosition(reference.name.length(),QDocumentCursor::NextCharacter,QDocumentCursor::KeepAnchor);
			cur -> replaceSelectedText(newName);
			offset += newName.length() - reference.name.length();
			oldLineNr = lineNo;
		}
	}

	if(!cursor){
		cur -> endEditBlock();
		delete cur;
	}
}


/*!
 * replace all labels name by newName
 * an optional QDocumentCursor may be passed in, if the operation should be
 * part of a larger editBlock of that cursor.
 */

void LatexDocument::replaceLabel(const QString & name,const QString & newName,QDocumentCursor * cursor){
	
	QMultiHash<QDocumentLineHandle *, ReferencePair> labelItemsMatchingName;
	QMultiHash<QDocumentLineHandle *, ReferencePair>::const_iterator it;
	
	for(it = mLabelItem.constBegin();it != mLabelItem.constEnd();++it)
		if(it.value().name == name)
            labelItemsMatchingName.insert(it.key(),it.value());

	replaceItems(labelItemsMatchingName,newName,cursor);
}


/*!
 * replace all references name by newName
 * an optional QDocumentCursor may be passed in, if the operation should be
 * part of a larger editBlock of that cursor.
 */

void LatexDocument::replaceRefs(const QString & name,const QString & newName,QDocumentCursor * cursor){

	QMultiHash<QDocumentLineHandle *, ReferencePair>::const_iterator it;
	QMultiHash<QDocumentLineHandle *, ReferencePair> refItemsMatchingName;

	for(it = mRefItem.constBegin();it != mRefItem.constEnd();++it)
		if(it.value().name == name)
		    refItemsMatchingName.insert(it.key(),it.value());

	replaceItems(refItemsMatchingName,newName,cursor);
}

void LatexDocument::replaceLabelsAndRefs(const QString & name,const QString & newName){
	
	QDocumentCursor cursor(this);
	
	cursor.beginEditBlock();
	
	replaceLabel(name,newName,& cursor);
	replaceRefs(name,newName,& cursor);
	
	cursor.endEditBlock();
}

void LatexDocument::setMasterDocument(LatexDocument * master,bool recheck){
    
	masterDocument = master;
    
	if(!recheck)
		return;
	
	QList<LatexDocument *>listOfDocs = getListOfDocs();

	QStringList items;
	
	for(const auto document : listOfDocs)
		items << document -> labelItems();

	for(auto document : listOfDocs)
		document -> recheckRefsLabels(listOfDocs,items);
}

void LatexDocument::addChild(LatexDocument * document){
	childDocs.insert(document);
}

void LatexDocument::removeChild(LatexDocument * document){
	childDocs.remove(document);
}

bool LatexDocument::containsChild(LatexDocument * document) const {
	return childDocs.contains(document);
}

QList<LatexDocument *> LatexDocument::getListOfDocs(QSet<LatexDocument *> * visitedDocs){
	
	QList<LatexDocument *> listOfDocs;
	
	bool deleteVisitedDocs = false;
	
	if(parent -> masterDocument)
		listOfDocs = parent -> getDocuments();
	else {

		LatexDocument * master = this;
		
		if(!visitedDocs){
			visitedDocs = new QSet<LatexDocument *>();
			deleteVisitedDocs = true;
		}

		for(auto document : parent -> getDocuments()){ // check children
			
			if(document != master && !master -> childDocs.contains(document))
				continue;

			if(visitedDocs && !visitedDocs -> contains(document)){
				listOfDocs << document;
				visitedDocs -> insert(document);
				listOfDocs << document -> getListOfDocs(visitedDocs);
			}
		}

		if(masterDocument){ //check masters

			master = masterDocument;
		
			if(!visitedDocs -> contains(master))
				listOfDocs << master -> getListOfDocs(visitedDocs);
		}
	}

	if(deleteVisitedDocs)
		delete visitedDocs;

	return listOfDocs;
}


void LatexDocument::updateRefHighlight(ReferencePairEx reference){

	const auto lineHandle = reference.dlh;

	const auto & formatList = reference.formatList;
	const auto & lengths = reference.lengths;
	const auto & formats = reference.formats;
	const auto & starts = reference.starts;

	lineHandle -> clearOverlays(formatList);

	for(int i = 0;i < starts.size();++i)
		lineHandle -> addOverlay(QFormatRange(starts[i],lengths[i],formats[i]));
}

void LatexDocument::recheckRefsLabels(QList<LatexDocument*> listOfDocs,QStringList items){

	// get occurences (refs)

	int 
		referenceMultipleFormat = getFormatId("referenceMultiple"),
		referencePresentFormat = getFormatId("referencePresent"),
		referenceMissingFormat = getFormatId("referenceMissing");
    
	const QList<int> formatList {
		referenceMissingFormat ,
		referencePresentFormat ,
		referenceMultipleFormat
	};
    
	QList<ReferencePairEx> results;


	// if not empty, assume listOfDocs *and* items are provided.
    // this avoid genearting both lists for each document again

    if(listOfDocs.isEmpty())
        for(const auto document : getListOfDocs())
            items << document -> labelItems();

	QMultiHash<QDocumentLineHandle *,ReferencePair>::const_iterator it;
	QSet<QDocumentLineHandle*> lineHandles;

	for(it = mLabelItem.constBegin();it != mLabelItem.constEnd();++it)
        lineHandles.insert(it.key());
	
    for(it = mRefItem.constBegin();it != mRefItem.constEnd();++it)
        lineHandles.insert(it.key());

    for(auto lineHandle : lineHandles){

        ReferencePairEx reference;
        reference.formatList = formatList;
        reference.dlh = lineHandle;
        
        for(const ReferencePair & rp : mLabelItem.values(lineHandle)){

            int count = items.count(rp.name);
            int format = referenceMissingFormat;
            
			if(count > 1)
                format = referenceMultipleFormat;
            else 
			if(count == 1)
				format = referencePresentFormat;
			
            reference.starts << rp.start;
            reference.lengths << rp.name.length();
            reference.formats << format;
		}

        for(const auto & item : mRefItem.values(lineHandle)){

            int count = items.count(item.name);
            int format= referenceMissingFormat;
            
			if(count > 1)
                format = referenceMultipleFormat;
            else
			if(count == 1)
				format = referencePresentFormat;
            
			reference.starts << item.start;
            reference.lengths << item.name.length();
            reference.formats << format;
        }

		results << reference;
	}

    QtConcurrent::blockingMap(results,LatexDocument::updateRefHighlight);
}

QStringList LatexDocument::someItems(const QMultiHash<QDocumentLineHandle *,ReferencePair> & list){
	
	auto lst = list.values();
	QStringList result;
	
	for(const auto & elem : lst)
		result << elem.name;

	return result;
}


QStringList LatexDocument::labelItems() const {
	return someItems(mLabelItem);
}

QStringList LatexDocument::refItems() const {
	return someItems(mRefItem);
}

QStringList LatexDocument::bibItems() const {
	return someItems(mBibItem);
}

QList<CodeSnippet> LatexDocument::userCommandList() const {

	QList<CodeSnippet> snippets;
    
	for(auto command : mUserCommandList)
		snippets.append(command.snippet);

    std::sort(snippets.begin(),snippets.end());

	return snippets;
}


void LatexDocument::updateRefsLabels(const QString & ref){

	// get occurences (refs)

	int 
		referenceMultipleFormat = getFormatId("referenceMultiple"),
		referencePresentFormat = getFormatId("referencePresent"),
		referenceMissingFormat = getFormatId("referenceMissing");

    const QList<int> formatList { 
        referenceMissingFormat ,
        referencePresentFormat ,
		referenceMultipleFormat
	};

	int cnt = countLabels(ref);
	
	auto occurences = getLabels(ref);
	occurences += getRefs(ref);
	
	QMultiHash<QDocumentLineHandle *, int>::const_iterator it;
	
	for(it = occurences.constBegin();it != occurences.constEnd();++it){
		
		auto lineHandle = it.key();
        lineHandle -> clearOverlays(formatList);
        
		for(const int pos : occurences.values(lineHandle))
			lineHandle -> addOverlay(QFormatRange(pos,ref.length(),
				(cnt  > 1) ? referenceMultipleFormat : 
				(cnt == 1) ? referencePresentFormat : referenceMissingFormat
			));
	}
}



LatexDocuments::LatexDocuments()
	: currentDocument(nullptr)
	, masterDocument(nullptr)
	, bibTeXFilesModified(false) {
	
	showCommentedElementsInStructure = false;
	markStructureElementsInAppendix = true;
	markStructureElementsBeyondEnd = true;
	showLineNumbersInStructure = false;
	indentIncludesInStructure = false;
	m_patchEnabled = true;

	indentationInStructure = -1;
}

void LatexDocuments::addDocument(LatexDocument * document,bool hidden){
	
	if(hidden){
		
		hiddenDocuments.append(document);
		
		auto view = document -> getEditorView();

		if(view){

			auto editor = view -> getEditor();

			if(editor){
				document -> remeberAutoReload = editor -> silentReloadOnExternalChanges();
				editor -> setSilentReloadOnExternalChanges(true);
				editor -> setHidden(true);
			}
		}
	} else {
		documents.append(document);
	}

	connect(document,SIGNAL(updateBibTeXFiles()),SLOT(bibTeXFilesNeedUpdate()));
	
	document -> parent = this;
	
	if(masterDocument)
		// repaint all docs
		for(const auto document : documents){
			
			auto view = document -> getEditorView();
			
			if(view)
				view -> documentContentChanged(0,view -> editor -> document() -> lines());
		}
}


void LatexDocuments::deleteDocument(LatexDocument * document,bool hidden,bool purge){
    
	if(!hidden)
        emit aboutToDeleteDocument(document);
    
	auto view = document -> getEditorView();
    
	if(view)
        view -> closeCompleter();
    
	if((document != masterDocument) || (documents.count() == 1)){

        // get list of all affected documents
        auto lstOfDocs = document -> getListOfDocs();
        
		// special treatment to remove document in purge mode (hidden doc was deleted on disc)
        if(purge){

            Q_ASSERT(hidden); //purging non-hidden doc crashes.
            
			auto rootDoc = document -> getRootDocument();
            
			hiddenDocuments.removeAll(document);
            
            for(auto doc : getDocuments())
                if(doc -> containsChild(document))
                    doc -> removeChild(document);

            //update children (connection to parents is severed)
            for(auto elem : lstOfDocs)
                if(elem -> getMasterDocument() == document)
                    if(elem -> isHidden())
                        deleteDocument(elem,true,true);
                    else
                        elem -> setMasterDocument(nullptr);


            delete document;

            if(rootDoc != document){

                // update parents
                lstOfDocs = rootDoc -> getListOfDocs();

                int n = 0;

                for(auto elem : lstOfDocs)
                    if(!elem -> isHidden()){
                        n++;
                        break;
                    }

                if(n == 0)
                    deleteDocument(rootDoc,true,true);
                else
                    updateMasterSlaveRelations(rootDoc,true,true);
            }

            return;
        }

        // count open related (child/parent) documents
        int n = 0;

        for(auto elem : lstOfDocs)
            if(!elem -> isHidden())
                n++;

        if(hidden){
            hiddenDocuments.removeAll(document);
            return;
        }

        if(n > 1){ // at least one related document will be open after removal
            
			hiddenDocuments.append(document);
            
			auto edView = document -> getEditorView();
            
			if(edView){
                auto editor = edView -> getEditor();
                
				if(editor){
                    document -> remeberAutoReload = editor -> silentReloadOnExternalChanges();
                    editor -> setSilentReloadOnExternalChanges(true);
                    editor -> setHidden(true);
                }
            }
        } else {

            // no open document remains, remove all others as well
            for(auto doc : getDocuments())
                if(doc -> containsChild(document))
                    doc -> removeChild(document);
            
			for(auto doc : lstOfDocs)
                if(doc -> isHidden()){
                    hiddenDocuments.removeAll(doc);
                    delete doc -> getEditorView();
                    delete doc;
                }
        }

        documents.removeAll(document);

        if(document == currentDocument)
            currentDocument = nullptr;

        if(n > 1){ // don't remove document, stays hidden instead
            
			hideDocInEditor(document -> getEditorView());
            
			if(masterDocument && documents.count() == 1){
                // special check if masterDocument, but document is not visible
                auto doc = documents.first();
                
				if(!doc -> getEditorView())
                    // no view left -> purge
                    deleteDocument(masterDocument);
            }
            
			return;
        }

        delete view;
        delete document;
    } else {

        if(hidden){
            hiddenDocuments.removeAll(document);
            return;
        }

        document -> setFileName(document -> getFileName());
        document -> clearAppendix();

        delete view;
        
		if(document == currentDocument)
            currentDocument = nullptr;
    }

    // purge masterdocument if none is left
    if(documents.isEmpty()){

        if(masterDocument)
            masterDocument=nullptr;

        hiddenDocuments.clear();
    }
}

void LatexDocuments::requestedClose(){
	
	auto editor = qobject_cast<QEditor *>(sender());
	auto document = qobject_cast<LatexDocument *>(editor -> document());

	deleteDocument(document,true,true);
}


/*!
 * \brief set \param document as new master document
 * Garcefully close old master document if set and set document as new master
 * \param document
 */

void LatexDocuments::setMasterDocument(LatexDocument * document){

	if(document == masterDocument)
		return;
	
	if(masterDocument != nullptr && masterDocument -> getEditorView() == nullptr){

		QString fn = masterDocument -> getFileName();
		
		addDocToLoad(fn);
		
		auto doc = masterDocument;
		masterDocument = nullptr;
		deleteDocument(doc);
	}

	masterDocument = document;
	
	if(masterDocument != nullptr){

		documents.removeAll(masterDocument);
		documents.prepend(masterDocument);
		
		// repaint doc
		for(auto doc : documents){

			auto view = doc -> getEditorView();
            
			if(view)
				view -> documentContentChanged(0,doc -> lines());
		}
	}

	emit masterDocumentChanged(masterDocument);
}


/*!
 * \brief return current document
 * \return current document
 */

LatexDocument * LatexDocuments::getCurrentDocument() const {
	return currentDocument;
}

/*!
 * \brief return master document if one is set
 * \return masterDocument
 */

LatexDocument * LatexDocuments::getMasterDocument() const {
	return masterDocument;
}

/*!
 * \brief return list of *all* open documents
 * This includes visibles and hidden documents in memory
 * \return list of documents
 */

QList<LatexDocument *> LatexDocuments::getDocuments() const {
    return documents + hiddenDocuments;
}

void LatexDocuments::move(int from,int to){
	documents.move(from,to);
}


/*!
 * \brief get file name of current document
 * \return file name
 */

QString LatexDocuments::getCurrentFileName() const {
	return (currentDocument)
		? currentDocument -> getFileName()
		: "";
}

QString LatexDocuments::getCompileFileName() const {

	if(masterDocument)
		return masterDocument -> getFileName();
	
	if(!currentDocument)
		return "";
	
	// check for magic comment
	QString curDocFile = currentDocument -> getMagicComment("root");
	
	if(curDocFile.isEmpty())
		curDocFile = currentDocument -> getMagicComment("texroot");

	if(!curDocFile.isEmpty())
		return currentDocument -> findFileName(curDocFile);

	const auto rootDoc = currentDocument -> getRootDocument();
	
	return (rootDoc)
		? rootDoc -> getFileName()
		: currentDocument -> getFileName();
}

QString LatexDocuments::getTemporaryCompileFileName() const {
	
	QString temp = getCompileFileName();
	
	if(!temp.isEmpty())
		return temp;
	
	if(masterDocument)
		return masterDocument -> getTemporaryFileName();

	if(currentDocument)
		return currentDocument -> getTemporaryFileName();
	
	return "";
}

QString LatexDocuments::getLogFileName() const {

	if(!currentDocument)
		return "";

	auto rootDoc = currentDocument -> getRootDocument();

	QString jobName = rootDoc -> getMagicComment("-job-name");

	if(jobName.isEmpty())
		return replaceFileExtension(getTemporaryCompileFileName(), ".log");
	else
		return ensureTrailingDirSeparator(rootDoc -> getFileInfo().absolutePath()) + jobName + ".log";
}

QString LatexDocuments::getAbsoluteFilePath(const QString & relName,const QString & extension,const QStringList & additionalSearchPaths) const {
	
	if(currentDocument)
		return currentDocument -> getAbsoluteFilePath(relName,extension,additionalSearchPaths);

	return relName;
}

LatexDocument * LatexDocuments::findDocumentFromName(const QString & fileName) const {

	for(auto document : getDocuments())
		if(document -> getFileName() == fileName)
			return document;

    return nullptr;
}


/*!
 * Adjust the internal order of documents to the given order.
 * \param order should contain exactly the same documents as this.
 */

void LatexDocuments::reorder(const QList<LatexDocument *> & order){

	if(order.size() != documents.size())
		qDebug() << "Warning: Size of list of documents for reordering differs from current documents";

	for(auto document : order){

		int n = documents.removeAll(document);
		
		if(n > 1)
			qDebug() << "Warning: document listed multiple times in LatexDocuments";
		
		if(n < 1)
			qDebug() << "Warning: encountered a document that is not listed in LatexDocuments";

		documents.append(document);
	}
}

LatexDocument * LatexDocuments::findDocument(const QDocument * document) const {

	for(auto doc : documents){
		auto view = doc -> getEditorView();

		if(view && view -> editor -> document() == document)
			return doc;
	}

	return nullptr;
}

LatexDocument * LatexDocuments::findDocument(const QString & fileName,bool checkTemporaryNames) const {
    
	if(fileName == "")
		return nullptr;
	
	if(checkTemporaryNames){
		
		auto temp = findDocument(fileName,false);
		
		if(temp)
			return temp;
	}

	QFileInfo fi(fileName);
	fi = getNonSymbolicFileInfo(fi);

	if(fi.exists()){

		for(auto document : documents)
			if(document -> getFileInfo() == fi)
				return document;

		if(checkTemporaryNames)
			for(auto document : documents)
				if(document -> getFileName().isEmpty() && document -> getTemporaryFileInfo() == fi)
					return document;
	}

	//check for relative file names
	fi.setFile(getAbsoluteFilePath(fileName));

	if(!fi.exists())
		fi.setFile(getAbsoluteFilePath(fileName),".tex");

	if(!fi.exists())
		fi.setFile(getAbsoluteFilePath(fileName),".bib");

	if(fi.exists())
		for(auto document : documents)
			if(document -> getFileInfo().exists() && document -> getFileInfo() == fi)
				return document;

    return nullptr;
}

void LatexDocuments::settingsRead(){
	return; // currently unused
}

bool LatexDocuments::singleMode() const {
	return ! masterDocument;
}

void LatexDocuments::updateBibFiles(bool updateFiles){

	mentionedBibTeXFiles.clear();
	
	auto additionalBibPaths = ConfigManagerInterface::getInstance()
		-> getOption("Files/Bib Paths")
		.toString()
		.split(getPathListSeparator());
	
	for(auto document : getDocuments())
		if(updateFiles){
			
			auto it = document -> mentionedBibTeXFiles().begin();
			auto itend = document -> mentionedBibTeXFiles().end();
			
			for(;it != itend;++it) {
				
				it.value().absolute = getAbsoluteFilePath(it.value().relative,".bib",additionalBibPaths)
					.replace(QDir::separator(),'/'); // update absolute path
				
				mentionedBibTeXFiles << it.value().absolute;
			}
		}

	if(updateFiles){

		auto bibFileEncoding = ConfigManagerInterface::getInstance()
			-> getOption("Bibliography/BibFileEncoding")
			.toString();
		
		auto defaultCodec = QTextCodec::codecForName(bibFileEncoding.toLatin1());
		
		for(int i = 0;i < mentionedBibTeXFiles.count();i++){

			auto & fileName = mentionedBibTeXFiles[i];
			
			QFileInfo fi(fileName);
			
			if(!fi.isReadable())
				continue; //ups...
			
			if(!bibTeXFiles.contains(fileName))
				bibTeXFiles.insert(fileName,BibTex::FileInfo());
			
			auto & bibTex = bibTeXFiles[mentionedBibTeXFiles[i]];

			// TODO: allow to use the encoding of the tex file which mentions the bib file (need to port this information from above)
			
			bibTex.codec = defaultCodec;
            bibTex.loadIfModified(QFileInfo(fileName));

			//handle obscure bib tex feature, a just line containing "link fileName"

			if(bibTex.ids.empty() && !bibTex.linksTo.isEmpty())
				mentionedBibTeXFiles.append(bibTex.linksTo);
		}
	}
}

void LatexDocuments::removeDocs(QStringList removeIncludes){

	for(auto & filename : removeIncludes){

		auto doc = findDocumentFromName(filename);
		
		if(!doc)
			continue;
			
		
		for(auto document : getDocuments())
			if(document -> containsChild(doc)){

				document -> removeChild(doc);
				
				if(!document -> labelItems().isEmpty())
					document -> recheckRefsLabels();
			}

		if(!doc -> isHidden())
			continue;

		auto toRemove = doc -> includedFiles();
		doc -> setMasterDocument(nullptr,false);
		hiddenDocuments.removeAll(doc);

		delete doc -> getEditorView();
		delete doc;

		if(!toRemove.isEmpty())
			removeDocs(toRemove);
	}
}

void LatexDocuments::addDocToLoad(QString filename){
	emit docToLoad(filename);
}

void LatexDocuments::hideDocInEditor(LatexEditorView * view){
	emit docToHide(view);
}

int LatexDocument::findStructureParentPos(const QList<StructureEntry *> & children,QList<StructureEntry *> & removedElements,int linenr,int count){
	
	QListIterator<StructureEntry *> iter(children);
	int parentPos = 0;
	
	while(iter.hasNext()){
		
		auto entry = iter.next();
		int realline = entry -> getRealLineNumber();
		
		Q_ASSERT(realline >= 0);
		
		if(realline >= linenr + count)
			break;

		if(realline >= linenr)
			removedElements.append(entry);

		++parentPos;
	}

	return parentPos;
}


void LatexStructureMergerMerge::mergeStructure(StructureEntry * entry){

	if(!entry)
		return;
	
	if(
		entry -> type != StructureEntry::SE_DOCUMENT_ROOT && 
		entry -> type != StructureEntry::SE_SECTION &&
		entry -> type != StructureEntry::SE_INCLUDE
	) return;
	
	int se_line = entry -> getRealLineNumber();
	
	if(se_line < linenr || entry -> type == StructureEntry::SE_DOCUMENT_ROOT){

		//se is before updated region, but children might still be in it
		updateParentVector(entry);

		int start = -1;
		
		for(int i = 0;i < entry -> children.size();i++){
			
			auto c = entry -> children[i];
			
			if(
				c -> type != StructureEntry::SE_SECTION && 
				c -> type != StructureEntry::SE_INCLUDE
			) continue;
			
			if(c -> getRealLineNumber() < linenr)
				updateParentVector(c);

			start = i;
			break;
		}

		if(start >= 0){
			
			if(start > 0)
				start--;
			
			mergeChildren(entry,start);
		}
	} else {

		//se is within or after the region
		// => insert flatStructure.first() before se or replace se with it (don't insert after since there might be another "se" to replace)
		
		while(
			!flatStructure -> isEmpty() && 
			entry -> getRealLineNumber() >= flatStructure -> first() -> getRealLineNumber()
		){
			auto next = flatStructure -> takeFirst();
			
			if(entry -> getRealLineNumber() == next -> getRealLineNumber()){

				//copy next to se except for parent/children
				next -> parent = entry -> parent;
				next -> children = entry -> children;
				* entry = * next;

 				next -> children.clear();

				delete next;
				
				document -> updateElementWithSignal(entry);
				
				moveToAppropiatePositionWithSignal(entry);
				
				mergeChildren(entry);
				return;
			}

			moveToAppropiatePositionWithSignal(next);
		}

		if(se_line < linenr + count)
			//se is within the region (delete if necessary and then merge children)
			if(
				flatStructure -> isEmpty() || 
				entry -> getRealLineNumber() < flatStructure -> first() -> getRealLineNumber()
			){
				auto oldChildren = entry -> children;
				int oldrow = entry -> getRealParentRow();

				for(int i = entry -> children.size() - 1;i >= 0;i--)
					document -> moveElementWithSignal(entry -> children[i],entry -> parent,oldrow);

				document -> removeElementWithSignal(entry);
				
				delete entry;
				
				for(int i = 1;i < parent_level.size();i++)
					if(parent_level[i] == entry)
						parent_level[i] = parent_level[i - 1];
				
				for(auto next : oldChildren)
					mergeStructure(next);

				return;
			}

		//se not replaced or deleted => se is after everything the region => keep children
		moveToAppropiatePositionWithSignal(entry);
		
		for(auto child : entry -> children)
			moveToAppropiatePositionWithSignal(child);
	}

	//insert unprocessed elements of flatStructure at the end of the structure
	if(entry -> type == StructureEntry::SE_DOCUMENT_ROOT && ! flatStructure -> isEmpty()){

		for(auto entry : * flatStructure){
			document -> addElementWithSignal(parent_level[entry -> level],entry);
			updateParentVector(entry);
		}

		flatStructure -> clear();
	}
}

void LatexStructureMergerMerge::mergeChildren(StructureEntry * entry,int start){
	
	 //need to cow-protected list, in case se will be changed by mergeStructure
	 auto oldChildren = entry -> children;
	
	 for(int i = start;i < oldChildren.size();i++)
		mergeStructure(oldChildren[i]);
}

bool LatexDocument::IsInTree(StructureEntry * entry){

	Q_ASSERT(entry);
	
	while(entry){
		
		if(entry -> type == StructureEntry::SE_DOCUMENT_ROOT)
			return true;
		
		entry = entry -> parent;
	}

	return false;
}

void LatexDocument::removeElementWithSignal(StructureEntry * entry){
	
	int sendSignal = IsInTree(entry);
	int parentRow = entry -> getRealParentRow();
	
	REQUIRE(parentRow >= 0);
	
	if(sendSignal)
		emit removeElement(entry,parentRow);
	
	entry -> parent -> children.removeAt(parentRow);
	entry -> parent = nullptr;
	
	if(sendSignal)
		emit removeElementFinished();
}

void LatexDocument::addElementWithSignal(StructureEntry * parent,StructureEntry * entry){
	
	int sendSignal = IsInTree(parent);
	
	if(sendSignal)
		emit addElement(parent,parent -> children.size());

	parent -> children.append(entry);
	entry -> parent = parent;

	if(sendSignal)
		emit addElementFinished();
}

void LatexDocument::insertElementWithSignal(StructureEntry * parent,int pos,StructureEntry * entry){
	
	int sendSignal = IsInTree(parent);
	
	if(sendSignal)
		emit addElement(parent,pos);
	
	parent -> children.insert(pos,entry);
	entry -> parent = parent;

	if(sendSignal)
		emit addElementFinished();
}

void LatexDocument::moveElementWithSignal(StructureEntry * entry,StructureEntry * parent,int pos){
	removeElementWithSignal(entry);
	insertElementWithSignal(parent,pos,entry);
}

void LatexStructureMerger::updateParentVector(StructureEntry * entry){

	REQUIRE(entry);

	switch(entry -> type){
	case StructureEntry::SE_DOCUMENT_ROOT :

		parent_level.fill(document -> baseStructure);
		return;
	case StructureEntry::SE_INCLUDE : {

		const auto parent = document -> parent;

		if(!parent || parent -> indentIncludesInStructure)
			return;

		parent_level.fill(document -> baseStructure);
		return;
	}
	case StructureEntry::SE_SECTION :

		for(int j = entry -> level + 1;j < parent_level.size();j++)
			parent_level.replace(j,entry);
	
		return;
	default:
		return;
	}
}


class LessThanRealLineNumber {
	
	public:
		
		inline bool operator () (const StructureEntry * const a,const StructureEntry * const b) const {

			const auto
				lineA = a -> getRealLineNumber(),
				lineB = b -> getRealLineNumber();

			if(lineA < lineB)
				return true;

			return lineA == lineB &&
				a -> columnNumber < b -> columnNumber;
		}
};


void LatexStructureMerger::moveToAppropiatePositionWithSignal(StructureEntry * se){

	REQUIRE(se);
	
	auto newParent = parent_level.value(se -> level,nullptr);
	
	if(!newParent){
		qDebug("structure update failed!");
		return;
	}

	int oldPos , newPos;
	
	LessThanRealLineNumber compare;
	
	if(se -> parent == newParent){

		//the construction somehow ensures that in this case
		//se is already at the correct position regarding line numbers.
		//but not necessarily regarding the column position
		
		oldPos = se -> getRealParentRow();
		
		if(
			(oldPos == 0 || compare(newParent -> children[oldPos - 1],se)) &&
			(oldPos == newParent -> children.size() - 1 || compare(se,newParent -> children[oldPos + 1]))
		){
			newPos = oldPos;
		} else {
           
		    newPos = std::upper_bound(newParent -> children.begin(),newParent -> children.end(),se,compare) - newParent -> children.begin();
			
			while (
				newPos > 0 &&
				newParent -> children[newPos - 1] -> getRealLineNumber() == se -> getRealLineNumber() &&
				newParent -> children[newPos - 1] -> columnNumber == se -> columnNumber
			) newPos--; //upperbound always returns the position after se if it is in newParent->children
		}
	} else {
		
		oldPos = -1;
		
		if(
			newParent -> children.size() > 0 &&
			newParent -> children.last() -> getRealLineNumber() >= se -> getRealLineNumber()
		)
            newPos = std::upper_bound(newParent->children.begin(), newParent->children.end(), se, compare) - newParent->children.begin();
		else
			newPos = newParent->children.size();
	}


	if(se -> parent) {
		if(newPos != oldPos)
			document -> moveElementWithSignal(se,newParent,newPos);
	} else
		document -> insertElementWithSignal(newParent,newPos,se);

	updateParentVector(se);

	return;
}


/*!
  Splits a [name] = [val] string into \a name and \a val removing extra spaces.

  \return true if splitting successful, false otherwise (in that case name and val are empty)
 */

bool LatexDocument::splitMagicComment(const QString & comment,QString & name,QString & val){

	int sep = comment.indexOf('=');
	
	if(sep < 0)
		return false;
	
	name = comment.left(sep).trimmed();
	val = comment.mid(sep + 1).trimmed();
	
	return true;
}


/*!
  Used by the parser to add a magic comment

\a text is the comment without the leading "! TeX" declaration. e.g. "spellcheck = DE-de"
\a lineNr - line number of the magic comment
\a position - Zero-based position of magic comment in the structure list tree view.
  */

void LatexDocument::addMagicComment(const QString & text,int lineNr,int position){

	auto newMagicComment = new StructureEntry(this,StructureEntry::SE_MAGICCOMMENT);
	auto lineHandle = line(lineNr).handle();
	
	QString name , val;

	splitMagicComment(text,name,val);
	parseMagicComment(name,val,newMagicComment);

	newMagicComment -> title = text;
	newMagicComment -> setLine(lineHandle,lineNr);
	
	insertElementWithSignal(magicCommentList,position,newMagicComment);
}


/*!
  Formats the StructureEntry and modifies the document according to the MagicComment contents
  */

void LatexDocument::parseMagicComment(const QString & name,const QString & val,StructureEntry * se){

	se -> valid = false;
	se -> tooltip = "";

	QString lowerName = name.toLower();

	if(lowerName == "spellcheck"){
		mSpellingDictName = val;
		emit spellingDictChanged(mSpellingDictName);
		se -> valid = true;
	} else 
	if((lowerName == "texroot") || (lowerName == "root")){

		QString fname = findFileName(val);
		auto dc = parent->findDocumentFromName(fname);
		
		if(dc){
			dc -> childDocs.insert(this);
			setMasterDocument(dc);
		} else {
			parent -> addDocToLoad(fname);
		}

		se -> valid = true;
	} else
	if(lowerName == "encoding") {
		
		auto codec = QTextCodec::codecForName(val.toLatin1());
		
		if(!codec){
			se -> tooltip = tr("Invalid codec");
			return;
		}

		setCodecDirect(codec);
		emit encodingChanged();
		se -> valid = true;
	} else 
	if(lowerName == "txs-script")
		se -> valid = true;
	else 
	if(lowerName == "program")
		se -> valid = true;
	else
	if(lowerName == "ts-program")
		se -> valid = true;
	if(lowerName.startsWith("txs-program:"))
		se -> valid = true;
	else 
	if(lowerName == "-job-name"){
		if(!val.isEmpty())
			se -> valid = true;
		else
			se -> tooltip = tr("Missing value for -job-name");
	} else {
		se -> tooltip = tr("Unknown magic comment");
	}
}


void LatexDocument::updateContext(QDocumentLineHandle * oldLine,QDocumentLineHandle * newLine,StructureEntry::Context context){
	
	int endLine = (newLine) 
		? indexOf(newLine) 
		: -1 ;
	
	int startLine = -1;
	
	if(oldLine){

		startLine = indexOf(oldLine);
		
		if(endLine < 0 || endLine > startLine) {
			// remove appendic marker
			auto se = baseStructure;
			setContextForLines(se,startLine,endLine,context,false);
		}
	}

	if(endLine > -1 && (endLine < startLine || startLine < 0)){
		auto se = baseStructure;
		setContextForLines(se,endLine,startLine,context,true);
	}
}


void LatexDocument::setContextForLines(StructureEntry * se,int startLine,int endLine,StructureEntry::Context context,bool state){

	bool first = false;
	
	for(int i = 0;i < se -> children.size(); i++) {
		
		auto elem = se -> children[i];
		
		if(endLine >= 0 && elem -> getLineHandle() && elem -> getRealLineNumber() > endLine)
			break;
		
		if(elem -> type == StructureEntry::SE_SECTION && elem -> getRealLineNumber() > startLine){

			if(!first && i > 0)
				setContextForLines(se -> children[i - 1],startLine,endLine,context,state);
			
			elem -> setContext(context,state);
			
			emit updateElement(elem);
			
			setContextForLines(se -> children[i],startLine,endLine,context,state);
			
			first = true;
		}
	}

	if(!first && !se -> children.isEmpty()){
		
		auto elem = se -> children.last();
		
		if(elem -> type == StructureEntry::SE_SECTION)
			setContextForLines(elem,startLine,endLine,context,state);
	}
}

bool LatexDocument::fileExits(QString filename){

	QString cwd = ensureTrailingDirSeparator(getFileInfo().absolutePath());

	const auto doesExist = [ & ](auto directory,auto extension){
		return QFile(getAbsoluteFilePath(directory + filename,extension))
			.exists();
	};
	
	return 
		doesExist("",".tex") ||
		doesExist(cwd,".tex") ||
		doesExist(cwd,"");
}


/*!
 * A line snapshot is a list of DocumentLineHandles at a given time.
 * For example, this is used to reconstruct the line number at latex compile time
 * allowing syncing from PDF to the correct source line also after altering the source document
 */

void LatexDocument::saveLineSnapshot(){

	for(auto lineHandle : mLineSnapshot)
		lineHandle -> deref();

	mLineSnapshot.clear();
	mLineSnapshot.reserve(lineCount());
	
	auto 
		it = begin(),
		e = end();
	
	while(it != e){

		mLineSnapshot.append(* it);
		
		(*it) -> ref();
		
		it++;
	}
}


// get the line with given lineNumber (0-based) from the snapshot

QDocumentLine LatexDocument::lineFromLineSnapshot(int line){
	
	if(line < 0)
		return QDocumentLine();

	if(line >= mLineSnapshot.count())
		return QDocumentLine();
	
	return QDocumentLine(mLineSnapshot.at(line));
}


// returns the 0-based number of the line in the snapshot, or -1 if line is not in the snapshot

int LatexDocument::lineToLineSnapshotLineNumber(const QDocumentLine & line){
	return mLineSnapshot.indexOf(line.handle());
}

QString LatexDocument::findFileName(QString filename){

	QString cwd = ensureTrailingDirSeparator(getFileInfo().absolutePath());
	
	QString result;
	
	if(QFile(getAbsoluteFilePath(filename,".tex")).exists())
		result = QFileInfo(getAbsoluteFilePath(filename,".tex"))
			.absoluteFilePath();
	
	if(result.isEmpty() && QFile(getAbsoluteFilePath(cwd + filename,".tex")).exists())
		result = QFileInfo(getAbsoluteFilePath(cwd + filename,".tex"))
			.absoluteFilePath();
	
	if(result.isEmpty() && QFile(getAbsoluteFilePath(cwd + filename,"")).exists())
		result = QFileInfo(getAbsoluteFilePath(cwd + filename,""))
			.absoluteFilePath();
	
	return result;
}

void LatexDocuments::bibTeXFilesNeedUpdate(){
	bibTeXFilesModified = true;
}


/*!
 * \brief update parent/child relations
 * doc is removed and the child settings needs to be adapted
 * \param doc
 * \param recheckRefs
 * \param updateCompleterNow
 */

void LatexDocuments::updateMasterSlaveRelations(LatexDocument * doc,bool recheckRefs,bool updateCompleterNow){
	
	//update Master/Child relations
	//remove old settings ...
	
	doc -> setMasterDocument(nullptr,false);

    const auto documents = getDocuments();
    
	QSet<LatexDocument *> removeCandidates;
	
	for(auto document : documents)
		if(document -> getMasterDocument() == doc)
            removeCandidates.insert(document);

	//check whether document is child of other docs
    QString file = doc -> getFileName();

	for(auto document : documents){

		if(document == doc)
			continue;
		
		auto includedFiles = document -> includedFiles();
        
		if(!includedFiles.contains(file))
			continue;

        if(!document -> containsChild(doc))
			document -> addChild(doc);

        doc -> setMasterDocument(document,false);
    }

	// check for already open child documents (included in this file)
	
	for(const auto & file : (doc -> includedFiles())){
        
		auto child = this -> findDocumentFromName(file);
        
		if(!child)
			continue;

		if(removeCandidates.contains(child))
			removeCandidates.remove(child);

		if(!doc -> containsChild(child)){
			
			doc -> addChild(child);
			child -> setMasterDocument(doc,false);
			
			if(recheckRefs)
				child -> reCheckSyntax(); // redo syntax checking (in case of defined commands)
        }
    }

    for(auto candidate : removeCandidates){
        doc -> removeChild(candidate);
        candidate -> setMasterDocument(nullptr,recheckRefs);
    }

	//recheck references

    if(recheckRefs)
        doc -> recheckRefsLabels();

    if(updateCompleterNow)
		doc -> emitUpdateCompleter();
}


const LatexDocument * LatexDocument::getRootDocument(QSet<const LatexDocument *> * visitedDocs) const {
	
	// special handling if explicit master is set
    
	if(!parent)	
		return nullptr;
	
	if(parent && parent -> masterDocument)
		return parent -> masterDocument;
	
	const LatexDocument * result = this;
	bool deleteVisitedDocs = false;
	
	if(!visitedDocs){
		visitedDocs = new QSet<const LatexDocument *>();
		deleteVisitedDocs = true;
	}
	
	visitedDocs -> insert(this);
	
	if(masterDocument && ! visitedDocs -> contains(masterDocument))
		result = masterDocument -> getRootDocument(visitedDocs);

    if(result -> getFileName().endsWith("bib"))
        for(const auto document : parent -> documents){
	
			auto it = document -> mentionedBibTeXFiles().constBegin();
			auto itend = document -> mentionedBibTeXFiles().constEnd();
	
			for(;it != itend;++it)
				if(it.value().absolute == result -> getFileName()){
					result = document -> getRootDocument(visitedDocs);
					break;
				}
			
			if(result == document)
				break;
		}

	if(deleteVisitedDocs)
		delete visitedDocs;
	
	return result;
}

LatexDocument * LatexDocument::getRootDocument(){
    return const_cast<LatexDocument *>(getRootDocument(nullptr));
}

QStringList LatexDocument::includedFiles(){

	QStringList files;
	
	for(const auto & file : mIncludedFilesList.values()){
		
		if(file.isEmpty())
			continue;
			
		if(files.contains(file))
			continue;
			
		files << file;
	}

	return files;
}


QStringList LatexDocument::includedFilesAndParent(){

	auto included = includedFiles();
	auto comment = getMagicComment("root");
	
	if(!comment.isEmpty() && !included.contains(comment))
		included << comment;
	
	comment = getMagicComment("texroot");
	
	if(!comment.isEmpty() && !included.contains(comment))
		included << comment;
	
	if(masterDocument && !included.contains(masterDocument -> getFileName()))
		included << masterDocument -> getFileName();
	
	return included;
}

CodeSnippetList LatexDocument::additionalCommandsList(){

	LatexPackage package;
	QStringList loadedFiles;

    auto files = mCWLFiles.values();

	gatherCompletionFiles(files,loadedFiles,package,true);
	
	return package.completionWords;
}

bool LatexDocument::updateCompletionFiles(const bool forceUpdate,const bool forceLabelUpdate,const bool delayUpdate,const bool dontPatch){

	QStringList files = mUsepackageList.values();

	bool update = forceUpdate;
	
	auto & parser = LatexParser::getInstance();

	//recheck syntax of ALL documents ...
	LatexPackage package;
	package.commandDescriptions = parser.commandDefs;
	package.specialDefCommands = parser.specialDefCommands;
	
	QStringList loadedFiles;
	
	for(int i = 0;i < files.count();i++)
		if(!files.at(i).endsWith(".cwl"))
			files[i] = files[i] + ".cwl";

	gatherCompletionFiles(files,loadedFiles,package);

	update = true;

    mCWLFiles = convertStringListtoSet(loadedFiles);
	
	auto userCommandsForSyntaxCheck = ltxCommands.possibleCommands["user"];
	auto columntypeForSyntaxCheck = ltxCommands.possibleCommands["%columntypes"];
	
	ltxCommands.specialTreatmentCommands = package.specialTreatmentCommands;
	ltxCommands.specialDefCommands = package.specialDefCommands;
	ltxCommands.environmentAliases = package.environmentAliases;
	ltxCommands.possibleCommands = package.possibleCommands;
	ltxCommands.optionCommands = package.optionCommands;
	ltxCommands.commandDefs = package.commandDescriptions;

	auto pckSet = package.possibleCommands["user"];
	ltxCommands.possibleCommands["user"] = userCommandsForSyntaxCheck.unite(pckSet);
	ltxCommands.possibleCommands["%columntypes"] = columntypeForSyntaxCheck;

	// user commands
	auto commands = mUserCommandList.values();
	
	for(auto command : commands){
		
		QString word = command.snippet.word;
		
		if(word.startsWith('%')){ // insert specialArgs
			
			int index = word.indexOf('%',1);

			QString category = word.left(index);
			word = word.mid(index + 1);
			ltxCommands.possibleCommands[category].insert(word);
			
			continue;
		}

		if(!word.startsWith("\\begin{") && !word.startsWith("\\end{")){

            int index = word.indexOf(QRegularExpression("\\W"),1);

			if(index >= 0)
				word = word.left(index);
		}
	}

	//patch lines for new commands (ref,def, etc)

	QStringList categories {
		"%ref" ,
		"%label" ,
		"%definition" ,
		"%cite" ,
		"%citeExtendedCommand" ,
		"%usepackage" ,
		"%graphics" ,
		"%file" ,
		"%bibliography" ,
		"%include" ,
		"%url" ,
		"%todo" ,
		"%replace"
	};
	

	QStringList newCommands;

	for(const auto & category : categories){

		auto possible = parser.possibleCommands[category];
		auto commands = ltxCommands.possibleCommands[category].values();
		
		for(const auto & command : commands){
			
			if(possible.contains(command) && !forceLabelUpdate)
				continue;
			
			newCommands << command;
			possible << command;
		}
	}

	
	bool needQNFAupdate = false;

	for(int i = 0;i < parser.MAX_STRUCTURE_LEVEL;i++){

		auto type = QString("%structure%1").arg(i);

		auto possible = parser.possibleCommands[type];
		auto commands = ltxCommands.possibleCommands[type].values();
		
		for(const auto & command : commands){

			bool update = ! possible.contains(command);
			

			//only update QNFA for added commands. When the default commands are not in ltxCommands.possibleCommands[elem], ltxCommands.possibleCommands[elem] and latexParser.possibleCommands[elem] will always differ and regenerate the QNFA needlessly after every key press

			if(update){
				possible << command;
                needQNFAupdate = true;
            }

			if(update || forceLabelUpdate)
				newCommands << command;
		}
	}

	if(needQNFAupdate)
		parent -> requestQNFAupdate();


    if(!dontPatch && !newCommands.isEmpty())
		patchLinesContaining(newCommands);

	if(delayUpdate)
		return update;

	if(update)
		updateLtxCommands(true);
	
	return false;
}


const QSet<QString> & LatexDocument::getCWLFiles() const {
	return mCWLFiles;
}

void LatexDocument::emitUpdateCompleter(){
	emit updateCompleter();
}

void LatexDocument::gatherCompletionFiles(QStringList & files,QStringList & loadedFiles,LatexPackage & pck,bool gatherForCompleter){
	
	LatexPackage package;

	auto completerConfig = edView
		-> getCompleter()
		-> getConfig();
	
	for(const auto & file :files){

		if(loadedFiles.contains(file))
			continue;
		
		if(parent -> cachedPackages.contains(file)){
			package = parent -> cachedPackages.value(file);
		} else {
          
		    // check if package is actually not depending on options
			QString fileName = LatexPackage::keyToCwlFilename(file);
			QStringList options = LatexPackage::keyToOptions(file);
          
		    bool found = false;
          
		    if(parent -> cachedPackages.contains(fileName)){
                package = parent -> cachedPackages.value(fileName);
                found = ! package.containsOptionalSections;
            }

            if(!found){

                package = loadCwlFile(fileName, completerConfig, options);
                
				if(!package.notFound){

                    fileName = (package.containsOptionalSections) 
						? file 
						: fileName;
                    
					parent -> cachedPackages.insert(fileName,package); // cache package
                } else {
                    LatexPackage package;
                    package.packageName = fileName;
                    parent -> cachedPackages.insert(fileName,package); // cache package as empty/not found package
                }
            }
		}

		if(package.notFound){

			QString name = file;
			auto masterDoc = getRootDocument();
			
			if(masterDoc){
				
				QString fn = masterDoc
					-> getFileInfo()
					.absolutePath();
				
				name += '/' + fn;

				// TODO: oha, the key can be even more complex: option#filename.cwl/masterfile
				// consider this in the key-handling functions of LatexPackage
			}

			emit importPackage(name);
		} else {
			pck.unite(package,gatherForCompleter);
			loadedFiles.append(file);
			
			if(!package.requiredPackages.isEmpty())
				gatherCompletionFiles(package.requiredPackages,loadedFiles,pck,gatherForCompleter);
		}
	}
}


QString LatexDocument::getMagicComment(const QString & name) const {
	
	QString seName;
	QString val;
	
	StructureEntryIterator iter(magicCommentList);
	
	while(iter.hasNext()){
		
		auto comment = iter.next();
		splitMagicComment(comment -> title,seName,val);
		
		if(seName.toLower() == name.toLower())
			return val;
	}
	
	return QString();
}


StructureEntry * LatexDocument::getMagicCommentEntry(const QString & name) const {

	QString seName , val;

	if(!magicCommentList)
		return nullptr;

	StructureEntryIterator iter(magicCommentList);
	
	while(iter.hasNext()){
		auto comment = iter.next();
		splitMagicComment(comment -> title,seName,val);
		
		if(seName == name)
			return comment;
	}
	
	return nullptr;
}


/*!
  replaces the value of the magic comment
 */

void LatexDocument::updateMagicComment(const QString & name,const QString & val,bool createIfNonExisting,QString prefix){
    
	QString line(QString("% %1 %2 = %3").arg(prefix,name,val));

	auto comment = getMagicCommentEntry(name);

    auto lineHandle = comment 
		? comment -> getLineHandle() 
		: nullptr;
	
	if(lineHandle){
		
		QString n , v;
		splitMagicComment(comment -> title,n,v);
		
		if(v != val){
			QDocumentCursor cursor(this,indexOf(lineHandle));
			cursor.select(QDocumentCursor::LineUnderCursor);
			cursor.replaceSelectedText(line);
		}
	} else
	if(createIfNonExisting) {
		QDocumentCursor cursor(this);
		cursor.insertText(line + '\n');
	}
}


void LatexDocument::updateMagicCommentScripts(){

	if(!magicCommentList)
		return;

	localMacros.clear();

	QRegExp regex_Trigger { " *// *(Trigger) *[:=](.*)" };

	StructureEntryIterator iter(magicCommentList);

	while(iter.hasNext()){

		auto comment = iter.next();
		
		QString seName , val;
		
		splitMagicComment(comment -> title,seName,val);

		if(seName == "TXS-SCRIPT"){
			
			QString 
				trigger = "",
				name = val,
				tag;

			int l = comment -> getRealLineNumber() + 1;
			
			for(;l < lineCount();l++){

				QString lt = line(l)
					.text()
					.trimmed();
				
				if(lt.endsWith("TXS-SCRIPT-END"))
					break;
					
				if(!lt.isEmpty() && !lt.startsWith('%'))
					break;
				
				lt.remove(0,1);

				tag += lt + '\n';
				
				if(regex_Trigger.exactMatch(lt))
					trigger = regex_Trigger.cap(2).trimmed();
			}

			Macro macro(name,Macro::Script,tag,"",trigger);
			macro.document = this;
			localMacros.append(macro);
		}
	}
}


/*!
 * Return whether the use of package \a name is declared in this document.
 */

bool LatexDocument::containsPackage(const QString & name){
	return containedPackages().contains(name);
}


/*!
 * Return all package names of packages that are declared in this document.
 */

QStringList LatexDocument::containedPackages(){

	QStringList packages;

    for(auto package : mUsepackageList){
		
		int i = package.indexOf('#');
		
		if(i >= 0)
			package = package.mid(i + 1);

		packages << package;
	}

	return packages;
}


/*!
 * Return a list of packages that are available in the document.
 * This includes all packages declared in all project files.
 */

QSet<QString> LatexDocument::usedPackages(){

	QSet<QString> packages;

	for(auto document : getListOfDocs())
        packages.unite(convertStringListtoSet(document -> containedPackages()));

	return packages;
}


// doc==0 means current document 

LatexDocument *LatexDocuments::getRootDocumentForDoc(LatexDocument * document) const {

	if(masterDocument)
		return masterDocument;
	
	auto current = currentDocument;
	
	if(document)
		current = document;
	
	if(current)
		return current -> getRootDocument();
		
	return current;
}

QString LatexDocument::getAbsoluteFilePath(const QString & relName,const QString & extension,const QStringList & additionalSearchPaths) const {
	
	QStringList searchPaths;
	
	const auto rootDoc = getRootDocument();
	
	QString compileFileName = rootDoc -> getFileName();
	
	if(compileFileName.isEmpty())
		compileFileName = rootDoc -> getTemporaryFileName();
	
	QString fallbackPath;
	
	if(!compileFileName.isEmpty()){
		fallbackPath = QFileInfo(compileFileName).absolutePath(); //when the file does not exist, resolve it relative to document (e.g. to create it there)
		searchPaths << fallbackPath;
	}

	searchPaths << additionalSearchPaths;
	
	return findAbsoluteFilePath(relName,extension,searchPaths,fallbackPath);
}

void LatexDocuments::lineGrammarChecked(LatexDocument * doc,QDocumentLineHandle * line,int lineNr,const QList<GrammarError> & errors){
    
	int d = documents.indexOf(doc);
	
	if(d == -1)
		return;
	
	if(!documents[d] -> getEditorView())
		return;
	
	documents[d] 
		-> getEditorView() 
		-> lineGrammarChecked(doc,line,lineNr,errors);
}

void LatexDocument::patchLinesContaining(const QStringList commands){

	for(auto document : getListOfDocs())
		
		// search all cmds in all lines, patch line if cmd is found

		for(int i = 0;i < document -> lines();i++){

			QString text = document -> line(i).text();
			
			for(const auto & command : commands)
				if(text.contains(command)){
					document -> patchStructure(i,1);
					break;
				}
		}
}

void LatexDocuments::enablePatch(const bool enable){
	m_patchEnabled = enable;
}

bool LatexDocuments::patchEnabled(){
	return m_patchEnabled;
}

void LatexDocuments::requestQNFAupdate(){
	emit updateQNFA();
}

QString LatexDocuments::findPackageByCommand(const QString command){

	// go through all cached packages (cwl) and find command in one of them
	QString result;
	
	for(const auto & key : cachedPackages.keys()){

		const LatexPackage pck = cachedPackages.value(key);
		const auto commands = pck.possibleCommands;

		for(const auto & envs : commands.keys())
			if(commands.value(envs).contains(command)){
				result = LatexPackage::keyToCwlFilename(key); //pck.packageName;
				break;
			}

		if(!result.isEmpty())
			break;
	}

	return result;
}


void LatexDocument::updateLtxCommands(bool updateAll){

	lp.init();
	lp.append(LatexParser::getInstance()); // append commands set in config
	
	auto listOfDocs = getListOfDocs();
	
	for(const auto document : listOfDocs)
		lp.append(document -> ltxCommands);

	if(updateAll){

		for(auto document : listOfDocs){
            document -> setLtxCommands(lp);
            document -> reCheckSyntax();
		}

		// check if other document have this doc as child as well (reused doc...)
		auto docs = parent;
		auto lstOfAllDocs = docs -> getDocuments();
		
		for(auto document : lstOfAllDocs){

			if(listOfDocs.contains(document))
				continue; // already handled
			
			if(document -> containsChild(this)) {
				
				// unhandled parent/child
				LatexParser parser;

				parser.init();
				parser.append(LatexParser::getInstance()); // append commands set in config
				
				auto listOfDocs = document -> getListOfDocs();
				
				for(const auto document : listOfDocs)
					parser.append(document -> ltxCommands);

				for(auto document : listOfDocs){
					document -> setLtxCommands(parser);
					document -> reCheckSyntax();
				}
			}
		}
	} else {
		SynChecker.setLtxCommands(lp);
	}

	auto view = getEditorView();
	
	if(view)
		view -> updateReplamentList(lp,false);
}

void LatexDocument::setLtxCommands(const LatexParser & commands){
	
	SynChecker.setLtxCommands(commands);
	
	lp = commands;

	auto view = getEditorView();
	
	if(view)
		view -> updateReplamentList(commands,false);
}

void LatexDocument::setSpeller(SpellerUtility * speller){
    SynChecker.setSpeller(speller);
}

void LatexDocument::setReplacementList(QMap<QString,QString> replacementList){
    SynChecker.setReplacementList(replacementList);
}

using StringPair = QPair<QString,QString>;

const QList<StringPair> SyntaxFormats = {
	{ "math" , "numbers" } ,
	{ "verbatim" , "verbatim" } ,
	{ "picture" , "picture" } ,
	{ "#math" , "math-keyword" } ,
	{ "#picture" , "picture-keyword" } ,
	{ "&math" , "math-delimiter" } ,
	{ "#mathText" , "math-text" } ,
	{ "align-ampersand" , "align-ampersand" } ,
	{ "comment" , "comment" } ,
};

void LatexDocument::updateSettings(){

	SynChecker.setErrFormat(syntaxErrorFormat);
    
	QMap<QString,int> fmtList;

    for(const auto & [ first , second ] : SyntaxFormats)
        fmtList.insert(first,getFormatId(second));
	
    SynChecker.setFormats(fmtList);
}


void LatexDocument::checkNextLine(QDocumentLineHandle *dlh, bool clearOverlay, int ticket, int hint){

    Q_ASSERT_X(dlh != nullptr,"checkNextLine","empty dlh used in checkNextLine");

	if(dlh->getRef() > 1 && dlh->getCurrentTicket() == ticket){

		StackEnvironment env;
		QVariant envVar = dlh->getCookieLocked(QDocumentLine::STACK_ENVIRONMENT_COOKIE);
	
		if(envVar.isValid())
			env = envVar.value<StackEnvironment>();
   
        int index = indexOf(dlh,hint);
	
		if(index == -1)
			return; //deleted
		
		REQUIRE(dlh->document() == this);
		
		if(index + 1 >= lines()){
			//remove old errror marker

			if(unclosedEnv.id != -1){

				unclosedEnv.id = -1;
				
				int unclosedEnvIndex = indexOf(unclosedEnv.dlh);
				
				if(unclosedEnvIndex >= 0 && unclosedEnv.dlh -> getCookieLocked(QDocumentLine::UNCLOSED_ENVIRONMENT_COOKIE).isValid()){

					StackEnvironment env;

					Environment newEnv;
					newEnv.name = "normal";
					newEnv.id = 1;
					env.push(newEnv);

					TokenStack remainder;

					if(unclosedEnvIndex >= 1){
						
						auto prev = line(unclosedEnvIndex - 1).handle();
						
						QVariant result = prev -> getCookieLocked(QDocumentLine::STACK_ENVIRONMENT_COOKIE);
						
						if(result.isValid())
							env = result.value<StackEnvironment>();
						
						remainder = prev 
							-> getCookieLocked(QDocumentLine::LEXER_REMAINDER_COOKIE)
							.value<TokenStack>();
					}

					SynChecker.putLine(unclosedEnv.dlh,env,remainder,true,unclosedEnvIndex);
				}
			}

			if(env.size() > 1){
				//at least one env has not been closed
				Environment environment = env.top();
				unclosedEnv = env.top();
				SynChecker.markUnclosedEnv(environment);
			}

			return;
		}
	
		TokenStack remainder = dlh 
			-> getCookieLocked(QDocumentLine::LEXER_REMAINDER_COOKIE)
			.value<TokenStack>();
        
		SynChecker.putLine(line(index + 1).handle(),env,remainder,clearOverlay,index + 1);
	}

    dlh -> deref();
}

bool LatexDocument::languageIsLatexLike() const {

	auto definition = languageDefinition();
	
	if(definition)
		return LATEX_LIKE_LANGUAGES.contains(definition -> language());

	return false;
}

/*
 * \brief Forces syntax recheck of a group of lines
 * \param[in] lineStart Starting line number to be checked
 * \param[in] lineNum Total number of lines to be checked. If -1, then check all lines to the end of the document.
 */

void LatexDocument::reCheckSyntax(int lineStart,int lineNum){

	// Basic sanity checks
	Q_ASSERT(lineStart >= 0);
	Q_ASSERT((lineNum == -1) || (lineNum > 0));

	// If the document does not support syntax checking just return silently
    
	if(!languageIsLatexLike())
		return;

	int lineTotal = lineCount();
	int lineEnd;

	if(lineNum == -1)
		lineEnd = lineTotal;
	else
	if((lineEnd = lineStart + lineNum) > lineTotal)
		lineEnd = lineTotal;
	
	// Fast return if zero lines will be checked
	
	if(lineStart == lineEnd)
		return;

	// Delete the environment cookies for the specified lines to force their re-check
	
	for(int i = lineStart;i < lineEnd;++i)
		// We rely on the fact that QDocumentLine::removeCookie() holds a write lock of the corresponding
		// line handle while removing the cookie. Lack of write locking causes crashes due to simultaneous
		// access from the syntax checker thread.
		line(i).removeCookie(QDocumentLine::STACK_ENVIRONMENT_COOKIE);

	// Enqueue the first line for syntax checking. The remaining lines will be enqueued automatically
	// through the checkNextLine signal because we deleted their STACK_ENVIRONMENT_COOKIE cookies.
	
	StackEnvironment prevEnv;
	getEnv(lineStart,prevEnv);
	
	TokenStack prevTokens;
	
	if(lineStart)
		prevTokens = line(lineStart-1)
			.getCookie(QDocumentLine::LEXER_REMAINDER_COOKIE)
			.value<TokenStack>();
	
	SynChecker.putLine(line(lineStart).handle(), prevEnv, prevTokens, true, lineStart);
}

QString LatexDocument::getErrorAt(QDocumentLineHandle * dlh,int pos,StackEnvironment previous,TokenStack stack){
	return SynChecker.getErrorAt(dlh,pos,previous,stack);
}

int LatexDocument::syntaxErrorFormat;

void LatexDocument::getEnv(int lineNumber,StackEnvironment & env){
	
	Environment newEnv;
	newEnv.name = "normal";
	newEnv.id = 1;
	
	env.push(newEnv);
	
	if(lineNumber > 0){
		auto prev = this->line(lineNumber - 1);
		
		REQUIRE(prev.isValid());
		
		auto result = prev.getCookie(QDocumentLine::STACK_ENVIRONMENT_COOKIE);
		
		if(result.isValid())
			env = result.value<StackEnvironment>();
	}
}


QString LatexDocument::getLastEnvName(int lineNumber){

	StackEnvironment env;
	
	getEnv(lineNumber,env);
	
	if(env.isEmpty())
		return "";

	return env.top().name;
}
