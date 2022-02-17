
#include "Include/Encoding.hpp"
#include "configmanager.h"
#include "minisplitter.h"
#include "Latex/LogWidget.hpp"


/*
 * row heights of tables are quite large by default. Experimentally detect the
 * optimal row height in a portable way by resizing to contents and getting
 * the resulting row height
 */

int getOptimalRowHeight(QTableView * view){

	QStandardItemModel testModel;
	auto testItem = QStandardItem("Test");
	testModel.appendRow(& testItem);
	
	view -> setModel(& testModel);
	view -> resizeRowsToContents();
	
	const int height = view
		-> rowHeight(0);

	const auto model = view
		-> model();

	view -> setModel(model);
	
	return height;
}


LatexLogWidget::LatexLogWidget(QWidget * widget) 
	: QWidget(widget)
	, proxyModel(nullptr)
	, logModel(nullptr)
	, filterErrorAction(nullptr)
	, filterWarningAction(nullptr)
	, filterBadBoxAction(nullptr)
	, logpresent(false) {

	//needs loaded line marks

	logModel = new LatexLogModel(this);
	errorTable = new QTableView(this);

	int rowHeight = getOptimalRowHeight(errorTable);
	errorTable->verticalHeader()->setDefaultSectionSize(rowHeight);

	QFontMetrics metrics(QApplication::font());

	errorTable -> setSelectionBehavior(QAbstractItemView::SelectRows);
	errorTable -> setSelectionMode(QAbstractItemView::SingleSelection);
	errorTable -> setColumnWidth(0,UtilsUi::getFmWidth(metrics,"> "));
	errorTable -> setColumnWidth(1,20 * UtilsUi::getFmWidth(metrics,"w"));
	errorTable -> setColumnWidth(2,UtilsUi::getFmWidth(metrics,"WarningW"));
	errorTable -> setColumnWidth(3,UtilsUi::getFmWidth(metrics,"Line WWWWW"));
	errorTable -> setColumnWidth(4,20 * UtilsUi::getFmWidth(metrics,"w"));

    connect(errorTable,
		SIGNAL(clicked(const QModelIndex&)),this,
		SLOT(clickedOnLogModelIndex(const QModelIndex&)));

	errorTable 
		-> horizontalHeader() 
		-> setStretchLastSection(true);

	errorTable -> setMinimumHeight(5 * rowHeight);
	errorTable -> setFrameShape(QFrame::NoFrame);
	errorTable -> setSortingEnabled(true);

	proxyModel = new QSortFilterProxyModel(this);
	proxyModel -> setSourceModel(logModel);
	errorTable -> setModel(proxyModel);

	auto action = new QAction(tr("&Copy"),errorTable);
	connect(action,SIGNAL(triggered()),SLOT(copyMessage()));
	errorTable -> addAction(action);

	action = new QAction(tr("&Copy All"),errorTable);
	connect(action,SIGNAL(triggered()),SLOT(copyAllMessages()));
	errorTable -> addAction(action);

	action = new QAction(tr("&Copy All With Line Numbers"),errorTable);
	connect(action,SIGNAL(triggered()),SLOT(copyAllMessagesWithLineNumbers()));
	errorTable -> addAction(action);
	
	errorTable -> setContextMenuPolicy(Qt::ActionsContextMenu);

	log = new LogEditor(this);
	log -> setFocusPolicy(Qt::ClickFocus);
	log -> setMinimumHeight(3 * (metrics.lineSpacing() + 4));
	log -> setReadOnly(true);
	log -> setFrameShape(QFrame::NoFrame);
	connect(log,SIGNAL(clickOnLogLine(int)),this,SLOT(gotoLogLine(int)));

	auto splitter = new MiniSplitter(Qt::Vertical,this);
	splitter -> setChildrenCollapsible(false);
	splitter -> addWidget(errorTable);
	splitter -> addWidget(log);

	infoLabel = new QLabel(tr("No log file available"),this);
	infoLabel -> setStyleSheet("color: black; background: #FFFBBF;");
	infoLabel -> setMargin(2);

	//contains the widgets for the normal mode (OutputTable + OutputLogTextEdit)

	auto vLayout = new QVBoxLayout();
	vLayout -> setSpacing(0);
    vLayout -> setContentsMargins(0,0,0,0);
	vLayout -> addWidget(infoLabel);
	vLayout -> addWidget(splitter);
	setLayout(vLayout);

	displayTableAction = new QAction(tr("Issues"),this);
	displayTableAction -> setCheckable(true);
	connect(displayTableAction,SIGNAL(triggered(bool)),this,SLOT(setWidgetVisibleFromAction(bool)));

	displayLogAction = new QAction(tr("Log File"),this);
	displayLogAction -> setCheckable(true);
	connect(displayLogAction,SIGNAL(triggered(bool)),this,SLOT(setWidgetVisibleFromAction(bool)));
	
	filterErrorAction = new QAction(QIcon(":/images-ng/error.svgz"),tr("Show Error"),this);
	filterErrorAction -> setCheckable(true);
	filterErrorAction -> setChecked(true);
	connect(filterErrorAction,SIGNAL(toggled(bool)),this,SLOT(filterChanged(bool)));
	
	filterWarningAction = new QAction(QIcon(":/images-ng/warning.svgz"),tr("Show Warning"),this);
	filterWarningAction -> setCheckable(true);
	filterWarningAction -> setChecked(true);
	connect(filterWarningAction,SIGNAL(toggled(bool)),this,SLOT(filterChanged(bool)));
	
	filterBadBoxAction = new QAction(QIcon(":/images-ng/badbox.svg"),tr("Show BadBox"),this);
	filterBadBoxAction -> setCheckable(true);
	filterBadBoxAction -> setChecked(true);
	connect(filterBadBoxAction,SIGNAL(toggled(bool)),this,SLOT(filterChanged(bool)));

	// initialize visibility

	displayTableAction -> setChecked(true);
	errorTable -> setVisible(true);
	displayLogAction -> setChecked(false);
	log -> setVisible(false);
}


