#include "latexcompleter_p.h"
#include "latexcompleter_config.h"

#include "usermacro.h"

#include "smallUsefulFunctions.h"

#include "qdocumentline.h"
#include "qdocument_p.h"
#include "qeditorinputbinding.h"
#include "qformatfactory.h"
#include "qdocumentline_p.h"

#include "qdocument.h"

#include "Latex/Reference.hpp"
#include "Latex/EditorView.hpp"
#include "Latex/Completer.hpp"
#include "Latex/Document.hpp"

#include <algorithm>
#include <QtConcurrentFilter>
#include <QtConcurrentMap>


//------------------------------Default Input Binding--------------------------------

class CompleterInputBinding : public QEditorInputBinding {

	private:

		bool 
			active,
			showAlways;
	
		int showMostUsed;
		
		LatexCompleter * completer;
		QEditor * editor;
		QEditorInputBindingInterface * oldBinding;
		QString completionWord;

		int 
			curStart, 
			maxWritten,
			curLineNumber;
	
	public:

		QString autoOverridenText;

	public:

		CompleterInputBinding()
			: active(false)
			, showAlways(false)
			, showMostUsed(0)
			, completer(nullptr)
			, editor(nullptr)
			, oldBinding(nullptr)
			, curStart(0)
			, maxWritten(0)
			, curLineNumber(0) {}

	public:
	

		virtual QString id() const {
			return "TXS::CompleterInputBinding";
		}

		virtual QString name() const {
			return "TXS::CompleterInputBinding";
		}

		virtual bool mousePressEvent(QMouseEvent * mouse,QEditor * editor){
			
			Q_UNUSED(mouse)
			Q_UNUSED(editor)
			
			simpleRestoreAutoOverride();
			resetBinding();
			
			return false;
		}

		QString getCurWord(){

			if(editor)
				return editor 
					-> text(curLineNumber)
					.  mid(curStart,editor -> cursor().columnNumber() - curStart);
			
			return "";
		}


		/// check if current cursor/placeholder is mirrored

		bool isMirrored(){

			if(!editor)
				return false;

			const auto index = editor 
				-> currentPlaceHolder();
			
			if(index < 0)
				return false;

			if(index >= editor -> placeHolderCount())
				return false;

			const auto placeholder = editor 
				-> getPlaceHolder(index);
			
			return placeholder.mirrors.count() > 0;
		}


		void insertText(const QString & text){

			if(!editor)
				return;

			maxWritten += text.length();

			const auto index = editor -> currentPlaceHolder();

			if(
				index >= 0 && 
				index < editor -> placeHolderCount()
			)	editor -> document() -> beginMacro();

			editor -> write(text);
			
			//cursor mirrors
			
			if(
				index >= 0 &&
				index < editor -> placeHolderCount()
			){
				auto placeholder = editor -> getPlaceHolder(editor->currentPlaceHolder());
				auto baseText = placeholder.cursor.selectedText();

				for(auto & mirror : placeholder.mirrors)
					mirror.replaceSelectedText(baseText);

				editor 
				-> document() 
				-> endMacro();
			}

			//end cursor mirrors

			if(
				editor -> cursor().columnNumber() > curStart + 1 && 
				! completer -> isVisible()
			){
				const auto word = getCurWord();
			
				completer -> filterList(word,showMostUsed);
				completer -> widget -> show();
			
				// if prefered list is empty, take next more extensive one

				if(completer -> countWords() == 0)
					switch(showMostUsed){
					break;case 1 : // Typical
						completer -> setTab(0);
					break;case 3 : // All
						completer -> setTab(3);
					}

				completer -> adjustWidget();
			}
		}


		bool insertCompletedWord(){

			if(!editor)
				return false;
			
			if(
				completer -> list -> isVisible() && 
				maxWritten >= curStart && 
				completer -> list -> currentIndex().isValid()
			){
				auto cursor = editor -> cursor();
				
				editor -> document() -> beginMacro();

				QVariant v = completer 
					-> list 
					-> model() 
					-> data(completer -> list -> currentIndex(),Qt::DisplayRole);
				
				if( ! v.isValid())
					return false;
					
				if( ! v.canConvert<CompletionWord>())
					return false;
				
				auto completion = v.value<CompletionWord>();

				completer -> listModel -> incUsage(completer -> list -> currentIndex());
				
				//remove current text for correct case

				if(completer -> forcedRef){

					QString line = cursor.line().text();
					int col = cursor.columnNumber();
					bool missingCloseBracket = (findClosingBracket(line, col) < 0);
					
					QString eow = "}],";
					
					// spaces are allowed in labels and should be deleted, 
					// however we stop deleting at spaces if the closing bracket is missing. 
					// otherwise it deletes the complete line.

					while(
						! cursor.atLineEnd() && 
						! eow.contains(cursor.nextChar()) && 
						! (cursor.nextChar().isSpace() && missingCloseBracket)
					) cursor.deleteChar();
				}

				if(completer -> forcedCite){

					QString line = cursor.line().text();
					int col = cursor.columnNumber();
					bool missingCloseBracket = (findClosingBracket(line,col) < 0);
					QString eow = "},";
					
					while(
						! cursor.atLineEnd() && 
						! eow.contains(cursor.nextChar()) && 
						! (cursor.nextChar().isSpace() && missingCloseBracket)
					) cursor.deleteChar();
				}

				if(
					isMirrored() && (	
						completion.lines.first().startsWith("\\begin") || 
						completion.lines.first().startsWith("\\end")
					)
				){
					QString text = completion.lines.first();
					int i = cursor.columnNumber() - curStart;
					text.remove(0, i);
					i = text.indexOf('}');
					
					if(i >= 0)
						text.remove(i, text.length());
				
					while(
						! cursor.atLineEnd() && 
						cursor.nextChar() != '}'
					)	cursor.deleteChar();

					insertText(text);
					editor -> document() -> endMacro();

					return true;
				}

				// check whether cursor is inside math in case of automatic delimter insertion
				// this is done here as the charcter format is used for detection and here we are sure that at least 1 character was used.
				
				QString cwCmd=completion.word;
				QRegExp rx("\\\\[a-zA-Z]+");
				int pos=rx.indexIn(cwCmd);
				
				if(pos > -1)
					cwCmd = rx.cap(0);

				bool inMath = false;

				if(
					completion.lines.size() == 1 && 
					completer -> latexParser.possibleCommands["math"].contains(cwCmd)
				){
					auto view = editor 
						-> property("latexEditor")
						.  value<LatexEditorView *>();
					
					Q_ASSERT(view);
					
					inMath = view -> isInMathHighlighting(cursor);
				}

				if(maxWritten>cursor.columnNumber()){
					cursor.movePosition(maxWritten-cursor.columnNumber(),QDocumentCursor::NextCharacter,QDocumentCursor::KeepAnchor);
					cursor.removeSelectedText();
				}

				if(curStart<cursor.columnNumber()){
					cursor.movePosition(cursor.columnNumber()-curStart,QDocumentCursor::PreviousCharacter,QDocumentCursor::KeepAnchor);
					cursor.removeSelectedText();
				}

				if(!autoOverridenText.isEmpty()){
					cursor.insertText(autoOverridenText);
					cursor.movePosition(autoOverridenText.length(), QDocumentCursor::PreviousCharacter);
					editor->resizeAutoOverridenPlaceholder(cursor, autoOverridenText.size());
					autoOverridenText = "";
				}

				auto phMode = (LatexCompleter::config && LatexCompleter::config -> usePlaceholders) 
					? CodeSnippet::PlacehodersActive 
					: CodeSnippet::PlaceholdersRemoved;

				if(completion.lines.size()==1 && completer->latexParser.possibleCommands["math"].contains(cwCmd)){
					if(!inMath && LatexCompleter::config && LatexCompleter::config->autoInsertMathDelimiters){
						// add $$ to mathcommand outsiode math env
						completion.lines.first().prepend(LatexCompleter::config->startMathDelimiter);
						completion.lines.first().append(LatexCompleter::config->stopMathDelimiter);
						// move cursors
						if(completion.cursorOffset>-1) completion.cursorOffset++;
						if(completion.anchorOffset>-1) completion.anchorOffset++;
						for(int i=0;i<completion.placeHolders.size();i++){
							for(int j=0;j<completion.placeHolders[i].size();j++){
								completion.placeHolders[i][j].offset++;
							}
						}
					}
				}

				completion.insertAt(editor,& cursor,phMode,! completer -> startedFromTriggerKey,completer -> forcedKeyval);
				editor -> document() -> endMacro();

				return true;
			}

			return false;
		}


		void removeRightWordPart(){

			if(!editor)
				return;
			
			auto cursor = editor->cursor();
			
			for(int i = maxWritten - cursor.columnNumber();i > 0;i--)
				cursor.deleteChar();
			
			maxWritten = cursor.columnNumber();
			
			//necessary to keep the cursor at the same place (but why???)  
			// TODO: remove this line (it cause \ to disable placeholders which other keys don't disable)
			editor -> setCursor(cursor);
		}


		///selects an index in the completion suggestion list

		void select(const QModelIndex & index){

			if(!completer)
				return;
			
			if(!completer -> list)
				return;
			
			completer -> list -> setCurrentIndex(index);
			completer -> selectionChanged(index);
		}


		///moves the selection index to the next/previous delta-th entry in the suggestion list
		
