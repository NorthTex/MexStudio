
#include "grammarcheck.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexreader.h"

#include <QThread>
#include <QJsonDocument>
#include <QJsonArray>


GrammarError::GrammarError()
	: offset(0)
	, length(0)
	, error(GET_UNKNOWN) {}


GrammarError::GrammarError(
	int offset,
	int length,
	const GrammarErrorType & error,
	const QString & message
)
	: offset(offset)
	, length(length)
	, error(error)
	, message(message) {}


GrammarError::GrammarError(
	int offset,
	int length,
	const GrammarErrorType & error,
	const QString & message,
	const QStringList & corrections
)
	: offset(offset)
	, length(length)
	, error(error)
	, message(message)
	, corrections(corrections) {}


GrammarError::GrammarError(
	int offset,
	int length,
	const GrammarError & other
)
	: offset(offset)
	, length(length)
	, error(other.error)
	, message(other.message)
	, corrections(other.corrections) {}


GrammarCheck::GrammarCheck(QObject * parent) 
	: QObject(parent)
	, ltstatus(LTS_Unknown)
	, backend(nullptr)
	, ticket(0)
	, pendingProcessing(false)
	, shuttingDown(false) {

	latexParser = new LatexParser();
}


/*!
 * \brief GrammarCheck::~GrammarCheck
 * Destructor
 */

GrammarCheck::~GrammarCheck(){
	delete latexParser;
    delete backend;
}


/*!
 * \brief GrammarCheck::init
 * initialize grammar checker
 * \param lp reference to latex parser
 * \param config reference to config
 */

void GrammarCheck::init(
	const LatexParser & parser,
	const GrammarCheckerConfig & config
){
	* latexParser = parser;
	
	this -> config = config;

    if(!backend){

        backend = new GrammarCheckLanguageToolJSON(this);

        connect(backend,SIGNAL(checked(uint,int,QList<GrammarError>)),this,SLOT(backendChecked(uint,int,QList<GrammarError>)));
        connect(backend,SIGNAL(errorMessage(QString)),this,SIGNAL(errorMessage(QString)));
        connect(backend,SIGNAL(languageToolStatusChanged()),this,SLOT(updateLTStatus()));
    }

	backend -> init(config);

	if(floatingEnvs.isEmpty())
		floatingEnvs 
			<< "figure" 
			<< "table" 
			<< "SCfigure" 
			<< "wrapfigure" 
			<< "subfigure" 
			<< "floatbox";

	// this is a heuristic, some LanguageTool languages have the format de-DE, others just it (instead of it-IT)
	// this list contains all two-character languages that do not have a four-character equivalent.
	
	languageMapping.insert("ca-CA","ca");
	languageMapping.insert("en-EN","en");
	languageMapping.insert("es-ES","es");
	languageMapping.insert("fa-FA","fa");
	languageMapping.insert("fr-FR","fr");
	languageMapping.insert("it-IT","it");
	languageMapping.insert("nl-NL","nl");
	languageMapping.insert("sv-SV","sv");
}

QString GrammarCheck::serverUrl(){
    return backend -> url();
}

QString GrammarCheck::getLastErrorMessage(){
    return backend -> getLastErrorMessage();
}


/*!
 * \brief readWordList
 * Read bad words/stop words from file
 * \param file
 * \return word list as set
 */

QSet<QString> readWordList(const QString & file){

	QFile f(file);
	
	if(!f.open(QFile::ReadOnly))
		return QSet<QString>();
	
	QSet<QString> words;
	
	for(const auto & line : f.readAll().split('\n')){
		
		auto word = line.simplified();
		
		if(word.startsWith('#'))
			continue;
		
		words << QString::fromUtf8(word);
	}

	return words;
}


struct TokenizedBlock {
	
	QStringList words;
	
	QList<int> 
		indices , 
		endindices , 
		lines;
};


struct CheckRequest {

	bool pending;
	QString language;
    LatexDocument * doc;
	QList<LineInfo> inlines;
	int firstLineNr;
	uint ticket;
	int handledBlocks;
    
	CheckRequest(
		const QString & language,
		LatexDocument * doc,
		const QList<LineInfo> inlines,
		const int firstLineNr,
		const uint ticket
	)
		: pending(true)
		, language(language)
		, doc(doc)
		, inlines(inlines)
		, firstLineNr(firstLineNr)
		, ticket(ticket)
		, handledBlocks(0) {}

