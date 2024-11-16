// common.h
// Copyright 2024 Ray Gardner
// License: 0BSD
// vi: tabstop=2 softtabstop=2 shiftwidth=2

#ifndef MONOLITHIC

#endif // MONOLITHIC
#ifndef FOR_TOYBOX
#ifndef __STDC_WANT_LIB_EXT2__
#define __STDC_WANT_LIB_EXT2__ 1  // for getline(), getdelim()
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <errno.h>
#include <assert.h>

// for isatty():
#include <unistd.h>
// for getopt_long():
#include <getopt.h>
#include <regex.h>
#if defined(__unix__) || defined(linux)
#include <langinfo.h>
#endif
extern char **environ;    // needed outside toybox with gcc ?

// __USE_MINGW_ANSI_STDIO will have MinGW use its own printf format system?
// Because "The vc6.0 msvcrt.dll that MinGW-w64 targets doesn't implement
// support for the ANSI standard format specifiers."
// https://www.msys2.org/wiki/Porting/
#if !(defined(__unix__) || defined(linux))
#  define __USE_MINGW_ANSI_STDIO            1
#endif

#endif  // FOR_TOYBOX
#ifdef __GNUC__
#ifndef MONOLITHIC
#define ATTR_UNUSED_FUNCTION __attribute__ ((unused))
#define ATTR_UNUSED_VAR __attribute__ ((unused))
#endif // MONOLITHIC
#define ATTR_FALLTHROUGH_INTENDED __attribute__ ((fallthrough))
#else
#ifndef MONOLITHIC
#define ATTR_UNUSED_FUNCTION
#define ATTR_UNUSED_VAR
#endif // MONOLITHIC
#define ATTR_FALLTHROUGH_INTENDED
#endif

#ifndef FOR_TOYBOX
#define maxof(a,b) ((a)>(b)?(a):(b))

#ifndef REG_STARTEND
#define REG_STARTEND 0
#endif

struct arg_list {
  struct arg_list *next;
  char *arg;
};
#endif  // FOR_TOYBOX
////////////////////
////   declarations
////////////////////

#define PBUFSIZE  512 // For num_to_zstring()

#ifndef FOR_TOYBOX
struct scanner_state {
    char *p;
    char *progstring;
    struct arg_list *prog_args;
    char *filename;
    char *line;
    size_t line_size;
    ssize_t line_len;
    int line_num;
    int ch;
    FILE *fp;

    // state includes latest token seen
    int tok;
    int tokbuiltin;
    int toktype;
    char *tokstr;
    // int maxtok;
    // int toklen;
    size_t maxtok;
    size_t toklen;
    // int symtab_entry;
    double numval;
    int error;  // Set if lexical error.
};

struct compiler_globals {
  int in_print_stmt;
  int paren_level;
  int in_function_body;
  int funcnum;
  int nparms;
  int compile_error_count;
  int first_begin;
  int last_begin;
  int first_end;
  int last_end;
  int first_recrule;  // recrule means "record rule"
  int last_recrule;
  int break_dest;
  int continue_dest;
  int stack_offset_to_fix;  // fixup stack if return in for(e in a)
  int range_pattern_num;
  int rule_type;  // tkbegin, tkend, or 0
};

// zvalue: the main awk value type
// Can be number or string or both, or else map (array) or regex
struct zvalue {
  unsigned flags;
  double num;
  union { // anonymous union not in C99; not going to fix it now.
    struct zstring *vst;
    struct zmap *map;
    regex_t *rx;
  };
};

struct runtime_globals {
  struct zvalue cur_arg;
  FILE *fp;           // current data file
  int narg;           // cmdline arg index
  int nfiles;         // num of cmdline data file args processed
  int eof;            // all cmdline files (incl. stdin) read
  char *recptr;
  struct zstring *zspr;      // Global to receive sprintf() string value
};

// zlist: expanding sequential list
struct zlist {
  char *base, *limit, *avail;
  size_t size;
};

struct zfile {
  struct zfile *next;
  char *fn;
  FILE *fp;
  char mode;  // w, a, or r
  char file_or_pipe;  // 1 if file, 0 if pipe
  char is_tty, is_std_file;
  char eof;
  int ro, lim, buflen;
  char *buf;
};

