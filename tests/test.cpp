#include "inc/io.h"
#include "inc/commontypes.h"
#include "inc/descriptors.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <string>
#include <string.h>

#include "inc/merger.h"
#include "dvb/NIT.h"
namespace mopa
{
	uint32_t dvb_crc32(const uint8_t *data, int len);
}
using namespace std;
using namespace mopa;
class Testbed
{
   public:
      Testbed(int (*test)(void))
         {
            m_test=test;
         }
      Testbed(int (*test)(void),string name,string description)
         {
            Test& t=g_tests[test];
            t.m_name=name;
            t.m_description=description;
            t.result=1;
         }
      Testbed& operator, (int (*dependency)(void))
         {
            std::map<int (*)(void), Test>::iterator it;
            it=g_tests.find(m_test);
            if(it!=g_tests.end())
            {
               (*it).second.m_deps.push_back(dependency);
            }
            return *this;
         }
      static int run(int (*test)(void), bool forced)
         {
            std::map<int (*)(void), Test>::iterator it;
            it=g_tests.find(test);
            if(it!=g_tests.end())
            {
               //test found
               if((*it).second.result<1)
               {
                  //printf("test already run\n");
                  if(!forced) return 0;
                  //return 0;
               }
               if((*it).second.result==2)
               {
                  printf("circular dependency, dropping\n");
               }
               (*it).second.result=2;
               int i;
               for(i=0;i<(*it).second.m_deps.size();i++)
               {
                  run((*it).second.m_deps[i],false);
               }
               (*it).second.result=0;
               printf("running test %s (%s)\n",(*it).second.m_name.c_str(),(*it).second.m_description.c_str());
               int result;
               result=(*it).first();
               printf("result=%d\n",result);
            }
            else
            {
               printf("no such test\n");
            }
         }
   private:
      int (*m_test)(void); //used to define dependencies for operator,

      struct Test
      {
            //int (*m_test)(void);
            std::vector<int (*)(void)> m_deps;
            string m_name;
            string m_description;
            int result; //-1 test failed, 0 test passed, 1 test not run, 2 test about to be run (for dependencied)
      };
      static std::map<int (*)(void), Test> g_tests;
};
std::map<int (*)(void), Testbed::Test> Testbed::g_tests;


#define MAKEDEP(test_name,...) Testbed makedep_##test_name=(*new Testbed(test_name), __VA_ARGS__)
#define DEFTEST(test_name,test_description) int test_name(); Testbed deftest_##test_name=(*new Testbed(test_name,#test_name,test_description))
#define RUNTEST(test_name) Testbed::run(test_name,true)





DEFTEST(test_parse_byte,"parse one byte message");
int test_parse_byte()
{
	const unsigned char byte[]={66};
	iox x=iox::parse_binary(byte,1);
	uint8_t vv;
	x.uint(8,vv);
	if(vv!=66) return -1;
	return 0;
}

DEFTEST(test_parse_bits,"parse sequence of bits");
MAKEDEP(test_parse_bits,test_parse_byte);
int test_parse_bits()
{
	const unsigned char byte[]={0xed};
	iox x=iox::parse_binary(byte,1);
	uint8_t vv;
	int i;
	uint8_t v[8];
	for(i=0;i<8;i++)
		x.uint(1,v[i]);
	if(v[0]!=1) return -1;
	if(v[1]!=1) return -2;
	if(v[2]!=1) return -3;
	if(v[3]!=0) return -4;
	if(v[4]!=1) return -5;
	if(v[5]!=1) return -6;
	if(v[6]!=0) return -7;
	if(v[7]!=1) return -8;
	return 0;
}

DEFTEST(test_parse_uint8_t,"parse uint8_t of various sizes");
MAKEDEP(test_parse_uint8_t,test_parse_bits,test_parse_byte);
int test_parse_uint8_t()
{
	//10100100 01000010 00001000 00010000 0001----
	const unsigned char bytes[]={0xa4,0x42,0x08,0x10,0x10};
	iox x=iox::parse_binary(bytes,5);
	uint8_t vv;
	int i;
	for(i=0;i<8;i++)
	{
		x.uint(i+1,vv);
		if(vv!=1) return -(i+1);
	}
	return 0;
}


DEFTEST(test_parse_uint16_t,"parse uint16_t of various sizes");
MAKEDEP(test_parse_uint16_t,test_parse_uint8_t);
int test_parse_uint16_t()
{
	//10100100 01000010 00001000 00010000 00010000
	//00001000 00000010 00000000 01000000 00000100
	//00000000 00100000 00000000 10000000 00000001
	//00000000 00000001
	const unsigned char bytes[]={0xa4,0x42,0x08,0x10,0x10,
								 0x08,0x02,0x00,0x40,0x04,
								 0x00,0x20,0x00,0x80,0x01,
								 0x00,0x01};
	iox x=iox::parse_binary(bytes,sizeof(bytes));
	uint16_t vv;
	int i;
	for(i=0;i<16;i++)
	{
		x.uint(i+1,vv);
		if(vv!=1) return -(i+1);
	}
	return 0;
}


