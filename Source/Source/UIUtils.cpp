#include "utilsUI.h"
#include "utilsSystem.h"
#include "utilsVersion.h"


#include "Dialogs/File.hpp"


#include <QErrorMessage>



extern void hideSplash();


namespace UtilsUi {

	/*!
	* \brief show confirmation message box
	* \param message
	* \return yes=true
	*/

	bool txsConfirm(const QString & message){
		
		hideSplash();

		return QMessageBox::question(QApplication::activeWindow(),TEXSTUDIO,message,QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes) == QMessageBox::Yes;
	}


	/*!
	* \brief show confirmation with warning message box
	* \param message
	* \return yes=true
	*/
	
	bool txsConfirmWarning(const QString & message){

		hideSplash();
		
		return QMessageBox::warning(QApplication::activeWindow(),TEXSTUDIO,message,QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes) == QMessageBox::Yes;
	}


	/*!
	* \brief show confirmation with warning message box
	* \param message
	* \param rememberChoice if true, return true and avoid Msg. rememberChoice is overwritten by checkbox result.
	* \return yes=true
	*/

	bool txsConfirmWarning(const QString & message,txsWarningState & rememberChoice){
		
		switch (rememberChoice){
		case RememberFalse : return false ;
		case RememberTrue : return true ;
		}
		
		hideSplash();
		
		QMessageBox msg(QMessageBox::Warning,TEXSTUDIO,message,QMessageBox::Yes | QMessageBox::No,QApplication::activeWindow());

		auto box = new QCheckBox(QApplication::tr("Remember choice ?"));
		msg.setCheckBox(box);

		bool result = (msg.exec() == QMessageBox::Yes);

		const bool remember = msg.checkBox() 
			-> checkState();
		
		rememberChoice = (remember == Qt::Checked)
			? RememberTrue
			: RememberFalse ;

		return result;
	}


	/*!
	* \brief show confirmation with warning message box
	* Here the available buttons can be set.
	* \param message
	* \param buttons
	* \return pressed button
	*/
	
	QMessageBox::StandardButton txsConfirmWarning(const QString & message,QMessageBox::StandardButtons buttons){
		
		hideSplash();
		
		return QMessageBox::warning(QApplication::activeWindow(),TEXSTUDIO,message,buttons,QMessageBox::Yes);
	}


	/*!
	* \brief show information message pop-up
	* \param message
	*/

	void txsInformation(const QString & message){

		hideSplash();
		
		QMessageBox::information(QApplication::activeWindow(),TEXSTUDIO,message,QMessageBox::Ok);
	}


	/*!
	* \brief show warning message pop-up
	* \param message
	*/

	void txsWarning(const QString & message){

		hideSplash();

		QMessageBox::warning(QApplication::activeWindow(),TEXSTUDIO,message,QMessageBox::Ok);
	}


	/*!
	* \brief show warning message pop-up
	* \param message
	* \param noWarnAgain
	*/

	void txsWarning(const QString & message,bool & noWarnAgain){

		hideSplash();

		QMessageBox msgBox(QMessageBox::Warning,TEXSTUDIO,message,QMessageBox::Ok,QApplication::activeWindow());

		QCheckBox cbNoWarnAgain(QCoreApplication::translate("Texstudio","Do not warn again.","General warning dialog"),& msgBox);
		cbNoWarnAgain.setChecked(noWarnAgain);
		cbNoWarnAgain.blockSignals(true); // quick hack to prevent closing the message box
		
		msgBox.addButton(& cbNoWarnAgain,QMessageBox::ActionRole);
		msgBox.exec();

		noWarnAgain = cbNoWarnAgain.isChecked();
	}


	/*!
	* \brief show critical-error message box
	* \param message
	*/
	
	void txsCritical(const QString & message){
		
		hideSplash();
		
		QMessageBox::critical(QApplication::activeWindow(),TEXSTUDIO,message,QMessageBox::Ok);
	}


