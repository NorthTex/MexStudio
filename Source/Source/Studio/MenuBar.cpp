

#include "TexStudio.hpp"

#include <functional>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>


const auto appIcon = ":appicon";
const auto documentView = "main/view/documents";

void Texstudio::updateToolBarMenu(const QString & name){

    const auto menu = configManager.getManagedMenu(name);

    if(!menu)
        return;


	using UtilsUi::createComboToolButton;
	using boost::adaptors::transformed;
	using boost::adaptors::filtered;
	using std::mem_fn;


	auto view = currentEditorView();

	const auto hasActions = [ & ](auto & toolbar){
		return toolbar.toolbar && toolbar.actualActions.contains(name);
	};

	const auto toToolbar = mem_fn(& ManagedToolBar::toolbar);

	const auto inMenu = [ & ](const auto & element){

		const auto menuId = element
			-> property("menuID")
			.  toString();

		return menuId == name;
	};

	const auto toButton = [](auto & element){
		return qobject_cast<QToolButton *>(element);
	};

	const auto notSeparator = [](auto & action){
		return ! action -> isSeparator();
	};

	const auto makeComboButton = [ & ](auto * toolbar,QToolButton * button){

		QVector<QString> text;
		QList<QIcon> icons;
		int defaultIndex = -1;

		for(const auto action : button
			-> actions()
			|  filtered(notSeparator)
		){

			text.append(action -> text());
			icons.append(action -> icon());

			if(name != documentView)
				continue;

			const auto actionView = action
				-> data()
				.  value<LatexEditorView *>();

			if(view == actionView)
				defaultIndex = text.length() - 1;
		}

		createComboToolButton(
			toolbar,text,icons,-1,
			this,SLOT(callToolButtonAction()),
			defaultIndex,button
		);
	};

	for(auto toolbar : configManager
		.managedToolBars
		| filtered(hasActions)
		| transformed(toToolbar)
	){
		for(auto button : toolbar 
			-> children()
			|  filtered(inMenu)
			|  transformed(toButton)
		){
			REQUIRE(button);
			makeComboButton(toolbar,button);
		}
	}
}



/*!
 *  \brief set-up all tool-bars
 */
void Texstudio::setupToolBars(){

	/*
	 * This method will be called multiple
	 * times and must not create something
	 * if this something already exists
	 */

	configManager.watchedMenus.clear();

	//customizable toolbars
	//first apply custom icons

	const auto & menus = configManager.replacedIconsOnMenus;

	const auto
		begin = menus.constBegin(),
		end = menus.constEnd();

	auto i = begin;

	while(i != end){

		QString id = i.key();
		QString iconFilename = configManager.parseDir(i.value().toString());

		auto obj = configManager.menuParent -> findChild<QObject *>(id);
		auto act = qobject_cast<QAction *>(obj);

		if(act && !iconFilename.isEmpty())
			act -> setIcon(QIcon(iconFilename));

		++i;
	}

	//setup customizable toolbars

	const auto createToolBar = [=](auto & managed){

		const auto name = managed.name;

		managed.toolbar = (name == "Central")
			? centralToolBar
			: addToolBar(tr(qPrintable(name)));

		managed.toolbar -> setObjectName(name);
		addAction(managed.toolbar -> toggleViewAction());

		if(name == "Spelling")
			addToolBarBreak();
	};

	const auto setupToolBar = [=](auto & managed){

		if(managed.toolbar)
			managed.toolbar -> clear();
		else
			createToolBar(managed);

		auto & toolbar = managed.toolbar;

		for(const auto & type : managed.actualActions){

			if(type == "separator"){
				toolbar -> addSeparator();
				continue;
			}

			if(type.startsWith("tags/")){

				int tagCategorySep = type.indexOf("/",5);
				auto tagsWidget = findChild<XmlTagsListWidget *>(type.left(tagCategorySep));

				if(!tagsWidget)
					continue;

				if(!tagsWidget -> isPopulated())
					tagsWidget -> populate();

				QStringList list = tagsWidget -> tagsTxtFromCategory(type.mid(tagCategorySep + 1));

				if(list.isEmpty())
					continue;

				auto combo = UtilsUi::createComboToolButton(
					managed.toolbar,
					list,
					QList<QIcon>(),
					0,
					this,
					SLOT(insertXmlTagFromToolButtonAction())
				);

				combo -> setProperty("tagsID",type);
				managed.toolbar -> addWidget(combo);

				continue;
			}


			auto object = configManager.menuParent -> findChild<QObject *>(type);
			auto action = qobject_cast<QAction *>(object);


			if(action){

				if(action -> icon().isNull())
					action -> setIcon(QIcon(appIcon));

				UtilsUi::updateToolTipWithShortcut(action,configManager.showShortcutsInTooltips);
				managed.toolbar -> addAction(action);

				continue;
			}


			auto menu = qobject_cast<QMenu *>(object);

			if(menu){

				configManager.watchedMenus << type;

				QList<QIcon> icons;
				QStringList list;

				for(const auto action : menu -> actions()){

					if(action -> isSeparator())
						continue;

					list.append(action -> text());
					icons.append(action -> icon());
				}

				/*
				 * TODO:
				 * Is the callToolButtonAction() - slot really needed?
				 * Can't we just add the menu itself as
				 * the menu of the qtoolbutton, without
				 * creating a copy? (should be much faster)
				 */

				auto combo = UtilsUi::createComboToolButton(
					managed.toolbar,list,icons,0,this,SLOT(callToolButtonAction()));

				combo -> setProperty("menuID",type);
				managed.toolbar -> addWidget(combo);

				continue;
			}



			qWarning("Unknown toolbar command %s", qPrintable(type));
		}

		if(managed.actualActions.empty())
			managed.toolbar -> setVisible(false);
	};


	for(auto & managed : configManager.managedToolBars)
		setupToolBar(managed);
}
