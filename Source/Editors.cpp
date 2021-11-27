#include "editors.h"
#include "txstabwidget.h"
#include "mostQtHeaders.h"
#include "utilsSystem.h"
#include "latexeditorview.h"
#include "minisplitter.h"
#include <algorithm>

using std::reverse;



/*!
 * This class manages all editors and their positioning in tabWidgets.
 *
 * It maintains the knowledge of the currentEditor() and the currentTabWidget().
 * Also, it sends appropriate signals to the editors and tabWidgets and provides
 * signals to hook to these changes.
 *
 * Currently this class serves two purposes:
 *
 * 1) It maintains an abstract order of editors. Editors can be grouped (currently
 *    implemented as tabs in tabWidgets). The groups are ordered and the editors
 *    within a group are ordered, thus providing a complete order of all editors.
 *    The main purpose of this class it to provide an interface to this order, which
 *    can be used in the main program without having to know details on the actual
 *    organization of groups.
 *
 * 2) It contains all tabWidgets and thus editors providing a simple horizontal or
 *    vertical layout of the tab groups (there are currently 2 groups).
 *    This purpose is secondary and mainly implemented for an easier transition from
 *    the single tabGroup to more complex editor layouts. It may be changed in the
 *    future and will probably move to a separate class. The public interface of the
 *    class should depend on this as little as possible.
 */

Editors::Editors(QWidget *parent) 
	: QWidget(parent)
	, splitter(nullptr)
	, currentGroupIndex(-1) {

	splitter = new MiniSplitter(Qt::Horizontal);
	splitter -> setChildrenCollapsible(false);

	auto layout = new QVBoxLayout(this);
	layout -> setContentsMargins(0,0,0,0);
	layout -> addWidget(splitter);

	changes = new EditorChangeProxy(this);

	connect(changes,SIGNAL(currentEditorChanged()),this,SIGNAL(currentEditorChanged()));
	connect(changes,SIGNAL(listOfEditorsChanged()),this,SIGNAL(listOfEditorsChanged()));
	connect(this,SIGNAL(editorsReordered()),this,SIGNAL(listOfEditorsChanged()));
}


/*!
 * Adds the given tabWidget. This take ownership of the tabWidget.
 */

void Editors::addTabWidget(TxsTabWidget * widget){

	splitter -> addWidget(widget);
	tabGroups.append(widget);
	
	if(currentGroupIndex < 0){
		currentGroupIndex = 0;
		widget -> setActive(true);
	} else {
		if(widget -> isEmpty())
			widget->hide();
	}

	connect(widget,SIGNAL(currentEditorChanged()),changes,SLOT(currentEditorChange()));
    connect(widget,SIGNAL(editorAboutToChangeByTabClick(LatexEditorView *,LatexEditorView *)),this,SLOT(onEditorChangeByTabClick(LatexEditorView *,LatexEditorView *)));
	connect(widget,SIGNAL(activationRequested()),SLOT(activateTabWidgetFromSender()));
    connect(widget,SIGNAL(closeEditorRequested(LatexEditorView *)),SLOT(requestCloseEditor(LatexEditorView *)));
	connect(widget,SIGNAL(tabBarContextMenuRequested(QPoint)),SLOT(tabBarContextMenu(QPoint)));
    connect(widget,SIGNAL(tabMoved(int,int)),SIGNAL(editorsReordered()));
}


void Editors::addEditor(LatexEditorView * view,bool asCurrent){
	insertEditor(view,currentTabWidget(),-1,asCurrent);
}


void Editors::insertEditor(LatexEditorView * view,int index,bool asCurrent){

	if(index >= 0)
		for(const auto group : tabGroups){
			if(group -> count() > index){
				insertEditor(view,group,index,asCurrent);
				return;
			}

			index -= group -> count();
		}

	//	Append view to last empty editor

	// auto groups(tabGroups);
	// reverse(groups.begin(),groups.end());
	// groups.pop_back();

	// for(const auto group : groups){

	// 	if(group -> isEmpty())
	// 		continue;

	// 	insertEditor(view,group,-1,asCurrent);
	// }

	for(int group = tabGroups.length() - 1;group >= 0;group--)
		if(!tabGroups[group] -> isEmpty() || group == 0){
			insertEditor(view,tabGroups[group],-1,asCurrent);
			break;
		}
}