DEFTEST(test_parse_uint32_t,"parse uint32_t of various sizes");
MAKEDEP(test_parse_uint32_t,test_parse_uint16_t);
int test_parse_uint32_t()
{
	//10100100 01000010 00001000 00010000 00010000
	//00001000 00000010 00000000 01000000 00000100
	//00000000 00100000 00000000 10000000 00000001
	//00000000 00000001
	//00000000 00000000 10000000 00000000 00100000
	//00000000 00000100 00000000 00000000 01000000
	//00000000 00000010 00000000 00000000 00001000
	//00000000 00000000 00010000 00000000 00000000
	//0001----

	const unsigned char bytes[]={0xa4,0x42,0x08,0x10,0x10,
								 0x08,0x02,0x00,0x40,0x04,
								 0x00,0x20,0x00,0x80,0x01,
								 0x00,0x01,
								 0x00,0x00,0x80,0x00,0x20,
								 0x00,0x04,0x00,0x00,0x40,
								 0x00,0x02,0x00,0x00,0x08,
								 0x00,0x00,0x10,0x00,0x00,
								 0x10};
	iox x=iox::parse_binary(bytes,sizeof(bytes));
	uint32_t vv;
	int i;
	for(i=0;i<24;i++)
	{
		x.uint(i+1,vv);
		if(vv!=1) return -(i+1);
	}
	return 0;
}

DEFTEST(test_parse_uint8_t_extensive,"parse uint8_t in all positions and sizes");
MAKEDEP(test_parse_uint8_t_extensive,test_parse_uint8_t);
int test_parse_uint8_t_extensive()
{
	uint8_t bytes[1];
	for(int pos=0;pos<8;pos++)
	{
		for(int width=1;width<8-pos;width++)
		{
			for(int i=0;i<100;i++)
			{
				uint32_t v,value;
				value=(i*123456)%(1<<width);
				v=value<<(8-pos-width);
				bytes[0]=v;
				iox x=iox::parse_binary(bytes,1);
				uint8_t skip;
				uint8_t read_value;
				if(pos!=0)
				{
					x.uint(pos,skip);
					if(skip!=0) return -(pos*100000+width*1000+i*10);
				}
				x.uint(width,read_value);
				if(read_value!=value) return -(pos*100000+width*1000+i*10+1);
			}
		}
	}
	return 0;
}

DEFTEST(test_parse_uint16_t_extensive,"parse uint16_t in all positions and sizes");
MAKEDEP(test_parse_uint16_t_extensive,test_parse_uint16_t);
int test_parse_uint16_t_extensive()
{
	uint8_t bytes[2];
	for(int pos=0;pos<16;pos++)
	{
		for(int width=1;width<16-pos;width++)
		{
			for(int i=0;i<100;i++)
			{
				uint32_t v,value;
				value=(i*123456)%(1<<width);
				v=value<<(16-pos-width);
				bytes[0]=v>>8;
				bytes[1]=v;
				iox x=iox::parse_binary(bytes,2);
				uint16_t skip;
				uint16_t read_value;
				if(pos!=0)
				{
					x.uint(pos,skip);
					if(skip!=0) return -(pos*100000+width*1000+i*10);
				}
				x.uint(width,read_value);
				if(read_value!=value) return -(pos*100000+width*1000+i*10+1);
			}
		}
	}
	return 0;
}

DEFTEST(test_parse_uint32_t_extensive,"parse uint32_t in all positions and sizes");
MAKEDEP(test_parse_uint32_t_extensive,test_parse_uint32_t);
int test_parse_uint32_t_extensive()
{
	uint8_t bytes[4];
	for(int pos=0;pos<32;pos++)
	{
		for(int width=1;width<32-pos;width++)
		{
			for(int i=0;i<100;i++)
			{
				uint32_t v,value;
				value=(i*123456)%(1<<width);
				v=value<<(32-pos-width);
				bytes[0]=v>>24;
				bytes[1]=v>>16;
				bytes[2]=v>>8;
				bytes[3]=v>>0;
				iox x=iox::parse_binary(bytes,4);
				uint32_t skip;
				uint32_t read_value;
				if(pos!=0)
				{
					x.uint(pos,skip);
					if(skip!=0) return -(pos*100000+width*1000+i*10);
				}
				x.uint(width,read_value);
				if(read_value!=value) return -(pos*100000+width*1000+i*10+1);
			}
		}
	}
	return 0;
}

//%d (%s:%d) parsing '%s'
void parseExceptionMsg(const std::string msg,std::string& file,uint32_t& line,std::string& name)
{
	size_t a,b,c,d,e;
	d=msg.find('\'');
	e=msg.find('\'',d+1);
	a=msg.find('(',e);
	b=msg.find(':',a);
	c=msg.find(')',b);

	file=msg.substr(a+1,b-a-1);
	line=atoi(msg.substr(b+1).c_str());
	name=msg.substr(d+1,e-d-1);
	printf("%s\n$\n",msg.c_str());
	printf("%s| %d| %s\n",file.c_str(),line,name.c_str());
}


DEFTEST(test_parse_exception,"generic catch exception from parse");
MAKEDEP(test_parse_exception,test_parse_uint8_t_extensive);
int test_parse_exception()
{
	const unsigned char bytes[]={0};
	iox x=iox::parse_binary(bytes,sizeof(bytes));
	unsigned char t1,t2;
	try
	{
	x.uint(7,t1);
	x.uint(7,t2);
	}
	catch(const Exception& e)
	{
		return 0;
	}
	catch(...)
	{
		return -1;
	}
	return -2;
}


