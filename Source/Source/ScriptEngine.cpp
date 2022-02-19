#include "scriptengine.h"
#include "FileChooser.hpp"
#include "smallUsefulFunctions.h"
#include "qdocumentsearch.h"
#include "buildmanager.h"
#include "PDFDocument.h"
#include "usermacro.h"
#include <QCryptographicHash>

#include "TexStudio.hpp"
#include "Latex/Document.hpp"


Q_DECLARE_METATYPE(BuildManager *)
Q_DECLARE_METATYPE(RunCommandFlags)
Q_DECLARE_METATYPE(QList<LatexDocument *>)

#ifndef NO_POPPLER_PREVIEW
	Q_DECLARE_METATYPE(QList<PDFDocument *>)
#endif

QStringList scriptengine::privilegedWriteScripts = QStringList();
QStringList scriptengine::privilegedReadScripts = QStringList();

BuildManager * scriptengine::buildManager = nullptr;
QList<Macro> * scriptengine::macros = nullptr;
Texstudio * scriptengine::app = nullptr;

int scriptengine::writeSecurityMode = 2;
int scriptengine::readSecurityMode = 2;


template <typename Type> 

QJSValue qScriptValueFromQObject(QJSEngine * engine,const Type & object){
	return engine -> newQObject(object);
}


template <typename Type>

void qScriptValueToQObject(const QJSValue & value,Type & object){
	object = qobject_cast<Type>(value.toQObject());
}


QJSValue qScriptValueFromDocumentCursor(QJSEngine * engine,const QDocumentCursor & cursor){
	return engine -> newQObject(new QDocumentCursor(cursor));
}


void qScriptValueToDocumentCursor(const QJSValue & value,QDocumentCursor & cursor){
	Q_ASSERT(value.toQObject());
	cursor = * qobject_cast<QDocumentCursor *>(value.toQObject());
}


template <typename Type>

QJSValue qScriptValueFromQList(QJSEngine * engine,const QList<Type> & list){

	auto array = engine -> newArray(list.size());

	for(int i = 0;i < list.size();i++)
		array.setProperty(i,engine -> newQObject(list[i]));

	return array;
}


QDocumentCursor cursorFromValue(const QJSValue & value){
	
	const auto cursor = qobject_cast<QDocumentCursor *> (value.toQObject());
	
	return cursor
		? * cursor
		: QDocumentCursor();
}


QJSValue qScriptValueFromQFileInfo(QJSEngine * engine,const QFileInfo & info){
	return engine -> toScriptValue(info.absoluteFilePath());
}


void qScriptValueToQFileInfo(const QJSValue & value,QFileInfo & info){
	info = QFileInfo(value.toString());
}


static quintptr PointerObsfuscationKey = 0;

// hide true pointers from scripts

quintptr pointerObsfuscationKey(){

	while(PointerObsfuscationKey == 0)
		for(unsigned int i = 0;i < sizeof(PointerObsfuscationKey);i++)
			PointerObsfuscationKey = (PointerObsfuscationKey << 8) | (rand() & 0xFF);

	return PointerObsfuscationKey;
}


QString * getStrPtr(QJSValue value){

	const auto dataStore = value.property("dataStore");
	
	if(dataStore.isUndefined())
		return nullptr;
	
	bool ok = false;
	
	const auto pointer = dataStore
		.toVariant()
		.toULongLong(& ok);
	
	if(!ok)
		return nullptr;
		
	if(pointer == 0)
		return nullptr;
		
	if(pointer == pointerObsfuscationKey())
		return nullptr;
	
	return reinterpret_cast<QString *>(pointer ^ pointerObsfuscationKey());
}


QJSValue getSetStrValue(QJSEngine *){
	return QJSValue();
}

