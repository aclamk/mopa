/**
 * \file
 * \mainpage
 * <b> MOre then PArser constructor toolkit (MOPA)</b>
 *
 *
 *
 * MOre then PArser (mopa) is a tool intended for DVB SI data processing.
 *
 * Its intention is to allow for parsing and constructing using the same code, therefore minimizing error possibilities.
 *
 *
 * For example definition of PDC descriptor (ETSI EN 300 468):
 * | Syntax | Number of bits | Identifier |
 * | :---- | :----: | :---- |
 * | PDC_descriptor(){    |      |     |
 * | descriptor_tag  | 8   | uimsbf  |
 * | descriptor_length  | 8   | uimsbf  |
 * | reserved_future_use  | 4   | bslbf  |
 * | programme_identification_label  | 20   | bslbf  |
 * | }  |    |   |
 *
 * may be represented as:
 *	\code
struct PDC_descriptor
{
	uint8_t descriptor_tag;
	uint8_t descriptor_length;
	uint32_t programme_identification_label;
	void io(iox& x)
	{
		x.uint(8,DVB_VAR(descriptor_tag));
		x.uint(8,DVB_VAR(descriptor_length));
		x.uint_req(4,0xf,DVB_INFO("reserved_future_use"));
		x.uint(20,DVB_VAR(programme_identification_label));
	}
};
 * \endcode
 *
 * <b>Operation modes</b>
 *
 * Mopa works in 4 modes:
 * | mode | |
 * | :- | :- |
 * | Parse binary bitstream to C++ data structures| \ref mopa::iox::parse_binary |
 * | Construct binary bitstream from C++ data structures| \ref mopa::iox::construct_binary |
 * | Parse text input to C++ data structures| \ref mopa::iox::parse_text |
 * | Construct text output from C++ data structures|\ref mopa::iox::construct_text |
 *
 * In all cases C++ data structures are either source or target of operation.
 *
 * In most common cases operations are performed on top-level \ref mopa::iox class.
 * Its methods are generalizations, suitable for each operation mode.
 * If this is sufficient, then this class should be used.
 *
 * However, in some cases behaviors for parsing and construction must be different.
 * In such cases mopa::iox can be converted to other, more specialized classed that will allow for required operations mopa::ix and mopa::ox.
 * Then, even these can be more specialized to binary and text cases.
 *
 * \dot
 * digraph example {
 * node [shape=record, fontname=Helvetica, fontsize=10];
 * edge [shape=record, fontname=Helvetica, fontsize=10];
 * iox [ label="class iox" URL="\ref mopa::iox"];
 * ix [ label="class ix" URL="\ref mopa::ix"];
 * ox [ label="class ox" URL="\ref mopa::ox"];
 * ibx [ label="class ibx" URL="\ref mopa::ibx"];
 * icx [ label="class icx" URL="\ref mopa::icx"];
 * obx [ label="class obx" URL="\ref mopa::obx"];
 * ocx [ label="class ocx" URL="\ref mopa::ocx"];
 * iox -> ix [ arrowhead="open", style="dashed",label="as_ix()" ];
 * iox -> ox [ arrowhead="open", style="dashed",label="as_ox()" ];
 * ix -> ibx [ arrowhead="open", style="dashed",label="as_ibx()" ];
 * ix -> icx [ arrowhead="open", style="dashed",label="as_icx()" ];
 * ox -> obx [ arrowhead="open", style="dashed",label="as_obx()" ];
 * ox -> ocx [ arrowhead="open", style="dashed",label="as_ocx()" ];
 * }
 * \enddot
 *
 */
#ifndef _DVB_IO_H
#define _DVB_IO_H
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <stdarg.h>
#include <string>