DEFTEST(test_parse_exception_uint8_t,"parse exception from uint8_t");
MAKEDEP(test_parse_exception_uint8_t,test_parse_exception);
int test_parse_exception_uint8_t()
{
	const unsigned char bytes[]={0};
	iox x=iox::parse_binary(bytes,sizeof(bytes));
	unsigned char t1,t2;
	int marker;
	try
	{
	x.uint(7,DVB_VAR(t1));
	marker=__LINE__;x.uint(7,DVB_VAR(t2));
	}
	catch(const Exception& e)
	{
		string file;
		uint32_t line;
		string name;
		parseExceptionMsg(e.message,file,line,name);
		if(file!=__FILE__) return -2;
		if(line!=marker) return -3;
		if(name!="t2") return -4;
		return 0;
	}
	return -1;
}



DEFTEST(test_parse_exception_uint16_t,"parse exception from uint16_t");
MAKEDEP(test_parse_exception_uint16_t,test_parse_exception);
int test_parse_exception_uint16_t()
{
	const unsigned char bytes[]={0};
	iox x=iox::parse_binary(bytes,sizeof(bytes));
	unsigned short t1;
	int marker;
	try
	{
		marker=__LINE__;x.uint(9,DVB_VAR(t1));
	}
	catch(const Exception& e)
	{
		string file;
		uint32_t line;
		string name;
		parseExceptionMsg(e.message,file,line,name);
		if(file!=__FILE__) return -2;
		if(line!=marker) return -3;
		if(name!="t1") return -4;
		return 0;
	}
	return -1;
}

DEFTEST(test_parse_exception_uint32_t,"parse exception from uint32_t");
MAKEDEP(test_parse_exception_uint32_t,test_parse_exception);
int test_parse_exception_uint32_t()
{
	const unsigned char bytes[]={0};
	iox x=iox::parse_binary(bytes,sizeof(bytes));
	uint32_t t1;
	int marker;
	try
	{
		marker=__LINE__;x.uint(9,DVB_VAR(t1));
	}
	catch(const Exception& e)
	{
		string file;
		uint32_t line;
		string name;
		parseExceptionMsg(e.message,file,line,name);
		if(file!=__FILE__) return -2;
		if(line!=marker) return -3;
		if(name!="t1") return -4;
		return 0;
	}
	return -1;
}


DEFTEST(test_basic_construct_uint8_t,"construct uint8_t of various sizes");
int test_basic_construct_uint8_t()
{
	//10100100 01000010 00001000 00010000 0001----
	const unsigned char bytes[]={0xa4,0x42,0x08,0x10,0x10};
	unsigned char data[100]={0};
	iox x=iox::construct_binary(data,100);
	uint8_t v=1;
	int i;
	for(i=0;i<8;i++)
	{
		x.uint(i+1,v);
	}
	if(memcmp(bytes,data,sizeof(bytes))!=0) return -1;
	return 0;
}


DEFTEST(test_basic_construct_uint16_t,"construct uint16_t of various sizes");
int test_basic_construct_uint16_t()
{
	//10100100 01000010 00001000 00010000 00010000
	//00001000 00000010 00000000 01000000 00000100
	//00000000 00100000 00000000 10000000 00000001
	//00000000 00000001
	const unsigned char bytes[]={0xa4,0x42,0x08,0x10,0x10,
								 0x08,0x02,0x00,0x40,0x04,
								 0x00,0x20,0x00,0x80,0x01,
								 0x00,0x01};
	unsigned char data[100]={0};
	iox x=iox::construct_binary(data,100);
	uint16_t v=1;
	int i;
	for(i=0;i<16;i++)
	{
		x.uint(i+1,v);
	}
	int d=memcmp(bytes,data,sizeof(bytes));
	return d;
	if(memcmp(bytes,data,sizeof(bytes))!=0) return -1;
	return 0;
}


DEFTEST(test_basic_construct_uint32_t,"construct uint32_t of various sizes");
int test_basic_construct_uint32_t()
{
	//10100100 01000010 00001000 00010000 00010000
	//00001000 00000010 00000000 01000000 00000100
	//00000000 00100000 00000000 10000000 00000001
	//00000000 00000001
	//00000000 00000000 10000000 00000000 00100000
	//00000000 00000100 00000000 00000000 01000000
	//00000000 00000010 00000000 00000000 00001000
	//00000000 00000000 00010000 00000000 00000000
	//0001----

	const unsigned char bytes[]={0xa4,0x42,0x08,0x10,0x10,
								 0x08,0x02,0x00,0x40,0x04,
								 0x00,0x20,0x00,0x80,0x01,
								 0x00,0x01,
								 0x00,0x00,0x80,0x00,0x20,
								 0x00,0x04,0x00,0x00,0x40,
								 0x00,0x02,0x00,0x00,0x08,
								 0x00,0x00,0x10,0x00,0x00,
								 0x10};
	unsigned char data[100]={0};
	iox x=iox::construct_binary(data,100);
	x=iox::construct_binary(data,100);

	uint32_t v=1;
	int i;
	for(i=0;i<24;i++)
	{
		x.uint(i+1,v);
	}
	if(memcmp(bytes,data,sizeof(bytes))!=0) return -1;
	return 0;
}



