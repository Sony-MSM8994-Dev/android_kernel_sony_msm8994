/* Atomic operations usable in machine independent code */
#ifndef _LINUX_ATOMIC_H
#define _LINUX_ATOMIC_H
#include <asm/atomic.h>

/*
 * Provide __deprecated wrappers for the new interface, avoid flag day changes.
 * We need the ugly external functions to break header recursion hell.
 */
#ifndef smp_mb__before_atomic_inc
static inline void __deprecated smp_mb__before_atomic_inc(void)
{
	extern void __smp_mb__before_atomic(void);
	__smp_mb__before_atomic();
}
#endif

#ifndef smp_mb__after_atomic_inc
static inline void __deprecated smp_mb__after_atomic_inc(void)
{
	extern void __smp_mb__after_atomic(void);
	__smp_mb__after_atomic();
}
#endif

#ifndef smp_mb__before_atomic_dec
static inline void __deprecated smp_mb__before_atomic_dec(void)
{
	extern void __smp_mb__before_atomic(void);
	__smp_mb__before_atomic();
}
#endif

#ifndef smp_mb__after_atomic_dec
static inline void __deprecated smp_mb__after_atomic_dec(void)
{
	extern void __smp_mb__after_atomic(void);
	__smp_mb__after_atomic();
}
#endif

/* atomic_cmpxchg_relaxed */
#ifndef atomic_cmpxchg_relaxed
#define  atomic_cmpxchg_relaxed		atomic_cmpxchg
#define  atomic_cmpxchg_acquire		atomic_cmpxchg
#define  atomic_cmpxchg_release		atomic_cmpxchg

#else /* atomic_cmpxchg_relaxed */

#ifndef atomic_cmpxchg_acquire
#define  atomic_cmpxchg_acquire(...)					\
	__atomic_op_acquire(atomic_cmpxchg, __VA_ARGS__)
#endif

#ifndef atomic_cmpxchg_release
#define  atomic_cmpxchg_release(...)					\
	__atomic_op_release(atomic_cmpxchg, __VA_ARGS__)
#endif

#ifndef atomic_cmpxchg
#define  atomic_cmpxchg(...)						\
	__atomic_op_fence(atomic_cmpxchg, __VA_ARGS__)
#endif
#endif /* atomic_cmpxchg_relaxed */

/* these were introduced together, so just a single check is enough */
#ifndef atomic_try_cmpxchg_acquire
#ifndef atomic_try_cmpxchg
#define __atomic_try_cmpxchg(type, _p, _po, _n)				\
({									\
	typeof(_po) __po = (_po);					\
	typeof(*(_po)) __r, __o = *__po;				\
	__r = atomic_cmpxchg##type((_p), __o, (_n));			\
	if (unlikely(__r != __o))					\
		*__po = __r;						\
	likely(__r == __o);						\
})

#define atomic_try_cmpxchg(_p, _po, _n)		__atomic_try_cmpxchg(, _p, _po, _n)
#define atomic_try_cmpxchg_relaxed(_p, _po, _n)	__atomic_try_cmpxchg(_relaxed, _p, _po, _n)
#define atomic_try_cmpxchg_acquire(_p, _po, _n)	__atomic_try_cmpxchg(_acquire, _p, _po, _n)
#define atomic_try_cmpxchg_release(_p, _po, _n)	__atomic_try_cmpxchg(_release, _p, _po, _n)
#else /* atomic_try_cmpxchg */
#define atomic_try_cmpxchg_relaxed	atomic_try_cmpxchg
#define atomic_try_cmpxchg_acquire	atomic_try_cmpxchg
#define atomic_try_cmpxchg_release	atomic_try_cmpxchg
#endif /* atomic_try_cmpxchg */

#endif /* atomic_try_cmpxchg_acquire */

/**
 * atomic_add_unless - add unless the number is already a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as @v was not already @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
	return __atomic_add_unless(v, a, u) != u;
}

/**
 * atomic_inc_not_zero - increment unless the number is zero
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1, so long as @v is non-zero.
 * Returns non-zero if @v was non-zero, and zero otherwise.
 */
#ifndef atomic_inc_not_zero
#define atomic_inc_not_zero(v)		atomic_add_unless((v), 1, 0)
#endif

/**
 * atomic_inc_not_zero_hint - increment if not null
 * @v: pointer of type atomic_t
 * @hint: probable value of the atomic before the increment
 *
 * This version of atomic_inc_not_zero() gives a hint of probable
 * value of the atomic. This helps processor to not read the memory
 * before doing the atomic read/modify/write cycle, lowering
 * number of bus transactions on some arches.
 *
 * Returns: 0 if increment was not done, 1 otherwise.
 */
#ifndef atomic_inc_not_zero_hint
static inline int atomic_inc_not_zero_hint(atomic_t *v, int hint)
{
	int val, c = hint;

	/* sanity test, should be removed by compiler if hint is a constant */
	if (!hint)
		return atomic_inc_not_zero(v);

	do {
		val = atomic_cmpxchg(v, c, c + 1);
		if (val == c)
			return 1;
		c = val;
	} while (c);

	return 0;
}
#endif

#ifndef atomic_inc_unless_negative
static inline int atomic_inc_unless_negative(atomic_t *p)
{
	int v, v1;
	for (v = 0; v >= 0; v = v1) {
		v1 = atomic_cmpxchg(p, v, v + 1);
		if (likely(v1 == v))
			return 1;
	}
	return 0;
}
#endif

#ifndef atomic_dec_unless_positive
static inline int atomic_dec_unless_positive(atomic_t *p)
{
	int v, v1;
	for (v = 0; v <= 0; v = v1) {
		v1 = atomic_cmpxchg(p, v, v - 1);
		if (likely(v1 == v))
			return 1;
	}
	return 0;
}
#endif

/*
 * atomic_dec_if_positive - decrement by 1 if old value positive
 * @v: pointer of type atomic_t
 *
 * The function returns the old value of *v minus 1, even if
 * the atomic variable, v, was not decremented.
 */
#ifndef atomic_dec_if_positive
static inline int atomic_dec_if_positive(atomic_t *v)
{
	int c, old, dec;
	c = atomic_read(v);
	for (;;) {
		dec = c - 1;
		if (unlikely(dec < 0))
			break;
		old = atomic_cmpxchg((v), c, dec);
		if (likely(old == c))
			break;
		c = old;
	}
	return dec;
}
#endif

#ifndef CONFIG_ARCH_HAS_ATOMIC_OR
static inline void atomic_or(int i, atomic_t *v)
{
	int old;
	int new;

	do {
		old = atomic_read(v);
		new = old | i;
	} while (atomic_cmpxchg(v, old, new) != old);
}
#endif /* #ifndef CONFIG_ARCH_HAS_ATOMIC_OR */

#include <asm-generic/atomic-long.h>
#ifdef CONFIG_GENERIC_ATOMIC64
#include <asm-generic/atomic64.h>
#endif
#endif /* _LINUX_ATOMIC_H */