namespace mopa
{
class iox_info;
class iox;
class ix;
class ox;
class ibx;
class obx;
class icx;
class ocx;
/**
 * \brief Adds metadata to mopa operation
 *
 * This allows to give textual name to variables and locate operation in source code.
 * This struct should be used in cooperation with I/O operations like \ref mopa::iox::uint , \ref mopa::iox::named_block_begin .
 * Information from this struct is used in 2 cases:
 * - for textual I/O: 'name' is used as variable names, 'hint' can determine specific representation(decimal,hex,binary)
 * - when I/O error occurs it allows to present error in textual form

 * It can be most easily constructed using \ref DVB_INFO macro.
 */
struct iox_info
{
	const char* file;	/**< file for operation's location, as generated by \__FILE\__ */
	uint32_t line;		/**< line for operation's location, as generated by \__LINE\__ */
	const char* name;	/**< name of variable being operated on */
	uint32_t hint;		/**< optional hint, used to select format of output. \ref mopa::FORMAT_HINT_* are applicable here */
};

/**
 * \brief Masks used for \ref iox_info::hint
 */
const uint32_t FORMAT_HINT_MASK=0xf;/**< bit mask for bits used in hint mask. used in iox_info.hint*/
const uint32_t FORMAT_NOHINT=0x0;	/**< default handling. used in iox_info.hint*/
const uint32_t FORMAT_HINT_DEC=0x0;	/**< decimal output. this is default. used in iox_info.hint*/
const uint32_t FORMAT_HINT_HEX=0x1;	/**< hexadecimal output. used in iox_info.hint*/
const uint32_t FORMAT_HINT_BIN=0x2;	/**< binary output. used in iox_info.hint*/

/**
 * \addtogroup IOCTX Helper contexts for IO
 *
 * \details Group of contexts used in various modes of mopa.
 *
 * \{
 */

/** \brief Root context
 *
 * This is base context for mopa operations. It is used by iox class.*/
class ioCtx
{
public:
	/** \brief R/W position.
	 * \details Current location of read/write position. It is used in both binary and text mode.
	 In text mode it simulates writing data and changes bitpos accordingly.*/
	uint32_t bitpos;
	/** \brief R/W limit.
	 *  \details Limit for read/write. This value changes when scope block is entered \ref iox::named_block_begin
	 * and when it is exited \ref iox::named_block_end. Its role is to protect against read/write overruns. It points to first bit that is unavailable.*/
	uint32_t bitlimit;
	/** \brief Checks if binary
	 \return true iff in one of binary modes*/
	inline bool is_binary() const {return binary;}
	/** \brief Checks if parsing
	 * \return true iff in one of parsing modes*/
	inline bool is_parsing() const {return parsing;}
private:
	bool parsing;
	bool binary;
	friend class iox;
};
/** \brief Context for output
 *
 * This class is actually empty. It is created for structural completeness*/
class oCtx : public ioCtx
{};

/** \brief Context for input
 *
 * This class is actually empty. It is created for structural completeness*/
class iCtx : public ioCtx
{};

/**
 * \brief Scope for binary output mode
 */
struct obScope
{
	/**
	 * Info copied at obx::block_begin invocation.
	 */
	const iox_info* info;
	/**
	 * Saved ioCtx.bitpos at obx::block_begin.
	 */
	uint32_t bitpos_at_enter;
	/**
	 * Saved ioCtx.bitlimit at obx::block_begin.
	 */
	uint32_t bitlimit_at_enter;
	/**
	 * Saved position where length of block will be written at obx::block_end.
	 */
	uint32_t position_for_write;
};

/**
 * \brief Context for binary output mode
 */
class obCtx : public oCtx
{
public:
	/**
	 * \brief Constructed bitstream.
	 * \details This field is filled by iox::construct_binary. It must not been changed.
	 */
	uint8_t* data;
	/**
	 * \brief Stack to track syntactic blocks.
	 * \details This is used by obx::block_begin and obx::block_end.
	 * It is unadvised to change it in any case outside those functions.
	 */
	std::vector<obScope> scope_stack;
	/**
	 * \brief Puts integer to bitstream.
	 * \details
	 * Function treats obCtx.data as begin of bitstream of length ioCtx.bitlimit.
	 * It writes to this bitstream value \b val at location ioCtx.bitpos.
	 *
	 * Example of writing bitsize=7, ioCtx.bitpos=83:
	 * \code
	 * byte   |          bitpos/8=10          |         bitpos/8+1=11         |
	 * bit    |  7|  6|  5|  4|  3|  2|  1|  0|  7|  6|  5|  4|  3|  2|  1|  0|
	 * weigth |128| 64| 32| 16|  8|  4|  2|  1|128| 64| 32| 16|  8|  4|  2|  1|
	 *        |   |   |   |v.6|v.5|v.4|v.3|v.2|v.1|v.0|   |   |   |   |   |   |
	 * \endcode
	 * After writing ioCtx.bitpos is moved to new position.\n
	 * Function does not perform any range-checks. It is error to write if ioCtx.bitpos+<b>bitsize</b>>ioCtx.bitlimit.
	 *
	 * \note There are uint8_t, uint16_t and uint32_t variants.
	 */
	inline void nocheck_uint(int bitsize, uint8_t val)
	{
		if((bitpos&7) + bitsize<=8)
		{
			//fast route
			uint8_t mask=(1<<bitsize)-1;
			int lshift=8-(bitpos&7)-bitsize;
			data[bitpos/8]= (data[bitpos/8] & ~(mask << lshift )) | (val << lshift);
		}
		else
		{
			uint16_t tmp;
			tmp=(data[bitpos/8] << 8) | data[(bitpos/8)+1];
			uint16_t mask=(1<<bitsize)-1;
			int lshift=16-(bitpos&7)-bitsize;
			tmp=(tmp & ~(mask << lshift)) | (val << lshift);
			data[bitpos/8] = tmp>>8;
			data[bitpos/8+1] = tmp&0xff;
		}
		bitpos+=bitsize;
	}
	inline void nocheck_uint(int bitsize, uint16_t val)
	{
		if((bitpos&7) + bitsize<=16)
		{
			//fast route
			uint16_t mask=(1<<bitsize)-1;
			int lshift=16-(bitpos&7)-bitsize;
			uint16_t tmp;
			tmp=(data[bitpos/8]<<8) | data[bitpos/8+1];
			tmp=(tmp & ~(mask << lshift )) | (val << lshift);
			data[bitpos/8] = tmp>>8;
			data[bitpos/8+1] = tmp&0xff;
		}
		else
		{
			uint32_t tmp;
			tmp=(data[bitpos/8] << 16) | (data[bitpos/8+1] << 8) | data[bitpos/8+2];
			uint32_t mask=(1<<bitsize)-1;
			int lshift=24-(bitpos&7)-bitsize;
			tmp=(tmp & ~(mask << lshift)) | (val << lshift);
			data[bitpos/8] = tmp>>16;
			data[bitpos/8+1] = tmp>>8;
			data[bitpos/8+2] = tmp&0xff;
		}
		bitpos+=bitsize;
	}
	inline void nocheck_uint(int bitsize, uint32_t val)
	{
		if((bitpos&7) + bitsize<=32)
		{
			//fast route
			//uint32_t mask=(1<<bitsize)-1;
			uint32_t mask=(2<<(bitsize-1))-1; //fix for x86 stupid bitshift policy
			int lshift=32-(bitpos&7)-bitsize;
			uint32_t tmp=0;
			tmp=    (data[bitpos/8+0]<<24) | (data[bitpos/8+1]<<16) |
					(data[bitpos/8+2]<<8)  | (data[bitpos/8+3]);
			tmp=(tmp & ~(mask << lshift )) | (val << lshift);
			data[bitpos/8+0] = tmp>>24;
			data[bitpos/8+1] = tmp>>16;
			data[bitpos/8+2] = tmp>>8;
			data[bitpos/8+3] = tmp>>0;
			bitpos+=bitsize;
		}
	}
};


struct ocScope
{
	const iox_info* info;
	uint32_t bitpos_at_enter;
	uint32_t bitlimit_at_enter;
	uint32_t position_for_write;
};
class ocCtx : public oCtx
{
public:
	std::vector<ocScope> scope_stack;
	std::string prod;
	void enter_scope(const iox_info* info=NULL);
	void leave_scope(const iox_info* info=NULL);
	void write_uint(int bitsize, uint32_t value, const iox_info* info=NULL);
private:
	void write_hex(int bitsize,uint32_t value);
	void write_bin(int bitsize,uint32_t value);
	void write_dec(int bitsize,uint32_t value);
};


struct ibScope
{
	const iox_info* info;
	uint32_t bitpos_at_enter;
	uint32_t bitlimit_at_enter;
};
class ibCtx : public iCtx
{
public:
	const uint8_t *data;
	std::vector<ibScope> scope_stack;
	inline uint8_t nocheck_uint8(int bitsize)
	{
		uint8_t val;
		if((bitpos&7) + bitsize<=8)
		{
			//fast route
			val=((data[bitpos/8]<<(bitpos&7)) & 0xff) >>(8-bitsize);
		}
		else
		{
			val=((( (data[bitpos/8] << 8) | data[(bitpos/8)+1] ) << (bitpos & 7)) & 0xffff) >> (16-bitsize);
		}
		bitpos+=bitsize;
		return val;
	}
	inline uint16_t nocheck_uint16(int bitsize)
	{
		uint16_t val;
		if((bitpos&7) + bitsize<=16)
		{
			//fast route
			uint16_t tmp;
			tmp=(data[bitpos/8]<<8) | data[bitpos/8+1];
			val=((tmp<<(bitpos&7)) & 0xffff)>>(16-bitsize);
		}
		else
		{
			uint32_t tmp;
			tmp=(data[bitpos/8]<<16) | (data[bitpos/8+1]<<8) | data[bitpos/8+2];
			val=((tmp << (bitpos & 7)) & 0xffffff) >> (24-bitsize);
		}
		bitpos+=bitsize;
		return val;

	}
	inline uint32_t nocheck_uint32(int bitsize)
	{
		uint32_t val=0;
		if((bitpos&7) + bitsize<=32)
		{
			//fast route
			uint32_t tmp;
			tmp=    (data[bitpos/8+0]<<24) | (data[bitpos/8+1]<<16) |
					(data[bitpos/8+2]<<8)  | (data[bitpos/8+3]);
			val=tmp<<(bitpos&7)>>(32-bitsize);
			bitpos+=bitsize;
		}
		return val;
	}
};


struct icScope
{
	const iox_info* info;
	uint32_t bitpos_at_enter;
	uint32_t bitlimit_at_enter;
};
class icCtx : public iCtx
{
public:
	std::vector<icScope> scope_stack;
	const char* parsed_text;
	const char* parse_pos;
	void enter_scope(const iox_info* info=NULL);
	void leave_scope(const iox_info* info=NULL);
	uint32_t read_uint(int bitsize,  const iox_info* info=NULL);
	bool expect(const char* req);
	bool skiptotoken();
private:
	uint32_t read_hex(int bitsize,const iox_info* info);
	uint32_t read_bin(int bitsize,const iox_info* info);
	uint32_t read_dec(int bitsize,const iox_info* info);

};

/**
 *\}
 */


/**
 * \addtogroup IOX Classes for fundamental IO.
 * \{
 */

/**
 * \brief Top-level class for Input/Output operations
 *
 * It is used as context for I/O operations.
 */
class iox
{
public:
	~iox();
	iox(iox const &from);
	iox& operator=(iox const &rhs);
	/**
	 * \brief Create iox object for binary parsing mode
	 *
	 * \param data - binary data for parsing
	 * \param size - size (in bytes) of data to be parsed
	 * \retval iox object
	 */
	static iox parse_binary(const uint8_t* data, uint32_t size);
	/**
	 * \brief Create iox object for parsing textual representation
	 *
	 * \param text - c string containing text to parse
	 * \retval iox object
	 */
	static iox parse_text(const char* text);
	/**
	 * \brief Create iox object for binary construction mode
	 *
	 * \param data - buffer to write constructed data
	 * \param size - size (in bytes) of buffer
	 * \retval iox object
	 */
	static iox construct_binary(uint8_t* data, uint32_t size);
	/**
	 * \brief Create iox object for constructing textual representation
	 * \retval iox object
	 */
	static iox construct_text();

