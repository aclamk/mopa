#ifndef __MERGER_H__
#define __MERGER_H__

#include <stdint.h>
#include <cstddef>

class psi_extractor
{
public:
	typedef void (*section_ready)(void* ctx, const uint8_t* section, size_t size);
	static psi_extractor* create(size_t max_table_size,int max_dbg=0);
	virtual ~psi_extractor(){};
	virtual void ts_packet(const uint8_t* bytes)=0;
	virtual void on_section_ready(void* ctx, section_ready callback)=0;
};

#endif
