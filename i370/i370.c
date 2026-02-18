/* Subroutines for insn-output.c for System/370.
   Copyright (C) 1989, 1993, 1995, 1997, 1998, 1999, 2000, 2002
   Free Software Foundation, Inc.
   Contributed by Jan Stein (jan@cd.chalmers.se).
   Modified for OS/390 LanguageEnvironment C by Dave Pitts (dpitts@cozx.com)
   Modified for Linux-ELF/390 by Linas Vepstas (linas@linas.org) 

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"
#include "conditions.h"
#include "output.h"
#include "insn-attr.h"
#include "function.h"
#include "expr.h"
#include "flags.h"
#include "recog.h"
#include "toplev.h"
#include "cpplib.h"
#include "tm_p.h"
#include "target.h"
#include "target-def.h"

#include "version.h"
#include <ctype.h>

/* from gcc.h */
extern const char *input_filename;
extern size_t input_filename_length;


/* maximum length output line */
/* need to allow for people dumping specs */
#define MAX_LEN_OUT 2000

extern FILE *asm_out_file;

/* Label node.  This structure is used to keep track of labels 
      on the various pages in the current routine.
   The label_id is the numeric ID of the label,
   The label_page is the page on which it actually appears,
   The first_ref_page is the page on which the true first ref appears.
   The label_addr is an estimate of its location in the current routine,
   The label_first & last_ref are estimates of where the earliest and
      latest references to this label occur.  */

typedef struct label_node
  {
    struct label_node *label_next;
    int label_id;
    int label_page;
    int first_ref_page;

    int label_addr;
    int label_first_ref;
    int label_last_ref;
  }
label_node_t;

/* if we just inspected a label on another page, we want to
   record that */
static int just_referenced_page = -1;

/* Is 1 when a label has been generated and the base register must be reloaded.  */
int mvs_need_base_reload = 0;

/* Is 1 when an entry point is to be generated.  */
int mvs_need_entry = 0;

/* Is 1 if we have seen main() */
int mvs_gotmain = 0;

int mvs_need_to_globalize = 1;

/* Current function starting base page.  */
int function_base_page;

/* Length of the current page code.  */
int mvs_page_code;

/* Length of the current page literals.  */
int mvs_page_lit;

/* Length of case statement entries */
int mvs_case_code = 0;

/* The desired CSECT name, set by "-mcsect=" option */
char *mvs_csect_name = 0;

/* Current function name.  */
char *mvs_function_name = 0;

/* Current source module.  */
char *mvs_module = 0;

/* Current function name length.  */
int mvs_function_name_length = 0;

/* Page number for multi-page functions.  */
int mvs_page_num = 0;

/* Label node list anchor.  */
static label_node_t *label_anchor = 0;

/* Label node free list anchor.  */
static label_node_t *free_anchor = 0;

/* Assembler source file descriptor.  */
static FILE *assembler_source = 0;

/* Flag that enables position independent code */
int i370_enable_pic = 1;

static label_node_t * mvs_get_label PARAMS ((int));
static void i370_label_scan PARAMS ((void));
#ifdef TARGET_HLASM
static bool i370_hlasm_assemble_integer PARAMS ((rtx, unsigned int, int));
#endif
static void i370_output_function_prologue PARAMS ((FILE *, HOST_WIDE_INT));
static void i370_output_function_epilogue PARAMS ((FILE *, HOST_WIDE_INT));
#ifdef TARGET_ALIASES
static int mvs_hash_alias PARAMS ((const char *));
#endif

/* ===================================================== */
/* defines and functions specific to the HLASM assembler */
#ifdef TARGET_HLASM

#define MVS_HASH_PRIME 999983
#if 1 /*defined(HOST_EBCDIC)*/
#define MVS_SET_SIZE 256
#else
#define MVS_SET_SIZE 128
#endif

#ifndef MAX_MVS_LABEL_SIZE
#define MAX_MVS_LABEL_SIZE 8
#endif

#define MAX_LONG_LABEL_SIZE 255

/* Alias node, this structure is used to keep track of aliases to external
   variables. The IBM assembler allows an alias to an external name 
   that is longer that 8 characters; but only once per assembly.
   Also, this structure stores the #pragma map info.  */
typedef struct alias_node {
	struct alias_node *alias_next;
	int  alias_emitted;
	int  alias_used;
	char alias_name [MAX_MVS_LABEL_SIZE + 1];
	char real_name [MAX_LONG_LABEL_SIZE + 1];
} alias_node_t;

/* Alias node list anchor.  */
static alias_node_t *alias_anchor = 0;

#ifdef TARGET_LE
/* Define the length of the internal MVS function table.  */
#define MVS_FUNCTION_TABLE_LENGTH 32

/* C/370 internal function table.  These functions use non-standard linkage
   and must handled in a special manner.  */
static const char *const mvs_function_table[MVS_FUNCTION_TABLE_LENGTH] =
{
#if defined(HOST_EBCDIC) /* Changed for EBCDIC collating sequence */
   "ceil",     "edc_acos", "edc_asin", "edc_atan", "edc_ata2", "edc_cos",
   "edc_cosh", "edc_erf",  "edc_erfc", "edc_exp",  "edc_gamm", "edc_lg10",
   "edc_log",  "edc_sin",  "edc_sinh", "edc_sqrt", "edc_tan",  "edc_tanh",
   "fabs",     "floor",    "fmod",     "frexp",    "hypot",    "jn",
   "j0",       "j1",       "ldexp",    "modf",     "pow",      "yn",
   "y0",       "y1"
#else
   "ceil",     "edc_acos", "edc_asin", "edc_ata2", "edc_atan", "edc_cos",
   "edc_cosh", "edc_erf",  "edc_erfc", "edc_exp",  "edc_gamm", "edc_lg10",
   "edc_log",  "edc_sin",  "edc_sinh", "edc_sqrt", "edc_tan",  "edc_tanh",
   "fabs",     "floor",    "fmod",     "frexp",    "hypot",    "j0",
   "j1",       "jn",       "ldexp",    "modf",     "pow",      "y0",
   "y1",       "yn"
#endif
};
#endif /* TARGET_LE */

