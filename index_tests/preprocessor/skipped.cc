
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
  "skipped_by_preprocessor": ["2:1-4:7", "6:1-10:7", "12:1-14:7"],
  "usr2func": [],
  "usr2type": [],
  "usr2var": []
}
*/