		bool selectDelta(const int delta){

			if(!completer){
				resetBinding();
				return false;
			}

			const auto list = completer -> list;

			if(!list || !list -> isVisible()){
				resetBinding();
				return false;
			}

			const auto model = list -> model();


			auto index = model -> index(list -> currentIndex().row() + delta,0,QModelIndex());


			if(index.isValid()){
				select(index);
				return true;
			}

			if(delta == 0)
				return true;
				
			auto start = 0;

			if(delta > 0)
				start = model -> rowCount() - 1;

			index = model -> index(start,0,QModelIndex());
			
			return true;
		}


		/*!
		* Complete the common part of all remaining possible completions.
		* \return true if there was a common part, that could be completed.
		*/

		bool completeCommonPrefix(bool reducedRange = false){

			QString my_curWord = getCurWord();
			
			if(my_curWord.isEmpty())
				return false;
			
			if(!completer)
				return false;

			// get list of most recent choices

			const auto & words = completer
				-> listModel
				-> getWords();

			// filter list for longest common characters

			if(words.count() > 1){

				QString myResult = words.first().word;
				int curWordLength = my_curWord.length();
				
				my_curWord = completer 
					-> listModel 
					-> getLastWord().word;

				if(reducedRange && words.count() > 10)
					my_curWord = words.at(10).word;

				if(showMostUsed == 2){

					// as the list is unsorted, all words must be checked !!!
					
					int j = 0;
					bool allIdentical = false;
					
					for(j = 0;(j < my_curWord.length() && j < myResult.length());j++){
						for(const auto &cw : words){

							if(cw.word.length()<=j){
								allIdentical=false;
								break;
							}

							if(cw.word[j]!=myResult[j]){
								allIdentical=false;
								break;
							}

							allIdentical=true;
						}

						if(!allIdentical)
							break;
					}

					if(!allIdentical)
						j -= 1;

					if(j <= 0)
						return true;
					
					myResult = myResult.left(j);
				} else
					for(int j = curWordLength;j < my_curWord.length() && j < myResult.length();j++)
						if(myResult[j] != my_curWord[j])
							myResult = myResult.left(j);

				// no common segment to complete

				if(myResult.length() == curWordLength)
					return false;  

				removeRightWordPart();

				if(showMostUsed == 2){

					// fuzzy mode
					// remove left hand side as well
					QDocumentCursor cursor = editor->cursor();
					for (int i = curWordLength; i > 0; i--) cursor.deletePreviousChar();
					maxWritten = cursor.columnNumber();
					editor->setCursor(cursor);
					curWordLength=0;
				}

				insertText(myResult.right(myResult.length() - curWordLength));
				
				completer->filterList(getCurWord(), getMostUsed());
				
				if( ! completer -> list -> currentIndex().isValid())
					select(completer -> list -> model() -> index(0,0,QModelIndex()));
				
				return true;
			} else {

				if(showMostUsed == 1)
					return false;
				
				insertCompletedWord();
				resetBinding();
				
				return true;
			}
		}


		//simple means without protecting the change from undo/redo

		void simpleRestoreAutoOverride(const QString & written = "????"){
			
			if(!editor)
				return;
			
			if( ! autoOverridenText.isEmpty() && ! editor -> isAutoOverrideText(written)){

				int curpos = editor -> cursor().columnNumber();
				
				if(curpos < maxWritten){
					auto cursor = editor -> cursor();
					cursor.movePosition(maxWritten - curpos,QDocumentCursor::NextCharacter);
					editor -> setCursor(cursor);
				}
				
				editor -> insertText(autoOverridenText);
				
				auto cursor = editor -> cursor();
				
				cursor.movePosition(
					autoOverridenText.length() + (
						(curpos < maxWritten) ? maxWritten - curpos : 0
					),QDocumentCursor::PreviousCharacter
				);
				
				editor -> setCursor(cursor);
				editor -> resizeAutoOverridenPlaceholder(cursor,autoOverridenText.size());
			}
		}


		virtual bool keyPressEvent(QKeyEvent * event,QEditor * editor){
			
			Q_ASSERT(completer && completer -> listModel);
			
			if(!completer)
				return false;
				
			if(!completer -> listModel)
				return false;

			const auto key = event -> key();

			switch(key){
			case Qt::Key_Shift:
			case Qt::Key_Alt:
			case Qt::Key_AltGr:
			case Qt::Key_Control:
			case Qt::Key_CapsLock:
				return false;
			}

			//we should never have been called

			if(!active)
				return false; 

			bool handled = false;

			switch(key){
			case Qt::Key_Backspace : {

				maxWritten--;

				auto handle = editor -> cursorHandle();

				Q_ASSERT(handle);
				
				handle -> deletePreviousChar();

				if(handle -> columnNumber() <= curStart){
					resetBinding();
					return true;
				}
				
				if(handle -> columnNumber() + 1 <= curStart && ! showAlways){
					completer -> widget -> hide();
					return true;
				}

				handled = true;

				break;
			}
			case Qt::Key_Escape:
				
				simpleRestoreAutoOverride("");
				resetBinding();

				return true;
			case Qt::Key_Delete:

				if(editor -> cursor().columnNumber() < maxWritten)
					maxWritten--;
			
				if(completer -> forcedRef)
					if(editor -> cursor().nextChar() == '}')
						completer -> forcedRef = false;

				editor -> cursor().deleteChar();

				handled = true;

				break;
			case Qt::Key_Left:

				if(event -> modifiers()){
					resetBinding();
					return false;
				}

				editor -> setCursorPosition(curLineNumber,editor -> cursor().columnNumber() - 1);
				
				if(editor -> cursor().columnNumber() <= curStart)
					resetBinding();

				handled = true;

				break;
			case Qt::Key_Right:

				if(event -> modifiers()){
					resetBinding();
					return false;
				}

				editor -> setCursorPosition(curLineNumber,editor -> cursor().columnNumber() + 1);
				
				if(editor -> cursor().columnNumber() > maxWritten)
					resetBinding();

				handled = true;

				break;
			case Qt::Key_Up:
				return selectDelta(-1);
			case Qt::Key_Down:
				return selectDelta(+1);
			case Qt::Key_PageUp:
				return  selectDelta(-5);
			case Qt::Key_PageDown:
				return  selectDelta(+5);
			case Qt::Key_Home : {

				if(!completer -> list -> isVisible()){
					resetBinding();
					return false;
				}

				auto index = completer
					-> list
					-> model()
					-> index(0,0,QModelIndex());
				
				if(index.isValid())
					select(index);
				
				return true;
			}
			case Qt::Key_End : {

				if(!completer -> list -> isVisible()){
					resetBinding();
					return false;
				}

				auto index = completer
					-> list
					-> model()
					-> index(completer -> list -> model() -> rowCount() - 1,0,QModelIndex());
				
				if(index.isValid())
					select(index);

				return true;
			}
			case Qt::Key_Tab:

				if(completeCommonPrefix(true))
					return true;
				
				if(insertCompletedWord()){
					resetBinding();
					return true;
				}

				return false;
			default: {
			
					if(key == Qt::Key_Space && event -> modifiers() == Qt::ShiftModifier){

						showMostUsed++;

						if(showMostUsed > 3)
							showMostUsed = 0;

						completer -> tbAbove -> setCurrentIndex(showMostUsed);
						completer -> tbBelow -> setCurrentIndex(showMostUsed);

						return true;
					}

					switch(key){
					case Qt::Key_Return:
					case Qt::Key_Enter:

						insertCompletedWord();
						resetBinding();

						return true;
					}

					if(event -> text().length() != 1 || event -> text() == " "){
						resetBinding();
						return false;
					}

					QChar written = event -> text().at(0);

					if(written == '\\'){

						if(getCurWord() == "\\"){
							resetBinding();
							return false;
						}

						if(getCurWord() == ""){
							maxWritten = curStart + 1;
						} else {

							if(LatexCompleter::config && LatexCompleter::config -> eowCompletes)
								insertCompletedWord();

							auto edc = editor -> cursor();

							if(edc.hasSelection()){
								edc.removeSelectedText();
								editor -> setCursor(edc);
							}

							curStart = edc.columnNumber();
							maxWritten = curStart + 1;
						}

						bool autoOverride = editor -> isAutoOverrideText("\\");

						if(autoOverride){
							auto cursor = editor -> cursor();
							editor -> document() -> beginMacro();
							cursor.deleteChar();
							cursor.insertText("\\");
							editor -> document() -> endMacro();
							autoOverridenText = '\\';
						} else {
							editor -> cursor().insertText(written);
						}

						handled = true;
					} else
					if(completer -> acceptChar(written,editor -> cursor().columnNumber() - curStart)){
						insertText(written);
						handled = true;
					} else
					if(event -> text().length() == 1 && getCommonEOW().contains(event -> text().at(0))){

						QString curWord = getCurWord();

						if(
							curWord == "\\" ||
							! LatexCompleter::config ||
							! LatexCompleter::config -> eowCompletes
						){
							resetBinding();
							simpleRestoreAutoOverride(written);

							return false;
						}

						const auto & words = completer -> listModel -> getWords();

						QString newWord;
						int eowchars = 10000;

						for(const auto & word : words){

							if(!word.word.startsWith(curWord))
								continue;

							if(word.word.length() == curWord.length()){
								newWord = word.word;
								break;
							}

							int newoffset = word.lines.first().indexOf(written,curWord.length());

							if(newoffset < 0)
								continue;

							int neweowchars = 0;

							for(int i = curWord.length();i < newoffset;i++)
								if(getCommonEOW().contains(word.lines.first()[i]))
									neweowchars++;

							if(neweowchars < eowchars){
								newWord = word.word;
								eowchars = neweowchars;

								if(eowchars == 1)
									break;
							}
						}

						if(!newWord.isEmpty() && newWord.length() != curWord.length()){
							QString insertion = newWord.mid(curWord.length(), newWord.indexOf(written, curWord.length()) - curWord.length() + 1); //choose text until written eow character
							insertText(insertion);
							handled = true;
						}

						if(!handled){

							insertCompletedWord();

							if (newWord.isEmpty())
								simpleRestoreAutoOverride(written);

							resetBinding();
							return false;
						}
					} else {

						int curLength = getCurWord().length();

						resetBinding();

						//call old input binding for long words
						// (=> key replacements after completions, but the user can still write \")

						return
							curLength >= 2 &&
							oldBinding -> keyPressEvent(event,editor);
					}
				}
			}

			completer -> filterList(getCurWord(),showMostUsed);
			
			if(!completer -> list -> currentIndex().isValid())
				select(completer -> list -> model() -> index(0,0,QModelIndex()));
			
			return handled;
		}
		

