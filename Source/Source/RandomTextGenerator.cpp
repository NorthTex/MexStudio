
#include "randomtextgenerator.h"
#include "latexparser/latexreader.h"
#include "Include/UtilsUI.hpp"
#include "ui_randomtextgenerator.h"
#include "Include/UtilsUI.hpp"


RandomTextGenerator::RandomTextGenerator(QWidget * widget,const QStringList & lines)
	: QDialog(widget)
	, ui(new Ui::RandomTextGenerator)
	, lines(lines) {

	ui -> setupUi(this);
	UtilsUi::resizeInFontHeight(this,41,39);

	connect(ui -> generateButton,SIGNAL(clicked()),this,SLOT(generateText()));
	connect(ui -> latexInput,SIGNAL(toggled(bool)),SLOT(resetWords()));
	connect(ui -> punctationCheckBox,SIGNAL(toggled(bool)),SLOT(resetWords()));
	connect(ui -> upperCaseCheckBox,SIGNAL(toggled(bool)),SLOT(resetWords()));
}


RandomTextGenerator::~RandomTextGenerator(){
	delete ui;
}


void RandomTextGenerator::changeEvent(QEvent * event){

	switch(event -> type()){
	case QEvent::LanguageChange:
		ui -> retranslateUi(this);
		return;
	}
}


int myrand(int max){
    return std::rand() % max;
}


std::tuple<bool,int> RandomTextGenerator::findConfig() const {

	if(ui -> wordOrder1RadioButton -> isChecked())
		return { true , 1 };

	if(ui -> wordOrder2RadioButton -> isChecked())
		return { true , 2 };

	if(ui -> wordOrder3RadioButton -> isChecked())
		return { true , 3 };
		
	if(ui -> wordOrderXRadioButton -> isChecked()){

		const auto order = ui 
			-> wordOrderSpinBox 
			-> value();

		return { true , order };
	}


	if(ui -> characterOrder1RadioButton -> isChecked())
		return { false , 1 };

	if(ui -> characterOrder2RadioButton -> isChecked())
		return { false , 2 };

	if(ui -> characterOrder3RadioButton -> isChecked())
		return { false , 3 };

	if(ui -> characterOrderXRadioButton -> isChecked()){

		const auto order = ui 
			-> characterOrderSpinBox
			-> value();

		return { false , order };
	}


	return { true , -1 };
}


const auto
	message_reading =
		"Reading all words\n"
		"(This will take a while but only on the first generation)",

	message_contentNeeded = 
		"The current document contains no words, but we need " 
		"some phrases as a base to create the random text from";