#endif /* TARGET_HLASM */

/* Initialize the GCC target structure.  */
#ifdef TARGET_HLASM
#undef TARGET_ASM_BYTE_OP
#define TARGET_ASM_BYTE_OP NULL
#undef TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP NULL
#undef TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP NULL
#undef TARGET_ASM_INTEGER
#define TARGET_ASM_INTEGER i370_hlasm_assemble_integer
#endif

#undef TARGET_ASM_FUNCTION_PROLOGUE
#define TARGET_ASM_FUNCTION_PROLOGUE i370_output_function_prologue
#undef TARGET_ASM_FUNCTION_EPILOGUE
#define TARGET_ASM_FUNCTION_EPILOGUE i370_output_function_epilogue

struct gcc_target targetm = TARGET_INITIALIZER;


#define CURRFUNC (IDENTIFIER_POINTER (DECL_NAME (current_function_decl)))


/* ===================================================== */
/* This is our last chance to clean up before starting to compile.
   We do this to fix up some initializations.   */

void
i370_override_options (void)
{
	static char buf[9];
	char 		*p;

	i370_enable_pic = flag_pic;

	if (mvs_csect_name) {
		/* "-mcsect=" option was specified by user */

		if (mvs_csect_name[0] > ' ') {
			/* user supplied a CSECT name so we'll use it */
			strncpy(buf, mvs_csect_name, 8);
		}
		else {
			/* user did not supply a csect name, use input file name */
			p = strrchr(input_filename, '/');
			if (!p) p = strrchr(input_filename, '\\');
			if (p) 
				p++;	/* skip the '/' or '\\' */
			else 
				p = (char*)(input_filename);
			
			strncpy(buf, p, 8);
		}

		/* fold csect name to upper case */
		buf[8] = 0;	/* make sure we're 0 byte terminated */
		p = buf;
		while (*p != '\0') {
			switch (*p) {
				case '.':
					*p = 0;	/* truncate name here */
					break;
				case '_':
					*p = '@'; /* map '_' to '@' character */
					break;
				default:
					/* fold to upper case */
					*p = toupper((unsigned char)*p);
					break;
			}
			p++;
		}

		/* set mvs_csect_name to our static buffer */
		mvs_csect_name = buf;
	}

#ifdef TARGET_LINUX
	/* Override CALL_USED_REGISTERS & FIXED_REGISTERS 
	** PIC requires r12, otherwise its free 
	*/
	if (i370_enable_pic) {
		fix_register ("r12", 1, 1);
	}
	else {
		fix_register ("r12", 0, 0);
	}
#endif /* TARGET_LINUX */

}

/* ===================================================== */
/* The following three routines are used to determine whther 
   forward branch is on this page, or is a far jump.  We use
   the "length" attr on an insn [(set_atter "length" "4")]
   to store the largest possible code length that insn
   could have.  This gives us a hint of the address of a
   branch destination, and from that, we can work out 
   the length of the jump, and whether its on page or not. 
 */

/* Return the destination address of a branch.  */

int
i370_branch_dest (rtx branch)
{
	rtx dest = SET_SRC (PATTERN (branch));
	int dest_uid;
	int dest_addr;

	/* first, compute the estimated address of the branch target */
	if (GET_CODE (dest) == IF_THEN_ELSE)
		dest = XEXP (dest, 1);

	dest = XEXP (dest, 0);
	dest_uid = INSN_UID (dest);
	dest_addr = INSN_ADDRESSES (dest_uid);

	/* next, record the address of this insn as the true addr of first ref */
	{
		label_node_t *lp;
		rtx label = JUMP_LABEL (branch);
		int labelno = CODE_LABEL_NUMBER (label);

		if (!label || CODE_LABEL != GET_CODE (label)) abort ();

		lp = mvs_get_label (labelno);
		if (-1 == lp->first_ref_page) lp->first_ref_page = mvs_page_num;
		just_referenced_page = lp->label_page;
	}
  
	return dest_addr;
}

int
i370_branch_length (rtx insn)
{
	int here, there;
  
	here = INSN_ADDRESSES (INSN_UID (insn));
	there = i370_branch_dest (insn);

	return (there - here);
}

int
i370_short_branch (rtx insn)
{
	int base_offset;

	base_offset = i370_branch_length(insn);
  
	/* If we just referenced something off-page, then you can
	** forget about doing a short branch to it! So for backward
	** references, we'll have a page number and can see that it is
	** different. For forward references, the page number isn't
	** available yet (ie it's still set to -1), so don't use
	** this logic on them. 
	*/
	if ((just_referenced_page != mvs_page_num) 
		&& (just_referenced_page != -1)) {
		return 0;
	}
  
	if (0 > base_offset) {
		base_offset += mvs_page_code;
	} 
	else {
		/* avoid bumping into lit pool; use 2x to estimate max possible lits */
		base_offset *= 2;
		base_offset += mvs_page_code + mvs_page_lit;
	}
  
	/* make a conservative estimate of room left on page */
	if ((MVS_PAGE_CONSERVATIVE >base_offset) && ( 0 < base_offset)) return 1;

	return 0;
}

/* The i370_label_scan() routine is supposed to loop over
   all labels and label references in a compilation unit,
   and determine whether all label refs appear on the same 
   code page as the label. If they do, then we can avoid 
   a reload of the base register for that label.
  
   Note that the instruction addresses used here are only 
   approximate, and make the sizes of the jumps appear
   farther apart then they will actually be.  This makes 
   this code far more conservative than it needs to be.
 */

#define I370_RECORD_LABEL_REF(label,addr) {				\
	label_node_t *lp;						\
	int labelno = CODE_LABEL_NUMBER (label);			\
	lp = mvs_get_label (labelno);					\
	if (addr < lp -> label_first_ref) lp->label_first_ref = addr;	\
	if (addr > lp -> label_last_ref) lp->label_last_ref = addr;	\
}

