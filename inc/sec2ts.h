#ifndef __SEC2TS_H__
#define __SEC2TS_H__

#include <stdint.h>
#include <cstddef>

class sec2ts
{
public:
	typedef void (*ts_packet_produced)(void* ctx, const uint8_t* packet);
	typedef uint32_t (*adaptation_field)(void* ctx, uint8_t* packet, uint32_t size);

	static sec2ts* create();
	virtual ~sec2ts();
	virtual void setPID(uint16_t pid)=0;
	virtual void section(const uint8_t* section, uint32_t size)=0;
	virtual void flush()=0;
	virtual void on_adaptation_field(void* ctx, adaptation_field callback)=0;
	virtual void on_ts_packet_produced(void* ctx, ts_packet_produced callback)=0;
	virtual void set_dbg_level(uint8_t level)=0;
protected:
	sec2ts();
};

#endif
