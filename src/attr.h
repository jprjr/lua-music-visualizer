#ifndef JPR_ATTR_H
#define JPR_ATTR_H

#ifdef __GNUC__
#define attr_const __attribute__((__const__))
#define attr_pure __attribute__((__pure__))
#define attr_noreturn __attribute__((__noreturn__))
#define attr_nonnull1 __attribute__((__nonnull__(1)))
#define attr_nonnull12 __attribute__((__nonnull__(1,2)))
#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)
#define RESTRICT restrict
#elif defined(_MSC_VER)
#if _MSC_VER >= 1310
#define attr_noreturn __declspec(noreturn)
#endif
#if _MSC_VER >= 1400
#define RESTRICT __restrict
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

#ifndef attr_nonnull1
#define attr_nonnull1
#endif

#ifndef attr_nonnull12
#define attr_nonnull12
#endif

#ifndef LIKELY
#define LIKELY(x) (!!(x))
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) (!!(x))
#endif

#ifndef RESTRICT
#define RESTRICT
#endif

#endif