scriptengine::scriptengine(QObject * object) 
	: QObject(object)
	, triggerId(-1)
	, m_editor(nullptr)
	, m_allowWrite(false) {

	engine = new QJSEngine(this);

	qRegisterMetaType<RunCommandFlags>();


	/*	For C++20

	const auto qmlType = [ & ] <typename Type> (){
		qmlRegisterType<Type>("com.txs.qmlcomponents",1,0,typeid(Type).name());
	};

	qmlType<QDocumentCursor>();
	qmlType<LatexDocuments>();
	qmlType<LatexDocument>();
	qmlType<BuildManager>();
	qmlType<QDocument>();
	qmlType<Texstudio>();
	qmlType<ProcessX>();
	qmlType<QEditor>();
	qmlType<QAction>();
	qmlType<QMenu>();

	*/

	qmlRegisterType<QDocumentCursor>("com.txs.qmlcomponents",1,0,"QDocumentCursor");
	qmlRegisterType<LatexDocuments>("com.txs.qmlcomponents",1,0,"LatexDocuments");
	qmlRegisterType<LatexDocument>("com.txs.qmlcomponents",1,0,"LatexDocument");
	qmlRegisterType<BuildManager>("com.txs.qmlcomponents",1,0,"BuildManager");
	qmlRegisterType<QDocument>("com.txs.qmlcomponents",1,0,"QDocument");
	qmlRegisterType<Texstudio>("com.txs.qmlcomponents",1,0,"Texstudio");
	qmlRegisterType<ProcessX>("com.txs.qmlcomponents",1,0,"ProcessX");
	qmlRegisterType<QEditor>("com.txs.qmlcomponents",1,0,"QEditor");
	qmlRegisterType<QAction>("com.txs.qmlcomponents",1,0,"QAction");
	qmlRegisterType<QMenu>("com.txs.qmlcomponents",1,0,"QMenu");


    // use config

	auto config = ConfigManagerInterface::getInstance();

    config -> registerOption("Scripts/Privileged Read Scripts",& privilegedReadScripts);
    config -> registerOption("Scripts/Read Security Mode",& readSecurityMode,1);
    config -> registerOption("Scripts/Privileged Write Scripts",& privilegedWriteScripts);
    config -> registerOption("Scripts/Write Security Mode",& writeSecurityMode,1);
}


scriptengine::~scriptengine(){
	
	engine -> collectGarbage();
	delete engine;
	
	engine = nullptr;
	
	// don't delete globalObject, it has either been destroyed in run, or by another script
}


void scriptengine::setScript(const QString & script,bool allowWrite){
	m_script = script;
	m_allowWrite = allowWrite;
}


void scriptengine::setEditorView(LatexEditorView * view){
	REQUIRE(view);
	m_editor = view -> editor;
	m_editorView = view;
}


