
#ifdef FOOBAR
void hello();
#endif

#if false



#endif

#if defined(OS_FOO)

#endif

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": ["2:1-5:1", "6:1-11:1", "12:1-15:1"],
  "usr2func": [],
  "usr2type": [],
  "usr2var": []
}
*/
