#include "linebuf.hpp"
#include "throw.hpp"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>

namespace nark {

LineBuf::LineBuf()
   	: capacity(0), n(0), p(NULL)
{}

LineBuf::~LineBuf() {
   	if (p)
	   	free(p);
}

ptrdiff_t LineBuf::getline(FILE* f) {
	assert(NULL != f);
#if defined(__USE_GNU) || defined(__CYGWIN__) || defined(__CYGWIN32__)
       	// has ::getline
	return n = ::getline(&p, &capacity, f);
#else
//	#error only _GNU_SOURCE is supported
	if (NULL == p) {
		capacity = BUFSIZ;
		p = (char*)malloc(BUFSIZ);
		if (NULL == p)
		   	THROW_STD(runtime_error, "malloc(BUFSIZ=%d) failed", BUFSIZ);
	}
	n = 0;
	p[0] = '\0';
	for (;;) {
		assert(n < capacity);
		char*  ret = ::fgets(p + n, capacity - n, f);
		size_t len = ::strlen(p + n);
		if (0 == len && (feof(f) || ferror(f)))
			return -1;
		n += len;
		if (ret) {
			if (capacity-1 == n && p[n-1] != '\n') {
				size_t newcap = capacity * 2;
				ret = (char*)realloc(p, newcap);
				if (NULL == ret)
					THROW_STD(runtime_error, "realloc(newcap=%zd)", newcap);
				p = ret;
				capacity = newcap;
			}
			else {
				return ptrdiff_t(n);
			}
		}
		else if (feof(f))
			return ptrdiff_t(n);
		else
			return -1;
	}
#endif
}

size_t LineBuf::trim() {
	assert(NULL != p);
	size_t n0 = n;
	while (n > 0 && isspace(p[n-1])) p[--n] = 0;
	return n0 - n;
}

size_t LineBuf::chomp() {
	assert(NULL != p);
	size_t n0 = n;
	while (n > 0 && strchr("\r\n", p[n-1])) p[--n] = 0;
	return n0 - n;
}

bool LineBuf::read_binary_tuple(int32_t* offsets, size_t arity, FILE* f) {
	assert(NULL != offsets);
	offsets[0] = 0;
	size_t n_read = fread(offsets+1, 1, sizeof(int32_t) * arity, f);
	if (n_read != sizeof(int32_t) * arity) {
		return false;
	}
	for (size_t i = 0; i < arity; ++i) {
		assert(offsets[i+1] >= 0);
		offsets[i+1] += offsets[i];
	}
	size_t len = offsets[arity];
	if (this->capacity < len) {
		char* q = (char*)realloc(this->p, len);
		if (NULL == q) {
			THROW_STD(invalid_argument
				, "Out of memory when reading record[size=%zd(0x%zX)]"
				, len, len
				);
		}
		this->p = q;
		this->capacity = len;
	}
	n_read = fread(this->p, 1, len, f);
	if (n_read != len) {
		THROW_STD(invalid_argument
			, "fread record data failed: request=%zd, readed=%zd\n"
			, len, n_read
			);
	}
	this->n = len;
	return true; // len can be 0
}

} // namespace nark