DEFTEST(test_block_parsing,"test simple block parsing");
MAKEDEP(test_block_parsing,test_parse_uint8_t_extensive);
int test_block_parsing()
{
	const unsigned char bytes[]={0xab,0x01,0xcd,0xef,0x78};
	iox x=iox::parse_binary(bytes,3);
	uint8_t v;
	uint8_t marker_v;
	int marker;
	int i;
	try
	{
		x.uint(4,DVB_VAR(v));
		if(v!=0xa) return -1;
		x.uint(4,DVB_VAR(v));
		if(v!=0xb) return -2;
		x.named_block_begin(8,DVB_INFO("length_of_items"));
		{
			x.uint(4,DVB_VAR(v));
			if(v!=0xc) return -3;
			x.uint(4,DVB_VAR(v));
			if(v!=0xd) return -4;
		}
		v=x.named_block_end(DVB_INFO(""));
		if(v!=1) return -5;
	}
	catch(const Exception& e)
	{
		return -6;
	}
	return 0;
}


DEFTEST(test_block_alignment_on_enter,"test byte alignment when block opens");
MAKEDEP(test_block_alignment_on_enter,test_block_parsing,test_parse_uint8_t_extensive);
int test_block_alignment_on_enter()
{
	const unsigned char bytes[]={0xab,0x01,0xcd,0xef,0x78};

	uint8_t v;
	uint8_t marker_v;
	int marker;
	int i;

	iox x=iox::parse_binary(bytes,5);
	try
	{
		x.uint(7,DVB_VAR(v));
		marker=__LINE__;x.named_block_begin(8,DVB_INFO("length_of_items"));
	}
	catch(const Exception& e)
	{
		string file;
		uint32_t line;
		string name;
		parseExceptionMsg(e.message,file,line,name);
		if(file!=__FILE__) return -1;
		if(line!=marker) return -2;
		if(name!="length_of_items") return -3;
		return 0;
	}
	//exception not thrown
	return -4;
}

DEFTEST(test_block_alignment_on_exit,"test byte alignment when block closes");
MAKEDEP(test_block_alignment_on_exit,test_block_parsing,test_parse_uint8_t_extensive);
int test_block_alignment_on_exit()
{
	const unsigned char bytes[]={0xab,0x01,0xcd,0xef,0x78};

	uint8_t v;
	uint8_t marker_v;
	int marker;
	int i;

	iox x=iox::parse_binary(bytes,5);
	try
	{
		x.uint(8,DVB_VAR(v));
		x.named_block_begin(8,DVB_INFO("length_of_items"));
		{
			x.uint(7,DVB_VAR(v));
		}
		marker=__LINE__;v=x.named_block_end(DVB_INFO("block_end"));
	}
	catch(const Exception& e)
	{
		string file;
		uint32_t line;
		string name;
		parseExceptionMsg(e.message,file,line,name);
		if(file!=__FILE__) return -1;
		if(line!=marker) return -2;
		if(name!="block_end") return -3;
		return 0;
	}
	//exception not thrown
	return -4;
}




DEFTEST(test_text_uint8_t_extensive,"construct text uint8_t values");
MAKEDEP(test_text_uint8_t_extensive,test_parse_uint8_t);
int test_text_uint8_t_extensive()
{
	iox x=iox::construct_text();
	for(int width=1;width<=8;width++)
	{
		for(int i=0;i<100;i++)
		{
			uint32_t value;
			value=(i*6337)%(1<<width);
			x.uint(width,value,DVB_INFO("value"));
		}
	}
	//printf("%s\n",x.as_ox().as_ocx().ctx->prod.c_str());
	iox y=iox::parse_text(x.as_ox().as_ocx().ctx->prod.c_str());
	try
	{
		for(int width=1;width<=8;width++)
		{
			for(int i=0;i<100;i++)
			{
				uint32_t value;
				y.uint(width,value,DVB_INFO("value"));
				//printf("%d %d %d\n",width,i,value);
				if(value!=(i*6337)%(1<<width)) return -(width*10000+i*10);
			}
		}
	}
	catch(const Exception& e)
	{
		return -1;
		//printf("%s",e.message.c_str());
	}
	return 0;
}

DEFTEST(test_text_construct_parse_consistency,"check parse/construct consistency");
MAKEDEP(test_text_construct_parse_consistency,test_parse_uint8_t);
int test_text_construct_parse_consistency()
{
	iox x=iox::construct_text();
	string prod;
	unsigned int a=1,b=2,c=2,d=4,e=5,f=6,g=7;
	try
	{
		for(int i=0;i<2;i++)
		{
			if(i==1)
			{
				prod=x.as_ox().as_ocx().ctx->prod;
				x=iox::parse_text(prod.c_str());
				a=b=c=d=e=f=g=0;
			}
			x.uint(3,DVB_VAR(a));
			x.uint(2,DVB_VAR(b));
			x.named_block_begin(3,DVB_INFO("c"));
			x.uint(6,DVB_VAR(d));
			x.uint(5,DVB_VAR(e));
			x.uint(5,DVB_VAR(f));
			c=x.named_block_end(DVB_INFO("c"));
			x.uint(8,DVB_VAR(g));
		}
		if(a!=1)return -1;
		if(b!=2)return -2;
		if(c!=2)return -3;
		if(d!=4)return -4;
		if(e!=5)return -5;
		if(f!=6)return -6;
		if(g!=7)return -7;
	}
	catch(const Exception& e)
	{
		printf("%s",e.message.c_str());
		return -8;
	}
	return 0;
}