/* there is no label for this jump so this had better be a ADDR_VEC 
** or an ADDR_DIFF_VEC and there had better be a vector of labels.  
** called by i370_label_scan(). 
*/
static void
i370_label_scan_jump_nolabel(rtx insn, int *tablejump_offset, int *here)
{
	int j;
	rtx body = PATTERN (insn);
	rtx label = JUMP_LABEL (insn);
				
	if (ADDR_VEC == GET_CODE(body)) {
		for (j=0; j < XVECLEN (body, 0); j++) {
			rtx lref = XVECEXP (body, 0, j);
						
			if (LABEL_REF != GET_CODE (lref)) abort ();
						
			label = XEXP (lref,0);
			if (CODE_LABEL != GET_CODE (label)) abort ();
						
			*tablejump_offset += 4;
			*here += 4;
			I370_RECORD_LABEL_REF(label,*here);
		}
		/* finished with the vector go do next insn */
		return;
	}

	if (ADDR_DIFF_VEC == GET_CODE(body)) {
/* XXX hack alert.
   Right now, we leave this as a no-op, but strictly speaking,
   this is incorrect.  It is possible that a table-jump
   driven off of a relative address could take us off-page,
   to a place where we need to reload the base reg.  So really,
   we need to examing both labels, and compare thier values
   to the current basereg value.
  
   More generally, this brings up a troubling issue overall:
   what happens if a tablejump is split across two pages? I do 
   not beleive that this case is handled correctly at all, and
   can only lead to horrible results if this were to occur.
  
   However, the current situation is not any worse than it was 
   last week, and so we punt for now.  */

		debug_rtx (insn);
		for (j=0; j < XVECLEN (body, 0); j++) ;

		/* finished with the vector go do next insn */
		return;
	}

/* XXX hack alert.
   Compiling the exception handling (L_eh) in libgcc2.a will trip
   up right here, with something that looks like
   (set (pc) (mem:SI (plus:SI (reg/v:SI 1 r1) (const_int 4))))
      {indirect_jump} 
   I'm not sure of what leads up to this, but it looks like
   the makings of a long jump which will surely get us into trouble
   because the base & page registers don't get reloaded.  For now
   I'm not sure of what to do ... again we punt ... we are not worse
   off than yesterday.  */

	/* print_rtl_single (stdout, insn); */
    /* debug_rtx (insn); */
    /* abort(); */
}

static void 
i370_label_scan (void) 
{
	rtx insn;
	label_node_t *lp;
	int tablejump_offset = 0;

	for (insn = get_insns(); insn; insn = NEXT_INSN(insn)) {
		int here = INSN_ADDRESSES (INSN_UID (insn));
		enum rtx_code code = GET_CODE(insn);

		/* ??? adjust for tables embedded in the .text section that
		** the compiler didn't take into account 
		*/
		here += tablejump_offset;
		INSN_ADDRESSES (INSN_UID (insn)) = here;

		/* check to see if this insn is a label ...  */
		if (CODE_LABEL == code) {
			int labelno = CODE_LABEL_NUMBER (insn);

			lp = mvs_get_label (labelno);
			lp->label_addr = here;
#if 0
			/* Supposedly, labels are supposed to have circular
              lists of label-refs that reference them, 
              setup in flow.c, but this does not appear to be the case.  */
           rtx labelref = LABEL_REFS (insn);
           rtx ref = labelref;
           do 
             {
               rtx linsn = CONTAINING_INSN(ref);
               ref =  LABEL_NEXTREF(ref);
             } while (ref && (ref != labelref));
#endif
			continue;
		} /* if (CODE_LABEL == code) { */

		if (JUMP_INSN == code) {
			rtx label = JUMP_LABEL (insn);

			if (!label) {
				/* no label for this jump_insn */
				i370_label_scan_jump_nolabel(insn, &tablejump_offset, &here);
			}
			else {
				/* At this point, this jump_insn had better be a plain-old
				** ordinary one, grap the label id and go 
				*/
				if (CODE_LABEL != GET_CODE (label)) abort ();

				I370_RECORD_LABEL_REF(label,here);
			}
			continue;
		} /* if (JUMP_INSN == code) { */

		/* Sometimes, we take addresses of labels and use them
		** as instruction operands ... these show up as REG_NOTES 
		*/
		if (INSN == code) {
			if ('i' == GET_RTX_CLASS (code)) {
				rtx note;

				for (note = REG_NOTES (insn); note;  note = XEXP(note,1)) {
					if (REG_LABEL == REG_NOTE_KIND(note)) {
						rtx label = XEXP (note,0);

						if (!label || CODE_LABEL != GET_CODE (label)) abort ();

						I370_RECORD_LABEL_REF(label,here);
					}
				}
			}
			continue;
		} /* if (INSN == code) { */
	} /* for (insn = get_insns(); insn; insn = NEXT_INSN(insn)) { */

}

/* ===================================================== */

/* Emit reload of base register if indicated.  This is to eliminate multiple
   reloads when several labels are generated pointing to the same place
   in the code.  

   The table of base register values is created at the end of the function.
   The MVS/OE/USS/HLASM version keeps this table in the text section, and
   it looks like the following:
      PGT0 EQU *
      DC A(PG0)
      DC A(PG1)
   
   The ELF version keeps the base register table in either the text or the 
   data section, depending on the setting of the i370_enable_pic flag.
   Disabling this flag frees r12 for general purpose use, but makes the 
   code non-relocatable.  The non-pic table resemble the mvs-style table.
   The pic table stores values for both r3 (the register used for branching)
   and r12 (the register to index the literal pool, also in the data section).
   Thus, the ELF pic version has twice as many entries, and double the offset.

     .LPGT0:          // PGT0 EQU *
     .long .LPG0      // DC A(PG0)
     .long .LPOOL0     
     .long .LPG1      // DC A(PG1)
     .long .LPOOL1     

  Note that the functin prologue loads the page addressing register:
      L       PAGE_REGISTER,=A(.LPGT0)

  The ELF version then stores this value at 0(r13), so that its always
  accessible. This frees up r4 for general register allocation; whereas
  the MVS version is stuck with r4.

  Note that this addressing scheme breaks down when a single subroutine
  has more than twelve MBytes of code or so for non-pic, and 6MB for pic.
  Its hard to imagine under what circumstances a single subroutine would 
  ever get that big ...
 */