void Editors::moveEditor(LatexEditorView * view,Editors::Position pos){

	switch(pos){
		case AbsoluteFront:
			moveToTabGroup(view, tabGroups.first(),0);
			break;
		case AbsoluteEnd: {
			
			int lastNonHiddenIndex = tabGroups.size() - 1;
			
			while(lastNonHiddenIndex > 0 && tabGroups[lastNonHiddenIndex] -> isHidden())
				lastNonHiddenIndex--;
			
			if(lastNonHiddenIndex < 0)
				return;
			
			moveToTabGroup(view,tabGroups[lastNonHiddenIndex],-1);
			
			break;
		}
		case GroupFront: {

			auto widget = tabWidgetFromEditor(view);
			moveToTabGroup(view,widget,0);
			break;
		}
		case GroupEnd: {
			
			auto widget = tabWidgetFromEditor(view);
			moveToTabGroup(view,widget,-1);
			break;
		}
	}
}


/*!
 * Insert the editor in the given tabWidget at pos.
 * If tabWidget is 0, insert into the current tabWidget.
 * If pos == -1, append.
 * If asCurrent, make the editor the current editor.
 */

void Editors::insertEditor(LatexEditorView * view, TxsTabWidget * widget,int pos,bool asCurrent){

	bool b = changes -> block();

	if(!widget)
		return;
	
	if(!widget -> isVisible()){

		widget -> setVisible(true);
		
		// distribute available space
		QList<int> sizes;

		for(int i = 0;i < splitter -> count();i++)
			sizes << 10; // excess space is distributed equally

		splitter -> setSizes(sizes);
	}

	widget -> insertEditor(view,pos,asCurrent);

	connect(view,SIGNAL(focusReceived()),this,SLOT(setCurrentEditorFromSender()));

	if(asCurrent)
		setCurrentEditor(view);

	if(b)
		changes -> release();
}


/*! \brief request to close the given editor.
 *
 * The widget cannot decide if the tab can really be closed, which can only be
 * determined at top level with user interaction if there are unsaved changes to
 * the editor. So we propagate the request up.
 */

void Editors::requestCloseEditor(LatexEditorView * view){

	auto originalCurrent = currentEditor();

	if(view == originalCurrent){
		// request to close the current editor
		emit closeCurrentEditorRequested();
	} else {

		// request to close a non-current editor
		setCurrentEditor(view);
		emit closeCurrentEditorRequested();

			// closing (of the originally non-current) editor was successful.
			// Therefore we activate the originally current editor again.
		if(currentEditor() != view)
			setCurrentEditor(originalCurrent);
	}
}


/*!
 * Remove the editor from the list of editors
 */

void Editors::removeEditor(LatexEditorView * view){
	disconnect(view,SIGNAL(focusReceived()),this,SLOT(setCurrentEditorFromSender()));
	removeEditor(view,tabWidgetFromEditor(view));
}


/*!
 * Remove the editor from the given tabWidget and hides the widget if its empty and not the only one.
 */

void Editors::removeEditor(LatexEditorView * view,TxsTabWidget * widget){

	if(!widget)
		return;
	
	bool b = changes -> block();
	bool isLastEditorInGroup = (widget -> indexOf(view)) == (widget -> count() - 1);

	if(view == currentEditor()){
		if (isLastEditorInGroup)
			activatePreviousEditor();
		else
			activateNextEditor();
	}

	widget -> removeEditor(view);
	
	if(widget -> isEmpty() && widget != tabGroups[0])
		widget -> hide();

	if(b)
		changes -> release();
}


bool Editors::containsEditor(LatexEditorView * view) const {
	return tabGroupIndexFromEditor(view) >= 0;
}


