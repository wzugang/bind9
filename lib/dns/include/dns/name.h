/*
 * Copyright (C) 1998, 1999  Internet Software Consortium.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef DNS_NAME_H
#define DNS_NAME_H 1

/*****
 ***** Module Info
 *****/

/*
 * DNS Names and Labels
 *
 * Provides facilities for manipulating DNS names and labels, including
 * conversions to and from wire format and text format.
 *
 * Given the large number of names possible in a nameserver, and because
 * names occur in rdata, it was important to come up with a very efficient
 * way of storing name data, but at the same time allow names to be
 * manipulated.  The decision was to store names in uncompressed wire format,
 * and not to make them fully abstracted objects; i.e. certain parts of the
 * server know names are stored that way.  This saves a lot of memory, and
 * makes adding names to messages easy.  Having much of the server know
 * the representation would be perilous, and we certainly don't want each
 * user of names to be manipulating such a low-level structure.  This is
 * where the Names and Labels module comes in.  The module allows name or
 * label handles to be created and attached to uncompressed wire format
 * regions.  All name operations and conversions are done through these
 * handles.
 *
 * MP:
 *	Clients of this module must impose any required synchronization.
 *
 * Reliability:
 *	This module deals with low-level byte streams.  Errors in any of
 *	the functions are likely to crash the server or corrupt memory.
 *
 * Resources:
 *	None.
 *
 * Security:
 *
 *	*** WARNING ***
 *
 *	dns_name_fromwire() deals with raw network data.  An error in
 *	this routine could result in the failure or hijacking of the server.
 *
 * Standards:
 *	RFC 1035
 *	Draft EDNS0 (0)
 *	Draft EDNS1 (0)
 *	Draft Binary Labels (2)
 *	Draft Local Compression (1)
 *
 */

/***
 *** Imports
 ***/

#include <isc/boolean.h>
#include <isc/buffer.h>
#include <isc/lang.h>

#include <dns/types.h>
#include <dns/result.h>

ISC_LANG_BEGINDECLS

/*****
 ***** Labels
 *****
 ***** A 'label' is basically a region.  It contains one DNS wire format
 ***** label of either type 00 (ordinary) or type 01000001 (bitstring).
 *****/

/***
 *** Extended Label Types
 ***/

#define DNS_LABELTYPE_GLOBALCOMP16	0x40
#define DNS_LABELTYPE_BITSTRING		0x41
#define DNS_LABELTYPE_LOCALCOMP		0x42

/***
 *** Properties
 ***/

dns_labeltype_t dns_label_type(dns_label_t *label);
/*
 * Get the type of 'label'.
 *
 * Requires:
 *	'label' is a valid label (i.e. not NULL, points to a
 *	struct dns_label)
 *	'label' is a type 00 or type 01000001 label (i.e. not compressed).
 *
 * Returns:
 *	dns_labeltype_ordinary		type 00 label
 *	dns_labeltype_bitstring		type 01000001 label
 */

/***
 *** Bitstring Labels
 ***/

unsigned int dns_label_countbits(dns_label_t *label);
/*
 * The number of bits in a bitstring label.
 *
 * Requires:
 *	'label' is a valid label
 *
 *	dns_label_type(label) == dns_labeltype_bitstring
 *
 * Ensures:
 *	Result is <= 256.
 *
 * Returns:
 *	The number of bits in the bitstring label.
 */

dns_bitlabel_t dns_label_getbit(dns_label_t *label, unsigned int n);
/*
 * The 'n'th most significant bit of 'label'.
 *
 * Notes:
 *	Numbering starts at 0.
 *
 * Require:
 *	n < dns_label_countbits(label)
 *
 * Returns:
 *	dns_bitlabel_0		The bit was 0.
 *	dns_bitlabel_1		The bit was 1.
 */

/***
 *** Note
 ***
 *** Some provision still needs to be made for splitting bitstring labels.
 ***/



/*****
 ***** Names
 *****
 ***** A 'name' is a handle to a binary region.  It contains a sequence of one
 ***** or more DNS wire format labels of either type 00 (ordinary) or type
 ***** 01000001 (bitstring).  Note that all names are not required to end
 ***** with the root label, as they are in the actual DNS wire protocol.
 *****/

/***
 *** Compression pointer chaining limit
 ***/

#define DNS_POINTER_MAXHOPS		16

/***
 *** Types
 ***/

/*
 * Clients are strongly discouraged from using this type directly,  with
 * the exception of the 'link' and 'list' fields which may be used directly
 * for whatever purpose the client desires.
 */
