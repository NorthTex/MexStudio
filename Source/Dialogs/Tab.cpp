/***************************************************************************
 *   copyright	  : (C) 2003-2011 by Pascal Brachet				 *
 *   http://www.xm1math.net/texmaker/							   *
 *														   *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or	*
 *   (at your option) any later version.							*
 *														   *
 ***************************************************************************/


#include "tabdialog.h"
#include "utilsUI.h"
#include "smallUsefulFunctions.h"
#include "Wrapper/Connect.hpp"


TabDialog::TabDialog(QWidget * parent, const char * name)
	: QDialog(parent) {

	setWindowTitle(name);
	setModal(true);
	ui.setupUi(this);

	UtilsUi::resizeInFontHeight(this,53,44);

	colData col;
	col.alignment = 0;
	col.leftborder = 0;

	liData li;
	li.topborder = true;
	li.merge = false;
	li.mergefrom = 1;
	li.mergeto = 1;

	colDataList.clear();
	liDataList.clear();

	for(int j = 0;j < 99;++j){
		colDataList.append(col);
		liDataList.append(li);
	}

	ui.tableWidget -> setRowCount( 2 );
	ui.tableWidget -> setColumnCount( 2 );

	ui.spinBoxRows -> setValue(2);
	ui.spinBoxRows -> setRange(1,99);
	
	connect(ui.spinBoxRows,SIGNAL(valueChanged(int)),this,SLOT(NewRows(int)));

	ui.spinBoxColumns -> setValue(2);
	ui.spinBoxColumns -> setRange(1,99);

	connect(ui.spinBoxColumns,SIGNAL(valueChanged(int)),this,SLOT(NewColumns(int)));

	ui.spinBoxNumCol -> setRange(1,2);
	ui.spinBoxNumLi -> setRange(1,2);
	ui.spinBoxSpanFrom -> setRange(1,1);
	ui.spinBoxSpanTo -> setRange(2,2);

	int index = 0;

	auto item = [ & ](const char * text){
		ui.comboBoxColAl -> insertItem(index++,tr(text,"tabular alignment"));
	};

	auto separator = [ & ](){
		ui.comboBoxColAl -> insertSeparator(index++);
	};

	item("Center");
	item("Left");
	item("Right");

	separator();

	item("p{} (fixed width - top / justified)");
	item("p{} (fixed width - top / left)");
	item("p{} (fixed width - top / center)");
	item("p{} (fixed width - top / right)");

	separator();

	item("m{} (fixed width - center / justified)");
	item("m{} (fixed width - center / left)");
	item("m{} (fixed width - center / center)");
	item("m{} (fixed width - center / right)");

	separator();

	item("b{} (fixed width - bottom / justified)");
	item("b{} (fixed width - bottom / left)");
	item("b{} (fixed width - bottom / center)");
	item("b{} (fixed width - bottom / right)");


	ui.comboBoxColAl -> setCurrentIndex(0);
	ui.comboBoxColAl -> setMaxVisibleItems(18);

	// alignList must match the entires in ui.comboBoxColAl
	alignlist << "c" << "l" << "r"
			  << QString("<SEP>")  << "p{3cm}" << ">{\\raggedright\\arraybackslash}p{3cm}" << ">{\\centering\\arraybackslash}p{3cm}" << ">{\\raggedleft\\arraybackslash}p{3cm}"
			  << QString("<SEP>")  << "m{3cm}" << ">{\\raggedright\\arraybackslash}m{3cm}" << ">{\\centering\\arraybackslash}m{3cm}" << ">{\\raggedleft\\arraybackslash}m{3cm}"
			  << QString("<SEP>")  << "b{3cm}" << ">{\\raggedright\\arraybackslash}p{3cm}" << ">{\\centering\\arraybackslash}b{3cm}" << ">{\\raggedleft\\arraybackslash}b{3cm}";

	alignlistLabels << QString("c") << QString("l") << QString("r")
			  << QString("<SEP>") << QString("j p{}") << QString("l p{}") << QString("c p{}") << QString("r p{}")
			  << QString("<SEP>")  << QString("j m{}") << QString("l m{}") << QString("c m{}") << QString("r m{}")
			  << QString("<SEP>")  << QString("j b{}") << QString("l b{}") << QString("c b{}") << QString("r b{}");

	ui.comboLeftBorder -> insertItem(0,"|");
	ui.comboLeftBorder -> insertItem(1,"||");
	ui.comboLeftBorder -> insertItem(2,tr("None", "tabular left border"));
	ui.comboLeftBorder -> insertItem(3,tr("@{text}", "tabular left border"));
	ui.comboLeftBorder -> setCurrentIndex(0);

	ui.comboBoxEndBorder -> insertItem(0,"|");
	ui.comboBoxEndBorder -> insertItem(1,"||");
	ui.comboBoxEndBorder -> insertItem(2,tr("None","tabular right border"));
	ui.comboBoxEndBorder -> insertItem(3,tr("@{text}","tabular right border"));
	ui.comboLeftBorder -> setCurrentIndex(0);

	// borderlist must match the entires in ui.comboLeftBorder and ui.comboBoxEndBorder
	borderlist << "|" << "||" << "" << "@{}";

	ui.spinBoxNumCol -> setValue(1);
	ui.spinBoxNumLi -> setValue(1);

	ui.checkBoxBorderTop -> setChecked(true);
	ui.checkBoxSpan -> setChecked(false);
	ui.spinBoxSpanFrom -> setValue(1);
	ui.spinBoxSpanTo -> setValue(1);
	ui.spinBoxSpanFrom -> setEnabled(false);
	ui.spinBoxSpanTo -> setEnabled(false);
	ui.checkBoxBorderBottom -> setChecked(true);
	ui.checkBoxMargin -> setChecked(false);

	wire(ui.checkBoxSpan,toggled(bool),updateSpanStatus(bool));

	wire(ui.pushButtonColumns,clicked(),applytoAllColumns());
	wire(ui.pushButtonLines,clicked(),applytoAllLines());

	wire(ui.spinBoxNumCol,valueChanged(int),showColSettings(int));
	wire(ui.spinBoxNumLi,valueChanged(int),showRowSettings(int));

	wire(ui.comboBoxColAl,currentIndexChanged(int),updateColSettings());
	wire(ui.comboLeftBorder,currentIndexChanged(int),updateColSettings());

	wire(ui.checkBoxBorderTop,toggled(bool),updateRowSettings());
	wire(ui.checkBoxSpan,toggled(bool),updateRowSettings());
	wire(ui.spinBoxSpanFrom,valueChanged(int),updateRowSettings());
	wire(ui.spinBoxSpanTo,valueChanged(int),updateRowSettings());

	wire(ui.comboBoxEndBorder,SIGNAL(currentIndexChanged(int)),updateTableWidget());

	wire(ui.tableWidget,cellClicked(int,int),showColRowSettings(int,int));
	wire(ui.tableWidget,currentCellChanged(int,int,int,int),showColRowSettings(int,int));

	setWindowTitle(tr("Quick Tabular"));
	updateTableWidget();
}


