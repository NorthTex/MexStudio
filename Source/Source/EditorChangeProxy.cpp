#include "EditorChangeProxy.hpp"


/*! \class EditorChangeProxy
 *
 * This class handles changes of the currentEditor
 *
 * There are two ways that can lead to emission of the signal currentEditorChanged():
 *
 * 1) First, other components can connect to the slot editorChange(). These signals are
 * just propagated, except if the EditorChangeProxy is blocked.
 *
 * 2) Second, one can block() the propagation of changes and release() later on. At release(),
 * the proxy checks if the currentEditor has changed from the one at blocking. If so, the
 * currentEditorChanged() signal is emitted.
 * Use the block/release mechanism to perform complex actions that consist of multiple
 * editor changes (potentially even with inconsistent intermediate states) during which
 * you don't want to emit currentEditorChanged().
 * You have to make sure that you release the block if and only if you acquired it. Therefore,
 * blocking should always use this pattern:
 *
 *     bool b = changes->block();
 *     ...
 *     if (b) changes->release();
 *
 * If you have return statements in the intermediate code, be sure to place the release command
 * also before every return.
 */

EditorChangeProxy::EditorChangeProxy(Editors * editors)
	: currentEditorAtBlock(nullptr)
	, editors(editors)
	, blocked(false) {}

//#define ecpDebug(x) qDebug(x)
#define ecpDebug(x)

bool EditorChangeProxy::block(){

	if(blocked){
		ecpDebug("already blocked");
		return false;
	}

	blocked = true;

	currentEditorAtBlock = editors -> currentEditor();
	listOfEditorsAtBlock = editors -> editors();
	
	ecpDebug("block activated");
	
	return true;
}


void EditorChangeProxy::release(){

	ecpDebug("release");
	
	if(blocked){

		blocked = false;
		
		if(editors -> currentEditor() != currentEditorAtBlock){
			ecpDebug("currentEditorChanged signaled at release");
			emit currentEditorChanged();
		}

		if(editors -> editors() != listOfEditorsAtBlock){
			ecpDebug("listOfEditorsChanged signaled at release");
			emit listOfEditorsChanged();
		}
	} else {
		// can only happen if the above mentioned blocking pattern was not used.
		qDebug("WARNING: trying to realease an unblocked EditorChangeProxy. This hints at inconsistent code.");
	}
}


void EditorChangeProxy::currentEditorChange(){

	if(blocked){
		ecpDebug("currentEditorChanged blocked");
	} else {
		ecpDebug("currentEditorChanged passed");
		emit currentEditorChanged();
	}
}


void EditorChangeProxy::listOfEditorsChange(){
	if(blocked){
		ecpDebug("listOfEditorsChanged blocked");
	} else {
		ecpDebug("listOfEditorsChanged passed");
		emit listOfEditorsChanged();
	}
}

#undef ecpDebug

