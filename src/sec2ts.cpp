/**
 * \file
 * \dot
 * digraph example {
 * node [shape=rectangle, style=filled, fillcolor=darkseagreen];
 * edge [shape=record, fontname=Helvetica, fontsize=10];
 *
 * empty -> pusi_written    when section start in packet
 * empty -> no_pusi         when no section start in packet
 * no_pusi -> pusi_written
 * no_pusi -> empty
 * pusi_written -> empty
 *
 *
 * wait_packet [shape=oval, peripheries=2];
 * end [shape=oval, peripheries=2];
 * send_section [shape=rectangle,peripheries=2];
 * wait_section_start [shape=oval, label="wait_start"];
 * continue_section [shape=oval, label="continue"];
 * packet_arrives [shape=rectangle];
 * skip_adaptation_field [label="skip adaptation field"];
 * skip_adaptation_field_2 [label="skip adaptation field"];
 * skip_ptr [label="skip pointer field"];
 * set_mode_wait_start [label="mode:=wait_start"];
 * set_mode_continue [label="mode:=continue"];
 * skip_to_section_begin [label="skip to section begin"];
 *
 * wait_packet -> packet_arrives;
 * packet_arrives -> wait_section_start [label="mode=wait_start"];
 * packet_arrives -> continue_section [label="mode=continue"];
 * wait_section_start -> end [label="pusi==0"];
 * wait_section_start -> skip_adaptation_field [label="pusi==1"];
 * skip_adaptation_field -> skip_to_section_begin;
 * skip_to_section_begin -> collect_section;
 * collect_section -> send_section [label="entire section present"];
 * collect_section -> set_mode_continue [label="more section needed"];
 * send_section -> collect_section [label="section data left"];
 * send_section -> set_mode_wait_start [label="no section data"];
 *
 * continue_section -> skip_adaptation_field_2 [label="cc OK"];
 * continue_section -> set_mode_wait_start [label="cc fail"];
 * set_mode_wait_start -> end;
 * set_mode_continue -> end;
 * skip_adaptation_field_2 -> skip_ptr;
 * skip_ptr -> collect_section;
 * }
 * \enddot
 * States of ts_packet.
 */
#include "inc/sec2ts.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TS_PACKET_LEN 188
#define AFC_RESERVED 0
#define AFC_PAYLOAD 1
#define AFC_ADAPTATION 2
#define AFC_ADAPTATION_AND_PAYLOAD 3

class sec2ts_impl : public sec2ts
{
public:
	sec2ts_impl();
	virtual ~sec2ts_impl();
	virtual void setPID(uint16_t pid);
	virtual void section(const uint8_t* section, uint32_t size);
	virtual void flush();
	virtual void on_adaptation_field(void* ctx, adaptation_field callback);
	virtual void on_ts_packet_produced(void* ctx, ts_packet_produced callback);
	virtual void set_dbg_level(uint8_t level);
	template<int DBG_LEVEL> void sectionX(const uint8_t* section, uint32_t size);

	void inline fix_header(bool payload_unit_start,uint32_t adaptation_value);
	uint16_t pid;
	ts_packet_produced on_packet_produced_cb;
	void* on_packet_produced_ctx;
	adaptation_field on_adaptation_field_cb;
	void* on_adaptation_field_ctx;
	void (sec2ts_impl::*section_impl)(const uint8_t* section, uint32_t size);
	uint8_t dbg_level;
	uint8_t cc;
	uint8_t ts_packet[TS_PACKET_LEN];

	//uint8_t adaptation_len;
	bool pusi;
	uint8_t payload_start;
	uint8_t payload_end;
};


sec2ts::sec2ts()
{
}
sec2ts::~sec2ts()
{
}

sec2ts* sec2ts::create()
{
	sec2ts* x=new sec2ts_impl();
	return x;
}




