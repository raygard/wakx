// scan.c
// Copyright 2023 Ray Gardner
// vi: tabstop=2 softtabstop=2 shiftwidth=2

#include "common.h"

////////////////////
//// scan (lexical analyzer)
////////////////////

// TODO:
// IS line_num getting incr correctly? Newline counts as start of line!?
// Handle nuls in file better.
// Open files "rb" and handle CRs in program.
// Roll gch() into get_char() ?
// Deal with signed char (at EOF? elsewhere?)
//
// 2023-01-11: Allow nul bytes inside strings? regexes?

EXTERN int opt_print_source = 0;

EXTERN struct scanner_state scs_current, *scs = &scs_current;

static void progfile_open(void)
{
  scs->filename = scs->progfiles[scs->cur_progfile];
  scs->fp = stdin;
  if (strcmp(scs->filename, "-")) scs->fp = fopen(scs->filename, "r");
  if (!scs->fp) error_exit("Can't open %s.\n", scs->filename);
  scs->line_num = 0;
}

static int get_char(void)
{
  static char *nl = "\n";
  // On first entry, scs->p points to progstring if any, or null string.
  for (;;) {
    int c = *(scs->p)++;
    if (c) {
      return c;
    }
    if (scs->progstring) {  // Fake newline at end of progstring.
      if (scs->progstring == nl) return EOF;
      scs->p = scs->progstring = nl;
      continue;
    }
    // Here if getting from progfile(s).
    if (scs->line == nl) return EOF;
    if (!scs->fp) {
      progfile_open();
    // The "  " + 1 is to set p to null string but allow ref to prev char for
    // "lastchar" test below.
    }
    // Save last char to allow faking final newline.
    int lastchar = (scs->p)[-2];
    scs->line_len = getline(&scs->line, &scs->line_size, scs->fp);
    if (scs->line_len > 0) {
      scs->line_num++;
      if (opt_print_source) fprintf(stderr, "|>%3d %s", scs->line_num, scs->line);
      scs->p = scs->line;
      continue;
    }
    // EOF
    // FIXME TODO or check for error? feof() vs. ferror()
    fclose(scs->fp);
    scs->fp = 0;
    scs->p = "  " + 2;
    scs->cur_progfile++;
    if (scs->cur_progfile >= scs->num_progfiles) {
      xfree(scs->line);
      if (lastchar == '\n') return EOF;
      // Fake final newline
      scs->line = scs->p = nl;
    }
  }
}

static void append_this_char(int c)
{
  if (scs->toklen == scs->maxtok - 1) {
    scs->maxtok *= 2;
    scs->tokstr = xrealloc(scs->tokstr, scs->maxtok);
  }
  scs->tokstr[scs->toklen++] = c;
  scs->tokstr[scs->toklen] = 0;
}

static void gch(void)
{
  // FIXME probably not right place to skip CRs.
  do {
    scs->ch = get_char();
  } while (scs->ch == '\r');
}

static void append_char(void)
{
  append_this_char(scs->ch);
  gch();
}

static int find_keyword_or_builtin(char *table,
    int first_tok_in_table)
{
  char s[16] = " ", *p;
  // keywords and builtin functions are spaced 10 apart for strstr() lookup,
  // so must be less than that long.
  if (scs->toklen >= 10) return 0;
  strcat(s, scs->tokstr);
  strcat(s, " ");
  p = strstr(table, s);
  if (!p) return 0;
  return first_tok_in_table + (p - table) / 10;
}

static int find_token(void)
{
  char s[6] = " ", *p;
  // tokens are spaced 3 apart for strstr() lookup, so must be less than
  // that long.
  strcat(s, scs->tokstr);
  strcat(s, " ");
  p = strstr(ops, s);
  if (!p) return 0;
  return tksemi + (p - ops) / 3;
}

static int find_keyword(void)
{
  return find_keyword_or_builtin(keywords, tkin);
}

static int find_builtin(void)
{
  return find_keyword_or_builtin(builtins, tkatan2);
}

