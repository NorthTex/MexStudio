
#include "ui_texdocdialog.h"
#include "Include/UtilsUI.hpp"

#include "Latex/Repository.hpp"
#include "Dialogs/TexDoc.hpp"


TexdocDialog::TexdocDialog(QWidget * widget,TexHelp * help) 
	: QDialog(widget)
	, ui(new Ui::TexdocDialog)
	, packageNameValidator(this)
	, openButton(nullptr)
	, help(help) {

	ui -> setupUi(this);
	UtilsUi::resizeInFontHeight(this,28,10);

	const auto buttonBox = ui -> buttonBox;
	const auto buttons = buttonBox -> buttons();

	for(const auto button : buttons){
		
		const auto role = buttonBox -> buttonRole(button);
		
		if(role == QDialogButtonBox::AcceptRole){
			openButton = button;
			break;
		}
	}

    packageNameValidator.setRegularExpression(QRegularExpression("[0-9a-zA-Z\\-\\.]*"));
	
	const auto packages = ui -> cbPackages;

	packages 
		-> lineEdit() 
		-> setValidator(& packageNameValidator);
	
	packages -> setMaxVisibleItems(15);

	checkTimer.setSingleShot(true);

	connect(& checkTimer,SIGNAL(timeout()),SLOT(checkDockAvailable()));
	connect(packages,SIGNAL(editTextChanged(QString)),SLOT(searchTermChanged(QString)));
    connect(help,SIGNAL(texdocAvailableReply(QString,bool,QString)),SLOT(updateDocAvailableInfo(QString,bool,QString)));

	// initially disable warning message

	updateDocAvailableInfo("",false);

    if(openButton) 
		openButton -> setEnabled(true);
}


TexdocDialog::~TexdocDialog(){
	delete ui;
}


void TexdocDialog::searchTermChanged(const QString & text){
	
	const auto repository = LatexRepository::instance();
	const auto description = repository -> shortDescription(text);

	ui 
		-> lbShortDescription
		-> setText(description);
	
	delayedCheckDocAvailable(text);
}


void TexdocDialog::setPackageNames(const QStringList & packages){

	auto packs = ui -> cbPackages;

	packs -> clear();

	if(packages.isEmpty())
		return;
		
	QStringList packageList(packages);
	packageList.sort();
	
	packs -> addItems(packageList);
	packs -> setCurrentIndex(0);

	packs 
		-> lineEdit() 
		-> selectAll();
}


void TexdocDialog::setPreferredPackage(const QString & package){

	auto packages = ui -> cbPackages;

	int index = packages -> findText(package);

	auto name = QString(package);
	auto position = 0;

	if(index < 0 && QValidator::Acceptable == packageNameValidator.validate(name,position)){
		packages -> addItem(package);
		index = packages -> findText(package);
	}

	packages -> setCurrentIndex(index);

	packages 
		-> lineEdit() 
		-> selectAll();
}


QString TexdocDialog::selectedPackage() const {
	return ui 
		-> cbPackages 
		-> currentText();
}


// use delayed checking because the auto completion of the combo box fires two events
// one with the actually typed text and one with the completed text

void TexdocDialog::delayedCheckDocAvailable(const QString & package){
	lastDocRequest = package;
	checkTimer.start(10);
}


void TexdocDialog::checkDockAvailable(){

    if(lastDocRequest.isEmpty())
		updateDocAvailableInfo("",false);
    else
        help -> texdocAvailableRequest(lastDocRequest);
}


void TexdocDialog::updateDocAvailableInfo(
	const QString & package,
	bool available,
	QString customWarning
){

	// the request may have come from someone else

	if(package != lastDocRequest)
		return;

	const bool showWarning = 
		! package.isEmpty() && 
		! available;

	const auto warning = customWarning.isNull() 
		? tr("No Documentation Available") 
		: customWarning;
	
	if(openButton)
		openButton -> setEnabled(available);

	ui 
		-> lbInfo
		-> setText(showWarning ? warning : "");

	ui 
		-> lbWarnIcon
		-> setVisible(showWarning);
}