DEFTEST(test_construct_block_1,"test exception when block content exceeds size");
MAKEDEP(test_construct_block_1,test_text_uint8_t_extensive);
int test_construct_block_1()
{
	int marker=0;
	iox x=iox::construct_text();
	try
	{
		uint32_t a=1,b=2,c=3,d=4,e=5,f=6;
		x.uint(6,DVB_VAR(a));
		x.named_block_begin(2,DVB_INFO("b"));
		x.uint(8,DVB_VAR(c));
		x.uint(8,DVB_VAR(d));
		x.uint(8,DVB_VAR(e));
		marker=__LINE__;x.uint(8,DVB_VAR(f));
	}
	catch(const Exception& e)
	{
		string file;
		uint32_t line;
		string name;
		parseExceptionMsg(e.message,file,line,name);
		if(file!=__FILE__) return -1;
		if(line!=marker) return -2;
		if(name!="f") return -3;
		return 0;
	}
	return -4;
}


DEFTEST(test_construct_block_2,"test exception when multiple blocks content exceeds size");
MAKEDEP(test_construct_block_2,test_construct_block_1,test_text_uint8_t_extensive);
int test_construct_block_2()
{
	int marker=0;
	iox x=iox::construct_text();
	try
	{
		uint32_t a=1,b=2,c=3,d=4,e=5,f=6,g=7;
		x.uint(6,DVB_VAR(a));
		x.named_block_begin(2,DVB_INFO("b"));
		x.uint(6,DVB_VAR(a));
		x.named_block_begin(2,DVB_INFO("b"));
		x.uint(6,DVB_VAR(a));
		x.named_block_begin(2,DVB_INFO("b"));
		x.uint(6,DVB_VAR(a));
		x.named_block_begin(2,DVB_INFO("b"));
		marker=__LINE__;x.uint(6,DVB_VAR(c));
	}
	catch(const Exception& e)
	{
		string file;
		uint32_t line;
		string name;
		parseExceptionMsg(e.message,file,line,name);
		if(file!=__FILE__) return -1;
		if(line!=marker) return -2;
		if(name!="c") return -3;
		return 0;
	}
	return -4;
}



DEFTEST(test_construct_exception_1,"manual: text of exception for value too large");
MAKEDEP(test_construct_exception_1,test_text_uint8_t_extensive);
int test_construct_exception_1()
{
	iox x=iox::construct_text();
	try
	{
		uint32_t a=8;
		x.uint(3,DVB_VAR(a));
	}
	catch(const Exception& e)
	{
		printf("%s\n",e.message.c_str());
		return 0;
	}
	return -1;
}


DEFTEST(test_construct_exception_2,"manual: text of exception for unaligned block");
MAKEDEP(test_construct_exception_2,test_text_uint8_t_extensive);
int test_construct_exception_2()
{
	iox x=iox::construct_text();
	try
	{
		x.named_block_begin(3,DVB_INFO("a"));
	}
	catch(const Exception& e)
	{
		printf("%s\n",e.message.c_str());
		return 0;
	}
	return -1;
}


DEFTEST(test_construct_exception_3,"manual: text of exception for callstack");
MAKEDEP(test_construct_exception_3,test_text_uint8_t_extensive,test_construct_exception_2);
int test_construct_exception_3()
{
	iox x=iox::construct_text();
	try
	{
		x.named_block_begin(8,DVB_INFO("a"));
		x.named_block_begin(8,DVB_INFO("b"));
		x.named_block_begin(3,DVB_INFO("c"));
	}
	catch(const Exception& e)
	{
		printf("%s\n",e.message.c_str());
		return 0;
	}
	return -1;
}


DEFTEST(test_construct_exception_4,"manual: text of exception for unaligned block size");
MAKEDEP(test_construct_exception_4,test_text_uint8_t_extensive);
int test_construct_exception_4()
{
	iox x=iox::construct_text();
	try
	{
		x.named_block_begin(8,DVB_INFO("a"));
		uint32_t b=7;
		x.uint(3,DVB_VAR(b));
		x.named_block_end(DVB_INFO("a"));
	}
	catch(const Exception& e)
	{
		printf("%s\n",e.message.c_str());
		return 0;
	}
	return -1;
}


DEFTEST(test_construct_exception_5,"manual: text of exception when no size for block");
MAKEDEP(test_construct_exception_5,test_text_uint8_t_extensive);
int test_construct_exception_5()
{
	iox x=iox::construct_text();
	try
	{
		for(int i=0;i<17;i++)
		{
			uint32_t a=7;
			x.uint(6,DVB_VAR(a));
			x.named_block_begin(2,DVB_INFO("b"));
		}
		x.named_block_begin(3,DVB_INFO("b"));
	}
	catch(const Exception& e)
	{
		printf("%s\n",e.message.c_str());
		return 0;
	}
	return -1;
}



