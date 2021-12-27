
/***************************************************************************
 *   copyright       : (C) 2008 by Benito van der Zander                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "spellerutility.h"
#include "smallUsefulFunctions.h"
#include "JlCompress.h"


bool SpellerUtility::hideNonTextSpellingErrors = true;
bool SpellerUtility::inlineSpellChecking = true;

int SpellerUtility::spellcheckErrorFormat = -1;


SpellerUtility::SpellerUtility(QString name)
	: mName(name)
	, currentDic("")
	, pChecker(nullptr)
	, spellCodec(nullptr) {}


bool SpellerUtility::loadDictionary(QString dictionary,QString ignoreFilePrefix){

	if(dictionary == currentDic)
		return true;
	
	unload();
	
	QString base = dictionary.left(dictionary.length() - 4);
	QString dicFile = base + ".dic";
	QString affFile = base + ".aff";

	if(
		! QFileInfo::exists(dicFile) ||
		! QFileInfo::exists(affFile)
	){
		if(!QFileInfo::exists(affFile))
			mLastError = tr("Missing .aff file:\n%1").arg(affFile);
		else
			mLastError = tr("Dictionary does not exist.");

		//remove spelling error marks from errors with previous dictionary
		//(because these marks could lead to crashes if not removed)
		emit dictionaryLoaded();
		
		return false;
	}

	currentDic = dictionary;
	
	pChecker = new Hunspell(affFile.toLocal8Bit(),dicFile.toLocal8Bit());
	
	if(!pChecker){
		currentDic = "";
		ignoreListFileName = "";
		mLastError = "Creation of Hunspell object failed.";
		REQUIRE_RET(false,false);
	}

	spell_encoding = QString(pChecker -> get_dic_encoding());
	spellCodec = QTextCodec::codecForName(spell_encoding.toLatin1());
    
	if(spellCodec == nullptr){
		mLastError = "Could not determine encoding.";
		unload();
		emit dictionaryLoaded();
		return false;
	}

	ignoredWords.clear();
	ignoredWordList.clear();
	ignoredWordsModel.setStringList(QStringList());

	ignoreListFileName = base + ".ign";
	
	if(!isFileRealWritable(ignoreListFileName))
		ignoreListFileName = ignoreFilePrefix + QFileInfo(dictionary).baseName() + ".ign";
	
	if(!isFileRealWritable(ignoreListFileName)){
		ignoreListFileName = "";
		mLastError.clear();
		emit dictionaryLoaded();
		return true;
	}

	QFile f(ignoreListFileName);
	
	if(!f.open(QFile::ReadOnly)){
		mLastError.clear();
		emit dictionaryLoaded();
		return true;
	}

    ignoredWordList = QTextCodec::codecForName("UTF-8")
		-> toUnicode(f.readAll())
		.  split("\n", Qt::SkipEmptyParts);
	
	// add words in user dic
	QByteArray encodedString;

	QString spell_encoding = QString(pChecker -> get_dic_encoding());
	auto codec = QTextCodec::codecForName(spell_encoding.toLatin1());

	for(const auto & word : ignoredWordList){
		encodedString = codec -> fromUnicode(word);
		pChecker -> add(encodedString.data());
	}

    std::sort(ignoredWordList.begin(),ignoredWordList.end(),localeAwareLessThan);

	while(
		! ignoredWordList.empty() && 
		ignoredWordList.first().startsWith("%")
	) ignoredWordList.removeFirst();
	
	ignoredWordsModel.setStringList(ignoredWordList);
    ignoredWords = convertStringListtoSet(ignoredWordList);
	
	mLastError.clear();
	emit dictionaryLoaded();
	
	return true;
}


void SpellerUtility::saveIgnoreList(){

	if(ignoreListFileName == "")
		return;

	if(ignoredWords.count() == 0)
		return;

	QFile file(ignoreListFileName);
	
	if(file.open(QFile::WriteOnly)) {
		auto codec = QTextCodec::codecForName("UTF-8");
		file.write(codec -> fromUnicode("%Ignored-Words;encoding:utf-8;version:" TEXSTUDIO ":1.8\n"));

		for(const auto & word : ignoredWordList)
			if(!word.startsWith('%'))
				file.write(codec -> fromUnicode(word + '\n'));
	}
}


void SpellerUtility::unload(){

    QMutexLocker locker(& mSpellerMutex);
	
	saveIgnoreList();
	
	currentDic = "";
	ignoreListFileName = "";

    if(pChecker == nullptr)
		return;

	delete pChecker;
	pChecker = nullptr;
}


void SpellerUtility::addToIgnoreList(QString toIgnore){

	const auto encoding = QString(pChecker -> get_dic_encoding());
	const auto codec = QTextCodec::codecForName(encoding.toLatin1());

	auto word = latexToPlainWord(toIgnore);
	const auto encodedString = codec -> fromUnicode(word);

    QMutexLocker locker(& mSpellerMutex);

    if(!pChecker)
        return;

	pChecker -> add(encodedString.data());
	ignoredWords.insert(word);

	if(!ignoredWordList.contains(word))
        ignoredWordList.insert(
			std::lower_bound(ignoredWordList.begin(),ignoredWordList.end(),word,localeAwareLessThan),
			word
		);
	
	ignoredWordsModel.setStringList(ignoredWordList);
	
	saveIgnoreList();
	emit ignoredWordAdded(word);
}


void SpellerUtility::removeFromIgnoreList(QString toIgnore){

    QMutexLocker locker(& mSpellerMutex);

    if(pChecker)
        return;

	const auto spell_encoding = QString(pChecker -> get_dic_encoding());
	const auto codec = QTextCodec::codecForName(spell_encoding.toLatin1());
	const auto encodedString = codec -> fromUnicode(toIgnore);
	
	pChecker -> remove(encodedString.data());
	
	ignoredWords.remove(toIgnore);
	ignoredWordList.removeAll(toIgnore);
	ignoredWordsModel.setStringList(ignoredWordList);
	
	saveIgnoreList();
}


QStringListModel * SpellerUtility::ignoreListModel(){
	return & ignoredWordsModel;
}


SpellerUtility::~SpellerUtility(){
	emit aboutToDelete();
	unload();
}


bool SpellerUtility::check(QString word){

    if(currentDic == "")
		return true;

	//no speller => everything is correct

	if(pChecker == nullptr)
		return true;

	if(word.length() <= 1)
		return true;
	
	if(ignoredWords.contains(word))
		return true;
	
	if(word.endsWith('.') && ignoredWords.contains(word.left(word.length() - 1)))
		return true;
    
	QMutexLocker locker(& mSpellerMutex);
    
	if(!pChecker)
        return true;
    
	const auto encodedString = spellCodec -> fromUnicode(word);
    const bool result = pChecker -> spell(encodedString.toStdString());

	return result;
}


QStringList SpellerUtility::suggest(QString word){

	Q_ASSERT(pChecker);
    
	if(currentDic == "")
		return QStringList();

	//no speller => everything is correct

	if(pChecker == nullptr)
		return QStringList();

    QMutexLocker locker(& mSpellerMutex);

    if(!pChecker)
        return QStringList();

    const auto encodedString = spellCodec -> fromUnicode(word);
    const auto suggestions = pChecker -> suggest(encodedString.toStdString());

	QStringList suggest;

	for(const auto & suggestion : suggestions)
		suggest << spellCodec -> toUnicode(QByteArray::fromStdString(suggestion));

	return suggest;
}


SpellerManager::SpellerManager(){
	emptySpeller = new SpellerUtility("<none>");
	mDefaultSpellerName = emptySpeller -> name();
}


SpellerManager::~SpellerManager(){

	unloadAll();
	
	if(emptySpeller){
		delete emptySpeller;
        emptySpeller = nullptr;
	}
}


const auto oxtDictionary = QStringList() 
	<< "oxt" 
	<< "zip";

bool SpellerManager::isOxtDictionary(const QString & fileName){

	QFileInfo file(fileName);
	
	if(!oxtDictionary.contains(file.suffix()))
		return false;

	auto files = JlCompress::getFileList(fileName);
	
	QString affFile , dicFile;

	for(const auto & file : files) {
	
		if(file.endsWith(".aff"))
			affFile = file;

		if(file.endsWith(".dic"))
			dicFile = file;
	}

	if(affFile.length() <= 4)
		return false;
		
	if(dicFile.length() <= 4)
		return false;
	
	// different names
	
	if(affFile.left(affFile.length() - 4) != dicFile.left(dicFile.length() - 4))
		return false;
	
	return true;
}


const auto missing_hunspell = 
	"The selected file does not seem to contain a Hunspell dictionary."
	"Do you want to import it nevertheless?";

const auto empty_dictionary =
	"Dictionary import failed: "
	"No files could be extracted.";

bool SpellerManager::importDictionary(
	const QString & fileName,
	const QString & targetDir
){
	if(!isOxtDictionary(fileName)){
		const bool abort = UtilsUi::txsConfirmWarning(tr(missing_hunspell));

		if(!abort)
			return false;
	}

	QFileInfo info(fileName);
	
	auto files = JlCompress::extractDir(fileName,QDir(targetDir).filePath(info.fileName()));

	const bool emptyDictionary = files.isEmpty();

	if(emptyDictionary)
		UtilsUi::txsWarning(tr(empty_dictionary));

	return ! emptyDictionary;
}


void SpellerManager::setIgnoreFilePrefix(const QString & prefix){
	ignoreFilePrefix = prefix;
}


void SpellerManager::setDictPaths(const QStringList & dictPaths){

	if(dictPaths == m_dictPaths)	
		return;
	
	m_dictPaths = dictPaths;

	auto old = dicts.values();

	dicts.clear();
	dictFiles.clear();
	
	for(const auto & path : dictPaths)
		scanForDictionaries(path);

	// delete after new dict files are identified so a user can reload the new dict in response to a aboutToDelete signal

	for(auto dict : old)
		delete dict;

	emit dictPathChanged();
}


void SpellerManager::scanForDictionaries(const QString & path,bool scansubdirs){
	
	if(path.isEmpty())
		return;
    
	QDirIterator iterator(path);
	
	while(iterator.hasNext()){

		iterator.next();
		
		if(!iterator.fileInfo().isDir()){
			if(
				iterator.fileName().endsWith(".dic") && 
				!iterator.fileName().startsWith("hyph_")
			){
				
				QFileInfo info(iterator.fileInfo());
				
				auto realDictFile = (info.isSymLink()) 
					? QFileInfo(info.symLinkTarget()).canonicalFilePath() 
					: info.canonicalFilePath();
				
				if(dictFiles.contains(info.baseName()))
					continue;

				dictFiles.insert(info.baseName(),realDictFile);
			}

			continue;
		}
		
		if(scansubdirs)
			scanForDictionaries(iterator.filePath(),false);
	}
}


QStringList SpellerManager::availableDicts(){

	auto available = QStringList()
		<< emptySpeller -> name();

	auto dicts = dictFiles.keys();

	if(!dicts.isEmpty()){
		dicts.sort();
		available << dicts;
	}

    return available;
}


bool SpellerManager::hasSpeller(const QString & name){

	if(name == emptySpeller -> name())
		return true;
    
	if(name == "none")
		return true;
		
	if(name == "<none>")
		return true;
	
	return dictFiles.contains(name);
}


bool match(QString & guess,const QList<QString> & keys){

	for(const auto & key : keys)
		if(QString::compare(key,guess,Qt::CaseInsensitive) == 0){
			guess = key;
			return true;
		}

	return false;
}


bool SpellerManager::hasSimilarSpeller(const QString & name,QString & bestName){
	
	if(name.length() < 2)
		return false;

	auto keys = dictFiles.keys();

	// case insensitive match
	bestName = name;

	if(match(bestName,keys))
		return true;

	// match also with "_" -> "-" replacement
	
	bestName = name;
	
	if(bestName.contains('_')){

		bestName.replace('_','-');
		
		if(match(bestName,keys))
			return true;
	}

	// match also with "-" -> "_" replacement
	bestName = name;

	if(bestName.contains('-')){
		
		bestName.replace('-','_');

		if(match(bestName, keys))
			return true;
	}

	// match only beginning to beginning of keys
	bestName = name;

	QString alternative = name;
	alternative.replace('_','-');
	
	for(const auto & key : keys)
		if(
			key.startsWith(bestName,Qt::CaseInsensitive) || 
			key.startsWith(alternative)
		){
			bestName = key;
			return true;
		}

	return false;
}


/*!
    If the language has not been used yet, a SpellerUtility for the language is loaded. Otherwise
    the existing SpellerUtility is returned. Possible names are the dictionary file names without ".dic"
    ending and "<default>" for the default speller
*/

