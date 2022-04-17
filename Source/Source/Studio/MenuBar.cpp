

#include "TexStudio.hpp"

#include <functional>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>


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
