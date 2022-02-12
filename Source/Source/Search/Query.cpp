#include "searchquery.h"
#include "buildmanager.h"
#include "Latex/Document.hpp"


SearchQuery::SearchQuery(QString expr,QString replaceText,SearchFlags flags) 
	: mType(tr("Search"))
	, mScope(CurrentDocumentScope)
	, mModel(nullptr)
	, searchFlags(flags) {

	mModel = new SearchResultModel(this);
	mModel -> setSearchExpression(expr,replaceText,flag(IsCaseSensitive),flag(IsWord),flag(IsRegExp));
}


SearchQuery::SearchQuery(
	QString expr,
	QString replaceText,
	bool isCaseSensitive,
	bool isWord,
	bool isRegExp
) : mType(tr("Search"))
  , mScope(CurrentDocumentScope)
  , mModel(nullptr)
  , searchFlags(NoFlags) {

	setFlag(IsCaseSensitive,isCaseSensitive);
	setFlag(IsWord,isWord);
	setFlag(IsRegExp,isRegExp);
	
	if(!expr.isEmpty()){
		setFlag(ScopeChangeAllowed);
		setFlag(SearchAgainAllowed);
		setFlag(ReplaceAllowed);
	}
	
	mModel = new SearchResultModel(this);
	mModel -> setSearchExpression(expr,replaceText,flag(IsCaseSensitive),flag(IsWord),flag(IsRegExp));
}


bool SearchQuery::flag(SearchQuery::SearchFlag flag) const {
	return searchFlags & flag;
}


void SearchQuery::setFlag(SearchQuery::SearchFlag flag,bool state){
	if(state)
		searchFlags |= flag;
	else
		searchFlags &= ~flag;
}


void SearchQuery::addDocSearchResult(QDocument * document,QList<QDocumentLineHandle *> lines){
	
	SearchInfo search;
	search.doc = document;
	search.lines = lines;
	
	for(int i = 0;i < lines.count();i++)
		search.checked << true;

	mModel -> addSearch(search);
}


int SearchQuery::getNextSearchResultColumn(QString text,int column) const {
	return mModel -> getNextSearchResultColumn(text,column);
}


void SearchQuery::run(LatexDocument * document){

	mModel -> removeAllSearches();

	QList<LatexDocument *> documents;

	switch(mScope){
	case CurrentDocumentScope:
		documents << document;
		break;
	case GlobalScope:
		
		documents << document 
			-> parent 
			-> getDocuments();
		
		break;
	case ProjectScope:
		
		documents << document 
			-> getListOfDocs();
		
		break;
	default:
		break;
	}

	const auto sensitive = flag(IsCaseSensitive)
		? Qt::CaseSensitive
		: Qt::CaseInsensitive ;

	for(auto & document : documents){

		if(!document)
			continue;
		
		QList<QDocumentLineHandle *> lines;
		
		for(int l = 0;l < document -> lineCount();l++){

			l = document -> findLineRegExp(searchExpression(),l,sensitive,flag(IsWord),flag(IsRegExp));
			
			if(l < 0)
				break;
			
			lines << document
				-> line(l)
				.  handle();
		}

		// don't add empty searches

	 	if(!lines.isEmpty()){
			
			if(document -> getFileName().isEmpty() && document -> getTemporaryFileName().isEmpty())
				document -> setTemporaryFileName(BuildManager::createTemporaryFileName());

			addDocSearchResult(document,lines);
		}
	}

	emit runCompleted();
}


void SearchQuery::setReplacementText(QString text){
	mModel -> setReplacementText(text);
}


QString SearchQuery::replacementText(){
	return mModel -> replacementText();
}


void SearchQuery::setExpression(QString expr){
	mModel -> setSearchExpression(expr,flag(IsCaseSensitive),flag(IsWord),flag(IsRegExp));
}


void SearchQuery::replaceAll(){

	auto searches = mModel -> getSearches();
	auto replaceText = mModel -> replacementText();
	
	bool isWord , isCase , isReg;
	
	mModel -> getSearchConditions(isCase,isWord,isReg);
	
	for(auto search : searches){

		auto document = qobject_cast<LatexDocument *>(search.doc.data());
		
		if(!document)
			continue;
		
		auto cursor = new QDocumentCursor(document);
		
		for(int i = 0;i < search.checked.size();i++){
			if(search.checked.value(i,false)) {
                
				auto lineHandle = search.lines.value(i,nullptr);
				
				if(lineHandle){
					if(isReg){
                        
						QRegularExpression rx(searchExpression(),isCase 
							? QRegularExpression::NoPatternOption 
							: QRegularExpression::CaseInsensitiveOption);
						
						auto content = lineHandle -> text();

						QString newText = content;
						newText.replace(rx,replaceText);

						int line = document -> indexOf(lineHandle,search.lineNumberHints.value(i,-1));

						cursor -> select(line,0,line,content.length());
						cursor -> replaceSelectedText(newText);
					} else {
						
						// simple replacement
						
						auto results = mModel -> getSearchMatches(QDocumentLine(lineHandle));

						if(!results.isEmpty()){
							int line = document -> indexOf(lineHandle,search.lineNumberHints.value(i, -1));
							int offset = 0;
							
							for(const auto & match : results){
								cursor -> select(line,offset + match.pos,line,offset + match.pos + match.length);
								cursor -> replaceSelectedText(replaceText);
								offset += replaceText.length() - match.length;
							}
						}
					}
				}
			}
		}

		delete cursor;
	}
}


LabelSearchQuery::LabelSearchQuery(QString label) 
	: SearchQuery(label,label,IsWord | IsCaseSensitive | SearchAgainAllowed | ReplaceAllowed) {

	mModel = new LabelSearchResultModel(this);
	mModel -> setSearchExpression(label,label,flag(IsCaseSensitive),flag(IsWord),flag(IsRegExp));

	mScope = ProjectScope;
	mType = tr("Label Search");
	mModel -> setAllowPartialSelection(false);
}


void LabelSearchQuery::run(LatexDocument * document){

	mModel -> removeAllSearches();
	
	const auto query = searchExpression();

	auto usages = document -> getLabels(query);
	usages += document -> getRefs(query);
	
	QHash<QDocument *,QList<QDocumentLineHandle *>> usagesByDocument;
	
	for(auto use : usages.keys()) {
		const auto document = use -> document();
		auto uses = usagesByDocument[document];
		uses.append(use);
		usagesByDocument.insert(document,uses);
	}

	for(const auto document : usagesByDocument.keys())
		addDocSearchResult(document,usagesByDocument.value(document));

	emit runCompleted();
}


void LabelSearchQuery::replaceAll(){

	auto searches = mModel -> getSearches();
	auto oldLabel = searchExpression();
	auto newLabel = mModel -> replacementText();
	
	for(auto & search : searches){
		
		auto document = qobject_cast<LatexDocument *>(search.doc.data());
		
		if(document)
			document -> replaceLabelsAndRefs(oldLabel,newLabel);
	}
}

