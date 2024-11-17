# Makefile for wak

.POSIX:
#.SILENT:

.PHONY: clean all allplus mono win windows toy help

prefix ?= /usr/local
DESTDIR ?=

CC = gcc

MUSLCC = /usr/local/musl/bin/musl-gcc

CFLAGS = -O3 -funsigned-char -std=c99 -Wall -Wextra -W -Wpointer-arith -Wstrict-prototypes -D_POSIX_C_SOURCE=200809L

# Order is significant for monolithic source wak.c and toybox awk!
SRC = ./src/lib.c ./src/common.c ./src/scan.c ./src/compile.c ./src/run.c ./src/main.c
ALLSRC = ./src/common.h $(SRC)

AWK=./aswak
AWK=./wak

TESTDIR=./test

all: wak

help:
	@echo "Usage: make OR make all OR make target ..."

	@echo "Where 'target' is one of all wak mwak aswak prwak muwak mmuwak ..."
	@echo "... mono check test install clean extra musl toy win help"
	@echo "Default (no target) makes wak"
	@echo "all      -- makes wak"
	@echo "wak      -- wak (from separate .c files)"
	@echo "mwak     -- wak from monolithic (single source file)"
	@echo "aswak    -- ASAN (address sanitizer) wak (for debugging)"
	@echo "prwak    -- profiling wak"
	@echo "muwak    -- musl wak from separate .c files"
	@echo "mmuwak   -- musl wak from monolithic source"
	@echo "mono     -- make monolithic source (in onefile/ , not compiled)"
	@echo "check    -- run some validation tests"
	@echo "test     -- same as check"
	@echo "install  -- try to install wak in /usr/local/bin; only for wak"
	@echo "clean    -- remove compiled versions etc."
	@echo "extra    -- makes mwak aswak prwak"
	@echo "musl     -- makes muwak mmuwak"
	@echo "toy      -- makes toybox source; not compiled"
	@echo "win      -- wak.exe for Windows"
	@echo "help     -- show this screen"
	@echo "muwak and mmuwak require musl system (musl-gcc)"
	@echo "wak.exe for Windows requires msys2, build under Windows"

extra: mwak aswak prwak

musl: muwak mmuwak

wak: $(ALLSRC)
	$(CC) $(CFLAGS) $(SRC) -lm -o wak

prwak: $(ALLSRC)
	$(CC) $(CFLAGS) $(SRC) -lm -pg -o prwak

aswak: $(ALLSRC)
	$(CC) $(CFLAGS) -fsanitize=address $(SRC) -lm -o aswak

mono: ./onefile/wak.c

./onefile/wak.c: $(ALLSRC)
	@mkdir -p $(@D)
	awk -f ./scripts/make_mono.awk $(ALLSRC) > $@

mwak: ./onefile/wak.c
	$(CC) $(CFLAGS) ./onefile/wak.c -lm -o mwak

mmuwak: ./onefile/wak.c
	$(MUSLCC) $(CFLAGS) $^ -static -s -lm -o mmuwak

muwak: $(ALLSRC)
	$(MUSLCC) $(CFLAGS) $(SRC) -static -s -lm -o muwak

win windows: ./wak.exe

./wak.exe: $(ALLSRC) ./getline.c
	$(CC) $(CFLAGS) $(SRC) ./getline.c -o $@  -lm -llibregex

toy: ./toybox/awk

# sed here reverts change made for clang warning about "string" + int
# toybox maintainer R. Landley prefers "string" + int over &("string"[int])
# also: second line of sed command reverts the anonymous union removal
# (i.e. restores the anonymous union in struct zvalue for toybox\awk.c)
./toybox/awk: ./onefile/wak.c
	@mkdir -p $(@D)
	awk -f ./scripts/make_toybox_awk.awk ./onefile/wak.c | \
		sed -e 's/\&(\("[^"]*"\)\[\([^]]*\)\]);.*/\1 + \2;/' \
		-e 's/\.u\././g' -e 's/->u./->/g' -e 's/} u;/};/' -e '/struct zvalue {/,/union/ s,union.*,union { // anonymous union not in C99; not going to fix it now.,' > $@.c

clean:
	-rm wak prwak aswak mwak muwak mmuwak

install: ./wak
	mkdir -p $(DESTDIR)$(prefix)/bin
	cp $? $(DESTDIR)$(prefix)/bin/
	chmod 775 $(DESTDIR)$(prefix)/bin/wak

check test: check-wak