	ix as_ix();
	ox as_ox();
	ibx as_ibx();
	icx as_icx();
	obx as_obx();
	ocx as_ocx();
	void uint(int bitsize, uint8_t&  val, const iox_info* info=NULL);
	void uint(int bitsize, uint16_t& val, const iox_info* info=NULL);
	/**
	 * \brief Declares integer value
	 *
	 * \param bitsize - number of bits to read/write
	 * \param var - variable to fill/variable to provide value
	 * \param info - location of operation
	 *
	 * This function is a facade that allows to write single statement that is suitable to all 4 PTC operation modes
	 *
	 * Actual operation is:
	 * | mode | action |
	 * | :- | :- |
	 * | parse binary | ibx::uint32 |
	 * | parse text | icx::uint |
	 * | construct binary | obx::uint |
	 * | construct text | ocx::uint |
	 *
	 * There are \b uint32_t, \b uint16_t and \b uint8_t variants or this method.
	 * \throws mopa::Exception
	 */
	void uint(int bitsize, uint32_t& val, const iox_info* info=NULL);
	void uint_req(int bitsize, uint8_t val, const iox_info* info=NULL);
	void uint_req(int bitsize, uint16_t val, const iox_info* info=NULL);
	/**
	 * \brief Require a specific value for subsequent integer
	 *
	 * \param bitsize - number of bits to read/write
	 * \param var - variable to fill/variable to provide value
	 * \param info - location of operation
	 *
	 * This function is a facade that allows to write single statement that is suitable to all 4 PTC operation modes
	 *
	 * In parsing mode, value is read from input and verified. If it does not match, exception is thrown.
	 * In construction mode, this value is streamed to output.
	 *
	 * Actual operation is:
	 * | mode | action |
	 * | :- | :- |
	 * | parse binary | ibx::uint32 |
	 * | parse text | icx::uint |
	 * | construct binary | obx::uint |
	 * | construct text | ocx::uint |
	 *
	 * There are \b uint32_t, \b uint16_t and \b uint8_t variants or this method.
	 * In addition \b int version is added for convenience. It redirects to specific cases based on \b bitsize.
	 * \throws mopa::Exception
	 */
	void uint_req(int bitsize, uint32_t val, const iox_info* info=NULL);
	void uint_req(int bitsize, int val, const iox_info* info=NULL);
	/**
	 * \brief Begin block declared by variable
	 *
	 * \param bitsize - number of bits that constitute length of the block
	 * \param info - location of operation
	 *
	 * Blocks are used to define length of syntactic element.\n
	 * Consider syntax definition:
	 * | Syntax | Number of bits | Identifier |
	 * | :---- | :----: | :---- |
	 * | subcell_info_loop_length | 8 | uimsbf |
	 * | for (j=0;j<N;j++){ | | |
	 * | cell_id_extension | 8 | uimsbf |
	 * | subcell_latitude | 16 | uimsbf |
	 * | } | | |
	 *
	 * Proper declaration of block:
	 * \code
	 * named_block_begin(8,DVB_INFO("subcell_info_loop_length");
	 * \endcode
	 *
	 * <b> In parsing modes </b>\n
	 * 1) \b bitsize bit long value X is read.\n
	 * 2) Block of size X bytes begins.\n
	 * 3) Block must be byte-aligned.\n
	 * 4) Exactly X bytes must be parsed before \ref named_block_end.
	 *
	 * <b> In construction modes </b>\n
	 * 1) \b bitsize bit long space is reserved. \n
	 * 2) Block of size limit 2^bitsize-1 bytes begins.\n
	 * 3) Block must be byte-aligned.\n
	 * 4) No more then 2^bitsize-1 bytes may follow before \ref named_block_end.\n
	 */
	void named_block_begin(int bitsize,const iox_info* info=NULL);
	/**
	 * \brief End block declared by variable
	 * \param info - location of operation
	 * \return number of bytes in block
	 *
	 * Closes syntactic block that is opened with named_block_begin.\n
	 * Size of block is returned by this method, because in construction cases size is not known earlier.
	 *
	 * <b> In parsing modes </b>\n
	 * 1) Exactly X (see \ref named_block_begin) bytes must had been read.\n
	 * 2) Block size must be byte-aligned.\n
	 * 3) Return X
	 *
	 * <b> In construction modes </b>\n
	 * 1) Block must be no more then 2^bitsize-1 bytes.\n
	 * 2) Block size must be byte-aligned.\n
	 * 3) Size of block is written to space reserved by named_block_begin.\n
	 * 4) Return size of constructed block.\n
	 */
	uint32_t named_block_end(const iox_info* info=NULL);
	/**
	 * \brief Query remaining block size
	 * \return number of available bits
	 *
	 * When used outside any syntactic block, entire message is treated as block.
	 *
	 * Actual operation is:
	 * | mode | action |
	 * | :- | :- |
	 * | parse binary | remaining unparsed bits |
	 * | parse text | remaining unparsed bits, but 0 if next token is '}' |
	 * | construct binary | remaining available bits |
	 * | construct text | remaining available bits |
	 */
	uint32_t block_size_left();
	/**
	 * \brief Checks if current mode is parsing
	 * \return true if in either binary or text parsing mode, false otherwise
	 */
	inline bool is_parsing(){return ctx->is_parsing();}
	/**
	 * \brief Checks if current mode is binary
	 * \return true if in either parsing or constructing binary, false otherwise
	 */
	inline bool is_binary(){return ctx->is_binary();}