	QList<int> linesToSkip;
	QList<TokenizedBlock> blocks;
	QVector<QList<GrammarError>> errors;
};


void GrammarCheck::check(
	const QString & language,
	LatexDocument * doc,
	const QList<LineInfo> & inlines,
	int firstLineNr
){
	if(shuttingDown)
		return;
		
	if(inlines.isEmpty())
		return;

	ticket++;

	for(int i = 0;i < inlines.size();i++){

		auto it = tickets.find(inlines[i].line);
		
		if(it == tickets.end())
			tickets[inlines[i].line] = QPair<uint,int>(ticket,1);
		else {
			it.value().first = ticket;
			it.value().second++;
		}
	}

	requests << CheckRequest(
		languageFromHunspellToLanguageTool(language),
		doc,inlines,firstLineNr,ticket
	);

	// Delay processing, because there might be more requests for the same line in the event queue and only the last one needs to be checked
	
	if(pendingProcessing)
		return;

	pendingProcessing = true;

	QTimer::singleShot(50,this,SLOT(processLoop()));
}


void GrammarCheck::shutdown(){

	if(backend)
		backend -> shutdown();
	
	shuttingDown = true;
	deleteLater();
}


void GrammarCheck::processLoop(){

	if(shuttingDown)
		return;

	for(int i = requests.size() - 1;i >= 0;i--)
		if(requests[i].pending){
			requests[i].pending = false;
			process(i);
		}
	
	pendingProcessing = false;
}


const QString uselessPunctation = "!:?,.;-"; //useful: \"(
const QString punctuationNotPreceededBySpace = "!:?,.;)\u00A0";  // \u00A0 is non-breaking space: assuming it is not surrounded by natural space (wouldn't make sense)
const QString punctuationNotFollowedBySpace = "(\"\u00A0";


/* Determine if words[i] should be preceeded by a space in the context of words.
 * If not continue.
 * If i > words.length() break;
 * This does always increase i by one. (Note: the checks below are written based on i++)
 * This is used in loops for selectively joining words with spaces. */


#define preceedWithSpaces(i,words)                                      \
                                                                        \
    (i)++;                                                              \
                                                                        \
    if((i) >= words.length())                                           \
        break;                                                          \
                                                                        \
    if(words[i].length() == 1                                           \
        && punctuationNotPreceededBySpace.contains(words[i][0]))        \
            continue;                                                   \
                                                                        \
    if(words[(i) - 1].length() == 1                                     \
        && punctuationNotFollowedBySpace.contains(words[(i) - 1][0]))   \
            continue;                                                   \
                                                                        \
    if(words[(i) - 1].length() == 2 && words[(i) - 1][1] == '.'         \
        && words[i].length() == 2 && words[i][1] == '.')                \
            continue;