		virtual bool inputMethodEvent(QInputMethodEvent * event,QEditor * editor){

			Q_ASSERT(completer && completer -> listModel);
			
			if(!completer)
				return false;
				
			if(!completer->listModel)
				return false;
			
			if(event -> commitString().isEmpty())
				return false;

			bool handled = false;
			
			QString text = event -> commitString();
			
			if(text.length() != 1 || text == " "){
				resetBinding();
				return false;
			}

			QChar written = text.at(0);

			if(written == '\\'){
				if (getCurWord() == "\\") {
					resetBinding();
					return false;
				} else if (getCurWord() == "") {
					maxWritten = curStart + 1;
				} else {
					if (LatexCompleter::config && LatexCompleter::config->eowCompletes) {
						insertCompletedWord();
					}
					QDocumentCursor edc = editor->cursor();
					if (edc.hasSelection()) {
						edc.removeSelectedText();
						editor->setCursor(edc);
					}
					curStart = edc.columnNumber();
					maxWritten = curStart + 1;
				}
				bool autoOverride = editor->isAutoOverrideText("\\");
				if (!autoOverride) editor->cursor().insertText(written);
				else {
					QDocumentCursor c = editor->cursor();
					editor->document()->beginMacro();
					c.deleteChar();
					c.insertText("\\");
					editor->document()->endMacro();
					autoOverridenText = "\\";
				}
				//editor->insertText(written); <- can't use that since it may break the completer by auto closing brackets
				handled = true;
			} else if (completer->acceptChar(written, editor->cursor().columnNumber() - curStart)) {
				insertText(written);
				handled = true;
			} else if (getCommonEOW().contains(written) ) {
				QString curWord = getCurWord();

				if (curWord == "\\" || !LatexCompleter::config || !LatexCompleter::config->eowCompletes) {
					resetBinding();
					simpleRestoreAutoOverride(written);
					return false;
				}
				const QList<CompletionWord> &words = completer->listModel->getWords();
				QString newWord;
				int eowchars = 10000;
				foreach (const CodeSnippet &w, words) {
					if (!w.word.startsWith(curWord)) continue;
					if (w.word.length() == curWord.length()) {
						newWord = w.word;
						break;
					}
					int newoffset = w.lines.first().indexOf(written, curWord.length());
					if (newoffset < 0) continue;
					int neweowchars = 0;
					for (int i = curWord.length(); i < newoffset; i++)
						if (getCommonEOW().contains(w.lines.first()[i]))
							neweowchars++;
					if (neweowchars < eowchars) {
						newWord = w.word;
						eowchars = neweowchars;
						if (eowchars == 1) break;
					}
				}

				if (!newWord.isEmpty() && newWord.length() != curWord.length()) {
					QString insertion = newWord.mid(curWord.length(), newWord.indexOf(written, curWord.length()) - curWord.length() + 1); //choose text until written eow character
					insertText(insertion);
					//insertText(written);
					handled = true;
				}

				if (!handled) {
					insertCompletedWord();
					if (newWord.isEmpty())
						simpleRestoreAutoOverride(written);
					//insertText(written);
					resetBinding();
					return false;//oldBinding->keyPressEvent(event,editor); //call old input binding for long words (=> key replacements after completions, but the user can still write \")
				}
			}

			completer->filterList(getCurWord(), showMostUsed);
			if (!completer->list->currentIndex().isValid())
				select(completer->list->model()->index(0, 0, QModelIndex()));
			return handled;
		}


		void cursorPositionChanged(QEditor * edit){

			if(edit != editor)
				return; //should never happen
			
			const auto cursor = editor -> cursor();

			if(cursor.lineNumber() == curLineNumber && cursor.columnNumber() >= curStart)
				return;
				
			resetBinding();
		}


		void setMostUsed(int mostUsed,bool quiet = false){

			showMostUsed = mostUsed;
			
			if(quiet)
				return;
			
			completer -> filterList(getCurWord(),showMostUsed);
			
			const auto list = completer -> list;

			if(! list -> currentIndex().isValid())
				select(list -> model() -> index(0,0,QModelIndex()));
		}


		int getMostUsed(){
			return showMostUsed;
		}


		void resetBinding(){

			if(completer)
				completer -> listModel -> setEnvironMode(false);

			showMostUsed = false;
			QString curWord = getCurWord();
			
			if(!active)
				return;
			
			QToolTip::hideText();
			
			//reenable auto close chars
			
			if(completer)
				editor -> setFlag(QEditor::AutoCloseChars,completer -> editorAutoCloseChars);
			
			editor -> setInputBinding(oldBinding);

			if(completer && completer -> widget && completer -> widget -> isVisible()){
				completer -> widget -> hide();
				editor -> setFocus();
			}

			if(completer)
				completer -> disconnect(editor,SIGNAL(cursorPositionChanged()),completer,SLOT(cursorPositionChanged()));

			active = false;

			//editor=0; this leads to a crash, as the editor is still in use after reseting the cursor
			
			if(completer && completer -> completingKey() && curWord.endsWith('=')){

				auto view = editor 
					-> property("latexEditor")
					.  value<LatexEditorView *>();
				
				Q_ASSERT(view);
				
				view -> mayNeedToOpenCompleter();
			}

			if(completer && completer -> completingGraphic() && curWord.endsWith('/'))
				completer -> complete(editor,LatexCompleter::CompletionFlags(LatexCompleter::CF_FORCE_VISIBLE_LIST | LatexCompleter::CF_FORCE_GRAPHIC));
		}


		void bindTo(QEditor * edit,LatexCompleter * caller,bool forced,int start){

			if(active)
				resetBinding();
			
			active = true;
			completer = caller;
			editor = edit;
			
			oldBinding = (editor -> inputBindings().count() > 0) 
				? editor -> inputBindings()[0] 
				: nullptr;
			
			editor -> setInputBinding(this);

			curStart = (start > 0) 
				? start 
				: 0;

			maxWritten = editor -> cursor().columnNumber();

			if(maxWritten < start)
				maxWritten = start;
			
			curLineNumber = editor -> cursor().lineNumber();
			showAlways = forced; //curWord!="\\";
			
			completer -> filterList(getCurWord());
			
			if(showAlways){
				completer -> widget -> show();
				select(completer -> list -> model() -> index(0,0,QModelIndex()));
			}
		}


		bool isActive(){
			return active;
		}
};



auto completerInputBinding = new CompleterInputBinding();


//------------------------------Item Delegate--------------------------------

class CompletionItemDelegate : public QItemDelegate {

	public:
    
		explicit CompletionItemDelegate(QObject * parent = nullptr)	
			: QItemDelegate(parent) {}

		void paint(
			QPainter * painter,
			const QStyleOptionViewItem & option,
			const QModelIndex & index
		) const {
		
			QVariant v = index.model() -> data(index);
		
			if(!v.isValid())
				return;
			
			if(!v.canConvert<CompletionWord>())
				return;
			
			CompletionWord cw = v.value<CompletionWord>();

			if(cw.lines.empty())
				return;
				
			if(cw.placeHolders.empty())
				return;
			
			QFont fNormal = option.font;
			QFont fPlHolder = option.font;
			QColor normalColor, plHolderColor;

			if((QStyle::State_HasFocus | QStyle::State_Selected) & option.state){
				painter -> fillRect(option.rect,option.palette.brush(QPalette::Highlight));
				normalColor = option.palette.color(QPalette::HighlightedText);
			} else {
				painter -> fillRect(option.rect,option.palette.brush(QPalette::Base)); //doesn't seem to be necessary
				normalColor = option.palette.color(QPalette::Text);
			}

			painter -> setPen(normalColor);
			plHolderColor = normalColor;
			plHolderColor.setAlpha(128);

			QRectF r = option.rect;
			r.setLeft(r.left() + 2);

			bool drawPlaceholder = ! cw.placeHolders.empty();

			QString firstLine = cw.lines[0];
			
			if(!cw.getName().isEmpty()){
				drawPlaceholder = false;
				firstLine = cw.getName();
			}

			if(!drawPlaceholder)
				painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,firstLine);
			else {

				QFontMetricsF fmn(fNormal);
				QFontMetricsF fmi(fPlHolder);
				
				int p = 0;
				
				for(int i = 0;i < cw.placeHolders[0].size();i++){

					QString temp = firstLine.mid(p,cw.placeHolders[0][i].offset - p);

					painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,temp);

					r.setLeft(r.left() + UtilsUi::getFmWidth(fmn,temp));
					
					temp = firstLine.mid(cw.placeHolders[0][i].offset,cw.placeHolders[0][i].length);
					
					painter -> setFont(fPlHolder);
					painter -> setPen(plHolderColor);
					painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,temp);
					