# tests from gawk test set passed by gawk and nawk
# Beware busybox goes infinite loop output on rsnullre!
gawk_nawk= anchor backgsub closebad convfmt dynlj fieldassign \
forsimp fpat8 fscaret fsrs funlen gsubtest hex lc_num1 negrange \
nulrsend ofmta onlynl opasnslf printf1 rebuf rsnul1nl rsnullre \
rstest1 rstest5 rswhite sortfor2 splitwht substr tailrecurse \
unicode1 uparrfs wjposer1 addcomma anchgsub arrayind3 arrayprm3 \
arrayref arrymem1 arynasty arysubnm aryunasgn asgext \
assignnumfield assignnumfield2 clobber compare2 concat2 concat4 \
concat5 dfacheck2 dfastress divzero2 elemnew2 escapebrace exitval2 \
fldchg fldchgnf fldterm fmttest fsbs fsfwfs fstabplus funsemnl \
gensub3 getline3 getline4 getlnbuf getlnhd getnr2tb getnr2tm \
gsubtst8 igncdym ignrcas2 inpref intest intprec leaddig leadnl \
longsub longwrds manglprm match4 math mbprintf2 mbprintf3 mdim3 \
mdim4 mdim8 minusstr mmap8k mpfrfield mpfrnonum mpfrrem mtchi18n \
nasty nasty2 negexp nested nfloop nfset nlfldsep nlinstr nlstrina \
numindex numstr1 numsubstr octsub ofmt ofmtbig ofmtfidl ofmts \
ofmtstrnum ofs1 opasnidx paramtyp paramuninitglobal pcntplus \
pipeio1 prdupval prec printfbad3 printfchar profile12 prt1eval \
range1 regeq regexprange reginttrad reint reint2 reparse resplit \
rs rsstart1 rstest2 rstest4 rstest6 setrec0 setrec1 sigpipe1 \
spacere splitarr splitdef splitvar sprintfc strcat1 strfieldnum \
strnum1 strnum2 strsubscript subamp subi18n subsepnm subslash \
swaplns tweakfld uplus wideidx wideidx2 widesub widesub2 widesub3 \
zero2 zeroe0 zeroflag

# tests from gawk test set passed by gawk and wak; others fail some
gawk_more= arrayind1 arrayprm2 aryprm8 aryprm9 backsmalls2 concat1 \
concat3 crlf delarprm elemnew1 eofsplit exit2 fnarydel fnparydl \
fordel forref getline getline5 gsubtst2 gsubtst3 gsubtst4 inputred \
intarray iobug1 mdim5 mdim7 membug1 mpfrieee mpfrnegzero \
mpfrnegzero2 nfldstr noloop1 noloop2 nondec posix prmreuse \
prtoeval rri1 rstest3 shadowbuiltin splitargv stupid2 tradanch

# Remove >/dev/null on echo "PASS:..." to see what passes
check-wak:
	@echo "================= begin wak tests =================" ; \
	ntest=0; \
	npass=0; \
	nfail=0; \
	ntotal=`echo ${gawk_nawk} ${gawk_more} | wc -w`; \
	for f in ${gawk_nawk} ${gawk_more}; \
	do \
		fn=$(TESTDIR)/gawktests/$$f; \
		ntest=`expr $$ntest + 1`; \
	    printf "                             \r[%2d / %d] %s \r" $$ntest $$ntotal $$f ;					\
		awkfn=$$fn.awk; \
		infn=$$fn.in; \
		okfn=$$fn.ok; \
		outfn=$$fn.out; \
		if [ -e $$awkfn ] && [ -e $$okfn ]; \
		then \
			if [ -e $$infn ]; \
			then \
				$(AWK) -f $$awkfn $$infn   >$$outfn </dev/null || true; \
			else \
				$(AWK) -f $$awkfn          >$$outfn </dev/null || true; \
			fi ;\
			if cmp $$okfn $$outfn > /dev/null ; \
			then \
				npass=`expr $$npass + 1`; \
				rm -f $$outfn; \
				echo "PASS: $$awkfn" >/dev/null; \
			else \
				nfail=`expr $$nfail + 1`; \
				echo "FAIL: $$awkfn"; \
			fi ;\
		else \
			echo "NOTFOUND: $$awkfn"; \
		fi ; \
	done; \
	echo ;											\
	echo PASS: $$npass of $$ntest tests ;							\
	test $$nfail -ne 0 && echo FAIL: $$nfail of $$ntest tests ;				\
	echo ;											\
	test $$nfail -eq 0 && echo ALL TESTS PASSED! ;						\
	echo ;											\
	echo "cleaning up after tests..." ;		\
	rm seq test1 test2 errors.cleanup && true; \
	echo ;											\
	echo "===================== end test =====================" ;		\
	echo