struct dns_name {
	unsigned int			magic;
	unsigned char *			ndata;
	unsigned int			length;
	unsigned int			labels;
	unsigned int			attributes;
	unsigned char *			offsets;
	isc_buffer_t *			buffer;
	ISC_LINK(dns_name_t)		link;
	ISC_LIST(dns_rdataset_t)	list;
};

#define DNS_NAMEATTR_ABSOLUTE		0x0001
#define DNS_NAMEATTR_READONLY		0x0002
#define DNS_NAMEATTR_DYNAMIC		0x0004

extern dns_name_t *dns_rootname;

/***
 *** Initialization
 ***/

void dns_name_init(dns_name_t *name, unsigned char *offsets);
/*
 * Make 'name' empty.
 *
 * Notes:
 *	'offsets' is never required to be non-NULL, but specifying a
 *	dns_offsets_t for 'offsets' will improve the performance of most
 *	name operations if the name is used more than once.
 *
 * Requires:
 *	'name' is not NULL and points to a struct dns_name.
 *
 *	offsets == NULL or offsets is a dns_offsets_t.
 *
 * Ensures:
 *	'name' is a valid name.
 *	dns_name_countlabels(name) == 0
 */

void dns_name_invalidate(dns_name_t *name);
/*
 * Make 'name' invalid.
 *
 * Requires:
 *	'name' is a valid name.
 *
 * Ensures:
 *	If assertion checking is enabled, future attempts to use 'name'
 *	without initializing it will cause an assertion failure.
 *
 *	If the name had a dedicated buffer, that association is ended.
 */


/***
 *** Dedicated Buffers
 ***/

void
dns_name_setbuffer(dns_name_t *name, isc_buffer_t *buffer);
/*
 * Dedicate a binary buffer for use with 'name'.
 *
 * Notes:
 *	Specification of a target buffer in dns_name_fromwire(),
 *	dns_name_fromtext(), and dns_name_concatentate() is optional if
 *	'name' has a dedicated buffer.
 *
 *	The caller must not write to buffer until the name has been
 *	invalidated or is otherwise known not to be in use.
 *
 *	If buffer is NULL and the name previously had a dedicated buffer,
 *	than that buffer is no longer dedicated to use with this name.
 *	The caller is responsible for ensuring that the storage used by
 *	the name remains valid.
 *
 * Requires:
 *	'name' is a valid name.
 *
 *	'buffer' is a valid binary buffer and 'name' doesn't have a
 *	dedicated buffer already, or 'buffer' is NULL.
 */

isc_boolean_t
dns_name_hasbuffer(dns_name_t *name);
/*
 * Does 'name' have a dedicated buffer?
 *
 * Requires:
 *	'name' is a valid name.
 *
 * Returns:
 *	ISC_TRUE	'name' has a dedicated buffer.
 *	ISC_FALSE	'name' does not have a dedicated buffer.
 */


/***
 *** Properties
 ***/

isc_boolean_t dns_name_isabsolute(dns_name_t *name);
/*
 * Does 'name' end in the root label?
 *
 * Requires:
 *	'name' is a valid name
 *
 *	dns_name_countlabels(name) > 0
 *
 * Returns:
 *	TRUE		The last label in 'name' is the root label.
 *	FALSE		The last label in 'name' is not the root label.
 */

isc_boolean_t dns_name_iswildcard(dns_name_t *name);
/*
 * Is 'name' a wildcard name?
 *
 * Requires:
 *	'name' is a valid name
 *
 *	dns_name_countlabels(name) > 0
 *
 * Returns:
 *	TRUE		The least significant label of 'name' is '*'.
 *	FALSE		The least significant label of 'name' is not '*'.
 */

unsigned int dns_name_hash(dns_name_t *name, isc_boolean_t case_sensitive);
/*
 * Provide a hash value for 'name'.
 *
 * Note: if 'case_sensitive' is ISC_FALSE, then names which differ only in
 * case will have the same hash value.
 *
 * Requires:
 *	'name' is a valid name
 *
 * Returns:
 *	A hash value
 */

/***
 *** Comparisons
 ***/

dns_namereln_t
dns_name_fullcompare(dns_name_t *name1, dns_name_t *name2,
		     int *orderp,
		     unsigned int *nlabelsp, unsigned int *nbitsp);
