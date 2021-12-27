
#include "textanalysis.h"

#include "smallUsefulFunctions.h"
#include "utilsUI.h"
#include "latexparser/latexreader.h"

#include "qeditor.h"
#include "qdocument.h"
#include "qdocumentline.h"
#include "filedialog.h"

#include "Latex/Document.hpp"

#include <algorithm>


ClsWord::ClsWord(QString word,int count)
	: word(word)
	, count(count) {}


bool ClsWord::operator < (const ClsWord & other) const {
	
	if(count > other.count)
		return true;
	
	if(count < other.count)
		return false;
	
	return word.localeAwareCompare(other.word) < 0;
}


int TextAnalysisModel::rowCount(const QModelIndex & parent) const {
	return (parent.isValid()) 
		? 0
		: words.size();
}


bool TextAnalysisModel::hasChildren(const QModelIndex & parent) const {
	return ! parent.isValid();
}


QVariant TextAnalysisModel::data(const QModelIndex & index,int role) const {

	if(!index.isValid())
		return QVariant();
		
	if(index.parent().isValid())
		return QVariant();

	if(index.row() >= words.size())
		return QVariant();

	if(role != Qt::DisplayRole)
		return QVariant();

	switch(index.column()){
	case 0: return words[index.row()].word;
	case 1: return words[index.row()].count;
	case 2:
		if(wordCount > 0){
			const auto count = words[index.row()].count;
			return QString::number(count * relativeProzentMultipler,'g',3) + " %";
		}
	}

	return QVariant();
}


QVariant TextAnalysisModel::headerData(int section,Qt::Orientation orientation,int role) const {

	if(role != Qt::DisplayRole)
		return QVariant();

	if(orientation != Qt::Horizontal)	
		return QString::number(section);
	
	switch(section){
	case 0 : return QString(TextAnalysisDialog::tr("Word/Phrase"));
	case 1 : return QString(TextAnalysisDialog::tr("Count","count as noun"));
	case 2 : return QString(TextAnalysisDialog::tr("Count relative"));
	default: return QVariant();
	}
}


int TextAnalysisModel::columnCount(const QModelIndex & parent) const {
	Q_UNUSED(parent)
	return 3;
}


void TextAnalysisModel::updateAll(){

	wordCount = 0;
	characterInWords = 0;
	
	for(const auto & word : words){
		wordCount += word.count;
		characterInWords += word.count * word.word.size();
	}

    std::sort(words.begin(),words.end());

	relativeProzentMultipler = (words.size() > 0)
		? 100.0 / words[0].count
		: 0;
}


TextAnalysisDialog::TextAnalysisDialog(QWidget * parent,QString name)
    : QDialog(parent)
	, document(nullptr)
	, editor(nullptr)
	, alreadyCount(false)
	, lastSentenceLength(-1)
	, lastMinSentenceLength(-1)
	, lastParsedMinWordLength(-1) {

	setWindowTitle(name);
	setAttribute(Qt::WA_DeleteOnClose);
	ui.setupUi(this);
	UtilsUi::resizeInFontHeight(this,43,51);

	ui.resultView -> setWordWrap(false);

	connect(ui.countButton,SIGNAL(clicked()),SLOT(slotCount()));
	connect(ui.closeButton,SIGNAL(clicked()),SLOT(slotClose()));
	connect(ui.searchSelectionButton,SIGNAL(clicked()),SLOT(slotSelectionButton()));
	connect(ui.exportButton,SIGNAL(clicked()),SLOT(slotExportButton()));
	connect(ui.resultView,SIGNAL(doubleClicked(const QModelIndex &)),SLOT(slotSelectionButton()));
}


TextAnalysisDialog::~TextAnalysisDialog(){}


void TextAnalysisDialog::setEditor(QEditor * aeditor){

    if(editor)
		disconnect(editor,nullptr,this,nullptr);

	if(aeditor){

		document = aeditor -> document();
		cursor = aeditor -> cursor();
		editor = aeditor;

		connect(editor, SIGNAL(destroyed()),this,SLOT(editorDestroyed()));

		return;
	}

	document = nullptr;
    cursor = nullptr;
    editor = nullptr;
}


int lowestStructureLevel(const StructureEntry * entry){

	if(!entry)	
		return 1000;

	//0 is part, 1 chapter, 2 section, ... we skip part, and take the next that exists

	if(entry -> level >= 1)
		return entry -> level;

	int level = 1000;

	for(const auto & entry : entry -> children)
		level = qMin(level,lowestStructureLevel(entry));

	return level;
}


