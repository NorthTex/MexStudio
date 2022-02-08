#include "fileselector.h"
#include "utilsUI.h"
#include "Wrapper/Connect.hpp"

FileSelector::FileSelector(QWidget * parent,bool multiselect) 
	: QWidget(parent)
	, multiselect(multiselect){

	setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	
	auto vlayout = new QVBoxLayout();
	vlayout -> setContentsMargins(0,0,0,0);
	vlayout -> setSpacing(0);
	setLayout(vlayout);

	list = new QListWidget(this);
	vlayout -> addWidget(list);
	
	filter = new QLineEdit(this);
	vlayout -> addWidget(filter);
	
	wire(filter,textChanged(QString),filterChanged(QString));
	// connect(filter,SIGNAL(textChanged(QString)),SLOT(filterChanged(QString)));
	filter -> installEventFilter(this);

	auto palette = QApplication::palette(); //let the list appear selected (does not work with gtk+ style)
	palette.setColor(QPalette::Inactive,QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
	palette.setColor(QPalette::Inactive,QPalette::HighlightedText,palette.color(QPalette::Active,QPalette::HighlightedText));
	list -> setPalette(palette);
	
	wire(list,itemDoubleClicked(QListWidgetItem *),SLOT(emitChoosen()));
	// connect(list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(emitChoosen()));

	setAttribute(Qt::WA_DeleteOnClose);

	if(multiselect)
		list -> setSelectionMode(QAbstractItemView::ExtendedSelection);
}


void FileSelector::init(const QStringList & files,int current){

	filter -> setFocus();
	rawFiles = files;
	filterChanged(filter -> text());
	
	if(current >= 0 && current < rawFiles.count()){

		int r = 0;
		
		for(int i = 0;i < current;i++)
			if(list -> item(i) -> text() == rawFiles[i])
				r++;
		
		if(list -> item(r) -> text() == rawFiles[current])
			list -> setCurrentRow(r);
	}
}


void FileSelector::setCentered(){

	const auto parent = parentWidget();

	if(!parent)
		return;
	
	auto rect = parent -> geometry();
	auto size = rect.size();

	auto point = parent -> mapToGlobal(rect.topLeft());  // popups live in global coordinates

	int width = size.width() / 2;
	int scrollbarwidth = 50; //value that works on my computer...

	auto font = list -> fontMetrics();

	for(int i = 0;i < rawFiles.size();i++)
		width = qMax(width,UtilsUi::getFmWidth(font,rawFiles[i]) + scrollbarwidth);
	
	width = qMin(width,size.width());

	setGeometry(size.width() / 2 - width / 2 + point.x(),size.height() / 8 + point.y(),width,size.height() / 2);
	setMinimumWidth(width); //set geometry alone leads to a too small window. but we need to call setGeometry first, or it crashes if fsw = s.width()
}


void FileSelector::filterChanged(const QString & newFilter){

	QString nf = newFilter;
	
	if(newFilter.contains(':') && QRegExp(".*:[0-9; ]*").exactMatch(newFilter))
		nf = newFilter.left(newFilter.lastIndexOf(':'));

	QStringList filterList = nf.split(' ');

	auto oldFiles = currentFiles();
	list -> clear();
	
	for(const auto & file : rawFiles){
		
		bool skip = false;

		for(const auto & filter : filterList)
			if(!file.contains(filter,Qt::CaseInsensitive)){
				skip = true;
				break;
			}

		if(skip)
			continue;

		list -> addItem(file);
	}
	// foreach(const QString &s, rawFiles) {
	// 	bool skip = false;
	// 	foreach (const QString &tf, filterList)
	// 		if (!s.contains(tf, Qt::CaseInsensitive)) {
	// 			skip = true;
	// 			break;
	// 		}
	// 	if (skip) continue;
	// 	list->addItem(s);
	// }

	if(!oldFiles.isEmpty()){

		bool foundOne = false;
		
		for(int o = 0;o < oldFiles.size();o++){

			const auto & oldFile = oldFiles[o];
			int duplicate = oldFile.second;
			
			for(int i = 0;i < list -> count();i++)
				if(list -> item(i) -> text() == oldFile.first){
					
					duplicate -= 1;
					
					if(duplicate < 0){
						if(!foundOne)
							list -> setCurrentRow(i);
						else 
						if(list -> selectionModel() && list -> model())
							list -> selectionModel() -> select(list -> model() -> index(i,0),QItemSelectionModel::Select);
						
						foundOne = true;
						break;
					}
				}
		}

		if(foundOne)
			return;
	}

	list -> setCurrentRow(0);
}


void FileSelector::showEvent(QShowEvent * event){
	setCentered();
	QWidget::showEvent(event);
}


bool FileSelector::eventFilter(QObject * obj,QEvent * event){
	
	if(event -> type() == QEvent::KeyPress){

		int key = static_cast<QKeyEvent *>(event) -> key();
	
		switch (key){
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:

			if(obj != filter)
				break;
			
			{
				int offset = -1;
				
				if(key == Qt::Key_Down)
					offset = 1;
				else
				if(key == Qt::Key_PageUp)
					offset = - list -> height() / qMax(1,list -> fontMetrics().height());
				else
				if(key == Qt::Key_PageDown)
					offset = list -> height() / qMax(1,list -> fontMetrics().height());
				
				int row = list -> currentRow() + offset;
				
				if(row + offset < 0)
					row = 0;
				else
				if(row >= list -> count())
					row = list -> count() - 1;
				
				if(row == list -> currentRow())
					break;
				
				list -> setCurrentRow(row);
				
				return true;
			}
		case Qt::Key_Return:
		case Qt::Key_Enter:
			emitChoosen();
			return true;
		case Qt::Key_Escape:
			close();
			return true;
		}
	}

	return QObject::eventFilter(obj,event);
}


// using QPairStringInt = QPair<QString,int>;
// typedef QPair<QString, int> QPairStringInt;

void FileSelector::emitChoosen(){

	QString jumpTo = filter -> text().mid(filter -> text().lastIndexOf(':') + 1);
	
	int
		col = -1,
		line = -1;
	
	if(!jumpTo.isEmpty())
		if(jumpTo.contains(';')){
			QStringList sl = jumpTo.split(';');
			line = sl[0].trimmed().toInt() - 1;
			col = sl[1].trimmed().toInt();
		} else {
			line = jumpTo.trimmed().toInt() - 1;
		}

	for(const auto & file : currentFiles())
		emit fileChoosen(file.first,file.second,line,col);

	// foreach (const QPairStringInt &p, currentFiles())
		// emit fileChoosen(p.first, p.second, line, col);
	close();
}

using FileSelectors = QList<QPair<QString,int>>;

FileSelectors FileSelector::currentFiles(){
	
	int cindex = list -> currentRow();

	if(cindex < 0)
		return FileSelectors();
		
	if(cindex >= rawFiles.count())
		return FileSelectors();
	
	QList<int> indices;
    
	if(multiselect){
		if(list -> selectionModel())
            for(const auto & index : list -> selectionModel() -> selectedIndexes())
                indices << index.row();
    } else {
        indices << cindex;
    }

	FileSelectors result;

	for(int index : indices){

		QString file = list -> item(index) -> text();
		
		int duplicate = 0;
		
		for(int i = 0;i < index;i++)
			if(list -> item(i) -> text() == file)
				duplicate++;

		result << QPair<QString,int>(file,duplicate);
	}

	return result;
}


#undef wire
#undef unwire