TabDialog::~TabDialog(){}


/*!
 * Return the LaTeX formatted text describing the table.
 */

QString TabDialog::getLatexText(){

	QString placeholder;//(0x2022);

	int ncols = ui.spinBoxColumns -> value();
	int nrows = ui.spinBoxRows -> value();

	QString text = "\\begin{tabular}{";

	for(int j = 0;j < ncols;j++){
		text += borderlist.at(colDataList.at(j).leftborder);
		text += alignlist.at(colDataList.at(j).alignment);
	}

	text += borderlist.at(ui.comboBoxEndBorder -> currentIndex());
	text += "}\n";

	QTableWidgetItem * item = nullptr;
	
	for(int i = 0;i < nrows;i++){

		if(liDataList.at(i).topborder)
			text += "\\hline\n";
		
		if(ui.checkBoxMargin -> isChecked())
			text += "\\rule[-1ex]{0pt}{2.5ex} ";
		
		if(liDataList.at(i).merge && (liDataList.at(i).mergeto > liDataList.at(i).mergefrom)){

			QString el = "";
			
			for(int j = 0;j < ncols;j++){

				item = ui.tableWidget -> item(i,j);
				
				QString itemText = (item) 
					? textToLatex(item -> text()) 
					: "";
				
				if(j == liDataList.at(i).mergefrom - 1){

					el += itemText;
					
					text += "\\multicolumn{";
					text += QString::number(liDataList.at(i).mergeto - liDataList.at(i).mergefrom + 1);
					text += "}{";
					
					if((j == 0) && (colDataList.at(j).leftborder < 2))
						text += borderlist.at(colDataList.at(j).leftborder);
					
					if(colDataList.at(j).alignment < 3)
						text += alignlist.at(colDataList.at(j).alignment);
					else 
						text += "c";
					
					if(liDataList.at(i).mergeto == ncols)
						text += borderlist.at(ui.comboBoxEndBorder -> currentIndex());
					else
						text += borderlist.at(colDataList.at(liDataList.at(i).mergeto).leftborder);
					
					text += "}{";
				} else if(j == liDataList.at(i).mergeto - 1){
					
					el += itemText;
					
					if(el.isEmpty())
						el = placeholder;
					
					text += el + "}";
					
					if(j < ncols - 1)
						text += " & ";
					else 
						text += " \\\\\n";
				} else if((j > liDataList.at(i).mergefrom - 1) && (j < liDataList.at(i).mergeto - 1)){
					el += itemText;
				} else {

					if(itemText.isEmpty())
						itemText = placeholder;

					text += itemText;

					if(j < ncols - 1)
						text += " & ";
					else
						text += " \\\\\n";
				}

			}
		} else {

			for(int j = 0;j < ncols - 1;j++){

				item = ui.tableWidget -> item(i,j);
				
				QString itemText = (item) 
					? textToLatex(item -> text()) 
					: "";
				
				if(itemText.isEmpty())
					itemText = placeholder;

				text += itemText + " & ";
			}

			item = ui.tableWidget -> item(i,ncols - 1);
			
			QString itemText = (item) 
				? textToLatex(item -> text()) 
				: "";
			
			if(itemText.isEmpty())
				itemText = placeholder;

			text += itemText + " \\\\\n";
		}
	}

	
	text += (ui.checkBoxBorderBottom -> isChecked())
		? "\\hline\n\\end{tabular}"
		: "\\end{tabular}";
	
	return text;
}