					r.setLeft(r.left() + UtilsUi::getFmWidth(fmi,temp) + 1);

					p = cw.placeHolders[0][i].offset + cw.placeHolders[0][i].length;
					
					painter -> setFont(fNormal);
					painter -> setPen(normalColor);
					
					if(p > firstLine.length())
						break;
				}

				painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,firstLine.mid(p));
			}
		}
};



//----------------------------list model------------------------------------

LatexCompleterConfig * CompletionListModel::config = nullptr;

int CompletionListModel::rowCount(const QModelIndex & parent) const {
	
	Q_UNUSED(parent)

	return words.count();
}


QVariant CompletionListModel::data(const QModelIndex & index,int role) const {
	
	if(!index.isValid())
		return QVariant();

	if(index.row() >= words.count())
		return QVariant();

	if(role != Qt::DisplayRole)
		return QVariant();
		
	QVariant temp;
	temp.setValue(words.at(index.row()));
	return temp;
}


QVariant CompletionListModel::headerData(
	int section, 
	Qt::Orientation orientation,
    int role
) const {

	Q_UNUSED(role)
	Q_UNUSED(orientation)
	Q_UNUSED(section)

	return QVariant();
}


bool CompletionListModel::isNextCharPossible(const QChar & c){

	//if (words.count()>100) //probable that every char is there (especially since acceptedChars is already checked)
	//	return true;  -> leads to faulty behaviour (\b{ is supposed to be within list ...)

	const auto extension = curWord + c;

	const auto sensitivity = (
		LatexCompleter::config &&
	    LatexCompleter::config -> caseSensitive == LatexCompleterConfig::CCS_CASE_SENSITIVE
	)
		? Qt::CaseSensitive
		: Qt::CaseInsensitive;
	
	for(const auto & word : words)
		if(word.word.startsWith(extension,sensitivity))
			return true;
	
	return false;
}


bool CompletionListModel::canFetchMore(const QModelIndex &) const {
	return mCanFetchMore;
}

void CompletionListModel::fetchMore(const QModelIndex &){
	beginInsertRows(QModelIndex(),words.count(),qMin(words.count() + 100,mWordCount));
	filterList(mLastWord,mLastMU,true,mLastType);
	endInsertRows();
}

CompletionWord CompletionListModel::getLastWord(){
	return mLastWordInList;
}


void CompletionListModel::setKeyValWords(const QString & name,const QSet<QString> & newwords){

	QList<CompletionWord> newWordList;
	newWordList.clear();
	
	for(auto i = newwords.constBegin();i != newwords.constEnd();++i){

		QString str = * i;
		QString validValues;
		
		if(str.contains('#') && !str.startsWith('#')){
			
			int j = str.indexOf('#');
			validValues = str.mid(j + 1);
			str = str.left(j);
			
			QStringList lst = validValues.split(',');
			QString key = str;
			
			if(key.endsWith('='))
				key.chop(1);

            setKeyValWords(name + '/' + key, convertStringListtoSet(lst));
		}

		CompletionWord cw(str,false);
		
		cw.index = 0;
		cw.usageCount = -2;
		cw.snippetLength = 0;
		
		newWordList.append(cw);
	}

    std::sort(newWordList.begin(),newWordList.end());

	keyValLists.insert(name,newWordList);
}


void CompletionListModel::setContextWords(const QSet<QString> & newwords,const QString & context){

	QList<CompletionWord> newWordList;
	newWordList.clear();

	for(auto i = newwords.constBegin();i != newwords.constEnd();++i){

		QString str = * i;
		QString validValues;
		
		if(str.contains('#')){

			int j = str.indexOf('#');
			validValues = str.mid(j + 1);
			str = str.left(j);

			QStringList lst = validValues.split(',');
			QString key = str;

			if(key.endsWith('='))
				key.chop(1);

            setKeyValWords(context + '/' + key, convertStringListtoSet(lst));
		}

		CompletionWord cw(str);
		cw.index = 0;
        cw.usageCount = 2;

		cw.snippetLength = 0;
		newWordList.append(cw);
	}

    std::sort(newWordList.begin(),newWordList.end());

	contextLists.insert(context,newWordList);
}


//js: still using dirty hack, however order should be ' '{[* abcde...
// needs to be replaced as well for sorting \bgein{abc*} after \bgein{abc}
//(first is a space->) !"# follow directly in the ascii table

QString makeSortWord(const QString &normal){
	QString res = normal.toLower();
	res.replace('{','!'); 
	res.replace('}','!'); 
	res.replace('[','\\');
	res.replace('*','#');
	return res;
}

void CompletionListModel::setEnvironMode(bool mode){
	mEnvMode = mode;
}

bool cwLessThan(const CompletionWord & a,const CompletionWord & b){
    return a.score > b.score;
}