	QToolButton *createComboToolButton(
		QWidget * parent,
		const QStringList & list,
		const QList<QIcon> & icons,
		int height,
		const QObject * receiver,
		const char * member,
		int defaultIndex,
		QToolButton * combo
	){
		Q_UNUSED(icons)
	
		const auto & fm = parent -> fontMetrics();
	
		if(height == -1)
			height = 0;
		else 
		if(height == 0) {
			if(parent -> property("innerButtonHeight").isValid())
				height = parent -> property("innerButtonHeight").toInt();
			else {
				height = parent -> height() - 2;
				parent -> setProperty("innerButtonHeight", height);
			}
		}

		if(combo == nullptr)
			combo = new QToolButton(parent);
		
		if(height != 0)
			combo -> setMinimumHeight(height);

		combo -> setPopupMode(QToolButton::MenuButtonPopup);
		combo -> setToolButtonStyle(Qt::ToolButtonTextOnly);

		// remove old actions
		foreach (QAction *mAction, combo->actions())
			combo->removeAction(mAction);

		auto mMenu = new QMenu(combo);
		
		int max = 0;
		bool defaultSet = false;
		
		for(int i = 0;i < list.length();i++){

			QString text = list[i];
			auto mAction = mMenu -> addAction(text,receiver,member);
			max = qMax(max,getFmWidth(fm,text + "        "));
			
			if(i == defaultIndex){
				combo -> setDefaultAction(mAction);
				defaultSet = true;
			}
		}

		if(!defaultSet){
			if(list.isEmpty())
				combo -> setDefaultAction(new QAction("<" + QApplication::tr("none") + ">", combo));
			else
				combo -> setDefaultAction(mMenu -> actions().at(0));
		}

		combo -> setMinimumWidth(max);
		combo -> setMenu(mMenu);
		
		return combo;
	}


	QToolButton * comboToolButtonFromAction(QAction * action){
		
		if(!action)
			return nullptr;
		
		auto button = qobject_cast<QToolButton *>(action -> parent());
		
		if(!button){
			
			auto menu = qobject_cast<QMenu *>(action -> parent());
			
			if(!menu)
				return nullptr;
			
			button = qobject_cast<QToolButton *>(menu -> parent());
			
			if(!button)
				return nullptr;
		}
		
		return button;
	}


	QToolButton * createToolButtonForAction(QAction * action){
		
		auto tool = new QToolButton();
		
		if(!action -> icon().isNull())
			tool -> setIcon(action -> icon());
		else
			tool -> setText(action -> text());
		
		tool -> setToolTip(action -> toolTip());
		tool -> setCheckable(action -> isCheckable());
		tool -> setChecked(action -> isChecked());
		tool -> connect(tool,SIGNAL(clicked(bool)),action,SLOT(setChecked(bool)));
		tool -> connect(action,SIGNAL(toggled(bool)),tool,SLOT(setChecked(bool)));
		
		return tool;
	}


	void setSubtreeExpanded(QTreeView * view,QModelIndex index,bool expand){

		for(int row = 0;;row++){
			
			const auto node = view
				-> model()
				-> index(row,0,index);
			
			if(!node.isValid())
				break;

			setSubtreeExpanded(view,node,expand);
		}
		
		view -> setExpanded(index,expand);
	}


	void setSubtreeExpanded(QTreeWidgetItem * item,bool expand){
		
		item -> setExpanded(expand);
		
		for(int i = 0;i < item -> childCount();++i)
			setSubtreeExpanded(item -> child(i),expand);
	}


	bool browse(
		QWidget * widget,
		const QString & title,
		const QString & extension,
		const QString & startPath,
		bool list
	){
		auto lineEdit = qobject_cast<QLineEdit *>(widget);
		auto comboBox = qobject_cast<QComboBox *>(widget);

		REQUIRE_RET(lineEdit || comboBox,false);

		QString oldpath = lineEdit 
			? lineEdit -> text() 
			: comboBox -> currentText();

		QString path = oldpath;
	
		if(list && !path.isEmpty())
			path = path
				.split(':')
				.last();
		
		if(path.startsWith('"'))
			path.remove(0,1);
		
		if(path.endsWith('"'))
			path.chop(1);
		
		if(path.isEmpty())	
			path = startPath;
		
		if(extension == "/")
			path = QFileDialog::getExistingDirectory(nullptr,title,path);
		else
			path = FileDialog::getOpenFileName(nullptr,title,path,extension,nullptr,QFileDialog::DontResolveSymlinks);
		
		if(!path.isEmpty()){
			
			path = QDir::toNativeSeparators(path);
			
			if(list && !oldpath.isEmpty())
				path = oldpath + ':' + path;
			
			if(lineEdit)
				lineEdit -> setText(path);
			
			if(comboBox)
				comboBox -> setEditText(path);
			
			return true;
		}
		
		return false;
	}


