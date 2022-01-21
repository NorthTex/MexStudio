/***************************************************************************
 *   copyright       : (C) 2012 by Tim Hoffmann                            *
 *   http://texstudio.sourceforge.net/                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "cursorhistory.h"
#include "qdocumentcursor.h"


CursorHistory::CursorHistory(LatexDocuments * documents) 
	: QObject(documents)
	, m_backAction(nullptr)
	, m_forwardAction(nullptr)
	, m_maxLength(30)
	, m_insertionEnabled(true){

    connect(documents,SIGNAL(aboutToDeleteDocument(LatexDocument *)),this,SLOT(aboutToDeleteDoc(LatexDocument *)));
	currentEntry = history.end();
}


/*!
 *	Inserts the cursor behind the current entry
 */

bool CursorHistory::insertPos(
	QDocumentCursor cursor,
	bool deleteBehindCurrent
){

	if(!m_insertionEnabled) 
		return false;
	
	if(!cursor.isValid())
		return false;

	CursorPosition position(cursor);

	connectUnique(position.doc(),SIGNAL(destroyed(QObject *)),this,SLOT(documentClosed(QObject *)));
	// TODO destroyed() may be duplicate to aboutToDeleteDocument() - needs more testing. anyway it does not harm
    connectUnique(position.doc(),SIGNAL(lineDeleted(QDocumentLineHandle *,int)),this,SLOT(lineDeleted(QDocumentLineHandle *,int)));
	connectUnique(position.doc(),SIGNAL(lineRemoved(QDocumentLineHandle *)),this,SLOT(lineDeleted(QDocumentLineHandle *)));

	if(deleteBehindCurrent && currentEntry != history.end()){
        ++currentEntry;
		currentEntry = history.erase(currentEntry,history.end());
	}

    if(currentEntry == history.end() && currentEntry != history.begin())
		--currentEntry;

	// Do not insert neighboring duplicates

	if(currentEntryValid() && (* currentEntry).equals(position)){
		updateNavActions();
		return false;
	}

	auto iterator = prevValidEntry(currentEntry);
	
	if(
		iterator != history.end() && 
		(* iterator).isValid() && 
		(* iterator).equals(position)
	){
		updateNavActions();
		return false;
	}

    if(history.size() >= m_maxLength)
		if(currentEntry == history.begin())
            history.pop_back();
		else
            history.pop_front();

	if(history.size() > 0){
		++currentEntry;
		history.insert(currentEntry,position);
	} else {
		history.insert(history.begin(),position);
	}

	updateNavActions();

	return true;
}


QDocumentCursor CursorHistory::currentPos(){

	if(!currentEntryValid())
		validate();

	if(!currentEntryValid()){
		qDebug() << "invalid current position in CursorHistory";
		return QDocumentCursor();
	}

	return (* currentEntry).toCursor();
}

void CursorHistory::setInsertionEnabled(bool b){
	m_insertionEnabled = b;
}


/*!
 *	Register an action as backward action.
 *	
 *	This is only used to set the enabled state of 
 *	the action depending on if back is possible.
 *	
 *	It does not actually connect the action to the 
 *	back() slot because it is unspecified if other 
 *	slots connected to the action are executed 
 *	before or after back().
 *	
 *	Therefore the user has to call back() manually, 
 *	or, if it is unabigous, connect the trigger 
 *	signal of the action to the back() slot himself.
 */

void CursorHistory::setBackAction(QAction * back){
	m_backAction = back;
	updateNavActions();
}


/*!
 *	Register an action as forward action.
 *
 *	This is only used to set the enabled state of
 *	the action depending on if forward is possible.
 *	
 *	It does not actually connect the action to the 
 *	forward() slot because it is unspecified if other 
 *	slots connected to the action are executed before 
 *	or after forward().
 *	
 *	Therefore the user has to call forward() manually,
 *	or, if it is unabigous, connect the trigger signal
 *	of the action to the forward() slot himself.
 */

void CursorHistory::setForwardAction(QAction * forward){
	m_forwardAction = forward;
	updateNavActions();
}

void CursorHistory::clear(){

	history.clear();
	
	currentEntry = history.end();
	updateNavActions();
}

QDocumentCursor CursorHistory::back(const QDocumentCursor &currentCursor){

	if(currentEntry == history.begin()){
		updateNavActions();
		return QDocumentCursor();
	}

	// Insert currentCursor to be able to go back

	if(
		currentCursor.isValid() && 
		insertPos(currentCursor,false)
	)	--currentEntry;

	CursorPosition pos(currentCursor);
	
	if(pos.isValid() && !pos.equals(*currentEntry)){
		updateNavActions();
		return currentPos();
	}

	currentEntry = prevValidEntry(currentEntry);
	updateNavActions();

	return currentPos();
}