	ioCtx* ctx;
private:
	iox();
	iox(ioCtx* ctx);
};

/** \brief Input class, aggregates both binary and text */
class ix
{
public:
	ix(iCtx* x);
	ibx as_ibx();
	icx as_icx();
	uint8_t uint8(int bitsize, const iox_info* info=NULL);
	uint16_t uint16(int bitsize, const iox_info* info=NULL);
	/**
	 * \brief Read integer
	 * \param bitsize - size of integer
	 * \param info - location of operation
	 * \return value read
	 * \note uint32_t, uint16_t and uint8_t versions exist
	 */
	uint32_t uint32(int bitsize, const iox_info* info=NULL);
	/** \brief see \ref iox::named_block_begin */
	void named_block_begin(int bitsize,const iox_info* info=NULL);
	/** \brief see \ref iox::named_block_end */
	uint32_t named_block_end(const iox_info* info=NULL);
	/** \brief see \ref iox::block_size_left */
	uint32_t block_size_left();
private:
	iCtx* ctx;
};
/** \brief Output class, aggregates both binary and text */
class ox
{
public:
	ox(oCtx* x);
	obx as_obx();
	ocx as_ocx();
	void uint(int bitsize, uint8_t val,  const iox_info* info=NULL);
	void uint(int bitsize, uint16_t val, const iox_info* info=NULL);
	/**
	 * \brief Write integer
	 * \param bitsize - size of integer
	 * \param val - value to write
	 * \param info - location of operation
	 * \note uint32_t, uint16_t and uint8_t versions exist
	 */
	void uint(int bitsize, uint32_t val, const iox_info* info=NULL);
	/** \brief see \ref iox::named_block_begin */
	void named_block_begin(int bitsize,const iox_info* info=NULL);
	/** \brief see \ref iox::named_block_end */
	uint32_t named_block_end(const iox_info* info=NULL);
	/** \brief see \ref iox::block_size_left */
	uint32_t block_size_left();
private:
	oCtx* ctx;
};
/** \brief Binary Input class */
class ibx
{
public:
	ibx(ibCtx* x);
	iox as_iox();
	uint8_t  uint8 (int bitsize, const iox_info* info=NULL);
	uint16_t uint16(int bitsize, const iox_info* info=NULL);
	/**
	 * \brief Read integer
	 * \param bitsize - size of integer
	 * \param info - location of operation
	 * \return value read
	 * \note uint32_t, uint16_t and uint8_t versions exist
	 */
	uint32_t uint32(int bitsize, const iox_info* info=NULL);
	/**
	 * \brief Begin block
	 * \param block_length - length of block in bytes
	 * \param info - location of operation
	 *
	 * This method is similar to iox::named_block_begin, but it gets length from parameter.
	 */
	void block_begin(int block_length, const iox_info* info=NULL);
	/**
	 * \brief End block
	 * \param info - location of operation
	 *
	 * Specialization of iox::named_block_end.
	 */
	uint32_t block_end(const iox_info* info=NULL);
	/**
	 * \brief Query remaining bits in block
	 * \return number of bits left unparsed
	 */
	uint32_t block_size_left();

