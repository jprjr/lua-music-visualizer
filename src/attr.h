#ifndef JPR_ATTR_H
#define JPR_ATTR_H

#ifdef __GNUC__
#define attr_const __attribute__((__const__))
#define attr_pure __attribute__((__pure__))
#else
#define attr_const
#define attr_pure
#endif

#endif