void scriptengine::run(const bool quiet){

	auto jsApp = engine -> newQObject(app);
	
	engine 
		-> globalObject()
		.  setProperty("app",jsApp);
	
	QQmlEngine::setObjectOwnership(app,QQmlEngine::CppOwnership);

	// create from handle, so modifying the cursor in the script directly affects the actual cursor

	QDocumentCursor c( m_editor ? m_editor->cursorHandle() : nullptr);
	
	QJSValue cursorValue;
	
	if(m_editorView)
		engine 
			-> globalObject()
			.  setProperty("editorView",engine -> newQObject(m_editorView));

    auto scriptJS = engine -> newQObject(this);

    // add general debug/warn functions

	auto global = engine -> globalObject();
    
    global.setProperty("registerAsBackgroundScript",scriptJS.property("registerAsBackgroundScript"));
    global.setProperty("confirmWarning",scriptJS.property("confirmWarning"));
    global.setProperty("crash_sigstack",scriptJS.property("crash_sigstack"));
	global.setProperty("hasPersistent",scriptJS.property("hasPersistent"));
    global.setProperty("crash_sigsegv",scriptJS.property("crash_sigsegv"));
    global.setProperty("setPersistent",scriptJS.property("setPersistent"));
    global.setProperty("getPersistent",scriptJS.property("getPersistent"));
    global.setProperty("crash_sigfpe",scriptJS.property("crash_sigfpe"));
    global.setProperty("crash_throw",scriptJS.property("crash_throw"));
	global.setProperty("information",scriptJS.property("information"));
    global.setProperty("crash_loop",scriptJS.property("crash_loop"));
    global.setProperty("writeFile",scriptJS.property("writeFile"));
    global.setProperty("critical",scriptJS.property("critical"));
    global.setProperty("readFile",scriptJS.property("readFile"));
    global.setProperty("warning",scriptJS.property("warning"));
    global.setProperty("confirm",scriptJS.property("confirm"));
    global.setProperty("system",scriptJS.property("system"));
	global.setProperty("alert",scriptJS.property("alert"));
    global.setProperty("debug",scriptJS.property("debug"));

	#ifndef QT_NO_DEBUG
		global.setProperty("crash_assert",scriptJS.property("crash_assert"));
	#endif


	if(m_editor){
		
		auto editorValue = engine -> newQObject(m_editor);

		QQmlEngine::setObjectOwnership(m_editor,QQmlEngine::CppOwnership);
		QQmlEngine::setObjectOwnership(this,QQmlEngine::CppOwnership);
		
		editorValue.setProperty("replaceSelectedText",scriptJS.property("replaceSelectedText"));
		editorValue.setProperty("insertSnippet",scriptJS.property("insertSnippet"));
		editorValue.setProperty("cutBuffer",m_editor -> cutBuffer);
		editorValue.setProperty("saveCopy",scriptJS.property("saveCopy"));
		editorValue.setProperty("replace",scriptJS.property("replaceFunction"));
		editorValue.setProperty("search",scriptJS.property("searchFunction"));
		editorValue.setProperty("save",scriptJS.property("save"));
		
		global.setProperty("editor", editorValue);

		cursorValue = engine -> newQObject(&c);

		global.setProperty("cursor",cursorValue);
		
		QQmlEngine::setObjectOwnership(&c,QQmlEngine::CppOwnership);

		auto matches = engine -> newArray(triggerMatches.size());
		
		for(int i = 0;i < triggerMatches.size();i++)
			matches.setProperty(i,triggerMatches[i]);

		global.setProperty("triggerMatches",matches);
	}

    global.setProperty("setTimeout",scriptJS.property("setTimeout"));
	global.setProperty("triggerId",QJSValue(triggerId));

    auto qsMetaObject = engine -> newQMetaObject(& QDocumentCursor::staticMetaObject);
    global.setProperty("QDocumentCursor",qsMetaObject);
    global.setProperty("cursorEnums",qsMetaObject);

	auto uidClass = engine -> newQMetaObject(& UniversalInputDialogScript::staticMetaObject);
	global.setProperty("UniversalInputDialog",uidClass);

	FileChooser flchooser(nullptr,scriptengine::tr("File Chooser"));
	global.setProperty("fileChooser",engine -> newQObject(& flchooser));
	global.setProperty("documentManager",engine -> newQObject(& app -> documents));

	QQmlEngine::setObjectOwnership(& app -> documents,QQmlEngine::CppOwnership);

	global.setProperty("documents",qScriptValueFromQList(engine,app -> documents.documents));

	#ifndef NO_POPPLER_PREVIEW
		global.setProperty("pdfs",qScriptValueFromQList(engine,PDFDocument::documentList()));
	#endif

	auto bm = engine -> newQObject(& app -> buildManager);
	QQmlEngine::setObjectOwnership(& app -> buildManager,QQmlEngine::CppOwnership);

	global.setProperty("buildManager",bm);

	auto result = engine -> evaluate(m_script);

	if(result.isError()){
		auto error = QString(tr("Uncaught exception at line %1: %2\n"))
			.arg(result.property("lineNumber").toInt())
			.arg(result.toString());
		
		qDebug() << error;
        
		if(!quiet)
            QMessageBox::critical(nullptr,tr("Script-Error"),error);
	}

	if(m_editor && !global.property("cursor").strictlyEquals(cursorValue))
		m_editor -> setCursor(cursorFromValue(global.property("cursor")));
}


void scriptengine::insertSnippet(const QString & code){
	
	CodeSnippet snippet(code);

	if(!m_editor)
		return;
	
	for(auto & cursor : m_editor -> cursors())
		snippet.insertAt(m_editor,& cursor);
}


#define assertTrue(condition,message)                     \
	if(!(condition)){                                     \
		engine -> throwError(scriptengine::tr(message));  \
		return QJSValue();                                \
	}