const auto failed_load = 
	"Loading of dictionary failed:\n%1\n\n%2";

SpellerUtility * SpellerManager::getSpeller(QString name){

	if(name == "<default>")
		name = mDefaultSpellerName;
	
	if(!dictFiles.contains(name))	
		return emptySpeller;

    auto speller = dicts.value(name,nullptr);

	if(!speller){
	
		speller = new SpellerUtility(name);
	
		if(!speller -> loadDictionary(dictFiles.value(name),ignoreFilePrefix)){
			UtilsUi::txsWarning(QString(failed_load)
				.arg(dictFiles.value(name))
				.arg(speller -> mLastError));
			
			delete speller;
            return nullptr;
		}

		dicts.insert(name,speller);
	}

	return speller;
}


QString SpellerManager::defaultSpellerName(){
	return mDefaultSpellerName;
}


bool SpellerManager::setDefaultSpeller(const QString & name){

	if(!dictFiles.contains(name))
		return false;

	mDefaultSpellerName = name;
	emit defaultSpellerChanged();
	return true;
}


void SpellerManager::unloadAll(){

	for(auto speller : dicts.values())
		delete speller;

	dicts.clear();
}


QString SpellerManager::prettyName(const QString & name){
	
	QLocale locale(name);
	
	if(locale == QLocale::c())
		return name;

	const auto
		country = QLocale::countryToString(locale.country()),
		language = QLocale::languageToString(locale.language());

	return QString("%1 - %2 (%3)")
		.arg(name,language,country);
}