/*
 * Determine the relative ordering under the DNSSEC order relation of
 * 'name1' and 'name2', and also determine the hierarchical
 * relationship of the names.
 *
 * Note: It makes no sense for one of the names to be relative and the
 * other absolute.  If both names are relative, then to be meaningfully
 * compared the caller must ensure that they are both relative to the
 * same domain.
 *
 * Requires:
 *	'name1' is a valid name
 *
 *	dns_name_countlabels(name1) > 0
 *
 *	'name2' is a valid name
 *
 *	dns_name_countlabels(name2) > 0
 *
 *	orderp, nlabelsp, and nbitsp are valid pointers.
 *
 *	Either name1 is absolute and name2 is absolute, or neither is.
 *
 * Ensures:
 *
 *	*orderp is -1 if name1 < name2, 0 if name1 = name2, 1 if
 *	name1 > name2.
 *
 *	*nlabelsp is the number of common significant labels.
 *
 *	If *nbitsp is non-zero, then the least-signficant of the
 *	common significant labels is a bitstring label, and the
 *	two names have *nbitsp significant bits in common.
 *
 * Returns:
 *	dns_namereln_none		There's no hierarchical relationship
 *					between name1 and name2.
 *	dns_namereln_contains		name1 properly contains name2; i.e.
 *					name2 is a proper subdomain of name1.
 *	dns_namereln_subdomain		name1 is a proper subdomain of name2.
 *	dns_namereln_equal		name1 and name2 are equal.
 *	dns_namereln_commonancestor	name1 and name2 share a common
 *					ancestor.
 */

int
dns_name_compare(dns_name_t *name1, dns_name_t *name2);
/*
 * Determine the relative ordering under the DNSSEC order relation of
 * 'name1' and 'name2'.
 *
 * Note: It makes no sense for one of the names to be relative and the
 * other absolute.  If both names are relative, then to be meaningfully
 * compared the caller must ensure that they are both relative to the
 * same domain.
 *
 * Requires:
 *	'name1' is a valid name
 *
 *	dns_name_countlabels(name1) > 0
 *
 *	'name2' is a valid name
 *
 *	dns_name_countlabels(name2) > 0
 *
 *	Either name1 is absolute and name2 is absolute, or neither is.
 *
 * Returns:
 *	-1		'name1' is less than 'name2'
 *	0		'name1' is equal to 'name2'
 *	1		'name1' is greater than 'name2'
 */

isc_boolean_t
dns_name_equal(dns_name_t *name1, dns_name_t *name2);
/*
 * Are 'name1' and 'name2' equal?
 *
 * Notes:
 *	Because it only needs to test for equality, dns_name_equal() can be
 *	significantly faster than dns_name_fullcompare() or dns_name_compare().
 *
 *	Offsets tables are not used in the comparision.
 *
 * 	It makes no sense for one of the names to be relative and the
 *	other absolute.  If both names are relative, then to be meaningfully
 * 	compared the caller must ensure that they are both relative to the
 * 	same domain.
 *
 * Requires:
 *	'name1' is a valid name
 *
 *	'name2' is a valid name
 *
 *	Either name1 is absolute and name2 is absolute, or neither is.
 *
 * Returns:
 *	ISC_TRUE	'name1' and 'name2' are equal
 *	ISC_FALSE	'name1' and 'name2' are not equal
 */

int
dns_name_rdatacompare(dns_name_t *name1, dns_name_t *name2);
/*
 * Compare two names as if they are part of rdata in DNSSEC cononical
 * form.
 *
 * Requires:
 *	'name1' is a valid absolute name
 *
 *	dns_name_countlabels(name1) > 0
 *
 *	'name2' is a valid absolute name
 *
 *	dns_name_countlabels(name2) > 0
 *
 * Returns:
 *	-1		'name1' is less than 'name2'
 *	0		'name1' is equal to 'name2'
 *	1		'name1' is greater than 'name2'
 */

isc_boolean_t
dns_name_issubdomain(dns_name_t *name1, dns_name_t *name2);
/*
 * Is 'name1' a subdomain of 'name2'?
 *
 * Notes:
 *	name1 is a subdomain of name2 if name1 is contained in name2, or
 *	name1 equals name2.
 *
 *	It makes no sense for one of the names to be relative and the
 *	other absolute.  If both names are relative, then to be meaningfully
 *	compared the caller must ensure that they are both relative to the
 *	same domain.
 *
 * Requires:
 *	'name1' is a valid name
 *
 *	dns_name_countlabels(name1) > 0
 *
 *	'name2' is a valid name
 *
 *	dns_name_countlabels(name2) > 0
 *
 *	Either name1 is absolute and name2 is absolute, or neither is.
 *
 * Returns:
 *	TRUE		'name1' is a subdomain of 'name2'
 *	FALSE		'name1' is not a subdomain of 'name2'
 */