QJSValue scriptengine::replaceSelectedText(QJSValue replacementText,QJSValue options){
	
	bool 
		onlyEmpty = false,
		prepend = false,
		noEmpty = false,
		append = false,
		macro = false;

	if(!options.isUndefined()){

        assertTrue(
			options.isObject(),
			"2nd value needs to be an object")
		
		noEmpty = options.property("noEmpty").toBool();
		onlyEmpty = options.property("onlyEmpty").toBool();
		append = options.property("append").toBool();
		prepend = options.property("prepend").toBool();
		macro = options.property("macro").toBool();
        
		// well it could, but there is no good way to	define what should happen to the selection

		assertTrue(
			!macro || !(append || prepend),
			"Macro option cannot be combined with append or prepend option.")
	}


	QList<QDocumentCursor> 
		cursors = m_editor -> cursors(),
		newMacroCursors;

	auto newMacroPlaceholder = (macro) 
		? m_editor -> getPlaceHolders() 
		: QList<PlaceHolder>();

	m_editor
		-> document()
		-> beginMacro();

	for(auto & cursor : cursors){

		const auto selected = cursor.selectedText();

		if(noEmpty && selected.isEmpty())
			continue;
		
		if(onlyEmpty && !selected.isEmpty())
			continue;
		
		QString newst;
		
		if(replacementText.isCallable()){

			const auto callback = replacementText.call(QJSValueList()
				<< engine -> toScriptValue(selected)
				<< engine -> newQObject(& cursor));

			newst = callback.toString();
		} else {
			newst = replacementText.toString();
		}

		if(macro){
			
			m_editor -> clearPlaceHolders();
			m_editor -> clearCursorMirrors();
			
			CodeSnippet snippet(newst);
			snippet.insertAt(m_editor,& cursor);
			
			newMacroPlaceholder << m_editor -> getPlaceHolders();
			
			if(m_editor -> cursor().isValid()){

				newMacroCursors << m_editor -> cursor();

				newMacroCursors
					.last()
					.setAutoUpdated(true);

			} else {
				// CodeSnippet does not select without placeholder. But here we do, since it is called replaceSelectedText
				newMacroCursors << cursor;
			}
		} else {
			if(append && prepend)
				newst = newst + selected + newst;
			else
			if(append)
				newst = selected + newst;
			else
			if(prepend)
				newst = newst + selected;
			
			cursor.replaceSelectedText(newst);
		}
	}

	m_editor
		-> document()
		-> endMacro();
	
	// inserting multiple macros destroyed the new cursors, we need to insert them again

	if(macro && (cursors.size() > 0)){
        
		if(noEmpty)
			for(const auto & cursor : cursors)
				if(cursor.isValid() && cursor.selectedText().isEmpty())
					newMacroCursors << cursor;

        if(onlyEmpty)
            for(const auto & cursor : cursors)
                if(cursor.isValid() && !cursor.selectedText().isEmpty())
                    newMacroCursors << cursor;

        if(newMacroCursors.size())
			m_editor -> setCursor(newMacroCursors.first());
        
		for(int i = 1;i < newMacroCursors.size();i++)
			m_editor -> addCursorMirror(newMacroCursors[i]);

		m_editor -> replacePlaceHolders(newMacroPlaceholder);
	}

	return QJSValue();
}