#ifdef TARGET_HLASM
void
check_label_emit ()
{
	if (mvs_need_base_reload) {
		mvs_need_base_reload = 0;
		mvs_page_code += 4;
		fprintf (assembler_source, "\tL\t%d,%d(,%d)\n",
			BASE_REGISTER, (mvs_page_num - function_base_page) * 4,
			PAGE_REGISTER);
	}
}
#endif /* TARGET_HLASM */

#ifdef TARGET_LINUX
void
check_label_emit ()
{
  if (mvs_need_base_reload)
    {
      mvs_need_base_reload = 0;

      if (i370_enable_pic) 
        {
          mvs_page_code += 12;
          fprintf (assembler_source, "\tL\tr3,0(,r13)\n");
          fprintf (assembler_source, "\tL\tr%d,%d(,r3)\n",
              PIC_BASE_REGISTER, ((mvs_page_num - function_base_page) * 8 +4));
          fprintf (assembler_source, "\tL\tr3,%d(,r3)\n",
              (mvs_page_num - function_base_page) * 8);
        }
      else
        {
          mvs_page_code += 8;
          fprintf (assembler_source, "\tL\tr3,0(,r13)\n");
          fprintf (assembler_source, "\tL\tr3,%d(,r3)\n",
              ((mvs_page_num - function_base_page) * 4));
        }
    }
}
#endif /* TARGET_LINUX */

/* Add the label to the current page label list.  If a free element is available
   it will be used for the new label.  Otherwise, a label element will be
   allocated from memory.
   ID is the label number of the label being added to the list.  */

static label_node_t *
mvs_get_label (int id)
{
	label_node_t *lp;

	/* first, lets see if we already go one, if so, use that.  */
	for (lp = label_anchor; lp; lp = lp->label_next) {
		if (lp->label_id == id) return lp;
	}

	/* not found, get a new one */
	if (free_anchor) {
		lp = free_anchor;
		free_anchor = lp->label_next;
	}
	else {
		lp = (label_node_t *) xmalloc (sizeof (label_node_t));
	}

	/* initialize for new label */
	lp->label_id = id;
	lp->label_page = -1;
	lp->label_next = label_anchor;
	lp->label_first_ref = 2000123123;
	lp->label_last_ref = -1;
	lp->label_addr = -1;
	lp->first_ref_page = -1;
	label_anchor = lp;

	return lp;
}

void
mvs_add_label (int id)
{
	label_node_t 	*lp;
	int 			fwd_distance;

	lp = mvs_get_label (id);
	lp->label_page = mvs_page_num;

#if 1
	/* Note that without this, some case statements are
	** not generating correct code, e.g. case '{' in
    ** do_spec_1 in gcc.c 
    */
	if (mvs_page_num != function_base_page) {
		mvs_need_base_reload ++;
		return;
	}
#endif

	/* OK, we just saw the label.  Determine if this label
	** needs a reload of the base register 
	*/
	if ((-1 != lp->first_ref_page) && 
		(lp->first_ref_page != mvs_page_num)) {
		/* Yep; the first label_ref was on a different page.  */
		mvs_need_base_reload ++;
		return;
	}

	/* Hmm.  Try to see if the estimated address of the last
	** label_ref is on the current page.  If it is, then we
	** don't need a base reg reload.  Note that this estimate
	** is very conservatively handled; we'll tend to have 
	** a good bit more reloads than actually needed.  Someday,
	** we should tighten the estimates (which are driven by
	** the (set_att "length") insn attibute.
	**
	** Currently, we estimate that number of page literals 
	** same as number of insns, which is a vast overestimate,
	** esp that the estimate of each insn size is its max size.  
	*/

	/* if latest ref comes before label, we are clear */
	if (lp->label_last_ref < lp->label_addr) return;

	fwd_distance = lp->label_last_ref - lp->label_addr;

	if (mvs_page_code + 2 * fwd_distance + mvs_page_lit < MVS_PAGE_CONSERVATIVE)
		return;

	mvs_need_base_reload ++;
}

/* Check to see if the label is in the list and in the current
   page.  If not found, we have to make worst case assumption 
   that label will be on a different page, and thus will have to
   generate a load and branch on register.  This is rather
   ugly for forward-jumps, but what can we do? For backward
   jumps on the same page we can branch directly to address.
   ID is the label number of the label being checked.  */

int
mvs_check_label (int id)
{
	label_node_t *lp;

	for (lp = label_anchor; lp; lp = lp->label_next) {
		if (lp->label_id == id) {
			if (lp->label_page == mvs_page_num) {
				return 1;
			} 
			else {
				return 0;
			} 
		}
	}

	return 0;
}

/* Get the page on which the label sits.  This will be used to 
   determine is a register reload is really needed.  */

#if 0
int
mvs_get_label_page(int id)
{
  label_node_t *lp;

  for (lp = label_anchor; lp; lp = lp->label_next)
    {
      if (lp->label_id == id)
	return lp->label_page;
    }
  return -1;
}
#endif

/* The label list for the current page freed by linking the list onto the free
   label element chain.  */

void
mvs_free_label_list (void)
{
	if (label_anchor) {
		label_node_t *last_lp = label_anchor;

		while (last_lp->label_next) last_lp = last_lp->label_next;

		last_lp->label_next = free_anchor;
		free_anchor = label_anchor;
	}

	label_anchor = 0;
}