bool LatexLogWidget::loadLogFile(
	const QString & logname,
	const QString & compiledFileName,
	QTextCodec * fallbackCodec
){
	resetLog();

	QFileInfo info(logname);

	if(logname.isEmpty() || !info.exists()){
		setInfo(tr("Log file not found."));
		return false;
	}

	if(!info.isReadable()){
		setInfo(tr("Log file not readable."));
		return false;
	}

	QFile file(logname);

	if(file.open(QIODevice::ReadOnly)){

        auto config = ConfigManagerInterface::getInstance();
        
		double fileSizeLimitMB = config
			-> getOption("LogView/WarnIfFileSizeLargerMB")
			.  toDouble();
        
		auto rememberChoice = static_cast<UtilsUi::txsWarningState>(
			config 
				-> getOption("LogView/RememberChoiceLargeFile",0)
				.  toInt()
		);
        
		if(file.size() > fileSizeLimitMB * 1024 * 1024){
            
			bool result = UtilsUi::txsConfirmWarning(
				tr("The logfile is very large (%1 MB) are you sure you want to load it?")
					.arg(double(file.size()) / 1024 / 1024, 0, 'f', 2),rememberChoice);
            
			config -> setOption("LogView/RememberChoiceLargeFile",static_cast<int>(rememberChoice));
            
			if(!result)
                return false;
        }

		QByteArray fullLog = file.readAll();

		file.close();

		int sure;
		auto codec = Encoding::guessEncodingBasic(fullLog,& sure);

		if(sure < 2 || !codec)
			codec = fallbackCodec 
				? fallbackCodec 
				: QTextCodec::codecForLocale();

		log -> setPlainText(codec -> toUnicode(fullLog));
		logModel -> parseLogDocument(log -> document(),compiledFileName);

		logpresent = true;

		// workaround to https://sourceforge.net/p/texstudio/feature-requests/622/
		// There seems to be a bug in Qt (4.8.4) that resizeRowsToContents() does not work correctly if
		// horizontalHeader()->setStretchLastSection(true) and the tableView has not yet been shown
		// when iterating through the columns to determine the maximal height, everything is fine
		// until the last column. There the calculated height is too large.
		// As a workaround we will temporarily deactivate column stretching.
		// Note: To reproduce, you can call the viewer via The ViewLog button. When showing the viewer
		// by clicking the ViewTab, the widget is shown before loading (so there the bug did not appear.)

		bool visible = errorTable -> isVisible();
		
		if(!visible)
			errorTable 
				-> horizontalHeader()
				-> setStretchLastSection(false);
		
		errorTable -> resizeColumnsToContents();
		errorTable -> resizeRowsToContents();
		
		if(!visible)
			errorTable
				-> horizontalHeader()
				-> setStretchLastSection(true);

		selectLogEntry(0);
		
		emit logLoaded();

		return true;
	}

	setInfo(tr("Log file not readable."));
	
	return false;
}


bool LatexLogWidget::logPresent(){
	return logpresent;
}


void LatexLogWidget::resetLog(){

	logModel -> clear();
	log -> clear();
	
	logpresent = false;
	
	setInfo("");
	
	emit logResetted();
}