void TextAnalysisDialog::interpretStructureTree(StructureEntry * entry){
	interpretStructureTreeRec(entry,lowestStructureLevel(entry));
}


void TextAnalysisDialog::interpretStructureTreeRec(StructureEntry * entry,int targetLevel){
	
	if(!entry)
		return;
	
	if(entry -> level == targetLevel){
		chapters.append(QPair<QString,int>(entry -> title,entry -> getCachedLineNumber()));
		ui.comboBox -> addItem(entry -> title);
		return;
	}

	for(auto & entry : entry -> children)
		interpretStructureTree(entry);
}


void TextAnalysisDialog::init(){
	alreadyCount = false;
	chapters.clear();
}


void TextAnalysisDialog::needCount(){

	if(
		alreadyCount && 
		lastSentenceLength == ui.sentenceLengthSpin -> value() &&
	    lastParsedMinWordLength == (ui.minimumLengthMeaning -> currentIndex() == 4 ? ui.minimumLengthSpin -> value() : 0) &&
	    lastEndCharacters == (ui.respectEndCharsCheck -> isChecked() ? ui.sentenceEndChars -> text() : "") &&
	    lastMinSentenceLength == (ui.wordsPerPhraseMeaning -> currentIndex() == 1 ? ui.sentenceLengthSpin -> value() : 0)
	) return;
	
	lastSentenceLength = ui.sentenceLengthSpin -> value();
	lastMinSentenceLength = (ui.wordsPerPhraseMeaning -> currentIndex() == 1 ? ui.sentenceLengthSpin -> value() : 0);
	
	int minimumWordLength = 0;
	
	if(ui.minimumLengthMeaning->currentIndex() == 4)
		minimumWordLength = ui.minimumLengthSpin->value();
	
	lastParsedMinWordLength = minimumWordLength;
	
	bool respectSentenceEnd = ui.respectEndCharsCheck -> isChecked();
	lastEndCharacters = respectSentenceEnd ? ui.sentenceEndChars -> text() : "";

	for(int i = 0;i < 3;i++){
		
		maps[i].resize(2 + chapters.size());
		
		for(int j = 0;j < maps[i].size();j++)
			maps[i][j].clear();
		
		lineCount[i].resize(2 + chapters.size());
		
		for(int j = 0;j < lineCount[i].size();j++)
			lineCount[i][j] = 0;
	}
	
	lineCount[0][0] = document -> lines();

	int
		selectionStartIndex = -1,
		selectionStartLine = -1,
		selectionEndIndex = -1,
		selectionEndLine = -1;

	if(cursor.hasSelection()){

		QDocumentSelection sel = cursor.selection();
		selectionStartLine = sel.startLine;
		selectionEndLine = sel.endLine;
		selectionStartIndex = sel.start;
		selectionEndIndex = sel.end;
		
		if (selectionStartLine > selectionEndLine){
			std::swap(selectionStartLine,selectionEndLine);
			std::swap(selectionStartIndex,selectionEndIndex);
		} else
		if(
			selectionStartLine == selectionEndLine &&
			selectionStartIndex > selectionEndIndex
		){
			std::swap(selectionStartIndex,selectionEndIndex);
		}

		lineCount[0][1] = selectionEndLine - selectionStartLine + 1;
	}

	int nextChapter = 0;
	int extraMap = 0;
	QList<QString> lastWords[3];
	int sentenceLengths[3] = { 0 , 0 , 0 };

	for(int l = 0;l < document -> lines();l++){

		if(nextChapter < chapters.size() && l + 1 >= chapters[nextChapter].second) {
			if (nextChapter == 0) extraMap = 2;
			else extraMap++;
			nextChapter++;
			if (extraMap >= maps[0].size()) extraMap = 0;
		}

		if(extraMap != 0)
			lineCount[0][extraMap]++;
		
		QString line = document -> line(l).text();
		
		bool commentReached = false;
		bool lineCountedAsText = false;
		int state;
		int lastIndex = 0;

		LatexReader lr(line);
		
		while((state = lr.nextWord(true)) != LatexReader::NW_NOTHING){

			if(lr.word.endsWith('.'))
				lr.word.chop(1);
			
			bool inSelection = (selectionStartLine == selectionEndLine)
				? (l == selectionStartLine) && 
				  (lr.index > selectionStartIndex) && 
				  (lr.wordStartIndex <= selectionEndIndex)
				: (l < selectionEndLine) && (l > selectionStartLine) ||
				  (l == selectionStartLine) && (lr.index > selectionStartIndex) ||
				  (l == selectionEndLine) && (lr.wordStartIndex <= selectionEndIndex);
				
			
			auto curWord = lr.word.toLower();
			int curType = -1;

			if(commentReached){
				if(respectSentenceEnd) 
					for(int l = lastIndex;l < lr.wordStartIndex;l++)
						if(lastEndCharacters.contains(line.at(l))){
							sentenceLengths[2] = 0;
							lastWords[2].clear();
							break;
						}

				curType = 2;

			} else 
			if(state == LatexReader::NW_COMMENT){

				//comment is only % character
				curType = 2;

				if(!commentReached){
					
					commentReached = true;
					
					lineCount[2][0]++;

					if(inSelection)
						lineCount[2][1]++;
					
					if(extraMap != 0)
						lineCount[2][extraMap]++;
					
					//find sentence end characters which belong to the words before the comment start
					
					if(respectSentenceEnd)
						for(int i = lastIndex;i < lr.wordStartIndex;i++){
							
							if(
								line.at(i) == QChar('%') && 
								(i == 0 || line.at(i - 1) != QChar('\\'))
							){
								lastIndex = i;
								break;
							}

							if(lastEndCharacters.contains(line.at(i))){
								sentenceLengths[0] = 0;
								lastWords[0].clear();
								break;
							}
						}
				}
			} else
			if(state == LatexReader::NW_COMMAND){
				curType = 1;
			} else 
			if(state == LatexReader::NW_TEXT){
			
				curType = 0;
			
				if(!lineCountedAsText){

					lineCountedAsText = true;
					lineCount[1][0]++;
					
					if(inSelection)
						lineCount[1][1]++;
					
					if(extraMap != 0)
						lineCount[1][extraMap]++;
				}
			}

			if(respectSentenceEnd && !commentReached)
				for(int i = lastIndex;i < lr.wordStartIndex;i++)
					if(lastEndCharacters.contains(line.at(i))){
						
						sentenceLengths[0] = 0;
						lastWords[0].clear();
						
						break;
					}
					
			lastIndex = lr.index;
			
			if(curType != -1 && curWord.size() >= minimumWordLength){

				lastWords[curType].append(curWord);
				sentenceLengths[curType] += curWord.size() + 1;

				if(lastWords[curType].size() > lastSentenceLength){
					sentenceLengths[curType] -= lastWords[curType].first().size() + 1;
					lastWords[curType].removeFirst();
				}

				if(lastWords[curType].size() >= lastMinSentenceLength){

					curWord = "";
					curWord.reserve(sentenceLengths[curType]);
					
					for(const auto & word : lastWords[curType])
						curWord += word + " ";

					curWord.truncate(curWord.size() - 1);
					maps[curType][0][curWord] = maps[curType][0][curWord] + 1;
					
					if(inSelection)
						maps[curType][1][curWord] = maps[curType][1][curWord] + 1;
					
					if(extraMap != 0)
						maps[curType][extraMap][curWord] = maps[curType][extraMap][curWord] + 1;
				}
			}
		}


		//it makes sense to ignore in something like .%. the sentence end after the % (and is less work)

		const auto c = (commentReached) ? 2 : 0 ;

		if(respectSentenceEnd)
			for(int i = lastIndex;i < line.size();i++)
				if(lastEndCharacters.contains(line.at(i))){
					sentenceLengths[c] = 0;
					lastWords[c].clear();
					break;
				}
	}

	alreadyCount = true;
}


