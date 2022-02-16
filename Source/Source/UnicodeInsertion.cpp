#include "unicodeinsertion.h"
#include "utilsUI.h"


QString unicodePointToString(unsigned int unicode){
	return QString::fromUcs4(& unicode,1);
}


QString unicodePointToUtf8Hex(unsigned int unicode){
	return unicodePointToString(unicode)
		.toUtf8()
		.toHex();
}


QLineEditWithMetaText::QLineEditWithMetaText(QWidget * widget)
	: QLineEdit(widget) {}


void QLineEditWithMetaText::setMetaText(const QString & string){
	metaText = string;
	update();
}


void QLineEditWithMetaText::paintEvent(QPaintEvent * event){

	QLineEdit::paintEvent(event);
	
	if(metaText.isEmpty())
		return;

	QFontMetrics metrics(font());

	const auto color = QApplication::palette()
		.windowText()
		.color()
		.lighter(50);

	const auto
		txtWidth = UtilsUi::getFmWidth(metrics,metaText) ,
		txtHeight = metrics.height();

	const auto
		x = width() - txtWidth - 5 ,
		y = (height() + txtHeight) / 2 - 2 ;

	QPainter painter(this);
	painter.setPen(color);
	painter.drawText(x,y,metaText);
}


UnicodeInsertion::UnicodeInsertion(QWidget * widget,uint defCharCode)
	: QWidget(widget)
	, defaultCharCode(defCharCode){

	QFontMetrics metrics(QApplication::font());
    
	int bw = metrics.maxWidth() + 1;
    int bh = qMax(metrics.height(),metrics.lineSpacing()) + 1;
    int bw_average = metrics.averageCharWidth();
    
	table = new QTableWidget(this);

	table -> setMinimumWidth(16 * bw);
    table -> setMinimumHeight(3 * bh);
	table -> setRowCount(3);
	table -> setColumnCount(16);
	
	for(int r = 0;r < table -> rowCount();r++)
		table -> setRowHeight(r,bh);
    
	for(int c = 0; c < table -> columnCount();c++)
		table -> setColumnWidth(c,bw);

	table -> horizontalHeader() -> hide();
	table -> verticalHeader() -> hide();
	table -> resizeRowsToContents();
	table -> resizeColumnsToContents();
	table -> setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table -> setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
	connect(table,SIGNAL(cellClicked(int,int)),SLOT(tableCellClicked(int,int)));
    connect(table,SIGNAL(cellDoubleClicked(int,int)),SLOT(tableCellDoubleClicked(int,int)));

	auto layout = new QVBoxLayout();
	
	edit = new QLineEditWithMetaText(this);
	layout -> addWidget(edit);
	layout -> addWidget(table);
    
	int widgetWidth = qMax(19 * bw,44 * bw_average);
    resize(widgetWidth,5 * bh + 2 * edit -> height());

	this -> setLayout(layout);

	setFocusProxy(edit);
	connect(edit,SIGNAL(textChanged(QString)),SLOT(editChanged(QString)));
	setAttribute(Qt::WA_DeleteOnClose,true);

	QString text = "0x";

	if(defaultCharCode != 0)
		text = QString("0x%1")
			.arg(defaultCharCode,0,16);

	edit -> setText(text);
}


void UnicodeInsertion::setTableText(int row,int column,const QString & string){

	auto item = table -> item(row,column);

	if(item)
		item -> setText(string);
	else
		item = new QTableWidgetItem(string);

	table -> setItem(row,column,item);
}


void UnicodeInsertion::keyPressEvent(QKeyEvent * event){

	switch(event -> key()){
	case Qt::Key_Return :
	case Qt::Key_Enter :{

		const auto item = table -> item(0,8);

		if(item){

			const auto text = item -> text();

			if(text != "" && text != unicodePointToString(defaultCharCode))
				emit insertCharacter(text);
		}
	}
	case Qt::Key_Escape :
		close();
		return;
	}
}


void UnicodeInsertion::editChanged(const QString & newText){

	QString nt = newText.trimmed();
	
	if(nt.startsWith('\'')){
		
		if(nt.length() < 2)
			return;
		
		unsigned int unicode = nt.at(1).unicode();
		
		if(nt.at(1).isHighSurrogate()){
			
			if(nt.length() < 3)	
				return;
			
			unicode = (((unicode & 0x3FF) << 10) | (nt.at(2).unicode() & 0x3FF)) + 0x10000;
		}

		edit -> setText("0x" + QString::number(unicode,16));

		return;
	}

    int base = 16;
	
	if(nt.startsWith("0x",Qt::CaseInsensitive))
		nt.remove(0, 2);
	else
	if(nt.startsWith("x",Qt::CaseInsensitive))
		nt.remove(0, 1);
	else
		base = 10;

    unsigned int c = QString(nt)
		.toUInt(nullptr,base);

	QString utf8 = "";
	
	if(c > 0x7f) 
		utf8 = QString(", utf-8: %1").arg(unicodePointToUtf8Hex(c));


	const auto meta = QString("%1 (cp: %2%3%4)")
		.arg(unicodePointToString(c))
		.arg(base == 16 ? "" : "0x")
		.arg(c,0,base)
		.arg(utf8);

	setTableText(0,8,unicodePointToString(c));

    for(int i = 0;i < base;i++)
		setTableText(2,i,unicodePointToString(c * base + i));

	if(nt.length() < 2)
		table -> resizeRowsToContents();
}


void UnicodeInsertion::tableCellClicked(int row,int column){

	if(row != 2)
		return;
	
	const auto cell = column >= 10 
		? QChar('A' + column - 10) 
		: QChar('0' + column);

	const auto text = edit -> text() + cell;
	
	edit -> setText(text);
	edit -> setFocus();
}


void UnicodeInsertion::tableCellDoubleClicked(int row,int column){

	if(row != 2)
		return;
		
	const auto item = table -> item(row,column);

	if(!item)
		return;
    
	//tricky, double click is reported as single click - double click
	// and the single click sets the edit box (4.6.3 on debian)

	emit insertCharacter(item -> text());
	
	close();
}