void CompletionListModel::filterList(const QString & word,int mostUsed,bool fetchMore,CodeSnippet::Type type){
	
	if(mostUsed < 0)
		mostUsed = LatexCompleter::config -> preferedCompletionTab;
	
	//don't return if mostUsed differnt from last call

	if(
		!word.isEmpty() && 
		word == curWord && 
		mostUsed == mostUsedUpdated && 
		!fetchMore
	) return; 
	
	mLastWord = word;
	mLastMU = mostUsed;
	mLastType = type;
	mCanFetchMore = false;
	mostUsedUpdated = mostUsed;
	
	if(!fetchMore)
		words.clear();
	
	auto cs = Qt::CaseInsensitive;
	bool checkFirstChar = false;

	if(LatexCompleter::config){
		
		if(LatexCompleter::config -> caseSensitive == LatexCompleterConfig::CCS_CASE_SENSITIVE)
			cs = Qt::CaseSensitive;

		checkFirstChar = 
			LatexCompleter::config -> caseSensitive == LatexCompleterConfig::CCS_FIRST_CHARACTER_CASE_SENSITIVE && 
			word.length() > 1;
	}
	
	int cnt = 0;
	QString sortWord = makeSortWord(word);

    if(mostUsed == 2){

        QStringList chars = word.split("",Qt::SkipEmptyParts);

        if(chars.value(0) == QChar('\\'))
            chars.takeFirst();

        QString regExpression = chars.join(".*");
        QRegularExpression rx(regExpression);

        words = QtConcurrent::blockingFiltered(baselist,[rx](const CompletionWord & item){
            return item.sortWord.contains(rx);
        });

        QtConcurrent::blockingMap(words,[word](CompletionWord &item){

            int 
				lastMatch = -2,
				score = 0,
            	l = 0;

            for(int i = 0;i < item.sortWord.length();i++){
                
				if(l >= word.length())
                    break;

                if(item.sortWord.at(i) == word.at(l)){
               
			        if(lastMatch+1==i)
                        score+=20;
               
			        score -= i; // later position are degraded
                    lastMatch = i;
                    l++;
                }
            }

            if(item.word.contains('@')){

				// special treatment for command-names containing @

                if(item.word.contains("@@")){ 
                    QString ln = item.lines[0];
                    ln.replace("@@","@");
                    item = CompletionWord(ln);
                } else {
                    QString ln = item.lines[0];
                    ln.replace("{@}","%<bibid%>");
                    ln.replace("{@l}","%<label%>");
                    item = CompletionWord(ln);
                }
            }

            // reduce score for atypical or unused
            // don't upvote reference commands
 
            if(item.index)
                score += (item.usageCount <= 0) 
					? 10 * item.usageCount 
					: 10;

            item.score=score;
        });

        std::stable_sort(words.begin(),words.end(),cwLessThan);
    } else {

        // normal sorting

        if(!fetchMore)
            it = std::lower_bound(baselist.begin(),baselist.end(),CompletionWord(word));
		
        // special treatment for citation commands as they generated on the fly
        //TODO: needs to be adapted to later code
 
        if(it == baselist.end() || !it -> word.startsWith(word, cs)){
            
			int i = word.lastIndexOf("{");
			QString test = word.left(i) + "{";
            
			auto lIt = std::lower_bound(baselist.begin(), baselist.end(), CompletionWord(test));

            if(lIt != baselist.end()){
                
				// part of command without argument is in baselist
                // is it a on-the-fly filled command ?

                if(lIt -> word.contains('@')){
                    
					QString citeStart = word.mid(i + 1);
                    QString ln = lIt -> lines[0];

                    struct Rpl {
                        QString id;
                        QList<CompletionWord> & lst;
                    };

                    QMap<QString,std::shared_ptr<Rpl>> replacement;

                    std::shared_ptr<Rpl> r0(new Rpl { "{%<bibid%>}" , wordsCitations });
                    replacement.insert("@",r0);

                    std::shared_ptr<Rpl> r1(new Rpl { "{%<label%>}" , wordsLabels });
                    replacement.insert("@l",r1);

                    for(auto localIt = replacement.cbegin();localIt != replacement.cend();++localIt){

                        QString searchWord = "{" + localIt.key() + "}";
                        QString key = localIt.key();
						auto repl = * localIt;

                        if(ln.contains(searchWord)){
                            
							if(repl -> id.startsWith("{%<" + citeStart)){
                                // keep general id if it matches input
                                ln.replace(searchWord,repl -> id);
                                words.append(CompletionWord(ln));
                            }
                            
							cnt++;
                            
                            for(const auto & id : repl -> lst){

                                if(!id.word.startsWith(citeStart))
                                    continue;
                                
								auto cw = * lIt;
                                int index = cw.lines[0].indexOf(key);
                                
								cw.word.replace(key, id.word);
                                cw.sortWord.replace(key, id.word);
                                cw.lines[0].replace(key, id.word);
                                
								for(int i = 0;i < cw.placeHolders.count();i++){
                                   
								    if (cw.placeHolders[i].isEmpty())
                                        continue;
                                   
								    for (int j = 0;j < cw.placeHolders[i].count();j++){
                                        
										auto & ph = cw.placeHolders[i][j];
                                        
										if(ph.offset > index)
                                            ph.offset += id.word.length() - 1;
                                    }
                                }
                            
							    words.append(cw);
                            }

                            cnt += repl -> id.length();
                        }
                    }
                }
            }
        }

        while(it != baselist.end()){
            if(
				it -> word.startsWith(word, cs) &&
                (!checkFirstChar || it -> word[1] == word[1])
			){
                if(
					mostUsed == 3 || 
					it -> usageCount >= mostUsed || 
					it -> usageCount == -2
				){
                    if(
						mostUsed < 2 && 
						type != CodeSnippet::none && 
						it -> type != type
					){
                        ++it;
                        continue; // leave out words which don't have the proper type (except for all-mode)
                    }

                    if(mEnvMode){

                        CompletionWord cw = * it;
                        
						if(
							cw.word.startsWith("\\begin") ||
							cw.word.startsWith("\\end")
						){
                            QString text = cw.word;
                            int i = text.indexOf('}');
                         
						    text.truncate(i + 1);
                         
						    cw.word = text;
                            cw.sortWord = text;
                            cw.lines = QStringList(text);
                            cw.placeHolders.clear();
                            cw.placeHolders.append(QList<CodeSnippetPlaceHolder>());
                        }

                        if(!words.contains(cw))
                            words.append(cw);
                    } else {

                        // cite command
                        
						if(it -> word.contains('@')){

							// special treatment for command-names containing @

                            if(it -> word.contains("@@")){
                                QString ln = it -> lines[0];
                                ln.replace("@@","@");
                                words.append(CompletionWord(ln));
                            } else {

                                QString ln = it -> lines[0];
                                
								struct Rpl {
                                    QString id;
                                    QList<CompletionWord> & lst;
                                };

                                QMap<QString,std::shared_ptr<Rpl>> replacement;
                               
							    std::shared_ptr<Rpl> r0(new Rpl { "{%<bibid%>}" , wordsCitations });
                                replacement.insert("@",r0);
                               
							    std::shared_ptr<Rpl> r1(new Rpl{ "{%<label%>}" , wordsLabels });
                                replacement.insert("@l",r1);
                               
							    for(auto localIt = replacement.cbegin();localIt != replacement.cend();++localIt){

                                    QString searchWord = "{" + localIt.key() + "}";
                                    QString key = localIt.key();
                                    
									std::shared_ptr<Rpl> repl = * localIt;
                                    
									if(ln.contains(searchWord)){

                                        ln.replace(searchWord,repl -> id);
                                        words.append(CompletionWord(ln));
                                        
										cnt++;
                                        
                                        for(const auto & id : repl -> lst){

                                            CompletionWord cw = * it;

                                            int index = cw.lines[0].indexOf(key);
                                            
											cw.word.replace(key, id.word);
                                            cw.sortWord.replace(key, id.word);
                                            cw.lines[0].replace(key, id.word);
                                            
											for(int i = 0;i < cw.placeHolders.count();i++){
                                               
											    if(cw.placeHolders[i].isEmpty())
                                                    continue;
                                                
												for(int j = 0;j < cw.placeHolders[i].count();j++){
                                                    
													auto & ph = cw.placeHolders[i][j];
                                                    
													if(ph.offset > index)
                                                        ph.offset += id.word.length() - 1;
                                                }
                                            }

                                            words.append(cw);
                                        }

                                        cnt += repl -> id.length();
                                    }
                                }
                            }
                        } else {
                            words.append(* it);
                        }
                    }

                    cnt++;
                }
            } else {

				// sorted list

                if(!it -> sortWord.startsWith(sortWord))
                    break;
            }

            ++it;

            if(cnt > 100){
                mCanFetchMore = true;
                break;
            }
        }
    }
    
	curWord = word;

    if(!fetchMore){

        mWordCount = words.count();
        
		if(!words.isEmpty())
            mLastWordInList = words.last();
    }

    if(mCanFetchMore && !fetchMore){

        // calculate real number of rows
        
		QString wordp = word;
        
		if(wordp.isEmpty()){
            mWordCount = baselist.count();
            mLastWordInList = baselist.last();
        } else {

            QChar lst = wordp[wordp.length() - 1];
            ushort nr = lst.unicode();
            
			wordp[wordp.length() - 1] = QChar(nr + 1);
            
			auto it2 = std::lower_bound(baselist.constBegin(),baselist.constEnd(),CompletionWord(wordp));
            
			mWordCount = it2 - it;

            if(it2 == baselist.constBegin())
                mLastWordInList = baselist.last();
            else
                mLastWordInList = * (--it2);
        }
    }

	if(!fetchMore){
		beginResetModel();
		endResetModel();
	}
}


void CompletionListModel::incUsage(const QModelIndex & index){

	if(!index.isValid())
		return ;

	if(index.row() >= words.size())
		return ;

	int j = index.row();
	
	CompletionWord curWord = words.at(j);

	// don't count text words

	if(curWord.usageCount < -1)
		return;

    auto it = std::lower_bound(wordsCommands.begin(),wordsCommands.end(),curWord);

	// not found, e.g. citations

	if(it == wordsCommands.end()) 
		return;
	
	if(it -> word != curWord.word)
		return;

	it -> usageCount++;

	if(curWord.snippetLength <= 0)
		return;
		
	bool replaced = false;
	
	auto res = config -> usage.values(curWord.index);
	
	for(int j = 0;j < res.size();++j)
		if(res.at(j).first == curWord.snippetLength){
			config -> usage.remove(curWord.index,res.at(j));
			config -> usage.insert(curWord.index,qMakePair(curWord.snippetLength,it -> usageCount));
			replaced = true;
			break;
		}

	// new word
	
	if(!replaced)
		config -> usage.insert(curWord.index,qMakePair(curWord.snippetLength,it -> usageCount));
}


void CompletionListModel::setBaseWords(const QSet<QString> & newwords,CompletionType completionType){

	QList<CompletionWord> newWordList;
	newWordList.clear();

	for(auto i = newwords.constBegin();i != newwords.constEnd();++i){

		QString str = * i;
		CompletionWord cw(str);

		if(completionType == CT_COMMANDS){

			cw.index = qHash(str);
			cw.snippetLength = str.length();
			cw.usageCount = 0;
			
			auto res = config -> usage.values(cw.index);
			
			for(const auto & elem : res)
				if(elem.first == cw.snippetLength){
					cw.usageCount = elem.second;
					break;
				}
		} else {
			cw.index = 0;
			cw.usageCount = -2;
			cw.snippetLength = 0;
		}

		newWordList.append(cw);
	}

    std::sort(newWordList.begin(),newWordList.end());

	switch (completionType) {
	case CT_NORMALTEXT:
		wordsText = newWordList;
		break;
	case CT_CITATIONS:
		wordsCitations = newWordList;
		break;
	default:
		wordsCommands = newWordList;
	}

	baselist = wordsCommands;
}


void CompletionListModel::setBaseWords(
	const std::set<QString> & newwords,
	CompletionType completionType
){
    QList<CompletionWord> newWordList;
    newWordList.clear();

    for(auto i = newwords.cbegin();i != newwords.cend();++i){

        QString str = * i;
        CompletionWord cw(str);

        if(completionType == CT_COMMANDS){

            cw.index = qHash(str);
            cw.snippetLength = str.length();
            cw.usageCount = 0;
            
			auto res = config -> usage.values(cw.index);
            
			for(const auto & elem : res)
                if(elem.first == cw.snippetLength){
                    cw.usageCount = elem.second;
                    break;
                }
        } else {
            cw.index = 0;
            cw.usageCount = -2;
            cw.snippetLength = 0;
        }

        newWordList.append(cw);
    }

    switch(completionType){
    case CT_NORMALTEXT:
        wordsText = newWordList;
        break;
    case CT_CITATIONS:
        wordsCitations = newWordList;
        break;
    default:
        wordsCommands = newWordList;
    }

    baselist = wordsCommands;
}

void CompletionListModel::setBaseWords(
	const QList<CompletionWord> & newwords,
	CompletionType completionType
){
	QList<CompletionWord> newWordList;
	newWordList.clear();

	for(const auto & cw : newwords)
		newWordList.append(cw);

    std::sort(newWordList.begin(),newWordList.end());

	switch(completionType){
	case CT_NORMALTEXT:
		wordsText = newWordList;
		break;
	case CT_CITATIONS:
		wordsCitations = newWordList;
		break;
	default:
		wordsCommands = newWordList;
	}

	baselist = wordsCommands;
}


