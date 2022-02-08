
#include "Latex/Structure.hpp"


StructureEntryIterator::StructureEntryIterator(StructureEntry * entry){
	
	if(!entry)
		return;
	
	while(entry -> parent){
		entryHierarchy.prepend(entry);
		indexHierarchy.prepend(entry -> getRealParentRow());
		entry = entry -> parent;
	}

	entryHierarchy.prepend(entry);
	indexHierarchy.prepend(0);
}


bool StructureEntryIterator::hasNext(){
	return ! entryHierarchy.isEmpty();
}


StructureEntry * StructureEntryIterator::next(){

    if(!hasNext())
		return nullptr;
	
	auto entry = entryHierarchy.last();
	

	if(entry -> children.isEmpty()){
	
		// no child, go to next on same level
		
		entryHierarchy.removeLast();
		indexHierarchy.last()++;

		while(
			! entryHierarchy.isEmpty() && 
			indexHierarchy.last() >= entryHierarchy.last() -> children.size()
		){

			// doesn't exists, proceed to travel upwards
			
			entryHierarchy.removeLast();
			indexHierarchy.removeLast();
			indexHierarchy.last()++;
		}

		if(!entryHierarchy.isEmpty())
			entryHierarchy.append(entryHierarchy.last() -> children.at(indexHierarchy.last()));
	} else {

		// first child is next element, go a level deeper
	
		entryHierarchy.append(entry -> children.at(0));
		indexHierarchy.append(0);
	}

	return entry;
}