	ibCtx* ctx;
};

/** \brief Binary Output class */
class obx
{
public:
	obx(obCtx* x);
	void uint(int bitsize, uint8_t val,  const iox_info* info=NULL);
	void uint(int bitsize, uint16_t val, const iox_info* info=NULL);
	/**
	 * \brief Write integer
	 * \param bitsize - size of integer
	 * \param val - value of integer
	 * \param info - location of operation
	 *
	 * Writes \b bitsize long integer of value \b val to bitstream.
	 * \exception iox::Exception If there is no space, or value exceeds 2^bitsize.
	 * \note uint32_t, uint16_t and uint8_t versions exist
	 */
	void uint(int bitsize, uint32_t val, const iox_info* info=NULL);
	/**
	 * \brief Begin block
	 * \param block_size_limit - maximum available bytes in block
	 * \param info - location of operation
	 *
	 * This method is similar to iox::named_block_begin, but it gets length from parameter.
	 * \exception iox::Exception when block is not byte-aligned.
	 */
	void block_begin(uint32_t block_size_limit, const iox_info* info=NULL);
	/**
	 * \brief End block
	 * \param info - location of operation
	 * \return number of bytes enclosed in block
	 *
	 * Specialization of iox::named_block_end.
	 * \exception iox::Exception when block size is not byte-aligned.
	 */
	uint32_t block_end(const iox_info* info=NULL);
	/**
	 * \brief Query remaining bits in block
	 * \return number of bits that can fit in current block
	 */
	uint32_t block_size_left();

