// main.c
// Copyright 2024 Ray Gardner
// License: 0BSD
// vi: tabstop=2 softtabstop=2 shiftwidth=2

#include "common.h"

////////////////////
//// main
////////////////////

#ifndef FOR_TOYBOX
char *version = "24.10 20241008";

#endif  // FOR_TOYBOX
static void progfiles_init(char *progstring, struct arg_list *prog_args)
{
  TT.scs->p = progstring ? progstring : &("  "[2]);   // Quiet clang warning on "  " + 2;
  TT.scs->progstring = progstring;
  TT.scs->prog_args = prog_args;
  TT.scs->filename = "(cmdline)";
  TT.scs->maxtok = 256;
  TT.scs->tokstr = xzalloc(TT.scs->maxtok);
}

static int awk(char *sepstring, char *progstring, struct arg_list *prog_args,
    struct arg_list *assign_args, int optind, int argc, char **argv,
    int opt_run_prog)
{
  struct scanner_state ss = {0};
  TT.scs = &ss;

  setlocale(LC_NUMERIC, "");
  progfiles_init(progstring, prog_args);
  compile();

  if (TT.cgl.compile_error_count)
    error_exit("%d syntax error(s)", TT.cgl.compile_error_count);
  else {
    if (opt_run_prog)
      run(optind, argc, argv, sepstring, assign_args);
  }

  return TT.cgl.compile_error_count;
}

#ifndef FOR_TOYBOX

static struct arg_list **new_arg(struct arg_list **pnext, char *arg)
{
  *pnext = xzalloc(sizeof(**pnext));
  (*pnext)->arg = arg;
  return &(*pnext)->next;
}

static void free_args(struct arg_list *p)
{
  for (struct arg_list *np; p; p = np) {
    np = p->next;
    xfree(p);
  }
}

int main(int argc, char **argv)
{
  char *usage = {
    "Usage:\n"
      "awk [-F sepstring] [-v assignment]... program [argument...]\n"
      "or:\n"
      "awk [-F sepstring] -f progfile [-f progfile]...\n"
      "               [-v assignment]...  [argument...]\n"
      "also:\n"
      "-V or --version  show version\n"
      "-h or --help     show this usage screen\n"

      "-b use bytes, not characters\n"
      "-c compile only, do not run\n"
  };
  char pbuf[PBUFSIZE];
  TT.pbuf = pbuf;
  TT.progname = argv[0];
  char *sepstring = " ";
  // FIXME Need check on these, or use dynamic mem.
  char *progstring = 0;
  int opt_run_prog = 1;
  int opt;
  int retval;

  struct arg_list *prog_args = 0, **tail_prog_args = &prog_args;
  struct arg_list *assign_args = 0, **tail_assign_args = &assign_args;

  struct option longopts[] = {{"version", 0, 0, 'V'}, {"help", 0, 0, 'h'}, {0}};
  
  char *p = setlocale(LC_CTYPE, "");
  if (!p || !strstr(p, "UTF-8")) p = setlocale(LC_CTYPE, "C.UTF-8");
  if (!p || !strstr(p, "UTF-8")) p = setlocale(LC_CTYPE, "en_US.UTF-8");
  while ((opt = getopt_long(argc, argv, "F:f:v:Vbch", longopts, 0)) != -1) {
    switch (opt) {
      case 'F':
        sepstring = escape_str(optarg, 0);
        break;
      case 'b':
        optflags.FLAG_b = 1;
        break;
      case 'f':
        tail_prog_args = new_arg(tail_prog_args, optarg);
        break;
      case 'v':
        tail_assign_args = new_arg(tail_assign_args, optarg);
        break;
      case 'V':
        printf("version %s, compiled %s %s\n", version, __DATE__, __TIME__);
        awk_exit(0);
        break;
      case 'c':
        opt_run_prog = 0;
        break;
      case 'h':
        printf("%s", usage);
        exit(0);
        break;
        break;
      default:
        error_exit("Option error.\n%s", usage);
    }
  }

  if (!prog_args) {
    if (optind >= argc) {
      error_exit("No program string.\n%s", usage);
    }
    progstring = argv[optind++];
  }

  retval = awk(sepstring, progstring, prog_args, assign_args, optind,
       argc, argv, opt_run_prog);
  free_args(assign_args);
  free_args(prog_args);
  return retval;
}
#endif  // FOR_TOYBOX