QJSValue scriptengine::searchReplaceFunction(
	QJSValue searchText,
	QJSValue arg1,
	QJSValue arg2,
	QJSValue arg3,
	bool replace
){
	// read arguments
    
	assertTrue(m_editor,"invalid object")
    assertTrue(!replace || !arg1.isUndefined(),"at least two arguments are required")
    assertTrue(!searchText.isUndefined(),"at least one argument is required")
    assertTrue(searchText.isString() || searchText.isRegExp(),"first argument must be a string or regexp")
	
	QDocumentSearch::Options flags = QDocumentSearch::Silent;
    
	bool 
		caseInsensitive = false,
		global = false;

	QString searchFor;
	
	if(searchText.isRegExp()){
		flags |= QDocumentSearch::RegExp;
        
		auto r = searchText
			.toVariant()
			.toRegularExpression();
		
		searchFor = r.pattern();
        caseInsensitive = (r.patternOptions() & QRegularExpression::CaseInsensitiveOption) != QRegularExpression::NoPatternOption;

		global = searchText
			.property("global")
			.toBool();
	} else {
		searchFor = searchText.toString();
	}

	QJSValue handler;

	auto m_scope = m_editor 
		-> document()
		-> cursor(0,0,m_editor -> document() -> lineCount(),0);

	int handlerCount = 0;
	
	for(int i = 1;i < 4;i++){

		QJSValue args;
		
		switch(i){
		case 1: args = arg1; break;
		case 2: args = arg2; break;
		case 3: args = arg3; break;
		}

		if(args.isUndefined())
			break;
		
		if(args.isString() || args.isCallable())
			handlerCount++;
	}

    assertTrue(handlerCount <= (replace ? 3 : 2),"too many string or function arguments")
	
	for(int i = 1;i < 4;i++){

		QJSValue a;
		
		switch(i){
		case 1: a = arg1; break;
		case 2: a = arg2; break;
		case 3: a = arg3; break;
		}

		if(a.isUndefined())
			break;
		
		if(a.isCallable()){
            assertTrue(handler.isUndefined(),"Multiple callbacks")
			handler = a;
		} else
		if(a.isString()){
			if(!replace || handlerCount > 1){
				QString s = a.toString().toLower();
				global = s.contains("g");
				caseInsensitive = s.contains("i");

				if(s.contains("w"))
					flags |= QDocumentSearch::WholeWords;
			} else {
                assertTrue(handler.isUndefined(), "Multiple callbacks")
				handler = a;
			}

			handlerCount--;
		} else
		if(a.isNumber())
			flags |= QDocumentSearch::Options((int) a.toNumber());
        else
		if(a.isObject())
			m_scope = cursorFromValue(a);
        else
			assertTrue(false,"Invalid argument")
	}

    assertTrue(!handler.isUndefined() || !replace, "No callback given")
	
	if(!caseInsensitive)
		flags |= QDocumentSearch::CaseSensitive;
	
	// search / replace
	
	QDocumentSearch search(m_editor,searchFor,flags);
	search.setScope(m_scope);

	if(replace && handler.isString()){
		search.setReplaceText(handler.toString());
		search.setOption(QDocumentSearch::Replace,true);
		return search.next(false,global,false,false);
	}

	if(handler.isUndefined())
		return search.next(false,global,true,false);
	
	int count = 0;
	
	while(search.next(false,false,true,false) && search.cursor().isValid()){

		count++;
		
		QDocumentCursor temp = search.cursor();
		QJSValue cb = handler.call(QJSValueList() << engine -> newQObject(& temp));
		
		if(replace && !cb.isError()){
			QDocumentCursor tmp = search.cursor();
			tmp.replaceSelectedText(cb.toString());
			search.setCursor(tmp.selectionEnd());
		}

		if(!global)
			break;
	}

	return count;
}


QJSValue scriptengine::searchFunction(QJSValue searchFor,QJSValue arg1,QJSValue arg2,QJSValue arg3){
	return searchReplaceFunction(searchFor,arg1,arg2,arg3,false);
}


QJSValue scriptengine::replaceFunction(QJSValue searchFor,QJSValue arg1,QJSValue arg2,QJSValue arg3){
	return searchReplaceFunction(searchFor,arg1,arg2,arg3,true);
}


void scriptengine::alert(const QString & message){
    UtilsUi::txsInformation(message);
}


void scriptengine::information(const QString & message){
    UtilsUi::txsInformation(message);
}


void scriptengine::critical(const QString & message){
    UtilsUi::txsCritical(message);
}


void scriptengine::warning(const QString & message){
    UtilsUi::txsWarning(message);
}


bool scriptengine::confirm(const QString & message){
    return UtilsUi::txsConfirm(message);
}


bool scriptengine::confirmWarning(const QString & message){
    return UtilsUi::txsConfirmWarning(message);
}


void scriptengine::debug(const QString & message){
    qDebug() << message;
}


#ifndef QT_NO_DEBUG
	
	void scriptengine::crash_assert(){
		Q_ASSERT(false);
	}

#endif


void scriptengine::crash_sigsegv(){
 
    if(!confirmWarning("Do you want to let txs crash with a SIGSEGV?"))
		return;
 
    char * c = nullptr;
    *c = 'A';
}


int global0 = 0;

void scriptengine::crash_sigfpe(){

    if(!confirmWarning("Do you want to let txs crash with a SIGFPE?"))
		return;
    
	int x = 1 / global0;
    
	Q_UNUSED(x)
}


void scriptengine::crash_stack(){

    if(!confirmWarning("Do you want to let txs crash with a stack overflow?"))
		return;
    
	int temp = global0;
    
	crash_stack();
    
	Q_UNUSED(temp)
}


void scriptengine::crash_loop(){

    if (!confirmWarning("Do you want to let txs freeze with an endless loop?")) return;

    int 
		a = 1,
		b = 2,
		c = 3,
		d = 4;
    
	while(true){
        
		void * x = malloc(16);
        
		free(x);
        
		// make sure, no register suddenly change

		Q_ASSERT(a == 1);
        Q_ASSERT(b == 2);
        Q_ASSERT(c == 3);
        Q_ASSERT(d == 4);
    };
}