void CompletionListModel::setBaseWords(
	const CodeSnippetList & baseCommands,
	const CodeSnippetList & newwords,
	CompletionType completionType
){
	CodeSnippetList newWordList;

	newWordList << baseCommands;
	newWordList.unite(newwords);

	auto last = std::unique(newWordList.begin(),newWordList.end());
	newWordList.erase(last,newWordList.end());

	switch(completionType){
	case CT_NORMALTEXT:
		wordsText = newWordList;
		break;
	case CT_CITATIONS:
		wordsCitations = newWordList;
		break;
	case CT_LABELS:
		wordsLabels = newWordList;
		break;
	default:
		wordsCommands = newWordList;
	}

	baselist = newWordList;
}

void CompletionListModel::setAbbrevWords(const QList<CompletionWord> & newwords){
	wordsAbbrev = newwords;
    std::sort(wordsAbbrev.begin(),wordsAbbrev.end());
}

void CompletionListModel::setConfig(LatexCompleterConfig * newConfig){
	config = newConfig;
}


//------------------------------completer-----------------------------------

LatexReference * LatexCompleter::latexReference = nullptr;
LatexCompleterConfig * LatexCompleter::config = nullptr;

LatexCompleter::LatexCompleter(const LatexParser &latexParser, QObject *p)
	: QObject(p)
	, latexParser(latexParser)
	, maxWordLen(0)
	, editorAutoCloseChars(false)
	, forcedRef(false)
	, forcedGraphic(false)
	, forcedCite(false)
	, forcedPackage(false)
	, forcedKeyval(false)
	, forcedSpecialOption(false)
	, forcedLength(false)
	, startedFromTriggerKey(false) {

	if(!qobject_cast<QWidget *>(parent()))
        QMessageBox::critical(nullptr, "Serious PROBLEM", 
			QString("The completer has been created without a parent widget. This is impossible!\n") +
		    QString("Please report it ASAP to the bug tracker on texstudio.sf.net and check if your computer is going to explode!\n") +
		    QString("(please report the bug *before* going to a safe place, you could rescue others)"),QMessageBox::Ok);

	list = new QListView(qobject_cast<QWidget *>(parent()));
	listModel = new CompletionListModel(list);

    connect(list,SIGNAL(clicked(const QModelIndex &)),this,SLOT(selectionChanged(const QModelIndex &)));

	list -> setModel(listModel);
	list -> setFocusPolicy(Qt::NoFocus);
	list -> setItemDelegate(new CompletionItemDelegate(list));
	list -> setAutoFillBackground(true);
    
	editor = nullptr;
	workingDir = '/';
    dirReader = nullptr;
    bibReader = nullptr;
    packageList = nullptr;

	widget = new QWidget(qobject_cast<QWidget *>(parent()));

	auto layout = new QVBoxLayout;
	layout -> setSpacing(0);
	
	tbAbove = new QTabBar();
	tbAbove -> setShape(QTabBar::RoundedNorth);
	tbAbove -> addTab(tr("typical"));
	tbAbove -> addTab(tr("most used"));
    tbAbove -> addTab(tr("fuzzy"));
	tbAbove -> addTab(tr("all"));
	tbAbove -> setToolTip(tr("press shift+space to change view"));
	layout -> addWidget(tbAbove);
	tbAbove -> hide();
	
	layout -> addWidget(list);
	
	tbBelow = new QTabBar();
	tbBelow -> setShape(QTabBar::RoundedSouth);
	tbBelow -> addTab(tr("typical"));
	tbBelow -> addTab(tr("most used"));
    tbBelow -> addTab(tr("fuzzy"));
	tbBelow -> addTab(tr("all"));
	tbBelow -> setToolTip(tr("press shift+space to change view"));
	layout -> addWidget(tbBelow);
	widget -> setLayout(layout);
	widget -> setContextMenuPolicy(Qt::PreventContextMenu);

	connect(list,SIGNAL(activated(QModelIndex)),this,SLOT(listClicked(QModelIndex)));
}


LatexCompleter::~LatexCompleter(){

	if(dirReader){
		dirReader -> quit();
		dirReader -> wait();
        delete dirReader;
	}

	if(bibReader){
		bibReader -> quit();
        bibReader -> wait();
        delete bibReader;
	}
}

void LatexCompleter::changeView(int pos){
	completerInputBinding -> setMostUsed(pos);
}

void LatexCompleter::listClicked(QModelIndex index){

	Q_UNUSED(index);

	if(!completerInputBinding -> insertCompletedWord())
		editor -> insertText("\n");
	
	completerInputBinding -> resetBinding();
}


void LatexCompleter::insertText(QString txt){

	if(!isVisible())
		return;

	completerInputBinding -> insertText(txt);
	QString word = completerInputBinding -> getCurWord();
	filterList(word,completerInputBinding->getMostUsed());
}


void LatexCompleter::setAdditionalWords(const QSet<QString> & newwords,CompletionType completionType){
	
	// convert to codesnippets
	CodeSnippetList newWordList;
	
	for(auto i = newwords.constBegin();i != newwords.constEnd();++i){

		QString str = * i;
		bool isReference = str.startsWith('@');
	
		if(isReference)
			str = str.mid(1);
		
		CompletionWord cw(str);
		
		if(completionType == CT_COMMANDS){

			cw.index = qHash(str);
			cw.snippetLength = str.length();

			// make reference always visible (most used) in completer

			cw.usageCount = isReference ? 2 : 0; 
			
			auto res = config->usage.values(cw.index);
			
			for(const auto & elem : res)
				if(elem.first == cw.snippetLength){
					cw.usageCount = elem.second;
					break;
				}
		} else {
			cw.index = 0;
			cw.usageCount = -2;
			cw.snippetLength = 0;
		}

		newWordList.append(cw);
	}

    std::sort(newWordList.begin(),newWordList.end());

	CodeSnippetList concated;

	if(config && completionType == CT_COMMANDS)
		concated.unite(config -> words);

	listModel -> setBaseWords(concated,newWordList,completionType);
	widget -> resize(200,200);
}


/*!
 * \brief setAdditionalWords from a std::set<QString>
 * The advantage is that a std::set is already sorted.
 * \param newwords
 * \param completionType
 */

void LatexCompleter::setAdditionalWords(const std::set<QString> & newwords,CompletionType completionType){

    // convert to codesnippets
    CodeSnippetList newWordList;

    for(auto i = newwords.cbegin();i != newwords.cend();++i){

        QString str = *i;
        bool isReference = str.startsWith('@');

        if(isReference)
            str = str.mid(1);
        
		CompletionWord cw(str);
        
		if(completionType == CT_COMMANDS){

            cw.index = qHash(str);
            cw.snippetLength = str.length();

			// make reference always visible (most used) in completer

            cw.usageCount = isReference ? 2 : 0;

            auto res = config -> usage.values(cw.index);

            for(const auto & elem : res)
                if(elem.first == cw.snippetLength){
                    cw.usageCount = elem.second;
                    break;
				}
        } else {
            cw.index = 0;
            cw.usageCount = -2;
            cw.snippetLength = 0;
        }

        newWordList.append(cw);
    }

    CodeSnippetList concated;

    if(config && completionType == CT_COMMANDS)
		concated.unite(config -> words);

    listModel -> setBaseWords(concated,newWordList,completionType);
    widget -> resize(200,200);
}


void LatexCompleter::setAdditionalWords(const CodeSnippetList & newwords,CompletionType completionType){
	
	CodeSnippetList concated;
	
	if(config && completionType == CT_COMMANDS)
		concated.unite(config -> words);

	listModel -> setBaseWords(concated,newwords,completionType);
	widget -> resize(200,200);
}


void LatexCompleter::setKeyValWords(const QString & name,const QSet<QString> & newwords){
	listModel -> setKeyValWords(name,newwords);
}


void LatexCompleter::setContextWords(const QSet<QString> & newwords,const QString & context){
	listModel -> setContextWords(newwords,context);
}


/*!
 * \brief adjust width and position of the widget.
 * \return BelowCursor if the calculated position is below the cursor, otherwise AboveCursor
 */

void LatexCompleter::adjustWidget(){

	// adjust width
	int newWordMax = 0;
	
	QFont f = QApplication::font();
	f.setItalic(true);
	
	QFontMetrics fm(f);

	const auto & words = listModel -> getWords();
	
	for(int i = 0;i < words.size();i++){
		
		if(words[i].lines.empty())
			continue;
			
		if(words[i].placeHolders.empty())
			continue;
		
		int temp = UtilsUi::getFmWidth(fm,words[i].lines[0]) + words[i].placeHolders[0].size() + 10;
		
		if(temp > newWordMax)
			newWordMax = temp;
	}
	
	maxWordLen = newWordMax;
    
	int width = qMax(400,maxWordLen);
	auto bar = list -> verticalScrollBar();
	
	if(bar && bar -> isVisible())
		width += bar -> width() * 4;
	
	widget -> resize(width, 200);

	// adjust position
    QPointF offset;
	bool isAboveCursor = false;
	
	if(editor -> getPositionBelowCursor(offset,widget -> width(),widget -> height(),isAboveCursor))
        widget -> move(editor -> mapTo(qobject_cast<QWidget *>(parent()),offset.toPoint()));

	// adjust visible tab bar depending on location relative to cursor
	QTabBar * tbOn = (isAboveCursor) ? tbAbove : tbBelow;
	QTabBar * tbOff = (isAboveCursor) ? tbBelow : tbAbove;
	
	disconnect(tbOn,SIGNAL(currentChanged(int)),this,SLOT(changeView(int)));
	disconnect(tbOff,SIGNAL(currentChanged(int)),this,SLOT(changeView(int)));
	
	tbOn -> show();
	tbOff -> hide();
	tbOn -> setCurrentIndex(config -> preferedCompletionTab);

	connect(tbOn,SIGNAL(currentChanged(int)),this,SLOT(changeView(int)));
}


