#include "inc/descriptors.h"

namespace mopa
{
descriptor* descriptor_factory(uint8_t tag)
{
	descriptor* dst=NULL;
	switch(tag)
	{
		case 0x41:  dst=new service_list_descriptor(); break;
		case 0x44:	dst=new cable_delivery_system_descriptor(); break;
		case 0x70:	dst=new adaptation_field_data_descriptor(); break;

		default:	dst=new unknown_descriptor(); break;
	}
	return dst;
}

descriptor_vector::~descriptor_vector()
{
	purge();
}
descriptor_vector::descriptor_vector()
{
};
descriptor_vector::descriptor_vector(const descriptor_vector& x)
{
	purge();
	for(size_t i=0;i<x.size();i++)
		push_back(x[i]->dup());
};
descriptor_vector& descriptor_vector::operator=(descriptor_vector const &x)
{
	if(&x!=this)
	{
		purge();
		for(size_t i=0;i<x.size();i++)
			push_back(x[i]->dup());
	}
	return *this;
	//printf("=\n");return *this;
};

void descriptor_vector::io(iox& x)
{
	if(x.is_parsing())
	{
		purge();
		while(x.block_size_left()>0)
		{
			//read tag ahead
			uint8_t tag;
			x.uint(8,DVB_VAR(tag));
			descriptor* dsc=descriptor_factory(tag);
			push_back(dsc);
			dsc->tag=tag;
			back()->io(x);
		}
	}
	else
	{
		for(size_t i=0;i<size();i++)
		{
			this->operator[](i)->io(x);
		}
	}
}
void descriptor_vector::purge()
{
	for(int i=0;i<size();i++)
		delete this->operator[](i);
	clear();
}



}



