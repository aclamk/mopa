#ifndef _DVB_DVBTYPES_H
#define _DVB_DVBTYPES_H

#include "inc/io.h"
#include "inc/commontypes.h"
#include <vector>
namespace mopa
{

struct descriptor
{
	uint8_t tag;
	uint8_t length;
	virtual void io(iox& x)
	{
		x.uint(8,DVB_VAR(tag));
		x.named_block_begin(8,DVB_INFO("length"));
		//switch
		length=x.named_block_end(DVB_INFO("descriptor_content"));
	}
	virtual ~descriptor(){};
	virtual descriptor* dup()=0;
};
descriptor* descriptor_factory(uint8_t tag);


struct adaptation_field_data_descriptor : public descriptor
{
	uint8_t adaptation_field_data_identifier;
	void io(iox& x)
	{
		x.named_block_begin(8,DVB_INFO("length"));
		x.uint(8,DVB_VAR(adaptation_field_data_identifier));
		length=x.named_block_end(DVB_INFO("descriptor_content"));
	}
	descriptor* dup(){return new adaptation_field_data_descriptor(*this);};
};


struct service_list_descriptor : public descriptor
{
	struct service
	{
		uint16_t service_id;
		uint8_t service_type;
		void io(iox& x)
		{
			x.uint(16,DVB_VAR(service_id));
			x.uint(8,DVB_VAR(service_type));
		}
	};
	std::vector<service> services;
	void io(iox& x)
	{
		if(!x.is_parsing())
			x.uint(8,DVB_VAR(tag));
		x.named_block_begin(8,DVB_INFO("length"));
		vector_io<service>(x,DVB_VAR(services));
		length=x.named_block_end(DVB_INFO("descriptor_content"));
	}
	descriptor* dup(){return new service_list_descriptor(*this);};
};

struct cable_delivery_system_descriptor : public descriptor
{
	uint32_t frequency;
	uint8_t FEC_outer;
	uint8_t modulation;
	uint32_t symbol_rate;
	uint8_t FEC_inner;
	void io(iox& x)
	{
		if(!x.is_parsing())
			x.uint(8,DVB_VAR(tag));
		x.named_block_begin(8,DVB_INFO("length"));
		x.uint(32,frequency,DVB_INFO_HINT("frequency",FORMAT_HINT_HEX));
		//x.uint(12,DVB_VAR(reserved_future_use));
		x.uint_req(12,(1<<12)-1,DVB_INFO("reserved_future_use"));
		x.uint(4,DVB_VAR(FEC_outer));
		x.uint(8,DVB_VAR(modulation));
		x.uint(28,DVB_VAR(symbol_rate));
		x.uint(4,DVB_VAR(FEC_inner));
		length=x.named_block_end(DVB_INFO("descriptor_content"));
	}
	descriptor* dup(){return new cable_delivery_system_descriptor(*this);};
};


struct extended_event_descriptor : public descriptor
{
	uint8_t descriptor_tag;
	uint8_t descriptor_length;
	uint8_t descriptor_number;
	uint8_t last_descriptor_number;
	std::string ISO_639_language_code;
	uint8_t length_of_items;
	struct item
	{
		std::string item_description;
		std::string item;
		void io(iox& x)
		{
			short_string_io(x,DVB_VAR(item_description));
			short_string_io(x,DVB_VAR(item));
		}
	};
	std::vector<item> items;
	std::string text;
	void io(iox& x)
	{
		x.uint(8,DVB_VAR(descriptor_tag));
		x.uint(8,DVB_VAR(descriptor_length));
		x.uint(4,DVB_VAR(descriptor_number));
		x.uint(4,DVB_VAR(last_descriptor_number));
		fixed_string_io(x,3,DVB_VAR(ISO_639_language_code));
		x.named_block_begin(8,DVB_INFO("length_of_items"));
		vector_io(x,DVB_VAR(items));
		length_of_items=x.named_block_end(DVB_INFO("items"));
		short_string_io(x,text,NULL);
	}
	descriptor* dup(){return new extended_event_descriptor(*this);};
};


struct unknown_descriptor : public descriptor
{
	std::string data;
	void io(iox& x)
	{
		if(!x.is_parsing())
			x.uint(8,DVB_VAR(tag));
		x.uint(8,DVB_VAR(length));
		if(length>0)
		fixed_string_io(x,length,DVB_VAR(data));
	}
	descriptor* dup(){return new unknown_descriptor(*this);};
};


struct descriptor_vector : public std::vector<struct descriptor*>
{
	~descriptor_vector();
	descriptor_vector();
	descriptor_vector(const descriptor_vector& x);
	descriptor_vector& operator=(descriptor_vector const &x);
	void io(iox& x);
	void purge();
};


}
#endif
