/* \file
 * \brief Input-Output general implementation
 */
#include "inc/io.h"
#include <assert.h>

namespace mopa
{


void ocCtx::enter_scope(const iox_info* info)
{
	prod+=std::string(scope_stack.size()*2,' ');
	prod+="{\n";
}
void ocCtx::leave_scope(const iox_info* info)
{
	prod+=std::string(scope_stack.size()*2,' ');
	prod+="}\n";
}
void ocCtx::write_uint(int bitsize, uint32_t value, const iox_info* info)
{
	if(bitsize<32 && value>(uint32_t)( (1<<bitsize)-1 )) throw Exception(this,info, "value %d exceeds %d bits",value,bitsize);
	prod+=std::string(scope_stack.size()*2,' ');
	prod+=info->name;
	prod+=":";
	uint32_t fmt=info->hint&FORMAT_HINT_MASK;
	if(fmt==FORMAT_HINT_HEX)
	{
		write_hex(bitsize, value);
	}
	else if(fmt==FORMAT_HINT_BIN)
	{
		write_bin(bitsize, value);
	}
	else
	{
		write_dec(bitsize, value);
	}
	prod+="\n";
}

void ocCtx::write_hex(int bitsize, uint32_t value)
{
	char buf[11];
	sprintf(buf,"0x%x",value);
	prod+=buf;
}
void ocCtx::write_bin(int bitsize, uint32_t value)
{
	char buf[33];
	int i;
	for(i=0;i<bitsize;i++)
	{
		if(value&(1<<(bitsize-1-i))) buf[i]='0'; else buf[i]='1';
	}
	buf[bitsize]=0;
	prod+=buf;
}
void ocCtx::write_dec(int bitsize, uint32_t value)
{
	char buf[12];
	sprintf(buf,"%d",value);
	prod+=buf;
}


void icCtx::enter_scope(const iox_info* info)
{
	expect("{");
}
void icCtx::leave_scope(const iox_info* info)
{
	expect("}");
}
bool icCtx::expect(const char* req)
{
	skiptotoken();
	while((*req!=0) && (*parse_pos==*req))
	{
		parse_pos++;
		req++;
	}
	return(*req==0);//all req was matched
}
bool icCtx::skiptotoken()
{
	while((*parse_pos=='\t') || (*parse_pos=='\n') || (*parse_pos==' '))
		parse_pos++;
	return(*parse_pos!=0); //there is something more to read
}

uint32_t icCtx::read_uint(int bitsize,  const iox_info* info)
{
	uint32_t value;
	if(!expect(info->name)) throw Exception(this,info,"'%s' expected",info->name);
	if(!expect(":")) throw Exception(this,info,"':' expected");
	skiptotoken();
	if(*parse_pos=='0' && *(parse_pos+1)=='x')
	{
		parse_pos+=2;
		value=read_hex(bitsize,info);
		goto done;
	}
	if(*parse_pos=='0' && *(parse_pos+1)=='b')
	{
		parse_pos+=2;
		value=read_bin(bitsize,info);
		goto done;
	}
	value=read_dec(bitsize,info);
	done:
	return value;
}


uint32_t icCtx::read_hex(int bitsize,const iox_info* info)
{
	uint64_t value=0;
	do
	{
		if(value>=0x100000000ULL) throw Exception(this,info,"value exceeds 32 bit");
		if(*parse_pos>='0' && *parse_pos<='9')
		{
			value=value*16+*parse_pos-'0';
			parse_pos++;
			continue;
		}
		if(*parse_pos>='A' && *parse_pos<='F')
		{
			value=value*16+*parse_pos-'A';
			parse_pos++;
			continue;
		}
		if(*parse_pos>='a' && *parse_pos<='f')
		{
			value=value*16+*parse_pos-'a';
			parse_pos++;
			continue;
		}
		break;
	}
	while(true);
	if(value >= (0x1ULL<<bitsize)) throw Exception(this,info,"value %x exceeds %d bit",value,bitsize);
	return value;
}
uint32_t icCtx::read_bin(int bitsize,const iox_info* info)
{
	uint64_t value=0;
	do
	{
		if(value>=0x100000000ULL) throw Exception(this,info,"value exceeds 32 bit");
		if(*parse_pos>='0' && *parse_pos<='1')
		{
			value=value*2+*parse_pos-'0';
			parse_pos++;
			continue;
		}
		break;
	}
	while(true);
	if(value >= (0x1ULL<<bitsize)) throw Exception(this,info,"value %d exceeds %d bit",value,bitsize);
	return value;
}
uint32_t icCtx::read_dec(int bitsize,const iox_info* info)
{
	uint64_t value=0;
	do
	{
		if(value>=0x100000000ULL) throw Exception(this,info,"value exceeds 32 bit");
		if(*parse_pos>='0' && *parse_pos<='9')
		{
			value=value*10+*parse_pos-'0';
			parse_pos++;
			continue;
		}
		break;
	}
	while(true);
	if(value >= (0x1ULL<<bitsize)) throw Exception(this,info,"value %d exceeds %d bits",(uint32_t)value,bitsize);
	return value;
}





iox::iox()
{
	ibCtx* x=new ibCtx();
	x->binary=true;
	x->parsing=true;
	x->data=NULL;
	x->bitlimit=0;
	x->bitpos=0;
	ctx=x;
};
iox::iox(iox const &from)
{
	ibCtx* x=new ibCtx();
	x->binary=true;
	x->parsing=true;
	x->data=NULL;
	x->bitlimit=0;
	x->bitpos=0;
	ctx=x;
	*this=from;
}
iox::iox(ioCtx* ctx)
{
	this->ctx=ctx;
}
iox& iox::operator=(iox const &rhs)
{
	if(this!=&rhs)
	{
		this->~iox();
		if(rhs.ctx->parsing)
			if(rhs.ctx->binary)
			{
				ctx=new ibCtx();
				*(ibCtx*)ctx=*(ibCtx*)rhs.ctx;
			}
			else
			{
				ctx=new icCtx();
				*(icCtx*)ctx=*(icCtx*)rhs.ctx;
			}
		else
			if(rhs.ctx->binary)
			{
				ctx=new obCtx();
				*(obCtx*)ctx=*(obCtx*)rhs.ctx;
			}
			else
			{
				ctx=new ocCtx();
				*(ocCtx*)ctx=*(ocCtx*)rhs.ctx;
			}
	}
	return *this;
}
iox::~iox()
{
	if(ctx->parsing)
		if(ctx->binary)
			delete (ibCtx*)ctx;
		else
			delete (icCtx*)ctx;
	else
		if(ctx->binary)
			delete (obCtx*)ctx;
		else
			delete (ocCtx*)ctx;
}

iox iox::parse_binary(const uint8_t* data, uint32_t size)
{
	ibCtx* x=new ibCtx();
	x->binary=true;
	x->parsing=true;
	x->data=data;
	x->bitlimit=size*8;
	x->bitpos=0;
	iox v(x);
	//v.ctx=x;
	return v;
}
iox iox::parse_text(const char* text)
{
	icCtx* x=new icCtx();
	x->binary=false;
	x->parsing=true;
	x->parsed_text=text;
	x->parse_pos=text;
	x->bitpos=0;
	x->bitlimit=8*1000000;
	iox v(x);
	return v;
}
iox iox::construct_binary(uint8_t* data, uint32_t size)
{
	obCtx* x=new obCtx();
	x->binary=true;
	x->parsing=false;
	x->data=data;
	x->bitlimit=size*8;
	x->bitpos=0;
	iox v(x);
	//v.ctx=x;
	return v;
}
iox iox::construct_text()
{
	ocCtx* x=new ocCtx();
	x->binary=false;
	x->parsing=false;
	x->bitlimit=8*1000000;
	x->bitpos=0;
	iox v(x);
	//v.ctx=x;
	return v;
}


ix iox::as_ix()
{
	return ix((iCtx*)ctx);
}
ox iox::as_ox()
{
	return ox((oCtx*)ctx);
}
ibx iox::as_ibx()
{
	return ibx((ibCtx*)ctx);
}
icx iox::as_icx()
{
	return icx((icCtx*)ctx);
}
obx iox::as_obx()
{
	return obx((obCtx*)ctx);
}
ocx iox::as_ocx()
{
	return ocx((ocCtx*)ctx);
}


void iox::uint(int bitsize,uint8_t& val,const iox_info* info)
{
	if(ctx->is_parsing())
		val=as_ix().uint8(bitsize,info);
	else
		as_ox().uint(bitsize,val,info);
}
void iox::uint(int bitsize,uint16_t& val,const iox_info* info)
{
	if(ctx->is_parsing())
		val=as_ix().uint16(bitsize,info);
	else
		as_ox().uint(bitsize,val,info);
}
void iox::uint(int bitsize,uint32_t& val,const iox_info* info)
{
	if(ctx->is_parsing())
		val=as_ix().uint32(bitsize,info);
	else
		as_ox().uint(bitsize,val,info);
}
void iox::uint_req(int bitsize,uint8_t val,const iox_info* info)
{
	if(ctx->is_parsing())
	{
		uint8_t v;
		v=as_ix().uint8(bitsize,info);
		if(v!=val)
		{
			throw Exception(ctx,info,"%d read %d required",v,val);
		}
	}
	else
	{
		as_ox().uint(bitsize,val,info);
	}
}
void iox::uint_req(int bitsize,uint16_t val,const iox_info* info)
{
	if(ctx->is_parsing())
	{
		uint16_t v;
		v=as_ix().uint16(bitsize,info);
		if(v!=val)
		{
			throw Exception(ctx,info,"%d read %d required",v,val);
		}
	}
	else
	{
		as_ox().uint(bitsize,val,info);
	}
}
void iox::uint_req(int bitsize,uint32_t val,const iox_info* info)
{
	if(ctx->is_parsing())
	{
		uint32_t v;
		v=as_ix().uint32(bitsize,info);
		if(v!=val)
		{
			throw Exception(ctx,info,"%d read %d required",v,val);
		}
	}
	else
	{
		as_ox().uint(bitsize,val,info);
	}
}
void iox::uint_req(int bitsize,int val,const iox_info* info)
{
	if(bitsize<=8)
		uint_req(bitsize,(uint8_t)val,info);
	else if(bitsize<=16)
		uint_req(bitsize,(uint16_t)val,info);
	else
		uint_req(bitsize,(uint32_t)val,info);
}

void iox::named_block_begin(int bitsize,const iox_info* info)
{
	if(ctx->is_parsing())
		as_ix().named_block_begin(bitsize,info);
	else
		as_ox().named_block_begin(bitsize,info);
}
uint32_t iox::named_block_end(const iox_info* info)
{
	if(ctx->is_parsing())
		return as_ix().named_block_end(info);
	else
		return as_ox().named_block_end(info);
}
uint32_t iox::block_size_left()
{
	if(ctx->is_parsing())
		return as_ix().block_size_left();
	else
		return as_ox().block_size_left();
}







ix::ix(iCtx* x)
{
	ctx=x;
}
ibx ix::as_ibx()
{
	return ibx((ibCtx*)ctx);
}
icx ix::as_icx()
{
	return icx((icCtx*)ctx);
}
uint8_t ix::uint8(int bitsize, const iox_info* info)
{
	if(ctx->is_binary())
		return as_ibx().uint8(bitsize,info);
	else
		return as_icx().uint(bitsize,info);
}
uint16_t ix::uint16(int bitsize, const iox_info* info)
{
	if(ctx->is_binary())
		return as_ibx().uint32(bitsize,info);
	else
		return as_icx().uint(bitsize,info);
}
uint32_t ix::uint32(int bitsize, const iox_info* info)
{
	if(ctx->is_binary())
		return as_ibx().uint32(bitsize,info);
	else
		return as_icx().uint(bitsize,info);
}
void ix::named_block_begin(int bitsize,const iox_info* info)
{
	if(ctx->is_binary())
	{
		uint32_t length=as_ibx().uint32(bitsize,info);
		as_ibx().block_begin(length,info);
	}
	else
	{
		uint32_t length=as_icx().uint(bitsize,info);
		as_icx().block_begin(length,info);
	}
}
uint32_t ix::named_block_end(const iox_info* info)
{
	if(ctx->is_binary())
		return as_ibx().block_end(info);
	else
		return as_icx().block_end(info);
}
uint32_t ix::block_size_left()
{
	if(ctx->is_binary())
		return as_ibx().block_size_left();
	else
		return as_icx().block_size_left();
}





ox::ox(oCtx* x)
{
	ctx=x;
}
obx ox::as_obx()
{
	return obx((obCtx*)ctx);
}
ocx ox::as_ocx()
{
	return ocx((ocCtx*)ctx);
}
void ox::uint(int bitsize, uint8_t val, const iox_info* info)
{
	if(ctx->is_binary())
		as_obx().uint(bitsize,val,info);
	else
		as_ocx().uint(bitsize,val,info);
}
void ox::uint(int bitsize, uint16_t val, const iox_info* info)
{
	if(ctx->is_binary())
		as_obx().uint(bitsize,val,info);
	else
		as_ocx().uint(bitsize,val,info);
}
void ox::uint(int bitsize, uint32_t val, const iox_info* info)
{
	if(ctx->is_binary())
		as_obx().uint(bitsize,val,info);
	else
		as_ocx().uint(bitsize,val,info);
}
void ox::named_block_begin(int bitsize,const iox_info* info)
{
	if(ctx->is_binary())
	{
		uint32_t pos=ctx->bitpos;
		ctx->bitpos+=bitsize;
		as_obx().block_begin((1<<bitsize)-1,info);
		as_obx().ctx->scope_stack.back().position_for_write=pos;
	}
	else
	{
		uint32_t pos=as_ocx().ctx->prod.size();
		ctx->bitpos+=bitsize;
		as_ocx().block_begin((1<<bitsize)-1,info);//size { }
		as_ocx().ctx->scope_stack.back().position_for_write=pos;
	}
}

uint32_t ox::named_block_end(const iox_info* info)
{
	uint32_t block_length;
	if(ctx->is_binary())
	{
		obx x=as_obx();
		obScope s=x.ctx->scope_stack.back();
		block_length=x.block_end(info);
		uint32_t temp_pos=ctx->bitpos;
		int bitsize=s.bitpos_at_enter-s.position_for_write;
		ctx->bitpos=s.position_for_write;
		x.uint(bitsize,block_length,info);
		ctx->bitpos=temp_pos;
	}
	else
	{
		ocx x=as_ocx();
		ocScope s=x.ctx->scope_stack.back();
		block_length=x.block_end(info);
		std::string tail=x.ctx->prod.substr(s.position_for_write);
		x.ctx->prod.resize(s.position_for_write);
		//x.uint(32,block_length,s.info);
		x.ctx->write_uint(32,block_length,s.info);
		x.ctx->prod+=tail;
	}
	return block_length;
}

uint32_t ox::block_size_left()
{
	if(ctx->is_binary())
		return as_obx().block_size_left();
	else
		return as_ocx().block_size_left();
}
















ibx::ibx(ibCtx* x)
{
	ctx=x;
}
uint8_t ibx::uint8(int bitsize,const iox_info* info)
{
	int8_t val=0;
	if(ctx->bitpos + bitsize > ctx->bitlimit) throw Exception(ctx,info,"left %d bits, needed %d",ctx->bitlimit-ctx->bitpos, bitsize);
	val=ctx->nocheck_uint8(bitsize);
	return val;
}
uint16_t ibx::uint16(int bitsize,const iox_info* info)
{
	int16_t val=0;
	if(ctx->bitpos + bitsize > ctx->bitlimit) throw Exception(ctx,info,"left %d bits, needed %d",ctx->bitlimit-ctx->bitpos, bitsize);
	val=ctx->nocheck_uint16(bitsize);
	return val;
}
uint32_t ibx::uint32(int bitsize,const iox_info* info)
{
	int32_t val=0;
	if(ctx->bitpos + bitsize > ctx->bitlimit) throw Exception(ctx,info,"left %d bits, needed %d",ctx->bitlimit-ctx->bitpos, bitsize);
	val=ctx->nocheck_uint32(bitsize);
	return val;
}
void ibx::block_begin(int block_length,const iox_info* info)
{
	if((ctx->bitpos&7) != 0 )
		throw Exception(ctx,info,"starting bit not byte-aligned");
	if(ctx->bitpos + block_length*8 > ctx->bitlimit)
		throw Exception(ctx,info,"block size %d exceeds available %d",block_length*8,ctx->bitlimit-ctx->bitpos);

	ibScope s;
	s.bitlimit_at_enter=ctx->bitlimit;
	s.bitpos_at_enter=ctx->bitpos;
	s.info=info;
	ctx->scope_stack.push_back(s);
	ctx->bitlimit=ctx->bitpos+block_length*8;
}
uint32_t ibx::block_end(const iox_info* info)
{
	if(ctx->scope_stack.size()==0) throw Exception(ctx,info,"unmatched named_block_end");
	ibScope &s=ctx->scope_stack.back();
	if(ctx->bitpos<ctx->bitlimit)
		throw Exception(ctx,info,"block ends with %d bits remaining",ctx->bitlimit-ctx->bitpos);
	uint32_t len=ctx->bitpos-s.bitpos_at_enter;
	if((len&7) != 0 )
		throw Exception(ctx,info,"block length not byte-aligned");
	ctx->bitlimit=s.bitlimit_at_enter;
	ctx->scope_stack.pop_back();
	return len/8;
}
uint32_t ibx::block_size_left()
{
	return ctx->bitlimit - ctx->bitpos;
}




obx::obx(obCtx* x)
{
	ctx=x;
}
void obx::uint(int bitsize,uint8_t val,const iox_info* info)
{
	if(ctx->bitpos + bitsize > ctx->bitlimit)
		throw Exception(ctx,info,"left %d bits, needed %d",ctx->bitlimit-ctx->bitpos, bitsize);
	ctx->nocheck_uint(bitsize,val);
}
void obx::uint(int bitsize,uint16_t val,const iox_info* info)
{
	if(ctx->bitpos + bitsize > ctx->bitlimit)
		throw Exception(ctx,info,"left %d bits, needed %d",ctx->bitlimit-ctx->bitpos, bitsize);
	ctx->nocheck_uint(bitsize,val);
}
void obx::uint(int bitsize,uint32_t val,const iox_info* info)
{
	if(ctx->bitpos + bitsize > ctx->bitlimit)
		throw Exception(ctx,info,"left %d bits, needed %d",ctx->bitlimit-ctx->bitpos, bitsize);

	if((ctx->bitpos&7) + bitsize<=32)
	{
		ctx->nocheck_uint(bitsize,val);
	}
	else
		throw Exception(ctx,info,"value spans over 5 bytes");
}
void obx::block_begin(uint32_t block_size_limit,const iox_info* info)
{
	block_size_limit=block_size_limit*8;
	if((ctx->bitpos&7) != 0 )
		throw Exception(ctx,info,"starting bit not byte-aligned");
	if(ctx->bitpos + block_size_limit > ctx->bitlimit)
		block_size_limit=ctx->bitlimit-ctx->bitpos;

	obScope s;
	s.bitlimit_at_enter=ctx->bitlimit;
	s.bitpos_at_enter=ctx->bitpos;
	s.info=info;
	ctx->scope_stack.push_back(s);
	ctx->bitlimit=ctx->bitpos+block_size_limit;
}

uint32_t obx::block_end(const iox_info* info)
{
	if(ctx->scope_stack.size()==0) throw Exception(ctx,info,"unmatched named_block_end");
	obScope &s=ctx->scope_stack.back();
	uint32_t len=ctx->bitpos-s.bitpos_at_enter;
	if((len&7) != 0 )
		throw Exception(ctx,info,"block length not byte-aligned");
	ctx->bitlimit=s.bitlimit_at_enter;
	ctx->scope_stack.pop_back();
	return len/8;
}
uint32_t obx::block_size_left()
{
	return ctx->bitlimit - ctx->bitpos;
}






icx::icx(icCtx* x)
{
	ctx=x;
}
uint32_t icx::uint(int bitsize,  const iox_info* info)
{
	uint32_t value=ctx->read_uint(bitsize,info);
	ctx->bitpos+=bitsize;
	return value;
}
void icx::block_begin(int block_length,const iox_info* info)
{
	if((ctx->bitpos&7) != 0 )
		throw Exception(ctx,info,"starting bit not byte-aligned");
	if(ctx->bitpos + block_length*8 > ctx->bitlimit)
		throw Exception(ctx,info,"block size %d exceeds available %d",block_length*8,ctx->bitlimit-ctx->bitpos);
	ctx->enter_scope(info);
	icScope s;
	s.bitlimit_at_enter=ctx->bitlimit;
	s.bitpos_at_enter=ctx->bitpos;
	s.info=info;
	ctx->scope_stack.push_back(s);
	ctx->bitlimit=ctx->bitpos+block_length*8;
}
uint32_t icx::block_end(const iox_info* info)
{
	if(ctx->scope_stack.size()==0) throw Exception(ctx,info,"unmatched named_block_end");
	ctx->leave_scope(info);
	icScope &s=ctx->scope_stack.back();
	uint32_t len=ctx->bitpos-s.bitpos_at_enter;
	if((len&7) != 0 )
		throw Exception(ctx,info,"block length not byte-aligned");
	ctx->bitlimit=s.bitlimit_at_enter;
	ctx->scope_stack.pop_back();
	return len/8;
}
uint32_t icx::block_size_left()
{
	if(!ctx->skiptotoken()) return 0;
	if(*ctx->parse_pos=='}') return 0;
	return ctx->bitlimit - ctx->bitpos;
}


ocx::ocx(ocCtx* x)
{
	ctx=x;
}
void ocx::uint(int bitsize, uint32_t val, const iox_info* info)
{
	if(ctx->bitpos+bitsize > ctx->bitlimit)
		throw Exception(ctx,info,"left %d bits, needed %d",ctx->bitlimit-ctx->bitpos, bitsize);
	ctx->write_uint(bitsize,val,info);
	ctx->bitpos+=bitsize;
}

void ocx::block_begin(int block_size_limit,const iox_info* info)
{
	block_size_limit*=8;
	if((ctx->bitpos&7) != 0 )
		throw Exception(ctx,info,"block begin is not byte-aligned");
	if(ctx->bitpos + block_size_limit > ctx->bitlimit)
		block_size_limit=ctx->bitlimit-ctx->bitpos;

	ocScope s;
	s.bitlimit_at_enter=ctx->bitlimit;
	s.bitpos_at_enter=ctx->bitpos;

	s.info=info;
	ctx->enter_scope(info);
	ctx->scope_stack.push_back(s);
	ctx->bitlimit=ctx->bitpos+block_size_limit;
}
uint32_t ocx::block_end(const iox_info* info)
{
	uint32_t block_length=ctx->bitpos-ctx->scope_stack.back().bitpos_at_enter;
	if((block_length&7) != 0 )
		throw Exception(ctx,info,"block length not byte-aligned");
	ctx->bitlimit=ctx->scope_stack.back().bitlimit_at_enter;
	ctx->scope_stack.pop_back();
	ctx->leave_scope(info);
	return block_length/8;
}
uint32_t ocx::block_size_left()
{
	return ctx->bitlimit - ctx->bitpos;
}

Exception::Exception(
			const ioCtx* x,
			const iox_info* _info,
			const char* fmt, ...)
	{
		static const iox_info empty_info={"-unknown-",0,"-unknown-",0};
		va_list argList;
		va_start(argList, fmt);
		char* p=NULL;
		int rr;
		rr=vasprintf(&p, fmt, argList);
		va_end(argList);
		if(rr>0){message=p;free(p);}

		const iox_info* info=_info;
		if(info==NULL) info=&empty_info;
		if(x->is_parsing())
		{
			if(x->is_binary())
			{
				ibCtx* y=(ibCtx*)x;
				p=NULL;
				rr=asprintf(&p," when parsing '%s' position %d.%d limit %d.%d declared in (%s:%d)",
						info->name,y->bitpos/8,y->bitpos%8,y->bitlimit/8,y->bitlimit%8,
						info->file==NULL?"":info->file,info->line);
				if(rr>0){message+=p;free(p);}
				for(int i=y->scope_stack.size()-1;i>=0;i--)
				{
					p=NULL;
					ibScope& s=y->scope_stack[i];
					info=s.info;
					if(info==NULL) info=&empty_info;
					rr=asprintf(&p,"\nin block '%s' position %d.%d limit %d.%d declared in (%s:%d)",
							info->name,s.bitpos_at_enter/8,s.bitpos_at_enter%8,s.bitlimit_at_enter/8,s.bitlimit_at_enter%8,
							info->file==NULL?"":info->file,info->line);
					if(rr>0){message+=p;free(p);}
				}
			}
			else
			{
				icCtx* y=(icCtx*)x;
				const char* line_begin=y->parsed_text;
				uint32_t line_nr=1;
				const char* r;
				for(r=y->parsed_text;r<y->parse_pos;r++)
				{
					if(*r=='\n')
					{
						line_begin=r+1;
						line_nr++;
					}
				}
				p=NULL;
				rr=asprintf(&p," at <input>:%d:%ld when parsing '%s' position %d.%d limit %d.%d declared in (%s:%d)",
						line_nr,r-line_begin,
						info->name,y->bitpos/8,y->bitpos%8,y->bitlimit/8,y->bitlimit%8,
						info->file==NULL?"":info->file,info->line);
				if(rr>0){message+=p;free(p);}
				for(int i=y->scope_stack.size()-1;i>=0;i--)
				{
					p=NULL;
					icScope& s=y->scope_stack[i];
					info=s.info;
					if(info==NULL) info=&empty_info;
					rr=asprintf(&p,"\nin block '%s' position %d.%d limit %d.%d declared in (%s:%d)",
							info->name,s.bitpos_at_enter/8,s.bitpos_at_enter%8,s.bitlimit_at_enter/8,s.bitlimit_at_enter%8,
							info->file==NULL?"":info->file,info->line);
					if(rr>0){message+=p;free(p);}
				}
			}
		}
		else
		{
			if(x->is_binary())
			{
				obCtx* y=(obCtx*)x;
				p=NULL;
				rr=asprintf(&p," when building '%s' position %d.%d limit %d.%d declared in (%s:%d)",
						info->name,y->bitpos/8,y->bitpos%8,y->bitlimit/8,y->bitlimit%8,
						info->file==NULL?"":info->file,info->line);
				if(rr>0){message+=p;free(p);}
				for(int i=y->scope_stack.size()-1;i>=0;i--)
				{
					p=NULL;
					obScope& s=y->scope_stack[i];
					info=s.info;
					if(info==NULL) info=&empty_info;
					rr=asprintf(&p,"\nin block '%s' position %d.%d limit %d.%d declared in (%s:%d)",
							info->name,s.bitpos_at_enter/8,s.bitpos_at_enter%8,s.bitlimit_at_enter/8,s.bitlimit_at_enter%8,
							info->file==NULL?"":info->file,info->line);
					if(rr>0){message+=p;free(p);}
				}
			}
			else
			{
				ocCtx* y=(ocCtx*)x;
				p=NULL;
				rr=asprintf(&p," when building '%s' position %d.%d limit %d.%d declared in (%s:%d)",
						info->name,y->bitpos/8,y->bitpos%8,y->bitlimit/8,y->bitlimit%8,
						info->file==NULL?"":info->file,info->line);
				if(rr>0){message+=p;free(p);}
				for(int i=y->scope_stack.size()-1;i>=0;i--)
				{
					p=NULL;
					ocScope& s=y->scope_stack[i];
					info=s.info;
					if(info==NULL) info=&empty_info;
					rr=asprintf(&p,"\nin block '%s' position %d.%d limit %d.%d declared in (%s:%d)",
							info->name,s.bitpos_at_enter/8,s.bitpos_at_enter%8,s.bitlimit_at_enter/8,s.bitlimit_at_enter%8,
							info->file==NULL?"":info->file,info->line);
					if(rr>0){message+=p;free(p);}
				}
			}
		}
	}




}