void GrammarCheck::process(int requestId){

	REQUIRE(latexParser);
	REQUIRE(!requests.isEmpty());

	auto & request = requests[requestId];

    request.linesToSkip.reserve(request.inlines.size());

	if(request.ticket != ticket){

		//processing an old request
		for(int i = 0;i < request.inlines.size();i++){
			
			auto it = tickets.find(request.inlines[i].line);
			
			Q_ASSERT(it != tickets.end());
			
			if(it == tickets.end())
				continue;
			
			if(it.value().first != request.ticket){
				
				it.value().second--;
				
				Q_ASSERT(it.value().second >= 0);
				
				request.linesToSkip << i;

				if(it.value().second <= 0)
					tickets.erase(it);
			}
		}

		if(request.linesToSkip.size() == request.inlines.size()){
			requests.removeAt(requestId);
			return;
		}
	}

	//gather words
	int nonTextIndex = 0;

	QList<TokenizedBlock> blocks;
	blocks << TokenizedBlock();

	int floatingBlocks = 0;
	int firstLineNr = request.firstLineNr;

	for(int l = 0;l < request.inlines.size();l++,firstLineNr++){
		
		auto & tb = blocks.last();
		
		LatexReader lr(* latexParser,request.inlines[l].text);
		
		int type;
		
		while((type = lr.nextWord(false))){

			if(type == LatexReader::NW_ENVIRONMENT){
			
				if(lr.word == "figure"){
					if(lr.lastCommand == "\\begin"){
						floatingBlocks ++;
					} else if (lr.lastCommand == "\\end"){
						floatingBlocks --;
					}
				}
			
				continue;
			}

			if(type == LatexReader::NW_COMMENT)
				break;
			
			if(type != LatexReader::NW_TEXT && type != LatexReader::NW_PUNCTATION){
				//reference, label, citation
				tb.words << QString("keyxyz%1").arg(nonTextIndex++);
				tb.indices << lr.wordStartIndex;
				tb.endindices << lr.index;
				tb.lines << l;
				continue;
			}

			if(latexParser -> structureCommandLevel(lr.lastCommand) >= 0){

				//don't check captions
				QStringList temp;
				QList<int> starts;
				
				LatexParser::resolveCommandOptions(lr.line,lr.wordStartIndex - 1,temp,& starts);
				
				for(int j = 0;j < starts.count() && j < 2;j++){
					
					lr.index = starts.at(j) + temp.at(j).length() - 1;
					
					if(temp.at(j).startsWith('{'))
						break;
				}

				tb.indices << lr.wordStartIndex;
				tb.endindices << 1;
				tb.words << ".";
				tb.lines << l;
				
				continue;
			}


			if(type == LatexReader::NW_TEXT)
				tb.words << lr.word;
			else 
			if(type == LatexReader::NW_PUNCTATION){
				if((lr.word == "-") && !tb.words.isEmpty()){
				
					//- can either mean a word-separator or a sentence -- separator
					// => if no space, join the words at both sides of the - (this could be easier handled in nextToken, but spell checking usually doesn't support - within words)
				
					if(lr.wordStartIndex == tb.endindices.last()){

                        tb.words.last() += lr.word;
						tb.endindices.last()++;

						int tempIndex = lr.index;
						int type = lr.nextWord(false);
						
						if(type == LatexReader::NW_COMMENT)
							break;
						
						if(tempIndex != lr.wordStartIndex){
							lr.index = tempIndex;
							continue;
						}

						tb.words.last() += lr.word;
						tb.endindices.last() = lr.index;
						
						continue;
					}
				} else 
				// replace " by ' because " is encoded as &quot; and screws up the (old) LT position calculation
				if(lr.word == '"'){
					lr.word = '\'';  
				} else 
				// rewrite LaTeX non-breaking space to unicode non-braking space
				if(lr.word == '~'){
					lr.word =  "\u00A0";
				} else 
				// rewrite non-breaking space followed by punctuation to punctuation only. e.g. "figure~\ref{abc}." -> "figure."
					// \ref{} is dropped by the reader and an erronous would leave "figure\u00A0."
					// As a heuristic no space before punctuation takes precedence over non-breaking space.
					// This is the best we can do for now. A complete handling of all cases is only possible with a full tokenization.
				if(punctuationNotPreceededBySpace.contains(lr.word) && !tb.words.isEmpty() && tb.words.last() == "\u00A0") {
					tb.endindices.last() = lr.index;
					tb.words.last() = lr.word;
					continue;
				}

				tb.words << lr.word;
			}

			tb.indices << lr.wordStartIndex;
			tb.endindices << lr.index;
			tb.lines << l;
		}


		while(floatingBlocks > blocks.size() - 1)
			blocks << TokenizedBlock();
		
		while(floatingBlocks >= 0 && floatingBlocks < blocks.size() - 1)
			request.blocks << blocks.takeLast();
	}

	while(blocks.size()) 
		request.blocks << blocks.takeLast();

	for(auto & block : request.blocks){

		auto & words = block.words;

		while(
			! words.isEmpty() && 
			words.first().length() == 1 && 
			uselessPunctation.contains(words.first()[0])
		){
			block.endindices.removeFirst();
			block.indices.removeFirst();
			block.lines.removeFirst();
			words.removeFirst();
		}
	}


    auto newstatus = (backend -> isWorking()) 
		? LTS_Working 
		: LTS_Error;

	if(newstatus != ltstatus){
		ltstatus = newstatus;
		emit languageToolStatusChanged();
	}


	//cr itself might become invalid during the following loop
//	auto blocks = request.blocks;
    auto ticket = request.ticket;
	auto language = request.language;

	int b = 0;

	for(auto & block : request.blocks){
		
		if(block.words.isEmpty() || ! backend -> isAvailable()){
			backendChecked(ticket,b,QList<GrammarError>(),true);
			b++;
			continue;
		}

		b++;

		const auto & words = block.words;
		int expectedLength = 0;
		QString combined;
		
		for(const auto & word : words)
			expectedLength += word.length();
		
		combined.reserve(expectedLength + words.length());
		
		for(int i = 0;;){
			combined += words[i];
			preceedWithSpaces(i,words)
			combined += ' ';
		}

		backend -> check(ticket,b,language,combined);
	}
}