void LatexCompleter::updateAbbreviations(){

	REQUIRE(config);
	
	QList<CompletionWord> wordsAbbrev;
	
	for(const auto & macro : config -> userMacros){

		if(macro.abbrev.isEmpty())
			continue;
		
		if(macro.snippet().isEmpty())
			continue;
		
		//CompletionWord cw(abbr);
		// for compatibility to texmaker ...
		
		CompletionWord cw(macro.snippet());
		
		// <!compatibility>
		cw.word = macro.abbrev;
		cw.sortWord = makeSortWord(cw.word);
		cw.setName(macro.abbrev + tr(" (Usertag)"));
		
		wordsAbbrev << cw;
	}
	listModel->setAbbrevWords(wordsAbbrev);
}


void LatexCompleter::complete(QEditor * newEditor,const CompletionFlags & flags){
	Q_ASSERT(list);
	Q_ASSERT(listModel);
	Q_ASSERT(completerInputBinding);

	// called with already open completer

    bool alreadyActive = completerInputBinding -> isActive();

	forcedRef = flags & CF_FORCE_REF;
	forcedGraphic = flags & CF_FORCE_GRAPHIC;
	forcedCite = flags & CF_FORCE_CITE;
	forcedPackage = flags & CF_FORCE_PACKAGE;
	forcedKeyval = flags & CF_FORCE_KEYVAL;
	forcedSpecialOption = flags & CF_FORCE_SPECIALOPTION;
	forcedLength = flags & CF_FORCE_LENGTH;
	startedFromTriggerKey = !(flags & CF_FORCE_VISIBLE_LIST);

	if(editor != newEditor){
		
		if(editor)
			disconnect(editor,SIGNAL(destroyed()),this,SLOT(editorDestroyed()));
		
		if(newEditor)
			connect(newEditor,SIGNAL(destroyed()),this,SLOT(editorDestroyed()));
		
		editor = newEditor;
	}

	if(!editor)
		return;
	
	auto c = editor -> cursor();

	if(!c.isValid())
		return;

	// completer does not handle cursor mirrors. It would just remove them. -> don't use completer

	if(editor -> cursorMirrorCount() > 0)
		return; 

	if(c.hasSelection()){
		c.setColumnNumber(qMax(c.columnNumber(),c.anchorColumnNumber()));
		editor -> setCursor(c);
	}

	//disable auto close char while completer is open
	// don't change again from open completer (and therin changed flag, see #1347)

    if(!alreadyActive)
        editorAutoCloseChars = editor -> flag(QEditor::AutoCloseChars);

    editor -> setFlag(QEditor::AutoCloseChars,false);
    completerInputBinding -> setMostUsed(config -> preferedCompletionTab,true);
    
	bool handled = false;

    if(forcedGraphic){
        
		if(!dirReader){
            dirReader = new directoryReader(this);
            connect(dirReader,& directoryReader::directoryLoaded,this,& LatexCompleter::directoryLoaded);
            connect(this,SIGNAL(setDirectoryForCompletion(QString)),dirReader,SLOT(readDirectory(QString)));
            dirReader -> start();
        }
        
		QSet<QString> files;
        
		listModel -> setBaseWords(files,CT_NORMALTEXT);
        listModel -> baselist = listModel -> wordsText;
        
		handled = true;
    }
	
	if(forcedCite){
		listModel -> baselist = listModel -> wordsCitations;
		handled = true;
	}

	if(forcedRef){
		listModel -> baselist = listModel -> wordsLabels;
		handled = true;
	}

	if(forcedPackage){
		listModel -> setBaseWords(* packageList,CT_NORMALTEXT);
		listModel -> baselist = listModel -> wordsText;
		handled = true;
	}

	if(forcedKeyval){

		listModel -> baselist.clear();
		
		handled = true;
		
		for(const auto & cw : listModel -> keyValLists.value(workingDir)){

			const auto word = cw.word;

			if(word.startsWith("#")){
				if(word == "#L"){
					// complete with length
					forcedLength = true;
                    forcedKeyval = false;
                    handled = false;
                    break;
                }

                handled = true;
                break;
			}

			if(word.startsWith('%')){
				
				auto specialList = cw.word;
				
				if(listModel -> contextLists.contains(specialList)){
					listModel -> baselist << listModel -> contextLists.value(specialList);
					auto middle = listModel -> baselist.end() - listModel -> contextLists.value(specialList).length();
					std::inplace_merge(listModel -> baselist.begin(),middle,listModel -> baselist.end());
				}
			} else {

				// nothing special, simply add

                const auto it = std::lower_bound(listModel -> baselist.begin(),listModel -> baselist.end(),cw);

				// keep sorting

				listModel -> baselist.insert(it,cw);
			}
		}
	}

	if(forcedSpecialOption){
		listModel -> baselist = listModel -> contextLists.value(workingDir);
		handled = true;
	}

	if(!handled){
		
		listModel -> baselist = (flags & CF_NORMAL_TEXT)
			? listModel -> wordsText
			: listModel -> wordsCommands;

		auto list = listModel -> baselist;
		
		list << listModel -> wordsAbbrev;

		const auto middle = list.end() - listModel -> wordsAbbrev.length();
		std::inplace_merge(list.begin(),middle,list.end());
	}

	if(editor -> currentPlaceHolder() >= 0 && editor -> currentPlaceHolder() < editor -> placeHolderCount()) {
		
		const auto placeholder = editor -> getPlaceHolder(editor -> currentPlaceHolder());
		
		if(placeholder.mirrors.count() > 0)
			listModel -> setEnvironMode(true);
	}

	if(c.previousChar() != '\\' || (flags & CF_FORCE_VISIBLE_LIST)){

		int start = c.columnNumber() - 1;
		
		if(flags & CF_NORMAL_TEXT)
			start = 0;
		
		if(flags & CF_FORCE_GRAPHIC)
			start = 0;

		QString eow = "~!@#$%^&*()_+{}|:\"<>?,./;[]-= \n\r`+�\t";

		// the prohibited chars in bibtex keys are not well documented and differ among bibtex tools
		// this is what JabRef uses (assuming they have a good understanding due to the maturity of the project):

		if(flags & CF_FORCE_CITE)
			eow = "\n\r\t #{}\\\"~,^'";

		if(flags & CF_FORCE_GRAPHIC){
			eow.remove('/');
			eow.remove('\\');
			eow.remove('.');
			eow.remove(':');
			eow.remove('_');
		}

		if(flags & CF_FORCE_PACKAGE){
			eow.remove('_');
            eow.remove('-');
		}
		
		if(flags == CF_FORCE_VISIBLE_LIST)
			eow.remove('{');

		if(flags & CF_FORCE_REF)
			eow = "[]{}\\";
		
		if(flags & CF_FORCE_REFLIST)
			eow = "[]{}\\,";
		
		QString lineText = c.line().text();
        int i = c.columnNumber() - 1;
        

		// avoid crash

		if(i > lineText.length())
            i = lineText.length() - 1;

        for(;i >= 0;i--){

			if((lineText.at(i) == QChar('\\')) && !(flags & CF_FORCE_GRAPHIC)){
				start = i;
				break;
			}
			
			if(eow.contains(lineText.at(i))){
				
				if(
					(flags & CF_NORMAL_TEXT) ||
					(flags & CF_FORCE_GRAPHIC) ||
					(flags & CF_FORCE_CITE) ||
					(flags & CF_FORCE_REF) ||
					(flags & CF_FORCE_PACKAGE) ||
					(flags & CF_FORCE_KEYVAL) ||
					(flags & CF_FORCE_SPECIALOPTION)
				) start = i + 1;
				
				break;
			}
		}
		
		QString path;

		if(flags & CF_FORCE_GRAPHIC){

			QString fn = lineText.mid(start, c.columnNumber() - start);
            int lastIndex = fn.lastIndexOf('/');
			
			if(lastIndex >= 0)
				start = start + lastIndex + 1;
		
			if(fn.isEmpty())
                fn = workingDir + '/';

            QFileInfo fi(QDir(workingDir),fn);
			path = fi.absolutePath();
		}

		completerInputBinding -> bindTo(editor,this,true,start);

		adjustWidget();

		if((flags & CF_FORCE_GRAPHIC) && !path.isEmpty())
			emit setDirectoryForCompletion(path);

	} else {
		completerInputBinding -> bindTo(editor,this,false,c.columnNumber() - 1);
	} 


	// if prefered list is empty, take next more extensive one

	if(completerInputBinding -> getMostUsed() == 1 && countWords() == 0) { 
        setTab(0); // typical
		adjustWidget();
	}

	if(completerInputBinding -> getMostUsed() && countWords() == 0){
        setTab(3); // all
		adjustWidget();
	}

	if(completerInputBinding -> getMostUsed() == config -> preferedCompletionTab)
		completerInputBinding -> setMostUsed(config -> preferedCompletionTab,false);

	connect(editor,SIGNAL(cursorPositionChanged()),this,SLOT(cursorPositionChanged()));

	completerInputBinding -> autoOverridenText = (flags & CF_OVERRIDEN_BACKSLASH) 
		? "\\"
		: "";

	// only complete common prefix if the completer was visible when called

    if(config && config -> completeCommonPrefix && alreadyActive)
		completerInputBinding->completeCommonPrefix();
}