	QColor colorFromRGBAstr(const QString & hex,QColor fallback){

		QString color = hex;
		
		if(color.startsWith('#'))
			color = color.mid(1);
		
		if(color.length() != 8)
			return fallback;

		bool r,g,b,a;

		if(r && g && b && a)
			return QColor(
				color.mid(0,2).toInt(&r,16),
				color.mid(2,2).toInt(&g,16),
				color.mid(4,2).toInt(&b,16),
				color.mid(6,2).toInt(&a,16)
			);

		return fallback;
	}


	/*!
	Return a color with a more medium value. The amount of change is determined by factor. I.e. for a factor > 100
	a dark color becomes lighter, a light color becomes darker. The reverse is true for factors < 100;
	This is a generalization of QColor.lighter()/darker() which should provide a reasonable change in dark as well
	as light contexts.
	*/

	QColor mediumLightColor(QColor color,int factor){

		// special handling for black because lighter() does not work there [QTBUG-9343].

		if(color.value() == 0){

			factor = qMax(0,factor - 100);

			// i.e. QColor(50, 50, 50) for factor 150

			return QColor(factor,factor,factor);
		}

		return (color.value() < 128)
			? color.lighter(factor)
			: color.darker(factor) ;
	}


	/*!
	* return the window to which an object belongs or the given fallback widget
	* Note: the signature is intentionally for a gerneric QObject, so that we can
	* simply call windowForObject(sender()).
	*/

	QWidget * windowForObject(QObject * object,QWidget * fallback){
	
		QWidget * widget;
		
		auto action = qobject_cast<QAction *>(object);
		
		widget = (action)
			? action -> parentWidget()
			: qobject_cast<QWidget *>(object) ;

		if(widget)
			widget = widget -> window();

		return (widget)
			? widget
			: fallback;
	}


	/*!
		interal
		guesses a descriptive text from a text suited for a menu entry
		This is copied from the internal function qt_strippedText() in qaction.cpp
	*/

	static QString strippedActionText(QString string){

		string.remove(QString::fromLatin1("..."));
		
		for(int i = 0;i < string.size();++i)
			if(string.at(i) == QLatin1Char('&'))
				string.remove(i,1);
		
		return string.trimmed();
	}


	/*!
	interal
	Adds possible shortcut information to the tooltip of the action.
	This provides consistent behavior both with default and custom tooltips
	when used in combination with removeShortcutToToolTip()
	*/
	
	void addShortcutToToolTip(QAction * action){

		if(action -> shortcut().isEmpty())
			return;

		if(action -> property("hasShortcutToolTip").toBool())
			return;

		auto tooltip = action 
			-> property("tooltipBackup")
			.  toString();
		
		if(tooltip.isEmpty()){
			
			tooltip = action -> toolTip();

			// action uses a custom tooltip. Backup so that we can restore it later.

			if(tooltip != strippedActionText(action -> text()))
				action -> setProperty("tooltipBackup",action -> toolTip());
		}

		auto shortcutTextColor = QApplication::palette()
			.color(QPalette::ToolTipText);
		
		QString shortCutTextColorName;

		if(shortcutTextColor.value() == 0)
			// special handling for black because lighter() does not work there [QTBUG-9343].
			shortCutTextColorName = "gray";
		else {
			int factor = (shortcutTextColor.value() < 128) ? 150 : 50;
			shortCutTextColorName = shortcutTextColor.lighter(factor).name();
		}

		action -> setToolTip(QString("<p style='white-space:pre'>%1&nbsp;&nbsp;<code style='color:%2; font-size:small'>%3</code></p>")
			.arg(tooltip,shortCutTextColorName,action -> shortcut().toString(QKeySequence::NativeText)));
		
		action -> setProperty("hasShortcutToolTip",true);
	}


