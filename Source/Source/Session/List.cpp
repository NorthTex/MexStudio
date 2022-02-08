
#include "SessionList.hpp"
#include "configmanager.h"
#include "smallUsefulFunctions.h"


SessionList::SessionList(QMenu * menu,QObject * parent) 
	: QObject(parent)
	, m_menu(menu){

	m_config = dynamic_cast<ConfigManager *>(ConfigManagerInterface::getInstance());
}


void SessionList::addFilenameToList(const QString & file){

	if(file.endsWith("lastSession.txss"))
		return;
	
	REQUIRE(m_config);
	
	auto files = m_config
		-> getOption("Files/Recent Session Files")
		.  toStringList();
	
	const auto maxFiles = m_config 
		-> getOption("Files/Max Recent Sessions")
		.  toInt();
	
	const bool changed = addMostRecent(file,files,maxFiles);
	
	if(changed){
		m_config -> setOption("Files/Recent Session Files",files);
		updateMostRecentMenu();
	}
}


/*
 * 	Updates the most recent session entries in the menu
 */

void SessionList::updateMostRecentMenu(){

	REQUIRE(m_config);
	
	const auto files = m_config
		-> getOption("Files/Recent Session Files")
		.  toStringList();
	
	REQUIRE(m_menu);
	
	const auto actions = m_menu -> actions();
	
	int i = 0;

	/* 
	 *	convention: session entries are actions with a data tag "session:<filename>"
	 */
	
	for(const auto & file : files){

		QString temp = QDir::toNativeSeparators(file);
        temp.replace("&","&&");
		
		while(
			i < actions.count() && 
			! actions[i] -> data().toString().startsWith("session")
		) i++;

		auto sessionAct = (i >= actions.count())
			? m_menu -> addAction("")
			: actions[i];

        sessionAct -> setText(temp);
		sessionAct -> setData("session:" + file);
		sessionAct -> setVisible(true);

		connectUnique(sessionAct,SIGNAL(triggered()),this,SLOT(menuActionTriggered()));

		i++;
	}

	// disable remaining unused actions

	for(int a = i + 1;a < actions.count();a++){
		
		auto action = actions[a];
		
		if(action -> data().toString().startsWith("session:"))
			action -> setVisible(false);
	}
}


void SessionList::menuActionTriggered(){

	auto action = qobject_cast<QAction *>(sender());
	
	REQUIRE(action);
    
	const auto path = action 
		-> data()
		.  toString()
		.  remove(QRegularExpression("^session:"));

	emit loadSessionRequest(path);
	addFilenameToList(path);
}
