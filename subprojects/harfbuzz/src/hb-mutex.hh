#ifndef HB_MUTEX_HH
#define HB_MUTEX_HH

#include "hb.hh"

#include <mutex>

struct hb_mutex_t
{
  std::mutex* m;

  void init   () { m = new std::mutex(); }
  void lock   () { m->lock();            }
  void unlock () { m->unlock();          }
  void fini   () { delete m;             }
};

struct hb_lock_t
{
  hb_lock_t (hb_mutex_t &mutex_) : mutex (mutex_) { mutex.lock (); }
  ~hb_lock_t () { mutex.unlock (); }
  private:
  hb_mutex_t &mutex;
};

#endif /* HB_MUTEX_HH */