/* Convert a float to a printable form.  */

char *
mvs_make_float (REAL_VALUE_TYPE r)
{
	char *p;
	static char buf[50];

	REAL_VALUE_TO_DECIMAL (r, "%.9G", buf);

	for (p = buf; *p; p++)
		if (ISLOWER(*p)) *p = TOUPPER(*p);

	if ((p = strrchr (buf, 'E')) != NULL) {
		char *t = p;
		
		for (p--; *p == '0'; p--) ;
		
		if (*p == '.') p++;

		p++;
		memmove (p, t, strlen(t) + 1);
	}

	return (buf);
}

/* ====================================================================== */
/* If the page size limit is reached a new code page is started, and the base
   register is set to it.  This page break point is counted conservatively,
   most literals that have the same value are collapsed by the assembler.
   True is returned when a new page is started.
   FILE is the assembler output file descriptor.
   CODE is the length, in bytes, of the instruction to be emitted.
   LIT is the length of the literal to be emitted.  */

int
mvs_check_page (FILE *file, int code, int lit)
{
	if (file) assembler_source = file;

	if (mvs_page_code + code + mvs_page_lit + lit > MAX_MVS_PAGE_LENGTH) {
		/* no need to dump literals if we're at the end of
		** a case statement - they will already have been 
		** dumped prior to the jump table generation. 
		*/
		if (mvs_case_code == 0) {
			fprintf(assembler_source, "\tB\t@@PGE%d\n", mvs_page_num);
			fprintf(assembler_source, "\tDS\t0F\n");
			fprintf(assembler_source, "\tLTORG\n");
		}

		fprintf(assembler_source, "\tDS\t0F\n");
		fprintf(assembler_source, "@@PGE%d\tEQU\t*\n", mvs_page_num);
		fprintf(assembler_source, "\tDROP\t%d\n", BASE_REGISTER);
		mvs_page_num++;
		fprintf(assembler_source, "\tBALR\t%d,0\n", BASE_REGISTER);
		fprintf(assembler_source, "\tUSING\t*,%d\n", BASE_REGISTER);
		fprintf(assembler_source, "@@PG%d\tEQU\t*\n", mvs_page_num);
		mvs_page_code = code;
		mvs_page_lit = lit;
		return 1;
	}

	mvs_page_code += code;
	mvs_page_lit += lit;
	return 0;
}

/* ===================================================== */
/* defines and functions specific to the HLASM assembler */
#ifdef TARGET_HLASM

int
mvs_function_check (const char *name)
{
	return 0;
}

/* Generate a hash for a given key.  */

#ifdef TARGET_ALIASES
static int
mvs_hash_alias (key)
     const char *key;
{
  int h;
  int i;
  int l = strlen (key);

  h = (unsigned char) MAP_OUTCHAR(key[0]);
  for (i = 1; i < l; i++)
    h = ((h * MVS_SET_SIZE) + (unsigned char) MAP_OUTCHAR(key[i])) % MVS_HASH_PRIME;
  return (h);
}
#endif

/* Add the alias to the current alias list.  */

void
mvs_add_alias (const char *realname, const char *aliasname, int emitted)
{
	alias_node_t *ap;

#ifdef DEBUG
	printf("mvs_add_alias: realname(%d) = '%s'\n", strlen(realname), realname);
	printf("   aliasname(%d) = '%s'\n", strlen(aliasname), aliasname);
	printf("   emitted = %d\n", emitted);
#endif

	ap = (alias_node_t *) xmalloc (sizeof (alias_node_t));
	if (strlen(realname) > MAX_LONG_LABEL_SIZE) {
		warning("real name is too long - alias ignored");
		return;
	}

	if (strlen(aliasname) > MAX_MVS_LABEL_SIZE) {
		warning("alias name is too long - alias ignored");
		return;
	}

	strcpy (ap->real_name, realname);
	strcpy (ap->alias_name, aliasname);
	ap->alias_emitted = emitted;
	ap->alias_used = 0 /* FALSE */;
	ap->alias_next = alias_anchor;
	alias_anchor = ap;
}

/* Check to see if the name needs aliasing. ie. the name is either:
     1. Longer than 8 characters
     2. Contains an underscore
     3. Is mixed case */

int
mvs_need_alias (const char *realname)
{
	int i, j = strlen (realname);

#ifdef DEBUG
	printf("mvs_need_alias: realname(%d) = '%s'\n", strlen(realname), realname);
#endif

#if defined(TARGET_DIGNUS) || defined(TARGET_PDPMAC)
	return 1;
#else
	if (mvs_function_check (realname)) return 0;

	if (j > MAX_MVS_LABEL_SIZE)	return 1;

	if (strchr (realname, '_') != 0) return 1;

	if (ISUPPER (realname[0])) {
		for (i = 1; i < j; i++) {
			if (ISLOWER (realname[i])) return 1;
		}
	}
	else {
		for (i = 1; i < j; i++) {
			if (ISUPPER (realname[i])) return 1;
		}
	}

	return 0;
#endif
}

/* Mark an alias as used as an external.  */
int
mvs_mark_alias (const char *realname)
{
	alias_node_t *ap;

#ifdef DEBUG
	printf("mvs_mark_alias: realname(%d) = '%s'\n", strlen(realname), realname);
#endif

	for (ap = alias_anchor; ap; ap = ap->alias_next) {
		if (!strcmp (ap->real_name, realname)) {
			ap->alias_used = 1;
			return 0;
		}
	}
	
	return 1;
}

/* Dump any used aliases that have been emitted.  */
int
mvs_dump_alias(FILE *f)
{
	alias_node_t *ap;

#ifdef DEBUG
	printf("mvs_dump_alias: \n");
#endif

	for (ap = alias_anchor; ap; ap = ap->alias_next) {
		if (ap->alias_used && !ap->alias_emitted) {
			fprintf (f, "%s\tALIAS\tC'%s'\n",
				ap->alias_name,	ap->real_name);
		}
	}

	return 0;
}

