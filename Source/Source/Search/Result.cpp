
#include "Search/ResultModel.hpp"
#include "Search/LabelResultModel.hpp"
#include "qdocument.h"
#include "qdocumentsearch.h"
#include "smallUsefulFunctions.h"


/* *** internal id (iid) semantics ***
 * the internal id associates QModelIndex with the internal data structure. The iid is structured as followed
 *
 * < search index >  < line index >
 * 00000000 00000000 00000000 00000000
 *
 * - The upper 16 bits encode the search (i.e. a top-level group):
 *     m_searches[i] maps to (i + 1) * (1 << 16)
 * - Bit 16 (1<<15) is a flag indcating that the item is a result entry within a search
 * - Bits 1-15 indicate the position of the result entry within the search
 *
 * Example:
 *  * tree structure *   ** iid **     * internal data *
 *  Search in file A     0x00010000    searches[0]
 *    Result entry a0    0x00018000    searches[0].lines[0]
 *    Result entry a1    0x00018001    searches[0].lines[1]
 *  Search in file B     0x00020000    seraches[1]
 *    Result entry a0    0x00018000    searches[1].lines[0]
 *    Result entry a1    0x00018001    searches[1].lines[1]
 */


const quint32 
	ResultEntryFlag = (1 << 15),
	SearchMask = 0xFFFF0000;


bool iidIsResultEntry(quint32 id){
    return id & ResultEntryFlag;

}

quint32 makeResultEntryIid(quint32 searchId,int row){
    return searchId + ResultEntryFlag + row;
}


quint32 searchIid(quint32 id){
    return id & SearchMask;
}


int searchIndexFromIid(quint32 id){
	return int(id >> 16) - 1;
}


quint32 iidFromSearchIndex(int index){
	return (index + 1) * (1 << 16);
}


SearchResultModel::SearchResultModel(QObject * parent)
	: QAbstractItemModel(parent)
	, mIsWord(false)
	, mIsCaseSensitive(false)
	, mIsRegExp(false)
	, mAllowPartialSelection(true) {

	m_searches.clear();
	mExpression.clear();
}


SearchResultModel::~SearchResultModel(){}


void SearchResultModel::addSearch(const SearchInfo & search){

	beginResetModel();

    m_searches.append(search);

	int line = 0;
	
	m_searches.last().lineNumberHints.clear();
	
	for(const auto & text : search.lines){
		
		line = search.doc -> indexOf(text,line);
		
		m_searches.last().lineNumberHints
			<< line;
	}

	endResetModel();
}


QList<SearchInfo> SearchResultModel::getSearches(){
	return m_searches;
}


void SearchResultModel::clear(){

	beginResetModel();

	m_searches.clear();
	mExpression.clear();
	mAllowPartialSelection = true;

	endResetModel();
}


void SearchResultModel::removeSearch(const QDocument *){
	return;
	// TODO: currently unused, also it requires beginResetModel() or similar
//	for (int i = m_searches.size() - 1; i >= 0; i--)
//		if (m_searches[i].doc == document)
//			m_searches.removeAt(i);
}


void SearchResultModel::removeAllSearches(){
	beginResetModel();
	m_searches.clear();
	endResetModel();
}


int SearchResultModel::columnCount(const QModelIndex &) const {
    return 1;
}


int SearchResultModel::rowCount(const QModelIndex & index) const {

	if(!index.isValid())
		return m_searches.size();

	const int i = index.row();

	// maximum search results limited

	if(i >= m_searches.size())
		return 0;

	if(iidIsResultEntry(index.internalId()))
		return 0;

	return qMin(m_searches[i].lines.size(),1000);
}


QModelIndex SearchResultModel::index(int row,int column,const QModelIndex & index) const {

	if(index.isValid())
		return createIndex(row,column,makeResultEntryIid(index.internalId(),row));

	return createIndex(row,column,iidFromSearchIndex(row));
}


QModelIndex SearchResultModel::parent(const QModelIndex & index) const {
	
	auto id = index.internalId();
	
	if(iidIsResultEntry(id))
		return createIndex(searchIndexFromIid(id),0,searchIid(id));
	
	return QModelIndex();
}