DEFTEST(test_short_string_extensive_text,"extensive test for construction/parsing short string (text)");
int test_short_string_extensive_text()
{
	iox x(iox::construct_text());
	try
	{
		for(int i=0;i<1000;i++)
		{
			uint32_t len;
			len=(i*23+(i%5)*117)%256;
			string str;
			str.resize(len);
			for(int j=0;j<len;j++)
			{
				str[j]=(i*71+i*j*29+j*213)%256;
			}
			short_string_io(x,DVB_VAR(str));
		}
		iox y(iox::parse_text(x.as_ocx().ctx->prod.c_str()));
		for(int i=0;i<1000;i++)
		{
			uint32_t len;
			len=(i*23+(i%5)*117)%256;
			string s;
			string str;
			s.resize(len);
			for(int j=0;j<len;j++)
			{
				s[j]=(i*71+i*j*29+j*213)%256;
			}
			short_string_io(y,DVB_VAR(str));
			if(s!=str) return -1000-i;
		}

	}
	catch(const Exception& e)
	{
		return -1;
	}
	return 0;
}


DEFTEST(test_short_string_extensive_bin,"extensive test for construction/parsing short string (binary)");
int test_short_string_extensive_bin()
{
	uint8_t data[256000];
	iox x(iox::construct_binary(data,256000));
	try
	{
		for(int i=0;i<1000;i++)
		{
			uint32_t len;
			len=(i*23+(i%5)*117)%256;
			string str;
			str.resize(len);
			for(int j=0;j<len;j++)
			{
				str[j]=(i*71+i*j*29+j*213)%256;
			}
			short_string_io(x,DVB_VAR(str));
		}
		iox y(iox::parse_binary(data,x.as_obx().ctx->bitpos/8));
		for(int i=0;i<1000;i++)
		{
			uint32_t len;
			len=(i*23+(i%5)*117)%256;
			string s;
			string str;
			s.resize(len);
			for(int j=0;j<len;j++)
			{
				s[j]=(i*71+i*j*29+j*213)%256;
			}
			short_string_io(y,DVB_VAR(str));
			if(s!=str) return -1000-i;
		}
	}
	catch(const Exception& e)
	{
		return -1;
	}
	return 0;
}


DEFTEST(test_fixed_strings_text,"test for construction/parsing of fixed-size strings (text)");
int test_fixed_strings_text()
{
	iox x(iox::construct_text());
	try
	{
		std::string a="X";
		std::string b="AB";
		std::string c="123";
		std::string d="abcd";
		std::string e="fixed";
		std::string f="sixsix";
		std::string g="!@#$%^&";
		std::string h="87654321";
		fixed_string_io(x,1,DVB_VAR(a));
		fixed_string_io(x,2,DVB_VAR(b));
		fixed_string_io(x,3,DVB_VAR(c));
		fixed_string_io(x,4,DVB_VAR(d));
		fixed_string_io(x,5,DVB_VAR(e));
		fixed_string_io(x,6,DVB_VAR(f));
		fixed_string_io(x,7,DVB_VAR(g));
		fixed_string_io(x,8,DVB_VAR(h));
		a="";b="";c="";d="";e="";f="";g="";h="";
		iox y(iox::parse_text(x.as_ocx().ctx->prod.c_str()));
		fixed_string_io(y,1,DVB_VAR(a));
		fixed_string_io(y,2,DVB_VAR(b));
		fixed_string_io(y,3,DVB_VAR(c));
		fixed_string_io(y,4,DVB_VAR(d));
		fixed_string_io(y,5,DVB_VAR(e));
		fixed_string_io(y,6,DVB_VAR(f));
		fixed_string_io(y,7,DVB_VAR(g));
		fixed_string_io(y,8,DVB_VAR(h));
		if(a!="X") return -1;
		if(b!="AB") return -2;
		if(c!="123") return -3;
		if(d!="abcd") return -4;
		if(e!="fixed") return -5;
		if(f!="sixsix") return -6;
		if(g!="!@#$%^&") return -7;
		if(h!="87654321") return -8;
	}
	catch(const Exception& e)
	{
		return -9;
	}
	return 0;
}

DEFTEST(test_fixed_strings_bin,"test for construction/parsing of fixed-size strings (binary)");
int test_fixed_strings_bin()
{
	uint8_t data[1000]={0};
	iox x(iox::construct_binary(data,1000));
	try
	{
		std::string a="X";
		std::string b="AB";
		std::string c="123";
		std::string d="abcd";
		std::string e="fixed";
		std::string f="sixsix";
		std::string g="!@#$%^&";
		std::string h="87654321";
		fixed_string_io(x,1,DVB_VAR(a));
		fixed_string_io(x,2,DVB_VAR(b));
		fixed_string_io(x,3,DVB_VAR(c));
		fixed_string_io(x,4,DVB_VAR(d));
		fixed_string_io(x,5,DVB_VAR(e));
		fixed_string_io(x,6,DVB_VAR(f));
		fixed_string_io(x,7,DVB_VAR(g));
		fixed_string_io(x,8,DVB_VAR(h));
		a="";b="";c="";d="";e="";f="";g="";h="";
		iox y(iox::parse_binary(data,x.as_obx().ctx->bitpos/8));
		fixed_string_io(y,1,DVB_VAR(a));
		fixed_string_io(y,2,DVB_VAR(b));
		fixed_string_io(y,3,DVB_VAR(c));
		fixed_string_io(y,4,DVB_VAR(d));
		fixed_string_io(y,5,DVB_VAR(e));
		fixed_string_io(y,6,DVB_VAR(f));
		fixed_string_io(y,7,DVB_VAR(g));
		fixed_string_io(y,8,DVB_VAR(h));
		if(a!="X") return -1;
		if(b!="AB") return -2;
		if(c!="123") return -3;
		if(d!="abcd") return -4;
		if(e!="fixed") return -5;
		if(f!="sixsix") return -6;
		if(g!="!@#$%^&") return -7;
		if(h!="87654321") return -8;
	}
	catch(const Exception& e)
	{
		return -9;
	}
	return 0;
}
DEFTEST(test_string_exception_1,"test invalid char in string");
int test_string_exception_1()
{
	try
	{
		iox y(iox::parse_text("A:'\\400'"));
		string A;
		short_string_io(y,DVB_VAR(A));
	}
	catch(const Exception& e)
	{
		return 0;
	}
	return -1;
}