	obCtx* ctx;
};

/** \brief Text Input class */
class icx
{
public:
	icx(icCtx* x);
	/**
	 * \brief Read integer
	 * \param bitsize - size of integer
	 * \param info - location of operation
	 * \return read value
	 *
	 * Parses input to read integer. Expects form like:
	 * \code
	 * frequency: 3120000
	 * \endcode
	 * Value must fit in \b bitsize bits. Name of variable is encoded in info.name .
	 * Size of actual message is tracked.
	 * \exception iox::Exception If there is no space, or value exceeds 2^bitsize.
	 * \note uint32_t, uint16_t and uint8_t versions exist
	 */
	uint32_t uint(int bitsize,  const iox_info* info=NULL);
	/**
	 * \brief Begin block
	 * \param block_size_limit - maximum available bytes in block
	 * \param info - location of operation
	 *
	 * This method is similar to iox::named_block_begin, but it gets block_size directly.
	 * It expects "{" as next token.
	 * Actual size of block will be determined by finding "}" in text, but no more then block_size_limit bytes.
	 * \exception iox::Exception when block is not byte-aligned or "{" not found.
	 */
	void block_begin(int block_size_limit, const iox_info* info=NULL);
	/**
	 * \brief End block
	 * \param info - location of operation
	 * \return number of bytes enclosed in block
	 *
	 * Specialization of iox::named_block_end.
	 * Expects "} as next token.
	 * \exception iox::Exception when block size is not byte-aligned or "}" not found.
	 */
	uint32_t block_end(const iox_info* info=NULL);
	uint32_t block_size_left();

