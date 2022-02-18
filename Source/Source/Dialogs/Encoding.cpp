
#include "Dialogs/EncodingDialog.hpp"
#include "Include/Encoding.hpp"


EncodingDialog::EncodingDialog(QWidget * parent,QEditor * editor) 
	: QDialog(parent)
	, edit(editor) {

	setupUi(this);
	UtilsUi::resizeInFontHeight(this,55,26);

	encodings -> setSelectionBehavior(QAbstractItemView::SelectRows);
	encodings -> setRowCount(QTextCodec::availableMibs().size());
	
	int row = 0;
	
	for(const int mib : QTextCodec::availableMibs()){

		auto codec = QTextCodec::codecForMib(mib);
		auto name = codec -> name();
		
		for(const auto & bytes : codec -> aliases())
			name += " / " + bytes;
		
		auto item = new QTableWidgetItem(name);
		item -> setData(Qt::UserRole, mib);
	
		if(
			mib == 0 || 
			mib == 4 || // latin1
			mib == 106 || // utf-8
			mib == 1013 || 
			mib == 1014 // utf16be + le
		){
			auto font = QApplication::font();
			font.setBold(true);
			item -> setFont(font);
		}
	
		encodings -> setItem(row,0,item);

		if(mib == edit -> getFileCodec() -> mibEnum())
			encodings -> setCurrentItem(item);

		item = new QTableWidgetItem(Encoding::latexNamesForTextCodec(codec).join(" or "));

		item -> setData(Qt::UserRole,mib);
		encodings -> setItem(row,1,item);
		
		row++;
	}

	encodings -> resizeColumnsToContents();
	encodings -> resizeRowsToContents();
	encodings -> setFocus();
	
	label -> setText(
		tr("Select Encoding for") + " \"" + 
		QDir::toNativeSeparators(edit -> fileName()) + 
		"\"");
    
	if(!QFileInfo::exists(edit -> fileName()))
		reload -> setEnabled(false);
}


void EncodingDialog::changeEvent(QEvent * event){

	if(event -> type() == QEvent::LanguageChange)
		retranslateUi(this);
}


const auto incompatible_encodings = 
	"If the new and old encodings are incompatible, "
	"some characters may be destroyed.\n"
	"Are you sure you want accept data loss?";

void EncodingDialog::on_view_clicked(){

	const auto rejected = UtilsUi::txsConfirmWarning(tr(incompatible_encodings));

	if(rejected){
		reject();
		return;
	}

	const auto mib = encodings 
		-> currentItem() 
		-> data(Qt::UserRole)
		.  toInt();

	edit -> viewWithCodec(QTextCodec::codecForMib(mib));
	accept();
}


void EncodingDialog::on_change_clicked(){

	const auto mib = encodings
		-> currentItem()
		-> data(Qt::UserRole)
		.  toInt();

	edit -> setFileCodec(QTextCodec::codecForMib(mib));
	accept();
}


const auto document_changes =
	"The document has been changed.\n"
	"These changes will be lost,"
	"if you reload it with the new encoding.\n"
	"Are you sure you want to undo all changes?";

void EncodingDialog::on_reload_clicked(){

	if(edit -> isContentModified()){

		const auto rejected = UtilsUi::txsConfirmWarning(tr(document_changes));

		if(rejected){
			reject();
			return;
		}
	}

	auto document = qobject_cast<LatexDocument *>(edit -> document());
	document -> initClearStructure();

	const auto mib = encodings
		-> currentItem()
		-> data(Qt::UserRole)
		.  toInt();

	edit -> load(edit -> fileName(),QTextCodec::codecForMib(mib));
	accept();
}


void EncodingDialog::on_cancel_clicked(){
	reject();
}
