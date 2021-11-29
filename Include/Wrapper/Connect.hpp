#ifndef Header_Wrapper_Connect
#define Header_Wrapper_Connect

#define wire(component,signal,slot) \
	connect(component,SIGNAL(signal),this,SLOT(slot))

#define unwire(component,signal,slot) \
	disconnect(component,SIGNAL(signal),this,SLOT(slot))

#endif