	icCtx* ctx;
};

/** \brief Text Output class */
class ocx
{
public:
	ocx(ocCtx* x);
	/**
	 * \brief Write integer
	 * \param bitsize - size of integer
	 * \param val - value of integer
	 * \param info - location of operation
	 *
	 * Writes \b bitsize long integer of value \b val to bitstream.
	 * Written integer is represented in textual form and looks like this:
	 * \code
	 * frequency: 3120000
	 * \endcode
	 * Name of variable is encoded in info.name .
	 * Textual representation is not limited in size.
	 * Size of actual message is tracked.
	 * \exception iox::Exception If there is no space, or value exceeds 2^bitsize.
	 * \note uint32_t, uint16_t and uint8_t versions exist
	 */
	void uint(int bitsize, uint32_t val,  const iox_info* info=NULL);
	/**
	 * \brief Begin block
	 * \param block_size_limit - maximum available bytes in block
	 * \param info - location of operation
	 *
	 * This method is similar to iox::named_block_begin, but it gets length directly.
	 * It emits "{" into textual representation.
	 * \exception iox::Exception when block is not byte-aligned.
	 */
	void block_begin(int block_size_limit,const iox_info* info=NULL);
	/**
	 * \brief End block
	 * \param info - location of operation
	 * \return number of bytes enclosed in block
	 *
	 * Specialization of iox::named_block_end.
	 * Emits "} into textual representation.
	 * \exception iox::Exception when block size is not byte-aligned.
	 */
	uint32_t block_end(const iox_info* info=NULL);
	/**
	 * \brief Query remaining bits in block
	 * \return number of bits that can fit in current block
	 */
	uint32_t block_size_left();

	ocCtx* ctx;
};

/**
 * \}
 */
/**
 * \brief Exception thrown on error
 *
 * Only relevant field in this exception is 'message' which contains formatted
 * textual representation of error that has occurred.
 */
class Exception
{
public:
	/**
	 * \param x - context of I/O operation
	 * \param info - location of operation that faulted
	 * \param fmt - format string, as for printf
	 * \param ... - parameters for fmt
	 */
	Exception(
			const ioCtx* x,
			const iox_info* info,
			const char* fmt, ...);
	std::string message;
};

/*!
 * \def DVB_INFO(str)
 * Convenience macro for constructing access information for variable.
 * It is used to provide names for textual I/O and for run-time error reporting.
 */
#define DVB_INFO(str) ({ static const iox_info id={__FILE__,__LINE__,str,FORMAT_NOHINT}; &id; })
/*!
 * \def DVB_INFO_HINT(str,hint)
 * Access information with format hint
 */
#define DVB_INFO_HINT(str,hint) ({ static const iox_info id={__FILE__,__LINE__,str,hint}; &id; })
/*!
 * \def DVB_VAR(var)
 * Macro to define variable and it's access information at the same time
 */
#define DVB_VAR(var) var,DVB_INFO(#var)

}
#endif