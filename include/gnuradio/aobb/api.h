
#ifndef INCLDUED_AOBB_API_H
#define INCLDUED_AOBB_API_H

#include <gnuradio/attributes.h>

#ifdef gnuradio_aobb_EXPORTS
#define AOBB_API __GR_ATTR_EXPORT
#else
#define AOBB_API __GR_ATTR_IMPORT
#endif
#endif