
#include "BibTex/Dialog.hpp"
#include "Include/UtilsUI.hpp"


using BibTex::Dialog;
using TexType = BibTex::Type;
using StringMap = QMap<QString,QString>;


Dialog::Dialog(QWidget * parent,strings files,int currentFile,string id)
	: QDialog(parent)
	, ui(new UI)
	, fileId(-2){

	ui -> setup(this);

	UtilsUi::resizeInFontHeight(this,59,36);

	auto fileList = ui -> files;

	fileList -> addItem(tr("<New File>"));
	fileList -> addItems(files);
	fileList -> setCurrentRow(currentFile + 1);

	updatePropertyLists();

	if(!id.isEmpty()){

		auto fields = ui -> fields;

		fields -> setRowCount(1);

		auto * item = new QTableWidgetItem("ID");

		auto font = QApplication::font();
		font.setBold(true);
		item -> setFont(font);

		fields -> setItem(0,0,item);
		fields -> setItem(0,1,new QTableWidgetItem(id));
	}

	for(auto && type : * entries){
		QString description = type.description;
		auto * item = new QListWidgetItem(description.remove('&'),ui -> types);
		item -> setToolTip(type.name);
	}

	connect(ui -> types,SIGNAL(currentRowChanged(int)),SLOT(updateTypeProperties()));

	setWindowTitle((type == BibLatex)
		? tr("New BibLaTeX Entry")
		: tr("New BibTeX Entry"));
}

Dialog:: ~ Dialog(){ delete ui; }

void Dialog::setType(Type t){
	type = t;
	updatePropertyLists();
}


QString Dialog::textToInsert(const TexType & entry,bool keepOptional,const QMap<QString,QString> & fields){

	QString text = entry.name + "{" + fields.value("ID", "%<ID%>") + ",\n";

	auto data = fields;
	data.remove("ID");

	auto addResult = [ & text , & data ](auto key) -> bool {

		if(data.contains(key)){
			auto value = data.take(key);
			text += key + " = {" + value + "},\n";
			return true;
		}

		return false;
	};


	//	Required Fields

	for(auto && key : entry.required){

		if(key.contains('/')){

			auto parts = key.split('/');
			bool contained = false;

			for(auto && part : parts)
				if((contained = addResult(part)))
					break;

			if(contained)
				continue;

			for(auto && part : parts)
				text += "ALT" + part + " = {%<" + part + "%>},\n";

			continue;
		}

		if(addResult(key))
			continue;

		text += key + " = {%<" + key + "%>},\n";
	}


	//	Optional Fields

	for(auto && key : entry.optional){

		if(data.contains(key)){
			auto value = data.take(key);
			text += key + " = {" + value + "},\n";
			continue;
		}

		if(keepOptional)
			text += "OPT" + key + " = {%<" + key + "%>},\n";
	}


	//	Remaining Fields

	for(auto const & [ key , value ] : data.toStdMap())
		text += key + " = {" + value + "},\n";


	text += "}\n";

	return text;
}


QString Dialog::textToInsert(const QString & entryName){

	updatePropertyLists();

	for(auto && type : * entries)
		if(type.name == entryName)
			return textToInsert(type,true,QMap<QString,QString>());

	return "";
}


QList<TexType> Dialog::entryTypesFor(Type type){
	return (type == BibTex)
		? generateBibTex(true)
		: generateBibLatex(true);
}


void Dialog::changeEvent(QEvent * event){

	if(event -> type() == QEvent::LanguageChange)
		ui -> translate();
}


void Dialog::accept(){

	result = "";

	const auto & types = ui -> types;

	if(types -> currentRow() < 0)
		return;

	if(types -> currentRow() >= entries -> count())
		return;

	QMap<QString,QString> data;

	const auto & fields = ui -> fields;

	for(int i = 0;i < fields -> rowCount();i++){

		if(! fields -> item(i,0))
			continue;

		if(! fields -> item(i,1))
			continue;

		QString key = fields -> item(i,0) -> text();

		if(key.contains('/'))
			key = key.split('/').first();

		QString value = fields -> item(i, 1) -> text();

		if(value == "")
			continue;

		data.insert(key,value);
	}

	result = textToInsert(entries -> at(types -> currentRow()),ui -> checkbox -> isChecked(),data);
	fileId = ui -> files -> currentRow() - 1;

	QDialog::accept();
}


void Dialog::updateTypeProperties(){

	const auto & types = ui -> types;

	if(types -> currentRow() < 0)
		return;

	if(types -> currentRow() >= entries -> count())
		return;


	QMap<QString,QString> data;

	const auto & fields = ui -> fields;

	for(int i = 0;i < fields -> rowCount();i++){

		if(! fields -> item(i,0))
			continue;

		if(! fields -> item(i,1))
			continue;

		data.insert(
			fields -> item(i,0) -> text(),
			fields -> item(i,1) -> text()
		);
	}


	fields -> clearContents();

	const auto & texType = entries -> at(types -> currentRow());

	auto required = texType.required;
	auto optional = texType.optional;


	fields -> setRowCount(required.count() + optional.count() + 10);

	QFont font = QApplication::font();
	font.setBold(true);

	int row = 0;
	required.prepend("ID");


	auto addItem = [ & row , & fields , & data , & font ]
	( auto type , bool required ){

		auto key = new QTableWidgetItem(type);
		key -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		key -> setTextAlignment(Qt::AlignCenter);

		if(required)
			key -> setFont(font);

		auto value = new QTableWidgetItem(data.value(type,""));

		fields -> setItem(row,0,key);
		fields -> setItem(row,1,value);

		row++;
	};


	for(auto && type : required)
		addItem(type,true);

	for(auto && type : optional)
		addItem(type,false);


	fields -> setCurrentCell(0,1);
	fields -> resizeRowsToContents();
}


void Dialog::updatePropertyLists(){
	if(type == BibTex){
		generateBibTex();
		entries = & bibtex;
	} else {
		generateBibLatex();
		entries = & biblatex;
	}
}