QVariant SearchResultModel::dataForResultEntry(
	const SearchInfo & search,
	int lineIndex,
	int role
) const {

	if(!search.doc)
		return QVariant();
	
	bool lineIndexValid = (lineIndex >= 0 && lineIndex < search.lines.size() && lineIndex < search.lineNumberHints.size());
	
	switch(role){
	case Qt::CheckStateRole:
	
		if(lineIndexValid)
			return (search.checked.value(lineIndex,true) 
				? Qt::Checked 
				: Qt::Unchecked);
	
		return QVariant();
	case LineNumberRole: {
		
		if(!lineIndexValid)
			return QVariant();
		
		int lineNo = search.doc->indexOf(search.lines[lineIndex], search.lineNumberHints[lineIndex]);
		
		search.lineNumberHints[lineIndex] = lineNo;
		
		if(lineNo < 0)
			return 0;
		
		// internal line number is 0-based

		return lineNo + 1;
	}
	case Qt::DisplayRole:
	case Qt::ToolTipRole: {

		if(!lineIndexValid)
			return "";
		
		search.lineNumberHints[lineIndex] = search.doc 
			-> indexOf(search.lines[lineIndex],search.lineNumberHints[lineIndex]);
		
		if(search.lineNumberHints[lineIndex] < 0)
			return "";
		
		QDocumentLine docline = search.doc
			-> line(search.lineNumberHints[lineIndex]);
		
		if(role == Qt::DisplayRole)
			return docline.text();
			
		// tooltip role
		return prepareReplacedText(docline);

		break;
	}
	case MatchesRole: {

		search.lineNumberHints[lineIndex] = search.doc 
			-> indexOf(search.lines[lineIndex],search.lineNumberHints[lineIndex]);
		
		if(search.lineNumberHints[lineIndex] < 0)
			return QVariant();
		
		auto document = search.doc
			-> line(search.lineNumberHints[lineIndex]);
		
		return QVariant::fromValue<QList<SearchMatch>>(getSearchMatches(document));
	}
	}

	return QVariant();
}


QVariant SearchResultModel::dataForSearchResult(const SearchInfo & search,int role) const {
	
	switch(role){
	case Qt::ToolTipRole:
		return QVariant();
	case Qt::CheckStateRole: {
		
		if(search.checked.isEmpty())
			return QVariant();
		
		bool state = search.checked.first();
		int cnt = search.checked.count(state);

		if(cnt == search.checked.size())
			return state 
				? Qt::Checked 
				: Qt::Unchecked;

		return Qt::PartiallyChecked;
	}
	case Qt::DisplayRole :{

		const auto name = search.doc 
			? QDir::toNativeSeparators(search.doc -> getFileName()) 
			: tr("File closed");

		const auto lines = search
			.lines
			.size();

		return QString("%1 (%2)")
			.arg(name)
			.arg(lines);
	}
	}

	return QVariant();
}


QVariant SearchResultModel::data(const QModelIndex & index,int role) const {

	if(!index.isValid())
		return QVariant();
	
	if(role != Qt::DisplayRole && role != Qt::CheckStateRole && role != Qt::ToolTipRole && role != LineNumberRole && role != MatchesRole)
		return QVariant();
	
	if(role == Qt::CheckStateRole && !mAllowPartialSelection)
		return QVariant();

	const int id = index.internalId();
	int searchIndex = searchIndexFromIid(id);

	if(searchIndex < 0)
		return QVariant();
		
	if(searchIndex >= m_searches.size())
		return QVariant();
	
	const auto & search = m_searches.at(searchIndex);
	
	if(iidIsResultEntry(id))
		return dataForResultEntry(search,index.row(),role);

	return dataForSearchResult(search,role);
}


Qt::ItemFlags SearchResultModel::flags(const QModelIndex & index) const {
	
	if(index.isValid())
		return 
			Qt::ItemIsUserCheckable | 
			Qt::ItemIsSelectable |
			Qt::ItemIsEnabled ;

    return Qt::ItemFlags();
}


bool SearchResultModel::setData(const QModelIndex & index,const QVariant & value,int role){

	if(role != Qt::CheckStateRole)
		return false;
		
	if(!mAllowPartialSelection )
		return false;

	int iid = index.internalId();
	int searchIndex = searchIndexFromIid(iid);

	if(searchIndex < 0)
		return false;
		
	if(searchIndex >= m_searches.size())
		return false;

	auto & search = m_searches[searchIndex];

	if(iidIsResultEntry(iid)){

		if(!search.doc)
			return false;
		
		int lineIndex = index.row();
		
		if(lineIndex < 0)
			return false;
			
		if(lineIndex > search.lines.size())
			return false;
			
		if(lineIndex > search.lineNumberHints.size())
			return false;

		if(role == Qt::CheckStateRole)
			search.checked.replace(lineIndex,(value == Qt::Checked));

		emit dataChanged(index,index);
	} else {

		bool state = (value == Qt::Checked);
		
		for(int l = 0;l < search.checked.size();l++)
			search.checked.replace(l,state);

		int lastRow = search.checked.size() - 1;
		
		auto endIndex = createIndex(lastRow,0,makeResultEntryIid(iid,lastRow));
		
		emit dataChanged(index, endIndex);
	}

	return true;
}


void SearchResultModel::setSearchExpression(
	const QString & expression,
	const QString & replacement,
	const bool isCaseSensitive,
	const bool isWord,
	const bool isRegExp
){
	mIsCaseSensitive = isCaseSensitive;
	mReplacementText = replacement;
	mExpression = expression;
	mIsRegExp = isRegExp;
	mIsWord = isWord;
}


void SearchResultModel::setSearchExpression(
	const QString & expression,
	const bool isCaseSensitive,
	const bool isWord,
	const bool isRegExp
){
	mIsCaseSensitive = isCaseSensitive;
	mExpression = expression;
	mIsRegExp = isRegExp;
	mIsWord = isWord;
}


