#ifndef __LINUX_SEQLOCK_H
#define __LINUX_SEQLOCK_H
/*
 * Reader/writer consistent mechanism without starving writers. This type of
 * lock for data where the reader wants a consistent set of information
 * and is willing to retry if the information changes.  Readers never
 * block but they may have to retry if a writer is in
 * progress. Writers do not wait for readers. 
 *
 * This is not as cache friendly as brlock. Also, this will not work
 * for data that contains pointers, because any writer could
 * invalidate a pointer that a reader was following.
 *
 * Expected reader usage:
 * 	do {
 *	    seq = read_seqbegin(&foo);
 * 	...
 *      } while (read_seqretry(&foo, seq));
 *
 *
 * On non-SMP the spin locks disappear but the writer still needs
 * to increment the sequence variables because an interrupt routine could
 * change the state of the data.
 *
 * Based on x86_64 vsyscall gettimeofday 
 * by Keith Owens and Andrea Arcangeli
 */

#include <linux/spinlock.h>
#include <linux/preempt.h>
#include <linux/lockdep.h>
#include <asm/processor.h>

/*
 * Version using sequence counter only.
 * This can be used when code has its own mutex protecting the
 * updating starting before the write_seqcountbeqin() and ending
 * after the write_seqcount_end().
 */
typedef struct seqcount {
	unsigned sequence;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map dep_map;
#endif
} seqcount_t;

static inline void __seqcount_init(seqcount_t *s, const char *name,
					  struct lock_class_key *key)
{
	/*
	 * Make sure we are not reinitializing a held lock:
	 */
	lockdep_init_map(&s->dep_map, name, key, 0);
	s->sequence = 0;
}