/*!
 * Return a list of packages the given latex table code depends upon.
 */

QStringList TabDialog::getRequiredPackages(const QString & text){

	QStringList requiredPackages;
	
	if(text.contains("arraybackslash"))
		requiredPackages << "array";

	return requiredPackages;
}


void TabDialog::NewRows(int num){

	ui.tableWidget -> setRowCount(num);
	ui.spinBoxNumLi -> setRange(1,num);

	updateTableWidget();
}


void TabDialog::NewColumns(int num){

	ui.tableWidget -> setColumnCount(num);
	ui.spinBoxNumCol -> setRange(1,num);
	
	if(num > 1){
		ui.spinBoxSpanFrom -> setRange(1,num - 1);
		ui.spinBoxSpanTo -> setRange(2,num);
	} else {
		ui.spinBoxSpanFrom -> setRange(1,num - 1);
		ui.spinBoxSpanTo -> setRange(2,num);
	}

	updateTableWidget();
}


void TabDialog::updateSpanStatus(bool enabled){

	ui.spinBoxSpanFrom -> setEnabled(enabled);
	ui.spinBoxSpanTo -> setEnabled(enabled);
	
	updateTableWidget();
}


void TabDialog::applytoAllColumns(){

	colData col;
	col.alignment = ui.comboBoxColAl -> currentIndex();
	col.leftborder = ui.comboLeftBorder -> currentIndex();
	
	for (int i = 0;i < 99;++i)
		colDataList.replace(i,col);

	updateTableWidget();
}


void TabDialog::applytoAllLines(){

	liData li;
	li.topborder = ui.checkBoxBorderTop -> isChecked();
	li.merge = ui.checkBoxSpan -> isChecked();
	li.mergefrom = ui.spinBoxSpanFrom -> value();
	li.mergeto = ui.spinBoxSpanTo -> value();
	
	if(li.mergefrom > li.mergeto){
		li.mergefrom = 1;
		li.mergeto = 1;
	}

	for (int i = 0;i < 99;++i)
		liDataList.replace(i,li);

	updateTableWidget();
}


void TabDialog::updateColSettings(){

	int i = ui.spinBoxNumCol -> value() - 1;

	colData col;
	col.alignment = ui.comboBoxColAl -> currentIndex();
	col.leftborder = ui.comboLeftBorder -> currentIndex();
	
	colDataList.replace(i,col);
	
	updateTableWidget();
	//qDebug() << "change" << i << colDataList.at(i).alignment << colDataList.at(i).leftborder;
}