void GrammarCheck::backendChecked(
	uint crticket,
	int subticket,
	const QList<GrammarError> & backendErrors,
	bool directCall
){
	
	if(shuttingDown)
		return;
	
	int requestId = -1;
	
	for(int i = requests.size() - 1;i >= 0;i--)
		if(requests[i].ticket == crticket)
			requestId = i;

	if(requestId == -1)
		return;

	auto & request = requests[requestId];

	REQUIRE(subticket >= 0 && subticket < request.blocks.size());

	auto & block = request.blocks[subticket];


	const auto appendError2 = [ & ](auto index,auto type,auto description,auto removedChars){
		request.errors[block.lines[index]]
			<< GrammarError(
				block.indices[index],
				block.endindices[index] - block.indices[index] - removedChars,
				type,
				description,
				QStringList() << ""
			);
	};

	const auto appendError = [ & ](auto index,auto type,auto description){
		appendError2(index,type,description,0);
	};


	
	request.handledBlocks++;

	for(int i = 0;i < request.inlines.size();i++){

		if(request.linesToSkip.contains(i))
			continue;
		
		auto it = tickets.find(request.inlines[i].line);
		
		Q_ASSERT(it != tickets.end());
		
		if(it == tickets.end())
			continue;
		
		Q_ASSERT(it.value().second >= 0);
		
		bool remove = request.handledBlocks == request.blocks.size();
		
		if(it.value().first != request.ticket){
			request.linesToSkip << i;
			remove = true;
		}

		if(remove){
			
			it.value().second--;
			
			if(it.value().second <= 0)
				tickets.erase(it);
		}
	}

	if(request.linesToSkip.size() == request.inlines.size()) {
		requests.removeAt(requestId);
		return;
	}

	auto it = languages.constFind(request.language);
	
	if(it == languages.constEnd()){

		LanguageGrammarData lgd;
        
		QString path = config.wordlistsDir + '/' + languageFromLanguageToolToHunspell(request.language) + ".stopWords";
        
		path = ConfigManagerInterface::getInstance() -> parseDir(path);
        
		lgd.stopWords = readWordList(path);
        
		path = config.wordlistsDir + '/' + languageFromLanguageToolToHunspell(request.language) + ".badWords";
        path = ConfigManagerInterface::getInstance() -> parseDir(path);
        
		lgd.badWords = readWordList(path);
		
		languages.insert(request.language,lgd);
		
		it = languages.constFind(request.language);
	}

	const auto & ld = * it;

	if(request.errors.size() != request.inlines.size())
		request.errors.resize(request.inlines.size());

	auto & words = block.words;

	if(config.longRangeRepetitionCheck){

		const int MAX_REP_DELTA = config.maxRepetitionDelta;
		bool checkLastWord = directCall;
		
		QString prevSW;
		//check repetition
		QHash<QString, int> repeatedWordCheck;
		int totalWords = 0;

		for(int w = 0 ; w < words.size(); w++) {
			
			totalWords++;
			
			if(words[w].length() == 1  && getCommonEOW().contains(words[w][0]))
				continue; //punctation

			//check words
			int truncatedChars = 0;
			QString normalized = words[w].toLower();

			if(normalized.endsWith('.')){
				normalized = normalized.left(normalized.length() -1);
				truncatedChars =  1;
			}

			if(ld.stopWords.contains(normalized)){
				
				if(checkLastWord){
				
					if(prevSW == normalized)
						appendError(w,GET_WORD_REPETITION,tr("Word repetition"));
						// request.errors[block.lines[w]] 
							// << GrammarError(block.indices[w],block.endindices[w] - block.indices[w], GET_WORD_REPETITION, tr("Word repetition"), QStringList() << "");
				
					prevSW = normalized;
				}

				continue;
			} else {
				prevSW.clear();
			}

            int lastSeen = repeatedWordCheck.value(normalized,-1);

            if(lastSeen > -1){

                int delta = totalWords - lastSeen;
              
			    if(delta <= MAX_REP_DELTA){
					appendError2(w,GET_WORD_REPETITION,tr("Word repetition. Distance %1").arg(delta),truncatedChars);
                    // request.errors[block.lines[w]] 
					// 	<< GrammarError(block.indices[w],block.endindices[w] - block.indices[w] - truncatedChars, GET_WORD_REPETITION, tr("Word repetition. Distance %1").arg(delta), QStringList() << "");
				}
				
				if(config.maxRepetitionLongRangeDelta > config.maxRepetitionDelta && delta <= config.maxRepetitionLongRangeDelta && normalized.length() >= config.maxRepetitionLongRangeMinWordLength)
                    appendError(w,GET_LONG_RANGE_WORD_REPETITION,tr("Long range word repetition. Distance %1").arg(delta));
					// request.errors[block.lines[w]] 
						// << GrammarError(block.indices[w],block.endindices[w] - block.indices[w], GET_LONG_RANGE_WORD_REPETITION, tr("Long range word repetition. Distance %1").arg(delta), QStringList() << "");
            }

			repeatedWordCheck.insert(normalized, totalWords);
		}
	}

	if(config.badWordCheck)
		for(int w = 0;w < words.size();w++){

			QString normalized = words[w].toLower();
			
			if(ld.badWords.contains(normalized)){
				appendError(w,GET_BAD_WORD,tr("Bad word"));
				
				// request.errors[block.lines[w]] 
				// 	<< GrammarError(block.indices[w],block.endindices[w] - block.indices[w],GET_BAD_WORD,tr("Bad word"),QStringList() << "");
				
				continue;
			}

			if(
				normalized.length() > 1 && 
				normalized.endsWith('.') && 
				ld.badWords.contains(normalized.left(normalized.length() - 1))
			)
				appendError2(w,GET_BAD_WORD,tr("Bad word"),1);
				// request.errors[block.lines[w]] 
					// << GrammarError(block.indices[w],block.endindices[w] - block.indices[w] - 1,GET_BAD_WORD,tr("Bad word"),QStringList() << "");
		}


	//map indices to latex lines and indices

	int 
		curOffset = 0,
		curWord = 0,
		err = 0;
	
	while(err < backendErrors.size()){
		
		if(backendErrors[err].offset >= curOffset + words[curWord].length()){
			curOffset += words[curWord].length();
            preceedWithSpaces(curWord, words)
			curOffset++; //space
		} else {

			int trueIndex = block.indices[curWord] + qMax(0,backendErrors[err].offset - curOffset);
			int trueLength = -1;
			int offsetEnd = backendErrors[err].offset + backendErrors[err].length;
			int tempOffset = curOffset;

			for(int w = curWord;;){
				
				tempOffset += words[w].length();
				
				if(tempOffset >= offsetEnd){

					if(block.lines[curWord] == block.lines[w]){
						int trueOffsetEnd = block.endindices[w] - qMax(0,tempOffset - offsetEnd);
						trueLength = trueOffsetEnd - trueIndex;
					}
					
					break;
				}

                preceedWithSpaces(w,words)
				
				tempOffset++; //space
			}
			
			if(trueLength == -1)
				trueLength = request.inlines[block.lines[curWord]].text.length() - trueIndex;
			
			request.errors[block.lines[curWord]] 
				<< GrammarError(trueIndex,trueLength,backendErrors[err]);
			
			err++;
		}
	}

	if(request.handledBlocks == request.blocks.size()) {
	
		//notify
		for(int l = 0;l < request.inlines.size();l++){
			
			//too late

			if(request.linesToSkip.contains(l))
				continue; 

			emit checked(request.doc,request.inlines[l].line,request.firstLineNr + l,request.errors[l]);
		}

		requests.removeAt(requestId);
	}
}


void GrammarCheck::updateLTStatus(){

    const auto status = (backend -> isWorking()) 
		? LTS_Working 
		: LTS_Error;
    
	if(ltstatus == status)
		return;
		
	ltstatus = status;
	emit languageToolStatusChanged();
}


/*!
 * Reformats a language identifier from the Hunspell notation to LanguageTool notation
 * e.g. en_GB -> en-GB and it_IT -> it
 */

QString GrammarCheck::languageFromHunspellToLanguageTool(QString language){
	
	language.replace('_','-');
	
	return languageMapping.value(language,language);
}


/*!
 * Reformats a language identifier from the LanguageTool notation to hunspell notation
 * e.g. en-GB -> en_GB and it -> it
 */

QString GrammarCheck::languageFromLanguageToolToHunspell(QString language){

    if(languageMapping.values().contains(language))
        language = languageMapping.key(language);
    
	language.replace('-', '_');
    
	return language;
}


GrammarCheckBackend::GrammarCheckBackend(QObject * parent)
	: QObject(parent) {}



#undef preceedWithSpaces