TxsTabWidget * Editors::currentTabWidget() const {
	return tabGroups.value(currentGroupIndex,nullptr);
}


LatexEditorView * Editors::currentEditor() const {

	auto tabs = currentTabWidget();
    
    if(tabs)
        return qobject_cast<LatexEditorView *>(tabs -> currentWidget());

    return nullptr;
}


/*!
 * Sets the current editor. It is expected that (containsEditor(edView) == true).
 * if setFocus, the focus will be changed to the new editor
 */

void Editors::setCurrentEditor(LatexEditorView * view,bool setFocus){

	if(currentEditor() == view)
		return;

	int group = tabGroupIndexFromEditor(view);

	if(group >= 0){
		
		bool b = changes->block();
		
		setCurrentGroup(group);
		
		tabGroups[group] -> setCurrentEditor(view);
		
		if(setFocus)
			view -> setFocus();
		
		if(b)
			changes -> release();
		
		return;
	}


	// catch calls in which editor is not a member tab.
	// TODO: such calls are deprecated as bad practice. We should avoid them in the long run. For the moment the fallback to do nothing is ok.
	qDebug() << "Warning (deprecated call): TxsTabWidget::setCurrentEditor: editor not member of TxsTabWidget";

	#ifndef QT_NO_DEBUG
		UtilsUi::txsWarning("Warning (deprecated call): TxsTabWidget::setCurrentEditor: editor not member of TxsTabWidget");
	#endif
}


QList<LatexEditorView *> Editors::editors(){

	QList<LatexEditorView *> editors;
	
	for(auto group : tabGroups)
		editors.append(group -> editors());

	return editors;
}


void Editors::setCurrentEditorFromAction(){

	auto action = qobject_cast<QAction *>(sender());
	
	REQUIRE(action);
	
	setCurrentEditor(action -> data().value<LatexEditorView *>());
}


void Editors::setCurrentEditorFromSender(){

	auto view = qobject_cast<LatexEditorView *>(sender());
	
	REQUIRE(view);
	
	setCurrentEditor(view);
}


void Editors::closeEditorFromAction(){

	auto action = qobject_cast<QAction *>(sender());
	
	REQUIRE(action);
	
	auto view = action -> data().value<LatexEditorView *>();
	
	requestCloseEditor(view);
}


void Editors::closeOtherEditorsFromAction(){

	auto action = qobject_cast<QAction *>(sender());
	
	REQUIRE(action);
	
	auto editorToKeep = action -> data().value<LatexEditorView *>();

	for(auto group : tabGroups)
		for(auto editor : group -> editors())
			if(editor != editorToKeep)
				requestCloseEditor(editor);
}


void Editors::toggleReadOnlyFromAction(){

	auto action = qobject_cast<QAction *>(sender());
	
	REQUIRE(action);
	
	auto view = action -> data().value<LatexEditorView *>();
	
	view -> editor -> setReadOnly(!view -> editor -> isReadOnly());
}


bool Editors::activateNextEditor(){

	if(currentGroupIndex < 0)
		return false;

	bool b = changes -> block();
	
	if(tabGroups[currentGroupIndex] -> currentEditorViewIsLast()){
		// find the next non-empty group
		int newIndex = currentGroupIndex;
		
		for(int unused = 0;unused < tabGroups.length();unused++){ // counter is only used as a stopping criterion
			
			newIndex = (newIndex + 1) % tabGroups.length();
			
			if(!tabGroups[newIndex] -> isEmpty()){

				if(newIndex == currentGroupIndex && tabGroups[currentGroupIndex] -> count() == 1){
					
					if(b)
						changes->release();
					
					return false;  // only one editor: we cannot change
				}

				setCurrentGroup(newIndex);
				break;
			}
		}
		tabGroups[currentGroupIndex] -> gotoFirstDocument();
	} else {
		tabGroups[currentGroupIndex] -> gotoNextDocument();
	}


	if(b)
		changes -> release();

	return true;
}