/* Get the alias from the list. 
   If 1 is returned then it's in the alias list, 0 if it was not */

int
mvs_get_alias (const char *realname, char *aliasname)
{
	alias_node_t *ap;

#ifdef DEBUG
	printf("mvs_get_alias: realname(%d) = '%s'\n", strlen(realname), realname);
#endif

#ifdef TARGET_ALIASES

	for (ap = alias_anchor; ap; ap = ap->alias_next) {
		if (!strcmp (ap->real_name, realname)) {
			strcpy (aliasname, ap->alias_name);
			return 1;
		}
	}

	if (mvs_need_alias (realname)) {
		char c1, c2;

		c1 = realname[0];
		c2 = realname[1];
		if (ISLOWER (c1)) c1 = TOUPPER (c1);
		else if (c1 == '_') c1 = 'A';

		if (ISLOWER (c2)) c2 = TOUPPER (c2);
		else if (c2 == '_' || c2 == '\0') c2 = '#';

		sprintf (aliasname, "%c%c%06d", c1, c2, mvs_hash_alias (realname));
		mvs_add_alias (realname, aliasname, 0);
		return 1;
	}
#else
	if (strlen (realname) > MAX_MVS_LABEL_SIZE) {
		strncpy (aliasname, realname, MAX_MVS_LABEL_SIZE);
		aliasname[MAX_MVS_LABEL_SIZE] = '\0';
		return 1;
	}
#endif
	return 0;
}

/* Check to see if the alias is in the list. 
   If 1 is returned then it's in the alias list, 2 it was emitted  */

int
mvs_check_alias (const char *realname, char *aliasname)
{
	alias_node_t *ap;

#ifdef DEBUG
	printf("mvs_check_alias: realname(%d) = '%s'\n", strlen(realname), realname);
#endif

#ifdef TARGET_ALIASES

	for (ap = alias_anchor; ap; ap = ap->alias_next) {
		if (!strcmp (ap->real_name, realname)) {
			int rc = (ap->alias_emitted == 1) ? 1 : 2; 

			strcpy (aliasname, ap->alias_name);
			ap->alias_emitted = 1; 
			return rc;
		}
	}

	if (mvs_need_alias (realname)) {
		char c1, c2;

		c1 = realname[0];
		c2 = realname[1];
		
		if (ISLOWER (c1)) c1 = TOUPPER (c1);
		else if (c1 == '_') c1 = 'A';

		if (ISLOWER (c2)) c2 = TOUPPER (c2);
		else if (c2 == '_' || c2 == '\0') c2 = '#';

		sprintf (aliasname, "%c%c%06d", c1, c2, mvs_hash_alias (realname));
		mvs_add_alias (realname, aliasname, 0);
		alias_anchor->alias_emitted = 1;
		return 2;
	}
#else
	if (strlen (realname) > MAX_MVS_LABEL_SIZE) {
		strncpy (aliasname, realname, MAX_MVS_LABEL_SIZE);
		aliasname[MAX_MVS_LABEL_SIZE] = '\0';
		return 1;
	}
#endif

	return 0;
}

#endif /* TARGET_HLASM */

/* ===================================================== */
/* ===================================================== */
/* defines and functions specific to the gas assembler */
#ifdef TARGET_LINUX

/* Check for C/370 runtime function, they don't use standard calling
   conventions.  True is returned if the function is in the table.
   NAME is the name of the current function.  */
/* no special calling conventions (yet ??) */

int
mvs_function_check (name)
     const char *name ATTRIBUTE_UNUSED;
{
   return 0;
}

#endif /* TARGET_LINUX */
/* ===================================================== */


/* Return 1 if OP is a valid S operand for an RS, SI or SS type instruction.
   OP is the current operation.
   MODE is the current operation mode.  */

int
s_operand (register rtx op, enum machine_mode mode)
{
	extern int volatile_ok;
	register enum rtx_code code = GET_CODE (op);

	if (CONSTANT_ADDRESS_P (op)) return 1;

	if (mode == VOIDmode || GET_MODE (op) != mode) return 0;

	if (code == MEM) {
		register rtx x = XEXP (op, 0);

		if (!volatile_ok && op->volatil) return 0;

		if (REG_P (x) && REG_OK_FOR_BASE_P (x))	return 1;

		if (GET_CODE (x) == PLUS
			&& REG_P (XEXP (x, 0)) 
			&& REG_OK_FOR_BASE_P (XEXP (x, 0))
			&& GET_CODE (XEXP (x, 1)) == CONST_INT
			&& (unsigned) INTVAL (XEXP (x, 1)) < 4096) return 1;
	}

	return 0;
}


/* Return 1 if OP is a valid R or S operand for an RS, SI or SS type
   instruction.
   OP is the current operation.
   MODE is the current operation mode.  */

int
r_or_s_operand (register rtx op, enum machine_mode mode)
{
	extern int volatile_ok;
	register enum rtx_code code = GET_CODE (op);

	if (CONSTANT_ADDRESS_P (op)) return 1;

	if (mode == VOIDmode || GET_MODE (op) != mode) return 0;

	if (code == REG) return 1;
	else if (code == MEM) {
		register rtx x = XEXP (op, 0);

		if (!volatile_ok && op->volatil) return 0;

		if (REG_P (x) && REG_OK_FOR_BASE_P (x))	return 1;

		if (GET_CODE (x) == PLUS
			&& REG_P (XEXP (x, 0)) 
			&& REG_OK_FOR_BASE_P (XEXP (x, 0))
			&& GET_CODE (XEXP (x, 1)) == CONST_INT
			&& (unsigned) INTVAL (XEXP (x, 1)) < 4096) return 1;
	}

	return 0;
}


