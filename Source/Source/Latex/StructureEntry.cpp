
#include "Latex/Structure.hpp"
#include "Latex/Document.hpp"


StructureEntry::StructureEntry(LatexDocument * document,Type newType)
	: type(newType)
	, level(0)
	, valid(false)
	, expanded(false)
	, parent(nullptr)
	, document(document)
	, columnNumber(0)
	, parentRow(-1)
	, lineHandle(nullptr)
	, lineNumber(-1)
	, m_contexts(Unknown) {

	#ifndef QT_NO_DEBUG
		Q_ASSERT(document);
		document -> StructureContent.insert(this);
	#endif
}


StructureEntry::~StructureEntry(){

	level = -1; // invalidate entry
	
	for(auto child : children)
		delete child;

	#ifndef QT_NO_DEBUG
		Q_ASSERT(document);
		bool removed = document -> StructureContent.remove(this);

		// prevent double deletion

		Q_ASSERT(removed);
	#endif
}


void StructureEntry::add(StructureEntry * entry){
    
	Q_ASSERT(entry != nullptr);

	children.append(entry);
	entry -> parent = this;
}


void StructureEntry::insert(int index,StructureEntry * entry){
    
	Q_ASSERT(entry != nullptr);
	
	children.insert(index,entry);
	entry -> parent = this;
}


void StructureEntry::setLine(QDocumentLineHandle * handle,int line){
	lineHandle = handle;
	lineNumber = line;
}


QDocumentLineHandle *StructureEntry::getLineHandle() const {
	return lineHandle;
}


int StructureEntry::getCachedLineNumber() const {
	return lineNumber;
}


int StructureEntry::getRealLineNumber() const {
	
	lineNumber = document -> indexOf(lineHandle,lineNumber);

	Q_ASSERT(lineNumber == -1 || document -> line(lineNumber).handle() == lineHandle);
	
	return lineNumber;
}


template <typename Type>

inline int hintedIndexOf(const QList<Type *> & list,const Type * elem,int hint){

	if(hint < 2)
		return list.indexOf(const_cast<Type *>(elem));

	int backward = hint, forward = hint + 1;
	
	for(;backward >= 0 && forward < list.size();backward--,forward++){
		
		if(list[backward] == elem)
			return backward;
		
		if(list[forward] == elem)
			return forward;
	}

	if(backward >= list.size())
		backward = list.size() - 1;

	for(;backward >= 0;backward--)
		if(list[backward] == elem)
			return backward;
	
	if(forward < 0)
		forward = 0;
	
	for(;forward < list.size();forward++)
		if(list[forward] == elem)
			return forward;

	return -1;
}


int StructureEntry::getRealParentRow() const {

	REQUIRE_RET(parent,-1);
	
	parentRow = hintedIndexOf<StructureEntry>(parent -> children,this,parentRow);
	return parentRow;
}


void StructureEntry::debugPrint(const char * message) const {
	qDebug("%s %p",message,this);
    qDebug() << "   level: " << level;
    qDebug() << "   type: " << static_cast<int>(type);
    qDebug() << "   line nr: " << lineNumber;
    qDebug() << "   title: " << title;
}