void TextAnalysisDialog::insertDisplayData(const QMap<QString,int> & map){

	int minLen = 0;
	int minCount = ui.minimumCountSpin->value();
	int phraseLength = ui.sentenceLengthSpin->value();
	
	bool filtered = (ui.filter -> currentIndex()) != 0;
	QString curFilter = ui.filter -> currentText();
	
	auto wordFilter = (
		ui.filter -> currentIndex() != -1 &&
	    ui.filter -> itemText(ui.filter -> currentIndex()) == curFilter
	)
		? QRegExp(curFilter.mid(curFilter.indexOf('(')))
		: QRegExp(curFilter);


	auto words = displayed.words;


	switch(ui.minimumLengthMeaning -> currentIndex()){
	//at least one word must have min length, all shorter with space: (min-1 +1)*phraseLength-1
	break ; case 2:

		//no break!
		minLen = ui.minimumLengthSpin -> value();
	
		for(auto const & [ key , value ] : map.toStdMap())
			if(value >= minCount)
				if(key.size() < minLen * phraseLength){

					if(filtered && ! wordFilter.exactMatch(key))
						continue;
					
					int last = 0;
					int i = 0;
					
					for(;i < key.size();i++)
						if(key.at(i) == ' '){
							if(i - last >= minLen){
								words.append(ClsWord(key,value));
								break;
							} else {
								last = i + 1;
							}
						}

					if(i == key.size() && i - last >= minLen)
						words.append(ClsWord(key,value));
				} else {
					if(filtered || wordFilter.exactMatch(key))
						words.append(ClsWord(key,value));
				}

	//all words must have min len, (min +1)*phraseLength-1
	break ; case 3:

		minLen = ui.minimumLengthSpin -> value();

		for(auto it = map.constBegin();it != map.constEnd();++it)
			if(it.value() >= minCount && it.key().size() >= minLen){
				//not minLen*phraseCount because there can be less words in a phrase
			
				if(filtered && !wordFilter.exactMatch(it.key()))
					continue;
				
				QString t = it.key();
				int last = 0;
				bool ok = true;
				
				for(int i = 0;i < t.size();i++)
					if(t.at(i) == ' '){
						if(i - last < minLen){
							ok = false;
							break;
						} else {
							last = i + 1;
						}
					}

				if(ok && t.size() - last >= minLen)
					words.append(ClsWord(it.key(),it.value()));
			}
	break ; case 1:
		minLen = ui.minimumLengthSpin -> value(); //no break!
        [[gnu::fallthrough]];
	default:

		const auto isRelevant = [ & ](const auto key,const auto value) -> bool {

			if(value < minCount)
				return false;

			if(key.size() < minLen)
				return false;

			if(filtered)
				return wordFilter.exactMatch(key);

			return true;
		};


		for(auto const & [ key , value ] : map.toStdMap())
			if(isRelevant(key,value))
				words.append(ClsWord(key,value));
	}
}


