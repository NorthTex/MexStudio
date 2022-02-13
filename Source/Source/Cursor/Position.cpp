
#include "cursorposition.h"
#include "qdocument.h"
#include "qdocumentline.h"
#include "qdocumentcursor.h"


QDocumentLineTrackedHandle::QDocumentLineTrackedHandle(const QDocumentCursor & cursor)
	: m_doc(nullptr)
	, m_dlh(nullptr)
	, m_oldLineNumber(0) {

	if(cursor.isValid()){
		m_doc = qobject_cast<QDocument *>(cursor.document());
		m_dlh = cursor.line().handle();
		m_oldLineNumber = cursor.lineNumber();
	}
}


int QDocumentLineTrackedHandle::lineNumber() const {
	m_oldLineNumber = m_doc -> indexOf(m_dlh);
	return m_oldLineNumber;
}


bool QDocumentLineTrackedHandle::isValid() const {
	
	if(!m_doc)
		return false;
	
	m_oldLineNumber = m_doc 
		-> indexOf(m_dlh,m_oldLineNumber);

	return m_oldLineNumber >= 0;
}


CursorPosition::CursorPosition(const QDocumentCursor & cursor)
	: QDocumentLineTrackedHandle(cursor)
	, m_columnNumber(0) {
	
	if(m_dlh)
		m_columnNumber = cursor.columnNumber();
}


QDocumentCursor CursorPosition::toCursor(){

	if(!m_doc)
		return QDocumentCursor();
	
	// update line number
	
	m_oldLineNumber = m_doc 
		-> indexOf(m_dlh,m_oldLineNumber);
	
	return QDocumentCursor(m_doc,m_oldLineNumber,m_columnNumber);
}


bool CursorPosition::equals(const CursorPosition & position) const {
	return m_columnNumber == position.m_columnNumber &&
		m_doc == position.m_doc && 
		m_dlh == position.m_dlh ;
}