void RandomTextGenerator::generateText(){

	const auto doExport = ui 
		-> exportCheckBox 
		-> isChecked();

	const auto exportName = ui
		-> exportFileNameLineEdit
		-> text();

	//---------------------------reading all words and characters in words-------------------
	
	if(words.empty()){

		if(lines.empty()){
			
			ui
				-> outputEdit
				-> setText(tr("No data given"));
			
			return;
		}
		
		ui
			-> outputEdit
			-> setText(tr(message_reading));
		
		QApplication::processEvents();
		
		words.clear();
		chars.clear();

		const bool 
			
			upcase = ui
				-> upperCaseCheckBox
				-> isChecked(),
			
			punctation = ui
				-> punctationCheckBox
				-> isChecked(),

			useLatex = ui
				-> latexInput
				-> isChecked();


		static const QString Punctation = ".,:;!?";

		const auto processLatex = [ & ](const QString & line){

			int
				wordStartIndex = 0,
				lastIndex = 0,
				index = 0;

			QString outWord;
			LatexReader reader(line);
			
			while(reader.nextTextWord()){

				if(upcase)
					outWord = outWord.toUpper();
				
				if(punctation)
					for(int i = lastIndex;i < wordStartIndex;i++)
						if(Punctation.indexOf(line[i]) >= 0)
							outWord += line[i];

				words << outWord;
				chars += outWord + " ";
				lastIndex = index;
			}
		};


		const auto processGeneric = [ & ](const QString & line){

			const auto splitAt = punctation 
				? QRegularExpression("\\s+") 
				: QRegularExpression("[~!@#$%^&*()_+{}|:\"\\<>?,./;[-= \t'+]");
			
			auto newlist = line.split(splitAt,Qt::SkipEmptyParts);
			
			if(upcase)
				for(int i = 0;i < newlist.size();i++)
					newlist[i] = newlist[i].toUpper();
		
			words << newlist;
		};


		for(const auto & line : lines)
			if(useLatex)
				processLatex(line);
			else
				processGeneric(line);

		if(words.empty()){
			
			ui
				-> outputEdit
				-> setText(tr(message_contentNeeded));
			
			return;
		}
	}


	//----------------------------------generating ---------------------------------------
	//like Shannon in "A Mathematical Theory of Communication" (1949)

	int amount = ui
		-> lengthSpinBox
		-> value();

	const auto [ useWords , order ] = findConfig();


	if(order <= 0){
		
		ui 
			-> outputEdit
			-> setText(tr("You didn't select an order!"));
		
		return;
	}

	ui
		-> outputEdit
		-> setText(tr("Generating random text..."));
	
	QApplication::processEvents();

	//TODO: milliseconds
    std::srand(QDateTime::currentDateTime().toSecsSinceEpoch());

	text = "";
	QFile file;

	auto newWordFound = & RandomTextGenerator::newWordForText;

	if(doExport){

		newWordFound = & RandomTextGenerator::newWordForStream;
		
		file.setFileName(exportName);
		
		if(!file.open(QFile::WriteOnly)){
			UtilsUi::txsWarning(tr("Couldn't create file %1").arg(exportName));
			return;
		}

		textStream.setDevice(&file);
		
		ui 
			-> outputEdit
			-> setText(tr("Generating random text..."));
	}

	if(useWords){

		//----------generate with words ------------------
		
		QMultiHash<int,int> startingIndices;
		QHash<int,QString> idToWord;
		QHash<QString,int> wordToId;
		QList<int> wordsIds;
		
		int totalIds = 0;
		
		for(int i = 0;i < words.size();i++){
			
			int id = wordToId.value(words[i],-1);
			
			if(id == -1){
				totalIds++;
				id = totalIds;
				wordToId.insert(words[i],totalIds);
				idToWord.insert(totalIds,words[i]);
			}
            
			startingIndices.insert(id,i);
			wordsIds << id;
		}

		QString text;
		QList<int> possibleMatches;

		auto last = QList<int>() 
			<< wordToId.value(words[myrand(words.size())]);
		
		
		for(int n = 1; n < amount; n++){
			
			if(last.size() == order)
				last.removeFirst();
			
			possibleMatches.clear();
			
			if(last.size() == 0)
				last << wordsIds[myrand(wordsIds.size())];
			else {

				//search possible extensions and choose one of them at random

				auto iterator = startingIndices.find(last.first());
				
				while(iterator != startingIndices.end() && iterator.key() == last.first()) {
					
					if(iterator.value() + last.size() >= wordsIds.size()){
						++iterator;
						continue;
					}

					bool found = true;
					
					for(int j = 1;j < last.size();j++)
						if(wordsIds[iterator.value() + j] != last[j]){
							found = false;
							break;
						}

					if(found)
						possibleMatches << (iterator.value() + last.size());
					
					++iterator;
				}

				if(possibleMatches.empty())
					last = QList<int>() << wordsIds[myrand(wordsIds.size())];
				else
					last << wordsIds[possibleMatches[myrand(possibleMatches.size())]];
			}

			const auto word = idToWord.value(last.last()) +
				(myrand(15) == 0 ? "\n" : " ");

			(this ->* newWordFound)(word);
		}
	} else {

		//----------generate with characters--------------
		
		QString text;
		QString last = chars.at(myrand(chars.size()));
		
		for(int n = 1;n < amount;n++){

			if(last.size() == order)
				last.remove(0, 1);
			
			int position = myrand(chars.size());

			//choose a random position and search next possible extension
			//(faster than the method used by words, but statistically not so sound,
			//should work best with infinite large texts)
			
			int foundPos = -1;
			
			for(int i = position;i < chars.size() - last.size();i++)
				if(chars.mid(i,last.size()) == last){
					foundPos = i + last.size();
					break;
				}

			if(foundPos == -1){
				
				for(int i = 0;i < position - last.size();i++)
					if(chars.mid(i,last.size()) == last){
						foundPos = i + last.size();
						break;
					}
				
				if(foundPos == -1){
					last = "";
					foundPos = myrand(chars.size());
				}
			}

			const QString & c = chars.at(foundPos);
			last += c;
			
			(this ->* newWordFound)(c);
		}
	}

	if(doExport){
		ui -> outputEdit -> setText(tr("Finished generation"));
		textStream.setDevice(nullptr);
	} else {
		ui -> outputEdit -> setText(text);
	}
}


void RandomTextGenerator::resetWords(){
	words.clear();
}


void RandomTextGenerator::newWordForText(const QString & word){
	
	text += word;
	
	ui 
		-> outputEdit
		-> setText(tr("Generating random text...") + "\n\n" + text);
	
	QApplication::processEvents();
}


void RandomTextGenerator::newWordForStream(const QString & word){
	textStream << word;
}

