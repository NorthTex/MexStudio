#include "DblClickMenubar.hpp"
#include "QMouseEvent"


DblClickMenuBar::DblClickMenuBar(QWidget * parent) 
	: QMenuBar(parent){}

void DblClickMenuBar::mouseDoubleClickEvent(QMouseEvent * event){

	if(actionAt(event -> pos())){
		QMenuBar::mouseDoubleClickEvent(event);
		return;
	}

	emit doubleClicked();
}