DEFTEST(test_string_exception_2,"test invalid char in string");
int test_string_exception_2()
{
	try
	{
		iox y(iox::parse_text("A:'\\x400'"));
		string A;
		short_string_io(y,DVB_VAR(A));
	}
	catch(const Exception& e)
	{
		return 0;
	}
	return -1;
}

DEFTEST(test_string_exception_3,"test invalid char in string");
int test_string_exception_3()
{
	try
	{
		iox y(iox::parse_text("A:'\\4n00'"));
		string A;
		short_string_io(y,DVB_VAR(A));
	}
	catch(const Exception& e)
	{
		return 0;
	}
	return -1;
}

DEFTEST(test_string_exception_4,"test invalid char in string");
int test_string_exception_4()
{
	try
	{
		iox y(iox::parse_text("A:'\\40y0'"));
		string A;
		short_string_io(y,DVB_VAR(A));
	}
	catch(const Exception& e)
	{
		return 0;
	}
	return -1;
}

DEFTEST(test_string_exception_5,"test invalid char in string");
int test_string_exception_5()
{
	try
	{
		iox y(iox::parse_text("A:'\001'"));
		string A;
		short_string_io(y,DVB_VAR(A));
	}
	catch(const Exception& e)
	{
		return 0;
	}
	return -1;
}

DEFTEST(test_ts_specification_1,"test ts_specification parsing binary");
int test_ts_specification_1()
{
	const unsigned char bytes[]={0xaa,0xaa,0xcc,0xcc,0xf0,0x06,0x01,0x00,0x02,0x00,0x03,0x00};
	iox x=iox::parse_binary(bytes,sizeof(bytes));
	struct ts_specification T={0};
	try
	{
		T.io(x);
	}
	catch(const Exception& e)
	{
		return -1;
	}
	if(T.transport_stream_id!=0xaaaa) return -2;
	if(T.original_network_id!=0xcccc) return -3;
#if 0
	if(T.transport_descriptors_length!=6) return -4;
	if(T.transport_descriptors.size()!=3) return -5;
	if(T.transport_descriptors[0]->tag!=1) return -6;
	if(T.transport_descriptors[0]->length!=0) return -7;
	if(T.transport_descriptors[1]->tag!=2) return -8;
	if(T.transport_descriptors[1]->length!=0) return -9;
	if(T.transport_descriptors[2]->tag!=3) return -10;
	if(T.transport_descriptors[2]->length!=0) return -11;
#endif
	iox y=iox::construct_text();
	try
	{
		T.io(y);
	}
	catch(const Exception& e)
	{
		return -12;
	}
	return 0;
}


DEFTEST(test_nit_table_parsing_1,"test parsing of example NIT tables");
int test_nit_table_parsing_1()
{
	const char* FILES[]={
			"tests/data/Bromley_NIT.sec",
			"tests/data/BBC_NIT.sec",
			"tests/data/MUX1_NIT.sec",
			"tests/data/MUX3_NIT.sec"};
	int file;
	for(file=0;file<sizeof(FILES)/sizeof(*FILES);file++)
	{

		int fd;
		fd=open(FILES[file],O_RDONLY);
		if(fd<0) return -10000*file-1;
		uint8_t data[2000];
		uint8_t out[2000];
		int r;
		r=read(fd,data,2000);
		close(fd);
		//if(r!=715) return -2;

		iox x=iox::parse_binary(data,r);
		iox y=iox::construct_text();//out,2000);
		struct network_information_section T={0};
		try
		{
			T.io(x);
			T.io(y);
			//int i;
			//for(i=0;i<r;i++)
			//	if(data[i]!=out[i]) return -1000-i;
			//printf("%s\n",y.as_ocx().ctx->prod.c_str());
			iox z=iox::parse_text(y.as_ocx().ctx->prod.c_str());
			iox v=iox::construct_binary(out,sizeof(out));
			struct network_information_section T1={0};
			T1.io(z);
			T1.io(v);
			int i;
			int R=v.as_obx().ctx->bitpos/8;
			if(r!=R) return -10000*file-1000;
			for(i=0;i<R;i++)
				if(data[i]!=out[i]) return -10000*file-1000-i;
		}
		catch(const Exception& e)
		{
			//printf("%s\n",y.as_ocx().ctx->prod.c_str());
			printf("%s\n",e.message.c_str());
			return -10000*file-2;
		}
	}
	return 0;
}