bool LatexLogWidget::logEntryNumberValid(int entry){

	if(entry < 0)
		return false;

	const auto size = logModel 
		-> count();

	return entry < size;
}


void LatexLogWidget::selectLogEntry(int entry){

	if(!logEntryNumberValid(entry))
		return;
	
	QModelIndex index = proxyModel
		-> mapFromSource(logModel -> index(entry,1));
	
	errorTable -> scrollTo(index,QAbstractItemView::PositionAtCenter);
	errorTable -> selectRow(index.row());

	const auto line = logModel 
		-> at(entry).logline ;

	log -> setCursorPosition(line,0);
}


void LatexLogWidget::copy(){

	if(log -> isVisible())
		log -> copy();
	else
		copyMessage();
}


void LatexLogWidget::gotoLogEntry(int entry){

	if(logEntryNumberValid(entry)){	
		selectLogEntry(entry);
		emit logEntryActivated(entry);
	}
}


void LatexLogWidget::clickedOnLogModelIndex(const QModelIndex & index){
	
	const auto source = proxyModel
		-> mapToSource(index);
	
	gotoLogEntry(source.row());
}


void LatexLogWidget::gotoLogLine(int line){

	const auto entry = logModel
		-> logLineNumberToLogEntryNumber(line);

	gotoLogEntry(entry);
}


void LatexLogWidget::copyMessage(){

	auto message = proxyModel -> mapToSource(errorTable -> currentIndex());
	
	if(!message.isValid())
		return;
	
	message = logModel -> index(message.row(),3);
	
	auto clipboard = QApplication::clipboard();

	REQUIRE(clipboard);
	
	const auto text = logModel 
		-> data(message,Qt::DisplayRole)
		.  toString();

	clipboard -> setText(text);
}


void LatexLogWidget::copyAllMessages(){

	QStringList messages;
	
	for(int i = 0;i < logModel -> count();i++)
		messages << logModel
			-> data(logModel -> index(i,3),Qt::DisplayRole)
			.  toString();
	
	auto clipboard = QApplication::clipboard();

	REQUIRE(clipboard);
	
	clipboard -> setText(messages.join('\n'));
}


void LatexLogWidget::copyAllMessagesWithLineNumbers(){

	QStringList messages;
	
	for(int i = 0;i < logModel -> count();i++){

		const auto line = logModel
			-> data(logModel -> index(i,2),Qt::DisplayRole)
			.  toString();

		const auto message = logModel
			-> data(logModel -> index(i,3),Qt::DisplayRole)
			.  toString();

		messages << line + ": " + message;
	}
	
	auto clipboard = QApplication::clipboard();

	REQUIRE(clipboard);
	
	clipboard -> setText(messages.join('\n'));
}


void LatexLogWidget::setWidgetVisibleFromAction(bool visible){

	auto action = qobject_cast<QAction *>(sender());
	
	if(action == displayTableAction){

		errorTable -> setVisible(visible);
		
		if(visible)
			return;
			
		if(log -> isVisible())
			return;
		
		// fallback, one widget should always be visible

		displayLogAction -> setChecked(true);
		log -> setVisible(true);

		return;
	}
	
	if(action == displayLogAction){

		log -> setVisible(visible);
		
		if(visible)
			return;

		if(errorTable -> isVisible())
			return;
			
		// fallback, one widget should always be visible

		displayTableAction -> setChecked(true);
		errorTable -> setVisible(true);
	}
}


void LatexLogWidget::setInfo(const QString & message){
	infoLabel -> setText(message);
	infoLabel -> setVisible(!message.isEmpty());
}


QList<QAction *> LatexLogWidget::displayActions(){

	const auto separator = [ & ](){
		auto action = new QAction(this);
		action -> setSeparator(true);
		return action;
	};
	
	return QList<QAction *>()
		<< displayLogAction 
		<< displayTableAction 
		<< separator() 
		<< filterErrorAction 
		<< filterWarningAction 
		<< filterBadBoxAction 
		<< separator() ;
}


void LatexLogWidget::filterChanged(bool){

	QStringList filters;
	
	if(filterErrorAction && filterErrorAction -> isChecked())
		filters << logModel -> returnString(LT_ERROR);
	
	if(filterWarningAction && filterWarningAction -> isChecked())
		filters << logModel -> returnString(LT_WARNING);
	
	if(filterBadBoxAction && filterBadBoxAction -> isChecked())
		filters << logModel -> returnString(LT_BADBOX);
	
	const auto regex = filters.join('|');

    proxyModel -> setFilterRegularExpression(regex);
	proxyModel -> setFilterKeyColumn(1);
}