template<int DBG_LEVEL> void sec2ts_impl::sectionX(const uint8_t* section, uint32_t size)
{
	bool wrote_section_start=false;
	uint32_t adalen=0;
	uint32_t rem;

	if(payload_start!=0)
	{
		//we have packet to finish
		if(!pusi)
		{
			//have to make place for section start
			if(payload_end>=TS_PACKET_LEN-1)
			{
				//cannot add pointer field when only 1 byte remains
				ts_packet[TS_PACKET_LEN-1]=0xff;
				fix_header(false,payload_start>4?AFC_ADAPTATION_AND_PAYLOAD:AFC_PAYLOAD);
				on_packet_produced_cb(on_packet_produced_ctx,ts_packet);
				payload_start=0;
				pusi=false;
				cc=(cc+1)&0xf;
				goto more;
			}
			//making space for pointer field
			memmove(ts_packet+payload_start+1, ts_packet+payload_start, payload_end-payload_start);
			//writing pointer field
			ts_packet[payload_start]=payload_end-payload_start;
			payload_end++;
			wrote_section_start=true;
			pusi=true;
		}
		uint32_t rem=TS_PACKET_LEN-payload_end;
		if(rem>size)
		{
			memcpy(ts_packet+payload_end,section,size);
			payload_end+=size;
			goto end;
		}
		else
		{
			memcpy(ts_packet+payload_end,section,rem);
			payload_end+=rem;
			section+=rem;
			size-=rem;
			fix_header(true,payload_start>4?AFC_ADAPTATION_AND_PAYLOAD:AFC_PAYLOAD);
			on_packet_produced_cb(on_packet_produced_ctx,ts_packet);
			payload_start=0;
			pusi=false;
			cc=(cc+1)&0xf;
			if(size==0) goto end;
			wrote_section_start=true;
		}
	}

	more:
	adalen=0;
	if(on_adaptation_field_cb!=NULL)
	{
		adalen=on_adaptation_field_cb(on_adaptation_field_ctx,&ts_packet[4],TS_PACKET_LEN-4);
	}
	payload_start=4+adalen;
	payload_end=payload_start;
	rem=TS_PACKET_LEN-payload_end;
	if(rem<=1)
	{
		fix_header(true,AFC_ADAPTATION);
		on_packet_produced_cb(on_packet_produced_ctx,ts_packet);
		payload_start=0;
		pusi=false;
		//no cc increment when no data
		//cc=(cc+1)&0xf;
		goto more;
	}
	//check if writing pusi is necessary
	if(!wrote_section_start)
	{
		ts_packet[payload_end]=0;
		payload_end++;
		rem=TS_PACKET_LEN-payload_end;
		pusi=true;
		wrote_section_start=true;
	}
	if(rem>size)
	{
		memcpy(ts_packet+payload_end,section,size);
		payload_end+=size;
		goto end;
	}
	else
	{
		memcpy(ts_packet+payload_end,section,rem);
		section+=rem;
		size-=rem;
		fix_header(pusi,payload_start>4?AFC_ADAPTATION_AND_PAYLOAD:AFC_PAYLOAD);
		on_packet_produced_cb(on_packet_produced_ctx,ts_packet);
		payload_start=0;
		pusi=false;
		cc=(cc+1)&0xf;
		if(size==0) goto end;
		goto more;
	}
	end:;
}

sec2ts_impl::sec2ts_impl():
		pid(0),
		on_packet_produced_cb(NULL),
		on_packet_produced_ctx(NULL),
		on_adaptation_field_cb(NULL),
		on_adaptation_field_ctx(NULL),
		dbg_level(0),
		cc(0),
		pusi(false),
		payload_start(0),
		payload_end(0)
{
	memset(ts_packet,0,TS_PACKET_LEN);
	section_impl=&sec2ts_impl::sectionX<0>;
};

sec2ts_impl::~sec2ts_impl(){};
void inline sec2ts_impl::fix_header(bool payload_unit_start,uint32_t adaptation_value)
{
	ts_packet[0]=0x47;
	ts_packet[1]=(payload_unit_start?1:0)<<6|pid>>8;
	ts_packet[2]=pid;
	ts_packet[3]=(adaptation_value)<<4|cc;
}
void sec2ts_impl::setPID(uint16_t pid)
{
	this->pid=pid&((1<<13)-1);
}
void sec2ts_impl::section(const uint8_t* section, uint32_t size)
{
	(this->*section_impl)(section,size);
}

void sec2ts_impl::flush()
{
	if(payload_start!=0)
	{
		fix_header(pusi,payload_start>4?AFC_ADAPTATION_AND_PAYLOAD:AFC_PAYLOAD);
		memset(ts_packet+payload_end,0xff,TS_PACKET_LEN-payload_end);
		on_packet_produced_cb(on_packet_produced_ctx,ts_packet);
		payload_start=0;
		pusi=false;
		cc=(cc+1)&0xf;
	}
}

void sec2ts_impl::set_dbg_level(uint8_t level)
{
	if(level>=0 && level<=5)
	{
		dbg_level=level;
		if(dbg_level==0) section_impl=&sec2ts_impl::sectionX<0>;
		if(dbg_level==1) section_impl=&sec2ts_impl::sectionX<1>;
		if(dbg_level==2) section_impl=&sec2ts_impl::sectionX<2>;
		if(dbg_level==3) section_impl=&sec2ts_impl::sectionX<3>;
		if(dbg_level==4) section_impl=&sec2ts_impl::sectionX<4>;
		if(dbg_level==5) section_impl=&sec2ts_impl::sectionX<5>;
	}
}

void sec2ts_impl::on_adaptation_field(void* ctx, adaptation_field callback)
{
	on_adaptation_field_ctx=ctx;
	on_adaptation_field_cb=callback;
}
void sec2ts_impl::on_ts_packet_produced(void* ctx, ts_packet_produced callback)
{
	on_packet_produced_ctx=ctx;
	on_packet_produced_cb=callback;
}