// Global data
struct global_data {
  struct scanner_state *scs;
  char *tokstr;
  // For checking end of prev statement for termination and if '/' can come next
  int prevtok;
  struct zlist globals_table;       // global symbol table
  struct zlist locals_table;        // local symbol table
  struct zlist func_def_table;      // function symbol table

  struct zlist literals;
  struct zlist fields;
  struct zlist zcode;
  struct zlist stack;
  char *progname;
  struct compiler_globals cgl;
  int spec_var_limit;               // used in compile.c and run.c
  int zcode_last;                   // used in only in compile.c
  struct zvalue *stackp;            // ptr to top of runtime stack

  struct runtime_globals rgl;

  char *pbuf;   // Used for number formatting in num_to_zstring()
  regex_t rx_default, rx_last; // Default and last used FS regex
#define FS_MAX  64
  char fs_last[FS_MAX];
  char one_char_fs[4];
  int nf_internal;  // should match NF
  char range_sw[64];   // FIXME TODO quick and dirty set of range switches
  int file_cnt, std_file_cnt;
  struct zfile *zfiles, *cfile, *zstdout;
  regex_t rx_printf_fmt;

#define RS_MAX  64
  char rs_last[RS_MAX];
  regex_t rx_rs_default, rx_rs_last;
};
#endif  // FOR_TOYBOX
enum toktypes {
    // EOF (use -1 from stdio.h)
    ERROR = 2, NEWLINE, VAR, NUMBER, STRING, REGEX, USERFUNC, BUILTIN, TOKEN,
    KEYWORD
    };

// Must align with lbp_table[]
enum tokens {
    tkunusedtoken, tkeof, tkerr, tknl,
    tkvar, tknumber, tkstring, tkregex, tkfunc, tkbuiltin,

// static char *ops = " ;  ,  [  ]  (  )  {  }  $  ++ -- ^  !  *  /  %  +  -     "
//        "<  <= != == >  >= ~  !~ && || ?  :  ^= %= *= /= += -= =  >> |  ";
    tksemi, tkcomma, tklbracket, tkrbracket, tklparen, tkrparen, tklbrace,
    tkrbrace, tkfield, tkincr, tkdecr, tkpow, tknot, tkmul, tkdiv, tkmod,
    tkplus, tkminus,
    tkcat, // !!! Fake operator for concatenation (just adjacent string exprs)
    tklt, tkle, tkne, tkeq, tkgt, tkge, tkmatchop, tknotmatch, tkand, tkor,
    tkternif, tkternelse, tkpowasgn, tkmodasgn, tkmulasgn, tkdivasgn,
    tkaddasgn, tksubasgn, tkasgn, tkappend, tkpipe,

// static char *keywords = " in        BEGIN     END       if        else      "
//    "while     for       do        break     continue  exit      function  "
//    "return    next      nextfile  delete    print     printf    getline   ";
    tkin, tkbegin, tkend, tkif, tkelse,
    tkwhile, tkfor, tkdo, tkbreak, tkcontinue, tkexit, tkfunction,
    tkreturn, tknext, tknextfile, tkdelete, tkprint, tkprintf, tkgetline,

// static char *builtins = " atan2     cos       sin       exp       "
//    "log       sqrt      int       rand      srand     length    "
//    "tolower   toupper   system    fflush    "
//    "and       or        xor       lshift    rshift    ";
    tkatan2, tkcos, tksin, tkexp, tklog, tksqrt, tkint, tkrand, tksrand,
    tklength, tktolower, tktoupper, tksystem, tkfflush,
    tkband, tkbor, tkbxor, tklshift, tkrshift,

// static char *specialfuncs = " close     index     match     split     "
//    "sub       gsub      sprintf   substr    ";
    tkclose, tkindex, tkmatch, tksplit,
    tksub, tkgsub, tksprintf, tksubstr, tklasttk
    };