void TextAnalysisDialog::slotCount(){

	needCount();
	displayed.words.clear(); //insert into map to sort
	
	int currentMap = ui.comboBox -> currentIndex();
	
	if(ui.normalTextCheck -> isChecked())
		insertDisplayData(maps[0][currentMap]);
	
	if(ui.commandsCheck -> isChecked()){
		insertDisplayData(maps[1][currentMap]);
		insertDisplayData(maps[2][currentMap]);
	}

	displayed.updateAll();

    ui.resultView -> setModel(nullptr);
	ui.resultView -> setModel(& displayed);

	ui.resultView -> setShowGrid(false);
	ui.resultView -> resizeRowsToContents();

	ui.totalLinesLabel -> setText(QString::number(lineCount[0][currentMap]));
	ui.textLinesLabel -> setText(QString::number(lineCount[1][currentMap]));
	ui.commentLinesLabel -> setText(QString::number(lineCount[2][currentMap]));

	ui.displayedWordLabel -> setText(QString::number(displayed.wordCount));
	ui.differentWordLabel -> setText(QString::number(displayed.words.size()));
	ui.characterInWordsLabel -> setText(QString::number(displayed.characterInWords));
}


void TextAnalysisDialog::editorDestroyed(){
    setEditor(nullptr);
}


void TextAnalysisDialog::slotSelectionButton(){

	if(!editor)
		return;
	
	int s = ui.resultView -> currentIndex().row();
	
	if(s < 0)
		return;
		
	if(s >= displayed.words.size())
		return;
	
	QString selected = displayed.words[s].word;

	if(selected == "")
		return;
	
	if(lastSentenceLength > 1 && selected.contains(' ')){
		selected.replace(' ',"\\W+");
		editor -> find(selected,true,true);
	} else {
		editor->find(selected, true, false);
	}
}


QByteArray escapeAsCSV(const QString & string){

	auto bytes = string.toUtf8();
	
	if(bytes.contains(',')){
		bytes.replace('"',"\"\"");
		bytes.prepend('"');
		bytes.append('"');
	}

	return bytes;
}


const auto newline = 
	#ifdef Q_OS_WIN
		"\r\n";
	#else
		"\n";
	#endif

void TextAnalysisDialog::slotExportButton(){

	QString fn = FileDialog::getSaveFileName(this,tr("CSV Export"),editor 
		? editor -> document() -> getFileName().replace(".tex", ".csv") 
		: QString(),
		tr("CSV file") + " (*.csv)" ";;" + tr("All files") + " (*)"
	);
	
	if(fn.isEmpty())
		return;
	
	QFile file(fn);
	
	if(!file.open(QFile::WriteOnly))
		return;

	for(const auto & word : displayed.words)
		file.write(escapeAsCSV(word.word) + ',' + QByteArray::number(word.count) + newline);
}


void TextAnalysisDialog::slotClose(){
	close();
}

