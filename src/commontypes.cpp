/** \file */

#include "inc/io.h"
#include <vector>
#include <string>
#include <string.h>
namespace mopa
{



std::string parse_string(icCtx* ctx,const iox_info* info)
{
	if((ctx->bitpos&7)!=0)
		throw Exception(ctx,info,"String '%s' not byte-aligned",info?info->name:"");
	ctx->expect(info?info->name:"");
	ctx->expect(":");
	ctx->expect("'");
	std::string s;
	do
	{
		uint32_t c;
		c=*(ctx->parse_pos++);
		if(c=='\'')break;
		if(c=='\\')
		{
			c=*(ctx->parse_pos++);
			if(c=='\'' || c=='\\' || c=='\r' || c=='\n')
			{
				s+=c;
				continue;
			}
			uint32_t val=0;
			if(c>='0' && c<='7')
				val=val*8+(c-'0');
			else
				throw Exception(ctx,info,"Illegal char `\\%3.3o`",c);
			c=*(ctx->parse_pos++);
			if(c>='0' && c<='7')
				val=val*8+(c-'0');
			else
				throw Exception(ctx,info,"Illegal char `\\%3.3o`",c);
			c=*(ctx->parse_pos++);
			if(c>='0' && c<='7')
				val=val*8+(c-'0');
			else
				throw Exception(ctx,info,"Illegal char `\\%3.3o`",c);
			if(val>255)
				throw Exception(ctx,info,"Value too big");
			s+=(uint8_t)val;
			continue;
		}
		if(c>=32 && c<127)
		{
			s+=c;
			continue;
		}
		throw Exception(ctx,info,"Illegal char `\\%3.3o`",c);
	}
	while(true);

	return s;
}

void produce_string(ocCtx* ctx,const std::string& str,const iox_info* info)
{
	if((ctx->bitpos&7)!=0)
		throw Exception(ctx,info,"String '%s' not byte-aligned",info?info->name:"");
	uint32_t len=str.size();
	std::string s;
	for(int i=0;i<len;i++)
	{
		uint8_t c=str[i];
		if(c=='\'' || c=='\\' || c=='\r' || c=='\n')
		{
			s+="\\";s+=c;
			continue;
		}
		if(c>=32 && c<127)
		{
			s+=c;
			continue;
		}
		//s+='.';continue;
		s+='\\';
		s+=('0'+((c>>6)&0x7));
		s+=('0'+((c>>3)&0x7));
		s+=('0'+((c>>0)&0x7));
	}
	ctx->prod+=std::string(ctx->scope_stack.size()*2,' ');
	ctx->prod+=info->name;
	ctx->prod+=":'";
	ctx->prod+=s+"'\n";
	//ctx->bitpos+=(len+1)*8;
}

void short_string_io(iox& x,std::string& str,const iox_info* info=NULL)
{
	if(x.is_parsing())
		if(x.is_binary())
		{
			ibx y=x.as_ibx();
			if((y.ctx->bitpos&7)!=0)
				throw Exception(y.ctx,info,"String '%s' not byte-aligned",info?info->name:"");
			uint8_t len;
			len=y.uint8(8,info);
			if(y.ctx->bitpos+len*8>y.ctx->bitlimit)
				throw Exception(y.ctx,info,"String '%s' is %d length, only %d bits available",
						info?info->name:"",len, y.ctx->bitlimit-y.ctx->bitpos);
			str=std::string((char*)&y.ctx->data[y.ctx->bitpos/8],len);
			y.ctx->bitpos+=len*8;
		}
		else
		{
			icx y=x.as_icx();
			str=parse_string(y.ctx,info);
			uint8_t len=1+str.size();
			if(y.ctx->bitpos+len*8>y.ctx->bitlimit)
				throw Exception(y.ctx,info,"String '%s' requires %d bytes length, only %d bytes available",
						info?info->name:"",len, (y.ctx->bitlimit-y.ctx->bitpos)/8);
			y.ctx->bitpos+=len*8;
		}
	else
		if(x.is_binary())
		{
			obx y=x.as_obx();
			if((y.ctx->bitpos&7)!=0)
				throw Exception(y.ctx,info,"String '%s' not byte-aligned",info?info->name:"");
			uint32_t len=str.size();
			if(len>255)
				throw Exception(y.ctx,info,"String '%s' length %d exceeds max 255 chars",info?info->name:"");
			if(y.ctx->bitpos+(len+1)*8>y.ctx->bitlimit)
							throw Exception(y.ctx,info,"String '%s' requires %d bytes length, only %d bytes available",
									info?info->name:"",len+1, (y.ctx->bitlimit-y.ctx->bitpos)/8);
			y.uint(8,(uint8_t)len,info);
			memcpy(y.ctx->data+y.ctx->bitpos/8,str.c_str(),len);
			y.ctx->bitpos+=len*8;
		}
		else
		{
			ocx y=x.as_ocx();
			uint32_t len=str.size();
			if(len>255)
				throw Exception(y.ctx,info,"String '%s' length %d exceeds max 255 chars",info?info->name:"");
			if(y.ctx->bitpos+(len+1)*8>y.ctx->bitlimit)
				throw Exception(y.ctx,info,"String '%s' requires %d bytes length, only %d bytes available",
						info?info->name:"",len+1, (y.ctx->bitlimit-y.ctx->bitpos)/8);
			produce_string(y.ctx,str,info);
			y.ctx->bitpos+=(len+1)*8;
		}
}
//template<uint32_t num_chars>
//void fixed_string_io(dvb::iox& x,std::string& str,const iox_info* info=NULL)
void fixed_string_io(iox& x,uint32_t num_chars,std::string& str,const iox_info* info=NULL)

{
	if(x.is_parsing())
		if(x.is_binary())
		{
			ibx y=x.as_ibx();
			if((y.ctx->bitpos&7)!=0)
				throw Exception(y.ctx,info,"String '%s' not byte-aligned",info?info->name:"");
			uint8_t len=num_chars;
			if(y.ctx->bitpos+len*8>y.ctx->bitlimit)
				throw Exception(y.ctx,info,"String '%s' is %d length, only %d bytes available",
						info?info->name:"",len, (y.ctx->bitlimit-y.ctx->bitpos)/8);
			str=std::string((char*)&y.ctx->data[y.ctx->bitpos/8],len);
			y.ctx->bitpos+=len*8;
		}
		else
		{
			icx y=x.as_icx();
			str=parse_string(y.ctx,info);
			uint8_t len=str.size();
			if(y.ctx->bitpos+len*8>y.ctx->bitlimit)
				throw Exception(y.ctx,info,"String '%s' requires %d bytes length, only %d bytes available",
						info?info->name:"",len, (y.ctx->bitlimit-y.ctx->bitpos)/8);
			y.ctx->bitpos+=len*8;

		}
	else
		if(x.is_binary())
		{
			obx y=x.as_obx();
			if((y.ctx->bitpos&7)!=0)
				throw Exception(y.ctx,info,"String '%s' not byte-aligned",info?info->name:"");
			uint32_t len=str.size();
			if(str.size()!=num_chars)
							throw Exception(y.ctx,info,"String '%s' must be %d long",info?info->name:"",num_chars);
			if(y.ctx->bitpos+(len)*8>y.ctx->bitlimit)
				throw Exception(y.ctx,info,"String '%s' requires %d bytes length, only %d bytes available",
						info?info->name:"",len, (y.ctx->bitlimit-y.ctx->bitpos)/8);
			memcpy(y.ctx->data+y.ctx->bitpos/8,str.c_str(),len);
			y.ctx->bitpos+=len*8;
		}
		else
		{
			ocx y=x.as_ocx();
			uint32_t len=str.size();
			if(len!=num_chars)
				throw Exception(y.ctx,info,"String '%s' must be %d long",info?info->name:"",num_chars);
			if(y.ctx->bitpos+len*8>y.ctx->bitlimit)
				throw Exception(y.ctx,info,"String '%s' requires %d bytes length, only %d bytes available",
						info?info->name:"",len, (y.ctx->bitlimit-y.ctx->bitpos)/8);
			produce_string(y.ctx,str,info);
			y.ctx->bitpos+=str.size()*8;
		}
}

uint32_t dvb_crc32(const uint8_t *data, int len);

void crc_io(iox& x,uint32_t started_at,uint32_t& crc,const iox_info* info)
{
	if((started_at&7) != 0)
		throw Exception(x.ctx,info,"CRC block was not started at byte aligned");
	if((x.ctx->bitpos&7) != 0)
		throw Exception(x.ctx,info,"CRC not byte aligned");
	if(x.is_binary())
	{
		if(x.is_parsing())
		{
			ibx y=x.as_ibx();
			uint32_t bytes=(y.ctx->bitpos-started_at)/8;
			uint32_t crc_calculated;
			crc_calculated=dvb_crc32(y.ctx->data+started_at/8,bytes);
			x.uint(32,crc,info);
			if(crc_calculated!=crc)
				throw Exception(x.ctx,info,"CRC mismatch. read=%8.8x,calculated=%8.8x",crc,crc_calculated);
		}
		else
		{
			obx y=x.as_obx();
			uint32_t bytes=(y.ctx->bitpos-started_at)/8;
			crc=dvb_crc32(y.ctx->data+started_at/8,bytes);
			x.uint(32,crc,info);
		}
	}
	else
		x.uint(32,crc,info);
}

void crc_late_fix(iox& x,uint32_t started_at,uint32_t crc_pos,uint32_t& crc,const iox_info* info)
{
	if((started_at&7) != 0)
		throw Exception(x.ctx,info,"CRC block was not started at byte aligned");
	if((x.ctx->bitpos&7) != 0)
		throw Exception(x.ctx,info,"CRC not byte aligned");
	if(x.is_binary())
	{
		if(!x.is_parsing())
		{
			obx y=x.as_obx();
			uint32_t bytes=(crc_pos-started_at)/8;
			crc=dvb_crc32(y.ctx->data+started_at/8,bytes);
			uint32_t saved_pos=x.ctx->bitpos;
			x.ctx->bitpos=crc_pos;
			x.as_obx().uint(32,crc,info);
			x.ctx->bitpos=saved_pos;
		}
	}
}

}