void scriptengine::crash_throw(){

    if(!confirmWarning("Do you want to let txs crash with an exception?"))
		return;
    
	throw "debug crash";
}


void scriptengine::save(const QString path){

    if(path.isEmpty()){
        m_editor -> save();
    } else{
        m_editor -> save(path);
    }
}


void scriptengine::saveCopy(const QString & path){
    m_editor -> saveCopy(path);
}


ProcessX * scriptengine::system(
	const QString & commandLine,
	const QString & workingDirectory
){
    if(!buildManager)
		return nullptr;
	
	if(!needWritePrivileges("system",commandLine))
        return nullptr;

    ProcessX * process = nullptr;

    if(commandLine.contains(BuildManager::TXS_CMD_PREFIX) || !commandLine.contains("|"))
        process = buildManager -> firstProcessOfDirectExpansion(commandLine,QFileInfo());
    else
		// use internal, so people can pass | to sh
		process = buildManager -> newProcessInternal(commandLine,QFileInfo());

    if(!process)
		return nullptr;
    
	connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),process,SLOT(deleteLater()));

    QMetaObject::invokeMethod(reinterpret_cast<QObject *>(app),"connectSubCommand",Q_ARG(ProcessX *,process),Q_ARG(bool,true));

    if(!workingDirectory.isEmpty())
        process -> setWorkingDirectory(workingDirectory);

    auto buffer = new QString;
    process -> setStdoutBuffer(buffer);
    process -> startCommand();
    process -> waitForStarted();

    return process;
}


void scriptengine::writeFile(const QString & path,const QString & data){

    if(!needWritePrivileges("writeFile",path))
        return;
    
    QFile file(path);
    
	if(!file.open(QFile::WriteOnly))
        return;
    
	file.write(data.toUtf8());
    file.close();
}


QVariant scriptengine::readFile(const QString & path){

    if(!needReadPrivileges("readFile",path))
        return QVariant();
    
    QFile file(path);
    
	if(!file.open(QFile::ReadOnly))
        return QVariant();
    
	QTextStream stream(& file);
    stream.setAutoDetectUnicode(true);
	return stream.readAll();
}


bool scriptengine::hasReadPrivileges(){

    if(readSecurityMode == 0)
        return false;

    if(readSecurityMode != 2)
        return false;
    
	const auto hash = getScriptHash();

    return
        privilegedWriteScripts.contains(hash) ||
        privilegedReadScripts.contains(hash);
}


bool scriptengine::hasWritePrivileges(){

    if(writeSecurityMode == 0)
        return false;

    if(writeSecurityMode != 2)
        return false;

    const auto hash = getScriptHash();
    
    return privilegedWriteScripts.contains(hash);
}


QByteArray scriptengine::getScriptHash(){
    return QCryptographicHash::hash(m_script.toLatin1(),QCryptographicHash::Sha1).toHex();
}


void scriptengine::registerAllowedWrite(){

    const auto hash = getScriptHash();

    if(privilegedWriteScripts.contains(hash))
		return;

    privilegedWriteScripts.append(hash);
}


bool scriptengine::needWritePrivileges(const QString & path,const QString & parameter){

    if(writeSecurityMode == 0)
		return false;
    
	if(hasWritePrivileges())
		return true;
    
	const int choice = QMessageBox::question(
		nullptr, "TeXstudio script watcher",
        tr("The current script has requested to enter privileged write mode and call following function:\n%1\n\nDo you trust this script?")
            .arg(path + "(\"" + parameter + "\")"),
		tr("Yes, allow this call"),
        tr("Yes, allow all calls it will ever make"),
		tr("No, abort the call"),0,2);

	// only now

    if(choice == 0)
		return true;
    
	if(choice != 1)
		return false;
    
    const auto hash = getScriptHash();

    privilegedWriteScripts.append(hash);
    
	return true;
}


