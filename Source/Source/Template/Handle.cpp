
#include "templatemanager.h"
#include "Template/Template.hpp"


TemplateHandle::TemplateHandle(const TemplateHandle & handle) 
    : m_tmpl(nullptr) { 
        setTmpl(handle.m_tmpl);
    }


TemplateHandle::TemplateHandle(Template * templet)
    : m_tmpl(nullptr) { 
        setTmpl(templet);
    }


TemplateHandle::~TemplateHandle(){
    setTmpl(nullptr);
}


bool TemplateHandle::isValid() const {
    return m_tmpl;
}


TemplateHandle & TemplateHandle::operator = (const TemplateHandle & handle){
	setTmpl(handle.m_tmpl);
	return * this;
}


QString TemplateHandle::name() const {
    return (m_tmpl) 
        ? m_tmpl -> name() 
        : "" ;
}

QString TemplateHandle::description() const {
    return (m_tmpl) 
        ? m_tmpl -> description() 
        : "" ;
}


QString TemplateHandle::author() const {
    return (m_tmpl) 
        ? m_tmpl -> author() 
        : "" ;
}


QString TemplateHandle::version() const {
    return (m_tmpl) 
        ? m_tmpl -> version() 
        : "" ;
}


QDate TemplateHandle::date() const {
    return (m_tmpl) 
        ? m_tmpl -> date() 
        : QDate() ;
}


QString TemplateHandle::license() const {
    return (m_tmpl) 
        ? m_tmpl -> license() 
        : "" ;
}


QPixmap TemplateHandle::previewImage() const {
    return (m_tmpl) 
        ? m_tmpl -> previewImage() 
        : QPixmap() ;
}


QString TemplateHandle::file() const {
    return (m_tmpl) 
        ? m_tmpl -> file() 
        : "" ;
}


bool TemplateHandle::isEditable() const {
    return (m_tmpl) 
        ? m_tmpl -> isEditable() 
        : false ;
}

bool TemplateHandle::isMultifile() const {
    return (m_tmpl) 
        ? m_tmpl -> isMultifile() 
        : false ;
}

bool TemplateHandle::createInFolder(const QString & path) const {
    return (m_tmpl) 
        ? m_tmpl -> createInFolder(path) 
        : false ;
}


QStringList TemplateHandle::filesToOpen() const {
    return (m_tmpl) 
        ? m_tmpl -> filesToOpen() 
        : QStringList();
}


void TemplateHandle::setTmpl(Template * templet){
	
    if(m_tmpl)
        m_tmpl -> deref(this);
	
	m_tmpl = templet;

	if(m_tmpl)
        m_tmpl -> ref(this);
}