void TabDialog::updateRowSettings(){

	int i = ui.spinBoxNumLi -> value() - 1;

	liData li;
	li.topborder = ui.checkBoxBorderTop -> isChecked();
	li.merge = ui.checkBoxSpan -> isChecked();
	li.mergefrom = ui.spinBoxSpanFrom -> value();
	li.mergeto = ui.spinBoxSpanTo -> value();
	
	if(li.mergefrom > li.mergeto){
		li.mergefrom = 1;
		li.mergeto = 1;
	}

	liDataList.replace(i,li);
	updateTableWidget();
}


void TabDialog::showColSettings(int column){

	int i = column - 1;

	if(i >= 99)
		return;
	
	unwire(ui.comboBoxColAl,currentIndexChanged(int),updateColSettings());
	unwire(ui.comboLeftBorder,currentIndexChanged(int),updateColSettings());
	
	ui.comboBoxColAl -> setCurrentIndex(colDataList.at(i).alignment);
	ui.comboLeftBorder -> setCurrentIndex(colDataList.at(i).leftborder);
	
	
	wire(ui.comboBoxColAl,currentIndexChanged(int),updateColSettings());
	wire(ui.comboLeftBorder,currentIndexChanged(int),updateColSettings());
	
	updateTableWidget();
}


void TabDialog::showRowSettings(int row){

	int i = row - 1;

	if(i >= 99)
		return;
	
	unwire(ui.checkBoxBorderTop,toggled(bool),updateRowSettings());
	unwire(ui.checkBoxSpan,toggled(bool),updateRowSettings());
	unwire(ui.spinBoxSpanFrom,valueChanged(int),updateRowSettings());
	unwire(ui.spinBoxSpanTo,valueChanged(int),updateRowSettings());
	
	ui.checkBoxBorderTop -> setChecked(liDataList.at(i).topborder);
	ui.checkBoxSpan -> setChecked(liDataList.at(i).merge);
	ui.spinBoxSpanFrom -> setValue(liDataList.at(i).mergefrom);
	ui.spinBoxSpanTo -> setValue(liDataList.at(i).mergeto);
	
	wire(ui.checkBoxBorderTop,toggled(bool),updateRowSettings());
	wire(ui.checkBoxSpan,toggled(bool),updateRowSettings());
	wire(ui.spinBoxSpanFrom,valueChanged(int),updateRowSettings());
	wire(ui.spinBoxSpanTo,valueChanged(int),updateRowSettings());
	
	updateTableWidget();
}


void TabDialog::showColRowSettings(int row,int column){
	ui.spinBoxNumLi -> setValue(row + 1);
	ui.spinBoxNumCol -> setValue(column + 1);
}


void TabDialog::updateTableWidget(){

	int nrows = ui.spinBoxRows -> value();
	int ncols = ui.spinBoxColumns -> value();
	
	QStringList headerList;
	QString tag = "";

	for(int j = 0;j < ncols;j++){

		tag = borderlist.at(colDataList.at(j).leftborder);
		tag += alignlistLabels.at(colDataList.at(j).alignment);

		if(j < ncols - 1)
			headerList.append(tag);
	}

	tag += borderlist.at(ui.comboBoxEndBorder -> currentIndex());

	headerList.append(tag);
	ui.tableWidget -> setHorizontalHeaderLabels(headerList);
	
	QTableWidgetItem * item , * new_item;
	QString content;
	
	for (int i = 0;i < nrows;i++){
		for (int j = 0;j < ncols;j++){

			item = ui.tableWidget -> item(i,j);

			if (item){
				content = item -> text();
				new_item = new QTableWidgetItem();
				new_item -> setText(content);
			} else {
				new_item = new QTableWidgetItem();
				new_item -> setText("");
			}

			if(alignlist.at(colDataList.at(j).alignment).contains("l"))
				new_item -> setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			else
			if(alignlist.at(colDataList.at(j).alignment).contains("r"))
				new_item -> setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			else
				new_item -> setTextAlignment(Qt::AlignCenter);

			if(ui.tableWidget -> columnSpan(i,j) > 1)
				ui.tableWidget -> setSpan(i,j,1,1);

			ui.tableWidget -> setItem(i,j,new_item);
		}

		if((liDataList.at(i).merge) && (liDataList.at(i).mergeto > liDataList.at(i).mergefrom))
			ui.tableWidget -> setSpan(i,liDataList.at(i).mergefrom - 1,1,liDataList.at(i).mergeto - liDataList.at(i).mergefrom + 1);
	}
}

#undef wire
#undef unwire
