int
unw_get_save_loc (unw_cursor_t *cursor, int regnum, unw_save_loc_t *loc)
{
#ifdef UNW_LOCAL_ONLY
  unw_word_t addr;

  YYY; this is wrong: need to consider cursor save locs first; YYY
  loc->type = UNW_SLT_NONE;
  addr = uc_addr (c->uc, regnum);
  if (addr)
    {
      loc->type = UNW_SLT_MEMORY;
      loc->addr = addr;
    }
#else
  XXX;
#endif
  return 0;
}
