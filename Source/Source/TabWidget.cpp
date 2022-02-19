#include "TxsTabWidget.hpp"
#include "ChangeAwareTabBar.hpp"
#include "smallUsefulFunctions.h"

#include "Latex/Document.hpp"
#include "Latex/EditorView.hpp"


TxsTabWidget::TxsTabWidget(QWidget * parent) 
	: QTabWidget(parent)
	, m_active(false) {

	setFocusPolicy(Qt::ClickFocus);
	setContextMenuPolicy(Qt::PreventContextMenu);

	auto tb = new ChangeAwareTabBar();
	tb -> setContextMenuPolicy(Qt::CustomContextMenu);
	tb -> setUsesScrollButtons(true);
	
	connect(tb,SIGNAL(customContextMenuRequested(QPoint)),this,SIGNAL(tabBarContextMenuRequested(QPoint)));
    connect(tb,SIGNAL(currentTabAboutToChange(int,int)),this,SLOT(currentTabAboutToChange(int,int)));
	connect(tb,SIGNAL(tabLeftClicked()),this,SIGNAL(activationRequested()));
	connect(tb,SIGNAL(middleMouseButtonPressed(int)),this,SLOT(onTabCloseRequest(int)));
	setTabBar(tb);

    setDocumentMode(true);

    const auto tb2 = tabBar();
    connect(tb2,SIGNAL(tabMoved(int,int)),this,SIGNAL(tabMoved(int,int)));
    connect(this,SIGNAL(tabCloseRequested(int)),this,SLOT(onTabCloseRequest(int)));
    
	setProperty("tabsClosable",true);
    setProperty("movable",true);

	connect(this,SIGNAL(currentChanged(int)),this,SIGNAL(currentEditorChanged()));
}


void TxsTabWidget::moveTab(int from,int to){

	int cur = currentIndex();
	auto text = tabText(from);
	auto tab = widget(from);
	
	removeTab(from);
	insertTab(to,tab,text);
	
	if(cur == from){
		setCurrentIndex(to);
		return;
	}

	if(from < to && cur >= from && cur < to){
		setCurrentIndex(cur - 1);
		return;
	}

	if(to < from && to && cur >= to && cur < from)
		setCurrentIndex(cur + 1);
}


QList<LatexEditorView *> TxsTabWidget::editors() const {

	QList<LatexEditorView *> list;
	
	for(int i = 0;i < count();i++){
		auto view = qobject_cast<LatexEditorView *>(widget(i));
		Q_ASSERT(view); // there should only be editors as tabs
		list.append(view);
	}

	return list;
}


bool TxsTabWidget::containsEditor(LatexEditorView * view) const {
	
	if(view)
		return indexOf(view) >= 0;

	return false;
}


LatexEditorView * TxsTabWidget::currentEditor() const {
	return qobject_cast<LatexEditorView *>(currentWidget());
}



const auto legacy_editor =
	"Warning (deprecated call): "
	"TxsTabWidget::setCurrentEditor: "
	"editor not member of TxsTabWidget";

void TxsTabWidget::setCurrentEditor(LatexEditorView * view){
	
	if(currentWidget() == view)
		return;

	// catch calls in which editor is not a member tab.
	// TODO: such calls are deprecated as bad practice. We should avoid them in the long run. For the moment the fallback to do nothing is ok.

	if(indexOf(view) < 0){
		
		qDebug() 
			<< legacy_editor
			<< view;
	
		#ifndef QT_NO_DEBUG
			UtilsUi::txsWarning(legacy_editor);
		#endif
		
		return;
	}

	setCurrentWidget(view);
}


LatexEditorView * TxsTabWidget::editorAt(QPoint point){

	int index = tabBar() -> tabAt(point);
    
	if(index < 0)
		return nullptr;
	
	return qobject_cast<LatexEditorView *>(widget(index));
}


/*! \brief Mark the widget as active.
 *
 * If there are multiple widgets, we want to visually indicate the active one,
 * i.e. the one containing the current editor.
 * We currently use bold on the current tab, but due to a Qt bug we're required
 * to increase the tab width all tabs in the active widget.
 * see https://bugreports.qt.io/browse/QTBUG-6905
 */

void TxsTabWidget::setActive(bool active){

	if(active == m_active)
		return;
	
	m_active = active;

	QString baseStyle = "QTabBar::close-button {image: url(:/images-ng/close-tab.svgz)} QTabBar::close-button:hover {image: url(:/images-ng/close-tab-hover.svgz)}";
	
	if(active)
		setStyleSheet(baseStyle);
	else
		setStyleSheet(baseStyle + " QTabBar {color: darkgrey;}");
}


bool TxsTabWidget::isEmpty() const {
	return (count() == 0);
}


/*!
 * \brief check if active tab is the left most
 * \return
 */

bool TxsTabWidget::currentEditorViewIsFirst() const {
	return (currentIndex() == 0);
}


/*!
 * \brief check if active tab is the right most
 * \return
 */

bool TxsTabWidget::currentEditorViewIsLast() const {
	return (currentIndex() >= count() - 1);
}


/*!
 * \brief activate tab to the right
 *
 * right most tab remains active, no roll over.
 */

void TxsTabWidget::gotoNextDocument(){

	if(count() <= 1)
		return;
	
	int cPage = currentIndex() + 1;
	
	setCurrentIndex((cPage >= count()) ? 0 : cPage);
}