/* Some remarks about unsigned_jump_follows_p():
   gcc is built around the assumption that branches are signed
   or unsigned, whereas the 370 doesn't care; its the compares that
   are signed or unsigned.  Thus, we need to somehow know if we
   need to do a signed or an unsigned compare, and we do this by 
   looking ahead in the instruction sequence until we find a jump.
   We then note whether this jump is signed or unsigned, and do the 
   compare appropriately.  Note that we have to scan ahead indefinitley,
   as the gcc optimizer may insert any number of instructions between 
   the compare and the jump.
  
   Note that using conditional branch expanders seems to be be a more 
   elegant/correct way of doing this.   See, for instance, the Alpha 
   cmpdi and bgt patterns.  Note also that for the i370, various
   arithmetic insn's set the condition code as well.

   The unsigned_jump_follows_p() routine  returns a 1 if the next jump 
   is unsigned.  INSN is the current instruction. We err on the side
   of assuming unsigned, so there are a lot of return 1. */

int
unsigned_jump_follows_p (register rtx insn)
{
	rtx orig_insn = insn;

	while (1) {
		register rtx tmp_insn;
		enum rtx_code coda;
  
		insn = NEXT_INSN (insn);
		if (!insn) return (1);
  
		if (GET_CODE (insn) != JUMP_INSN) continue;
    
		tmp_insn = PATTERN (insn);
		if (!tmp_insn) continue;

		if (GET_CODE (tmp_insn) != SET) continue;
    
		if (GET_CODE (XEXP (tmp_insn, 0)) != PC) continue;
    
		tmp_insn = XEXP (tmp_insn, 1);
		if (GET_CODE (tmp_insn) != IF_THEN_ELSE) continue;
    
		/* if we got to here, this instruction is a jump.  Is it signed? */
		tmp_insn = XEXP (tmp_insn, 0);
		coda = GET_CODE (tmp_insn);
  
		/* if we get an equal or not equal, either comparison
		** will work. What we're really interested in what happens
		** after that. So check one more instruction to see if
		** anything comes up. 
		*/
		if ((coda == EQ) || (coda == NE)) {
			insn = NEXT_INSN (insn);
			if (!insn) return (1);
  
			if (GET_CODE (insn) != JUMP_INSN) {
				/* skip any labels or notes or non-branching
				** instructions, looking to see if there's a
				** branch ahead 
				*/
				while (GET_CODE (insn) != JUMP_INSN) {
					if ((GET_CODE (insn) != CODE_LABEL)
						&& (GET_CODE (insn) != NOTE)
						&& (GET_CODE (insn) != INSN)
						&& (GET_CODE (insn) != JUMP_INSN)) return (1);

					insn = NEXT_INSN (insn);
					if (!insn) return (1);
				}
			}
    
			tmp_insn = PATTERN (insn);
			if (!tmp_insn) continue;

			if (GET_CODE (tmp_insn) != SET) return (1);
    
			if (GET_CODE (XEXP (tmp_insn, 0)) != PC) return (1);
    
			tmp_insn = XEXP (tmp_insn, 1);
			if (GET_CODE (tmp_insn) != IF_THEN_ELSE) return (1);
    
			tmp_insn = XEXP (tmp_insn, 0);
			coda = GET_CODE (tmp_insn);
		}

		/* if we got to here, this instruction is a jump.  Is it signed? */
		return coda != GE && coda != GT && coda != LE && coda != LT;
	}
}

/* Target hook for assembling integer objects.  This version handles all
   objects when TARGET_HLASM is defined.  */

static bool
i370_hlasm_assemble_integer (rtx x, unsigned int size, int aligned_p)
{
	const char *int_format = NULL;
	int intmask;

	if (aligned_p) {
		switch (size) {
			case 1:
				int_format = "\tDC\tX'%02X'\n";
				intmask = 0xFF;
				break;

			case 2:
				int_format = "\tDC\tX'%04X'\n";
				intmask = 0xFFFF;
				break;

			case 4:
				if (GET_CODE (x) == CONST_INT) {
					fputs ("\tDC\tF'", asm_out_file);
					output_addr_const (asm_out_file, x);
					fputs ("'\n", asm_out_file);
				}
				else {
					if (GET_CODE (x) == CONST			
						&& GET_CODE (XEXP (XEXP (x, 0), 0)) == SYMBOL_REF	
						&& SYMBOL_REF_FLAG (XEXP (XEXP (x, 0), 0))) {
						const char *fname;
						typedef struct _entnod { 
							char *data; 
							struct _entnod *next; 
						} entnod;
						static entnod *enstart = NULL;
						entnod **en;
                
						fname = XSTR((XEXP (XEXP (x, 0), 0)), 0);
						en = &enstart;
						while (*en != NULL) {
							if (strcmp((*en)->data, fname) == 0) break;
							en = &((*en)->next);
						}
						if (*en == NULL) {
							*en = xmalloc(sizeof(entnod));
							(*en)->data = xmalloc(strlen(fname) + 1);
							strcpy((*en)->data, fname);
							(*en)->next = NULL;

							fputs ("\tEXTRN\t", asm_out_file);
							assemble_name(asm_out_file, 
								XSTR((XEXP (XEXP (x, 0), 0)), 0));
							fputs ("\n", asm_out_file);
						}                
					}

					if (SYMBOL_REF_FLAG(x)) {
						fputs ("\tDC\tV(", asm_out_file);
					}
					else {
						fputs ("\tDC\tA(", asm_out_file);
					}

					output_addr_const (asm_out_file, x);
					fputs (")\n", asm_out_file);
				}
				
				return true;
		}
	}
	
	if (int_format && GET_CODE (x) == CONST_INT) {
		fprintf (asm_out_file, int_format, INTVAL (x) & intmask);
		return true;
	}

	return default_assemble_integer (x, size, aligned_p);
}

/* Generate the assembly code for function entry.  FILE is a stdio
   stream to output the code to.  SIZE is an int: how many units of
   temporary storage to allocate.

   Refer to the array `regs_ever_live' to determine which registers to
   save; `regs_ever_live[I]' is nonzero if register number I is ever
   used in the function.  This function is responsible for knowing
   which registers should not be saved even if used.  */

