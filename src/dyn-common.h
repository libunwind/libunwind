static int
find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		void *arg, unw_dyn_info_list_t *list)
{
  unw_dyn_info_t *di;

  for (di = list->first; di; di = di->next)
    {
      if (ip >= di->start_ip && ip < di->end_ip)
	{
	  pi->start_ip = di->start_ip;
	  pi->end_ip = di->end_ip;
	  pi->gp = di->gp;
	  pi->format = di->format;
	  switch (di->format)
	    {
	    case UNW_INFO_FORMAT_DYNAMIC:
	      pi->proc_name = di->u.pi.name;
	      pi->handler = di->u.pi.handler;
	      pi->flags = di->u.pi.flags;
	      pi->unwind_info = di;
	      return 0;

	    case UNW_INFO_FORMAT_TABLE:
#ifdef unw_search_unwind_table
	      /* call platform-specific search routine: */
	      return sysdep_search_unwind_table (as, ip, &di->u.ti, pi, arg);
#endif

	    default:
	      break;
	    }
	}
    }
  return -UNW_ENOINFO;
}

static int
remote_find_proc_info (unw_addr_space_t as, unw_word_t ip,
		       unw_proc_info_t *pi, void *arg)
{
  unw_dyn_info_list_t *list = NULL;

  fprintf (stderr, "remote_find_proc_info: not implemented yet\n");
  /* use as.accessors to locate _U_dyn_info_list.  */
  /* check _U_dyn_info_list.generation to see if cached info is stale */
  /* if cached info is stale, use as.accessors to read new info */
  /* internalize the data */
  /* re-check _U_dyn_info_list.generation to verify that read-in info
     is consistent */
  return find_proc_info (as, ip, pi, arg, list);
}
