
#include "universalinputdialog.h"
#include "qdialogbuttonbox.h"
#include "smallUsefulFunctions.h"

UniversalInputDialog::UniversalInputDialog(QWidget * widget)
	: QDialog(widget) {
	
	setWindowTitle(TEXSTUDIO);
	gridLayout = new QGridLayout();
}


void UniversalInputDialog::myAccept(){

	for(auto & property : properties)
		property.readFromObject((QWidget *) property.widgetOffset);
	
	accept();
}


void UniversalInputDialog::addWidget(
	QWidget * widget,
	const QString & description,
	const ManagedProperty & property
){
	Q_ASSERT(property.storage);

	properties << property;
	
	properties.last().widgetOffset = (ptrdiff_t) widget;
	property.writeToObject(widget);

	widget -> setSizePolicy(QSizePolicy::MinimumExpanding,widget -> sizePolicy().verticalPolicy());

	auto label = new QLabel(description,this);
	label -> setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Preferred);

	if(description.length() < 32){
        gridLayout -> addWidget(label,gridLayout -> rowCount(),0);
        gridLayout -> addWidget(widget,gridLayout -> rowCount() - 1,1);
	} else {
        gridLayout -> addWidget(label,gridLayout -> rowCount(),0,1,2);
        gridLayout -> addWidget(widget,gridLayout -> rowCount(),0,1,2);
	}
}


QCheckBox * UniversalInputDialog::addCheckBox(
	const ManagedProperty & property,
	const QString & description
){
	auto checkBox = new QCheckBox(this);
	checkBox -> setText(description);
	
	properties << property;
	
	property.writeToObject(checkBox);
	properties.last().widgetOffset = (ptrdiff_t) checkBox;
    
	gridLayout -> addWidget(checkBox,gridLayout -> rowCount(),1);
	
	return checkBox;
}


QComboBox * UniversalInputDialog::addComboBox(
	const ManagedProperty & property,
	const QString & description
){
	auto box = new QComboBox(this);
	addWidget(box,description,property);
	return box;
}


QSpinBox * UniversalInputDialog::addSpinBox(
	const ManagedProperty & property,
	const QString & description
){
	auto box = new QSpinBox(this);
	box -> setMaximum(10000000);
	
	addWidget(box,description,property);
	
	return box;
}

QDoubleSpinBox * UniversalInputDialog::addDoubleSpinBox(
	const ManagedProperty & property,
	const QString & description
){
	auto box = new QDoubleSpinBox(this);
	box -> setMinimum(-10000000);
	box -> setMaximum(10000000);
	
	addWidget(box,description,property);
	
	return box;
}


QLineEdit * UniversalInputDialog::addLineEdit(
	const ManagedProperty & property,
	const QString & description
){
	auto edit = new QLineEdit(this);
	addWidget(edit,description,property);
	return edit;
}


QTextEdit * UniversalInputDialog::addTextEdit(
	const ManagedProperty & property,
	const QString & description
){
	auto edit = new QTextEdit(this);
	addWidget(edit,description,property);
	return edit;
}


QCheckBox * UniversalInputDialog::addVariable(
	bool * variable,
	const QString & description
){
	return addCheckBox(ManagedProperty(variable),description);
}


QSpinBox * UniversalInputDialog::addVariable(
	int * variable,
	const QString & description
){
	return addSpinBox(ManagedProperty(variable),description);
}


QLineEdit * UniversalInputDialog::addVariable(
	QString * variable,
	const QString & description
){
	return addLineEdit(ManagedProperty(variable),description);
}


QComboBox * UniversalInputDialog::addVariable(
	int * variable,
	const QStringList & options,
	const QString & description
){
	ManagedProperty property(variable);
	auto box = new QComboBox(this);
	box -> addItems(options);
	
	addWidget(box,description,property);
	
	return box;
}


QComboBox * UniversalInputDialog::addVariable(
	QStringList * variable,
	const QString & description
){
	return addComboBox(ManagedProperty(variable),description);
}


QTextEdit * UniversalInputDialog::addTextEdit(
	QString * variable,
	const QString & description
){
	return addTextEdit(ManagedProperty(variable),description);
}


QDoubleSpinBox * UniversalInputDialog::addVariable(
	float * variable,
	const QString & description
){
	return addDoubleSpinBox(ManagedProperty(variable),description);
}


void UniversalInputDialog::showEvent(QShowEvent * event){

	QDialog::showEvent(event);
	
	if(event -> spontaneous())
		return;
	
	auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal,this);
	box -> button(QDialogButtonBox::Ok)->setDefault(true);
	
	connect(box,SIGNAL(accepted()),this,SLOT(myAccept()));
	connect(box,SIGNAL(rejected()),this,SLOT(reject()));
    
	gridLayout -> addWidget(box,gridLayout -> rowCount(),0,1,2);
	setLayout(gridLayout);
}
