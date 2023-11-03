#include "stdinc.h"

#ifdef _WIN32
// ensure windows.h is included before including ssl.h
#include "w.h"
#endif

#include "ssl.h"

#include "Util.h"

namespace dcpp {

namespace ssl {

ByteVector X509_digest(::X509* x509, const ::EVP_MD* md) {
	unsigned int n;
	unsigned char buf[EVP_MAX_MD_SIZE];

	if (!X509_digest(x509, md, buf, &n)) {
		return ByteVector(); // Throw instead?
	}

	return ByteVector(buf, buf+n);
}

}

}
