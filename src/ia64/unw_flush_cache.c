void
unw_flush_cache (void)
{
  /* this lets us flush caches lazily... */
  ++unw.cache_generation;
}