/***
 *** Labels
 ***/
	
unsigned int dns_name_countlabels(dns_name_t *name);
/*
 * How many labels does 'name' have?
 *
 * Notes:
 *	In this case, as in other places, a 'label' is an ordinary label
 *	or a bitstring label.  The term is not meant to refer to individual
 *	bit labels.
 *
 * Requires:
 *	'name' is a valid name
 *
 * Ensures:
 *	The result is <= 128.
 *
 * Returns:
 *	The number of labels in 'name'.
 */

void dns_name_getlabel(dns_name_t *name, unsigned int n, dns_label_t *label);
/*
 * Make 'label' refer to the 'n'th least significant label of 'name'.
 *
 * Notes:
 *	Numbering starts at 0.
 *
 *	Given "rc.vix.com.", the label 0 is "rc", and label 3 is the
 *	root label.
 *
 *	'label' refers to the same memory as 'name', so 'name' must not
 *	be changed while 'label' is still in use.
 *
 * Requires:
 *	n < dns_label_countlabels(name)
 */

void dns_name_getlabelsequence(dns_name_t *source,
			       unsigned int first,
			       unsigned int n,
			       dns_name_t *target);
/*
 * Make 'target' refer to the 'n' labels including and following 'first'
 * in 'source'.
 *
 * Notes:
 *	Numbering starts at 0.
 *
 *	'target' refers to the same memory as 'source', so 'source'
 *	must not be changed while 'target' is still in use.
 *
 * Requires:
 *	'source' and 'target' are valid names.
 *
 *	first < dns_label_countlabels(name)
 *
 *	first + n <= dns_label_countlabels(name)
 */


void
dns_name_clone(dns_name_t *source, dns_name_t *target);
/*
 * Make 'target' refer to the same name as 'source'.
 *
 * Notes:
 *
 *	'target' refers to the same memory as 'source', so 'source'
 *	must not be changed while 'target' is still in use.
 *
 *	This call is functionally equivalent to:
 *
 *		dns_name_getlabelsequence(source, 0, 
 *					  dns_label_countlabels(source),
 *					  target);
 *
 *	but is more efficient.  Also, dns_name_clone() works even if 'source'
 *	is empty.
 *
 * Requires:
 *
 *	'source' is a valid name.
 *
 *	'target' is a valid name that is not read-only.
 */

/***
 *** Conversions
 ***/

void dns_name_fromregion(dns_name_t *name, isc_region_t *r);
/*
 * Make 'name' refer to region 'r'.
 *
 * Note:
 *	If the conversion encounters a root label before the end of the
 *	region the conversion stops and the length is set to the length
 *	so far converted.
 *
 * Requires:
 *	The data in 'r' is a sequence of one or more type 00 or type 01000001
 *	labels.
 *	The length of 'r' is <= 255.
 */

void dns_name_toregion(dns_name_t *name, isc_region_t *r);
/*
 * Make 'r' refer to 'name'.
 *
 * Requires:
 *
 *	'name' is a valid name.
 *
 *	'r' is a valid region.
 */

dns_result_t dns_name_fromwire(dns_name_t *name,
			       isc_buffer_t *source,
			       dns_decompress_t *dctx,
			       isc_boolean_t downcase,
			       isc_buffer_t *target);
/*
 * Copy the possibly-compressed name at source (active region) into target,
 * decompressing it.
 *
 * Notes:
 *	Decompression policy is controlled by 'dctx'.
 *
 *	If 'downcase' is true, any uppercase letters in 'source' will be
 *	downcased when they are copied into 'target'.
 *
 * Security:
 *
 *	*** WARNING ***
 *
 *	This routine will often be used when 'source' contains raw network
 *	data.  A programming error in this routine could result in a denial
 *	of service, or in the hijacking of the server.
 *
 * Requires:
 *
 *	'name' is a valid name.
 *
 *	'source' is a valid buffer of type ISC_BUFFERTYPE_BINARY, and the
 *	first byte of the active region should be the first byte of a DNS
 *	wire format domain name.
 *
 *	'target' is a valid buffer of type ISC_BUFFERTYPE_BINARY or 
 *	'target' is NULL and 'name' has a dedicated buffer.
 *
 *	'dctx' is a valid decompression context.
 *
 * Ensures:
 *
 *	If result is success:
 *	 	If 'target' is not NULL, 'name' is attached to it.
 *
 *		Uppercase letters are downcased in the copy iff. 'downcase' is
 *		true.
 *
 *		Any bitstring labels in source are canonicalized.
 *		(i.e. maximally packed and any padding bits zeroed.)
 *
 *		The current location in source is advanced, and the used space
 *		in target is updated.
 *
 * Result:
 *	Success
 *	Bad Form: Label Length
 *	Bad Form: Unknown Label Type
 *	Bad Form: Name Length
 *	Bad Form: Compression type not allowed
 *	Bad Form: Bad compression pointer
 *	Bad Form: Input too short
 *	Resource Limit: Too many compression pointers
 *	Resource Limit: Not enough space in buffer
 */

