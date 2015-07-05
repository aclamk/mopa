/**

 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//table merger
#include "inc/merger.h"


/**
 * \file
 * \dot
 * digraph example {
 * node [shape=rectangle, style=filled, fillcolor=darkseagreen];
 * edge [shape=record, fontname=Helvetica, fontsize=10];
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
 * Diagram for state machine for PSI section merging from TS packets.
 */

void dbg(const char* fmt, ...)
{
	va_list argList;
	va_start(argList, fmt);
	char* p=NULL;
	int rr;
	rr=vasprintf(&p, fmt, argList);
	va_end(argList);
	if(rr>0)
	{
		//write(1,p,strlen(p));
		printf("%s",p);
		free(p);
	}
}

template <int max_dbg>
class psi_extractor_impl: public psi_extractor
{
public:
	psi_extractor_impl(size_t max_section_size);
	~psi_extractor_impl();
	void ts_packet(const uint8_t* p);
	void on_section_ready(void* ctx, section_ready callback);
private:
	size_t		  max_section_size;
	section_ready callback;
	void*		  callback_ctx;
	size_t 		  dvb_section_length;
	enum {wait_start, wait_more} state;
	uint8_t* 	  data;
	size_t		  data_len;
	uint8_t		  cc;
	int8_t 		  curr_dbg;
};
#define DBG(level) ((level<=max_dbg) && (level<=curr_dbg))
#define TS_PACKET_LEN 188
psi_extractor* psi_extractor::create(size_t max_section_size, int max_dbg)
{
	psi_extractor* extr=NULL;
	if(max_dbg>5) max_dbg=5;
	switch(max_dbg)
	{
		default: extr=new psi_extractor_impl<0>(max_section_size);break;
		case 1: extr=new psi_extractor_impl<1>(max_section_size);break;
		case 2: extr=new psi_extractor_impl<2>(max_section_size);break;
		case 3: extr=new psi_extractor_impl<3>(max_section_size);break;
		case 4: extr=new psi_extractor_impl<4>(max_section_size);break;
		case 5: extr=new psi_extractor_impl<5>(max_section_size);break;
	}
	return extr;
}
template <int max_dbg>
psi_extractor_impl<max_dbg>::psi_extractor_impl(size_t max_section_size)
{
	if(max_section_size>(1<<12)) max_section_size=1<<12;
	this->max_section_size=max_section_size;
	callback=NULL;
	callback_ctx=NULL;
	dvb_section_length=0;
	state=wait_start;
	data=new uint8_t[max_section_size+184]; //some additional size may be required for processing
	data_len=0;
	cc=0;
	curr_dbg=5;
}

template <int max_dbg>
void psi_extractor_impl<max_dbg>::on_section_ready(void* ctx, section_ready callback)
{
	this->callback=callback;
	this->callback_ctx=ctx;
}

template <int max_dbg>
psi_extractor_impl<max_dbg>::~psi_extractor_impl()
{
	delete[] data;
}

template <int max_dbg>
void psi_extractor_impl<max_dbg>::ts_packet(const uint8_t* p)
{
	int reminder=TS_PACKET_LEN;
	if(DBG(5)) dbg("ts_packet %2.2x:%2.2x:%2.2x:%2.2x\n",p[0],p[1],p[2],p[3]);
	int pusi=(p[1]>>6) & 1; //payload_unit_start_indicator
	uint16_t pid=(p[1]<<8 | p[2]) & ((1<<13) - 1);
	uint8_t afc=(p[3]>>4) & 3; //adaptation_field_control
	uint8_t cc=p[3] & 0xf;
	if(DBG(3)) dbg("pusi=%d pid=%d afc=%d cc=%d\n",pusi,pid,afc,cc);
	uint8_t* sec_p;
	int adapt_len;
	reminder-=4;
	p+=4;
	if(state==wait_start)
	{
		if(pusi==0)
		{
			if(DBG(4)) dbg("skipping until pusi==1\n");
			goto end;
		}
		data_len=0;
		this->cc=cc;
	}
	else
	{
		if(((this->cc + 1)&0xf) != cc)
		{
			if(DBG(4)) dbg("cc %d read %d expected => reset\n", (this->cc + 1)&0xf , cc);
			state=wait_start;goto end;
		}
		this->cc=(this->cc + 1)&0xf;
	}
	//skipping over adaptation
	adapt_len=0;
	if(afc==2 || afc==3)
	{
		//skip over adaptation field
		//p+=4;reminder=-4;
		adapt_len=1+p[0];
		if(adapt_len>reminder)
		{
			//nothing left of the packet
			if(DBG(2)) dbg("adapt_len=%d => reset\n",adapt_len);
			state=wait_start;goto end;
		}
		p+=adapt_len;
		reminder-=adapt_len;
		if(DBG(5)) dbg("skipped afc. p=%p rem=%d\n",p,reminder);
	}
	//now extract ptr field
	if(pusi==1)
	{
		int ptr;
		if(state==wait_start)
		{
			ptr=1+p[0];
			if(ptr>reminder)
			{
				if(DBG(2)) dbg("adapt_len=%d ptr=%d; pointer_field points outside packet\n",adapt_len,ptr);
				state=wait_start;goto end;
			}
		}
		else
		{
			ptr=1;
			if(ptr>reminder)
			{
				if(DBG(2)) dbg("ptr field outside packet => reset\n");
				state=wait_start;goto end;
			}
		}
		p+=ptr;
		reminder-=ptr;
		if(DBG(5)) dbg("skipped ptr field. p=%p rem=%d\n",p,reminder);
	}

	memcpy(data+data_len,p,reminder);
	data_len+=reminder;
	if(DBG(5)) dbg("appended %d bytes. length=%d\n",reminder,data_len);
	sec_p=data;
	more_sections:
	if(data_len>=1 && *sec_p==0xff)
	{
		//illegal table ID, means no more sections
		state=wait_start;goto end;
	}
	if(data_len<3)
	{
		//can't even retrieve length
		//copy rest data to beginning of buffer
		memcpy(data,sec_p,data_len);
		if(DBG(5)) dbg("%d bytes left for next section\n",data_len);
		state=wait_more;goto end;
	}
	//can calculate section_length
	dvb_section_length=(sec_p[1]<<8 | sec_p[2]) & ((1<<12) - 1);
	dvb_section_length+=3;
	if(dvb_section_length>max_section_size)
	{
		if(DBG(2)) dbg("section length %d > limit %d => reset\n",dvb_section_length>max_section_size);
		state=wait_start;goto end;
	}
	if(data_len>=dvb_section_length)
	{
		if(callback==NULL)
		{
			if(DBG(1)) dbg("section callback not set\n");
			state=wait_start;goto end;
		}
		if(DBG(3)) dbg("Section completed len=%d\n",dvb_section_length);
		callback(callback_ctx, sec_p, dvb_section_length);
		sec_p+=dvb_section_length;
		data_len-=dvb_section_length;
		if(DBG(5)) dbg("data_len=%d\n",data_len);
		if(data_len>0)
			goto more_sections;
		state=wait_start;goto end;
	}
	memcpy(data,sec_p,data_len);
	if(DBG(5)) dbg("%d bytes left for next section\n",data_len);
	state=wait_more;goto end;
	end:;
}

