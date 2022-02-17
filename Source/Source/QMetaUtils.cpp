#include "qmetautils.h"

void QMetaObjectInvokeMethod(
	QObject * object,
	const char * signature,
	const QList<QGenericArgument> & arguments
){
	// TODO : ask trolltech to provide an overloaded invokeMetaMethod()
	// taking a QList<> instead of nine defaulted args...

	if(arguments.isEmpty()){
		QMetaObject::invokeMethod(object,signature);
		return;
	}

	switch(arguments.count()){
	case 1:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0));
		return;
	case 2:
		QMetaObject::invokeMethod(object,signature,
		    arguments.at(0),
		    arguments.at(1));
		return;
	case 3:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0),
			arguments.at(1),
			arguments.at(2));
		return;
	case 4:
		QMetaObject::invokeMethod(object,signature,
		    arguments.at(0),
		    arguments.at(1),
			arguments.at(2),
			arguments.at(3));
		return;
	case 5:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0),
			arguments.at(1),
			arguments.at(2),
			arguments.at(3),
			arguments.at(4));
		return;
	case 6:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0),
			arguments.at(1),
			arguments.at(2),
			arguments.at(3),
			arguments.at(4),
			arguments.at(5));
		return;
	case 7:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0),
			arguments.at(1),
			arguments.at(2),
			arguments.at(3),
			arguments.at(4),
			arguments.at(5),
			arguments.at(6));
		return;
	case 8:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0),
			arguments.at(1),
			arguments.at(2),
			arguments.at(3),
			arguments.at(4),
			arguments.at(5),
			arguments.at(6),
			arguments.at(7));
		return;
	case 9:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0),
			arguments.at(1),
			arguments.at(2),
			arguments.at(3),
			arguments.at(4),
			arguments.at(5),
			arguments.at(7),
			arguments.at(8));
		return;
	case 10:
		QMetaObject::invokeMethod(object,signature,
			arguments.at(0),
			arguments.at(1),
			arguments.at(2),
			arguments.at(3),
			arguments.at(4),
			arguments.at(5),
			arguments.at(7),
			arguments.at(8),
			arguments.at(9));
		return;
	}
}


void QMetaObjectInvokeMethod(
	QObject * object, 
	const char * signature,
	const QList<QVariant> & arguments
){
	QList<QGenericArgument> temp;

	for(const auto & argument : arguments)
		temp << QGenericArgument(argument.typeName(),argument.data());

	QMetaObjectInvokeMethod(object,signature,temp);
}


QByteArray createMethodSignature(
	const char * methodName,
	const QList<QVariant> & args
){
	QByteArray signature = methodName;

	if(args.isEmpty())
		signature.append("()");
	else {
		signature.append("(");

		for(int i = 0;i < args.size();i++){

			if(i != 0)
				signature.append(",");
			
			signature.append(args[i].typeName());
		}
		
		signature.append(")");
	}

	return signature;
}


ConnectionWrapper::ConnectionWrapper(QObject * parent,QObject * receiver,const char * slot)
	: QObject(parent)
	, realReceiver(receiver)
	, realSlot(slot) {}


ConnectionWrapper::ConnectionWrapper(QObject * receiver,const char * slot) 
	: QObject(receiver)
	, realReceiver(receiver)
	, realSlot(slot) {}


void ConnectionWrapper::activated(){
	QMetaObjectInvokeMethod(realReceiver,realSlot,args);
}


void connectWithAdditionalArguments(
	QObject * sender,
	const char * signal,
	QObject * receiver,
	const char * slot,
	const QList<QVariant> & arguments
){
	auto wrapper = new ConnectionWrapper(receiver,receiver,slot);
	wrapper -> args = arguments;
	sender -> connect(sender,signal,wrapper,SLOT(activated()));
}