dns_result_t dns_name_towire(dns_name_t *name,
			     dns_compress_t *cctx,
			     isc_buffer_t *target);
/*
 * Convert 'name' into wire format, compressing it as specified by the
 * compression context 'cctx', and storing the result in 'target'.
 *	
 * Notes:
 *	If the compression context allows global compression, then the
 *	global compression table may be updated.
 *
 * Requires:
 *	'name' is a valid name
 *
 *	dns_name_countlabels(name) > 0
 *
 *	dns_name_isabsolute(name) == TRUE
 *
 *	target is a valid buffer of type ISC_BUFFERTYPE_BINARY.
 *
 *	Any offsets specified in a global compression table are valid
 *	for buffer.
 *
 * Ensures:
 *
 *	If the result is success:
 *
 *		Any bitstring labels are in canonical form.
 *
 *		The used space in target is updated.
 *
 * Returns:
 *	Success
 *	Resource Limit: Not enough space in buffer
 */

dns_result_t dns_name_fromtext(dns_name_t *name,
			       isc_buffer_t *source,
			       dns_name_t *origin,
			       isc_boolean_t downcase,
			       isc_buffer_t *target);
/*
 * Convert the textual representation of a DNS name at source
 * into uncompressed wire form stored in target.
 *
 * Notes:
 *	Relative domain names will have 'origin' appended to them
 *	unless 'origin' is NULL, in which case relative domain names
 *	will remain relative.
 *
 *	If 'downcase' is true, any uppercase letters in 'source' will be
 *	downcased when they are copied into 'target'.
 *
 * Requires:
 *
 *	'name' is a valid name.
 *
 *	'source' is a valid buffer of type ISC_BUFFERTYPE_TEXT.
 *
 *	'target' is a valid buffer of type ISC_BUFFERTYPE_BINARY or 
 *	'target' is NULL and 'name' has a dedicated buffer.
 *
 * Ensures:
 *
 *	If result is success:
 *	 	If 'target' is not NULL, 'name' is attached to it.
 *
 *		Any bitstring labels in source are canonicalized.
 *
 *		Uppercase letters are downcased in the copy iff. 'downcase' is
 *		true.
 *
 *		The current location in source is advanced, and the used space
 *		in target is updated.
 *
 * Result:
 *	DNS_R_SUCCESS
 *	DNS_R_EMPTYLABEL
 *	DNS_R_LABELTOOLONG
 *	DNS_R_BADESCAPE
 *	DNS_R_BADBITSTRING
 *	DNS_R_BITSTRINGTOOLONG
 *	DNS_R_BADDOTTEDQUAD
 *	DNS_R_NOSPACE
 *	DNS_R_UNEXPECTEDEND
 */

dns_result_t dns_name_totext(dns_name_t *name,
			     isc_boolean_t omit_final_dot,
			     isc_buffer_t *target);
/*
 * Convert 'name' into text format, storing the result in 'target'.
 *	
 * Notes:
 *	If 'omit_final_dot' is true, then the final '.' in an absolute
 *	name will not be emitted.
 *
 * Requires:
 *
 *	'name' is a valid name
 *
 *	'target' is a valid buffer of type ISC_BUFFERTYPE_TEXT
 *
 *	dns_name_countlabels(name) > 0
 *
 *	if dns_name_isabsolute == FALSE, then omit_final_dot == FALSE
 *
 * Ensures:
 *
 *	If the result is success:
 *
 *		Any bitstring labels are in canonical form.
 *
 *		The used space in target is updated.
 *
 * Returns:
 *	DNS_R_SUCCESS
 *	DNS_R_NOSPACE
 */