QDocumentCursor CursorHistory::forward(const QDocumentCursor & currentCursor){

	Q_UNUSED(currentCursor)
	
	CursorPosList::iterator next = nextValidEntry(currentEntry);
	
	if(currentEntry == history.end() || next == history.end()){
		updateNavActions();
		return QDocumentCursor();
	}

	currentEntry = next;
	updateNavActions();
	
	return currentPos();
}


void CursorHistory::aboutToDeleteDoc(LatexDocument * document){

	// Remove all entries with document from list.

	for(auto iterator = history.begin();iterator != history.end();){
		if((* iterator).doc() == document){
			if(currentEntry == iterator)
				currentEntry = nextValidEntry(currentEntry);
			
			removeEntry(iterator);
		} else {
			++iterator;
		}
	}

	updateNavActions();
}


void CursorHistory::documentClosed(QObject * object){

	auto document = qobject_cast<LatexDocument *>(object);

	if(document)
		aboutToDeleteDoc(document);
}


void CursorHistory::lineDeleted(QDocumentLineHandle * lineHandle,int){

	for(auto iterator = history.begin();iterator != history.end();++iterator){
		
		if((* iterator).dlh() != lineHandle)
			continue;

		if(currentEntry == iterator)
			currentEntry = nextValidEntry(currentEntry);

		removeEntry(iterator);
	}

	updateNavActions();
}

void CursorHistory::updateNavActions(){

	if(m_backAction)
		m_backAction -> setEnabled(currentEntry != history.begin());

	if(m_forwardAction)
		m_forwardAction -> setEnabled(nextValidEntry(currentEntry) != history.end());
}

void CursorHistory::removeEntry(CursorPosList::iterator & iterator){

	Q_ASSERT(iterator != history.end());
	
	if(currentEntry == iterator)
		currentEntry = nextValidEntry(currentEntry);

	Q_ASSERT(currentEntry != iterator);

	iterator = history.erase(iterator);
}


bool CursorHistory::currentEntryValid(){
	
	if(currentEntry == history.end())
		return false;

	return (* currentEntry).isValid();
}


/*!
 *	Removes all invalid entries from history
 */

void CursorHistory::validate(){

	auto iterator = history.begin();
	
	while(iterator != history.end()){

		if(!(* iterator).isValid()){
            
			if(iterator == currentEntry)
				++currentEntry;
			
			qDebug() << "removed invalid cursorHistory entry" << (* iterator).doc() -> getFileName();
			Q_ASSERT(currentEntry != iterator);

			iterator = history.erase(iterator);
		} else {
			++iterator;
		}
	}
}


CursorPosList::iterator CursorHistory::prevValidEntry(const CursorPosList::iterator & start){

	auto iterator = start;
	
	while(true){
		
		if(iterator == history.begin())
			return iterator;
		
		--iterator;

		if((* iterator).isValid())
			return iterator;

		bool moveCurrent = (currentEntry == iterator);

		iterator = history.erase(iterator);
		
		if(moveCurrent)
			currentEntry = iterator;
	}

	Q_ASSERT(0);
	
	return history.end(); // never reached
}


CursorPosList::iterator CursorHistory::nextValidEntry(const CursorPosList::iterator & start){

	auto iterator = start;

	if(iterator == history.end())
		return iterator;
	
	++iterator;
	
	while(true){

		if(iterator == history.end())
			return iterator;
		
		if((* iterator).isValid())
			return iterator;

		bool moveCurrent = (currentEntry == iterator);

		iterator = history.erase(iterator);
		
		if(moveCurrent)
			currentEntry = iterator;
	}

	Q_ASSERT(0);

	return history.end(); // never reached
}


void CursorHistory::debugPrint(){

	qDebug() << "*** Cursor History ***";
	
	auto iterator = history.begin();
	
	while(iterator != history.end()){

		auto position = * iterator;
		
		qDebug() 
			<< ((iterator == currentEntry) ? "*" : " ") 
			<< position.doc() -> getFileName() 
			<< position.oldLineNumber() 
			<< "col:" 
			<< position.columnNumber();
        
		++iterator;
	}
	
	qDebug() 
		<< ((iterator == currentEntry) ? "*" : " ") 
		<< "end";
}