static void get_number(void)
{
  // Assumes scs->ch is digit or dot on entry.
  // scs->p points to the following character.
  // OK formats: 1 1. 1.2 1.2E3 1.2E+3 1.2E-3 1.E2 1.E+2 1.E-2 1E2 .1 .1E2
  // .1E+2 .1E-2
  // NOT OK: . .E .E1 .E+ .E+1 ; 1E .1E 1.E 1.E+ 1.E- parse as number
  // followed by variable E.
  // gawk accepts 12.E+ and 12.E- as 12; nawk & mawk say syntax error.
  char *leftover;
  int len;
  scs->numval = strtod(scs->p - 1, &leftover);
  len = leftover - scs->p + 1;
  if (len == 0) {
    append_char();
    scs->toktype = ERROR;
    scs->tok = tkerr;
    scs->error = 1;
    ffatal("Unexpected token '%s'\n", scs->tokstr);
    return;
  }
  while (len--)
    append_char();
}

static void get_string_or_regex(int endchar)
{
  gch();
  while (scs->ch != endchar) {
    if (scs->ch == '\n') {
      // FIXME Handle unterminated string or regex. Is this OK?
      // FIXME TODO better diagnostic here?
      fprintf(stderr, "unterminated string or regex\n");
      cgl.compile_error_count++;
      break;
    } else if (scs->ch == '\\') {
      // \\ \a \b \f \n \r \t \v \" \/ \ddd
      char *p, *escapes = "\\abfnrtv\"/";
      gch();
      if (scs->ch == '\n') {  // backslash newline is continuation
        gch();
        continue;
      } else if ((p = strchr(escapes, scs->ch))) {
        // posix regex does not use these escapes,
        // but awk does, so do them.
        int c = "\\\a\b\f\n\r\t\v\"/"[p-escapes];
        append_this_char(c);
        // Need to double up \ inside literal regex
        if (endchar == '/' && c == '\\') append_this_char('\\');
        gch();
      } else if (scs->ch == 'x') {
        gch();
        if (isxdigit(scs->ch)) {
          int c = hexval(scs->ch);
          gch();
          if (isxdigit(scs->ch)) {
            c = c * 16 + hexval(scs->ch);
            gch();
          }
          append_this_char(c);
        } else append_this_char('x');
      } else if (isdigit(scs->ch)) {
        if (scs->ch < '8') {
          int k, c = 0;
          for (k = 0; k < 3; k++) {
            if (isdigit(scs->ch) && scs->ch < '8') {
              c = c * 8 + scs->ch - '0';
              gch();
            } else
              break;
          }
          append_this_char(c);
        } else {
          append_char();
        }
      } else {
        if (endchar == '/') {
          // pass \ unmolested if not awk escape,
          // so that regex routines can see it.
          if (!strchr(".[]()*+?{}|^$-", scs->ch)) {
            fprintf(stderr, "%s: %d: ", scs->filename, scs->line_num);
            fprintf(stderr, "warning: '\\%c' -- unknown regex escape\n", scs->ch);
          }
          append_this_char('\\');
        } else {
          fprintf(stderr, "%s: %d: ", scs->filename, scs->line_num);
          fprintf(stderr, "warning: '\\%c' treated as plain '%c'\n", scs->ch, scs->ch);
        }
      }
    } else if (scs->ch == EOF) {
      fatal("EOF in string or regex\n");
    } else {
      append_char();
    }
  }
  gch();
}


