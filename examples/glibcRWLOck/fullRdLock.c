static __always_inline int
__pthread_rwlock_rdlock_full (pthread_rwlock_t *rwlock,
    const struct timespec *abstime)
{
  unsigned int r;

  /* Make sure we are not holding the rwlock as a writer.  This is a deadlock
     situation we recognize and report.  */
  if (__glibc_unlikely (atomic_load_relaxed (&rwlock->__data.__cur_writer)
			== THREAD_GETMEM (THREAD_SELF, tid)))
    return EDEADLK;

    /* Register as a reader, using an add-and-fetch so that R can be used as
     expected value for future operations.  Acquire MO so we synchronize with
     prior writers as well as the last reader of the previous read phase (see
     below).  */
  r = (atomic_fetch_add_acquire (&rwlock->__data.__readers,
				 (1 << PTHREAD_RWLOCK_READER_SHIFT))
       + (1 << PTHREAD_RWLOCK_READER_SHIFT));

  /* Check whether there is an overflow in the number of readers.  We assume
     that the total number of threads is less than half the maximum number
     of readers that we have bits for in __readers (i.e., with 32-bit int and
     PTHREAD_RWLOCK_READER_SHIFT of 3, we assume there are less than
     1 << (32-3-1) concurrent threads).
     If there is an overflow, we use a CAS to try to decrement the number of
     readers if there still is an overflow situation.  If so, we return
     EAGAIN; if not, we are not a thread causing an overflow situation, and so
     we just continue.  Using a fetch-add instead of the CAS isn't possible
     because other readers might release the lock concurrently, which could
     make us the last reader and thus responsible for handing ownership over
     to writers (which requires a CAS too to make the decrement and ownership
     transfer indivisible).  */
  while (__glibc_unlikely (r >= PTHREAD_RWLOCK_READER_OVERFLOW))
    {
      /* Relaxed MO is okay because we just want to undo our registration and
	 cannot have changed the rwlock state substantially if the CAS
	 succeeds.  */
      if (atomic_compare_exchange_weak_relaxed
	  (&rwlock->__data.__readers,
	   &r, r - (1 << PTHREAD_RWLOCK_READER_SHIFT)))
	return EAGAIN;
    }

  /* We have registered as a reader, so if we are in a read phase, we have
     acquired a read lock.  This is also the reader--reader fast-path.
     Even if there is a primary writer, we just return.  If writers are to
     be preferred and we are the only active reader, we could try to enter a
     write phase to let the writer proceed.  This would be okay because we
     cannot have acquired the lock previously as a reader (which could result
     in deadlock if we would wait for the primary writer to run).  However,
     this seems to be a corner case and handling it specially not be worth the
     complexity.  */
  if (__glibc_likely ((r & PTHREAD_RWLOCK_WRPHASE) == 0))
    return 0;
  /* Otherwise, if we were in a write phase (states #6 or #8), we must wait
     for explicit hand-over of the read phase; the only exception is if we
     can start a read phase if there is no primary writer currently.  */
  while ((r & PTHREAD_RWLOCK_WRPHASE) != 0
	 && (r & PTHREAD_RWLOCK_WRLOCKED) == 0)
    {
      /* Try to enter a read phase: If the CAS below succeeds, we have
	 ownership; if it fails, we will simply retry and reassess the
	 situation.
	 Acquire MO so we synchronize with prior writers.  */
      if (atomic_compare_exchange_weak_acquire (&rwlock->__data.__readers, &r,
						r ^ PTHREAD_RWLOCK_WRPHASE))
	{
	  /* We started the read phase, so we are also responsible for
	     updating the write-phase futex.  Relaxed MO is sufficient.
	     We have to do the same steps as a writer would when handing
	     over the read phase to us because other readers cannot
	     distinguish between us and the writer; this includes
	     explicit hand-over and potentially having to wake other readers
	     (but we can pretend to do the setting and unsetting of WRLOCKED
	     atomically, and thus can skip this step).  */
	  if ((atomic_exchange_relaxed (&rwlock->__data.__wrphase_futex, 0)
	       & PTHREAD_RWLOCK_FUTEX_USED) != 0)
	    {
	      int private = __pthread_rwlock_get_private (rwlock);
	      futex_wake (&rwlock->__data.__wrphase_futex, INT_MAX, private);
	    }
	  return 0;
	}
      else
	{
	  /* TODO Back off before retrying.  Also see above.  */
	}
    }

  /* We were in a write phase but did not install the read phase.  We cannot
     distinguish between a writer and another reader starting the read phase,
     so we must wait for explicit hand-over via __wrphase_futex.
     However, __wrphase_futex might not have been set to 1 yet (either
     because explicit hand-over to the writer is still ongoing, or because
     the writer has started the write phase but has not yet updated
     __wrphase_futex).  The least recent value of __wrphase_futex we can
     read from here is the modification of the last read phase (because
     we synchronize with the last reader in this read phase through
     __readers; see the use of acquire MO on the fetch_add above).
     Therefore, if we observe a value of 0 for __wrphase_futex, we need
     to subsequently check that __readers now indicates a read phase; we
     need to use acquire MO for this so that if we observe a read phase,
     we will also see the modification of __wrphase_futex by the previous
     writer.  We then need to load __wrphase_futex again and continue to
     wait if it is not 0, so that we do not skip explicit hand-over.
     Relaxed MO is sufficient for the load from __wrphase_futex because
     we just use it as an indicator for when we can proceed; we use
     __readers and the acquire MO accesses to it to eventually read from
     the proper stores to __wrphase_futex.  */
  unsigned int wpf;
  bool ready = false;
  for (;;)
    {
      while (((wpf = atomic_load_relaxed (&rwlock->__data.__wrphase_futex))
	      | PTHREAD_RWLOCK_FUTEX_USED) == (1 | PTHREAD_RWLOCK_FUTEX_USED))
	{
	  int private = __pthread_rwlock_get_private (rwlock);
	  if (((wpf & PTHREAD_RWLOCK_FUTEX_USED) == 0)
	      && (!atomic_compare_exchange_weak_relaxed
		  (&rwlock->__data.__wrphase_futex,
		   &wpf, wpf | PTHREAD_RWLOCK_FUTEX_USED)))
	    continue;
	  int err = futex_abstimed_wait (&rwlock->__data.__wrphase_futex,
					 1 | PTHREAD_RWLOCK_FUTEX_USED,
					 abstime, private);
	  if (err == ETIMEDOUT)
	    {
	      /* If we timed out, we need to unregister.  If no read phase
		 has been installed while we waited, we can just decrement
		 the number of readers.  Otherwise, we just acquire the
		 lock, which is allowed because we give no precise timing
		 guarantees, and because the timeout is only required to
		 be in effect if we would have had to wait for other
		 threads (e.g., if futex_wait would time-out immediately
		 because the given absolute time is in the past).  */
	      r = atomic_load_relaxed (&rwlock->__data.__readers);
	      while ((r & PTHREAD_RWLOCK_WRPHASE) != 0)
		{
		  /* We don't need to make anything else visible to
		     others besides unregistering, so relaxed MO is
		     sufficient.  */
		  if (atomic_compare_exchange_weak_relaxed
		      (&rwlock->__data.__readers, &r,
		       r - (1 << PTHREAD_RWLOCK_READER_SHIFT)))
		    return ETIMEDOUT;
		  /* TODO Back-off.  */
		}
	      /* Use the acquire MO fence to mirror the steps taken in the
		 non-timeout case.  Note that the read can happen both
		 in the atomic_load above as well as in the failure case
		 of the CAS operation.  */
	      atomic_thread_fence_acquire ();
	      /* We still need to wait for explicit hand-over, but we must
		 not use futex_wait anymore because we would just time out
		 in this case and thus make the spin-waiting we need
		 unnecessarily expensive.  */
	      while ((atomic_load_relaxed (&rwlock->__data.__wrphase_futex)
		      | PTHREAD_RWLOCK_FUTEX_USED)
		     == (1 | PTHREAD_RWLOCK_FUTEX_USED))
		{
		  /* TODO Back-off?  */
		}
	      ready = true;
	      break;
	    }
	  /* If we got interrupted (EINTR) or the futex word does not have the
	     expected value (EAGAIN), retry.  */
	}
      if (ready)
	/* See below.  */
	break;
      /* We need acquire MO here so that we synchronize with the lock
	 release of the writer, and so that we observe a recent value of
	 __wrphase_futex (see below).  */
      if ((atomic_load_acquire (&rwlock->__data.__readers)
	   & PTHREAD_RWLOCK_WRPHASE) == 0)
	/* We are in a read phase now, so the least recent modification of
	   __wrphase_futex we can read from is the store by the writer
	   with value 1.  Thus, only now we can assume that if we observe
	   a value of 0, explicit hand-over is finished. Retry the loop
	   above one more time.  */
	ready = true;
    }

  return 0;
}