/*!
 * \brief activate tab to the left
 *
 * Left most tab (0) remains active, no roll over.
 */

void TxsTabWidget::gotoPrevDocument(){

	if(count() <= 1)
		return;
	
	int cPage = currentIndex() - 1;
	
	setCurrentIndex((cPage < 0) ? (count() - 1) : cPage);
}


/*!
 * \brief activate first document/tab
 */

void TxsTabWidget::gotoFirstDocument(){
	
	if(count() <= 1)
		return;
	
	setCurrentIndex(0);
}


/*!
 * \brief activate last document/tab
 */


void TxsTabWidget::gotoLastDocument(){
	
	if(count() <= 1)
		return;
	
	setCurrentIndex(count() - 1);
}


void TxsTabWidget::currentTabAboutToChange(int from,int to){

	auto edFrom = qobject_cast<LatexEditorView *>(widget(from));
	auto edTo = qobject_cast<LatexEditorView *>(widget(to));
	
	REQUIRE(edFrom);
	REQUIRE(edTo);
	
	emit editorAboutToChangeByTabClick(edFrom,edTo);
}


/*! \brief Handler for close requests coming from the tab bar.
 *
 * The widget cannot decide if the tab can really be closed, which can only be
 * determined at top level with user interaction if there are unsaved changes to
 * the editor. So we propagate the request up.
 * In the long terms one might consider asking the editor instead.
 */

void TxsTabWidget::onTabCloseRequest(int i){
	emit closeEditorRequested(editorAt(i));
}


/*!
 * \brief insert tab with given editor
 * \param edView editor
 * \param pos position, 0 is left most
 * \param asCurrent put editor as active into foreground
 */

void TxsTabWidget::insertEditor(LatexEditorView * view,int pos,bool asCurrent){

	Q_ASSERT(view);

	pos = insertTab(pos,view,"?bug?");
	
	updateTab(pos);
	connectEditor(view);

	if(asCurrent)
		setCurrentEditor(view);
}


/*!
 * \brief remove tab which is connected to the given editor
 * \param edView
 */

void TxsTabWidget::removeEditor(LatexEditorView * view){

	int i = indexOf(view);
	
	if(i >= 0)
		removeTab(i);
	
	disconnectEditor(view);
}


/*!
 * Returns the LatexEditorView at the given tab index or 0.
 */

LatexEditorView * TxsTabWidget::editorAt(int index){

	if(index < 0)
		return nullptr;
		
	if(index >= count())
        return nullptr;
	
	return qobject_cast<LatexEditorView *>(widget(index));
}


/*!
 * Connect to signals of the editor so that the TabWidget will update the modified
 * status and the tab text when these properties of the editor change.
 *
 */

void TxsTabWidget::connectEditor(LatexEditorView * view){

	connect(view -> editor,SIGNAL(contentModified(bool)),this,SLOT(updateTabFromSender()));
	connect(view -> editor,SIGNAL(readOnlyChanged(bool)),this,SLOT(updateTabFromSender()));
	connect(view -> editor,SIGNAL(titleChanged(QString)),this,SLOT(updateTabFromSender()));
}


/*!
 * Disconnect all connections from the editor to this.
 */

void TxsTabWidget::disconnectEditor(LatexEditorView * view){
	view -> editor -> disconnect(this);
}


/*!
 * Updates modified icon, tab text and tooltip.
 * TODO: tooltip should be dynamically generated when required because
 * this is not called always when root information changes.
 */

void TxsTabWidget::updateTab(int index){

	//cache icons, getRealIcon is *really* slow
	static QIcon readOnly = getRealIcon("document-locked");
	static QIcon modified = getRealIcon("modified");
	static QIcon empty = QIcon(":/images/empty.png");

	auto view = editorAt(index);
	
	if(!view)
		return;

	const auto editor = view -> editor;

	// update icon

	setTabIcon(index,
		(editor -> isReadOnly())
			? readOnly :
		(editor -> isContentModified())
			? modified
			: empty
	);

	// update tab text

	setTabText(index,view -> displayNameForUI());

	// update tooltip text

	auto doc = view -> document;
	auto rootDoc = doc -> getRootDocument();
	
	QString tooltip = QDir::toNativeSeparators(view -> editor -> fileName());
	
	if(doc != rootDoc)
		tooltip += tr("\nincluded document in %1").arg(rootDoc -> getName());

	setTabToolTip(index,tooltip);
}


/*!
 * Helper function. This is bound to editor signals and calls updateTab() on
 * the tab to which the sending editor belongs.
 */

void TxsTabWidget::updateTabFromSender(){

	auto editor = qobject_cast<QEditor *>(sender());
	
	for(int i = 0;i < count();i++)
		if(editorAt(i) -> editor == editor){
			updateTab(i);
			return;
		}
}


void ChangeAwareTabBar::mousePressEvent(QMouseEvent * event){

	int current = currentIndex();
	int toIndex = tabAt(event -> pos());
	
	if(event -> button() == Qt::LeftButton){
		if(toIndex >= 0)
			emit currentTabAboutToChange(current,toIndex);
	} else
	if(event -> button() == Qt::MiddleButton){
		
		int tabNr = tabAt(event -> pos());
		
		if(tabNr >= 0)
			emit middleMouseButtonPressed(tabNr);
	}

	QTabBar::mousePressEvent(event);
	
	if(event -> button() == Qt::LeftButton)
		emit tabLeftClicked();
}