enum opcodes {
    opunusedop = tklasttk,
    opvarref, opmapref, opfldref, oppush, opdrop, opdrop_n, opnotnot,
    oppreincr, oppredecr, oppostincr, oppostdecr, opnegate, opjump, opjumptrue,
    opjumpfalse, opprepcall, opmap, opmapiternext, opmapdelete, opmatchrec,
    opquit, opprintrec, oprange1, oprange2, oprange3, oplastop
};

// Special variables (POSIX). Must align with char *spec_vars[]
enum spec_var_names { ARGC=1, ARGV, CONVFMT, ENVIRON, FILENAME, FNR, FS, NF,
    NR, OFMT, OFS, ORS, RLENGTH, RS, RSTART, SUBSEP };

struct symtab_slot {    // global symbol table entry
  unsigned flags;
  char *name;
};

// zstring: flexible string type.
// Capacity must be > size because we insert a NUL byte.
struct zstring {
  int refcnt;
  unsigned size;
  unsigned capacity;
  char str[];   // C99 flexible array member
};

// Flag bits for zvalue and symbol tables
#define ZF_MAYBEMAP (1u << 1)
#define ZF_MAP      (1u << 2)
#define ZF_SCALAR   (1u << 3)
#define ZF_NUM      (1u << 4)
#define ZF_RX       (1u << 5)
#define ZF_STR      (1u << 6)
#define ZF_NUMSTR   (1u << 7)   // "numeric string" per posix
#define ZF_REF      (1u << 9)   // for lvalues
#define ZF_MAPREF   (1u << 10)  // for lvalues
#define ZF_FIELDREF (1u << 11)  // for lvalues
#define ZF_EMPTY_RX (1u << 12)
#define ZF_ANYMAP   (ZF_MAP | ZF_MAYBEMAP)

// Macro to help facilitate possible future change in zvalue layout.
#define ZVINIT(flags, num, ptr) {(flags), (double)(num), {(ptr)}}

#define IS_STR(zvalp) ((zvalp)->flags & ZF_STR)
#define IS_RX(zvalp) ((zvalp)->flags & ZF_RX)
#define IS_NUM(zvalp) ((zvalp)->flags & ZF_NUM)
#define IS_MAP(zvalp) ((zvalp)->flags & ZF_MAP)
#define IS_EMPTY_RX(zvalp) ((zvalp)->flags & ZF_EMPTY_RX)

#define GLOBAL      ((struct symtab_slot *)TT.globals_table.base)
#define LOCAL       ((struct symtab_slot *)TT.locals_table.base)
#define FUNC_DEF    ((struct functab_slot *)TT.func_def_table.base)

#define LITERAL     ((struct zvalue *)TT.literals.base)
#define STACK       ((struct zvalue *)TT.stack.base)
#define FIELD       ((struct zvalue *)TT.fields.base)

#define ZCODE       ((int *)TT.zcode.base)

#define FUNC_DEFINED    (1u)
#define FUNC_CALLED     (2u)

#define MIN_STACK_LEFT 1024

struct functab_slot {    // function symbol table entry
  unsigned flags;
  char *name;
  struct zlist function_locals;
  int zcode_addr;
};

// Elements of the hash table (key/value pairs)
struct zmap_slot {
  int hash;       // store hash key to speed hash table expansion
  struct zstring *key;
  struct zvalue val;
};
#define ZMSLOTINIT(hash, key, val) {hash, key, val}

// zmap: Mapping data type for arrays; a hash table. Values in hash are either
// 0 (unused), -1 (marked deleted), or one plus the number of the zmap slot
// containing a key/value pair. The zlist slot entries are numbered from 0 to
// count-1, so need to add one to distinguish from unused.  The probe sequence
// is borrowed from Python dict, using the "perturb" idea to mix in upper bits
// of the original hash value.
struct zmap {
  unsigned mask;  // tablesize - 1; tablesize is 2 ** n
  int *hash;      // (mask + 1) elements
  int limit;      // 80% of table size ((mask+1)*8/10)
  int count;      // number of occupied slots in hash
  int deleted;    // number of deleted slots
  struct zlist slot;     // expanding list of zmap_slot elements
};