dns_result_t dns_name_concatenate(dns_name_t *prefix, dns_name_t *suffix,
				  dns_name_t *name, isc_buffer_t *target);
/*
 *	Concatenate 'prefix' and 'suffix'.
 *
 * Requires:
 *
 *	'prefix' is a valid name or NULL.
 *
 *	'suffix' is a valid name or NULL.
 *
 *	'name' is a valid name or NULL.
 *
 *	'target' is a valid buffer of type ISC_BUFFERTYPE_BINARY, or 
 *	'target' is NULL and 'name' has a dedicated buffer.
 *
 *	If 'prefix' is absolute, 'suffix' must be NULL or the empty name.
 *
 * Ensures:
 *
 *	On success,
 *	 	If 'target' is not NULL and 'name' is not NULL, then 'name'
 *		is attached to it.
 *
 *		Any bitstring labels are in canonical form.
 *
 *		The used space in target is updated.
 *
 * Returns:
 *	DNS_R_SUCCESS
 *	DNS_R_NOSPACE
 */

dns_result_t
dns_name_split(dns_name_t *name,
	       unsigned int suffixlabels, unsigned int nbits,
	       dns_name_t *prefix, dns_name_t *suffix);
/*
 *
 * Split 'name' into two pieces on a label or bitlabel boundary.
 *
 * Notes:
 *	Copying name data is avoided as much as possible, so 'prefix'
 *	and 'suffix' will usually end up pointing at the data for 'name',
 *	except when 'nbits' > 0.  The name data is copied to the
 *	the dedicated buffers when splitting on bitlabel boundaries
 *	because of the bit fiddling that must be done.
 *
 *	It is legitimate to pass a 'prefix' or 'suffix' that has
 *	its name data stored someplace other than the dedicate buffer.
 *	This is useful to avoid name copying in the calling function.
 *
 *	It is also legitimate to pass a 'prefix' or 'suffix' that is
 *	the same dns_name_t as 'name', but note well the requirement
 *	below if splitting on a bitlabel boundary.
 *
 * Requires:
 *	'name' is a valid name.
 *
 * 	'suffixlabels' cannot exceed the number of labels in 'name'.
 *
 *	'nbits' can be greater than zero only when the least significant
 *	label of 'suffixlabels' is a bitstring label.
 *
 *	'nbits' cannot exceed the number of bits in the bitstring label.
 *
 *	'prefix' is a valid name or NULL, and cannot be read-only.
 *
 *	'suffix' is a valid name or NULL, and cannot be read-only.
 *
 *	If non-NULL, 'prefix' and 'suffix' must have dedicated buffers.
 *
 *	'prefix' and 'suffix' cannot point to the same buffer.
 *
 *	If 'nbits' > 0 and 'prefix' and 'suffix' are both non-NULL,
 *	the buffer for 'prefix' cannot be storing the labels for 'name'.
 *
 * Ensures:
 *
 *	On success:
 *		If 'prefix' is not NULL it will contain the least significcant
 *		labels and bits.  dns_name_countlabels(name) - suffixlabels
 *		will be equal to dns_name_countlabels(prefix).
 *
 *		If 'suffix' is not NULL it will contain the most significant
 *		labels and bits.  dns_name_countlabels(suffix) will be
 *		equal to suffixlabels.
 *
 *	On failure:
 *		Either 'prefix' or 'suffix' is invalidated (depending
 *		on which one the problem was encountered with).
 *
 * Returns:
 *	DNS_R_SUCCESS	No worries.
 *	DNS_R_NOSPACE	An attempt was made to split a name on a bitlabel
 *			boundary but either 'prefix' or 'suffix' did not
 *			have enough room to receive the split name.
 */

dns_result_t
dns_name_dup(dns_name_t *source, isc_mem_t *mctx, dns_name_t *target);
/*
 * Make 'target' a dynamically allocated copy of 'source'.
 *
 * Requires:
 *
 *	'source' is a valid non-empty name.
 *	
 *	'target' is a valid name that is not read-only.
 *	
 *	'mctx' is a valid memory context.
 */

void
dns_name_free(dns_name_t *name, isc_mem_t *mctx);
/*
 * Free 'name'.
 *
 * Requires:
 *
 *	'name' is a valid name created previously in 'mctx' by dns_name_dup().
 *
 *	'mctx' is a valid memory context.
 *
 * Ensures:
 *
 *	All dynamic resources used by 'name' are freed and the name is
 *	invalidated.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_NAME_H */