bool scriptengine::needReadPrivileges(const QString & path,const QString & parameter){

    if(readSecurityMode == 0)
		return false;
    
	if(hasReadPrivileges())
		return true;
    
	const int choice = QMessageBox::question(
		nullptr,"TeXstudio script watcher",
        tr("The current script has requested to enter privileged mode and read the following value:\n%1\n\nDo you trust this script?")
            .arg(path + "(\"" + parameter + "\")"),
		tr("Yes, allow this reading"),
        tr("Yes, grant permanent read access to everything"),
		tr("No, abort the call"),0,2);

	// only now
    
	if(choice == 0)
		return true;
    
	if(choice != 1)
		return false;

    const auto hash = getScriptHash();
    
    privilegedReadScripts.append(hash);
    
	return true;
}


bool scriptengine::hasPersistent(const QString & name){
    return ConfigManagerInterface::getInstance()
		-> existsOption(name);
}


void scriptengine::setPersistent(const QString & name,const QVariant & value){
    
	if(needWritePrivileges("setPersistent",name + "=" + value.toString()))
		ConfigManagerInterface::getInstance()
			-> setOption(name,value);
}


QVariant scriptengine::getPersistent(const QString & name){

	if(needReadPrivileges("getPersistent",name))
		return ConfigManagerInterface::getInstance()
			-> getOption(name);

	return QVariant();
}


void scriptengine::registerAsBackgroundScript(const QString & name){

    static QMap<QString,QPointer<scriptengine>> backgroundScripts;

    const auto realName = name.isEmpty() 
		? getScriptHash() 
		: name;

    if(!backgroundScripts.value(realName,QPointer<scriptengine>(nullptr)).isNull())
        delete backgroundScripts.value(realName,QPointer<scriptengine>(nullptr)).data();

    backgroundScripts.insert(realName,this);
}


bool scriptengine::setTimeout(const QString & function,const int timeout){

    auto timer = new QTimer();
    auto parts = function.split(" ");
    
	timer -> singleShot(timeout,this,std::bind(& scriptengine::runTimed,this,parts.value(1)));

    return true;
}


void scriptengine::runTimed(const QString function){

    if(function.isEmpty())
		return;

    engine -> evaluate(function);
}


UniversalInputDialogScript::UniversalInputDialogScript(QWidget * widget)
	: UniversalInputDialog(widget) {}

UniversalInputDialogScript::~UniversalInputDialogScript(){
	
	for(auto & property : properties)
		property.deallocate();
}

QWidget * UniversalInputDialogScript::add(
	const QJSValue & def,
	const QJSValue & description,
	const QJSValue & id
){
	QWidget * widget = nullptr;

	if(def.isArray()){
	
		QStringList options;
		QJSValueIterator it(def);
	
		while(it.hasNext()){
			
			it.next();

			// skip length property

            if(it.name() == "length")
				continue;

			if(it.value().isString() || it.value().isNumber())
				options << it.value().toString();
		}

		widget = addComboBox(ManagedProperty::fromValue(options),description.toString());
	} else 
	if(def.isBool()) {
		widget = addCheckBox(ManagedProperty::fromValue(def.toBool()),description.toString());
	} else
	if (def.isNumber()) {
		widget = addDoubleSpinBox(ManagedProperty::fromValue(def.toNumber()),description.toString());
	} else
	if (def.isString()) {
		widget = addLineEdit(ManagedProperty::fromValue(def.toString()),description.toString());
	} else {
		return nullptr;
	}

	if(!id.isUndefined())
		properties.last().name = id.toString();
	
	return widget;
}


QJSValue UniversalInputDialogScript::execDialog(){

	return UniversalInputDialog::exec()
		? getAll()
		: QJSValue();
}


QJSValue UniversalInputDialogScript::getAll(){

	auto array = engine -> newArray(properties.size());
	
	for(int i = 0;i < properties.size();i++){

		const auto value = properties[i];
		const auto variant = value.valueToQVariant();
		const auto name = value.name;

		array.setProperty(i,engine -> toScriptValue(variant));
		
		if(name.isEmpty())
			continue;

		array.setProperty(name,engine -> toScriptValue(variant));
	}

	return array;
}


QVariant UniversalInputDialogScript::get(const QJSValue & key){

	if(key.isNumber()){
		
		const auto index = key.toInt();

		if(index >= 0 && index < properties.size())
			return properties[index].valueToQVariant();
	}

	if(key.isString()){

		const auto name = key.toString();
		
		for(const auto & property : properties)
			if(property.name == name)
				return property.valueToQVariant();
	}

	return QVariant();
}


#undef assertTrue