#ifdef CONFIG_DEBUG_LOCK_ALLOC
# define SEQCOUNT_DEP_MAP_INIT(lockname) \
		.dep_map = { .name = #lockname } \

# define seqcount_init(s)				\
	do {						\
		static struct lock_class_key __key;	\
		__seqcount_init((s), #s, &__key);	\
	} while (0)

static inline void seqcount_lockdep_reader_access(const seqcount_t *s)
{
	seqcount_t *l = (seqcount_t *)s;
	unsigned long flags;

	local_irq_save(flags);
	seqcount_acquire_read(&l->dep_map, 0, 0, _RET_IP_);
	seqcount_release(&l->dep_map, 1, _RET_IP_);
	local_irq_restore(flags);
}

#else
# define SEQCOUNT_DEP_MAP_INIT(lockname)
# define seqcount_init(s) __seqcount_init(s, NULL, NULL)
# define seqcount_lockdep_reader_access(x)
#endif

#define SEQCNT_ZERO(lockname) { .sequence = 0, SEQCOUNT_DEP_MAP_INIT(lockname)}


/**
 * __read_seqcount_begin - begin a seq-read critical section (without barrier)
 * @s: pointer to seqcount_t
 * Returns: count to be passed to read_seqcount_retry
 *
 * __read_seqcount_begin is like read_seqcount_begin, but has no smp_rmb()
 * barrier. Callers should ensure that smp_rmb() or equivalent ordering is
 * provided before actually loading any of the variables that are to be
 * protected in this critical section.
 *
 * Use carefully, only in critical code, and comment how the barrier is
 * provided.
 */
static inline unsigned __read_seqcount_begin(const seqcount_t *s)
{
	unsigned ret;

repeat:
	ret = ACCESS_ONCE(s->sequence);
	if (unlikely(ret & 1)) {
		cpu_relax();
		goto repeat;
	}
	return ret;
}

/**
 * read_seqcount_begin_no_lockdep - start seq-read critical section w/o lockdep
 * @s: pointer to seqcount_t
 * Returns: count to be passed to read_seqcount_retry
 *
 * read_seqcount_begin_no_lockdep opens a read critical section of the given
 * seqcount, but without any lockdep checking. Validity of the critical
 * section is tested by checking read_seqcount_retry function.
 */
static inline unsigned read_seqcount_begin_no_lockdep(const seqcount_t *s)
{
	unsigned ret = __read_seqcount_begin(s);
	smp_rmb();
	return ret;
}

/**
 * read_seqcount_begin - begin a seq-read critical section
 * @s: pointer to seqcount_t
 * Returns: count to be passed to read_seqcount_retry
 *
 * read_seqcount_begin opens a read critical section of the given seqcount.
 * Validity of the critical section is tested by checking read_seqcount_retry
 * function.
 */
static inline unsigned read_seqcount_begin(const seqcount_t *s)
{
	seqcount_lockdep_reader_access(s);
	return read_seqcount_begin_no_lockdep(s);
}

/**
 * raw_seqcount_begin - begin a seq-read critical section
 * @s: pointer to seqcount_t
 * Returns: count to be passed to read_seqcount_retry
 *
 * raw_seqcount_begin opens a read critical section of the given seqcount.
 * Validity of the critical section is tested by checking read_seqcount_retry
 * function.
 *
 * Unlike read_seqcount_begin(), this function will not wait for the count
 * to stabilize. If a writer is active when we begin, we will fail the
 * read_seqcount_retry() instead of stabilizing at the beginning of the
 * critical section.
 */
static inline unsigned raw_seqcount_begin(const seqcount_t *s)
{
	unsigned ret = ACCESS_ONCE(s->sequence);

	seqcount_lockdep_reader_access(s);
	smp_rmb();
	return ret & ~1;
}

/**
 * __read_seqcount_retry - end a seq-read critical section (without barrier)
 * @s: pointer to seqcount_t
 * @start: count, from read_seqcount_begin
 * Returns: 1 if retry is required, else 0
 *
 * __read_seqcount_retry is like read_seqcount_retry, but has no smp_rmb()
 * barrier. Callers should ensure that smp_rmb() or equivalent ordering is
 * provided before actually loading any of the variables that are to be
 * protected in this critical section.
 *
 * Use carefully, only in critical code, and comment how the barrier is
 * provided.
 */
static inline int __read_seqcount_retry(const seqcount_t *s, unsigned start)
{
	return unlikely(s->sequence != start);
}

/**
 * read_seqcount_retry - end a seq-read critical section
 * @s: pointer to seqcount_t
 * @start: count, from read_seqcount_begin
 * Returns: 1 if retry is required, else 0
 *
 * read_seqcount_retry closes a read critical section of the given seqcount.
 * If the critical section was invalid, it must be ignored (and typically
 * retried).
 */
static inline int read_seqcount_retry(const seqcount_t *s, unsigned start)
{
	smp_rmb();
	return __read_seqcount_retry(s, start);
}

/*
 * raw_write_seqcount_latch - redirect readers to even/odd copy
 * @s: pointer to seqcount_t
 */
static inline void raw_write_seqcount_latch(seqcount_t *s)
{
       smp_wmb();      /* prior stores before incrementing "sequence" */
       s->sequence++;
       smp_wmb();      /* increment "sequence" before following stores */
}

/*
 * Sequence counter only version assumes that callers are using their
 * own mutexing.
 */
static inline void write_seqcount_begin_nested(seqcount_t *s, int subclass)
{
	s->sequence++;
	smp_wmb();
	seqcount_acquire(&s->dep_map, subclass, 0, _RET_IP_);
}

static inline void write_seqcount_begin(seqcount_t *s)
{
	write_seqcount_begin_nested(s, 0);
}

static inline void write_seqcount_end(seqcount_t *s)
{
	seqcount_release(&s->dep_map, 1, _RET_IP_);
	smp_wmb();
	s->sequence++;
}

/**
 * write_seqcount_barrier - invalidate in-progress read-side seq operations
 * @s: pointer to seqcount_t
 *
 * After write_seqcount_barrier, no read-side seq operations will complete
 * successfully and see data older than this.
 */
static inline void write_seqcount_barrier(seqcount_t *s)
{
	smp_wmb();
	s->sequence+=2;
}

typedef struct {
	struct seqcount seqcount;
	spinlock_t lock;
} seqlock_t;

/*
 * These macros triggered gcc-3.x compile-time problems.  We think these are
 * OK now.  Be cautious.
 */
#define __SEQLOCK_UNLOCKED(lockname)			\
	{						\
		.seqcount = SEQCNT_ZERO(lockname),	\
		.lock =	__SPIN_LOCK_UNLOCKED(lockname)	\
	}

#define seqlock_init(x)					\
	do {						\
		seqcount_init(&(x)->seqcount);		\
		spin_lock_init(&(x)->lock);		\
	} while (0)

#define DEFINE_SEQLOCK(x) \
		seqlock_t x = __SEQLOCK_UNLOCKED(x)

/*
 * Read side functions for starting and finalizing a read side section.
 */
static inline unsigned read_seqbegin(const seqlock_t *sl)
{
	return read_seqcount_begin(&sl->seqcount);
}

static inline unsigned read_seqretry(const seqlock_t *sl, unsigned start)
{
	return read_seqcount_retry(&sl->seqcount, start);
}

/*
 * Lock out other writers and update the count.
 * Acts like a normal spin_lock/unlock.
 * Don't need preempt_disable() because that is in the spin_lock already.
 */
static inline void write_seqlock(seqlock_t *sl)
{
	spin_lock(&sl->lock);
	write_seqcount_begin(&sl->seqcount);
}

static inline void write_sequnlock(seqlock_t *sl)
{
	write_seqcount_end(&sl->seqcount);
	spin_unlock(&sl->lock);
}

static inline void write_seqlock_bh(seqlock_t *sl)
{
	spin_lock_bh(&sl->lock);
	write_seqcount_begin(&sl->seqcount);
}

static inline void write_sequnlock_bh(seqlock_t *sl)
{
	write_seqcount_end(&sl->seqcount);
	spin_unlock_bh(&sl->lock);
}

static inline void write_seqlock_irq(seqlock_t *sl)
{
	spin_lock_irq(&sl->lock);
	write_seqcount_begin(&sl->seqcount);
}

static inline void write_sequnlock_irq(seqlock_t *sl)
{
	write_seqcount_end(&sl->seqcount);
	spin_unlock_irq(&sl->lock);
}

static inline unsigned long __write_seqlock_irqsave(seqlock_t *sl)
{
	unsigned long flags;

	spin_lock_irqsave(&sl->lock, flags);
	write_seqcount_begin(&sl->seqcount);
	return flags;
}

#define write_seqlock_irqsave(lock, flags)				\
	do { flags = __write_seqlock_irqsave(lock); } while (0)

static inline void
write_sequnlock_irqrestore(seqlock_t *sl, unsigned long flags)
{
	write_seqcount_end(&sl->seqcount);
	spin_unlock_irqrestore(&sl->lock, flags);
}

#endif /* __LINUX_SEQLOCK_H */
