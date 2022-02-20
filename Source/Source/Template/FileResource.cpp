
#include "Template/Local/Resource/File.hpp"


LocalFileTemplateResource::LocalFileTemplateResource(
	QString path,
	QStringList filters,
	QString name,
	QObject * parent,
	QIcon icon
) : QObject(parent)
  , AbstractTemplateResource()
  , m_path(path)
  , m_filters(filters)
  , m_name(name)
  , m_icon(icon) {}


LocalFileTemplateResource::~LocalFileTemplateResource(){
	for(auto templet : m_templates)
		delete templet;
}


QList<TemplateHandle> LocalFileTemplateResource::getTemplates(){

	QList<TemplateHandle> handles;

	for(auto templet : m_templates)
		handles.append(TemplateHandle(templet));
	
	return handles;
}


bool LocalFileTemplateResource::isAccessible(){
	const QDir directory(m_path);
	return directory.exists() && directory.isReadable();
}


void LocalFileTemplateResource::setEditable(bool editable){
	for(auto templet : m_templates)
		templet -> m_editable = editable;
}


void LocalFileTemplateResource::update(){

	for(auto templet : m_templates)
		delete templet;
	
	m_templates.clear();

	QDir directory(m_path);

	const auto files = directory.entryList(m_filters,QDir::Files | QDir::Readable,QDir::Name);

	for(auto & path : files){
		
		const auto info = QFileInfo(directory,path).absoluteFilePath();
		const auto templet = createTemplate(info);
		
		if(templet)
			m_templates.append(templet);
	}
}

