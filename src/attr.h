#ifndef JPR_ATTR_H
#define JPR_ATTR_H

#ifdef __GNUC__
#define attr_const __attribute__((__const__))
#define attr_pure __attribute__((__pure__))
#define attr_noreturn __attribute__((__noreturn__))
#elif defined(_MSC_VER)
#if _MSC_VER >= 1310
#define attr_noreturn __declspec(noreturn)
#endif
#endif

#ifndef attr_const
#define attr_const
#endif

#ifndef attr_pure
#define attr_pure
#endif

#ifndef attr_noreturn
#define attr_noreturn
#endif


#endif