QString SearchResultModel::prepareReplacedText(const QDocumentLine & docline) const {

	const auto & placements = getSearchMatches(docline);
	auto result = docline.text();
	
	int offset = 0;
	
	for(const auto & match : placements){
		if(mIsRegExp)
			continue;
			
		result = result.left(offset + match.position) +
			"<b>" + mReplacementText + "</b>" + 
			result.mid(match.position + match.length + offset);

		// 7 is the length of the html tags.
		offset += mReplacementText.length() - match.length + 7; 
	}

	return result;
}


QList<SearchMatch> SearchResultModel::getSearchMatches(const QDocumentLine & docline) const {

	if(!docline.isValid())
		return {};//QList<SearchMatch>();
		
	if(mExpression.isEmpty())
		return {};//QList<SearchMatch>();

	const auto regex = generateRegExp(mExpression,mIsCaseSensitive,mIsWord,mIsRegExp);
	QString text = docline.text();

	QList<SearchMatch> result;
	int index = 0;

	while(index < text.length()){

		index = regex.indexIn(text,index);
		
		if(index < 0)
			break;

		SearchMatch match;
		match.position = index;
		match.length = regex.matchedLength();
		
		result << match;
        
        index += match.length;
	}

	return result;
}


QDocument * SearchResultModel::getDocument(const QModelIndex & index){

	const int i = searchIndexFromIid(index.internalId());

	 if(i < 0)
		return nullptr;
		
	 if(i >= m_searches.size())
		return nullptr;
    
     if(!m_searches[i].doc)
        return nullptr;
	
	 return m_searches[i].doc;
}


int SearchResultModel::getLineNumber(const QModelIndex & index){

	const int id = index.internalId();
	
	if(!iidIsResultEntry(id))
		return -1;
	
	const int searchId = searchIndexFromIid(id);
	
	if(searchId)
		return -1;
		
	if(searchId >= m_searches.size())
		return -1;
	
	const auto & search = m_searches.at(searchId);
	
	if(!search.doc)
		return -1;
	
	int line = index.row();
	
	if(line < 0)
		return -1;
		
	if(line > search.lines.size())
		return -1;
		
	if(line > search.lineNumberHints.size())
		return -1;
	
	search.lineNumberHints[line] = search.doc 
		-> indexOf(search.lines[line],search.lineNumberHints[line]);
	
	return search.lineNumberHints[line];
}


QVariant SearchResultModel::headerData(int section,Qt::Orientation orientation,int role) const {
	
	if(role != Qt::DisplayRole)
		return QVariant();
	
	if(orientation != Qt::Horizontal)
		return QVariant();
	
	if(section != 0)
		return QVariant();

	return tr("Results");
}


int SearchResultModel::getNextSearchResultColumn(const QString & text,int column){

	const auto regex = generateRegExp(mExpression,mIsCaseSensitive,mIsWord,mIsRegExp);

	int 
		previous = 0,
		index = 0;

	while(index <= column && index > -1){

		index = regex.indexIn(text,index);
		
		if(index < 0)
			continue;

		previous = index;
		index++;
	}

	return previous;
}


LabelSearchResultModel::LabelSearchResultModel(QObject * parent) 
	: SearchResultModel(parent) {}


/*!
 * This is a workaround:
 * In contrast to LabelSearchQuery::run() which uses the internal label information, this
 * function relies only on textual matching. It's currently only used for highlighting the
 * results. The actual search and replace are handled by the query and have always been correct.
 *
 * To reduce the chance of false highlighting such as "label \\ref{label}", we assume that
 * labels always appear in curly braces and only match those occurences.
 *
 * A clean solution would have to track not only the lines but also the column positions
 * within the lines. Alternatively, the QDocumentLine might be queried for the label positions
 * of the matching type.
 */

QList<SearchMatch> LabelSearchResultModel::getSearchMatches(const QDocumentLine & line) const {

	auto matches = SearchResultModel::getSearchMatches(line);
	const auto & text = line.text();

	std::reverse(matches.begin(),matches.end());

	std::remove_if(matches.begin(),matches.end(),[ & ](const auto & match){

		const auto position = match.position;
		const auto first = position - 1;

		if(first < 0)
			return true;

		const auto open = text.at(first);

		if(open != '{')
			return true;


		const auto length = match.length;
		const auto last = position + length;

		if(last >= text.length())
			return true;

		const auto close = text.at(last);

		if(close != '}')
			return true;

		return false;
	});

//	for(int i = matches.count() - 1;i >= 0;i--){

//		const auto & match = matches.at(i);
		
//		const auto position = match.pos;
//		const auto length = match.length;

//		if(
//			(position + length >= text.length()) ||
//			(position <= 0) ||
//			(text.at(position + length) != '}') ||
//			(text.at(position - 1) != '{')
//		){
//			matches.removeAt(i);
//			continue;
//		}
//	}

	return matches;
}
