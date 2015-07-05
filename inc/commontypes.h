#ifndef _DVB_COMMONTYPES_H
#define _DVB_COMMONTYPES_H
#include "inc/io.h"
#include <vector>
#include <string>
#include <string.h>
namespace mopa
{

template<typename Item>
void vector_io(iox& x,std::vector<Item>& list,const iox_info* info=NULL)
{
	if(x.is_parsing())
	{
		list.clear();
		while(x.block_size_left()>0)
		{
			Item i;
			list.push_back(i);
			list.back().io(x);
		}
	}
	else
	{
		unsigned int i;
		for(i=0;i<list.size();i++)
		{
			list[i].io(x);
		}
	}
}

void short_string_io(iox& x,std::string& str,const iox_info* info=NULL);
void fixed_string_io(iox& x,uint32_t num_chars,std::string& str,const iox_info* info=NULL);

void crc_io(iox& x,uint32_t started_at,uint32_t& crc,const iox_info* info=NULL);
void crc_late_fix(iox& x,uint32_t started_at,uint32_t crc_pos,uint32_t& crc,const iox_info* info);
}
#endif
