#ifndef _MOPA_NIT_
#define _MOPA_NIT_
#include "inc/io.h"
#include "inc/commontypes.h"
#include "inc/descriptors.h"


namespace mopa
{


struct ts_specification
{
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint8_t reserved_future_use;
	uint16_t transport_descriptors_length;
	descriptor_vector transport_descriptors;
	void io(iox& x)
	{
		x.uint(16,DVB_VAR(transport_stream_id));
		x.uint(16,DVB_VAR(original_network_id));
		x.uint_req(4,0xf,DVB_INFO("reserved_future_use"));
		x.named_block_begin(12,DVB_INFO("transport_descriptors_length"));
		transport_descriptors.io(x);
		transport_descriptors_length=x.named_block_end(DVB_INFO("transport_descriptors"));
	}
};



   
struct network_information_section
{
	uint8_t table_id;
	uint8_t section_syntax_indicator;
	uint16_t section_length;
	uint16_t network_id;
	uint8_t version_number;
	uint8_t current_next_indicator;
	uint8_t section_number;
	uint8_t last_section_number;
	uint16_t network_descriptors_length;
	descriptor_vector network_descriptors;
	uint16_t transport_stream_loop_length;
	std::vector<ts_specification> ts_loop;
	uint32_t CRC;
	void io(iox& x)
	{
		uint32_t nit_begin=x.ctx->bitpos;

		x.uint(8,DVB_VAR(table_id));
		x.uint(1,DVB_VAR(section_syntax_indicator));
		x.uint_req(1,1,DVB_INFO("reserved_future_use"));
		x.uint_req(2,0x3,DVB_INFO("reserved"));

		x.named_block_begin(12,DVB_INFO("section_length"));
		if(x.is_parsing())
			if(section_length>1021)
				throw Exception(x.ctx,DVB_INFO("section_length"),"NIT size exceeds 1024");

		x.uint(16,DVB_VAR(network_id));
		x.uint_req(2,0x3,DVB_INFO("reserved"));
		x.uint(5,DVB_VAR(version_number));
		x.uint(1,DVB_VAR(current_next_indicator));
		x.uint(8,DVB_VAR(section_number));
		x.uint(8,DVB_VAR(last_section_number));
		x.uint_req(4,0xf,DVB_INFO("reserved_future_use"));

		x.named_block_begin(12,DVB_INFO("network_descriptors_length"));
		network_descriptors.io(x);
		network_descriptors_length=x.named_block_end(DVB_INFO("network_descriptors_length"));

		x.uint_req(4,0xf,DVB_INFO("reserved_future_use"));
		x.named_block_begin(12,DVB_INFO("transport_stream_loop_length"));
		vector_io<ts_specification>(x,DVB_VAR(ts_loop));
		transport_stream_loop_length=x.named_block_end(DVB_INFO("transport_stream_loop_length"));

		uint32_t crc_pos=x.ctx->bitpos;
		crc_io(x,nit_begin,DVB_VAR(CRC));
		section_length=x.named_block_end(DVB_INFO("section_length"));
		crc_late_fix(x,nit_begin,crc_pos,DVB_VAR(CRC));
		if(!x.is_parsing())
			if(section_length>1021)
					throw Exception(x.ctx,DVB_INFO("section_length"),"NIT size exceeds 1024");


	}

};

}

#endif
