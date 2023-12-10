// main.c
// Copyright 2023 Ray Gardner
// vi: tabstop=2 softtabstop=2 shiftwidth=2

#include "common.h"

////////////////////
//// main
////////////////////

static void progfiles_init(char *progstring, char **progfiles,
    int num_progfiles)
{

  scs->p = progstring ? progstring : "  " + 2;
  scs->progstring = progstring;
  scs->num_progfiles = num_progfiles;
  scs->progfiles = progfiles;
  scs->cur_progfile = 0;
  scs->filename = "(None)";  // Not needed...
  scs->line = 0;      // for getline()
  scs->line_size = 0; // for getline()
  scs->line_num = 0;  // Not needed...
  scs->fp = 0;        // For get_char() initial state.

  scs->tok = 0;
  scs->tokbuiltin = 0;
  scs->toktype = 0;
  scs->maxtok = 256;
  scs->tokstr = xzalloc(scs->maxtok);
  scs->toklen = 0;        // Needed?
  scs->ch = 0;        // Needed?

  scs->numval = 0;
  scs->error = 0;
}

static int awk(char *sepstring, int num_assignments, char **assignments,
    int num_progfiles, char **progfiles, char *progstring, int optind,
    int argc, char **argv, int opt_test_scanner, int opt_dump_symbols,
    int opt_run_prog, char **envp)
{
(void)opt_test_scanner, (void)opt_dump_symbols;

  progfiles_init(progstring, progfiles, num_progfiles);
  compile();

  if (cgl.compile_error_count)
    fprintf(stderr, "Compile ended with %d error%s.\n", cgl.compile_error_count,
        cgl.compile_error_count == 1 ? "" : "s");
  else {
    if (opt_run_prog)
      run(optind, argc, argv, sepstring, num_assignments, assignments, envp);
  }

  return cgl.compile_error_count;
}

EXTERN int trace_sw = 0;

#ifndef FOR_TOYBOX
int main(int argc, char **argv, char **envp)
{
  char *usage = {
    "Usage:\n"
      "awk [-F sepstring] [-v assignment]... program [argument...]\n"
      "or:\n"
      "awk [-F sepstring] -f progfile [-f progfile]...\n"
      "               [-v assignment]...  [argument...]\n"
      "also:\n"
      "-V show version\n"
      "-r do not run, just compile\n"
      "-p print source\n"
  };
  progname = argv[0];
  char *sepstring = " ";
  // FIXME Need check on these, or use dynamic mem.
  int num_assignments = 0, num_progfiles = 0;
  char *assignments[42];
  char *progfiles[42];
  char *progstring = 0;
  int opt_test_scanner = 0;
  int opt_dump_symbols = 0;
  int opt_run_prog = 1;
  int opt;

  while ((opt = getopt(argc, argv, ":F:f:v:tsVzdD:pr")) != -1) {
    switch (opt) {
      case 'F':
        sepstring = escape_str(optarg);
        break;
      case 'f':
        progfiles[num_progfiles++] = optarg;
        break;
      case 'v':
        assignments[num_assignments++] = optarg;
        break;
      case 'V':
        printf("<%s %s>\n", __DATE__, __TIME__);
        exit(0);
        break;
      case 'p':
        opt_print_source = 1;
        break;
      case 'r':
        opt_run_prog = 0;
        break;
        break;
      default:
        fprintf(stderr, "%s", usage);
        exit(EXIT_FAILURE);
    }
  }

  if (num_progfiles == 0) {
    if (optind >= argc) {
      fprintf(stderr, "No program string.\n%s", usage);
      exit(EXIT_FAILURE);
    }
    progstring = argv[optind++];
  }
#if defined(__unix__) || defined(linux)
  // Toybox main.c does this, so do we.
  // Try user's locale, but if that isn't UTF-8 merge in a UTF-8 locale's
  // character type data. (Fall back to en_US for MacOS.)
  setlocale(LC_CTYPE, "");
  if (strcmp("UTF-8", nl_langinfo(CODESET)))
    uselocale(newlocale(LC_CTYPE_MASK, "C.UTF-8", 0) ? :
      newlocale(LC_CTYPE_MASK, "en_US.UTF-8", 0));
#endif

  return awk(sepstring, num_assignments, assignments, num_progfiles, progfiles,
      progstring, optind, argc, argv, opt_test_scanner, opt_dump_symbols,
      opt_run_prog, envp);
}
#endif  // FOR_TOYBOX