void LatexCompleter::directoryLoaded(QString,QSet<QString> content){

	listModel -> setBaseWords(content,CT_NORMALTEXT);
	listModel -> baselist = listModel -> wordsText;

    completerInputBinding -> setMostUsed(3); // all

	adjustWidget();
}

bool LatexCompleter::acceptTriggerString(const QString & trigger){
	return trigger == '\\' && (!config || config -> enabled);
}

void LatexCompleter::setConfig(LatexCompleterConfig * config){
	
	REQUIRE(config);
	
	this -> config = config;
	listModel -> setConfig(config);

	QFont font(QApplication::font());
	font.setPointSize(font.pointSize() * config -> tabRelFontSizePercent / 100);
	
	tbAbove -> setFont(font);
	tbBelow -> setFont(font);
}

LatexCompleterConfig *LatexCompleter::getConfig() const {
	return config;
}

void LatexCompleter::setPackageList(std::set<QString> * packages){
	packageList = packages;
}

int LatexCompleter::countWords(){
	return listModel -> getWords().count();
}

void LatexCompleter::setTab(int index){

    Q_ASSERT(index >= 0 && index < 4);
	
	if(tbBelow -> isVisible())
		tbBelow -> setCurrentIndex(index);
	
	if(tbAbove -> isVisible())
		tbAbove -> setCurrentIndex(index);
}


void LatexCompleter::filterList(QString word,int showMostUsed){
	
	QString cur = ""; //needed to preserve selection
	
	if(list -> isVisible() && list -> currentIndex().isValid())
		cur = list 
			-> model() 
			-> data(list -> currentIndex(),Qt::DisplayRole)
			.  toString();
	
	const auto type = (forcedLength)
		? CodeSnippet::length
		: CodeSnippet::none;
	
	listModel -> filterList(word,showMostUsed,false,type);
	
	if(cur == "")
		return;
		
	int p = listModel -> getWords().indexOf(cur);
	
	if(p < 0)
		return;
	
	list -> setCurrentIndex(list -> model() -> index(p,0,QModelIndex()));
}


bool LatexCompleter::acceptChar(QChar c,int position){

	//always accept alpha numerical characters
	
	if(c >= QChar('a') && c <= QChar('z'))
		return true;
		
	if(c >= QChar('A') && c <= QChar('Z'))
		return true;
		
	if(c >= QChar('0') && c <= QChar('9'))
		return true;

	if(position <= 1)
		return false;
	
	//only accept non standard character, if one of the current words contains it

	if(listModel -> isNextCharPossible(c))
		return true; 
	
	return false;
}

void LatexCompleter::cursorPositionChanged(){

	if(completerInputBinding && completerInputBinding -> isActive()){
		completerInputBinding -> cursorPositionChanged(editor);
		return;
	}

	disconnect(editor,SIGNAL(cursorPositionChanged()),this,SLOT(cursorPositionChanged()));
}

//called when item is clicked, more important: normal (not signal/slot) called when completerbinding change selection

void LatexCompleter::selectionChanged(const QModelIndex & index){

	// Note on tooltip usage: It is not possible to hide and shortly afterwards reshow a tooltip
	// with the same text (there's some interference between the internal hideTimer of the tooltip and
	// the internal tooltip-reuse when QToolTip::showText() for the same text. As a result, the tooltip
	// does not persist. This is a Qt-Bug.
	// Workaround: we cannot QToolTip::hideText generally and QToolTip::showText as needed. Therefore we have to
	// hide the tooltip in every single exit branch of the function that does not show the tooltip.
	
	if(!index.isValid()){
		QToolTip::hideText();
		return;
	}
	
	if(!isVisible()){
		QToolTip::hideText();
		return;
	}


	// picture preview even if help is disabled (maybe the same for cite/ref ?)

	if(config -> tooltipPreview && forcedGraphic){ 
		const auto path = workingDir + QDir::separator() + listModel->words[index.row()].word;
		QToolTip::hideText();
		emit showImagePreview(path);
		return;
	}

	if(index.row() < 0){
		QToolTip::hideText();
		return;
	}

	if(index.row() >= listModel -> words.size()){
		QToolTip::hideText();
		return;
	}

	if(config -> tooltipPreview && forcedSpecialOption && workingDir == "%color"){
		QToolTip::hideText();
		QString text;
		text = QString("{\\color{%1} \\rule{1cm}{1cm}}").arg(listModel -> words[index.row()].word);
		emit showPreview(text);
		return;
	}

	QRegExp wordrx("^\\\\([^ {[*]+|begin\\{[^ {}]+)");
	
	if(!forcedCite && wordrx.indexIn(listModel->words[index.row()].word) == -1){
		QToolTip::hideText();
		return;
	}

	QString cmd = wordrx.cap(0);
	QString topic;

	if(config -> tooltipPreview && latexParser.possibleCommands["%ref"].contains(cmd)){

		QString value = listModel -> words[index.row()].word;
		
		int i = value.indexOf("{");
		value.remove(0, i + 1);

		i = value.indexOf("}");
		value = value.left(i);
		
		auto document = qobject_cast<LatexDocument *>(editor -> document());
		
		int cnt = document -> countLabels(value);
		topic = "";

		if(cnt == 0){
			topic = tr("label missing!");
		} else
		if(cnt > 1){
			topic = tr("label defined multiple times!");
		} else {

			auto result = document -> getLabels(value);
            
			auto mLine = result
				.keys()
				.constFirst();
			
			int l = mLine
				-> document()
				-> indexOf(mLine);

			if(mLine -> document() != editor -> document()) {
				
				auto doc = qobject_cast<LatexDocument *>(mLine -> document());

				Q_ASSERT_X(doc, "missing latexdoc", "qdoc is not latex document !");
				
				if(doc)
					topic = tr("<p style='white-space:pre'><b>Filename: %1</b>\n").arg(doc -> getFileName());
			}

			for(int i = qMax(0,l - 2);i < qMin(mLine -> document() -> lines(),l + 3);i++){
				
				const auto text = mLine
					-> document()
					-> line(i)
					.  text();

				topic += text.left(80);

				if(text.length() > 80)
					topic += "...";
				
				if(i < l + 2)
					topic += '\n';
			}
		}
	} else
	if(config -> tooltipPreview && (forcedCite || latexParser.possibleCommands["%cite"].contains(cmd))){

		QToolTip::hideText();
		QString value = listModel -> words[index.row()].word;
        
		const QRegularExpression re{"{([^}]+?)}"};
        const QRegularExpressionMatch match = re.match(value);
        
		if(match.hasMatch())
            value = match.captured(1);

        auto document = qobject_cast<LatexDocument *>(editor -> document());
		
		if(!bibReader){
			bibReader = new BibTex::Reader(this);
			connect(bibReader,SIGNAL(sectionFound(QString)),this,SLOT(bibtexSectionFound(QString)));
            connect(this,SIGNAL(searchBibtexSection(QString,QString)),bibReader,SLOT(searchSection(QString,QString)));
			bibReader -> start();
		}

		QString file = document -> findFileFromBibId(value);
		
		if(!file.isEmpty())
			emit searchBibtexSection(file,value);
		
		return;
	} else {
		
		if(latexReference && config -> tooltipHelp)
			topic = latexReference -> tooltipText(cmd);
		
		if(topic.isEmpty()){
			QToolTip::hideText();
			return;
		}
	}

	showTooltip(topic);
}


void LatexCompleter::showTooltip(QString text){

	if(isVisible()){

		auto index = list -> currentIndex();
		auto rect = list -> visualRect(index);
		const auto cursor = editor -> cursor();
		const auto lineHeight = cursor.line().document() -> getLineSpacing();
		auto position = list -> mapToGlobal(QPoint(list -> width(),rect.top() - lineHeight));

		showTooltipLimited(position,text,list -> width());

		return;
	}

	QToolTip::hideText();
}

void LatexCompleter::editorDestroyed(){
    editor = nullptr;
}

void LatexCompleter::bibtexSectionFound(QString content){
	showTooltip(content);
}

//ends completion (closes the list) and returns true if there was any

bool LatexCompleter::close(){

	if(completerInputBinding -> isActive()){
		completerInputBinding -> simpleRestoreAutoOverride();
		completerInputBinding -> resetBinding();
		widget -> setVisible(false);
		listModel -> curWord = "";
		return true;
	}
	
	return false;
}

void LatexCompleterConfig::setFiles(const QStringList & newFiles){
	files = newFiles;
}

const QStringList & LatexCompleterConfig::getLoadedFiles() const {
	return files;
}

bool LatexCompleter::existValues(){
	return listModel->keyValLists.value(workingDir).size() > 0;
}