bool Editors::activatePreviousEditor(){

	if(currentGroupIndex < 0)
		return false;

	bool b = changes->block();

	if(tabGroups[currentGroupIndex] -> currentEditorViewIsFirst()){
		// find the previous non-empty group
		int newIndex = currentGroupIndex;
		
		for(int unused = 0; unused < tabGroups.length(); unused++) { // counter is only used as a stopping criterion
			
			newIndex = (newIndex == 0) ? (tabGroups.length() -1) : (newIndex - 1);
			
			if(!tabGroups[newIndex] -> isEmpty()){
				if(newIndex == currentGroupIndex && tabGroups[currentGroupIndex]->count() == 1)
					return false;  // only one editor: we cannot change
				setCurrentGroup(newIndex);
				break;
			}
		}
		tabGroups[currentGroupIndex] -> gotoLastDocument();
	} else {
		tabGroups[currentGroupIndex] -> gotoPrevDocument();
	}

	if(b)
		changes -> release();

	return true;
}


/*!
 * Activates the tabWidget that is the sender(). Returns true if the widget could be activated.
 */

bool Editors::activateTabWidgetFromSender(){

	auto widget = qobject_cast<TxsTabWidget *>(sender());
	
	if(!widget)
		return false;
	
	auto view = widget -> currentEditor();

	if(!view)
		return false;
	
	setCurrentEditor(widget -> currentEditor());
	
	return true;
}


/*!
 * Context menu handler for tabbars.
 */

void Editors::tabBarContextMenu(const QPoint & point){

	if(point.isNull())
		return;
	
	auto group = qobject_cast<TxsTabWidget *>(sender());
	
	if(!group)
		return;

	QMenu menu;
	
	for(auto view : editors()){

		auto action = menu.addAction(view -> displayNameForUI());

		action -> setData(QVariant::fromValue<LatexEditorView *>(view));
		connect(action,SIGNAL(triggered()),SLOT(setCurrentEditorFromAction()));
	}

	menu.addSeparator();

	auto action = menu.addAction(tr("Move to other view"));
	auto editorUnderCursor = group -> editorAt(point);

	action -> setData(QVariant::fromValue<LatexEditorView *>(editorUnderCursor));

	if(!editorUnderCursor)
		action -> setEnabled(false);
	
	connect(action,SIGNAL(triggered()),SLOT(moveToOtherTabGroup()));

	action = menu.addAction(tr("Move all to other view"));
	action -> setData(QVariant::fromValue<TxsTabWidget *>(group));

	if(!editorUnderCursor)
		action -> setEnabled(false);
	
	connect(action,SIGNAL(triggered()),SLOT(moveAllToOtherTabGroup()));

	action = menu.addAction((splitter -> orientation() == Qt::Horizontal) ? tr("Split Vertically") : tr("Split Horizontally"));
	
	connect(action,SIGNAL(triggered()),SLOT(changeSplitOrientation()));

	if(editorUnderCursor){

		menu.addSeparator();
		
		QString text = tr("Set Read-Only");
		
		if(editorUnderCursor -> editor -> isReadOnly())
			text = tr("Unset Read-Only");
		
		action = menu.addAction(text);
		action -> setData(QVariant::fromValue<LatexEditorView *>(editorUnderCursor));
		
		connect(action, SIGNAL(triggered()),SLOT(toggleReadOnlyFromAction()));
		
		menu.addSeparator();
		
		action = menu.addAction(tr("Close"));
		action -> setData(QVariant::fromValue<LatexEditorView *>(editorUnderCursor));
		
		connect(action,SIGNAL(triggered()),SLOT(closeEditorFromAction()));
		
		action = menu.addAction(tr("Close All Other Documents"));
		action -> setData(QVariant::fromValue<LatexEditorView *>(editorUnderCursor));
		
		connect(action,SIGNAL(triggered()),SLOT(closeOtherEditorsFromAction()));
	}

	menu.exec(group -> mapToGlobal(point));
}


void Editors::onEditorChangeByTabClick(LatexEditorView * from,LatexEditorView * to){
	
	Q_UNUSED(from)
	
	// the original event comes from a tab widget. from is the previously selected tab in that widget
	// which has not been the current one one if the tab widget has not been the current
	emit editorAboutToChangeByTabClick(currentEditor(),to);
}