#define MAPSLOT    ((struct zmap_slot *)(m->slot).base)
#define FFATAL(format, ...) zzerr("$" format, __VA_ARGS__)
#define FATAL(...) zzerr("$%s\n", __VA_ARGS__)
#define XERR(format, ...) zzerr(format, __VA_ARGS__)

#define NO_EXIT_STATUS  (9999987)  // value unlikely to appear in exit stmt

ssize_t getline(char **lineptr, size_t *n, FILE *stream);
ssize_t getdelim(char ** restrict lineptr, size_t * restrict n, int delimiter, FILE *stream);

#define EXTERN extern
#ifndef FOR_TOYBOX

// Common (global) data
struct optflags {
  char FLAG_b;
};
#define FLAG(x) (optflags.FLAG_##x)
#ifndef MONOLITHIC
EXTERN struct optflags optflags;
EXTERN struct global_data TT;
#endif // MONOLITHIC
#endif  // FOR_TOYBOX

#ifndef MONOLITHIC
EXTERN char *escape_str(char *s, int is_regex);
EXTERN char *progname;
EXTERN struct zvalue uninit_zvalue;
EXTERN struct zvalue uninit_string_zvalue;

EXTERN char *ops, *keywords, *builtins;


EXTERN struct compiler_globals cgl;

EXTERN void zvalue_dup_zstring(struct zvalue *v);

EXTERN void zzerr(char *format, ...);

EXTERN void xregcomp(regex_t *preg, char *regex, int cflags);
EXTERN int regexec0(regex_t *preg, char *string, long len, int nmatch,
  regmatch_t *pmatch, int eflags);
EXTERN int wctoutf8(char *s, unsigned wc);
EXTERN int utf8towc(unsigned *wc, char *str, unsigned len);

EXTERN int bytesinutf8(char *str, size_t len, size_t cnt);
EXTERN int utf8cnt(char *str, size_t len);

EXTERN void awk_exit(int status);
EXTERN void error_exit(char *format, ...);
EXTERN void xfree(void *p);
EXTERN void *xmalloc(size_t size);
EXTERN void *xrealloc(void *p, size_t size);
EXTERN void *xzalloc(size_t size);
EXTERN char *xstrdup(char *s);
EXTERN int hexval(int c);
EXTERN struct zlist *zlist_initx(struct zlist *p, size_t size, size_t count);
EXTERN struct zlist *zlist_init(struct zlist *p, size_t size);
EXTERN void zlist_expand(struct zlist *p);
EXTERN size_t zlist_append(struct zlist *p, void *obj);
EXTERN int zlist_len(struct zlist *p);
EXTERN void get_token_text(char *op, int tk);
EXTERN void zstring_release(struct zstring **s);
EXTERN void zstring_incr_refcnt(struct zstring *s);
EXTERN struct zstring *zstring_update(struct zstring *to, size_t at, char *s, size_t n);
EXTERN struct zstring *zstring_copy(struct zstring *to, struct zstring *from);
EXTERN struct zstring *zstring_extend(struct zstring *to, struct zstring *from);
EXTERN struct zstring *new_zstring(char *s, size_t size);
EXTERN struct zvalue new_str_val(char *s);
EXTERN void zvalue_release_zstring(struct zvalue *v);
EXTERN void push_val(struct zvalue *v);
EXTERN void zvalue_copy(struct zvalue *to, struct zvalue *from);
EXTERN void init_scanner(void);
EXTERN void scan(void);
EXTERN int find_global(char *s);
EXTERN void compile(void);
EXTERN int zstring_match(struct zstring *a, struct zstring *b);
EXTERN struct zvalue *zmap_find(struct zmap *m, struct zstring *key);
EXTERN void zvalue_map_init(struct zvalue *v);
EXTERN void zmap_delete_map_incl_slotdata(struct zmap *m);
EXTERN void zmap_delete_map(struct zmap *m);
EXTERN struct zmap_slot *zmap_find_or_insert_key(struct zmap *m, struct zstring *key);
EXTERN void zmap_delete(struct zmap *m, struct zstring *key);
EXTERN void run(int optind, int argc, char **argv, char *sepstring,
    struct arg_list *assign_args);

#endif // MONOLITHIC
#undef EXTERN
#define EXTERN