static void
i370_output_function_prologue (FILE *f, HOST_WIDE_INT l)
{
	fprintf(f, "\tEJECT\n");
	fprintf(f, "* %s function '%s' prologue\n",
		mvs_need_entry ? "external" : "static",
		CURRFUNC);

	fprintf(f, "* frame base=%d, local stack=%d, call args=%d\n", 
		STACK_FRAME_BASE, l, current_function_outgoing_args_size);

	fprintf(f, "&FUNC\tSETC\t'%s'\n", CURRFUNC);

	assemble_name(f, mvs_function_name);
	fprintf(f, "\tPDPPRLG CINDEX=%d,FRAME=%d,BASER=%d,ENTRY=%s\n",
		mvs_page_num,
		STACK_FRAME_BASE + l + current_function_outgoing_args_size,
		BASE_REGISTER,
		mvs_need_entry ? "YES" : "NO");

	fprintf(f, "\tB\t@@FEN%d\n", mvs_page_num);
	fprintf(f, "\tLTORG\n");
	fprintf(f, "@@FEN%d\tEQU\t*\n", mvs_page_num);
	fprintf(f, "\tDROP\t%d\n", BASE_REGISTER);
	fprintf(f, "\tBALR\t%d,0\n", BASE_REGISTER);
	fprintf(f, "\tUSING\t*,%d\n", BASE_REGISTER);
	fprintf(f, "@@PG%d\tEQU\t*\n", mvs_page_num );

	/* we use the size of these instructions for mvs_page_code below */
	fprintf(f, "\tLR\t11,1\n"); 		/* 2 bytes of code */
	fprintf(f, "\tL\t%d,=A(@@PGT%d)\n", /* 4 bytes of code */
		PAGE_REGISTER, mvs_page_num);

	fprintf(f, "* Function '%s' code\n", CURRFUNC);

	mvs_free_label_list ();

	mvs_page_code = 2 + 4;	/* LR 11,1 and L 10,=A(@@PGTnn) */
	mvs_page_lit = 4;		/* =A(@@PGTnn) */

	mvs_check_page(f, 0, 0);	/* sets assembler_source to f */

	function_base_page = mvs_page_num;

	/* find all labels in this routine */
	i370_label_scan ();
}


/* This function generates the assembly code for function exit.
   Args are as for output_function_prologue ().

   The function epilogue should not depend on the current stack
   pointer!  It should use the frame pointer only.  This is mandatory
   because of alloca; we also take advantage of it to omit stack
   adjustments before returning.  */

static void
i370_output_function_epilogue (FILE *file, HOST_WIDE_INT l)
{
	int i;

	check_label_emit ();
	mvs_check_page (file, 14, 0);
	fprintf (file, "* Function '%s' epilogue\n", CURRFUNC);

	mvs_page_num++;

	fprintf (file, "\tPDPEPIL\n");
	fprintf (file, "* Function '%s' literal pool\n", CURRFUNC);
	fprintf (file, "\tDS\t0D\n" );
	fprintf (file, "\tLTORG\n");
	fprintf (file, "* Function '%s' page table\n", CURRFUNC);
	fprintf (file, "@@PGT%d\tDS\t0F\n", function_base_page);

	mvs_free_label_list();
  
	for (i = function_base_page; i < mvs_page_num; i++) {
		fprintf (file, "\tDC\tA(@@PG%d)\n", i);
	}

	mvs_need_entry = 0;
}


#if defined(PUREISO) && !defined(NO_DETAB)

#undef fputs
#undef fprintf
#undef vfprintf
#undef fwrite
#undef fputc

int
t_fputs (const char *str, FILE *file)
{
    t_fprintf(file, "%s", str);
    if (ferror(file)) return (EOF);
    else return (0);
}

size_t
t_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *file)
{
    size_t tot;
    
    tot = size * nmemb;
    t_fprintf(file, "%.*s", tot, ptr);
    return (nmemb);
}

int
t_fprintf (FILE *file, const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = t_vfprintf(file, format, arg);
    va_end(arg);
    return (ret);
}

static int ocnt = 0;
static char obuf[MAX_LEN_OUT];

#if 1 /* experimental code */
int get_ocnt(void)
{
    return ocnt;
}
#endif /* experimental code */

int
t_fputc (int c, FILE *file)
{
    if (c == '\t')
    {
        if (ocnt < 9)
        {
            for (; ocnt < 9; ocnt++)
            {
                obuf[ocnt] = ' ';
            }
        }
        else if (ocnt < 15)
        {
            for (; ocnt < 15; ocnt++)
            {
                obuf[ocnt] = ' ';
            }
        }
        else
        {
            obuf[ocnt] = ' ';
            ocnt++;
        }
    }
    else if (c == '\n')
    {
        t_fprintf(file, "%c", c);
    }
    else
    {
        obuf[ocnt] = c;
        ocnt++;
    }
    return (c);
}

int
t_vfprintf (FILE *file, const char *format, va_list arg)
{
    char buf[MAX_LEN_OUT];
    int icnt;

    vsprintf(buf, format, arg);
    icnt = 0;
    while (buf[icnt] != '\0')
    {
        if (buf[icnt] == '\t')
        {
            if (ocnt < 9)
            {
                for (; ocnt < 9; ocnt++)
                {
                    obuf[ocnt] = ' ';
                }
            }
            else if (ocnt < 15)
            {
                for (; ocnt < 15; ocnt++)
                {
                    obuf[ocnt] = ' ';
                }
            }
            else
            {
                obuf[ocnt] = ' ';
                ocnt++;
            }
        }
        else
        {
            obuf[ocnt] = buf[icnt];
            ocnt++;
            if (buf[icnt] == '\n')
            {
                fwrite(obuf, ocnt, 1, file);
                ocnt = 0;
            }
        }
        icnt++;
    }
    return (icnt);
}

#endif