	/*!
	interal
	Removes possible shortcut information from the tooltip of the action.
	This provides consistent behavior both with default and custom tooltips
	when used in combination with addShortcutToToolTip()
	*/

	void removeShortcutFromToolTip(QAction * action){

		const auto tooltip = action 
			-> property("tooltipBackup")
			.  toString();

		action -> setToolTip(tooltip);
		action -> setProperty("tooltipBackup",QVariant());
		action -> setProperty("hasShortcutToolTip",false);
	}


	/*!
	Adds or removes shortcut information from the tooltip of the action.
	This provides consistent behavior both with default and custom tooltips.
	*/

	void updateToolTipWithShortcut(QAction * action,bool showShortcut){

		if(showShortcut)
			addShortcutToToolTip(action);
		else
			removeShortcutFromToolTip(action);
	}


	/*!
	* \brief Adds support for a single-finger panning gesture to the widget using a QScroller.
	* \note ItemViews will be switched to ScrollPerPixel scrollMode.
	*/

	void enableTouchScrolling(QWidget * widget,bool enable){

		#if !defined(Q_OS_MAC)

			if(enable){
				
				auto view = qobject_cast<QAbstractItemView *>(widget);

				// QScroller needs per pixel scrolling otherwise distances and speed
				// are calculated incorrectly.

				if(view)
					view -> setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

				QScroller::grabGesture(widget,QScroller::TouchGesture);
			} else {
				QScroller::ungrabGesture(widget);
			}

		#endif
	}


	/*!
	* Resize a window with width and height given in units of the application font height.
	* This allows to scale dialogs to a size that you'll get almost the same context visible
	* independent of UI scaling parameters.
	*/

	void resizeInFontHeight(QWidget * widget,int width,int height){

		const int fontHeight = qApp 
			-> fontMetrics()
			.  height();
		
		const auto rectangle = widget 
			-> screen()
			-> availableGeometry();
		
		const auto size = QSize(
			qMin(fontHeight * width,rectangle.width()),
			qMin(fontHeight * height,rectangle.height())
		);

		widget -> resize(size);
	}


	/*!
	* \brief Given font metrics return pixel size of a character
	* \param[in] fm Font metrics
	* \param[in] ch Character
	* \returns Returns pixel size of character
	*/
	
	qreal getFmWidth(const QFontMetricsF & metrics,QChar c){
		return metrics.horizontalAdvance(c);
	}


	/*!
	* \brief Given font metrics return pixel size of a character
	* \param[in] fm Font metrics
	* \param[in] ch Character
	* \returns Returns pixel size of character
	*/

	int getFmWidth(const QFontMetrics & metrics,QChar c){
		return metrics.horizontalAdvance(c);
	}


	/*!
	* \brief Given font metrics return pixel size of a text string
	* \param[in] fm Font metrics
	* \param[in] text Text string
	* \param[in] len Only calculate width of the first len characters of the string. If not specified,
	* then -1 is assumed which means calculate width of the whole string.
	* \returns Returns pixel size of the text string
	*/

	qreal getFmWidth(const QFontMetricsF & metrics,const QString & text,int length){
		return metrics.horizontalAdvance(text,length);
	}


	/*!
	* \brief Given font metrics return pixel size of a text string
	* \param[in] fm Font metrics
	* \param[in] text Text string
	* \param[in] len Only calculate width of the first len characters of the string. If not specified,
	* then -1 is assumed which means calculate width of the whole string.
	* \returns Returns pixel size of the text string
	*/

	int getFmWidth(const QFontMetrics & metrics,const QString & text,int length){
		return metrics.horizontalAdvance(text,length);
	}


	/*!
	* \brief Return the screen geometry for a given point
	* \param[in] pos Position
	* \returns The screen geometry at the given point
	*/

	QRect getAvailableGeometryAt(const QPoint & position){

		const auto screen = QGuiApplication::screenAt(position);
		
		return (screen)
			? screen -> availableGeometry()
			: QRect();
	}
}