/*!
 * Called from and action with data() set to an editor. Moves the editor to the other tabGroup.
 * This currently assumes a restriction to two tab groups.
 */

void Editors::moveToOtherTabGroup(){

	auto action = qobject_cast<QAction *>(sender());
	
	REQUIRE(action);

	auto view = action -> data().value<LatexEditorView *>();

	if(!view)
		return;

	// NOTE: This code assumes exactly two tabGroups
	int otherGroupIndex = (tabGroups[0] == tabWidgetFromEditor(view)) ? 1 : 0;
	
	moveToTabGroup(view,tabGroups[otherGroupIndex],-1);
}


void Editors::moveAllToOtherTabGroup(){

	auto action = qobject_cast<QAction *>(sender());
	
	REQUIRE(action);
	
	auto group = action -> data().value<TxsTabWidget *>();
	
	if(!group)
		return;

	// NOTE: This code assumes exactly two tabGroups
	int otherGroupIndex = (tabGroups[0] == group) ? 1 : 0;
	
	for(auto view : group -> editors())
		moveToTabGroup(view,tabGroups[otherGroupIndex],-1);
}


/*!
 * Move the editor to the given tabWidget at position targetIndex (if < 0, append).
 */

void Editors::moveToTabGroup(LatexEditorView * view,int groupIndex,int targetIndex){
	
	if(groupIndex < 0)
		groupIndex = 0;
	
	if(groupIndex > tabGroups.length() -1)
		groupIndex = tabGroups.length() -1;
	
	moveToTabGroup(view,tabGroups[groupIndex],targetIndex);
}


/*!
 * Move the editor to the given tabWidget at position targetIndex (if < 0, append).
 */

void Editors::moveToTabGroup(LatexEditorView * view,TxsTabWidget * target,int targetIndex){
	
	if(!target || target -> containsEditor(view)){

		// only move within the tab
        if(target == nullptr)
            target = tabWidgetFromEditor(view);
        
		if(target == nullptr){
            // the tab is REALLY not there, see crash in issue #899
            insertEditor(view,targetIndex);
            return;
        }

		if(targetIndex < 0)
			targetIndex = qMax(0,target -> count() - 1);

		int currentIndex = target -> indexOf(view);
		
		if(currentIndex != targetIndex){  // nothing todo otherwise
			target -> moveTab(target -> indexOf(view),targetIndex);
			emit editorsReordered();
		}
	} else {
		bool b = changes -> block();

		auto source = tabWidgetFromEditor(view);
		
		removeEditor(view,source);
		insertEditor(view,target,targetIndex,true);
		
		if(b)
			changes->release();
	}
}


void Editors::changeSplitOrientation(){

	auto orientation = (splitter -> orientation() == Qt::Horizontal)
		? Qt::Vertical
		: Qt::Horizontal;

	splitter -> setOrientation(orientation);
}


/*!
 * Returns the number of the tabGroup to which the povided editor belongs or -1.
 */

int Editors::tabGroupIndexFromEditor(LatexEditorView * view) const {

	for(int group = 0;group < tabGroups.length();group++){

		int i = tabGroups[group] -> indexOf(view);

		if(i >= 0)
			return group;
	}

	return -1;
}


/*!
 * Returns a pointer to the tabWidget to which the provided editor belongs or 0.
 */

TxsTabWidget * Editors::tabWidgetFromEditor(LatexEditorView * view) const {
	
	int index = tabGroupIndexFromEditor(view);
    
	return (index >= 0)
		? tabGroups[index]
		: nullptr;
}


void Editors::setCurrentGroup(int index){

	if(index == currentGroupIndex)
		return;
	
	if(currentGroupIndex >= 0)
		tabGroups[currentGroupIndex] -> setActive(false);
	
	currentGroupIndex = index;
	
	tabGroups[currentGroupIndex] -> setActive(true);
}



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