DEFTEST(test_nit_table_parsing_2,"test parsing of example NIT table with error injections");
MAKEDEP(test_nit_table_parsing_2,test_nit_table_parsing_1);
int test_nit_table_parsing_2()
{
	for(int iter=0;iter<100;iter++)
	{
		int fd;
		fd=open("tests/data/Bromley_NIT.sec",O_RDONLY);
		if(fd<0) return -1;
		uint8_t data[2000];
		uint8_t out[2000];
		int r;
		r=read(fd,data,2000);
		close(fd);
		if(r!=715) return -2;
		int pos=((iter+10)*6317) % (r*8);
		data[pos/8]^=1<<(r&7);

		iox x=iox::parse_binary(data,r);
		iox y=iox::construct_text();//out,2000);
		struct network_information_section T={0};
		try
		{
			T.io(x);
		}
		catch(const Exception& e)
		{
			//printf("<<<<<%s\n",e.message.c_str());
			try
			{
				y.ctx->bitlimit=x.ctx->bitpos;
				T.io(y);
			}

			catch(const Exception& e)
			{
				printf("%s\n",y.as_ocx().ctx->prod.c_str());
				printf(">>>>>%s\n",e.message.c_str());
				return 0;
				//return -1;
			}
		}
	}
	return -1;
}


DEFTEST(test_nit_table_parsing_3,"test parsing of example NIT table with any possible bit change");
MAKEDEP(test_nit_table_parsing_3,test_nit_table_parsing_2);
int test_nit_table_parsing_3()
{
	for(int iter=0;iter<715*8;iter++)
	{
		int fd;
		fd=open("tests/data/Bromley_NIT.sec",O_RDONLY);
		if(fd<0) return -1;
		uint8_t data[2000];
		uint8_t out[2000];
		int r;
		r=read(fd,data,2000);
		close(fd);
		if(r!=715) return -2;
		//int pos=((iter+10)*6317) % (r*8);
		data[iter/8]^=1<<(iter&7);

		iox x=iox::parse_binary(data,r);
		iox y=iox::construct_text();//out,2000);
		struct network_information_section T={0};
		try
		{
			T.io(x);
		}
		catch(const Exception& e)
		{
			try
			{
				y.ctx->bitlimit=x.ctx->bitpos;
				T.io(y);
			}

			catch(const Exception& e)
			{
				//printf("%s\n",y.as_ocx().ctx->prod.c_str());
				//printf("%s\n",e.message.c_str());
				//return 0;
				//return -1;
			}
		}
	}
	return 0;
}

DEFTEST(test_bbc_eit_extract,"test extraction of EIT sections from bbc ts");
//MAKEDEP(test_bbc_eit_parse,test_nit_table_parsing_2);

void test_bbc_on_section(void* ctx,const uint8_t* section,size_t len)
{
	uint32_t crc=dvb_crc32(section,len);
	printf("SECTION len=%zu crc=%8.8x\n",len,crc);
}

int test_bbc_eit_extract()
{
	psi_extractor* p=psi_extractor::create(8192,0);
	p->on_section_ready(NULL,test_bbc_on_section);
	int fd;
	int result=0;
	fd=open("tests/data/bbc.pid18",O_RDONLY);
	uint8_t buffer[188];
	do
	{
		int r;
		r=read(fd,buffer,188);
		if(r!=188) break;
		p->ts_packet(buffer);
	}
	while(true);
	if(fd>0) close(fd);
	delete p;
	return result;
}


int main(int argc, char** argv)
{
	RUNTEST(test_bbc_eit_extract);

	RUNTEST(test_parse_byte);
	RUNTEST(test_parse_bits);
	RUNTEST(test_parse_uint8_t);
	RUNTEST(test_parse_uint16_t);
	RUNTEST(test_parse_uint32_t);
	RUNTEST(test_parse_uint8_t_extensive);
	RUNTEST(test_parse_uint16_t_extensive);
	RUNTEST(test_parse_uint32_t_extensive);

	RUNTEST(test_parse_exception);
	RUNTEST(test_parse_exception_uint8_t);
	RUNTEST(test_parse_exception_uint16_t);
	RUNTEST(test_parse_exception_uint32_t);

	RUNTEST(test_basic_construct_uint8_t);
	RUNTEST(test_basic_construct_uint16_t);
	RUNTEST(test_basic_construct_uint32_t);

	RUNTEST(test_block_parsing);
	RUNTEST(test_block_alignment_on_enter);
	RUNTEST(test_block_alignment_on_exit);

	RUNTEST(test_text_uint8_t_extensive);

	RUNTEST(test_text_construct_parse_consistency);
	RUNTEST(test_construct_block_1);
	RUNTEST(test_construct_block_2);
	RUNTEST(test_construct_exception_1);
	RUNTEST(test_construct_exception_2);
	RUNTEST(test_construct_exception_3);
	RUNTEST(test_construct_exception_4);
	RUNTEST(test_construct_exception_5);

	RUNTEST(test_short_string_extensive_text);
	RUNTEST(test_short_string_extensive_bin);

	RUNTEST(test_fixed_strings_text);
	RUNTEST(test_fixed_strings_bin);

	RUNTEST(test_string_exception_1);
	RUNTEST(test_string_exception_2);
	RUNTEST(test_string_exception_3);
	RUNTEST(test_string_exception_4);
	RUNTEST(test_string_exception_5);

	RUNTEST(test_ts_specification_1);
	RUNTEST(test_nit_table_parsing_1);
	RUNTEST(test_nit_table_parsing_2);
	RUNTEST(test_nit_table_parsing_3);

	RUNTEST(test_bbc_eit_extract);
//goto x;
}