static void ascan_opt_div(int div_op_allowed_here)
{
  int n;
  for (;;) {
    scs->tokbuiltin = 0;
    scs->toklen = 0;
    scs->tokstr[0] = 0;
    while (scs->ch == ' ' || scs->ch == '\t')
      gch();
    if (scs->ch == '\\') {
      append_char();
      if (scs->ch == '\n') {
        gch();
        continue;
      }
      scs->toktype = ERROR;   // \ not last char in line.
      scs->tok = tkerr;
      scs->error = 3;
      fatal("backslash not last char in line\n");
      return;
    }
    break;
  }
  // Note \<NEWLINE> in comment does not continue it.
  if (scs->ch == '#') {
    gch();
    while (scs->ch != '\n')
      gch();
    // Need to fall through here to pick up newline.
  }
  if (scs->ch == '\n') {
    scs->toktype = NEWLINE;
    scs->tok = tknl;
    append_char();
  } else if (isalpha(scs->ch) || scs->ch == '_') {
    append_char();
    while (isalnum(scs->ch) || scs->ch == '_') {
      append_char();
    }
    if ((n = find_keyword()) != 0) {
      scs->toktype = KEYWORD;
      scs->tok = n;
    } else if ((n = find_builtin()) != 0) {
      scs->toktype = BUILTIN;
      scs->tok = tkbuiltin;
      scs->tokbuiltin = n;
    } else if ((scs->ch == '(')) {
      scs->toktype = USERFUNC;
      scs->tok = tkfunc;
    } else {
      scs->toktype = VAR;
      scs->tok = tkvar;
      // skip whitespace to be able to check for , or )
      while (scs->ch == ' ' || scs->ch == '\t')
        gch();
    }
    return;
  } else if (scs->ch == '"') {
    scs->toktype = STRING;
    scs->tok = tkstring;
    get_string_or_regex('"');
  } else if (isdigit(scs->ch) || scs->ch == '.') {
    scs->toktype = NUMBER;
    scs->tok = tknumber;
    get_number();
  } else if (scs->ch == '/' && ! div_op_allowed_here) {
    scs->toktype = REGEX;
    scs->tok = tkregex;
    get_string_or_regex('/');
  } else if (scs->ch == EOF) {
    scs->toktype = EOF;
    scs->tok = tkeof;
  } else if (scs->ch == '\0') {
    append_char();
    scs->toktype = ERROR;
    scs->tok = tkerr;
    scs->error = 5;
    fatal("null char\n");
  } else {
    // All other tokens.
    scs->toktype = scs->ch;
    append_char();
    // Special case for **= and ** tokens
    if (scs->toktype == '*' && scs->ch == '*') {
      append_char();
      if (scs->ch == '=') {
        append_char();
        scs->tok = tkpowasgn;
      } else scs->tok = tkpow;
      scs->toktype = scs->tok + 200;
      return;
    }
    // Is it a 2-character token?
    if (scs->ch != ' ' && scs->ch != '\n') {
      append_this_char(scs->ch);
      if (find_token()) {
        scs->tok = find_token();
        scs->toktype = scs->tok + 200;
        gch();  // Eat second char of token.
        return;
      }
      scs->toklen--;  // Not 2-character token; back off.
      scs->tokstr[scs->toklen] = 0;
    }
    scs->tok = find_token();
    if (scs->tok) return;
    scs->toktype = ERROR;
    scs->tok = tkerr;
    scs->error = 4;
    ffatal("Unexpected token '%s'\n", scs->tokstr);
  }
}

static void scan_opt_div(int div_op_allowed_here)
{
  // TODO FIXME need better diags for bad tokens!
  // TODO Also set global syntax error flag.
  do ascan_opt_div(div_op_allowed_here); while (scs->tok == tkerr);
}

#define DIV_ALLOWED 1

// Used when scanning in context where '/' means div not regex.
// Standard says that's wherever / or /= can mean divide.
static void scan_div_ok(void)
{
  scan_opt_div(DIV_ALLOWED);
}

// Used when scanning in context where '/' means regex.
static void scan_regex_ok(void)
{
  scan_opt_div(! DIV_ALLOWED);
}

EXTERN void init_scanner(void)
{
  gch();
}

static char div_preceders[] = {tknumber, tkstring, tkvar, tkgetline, tkrparen, tkrbracket, tkincr, tkdecr, 0};

EXTERN char *tokstr = 0;

static int prevscstok = tkeof; // Only for checking if div op can appear next
EXTERN int prevtok = tkeof; // For checking end of prev statement for termination

EXTERN void scan(void)
{
  prevtok = scs->tok;
  if (prevtok && strchr(div_preceders, prevtok)) scan_div_ok();
  else scan_regex_ok();
  prevscstok = scs->tok;
  tokstr = scs->tokstr;
}
