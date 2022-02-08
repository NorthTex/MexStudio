#include "ui_maketemplatedialog.h"
#include "smallUsefulFunctions.h"
#include "utilsUI.h"

#include <QJsonDocument>

#include "Wrapper/Connect.hpp"
#include "Dialogs/MakeTemplate.hpp"


inline QString today(){
	return QDate::currentDate().toString(Qt::ISODate);
}

inline QString toJson(const QJsonObject & object){
	return QJsonDocument(object).toJson();
}


MakeTemplateDialog::MakeTemplateDialog(QString templateDir, QString editorFilename, QWidget *parent) 
	: QDialog(parent)
	, ui(new Ui::MakeTemplateDialog)
	, m_templateDir(templateDir)
	, m_editorFilename(editorFilename) {

	ui -> setupUi(this);

	UtilsUi::resizeInFontHeight(this,31,25);

	wire(ui -> buttonBox,accepted(),tryAccept());
	wire(ui -> buttonBox,rejected(),reject());

	ui -> leVersion -> setText("1.0");
	ui -> leAuthor -> setText(getUserName());

	ui -> cbLicense -> clearEditText();
}


MakeTemplateDialog::~MakeTemplateDialog(){
	delete ui;
}


QString invalidChars = "\\/:*?\"<>|"; // for windows, OSX and linux is less restrictive but we use this to guarantee compatibility

void MakeTemplateDialog::tryAccept(){

	QString fn = ui -> leName -> text();
	
	for(auto && c : invalidChars)
		fn.remove(c);

	if(fn.length() > 80)
        fn.remove(80,fn.length() - 80);
	
	fn.prepend("template_");
	
	QString ext = QFileInfo(m_editorFilename).completeSuffix();
	
	if(ext.isEmpty())
		ext = "tex";

	fn.append("." + ext);
	
	m_suggestedFile = QFileInfo(QDir(m_templateDir),fn);
	
	if(m_suggestedFile.exists()){

		bool overwrite = UtilsUi::txsConfirmWarning(tr("A template with the given name already exists.\nDo you want to overwrite it?") + "\n" + m_suggestedFile.canonicalFilePath());
		
		if(!overwrite)
			return;
	}

	accept();
}


QString MakeTemplateDialog::generateMetaData(){

    QJsonObject data;

    data["Description"] = ui -> leDescription -> toPlainText();
    data["License"] = ui -> cbLicense -> currentText(); // last entry does not have colon
    data["Version"] = ui -> leVersion -> text();
    data["Author"] = ui -> leAuthor -> text();
    data["Name"] = ui -> leName -> text();
    data["Date"] = today();

	return toJson(data);
}


#undef wire
#undef unwire
