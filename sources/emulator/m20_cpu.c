/*
 * File:     m20_cpu.c
 * Purpose:  M-20 CPU and memory simulator
 *
 * Copyright (c) 2008-2009, Serge Vakulenko
 * Copyright (c) 2014-2015, Dmitry Stefankov
 * Copyright (c) 2021-2022, Leonid Yadrennikov
 *
 * $Id$
 *
 * Revision History.
 *
 *  03-Nov-2014  DVS  Initial Implemementation
 *  04-Nov-2014  DVS  Added S.Vakulenko code
 *  08-Nov-2014  DVS  Fixed a bugs
 *  09-Nov-2014  DVS  Fixed a bugs
 *  11-Nov-2014  DVS  Added card reader
 *  12-Nov-2014  DVS  Added card punch
 *  13-Nov-2014  DVS  Added line printer
 *  14-Nov-2014  DVS  Added drum
 *  17-Nov-2014  DVS  Cleanup and more debugging
 *  18-Nov-2014  DVS  Right cyclic_sum
 *  20-Nov-2014  DVS  Added CPU device modifiers
 *  22-Nov-2014  DVS  Fixed a square root operation error
 *                    (forgetten exponent shift to left)
 *  30-Nov-2014  DVS  Fixed a square root operation error
 *                    (forgetten exponent shift if exponent is odd)
 *  01-Dec-2014  DVS  Fixed a cdr boot error
 *  03-Dec-2014  DVS  Changed a cdr boot logic
 *  04-Dec-2014  DVS  Added debug dump for regs and mem
 *                    Fixed a problem with rouding of addition (my wrong!)
 *  05-Dec-2014  DVS  Minor fixes
 *  06-Dec-2014  DVS  Change logic for buffered print (update buffer only)
 *  07-Dec-2014  DVS  First version of new computer arithemetic because
 *                    even corrected Vakulenko arithmetic failed testing
 *  08-Dec-2014  DVS  New versions of ADD,SUB,SUB_MOD (passed all tests!)
 *                    (we need new MULT and DIV)
 *  12-Jan-2015  DVS  Fixed problems and bugs of addition arithmetic operation
 *                    SSR passed tests successfully at first time!
 *  13-Jan-2015  DVS  Added control for rounding
 *                    Removed some code for readability
 *  21-Jan-2015  DVS  Returned back Shura-Bura and Starkman algorithm implementation
 *  23-Jan-2015  DVS  Checked operations times and conditions accoring M-20 technical description
 *  26-Jan-2015  DVS  Disabled workaround hack for BESM-4 opcode
 *  13-Feb-2015  DVS  Fixed a cyclic shift command implementation (op=67)
 *  27-Feb-2015  DVS  Removed auto skip zero address option
 *  06-Mar-2015  DVS  Added MOSU mode I and II, fixed cmd 020 error (must be used only 3 bits of A1)
 *  06-Mar-2015  DVS  Added statistics print on execution break
 *  09-Mar-2015  DVS  Some fixes and corrections for some instructions
 *                    But problems with arithmetic are remained, wee see these on complex tests
 *  23-May-2021  LOY  Changes in old multiplication algorithm. Now passes almost all multiply examples.
 *					  Only 3 still fails.
 *  26-May-2021	 LOY  Shura-Bura multiplication algorithm works. Fails the same 3 tests.
 *  27-May-2021  LOY  Shura-Bura multiplication simplified.
 *  28-May-2021  LOY  Shura-Bura multiplication simplified a bit more.
 *  30-May-2021  LOY  Shura-Bura addition passed full "test 6" (if to correct one erroneous exercise)
 *  01-Jun-2021  LOY  Shura-Bura addition refactored a little: sign determination shortened.
 *  05-Jun-2021  LOY  Old multiplication passes full "Multiply test 5". To do this, checking if
 *                    zero mantissa performs independently for regRR and regRMR, and the same for rexp.
 *  07-Jun-2021  LOY  Shura-Bura addition bugfix for linux gcc (>> more than 36 blocked)
 *  08-Jun-2021  LOY  Shura-Bura multiplication passes full "Multiply test 5".
 *                    (same changes as in Old Multiplication)
 *  22-Jun-2021  LOY  new_addition_v44 passed full "test 6" (if to correct one erroneous exercise),
 *                    and after that refactored a little (swap x<->y has thrown out). Also
 *                    cpu_one_inst changed (subtraction section). But not all hypotetic situations
 *                    covered by tests yet.
 *  24-Jun-2021  LOY  is_norm_zero moved; new_addition_v44 bugfix for linux gcc (>> more than 36)
 *  25-Jun-2021  LOY  -0 subtraction in new_addition_v44 processed correctly.
 *			          Bugfix in shift commands (14,34,54,74) for linux.
 *  18-Jul-2021  LOY  Type mismatch fixed in sim_instr. No more warnings during visual studio compile.
 *  24-Jul-2021  LOY  Fixed negative rexp bug in new_addition_v44. Now new_addition_v44 passes all tests.
 *  01-Aug-2021  LOY  Shura-Bura division quick-and-dirty fix:
 *                    1)ak_prev now reflects not (n-2), but (n-1) sign of qk.
 *                      This error led to twice calculation
 *                       of the 1st digit and NO calculation of the LSB.
 *                    2)On norm-to-right, norm first, round later.
 *                    3)Final right-shift to remove auxilary bit.
 *  01-Aug-2021  LOY  Shura-Bura division simplified.
 *  01-Aug-2021  LOY  Shura-Bura sqrt bugfix (TAG forgotten if machine zero).
 *  03-Aug-2021  LOY  Shura-Bura division simplified (no ak_prev, all in current loop pass, no x1-y1).
 *  10-Aug-2021  LOY  Removed wrong functions: addition, new_addition, new_addition_v20.
 *                    USE_NEW_ADD - Shurabura addition, else - new_addition_v44. USE_ADD_SBST = USE_NEW_ADD.
 *  26-May-2022  LOY  ITEP mode: draft FA command, regRA to 7776 mapping. 
 *                    And also a little tab-space cleanup.
 *  29-May-2022  LOY  Bugfix in FA (ITEP mode) for jump commands.
 *  30-May-2022  LOY  Yet another bugfix in FA (correct debug printing);
 *                    Order of steps in jump commands 16,36,56,76 changed (for correct regRK to 7777 mapping);
 *                    Draft regRK to 7777 mapping.
 *  01-Jun-2022  LOY  Draft trace of commmands executed by FA and RK-to-7777;
 *                    Fixed MOSU_MODE_II handling in mosu_store
 *  06-Jun-2022  LOY  Write to RA by writing to 7776 from SIMH file (cpu_deposit);
 *                    FA fix (modifiers use in the 2nd word)
 *  15-Jun-2023  LOY  Ability to disable B-61 trace.
 */

#include "m20_defs.h"
#include <math.h>
#include <float.h>


/* Register core */
uint16   regKRA;
uint16   regRA;
uint16   regSMA;
int      trgSW;
int      regROP;
t_value  regRK;
t_value  regRR;

t_value  regRMR;  /* M-220M */

int    old_trgSW = 0;
int    old_opcode = 0;
t_value   regP1 = 0;

/* control engineering panel */
t_value  RPU1 = 0;
//t_value  RPU1 = 0111000100010377;
//t_value  RPU1 = 0100000100010377;
t_value  RPU2 = 0;
t_value  RPU3 = 0;
t_value  RPU4 = 0;


/* card reader i/o addresses */
t_value cr_io_addr_1;
t_value cr_io_addr_2;
t_value cr_io_addr_3;


/* internal counters */
double delay;

/* special variable */




/*
 * Параметры обмена с внешним устройством.
 */
int ext_io_op;			/* УЧ - условное число */
int ext_io_dev_zone_addr;	/* А_МЗУ - начальный адрес на барабане/ленте/буфере_печ. */
int ext_io_ram_start;		/* Н_МОЗУ - начальный адрес памяти */
int ext_io_ram_end;		/* К_МОЗУ - конечный адрес памяти */
int ext_io_ram_jump;            /* П_МОЗУ - адрес памяти передачи упрваления если нет совпадения кс */
int ext_io_ram_chksum;          /* КС_МОЗУ - адрес памяти для запис КС */


/* MOSU - Magnetic Operating Storage Unit (main memory) */

t_value  MOSU[MAX_MEM_SIZE] = {0};


/* SIMH required declarations */

extern int32 sim_emax;

/* external devices */
extern t_stat read_card (t_value * csum, t_value * rsum, int * rcodes,
                         int * stop_blocking, int * control_blocking);
extern t_stat punch_card (int start_addr, int end_addr, int zone_buf_addr, int add_only_flag,
                          int dis_mem_acc, int dis_chksum, int * ocodes, t_value *sum );
extern t_stat write_line_printer (int start_addr, int end_addr, int zone_buf_addr, int pr_type,
                                  int add_only_flag, int dis_mem_acc, int dis_chksum,
                                  int * ocodes );
extern t_stat drum_io (t_value * sum, int * ocodes);
//extern t_stat mt_format_tape(t_value *sum, int * ocodes);
extern t_stat mt_format_tape (t_value *sum, int * ocodes, int user_first, int user_last);
extern t_stat mt_tape_io(t_value *sum, int * ocodes);


/* SYS module references */

extern t_value ieee_to_m20 (double d);

extern const char *m20_opname [M20_SYM_OPCODE_TABLE_SIZE];
extern const char *m20_short_opname [M20_SYM_OPCODE_TABLE_SIZE];


t_value  cdr_csum;
t_value  cdr_rsum;
int      cdr_rcodes;
int      cdr_stop_blocking;
int      cdr_control_blocking;
int      boot_device_req_cdr = 0;
int      debug_dump_regs = 0;
int      debug_dump_mem = 0;
int      debug_dump_modern_mem = 0;
int      disable_is2_trace = 0;
int      disable_b61_trace = 0;
int      arithmetic_op_debug = 0;
int      print_sys_stat = 1;
int      memory_45_checking = 1;


int      new_add = 0;
int      new_mult = 0;
int      new_div = 0;
int      new_sqrt = 0;
int      itep_mode = 0;

static  int  enable_m20_print_ascii_text = 0;

int  diag_print = 0;
int  enable_opcode_040_hack = 0;

int  print_stat_on_break = 1;

int  run_mode = M20_AUTO_MODE;
int  mosu_mode = MOSU_MODE_I;

/* workaround for buffered print */
int active_lpt = 0;
int active_cdp = 0;


/* Functions */
t_stat cpu_examine (t_value *vptr, t_addr addr, UNIT *uptr, int32 sw);
t_stat cpu_deposit (t_value val, t_addr addr, UNIT *uptr, int32 sw);
t_stat cpu_reset (DEVICE *dptr);
t_stat cpu_one_inst ();


/*
 * CPU data structures
 *
 * cpu_dev      CPU device descriptor
 * cpu_unit     CPU unit descriptor
 * cpu_reg      CPU register list
 * cpu_mod      CPU modifiers list
 */

UNIT cpu_unit = { UDATA (NULL, UNIT_FIX, MAX_MEM_SIZE) };


extern char  reg_rk_name[];
extern char  reg_ra_name[];
extern char  reg_rop_name[];
extern char  reg_kra_name[];
extern char  reg_sma_name[];
extern char  trg_sw_name[];
extern char  reg_rr_name[];
extern char  reg_rpu1_name[];
extern char  reg_rpu2_name[];
extern char  reg_rpu3_name[];
extern char  reg_rpu4_name[];

extern char  reg_rk_desc[];
extern char  reg_ra_desc[];
extern char  reg_rop_desc[];
extern char  reg_kra_desc[];
extern char  reg_sma_desc[];
extern char  trg_sw_desc[];
extern char  reg_rr_desc[];
extern char  reg_rpu1_desc[];
extern char  reg_rpu2_desc[];
extern char  reg_rpu3_desc[];
extern char  reg_rpu4_desc[];


REG cpu_reg[] = {
	{ reg_kra_name,  &regKRA,   8, 12, 0, 1, reg_kra_desc },
	{ reg_rk_name,   &regRK,    8, 45, 0, 1, reg_rk_desc  },
	{ reg_rop_name,  &regROP,   8,  6, 0, 1, reg_rop_desc },
	{ reg_ra_name,   &regRA,    8, 12, 0, 1, reg_ra_desc  },
	{ reg_sma_name,  &regSMA,   8, 12, 0, 1, reg_sma_desc },
	{ trg_sw_name,	 &trgSW,    8, 1,  0, 1, trg_sw_desc  },
	{ reg_rr_name,   &regRR,    8, 45, 0, 1, reg_rr_desc  },
	{ reg_rpu1_name, &RPU1,     8, 45, 0, 1, reg_rpu1_desc },
	{ reg_rpu2_name, &RPU2,     8, 45, 0, 1, reg_rpu2_desc },
	{ reg_rpu3_name, &RPU3,     8, 45, 0, 1, reg_rpu3_desc },
	{ reg_rpu4_name, &RPU4,     8, 45, 0, 1, reg_rpu4_desc },
        { DRDATA (PRINT_STAT_ON_BREAK, print_stat_on_break, 8), PV_LEFT },
        { DRDATA (RUN_MODE, run_mode, 8), PV_LEFT },
        { DRDATA (MOSU_MODE, mosu_mode, 8), PV_LEFT },
        { DRDATA (PRINT_SYS_STAT, print_sys_stat, 8), PV_LEFT },
        { DRDATA (DIAG_PRINT, diag_print, 8), PV_LEFT },
        { DRDATA (DEBUG_DUMP_REGS, debug_dump_regs, 8), PV_LEFT },
        { DRDATA (DEBUG_DUMP_MEM,  debug_dump_mem,  8), PV_LEFT },
        { DRDATA (DEBUG_DUMP_MODERM_MEM, debug_dump_modern_mem,  8), PV_LEFT },
        { DRDATA (ENABLE_M20_PRINT_ASCII_TEXT, enable_m20_print_ascii_text, 8), PV_LEFT },
        { DRDATA (DISABLE_IS2_TRACE, disable_is2_trace, 8), PV_LEFT },
        { DRDATA (DISABLE_B61_TRACE, disable_b61_trace, 8), PV_LEFT },
        { DRDATA (MEMORY_45_CHECKING, memory_45_checking, 8), PV_LEFT },
        { DRDATA (ENABLE_OPCODE_040_HACK, enable_opcode_040_hack, 8), PV_LEFT },
        { DRDATA (ARITHMETIC_OP_DEBUG, arithmetic_op_debug, 8), PV_LEFT },
        { DRDATA (USE_NEW_ADD, new_add, 8), PV_LEFT },
        { DRDATA (USE_NEW_MULT, new_mult, 8), PV_LEFT },
        { DRDATA (USE_NEW_DIV, new_div, 8), PV_LEFT },
        { DRDATA (USE_NEW_SQRT, new_sqrt, 8), PV_LEFT },
        { DRDATA (USE_ADD_SBST, new_add, 8), PV_LEFT },
        { DRDATA (ITEP_MODE, itep_mode, 8), PV_LEFT },
	{ 0 }
};



MTAB cpu_mod[] = {
    { SHORT_SYM_OP, SHORT_SYM_OP, "short symbolic instruction name", "SHORT_SYM_OPCODE", NULL },
    { SHORT_SYM_OP, 0,            "long  symbolic instruction name", "LONG_SYM_OPCODE", NULL },
    { 0 }
};


DEVICE cpu_dev = {
	"CPU", &cpu_unit, cpu_reg, cpu_mod,
	1, 8, 12, 1, 8, 45,
	&cpu_examine, &cpu_deposit, &cpu_reset,
	NULL, NULL, NULL, NULL,
	DEV_DEBUG
};


typedef  struct command_profile_stat {
    int      op_code;
    double   us_count;
    double   us_time;
} COMMAND_PROFILE_STAT, * PCOMMAND_PROFILE_STAT;


COMMAND_PROFILE_STAT   cmd_profile_table[M20_SYM_OPCODE_TABLE_SIZE] = {
   {OPCODE_TRANSFER_MEM2MEM, 0, 0},
   {OPCODE_ADD_ROUND_NORM, 0, 0},
   {OPCODE_SUB_ROUND_NORM, 0, 0},
   {OPCODE_SUB_MOD_ROUND_NORM, 0, 0},
   {OPCODE_DIV_ROUND_NORM, 0, 0},
   {OPCODE_MULT_ROUND_NORM, 0, 0},
   {OPCODE_ADD_ADDR_TO_EXP, 0, 0},
   {OPCODE_ADD_CYCLIC, 0, 0},
   {OPCODE_INPUT_CODES_FROM_PUNCH_CARDS_WITH_STOP, 0, 0},
   {OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_1_011, 0, 0},
   {OPCODE_GOTO_AFTER_CYCLE_BY_PA_012, 0, 0},
   {OPCODE_ADD_CMDS, 0, 0},
   {OPCODE_SHIFT_MANTISSA_BY_ADDR, 0, 0},
   {OPCODE_COMPARE, 0, 0},
   {OPCODE_JUMP_WITH_RETURN, 0, 0},
   {OPCODE_STOP_017, 0, 0},
   {OPCODE_LOAD_FROM_KEY_REGISTER, 0, 0},
   {OPCODE_ADD_NORM, 0, 0},
   {OPCODE_SUB_NORM, 0, 0},
   {OPCODE_SUB_MOD_NORM, 0, 0},
   {OPCODE_DIV_NORM, 0, 0},
   {OPCODE_MULT_NORM, 0, 0},
   {OPCODE_ADD_EXP_TO_EXP, 0, 0},
   {OPCODE_SUB_CYCLIC, 0, 0},
   {OPCODE_INPUT_CODES_FROM_PUNCH_CARDS, 0, 0},
   {OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_1_031, 0, 0},
   {OPCODE_GOTO_AFTER_CYCLE_BY_PA_032, 0, 0},
   {OPCODE_SUB_CMDS, 0, 0},
   {OPCODE_SHIFT_MANTISSA_BY_EXP, 0, 0},
   {OPCODE_COMPARE_WITH_STOP, 0, 0},
   {OPCODE_COND_JUMP_BY_SIG_W_1, 0, 0},
   {OPCODE_STOP_037, 0, 0},
   {OPCODE_BLANKING_040, 0, 0},
   {OPCODE_ADD_ROUND, 0, 0},
   {OPCODE_SUB_ROUND, 0, 0},
   {OPCODE_SUB_MOD_ROUND, 0, 0},
   {OPCODE_SQRT_ROUND_NORM, 0, 0},
   {OPCODE_MULT_ROUND, 0, 0},
   {OPCODE_SUB_ADDR_FROM_EXP, 0, 0},
   {OPCODE_OUT_LOWER_BITS_OF_MULT, 0, 0},
   {OPCODE_IO_EXT_DEV_TO_MEM_050, 0, 0},
   {OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_0_051, 0, 0},
   {OPCODE_CHANGE_RA_BY_ADDR, 0, 0},
   {OPCODE_ADD_OPCS, 0, 0},
   {OPCODE_SHIFT_CODE_BY_ADDR, 0, 0},
   {OPCODE_LOGICAL_MULT, 0, 0},
   {OPCODE_JUMP_BY_ADDR, 0, 0},
   {OPCODE_STOP_057, 0, 0},
   {OPCODE_BLANKING_060, 0, 0},
   {OPCODE_ADD, 0, 0},
   {OPCODE_SUB, 0, 0},
   {OPCODE_SUB_MOD, 0, 0},
   {OPCODE_SQRT_NORM, 0, 0},
   {OPCODE_MULT, 0, 0},
   {OPCODE_SUB_EXP_FROM_EXP, 0, 0},
   {OPCODE_SHIFT_CYCLIC, 0, 0},
   {OPCODE_IO_EXT_DEV_TO_MEM_070, 0, 0},
   {OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_0_071, 0, 0},
   {OPCODE_CHANGE_RA_BY_CODE, 0, 0},
   {OPCODE_SUB_OPCS, 0, 0},
   {OPCODE_SHIFT_CODE_BY_EXP, 0, 0},
   {OPCODE_LOGICAL_ADD, 0, 0},
   {OPCODE_COND_JUMP_BY_SIG_W_0, 0, 0},
   {OPCODE_STOP_077, 0, 0}
};




/* Moderm floating math */
double m20_to_ieee (t_value w)
{
    double d;
    int exponent;

    //d = word & 0xfffffffffLL;
    d = (double)(w & 0xfffffffffLL);
    exponent = (w >> BITS_36) & 0x7f;
    d = ldexp (d, exponent - 64 - 36);
    if ((w >> 43) & 1) d = -d;

    return d;
}



/*
 *  Checksum calculation
 */

t_value  cyclic_checksum( t_value x, t_value y)
{
   t_value  t1,t2;
   t1 = (x & EXP_SIGN_TAG) + (y & EXP_SIGN_TAG);
   t2 = (x & MANTISSA) + (y & MANTISSA);
   if (t1 >= BIT46) { t1 -= BIT46; t1 += BIT37; }
   t1 &= WORD45;
   if (t2 >= BIT37) { t2 -= BIT37; t2 += 1; }
   t1 |= (t2 & MANTISSA);
   t1 &= WORD45;

   return t1;
}




/*
 * Memory examine implementaton
 */
t_stat cpu_examine (t_value *vptr, t_addr addr, UNIT *uptr, int32 sw)
{
   t_value t;

   if (addr >= MAX_MEM_SIZE) return SCPE_NXM;

   if (vptr != NULL) *vptr = MOSU[addr];
   if (mosu_mode == MOSU_MODE_II) {
     if (addr == 07770) { t = 0;     *vptr=t; }
     if (addr == 07771) { t = RPU1;  *vptr=t; }
     if (addr == 07772) { t = RPU2;  *vptr=t; }
     if (addr == 07773) { t = RPU3;  *vptr=t; }
     if (addr == 07774) { t = RPU4;  *vptr=t; }
     if (addr == 07775) { t = regRR; *vptr=t; }
     if (addr == 07776) { t = 0;     *vptr=t; }
     if (addr == 07777) { t = 0;     *vptr=t; }
   }

   return SCPE_OK;
}


/*
 * Memory deposit
 */
t_stat cpu_deposit (t_value val, t_addr addr, UNIT *uptr, int32 sw)
{
   if (addr >= MAX_MEM_SIZE) return SCPE_NXM;

   /* Word by address 0 always contains 0. */
   //if (addr != 0) MOSU[addr] = val;
   if (addr == 0) return STOP_WRITE_TO_RO_MEM_LOC;

   if (mosu_mode == MOSU_MODE_II) {
     if (addr == 07770) return STOP_WRITE_TO_RO_MEM_LOC;
     if (addr == 07771) return STOP_WRITE_TO_RO_MEM_LOC;
     if (addr == 07772) return STOP_WRITE_TO_RO_MEM_LOC;
     if (addr == 07773) return STOP_WRITE_TO_RO_MEM_LOC;
     if (addr == 07774) return STOP_WRITE_TO_RO_MEM_LOC;
     if (addr == 07775) return STOP_WRITE_TO_RO_MEM_LOC;
     if (addr == 07776) {
	if (itep_mode) {
		if (addr == 07776) {
			regRA = val >> BITS_12 & MAX_ADDR_VALUE;
			return SCPE_OK;  }
		else return STOP_WRITE_TO_RO_MEM_LOC; }
     }
     if (addr == 07777) return STOP_WRITE_TO_RO_MEM_LOC;
   }

   MOSU[addr] = val;

   return SCPE_OK;
}


/*
 * Reset routine
 */
t_stat cpu_reset (DEVICE *dptr)
{
    if (sim_deb && cpu_dev.dctrl)
	fprintf (sim_deb, "cpu: reset\n" );

    //regRA   = 0;
    //regKRA  = 0;
    regSMA  = 0;
    //trgSW   = 0;
    regRK   = 0;
    regRMR  = 0;
    ext_io_op = MAX_ADDR_VALUE;

    sim_brk_types = sim_brk_dflt = SWMASK ('E');

    //memset( MOSU, 0, sizeof(MOSU) );

    return SCPE_OK;
}


void trace_before_run(pa1,pa2,pa3,pt_ra,pt_sw,pt_rr,pm1,pm2,pm3,irreg)
int *pa1, *pa2, *pa3, *pt_sw, irreg;
uint16 *pt_ra;
t_value *pm1, *pm2, *pm3, *pt_rr;
{
  if (sim_deb && cpu_dev.dctrl) {
	int a1,a2,a3,addr_tags;
	    if (disable_is2_trace) {
	      if ((regKRA >= 07200) && (regKRA <= 07767)) goto trace_before_done;
	    }
	    if (disable_b61_trace) {
	      if ((regKRA >= 00200) && (regKRA <= 00677)) goto trace_before_done;
	    }
	    addr_tags = regRK >> BITS_42 & MAX_ADDR_TAG_VALUE;
	    a1 = regRK >> BITS_24 & MAX_ADDR_VALUE;
	    a2 = regRK >> BITS_12 & MAX_ADDR_VALUE;
	    a3 = regRK >> BITS_0  & MAX_ADDR_VALUE;
	    if (addr_tags & 4) a1 = (a1 + regRA) & MAX_ADDR_VALUE;
	    if (addr_tags & 2) a2 = (a2 + regRA) & MAX_ADDR_VALUE;
	    if (addr_tags & 1) a3 = (a3 + regRA) & MAX_ADDR_VALUE;
	    /*fprintf (sim_deb, "*** (%.0f) %04o: ", sim_gtime(), RVK);*/
            if (debug_dump_regs || debug_dump_mem) {
                int i;
                for( i=0; i<100; i++ ) fprintf (sim_deb, "-");
                fprintf (sim_deb, "\n");
             }
	     if (irreg) fprintf (sim_deb, "cpu: byRK: ");
	     else fprintf (sim_deb, "cpu: %04o: ", regKRA);
	    fprint_sym (sim_deb, regKRA, &regRK, 0, SWMASK ('M'));
	    fprintf (sim_deb, "\n");
	    if (debug_dump_regs) {
	      *pt_ra = regRA; *pt_rr = regRR; *pt_sw = trgSW;
	      fprintf (sim_deb, "cpu: [dreg]: ra=%04o,  sw=%d,  rr=%015llo\n", *pt_ra, *pt_sw, *pt_rr );
	    }
	    if (debug_dump_mem) {
	      *pm1 = MOSU[a1]; *pm2 = MOSU[a2]; *pm3 = MOSU[a3];
	      fprintf (sim_deb, "cpu: [dmem]: a1[%04o]=%015llo,  a2[%04o]=%015llo,  a3[%04o]=%015llo\n",
                       a1, *pm1, a2, *pm2, a3, *pm3 );
              if (debug_dump_modern_mem) {
	          fprintf (sim_deb, "cpu: [fmem]: a1[%04o]=%.12f,  a2[%04o]=%.12f,  a3[%04o]=%.12f\n",
                           a1,m20_to_ieee(MOSU[a1]), a2,m20_to_ieee(MOSU[a2]), a3,m20_to_ieee(MOSU[a3]) );
              }
	    }
            if (debug_dump_regs || debug_dump_mem) fprintf (sim_deb, "\n");
	    *pa1=a1; *pa2=a2; *pa3=a3;
          trace_before_done: {};
  }
}


void trace_after_run(a1,a2,a3,t_ra,t_sw,t_rr,m1,m2,m3)
int a1, a2, a3, t_sw;
uint16 t_ra;
t_value m1, m2, m3, t_rr;
{
if (sim_deb && cpu_dev.dctrl) {
  char c1,c2,c3;
	    if (disable_is2_trace) {
	      if ((regKRA >= 07200) && (regKRA <= 07767)) goto trace_after_done;
	    }
	    if (disable_b61_trace) {
	      if ((regKRA >= 00200) && (regKRA <= 00677)) goto trace_after_done;
	    }
	           if (debug_dump_regs) {
	             c1='-'; c2='-'; c3='-';
	             if (t_ra != regRA) c1 = '*';
	             if (t_sw != trgSW) c2 = '*';
	             if (t_rr != regRR) c3 = '*';
	             fprintf (sim_deb, "cpu: [dreg]: ra=%04o%c, sw=%d%c, rr=%015llo%c\n",
                              regRA, c1, trgSW, c2, regRR, c3 );
	           }
	           if (debug_dump_mem) {
                     c1='-'; c2='-'; c3='-';
                     if (m1 != MOSU[a1]) c1 = '*';
                     if (m2 != MOSU[a2]) c2 = '*';
                     if (m3 != MOSU[a3]) c3 = '*';
	             fprintf (sim_deb, "cpu: [dmem]: a1[%04o%c]=%015llo, a2[%04o%c]=%015llo, a3[%04o%c]=%015llo\n",
                              a1, c1, MOSU[a1], a2, c2, MOSU[a2], a3, c3, MOSU[a3] );
                     if (debug_dump_modern_mem) {
	                 fprintf (sim_deb, "cpu: [fmem]: a1[%04o%c]=%.12f,  a2[%04o%c]=%.12f,  a3[%04o%c]=%.12f\n",
                          a1,c1,m20_to_ieee(MOSU[a1]), a2,c2,m20_to_ieee(MOSU[a2]), a3,c3,m20_to_ieee(MOSU[a3]) );
                     }
	           }
	           if (debug_dump_regs || debug_dump_mem) fprintf (sim_deb, "\n");
          trace_after_done: {};
  }
}

t_stat irregular_cmd()
{
	int a1,a2,a3,t_sw;
	uint16 t_ra;
	t_value m1,m2,m3,t_rr;
	t_stat err;
		trace_before_run(&a1,&a2,&a3,&t_ra,&t_sw,&t_rr,&m1,&m2,&m3,1);
		err=cpu_one_inst();
		trace_after_run(a1,a2,a3,t_ra,t_sw,t_rr,m1,m2,m3);
		return err;
}

/*
 * Считывание слова из памяти.
 */
t_value mosu_load (int addr)
{
    t_value val;

    addr &= MAX_ADDR_VALUE;
    //if (addr == 0) return 0;

    val = MOSU[addr];

    if (mosu_mode == MOSU_MODE_II) {
      if (addr == 07770) { val = 0;     }
      if (addr == 07771) { val = RPU1;  }
      if (addr == 07772) { val = RPU2;  }
      if (addr == 07773) { val = RPU3;  }
      if (addr == 07774) { val = RPU4;  }
      if (addr == 07775) { val = regRR; }
            if (itep_mode) {
	if (addr == 07776)
		{val = regRA << BITS_12; }
/*	if (addr == 07777) {
		//РК */
	}
      else {
      if (addr == 07776) { val = 0;     }
      if (addr == 07777) { val = 0;     }
	}
      }

    return val;
}



/*
 * Запись слова в память.
 */
void mosu_store (int addr, t_value val)
{
    addr &= MAX_ADDR_VALUE;
    if (addr == 0) return;

    if ( (mosu_mode == MOSU_MODE_II) && (addr > 07767) ) {
	if (itep_mode) {
		if (addr == 07776)
			{ regRA = val >> BITS_12 & MAX_ADDR_VALUE; }
		if (addr == 07777) {
			regRK = val;
			irregular_cmd();
		}
	}
    }
    else MOSU[addr] = val;
}


/*
 * Проверка числа на равенство нулю.
 */
//inline static int is_zero (t_value x)
static int is_zero (t_value x)
{
    x &= ~(TAG | SIGN);

    return (x == 0);
}


/*
 * Обычная нормализация числа (влево).
 */
t_value normalize (t_value x)
{
    int exp;
    t_value m;

    exp = x >> BITS_36 & 0177;
    m = x & MANTISSA;

    if (m == 0) {
//zero:		/* Нулевая мантисса, превращаем в ноль. */
	return x & TAG;
    }

    for (;;) {
 	if (m & 0400000000000LL)  break;
	m <<= 1;
	--exp;
	if (exp < 0) return x & TAG;
          //goto zero;
    }

    x &= TAG | SIGN;
    x |= (t_value) exp << BITS_36 | m;

    return x;
}


/*
 * Коррекция порядка.
 */
t_stat add_exponent (t_value *result, t_value x, int n, int opcode)
{
    int exp_val;

    exp_val = (int) (x >> BITS_36 & 0177) + n;

    if ((exp_val > 127) && (x & MANTISSA) == 0) {
      x = TAG;
      *result = x;
      return 0;
    }

    if ((exp_val < 0) || (x & MANTISSA) == 0) {
      /* Ноль. */
      x = TAG;
      *result = x;
      return 0;
    }

    if (exp_val > 127) {
	/* переполнение при сложении порядков */
	return STOP_EXPOVF;
    }

    //*result = x;
    *result = (x&SIGN) | (x&MANTISSA) | (x&TAG) | ((t_value)exp_val << BITS_36);

    return 0;
}


/*
 * Умножение двух 36-битовых целых чисел, с выдачей двух половин результата.
 */
void mul36x36 (t_value x, t_value y, t_value *hi, t_value *lo)
{
    int yhi, ylo;
    t_value rhi, rlo;

    /* Разбиваем второй множитель на две половины. */
    //yhi = y >> 18;
    yhi = (int)(y >> BITS_18);
    ylo = y & 0777777;

    /* Частичные 54-битовые произведения. */
    rhi = x * yhi;
    rlo = x * ylo;

    /* Составляем результат. */
    rhi += rlo >> BITS_18;
    *hi = rhi >> BITS_18;
    *lo = (rhi & 0777777) << BITS_18 | (rlo & 0777777);
}



/*
 * Умножение двух чисел, с блокировкой округления и нормализации,
 * если требуется.
 */
t_stat multiplication (t_value *result, t_value x, t_value y, int no_round, int no_norm)
{
    int xexp, yexp, rexp;
    t_value xm, ym, r;

    if (arithmetic_op_debug)
      fprintf( stderr, "MULT: ENTER: x=%015llo, y=%015llo no_round=%d no_norm=%d\n", x, y, no_round, no_norm );

    /* Извлечем порядок чисел. */
    xexp = x >> BITS_36 & 0177;
    yexp = y >> BITS_36 & 0177;

    /* Извлечем мантиссу чисел. */
    xm = x & MANTISSA;
    ym = y & MANTISSA;

    if (arithmetic_op_debug)
      fprintf( stderr, "mult: xm=%015llo, ym=%015llo xexp=%d yexp=%d\n", xm, ym, xexp, yexp );

    /* Умножим. */
    rexp = xexp + yexp - M20_MANTISSA_SHIFT;
    mul36x36 (xm, ym, &r, &regRMR);

    if (arithmetic_op_debug) fprintf( stderr, "mult: r=%015llo, regRMR=%015llo rexp=%d\n", r, regRMR, rexp );


	 if (! no_round) {
	/* 1-я стадия округления: прибавляем 1 к 35-му разряду мантиссы (старший 8ричн разряд слагаемого 010 =2).
	В реальной М-20 regRMR с 1 по 35 разряд не является суммирующим и это прибавление делается иначе, в процессе
	сдвигов при умножении. Однако результат и смысл ровно те же. */

	regRMR += 0200000000000LL;
	if (regRMR & BIT37) {
		r += 1;
		regRMR &= MANTISSA;
	}
	//старое округление по Вакуленко (а в сущности по Ляшенко)
	/*
	if (regRMR & 0 4000 0000 0000LL) {
	    r += 1;
	} */
       if (arithmetic_op_debug) fprintf( stderr, "mult: 1st_ROUND_DONE: r=%015llo, regRMR=%015llo rexp=%d\n", r, regRMR, rexp );
    }

	  if (! no_norm && !(r & 0400000000000LL)) {
	/* Нормализация на один разряд влево. */

	--rexp;
	r <<= 1;
	regRMR <<= 1;
	if (regRMR & BIT37) {
 	    r |= 1;
	    regRMR &= MANTISSA;
	}
       if (arithmetic_op_debug) fprintf( stderr, "mult: NORM_DONE: r=%015llo, regRMR=%015llo rexp=%d\n", r, regRMR, rexp );
    }
	else {
	/* при отсутствии нормализации и включенном режиме округления необходимо произвести 2-ю стадию округления.
	Прибавлять нужно вновь 1 к 35-му разряду regRMR, но т.к. он в М-20 не является суммирующим (умеющие суммировать
	разряды начинаются с Др СмЧ, что в эмуляторе применительно к умножению эквивалентно 36-му рязряду regRMR),
	нужно посмотреть на 35-й разряд, и если там есть 1, прибавить 1 к 36-му разряду, имея в виду перенос в r
    при необходимости. Единица в 35 разряде останется.	*/

		if (! no_round && (regRMR & 0200000000000LL)) {
			regRMR += BIT36;
			if (regRMR & BIT37) {
				r += 1;
				regRMR &= MANTISSA;
			}
			if (arithmetic_op_debug) fprintf( stderr, "mult: 2nd_ROUND_DONE: r=%015llo, regRMR=%015llo rexp=%d\n", r, regRMR, rexp );
		}
	}

    if (arithmetic_op_debug) fprintf( stderr, "mult: r=%015llo, regRMR=%015llo rexp=%d\n", r, regRMR, rexp );

    if (rexp > 127) {
	/* переполнение при умножении */
	return STOP_MULOVF;
    }

    /* Конструируем результат. */
	if (r == 0 || rexp < 0) {
	/* Нуль. */
	r = (x | y) & TAG;
        if (arithmetic_op_debug)
          fprintf( stderr, "mult: return NORMZERO=%015llo, regRMR=%015llo rexp=%d\n", r, regRMR, rexp );
    }
	else {
    if (arithmetic_op_debug) fprintf( stderr, "mult: FINAL 1: r=%015llo, rexp=%d\n", r, rexp );

     r |= (t_value) rexp << BITS_36;
     r |= ((x ^ y) & SIGN) | ((x | y) & TAG);
	}

     regRMR &= MANTISSA;
	 if (regRMR == 0 || rexp < 0) {
     /* Unlike the techref, criteria for NORMZERO return are the same as for higher bits.
	 If to obey the techref, we should return NORMZERO only if mantissa of regRMR ==0.
	 But in this case we can't pass multiplication test.
	 So we decide that test passing is more significant.*/
		 regRMR = (x | y) & TAG;
	 }
	 else {
     if (arithmetic_op_debug)
       fprintf( stderr, "mult: FINAL 2: r=%015llo, regRMR=%015llo rexp=%d\n", r, regRMR, rexp );
     regRMR |= (t_value) rexp << BITS_36;
     regRMR |= ((x ^ y) & SIGN) | ((x | y) & TAG);
	 }



     if (arithmetic_op_debug)
       fprintf( stderr, "mult: FINAL 3: r=%015llo, regRMR=%015llo rexp=%d\n\n", r, regRMR, rexp );

     *result = r;

     return 0;
}



/*
 * Деление двух чисел, с блокировкой округления, если требуется.
 */
t_stat division (t_value *result, t_value x, t_value y, int no_round)
{
    int xexp, yexp, rexp;
    t_value xm, ym, r;

    /* Извлечем порядок чисел. */
    xexp = x >> BITS_36 & 0177;
    yexp = y >> BITS_36 & 0177;

    /* Извлечем мантиссу чисел. */
    xm = x & MANTISSA;
    ym = y & MANTISSA;
    if (xm >= 2*ym) {
 	/* переполнение мантиссы при делении */
	return STOP_DIVMOVF;
    }

    /* Поделим. */
    rexp = xexp - yexp + 64;
    //r = (double) xm / ym * BIT37;
    r = (t_value)((double) xm / ym * BIT37);

    if (r >> BITS_36) {
	/* Выход за 36 разрядов, нормализация вправо. */
	if (! no_round) {
  	    /* Округление. */
	    r += 1;
	}
	r >>= 1;
	++rexp;
    }

    if (r == 0 || rexp < 0) {
	/* Нуль. */
	*result = (x | y) & TAG;
	return 0;
    }

    if (rexp > 127) {
	/* переполнение при делении */
	return STOP_DIVOVF;
    }

    /* Конструируем результат. */
    r |= (t_value) rexp << BITS_36;
    r |= ((x ^ y) & SIGN) | ((x | y) & TAG);

    *result = r;

    return 0;
}




/*
 * Вычисление квадратного корня, с блокировкой округления, если требуется.
 */
t_stat square_root (t_value *result, t_value x, int no_round)
{
    int exponent;
    int exp_shift = 0;
    t_value r;
    double q;

    if (x & SIGN) {
	/* корень из отрицательного числа */
	return STOP_NEGSQRT;
    }

    /* Извлечем порядок числа. */
    exponent = x >> BITS_36 & 0177;

    /* Извлечем мантиссу чисел. */
    r = x & MANTISSA;

    /* Вычисляем корень. */
    if (exponent & 1) {
	/* Нечетный порядок. */
	r >>= 1;
        exp_shift = 1;
    }

    exponent = (exponent >> 1) + 32;
    q = sqrt ((double) r) * BIT19;
    r = (t_value) q;
    if (! no_round) {
	/* Смотрим остаток. */
	if (q - r >= 0.5) {
		/* Округление. */
		r += 1;
	}
    }

    if (r == 0) {
	/* Нуль. */
	*result = x & TAG;
	return 0;
    }

    if (r & ~MANTISSA) {
	/* ошибка квадратного корня */
	return STOP_SQRTERR;
    }

    /* Конструируем результат. */
    //r |= (t_value) exponent << BITS_36;
    r |= ((t_value)exponent+exp_shift) << BITS_36;
    r |= x & TAG;

    *result = r;

    return 0;
}




/*
 *  New arithmetic operations implementations
 *  (used shura-bura and other sources)
 */

static int  is_norm_zero( t_value num )
{
    if ( ((num & SIGN) == 0) && ((num & MANTISSA) == 0) && ((num & EXPONENT) == 0))
        return 1;
    else
        return 0;
}

static t_value  norm_zero( void )
{
  t_value t;

  t = 0 | ((t_value)0 << BITS_36);

  return ( t );
}


/*
 * Two numbers addition. If required then blocking of rounding and normalization.
 */
t_stat new_addition_v44 (t_value *result, t_value x, t_value y, int no_round, int no_norm, int force_round)
    /* force_round=1 forces rounding: machine zero and opposite signs checking are disabled.
    rounding performs if no_round=0 and mantissa aling occured */
{
    int xexp, yexp, rexp, fix_sign, xs, ys, rs, r_bit, shift_count, delta_exp;
    t_value r;
    t_int64 xm, ym, xm1, ym1, rr;

    r = 0;
    if (arithmetic_op_debug)
      fprintf( stderr, "ADD v44: no_round=%d no_norm=%d x=%015llo y=%015llo\n", no_round, no_norm, x, y );

    /* Get exponent */
    xexp = x >> BITS_36 & 0177;
    yexp = y >> BITS_36 & 0177;

    if (arithmetic_op_debug) fprintf( stderr, "add01: x_exp=%d y_exp=%d\n", xexp, yexp );

    /* Get mantissa */
    xm = x & MANTISSA;
    ym = y & MANTISSA;

    xs = ys = 1;
    if (x & SIGN) xs = -1;
    if (y & SIGN) ys = -1;

    xm1 = xm << 1;
    ym1 = ym << 1;

    if (arithmetic_op_debug) fprintf( stderr, "add02: xm=%015llo ym=%015llo xm1=%018llo ym1=%018llo\n", xm, ym, xm1, ym1 );

	delta_exp = xexp - yexp;
    if (!no_round && (force_round || (((x ^ y) & SIGN) == 0))) {
			if (arithmetic_op_debug) fprintf(stderr,"xnormzero=%d ynormzero=%d \n",is_norm_zero(x),is_norm_zero(y));
		if ( (xexp != yexp) && (force_round || (!(is_norm_zero(x)) && !(is_norm_zero(y))))) {
			//Round process is set 1 to auxilary bit of not shifted summand only
			if (delta_exp > 0)  xm1 |= 1;
            else ym1 |= 1;
          if (arithmetic_op_debug) fprintf( stderr, "add: ROUND: xm=%015llo ym=%015llo xm1=%018llo ym1=%018llo\n", xm, ym, xm1, ym1 );
       }
    }

    if (arithmetic_op_debug) fprintf( stderr, "add03: delta_exp=%d xm1=%018llo ym1=%018llo\n", delta_exp, xm1, ym1 );

    /* Mantissa alignment */
    if (delta_exp >= 0) {
		if (delta_exp < 37) ym1 >>= delta_exp;
		else ym1 = 0;
		rexp = xexp;
	}
    else {
		if ((-delta_exp) < 37) xm1 >>= -delta_exp;
		else xm1 = 0;
		rexp = yexp;
	}

	/* Sign setting - only here, if do it before rounding, we can get wrong results */
	xm1 *= xs;
	ym1 *= ys;

    /* Addition */
    if (arithmetic_op_debug) fprintf( stderr, "add04: rexp=%d xm1=%018llo ym1=%018llo\n", rexp, xm1, ym1 );

    rr = xm1 + ym1;
    if (arithmetic_op_debug) fprintf( stderr, "add05: rr=%018llo\n", rr );
    rs = 1;
    if (rr < 0) {
      rs = -1;
	  rr = -rr;
    }
    if (arithmetic_op_debug) fprintf( stderr, "add06: rr=%018llo rs=%d\n", rr, rs );
    r = (rr & (MANTISSA|BIT37|BIT38));
    if (arithmetic_op_debug) fprintf( stderr, "add07: rr=%018llo\n", rr );

    r_bit = 0;
    shift_count = 0;

    /* normalization to right */
    if (r & (BIT37<<1)) {
        if (arithmetic_op_debug) fprintf( stderr, "add: C1: NORM_R: rexp=%d r=%018llo\n", rexp, r );
        r_bit = r & 1;
        if (!no_round && r_bit) {
           r += 1;
           if (arithmetic_op_debug) fprintf( stderr, "add: C3: ROUND: rexp=%d r=%018llo\n", rexp, r );
        }
        r >>= 1;
        rexp++;
        if (arithmetic_op_debug) fprintf( stderr, "add: C5: rexp=%d r=%018llo\n", rexp, r );
	if (rexp > 127) {
	    /* addition overflow  */
	    return STOP_ADDOVF;
        }
        if (arithmetic_op_debug) fprintf( stderr, "add: C7: rexp=%d r=%018llo\n", rexp, r );
    }


    /* normalization to left */
    if (!no_norm) {
        if (arithmetic_op_debug) fprintf( stderr, "add: D1: NORM_L: rexp=%d r=%018llo\n", rexp, r );
        for (;;) {
           if (r == 0) {
             /* Zero mantissa - make a null */
	     break;
           }
 	   if (r & BIT37)  break;
 	   fix_sign = 0;
 	   if (r & (SIGN<<1)) fix_sign =1 ;
	   r <<= 1;
	   r &= WORD45;
	   if (fix_sign) r |= SIGN<<1;
	   --rexp;
           if (arithmetic_op_debug) fprintf( stderr, "add: D5: rexp=%d r=%018llo\n", rexp, r );
	   if (rexp < 0) break;
        }
        if (arithmetic_op_debug) fprintf( stderr, "add: D9: rexp=%d r=%018llo\n", rexp, r );
    }

    if (arithmetic_op_debug) fprintf( stderr, "add: E1: rexp=%d r=%018llo\n", rexp, r );

    r >>= 1;
    if (arithmetic_op_debug) fprintf( stderr, "add: E3: rexp=%d r=%018llo\n", rexp, r );

    /* check for machine zero */
    if ((r == 0) || (rexp < 0)) {
      if (arithmetic_op_debug)  fprintf( stderr, "add: return NORMZERO: rexp=%d \n", rexp );
      r = norm_zero();
      goto final;
    }

    /* Make final result. */

    if (arithmetic_op_debug) fprintf( stderr, "add: FINAL 10: r=%018llo\n", r );

    r |= (t_value) rexp << BITS_36;
    if (arithmetic_op_debug) fprintf( stderr, "add: FINAL 11: r=%018llo\n", r );

    if (rs < 0) r |= SIGN;
    if (arithmetic_op_debug) fprintf( stderr, "add: FINAL 12: r=%018llo\n", r );


 final:
    r |= ((x | y) & TAG);
	*result = r;
    if (arithmetic_op_debug) fprintf( stderr, "add: FINAL 15: r=%018llo\n\n", r );

    return SCPE_OK;
}







#define  AUX_BIT_SHIFT     1


static int  get_number_sign( t_value num )
{
  return( num & SIGN ? -1 : 1 );
}



#define  MATH_OP_ADD       1
#define  MATH_OP_SUB       2
#define  MATH_OP_SUB_MOD   3

#if !defined(max)
t_value max(t_value a,t_value b)
{
  if (a>b)return a;
  else return b;
}
#endif

/*
 *  Addition,subtraction,subtraction by module arithmetic operations implementation.
 *  According to this book: Shura-Bura,Starkman pp. 70-75
 *  (russian edition, 1962)
 */
t_stat  new_arithmetic_op( t_value * result, t_value x, t_value y, int op_code )
{
   int math_op_type = 0, rr, e0, n0, j;
   int u, u1, sigma, sigma1, p, q, beta_round, beta_norm, rr_shift;
   int sign1, sign2, sign_x, sign_y, sign_z, v, v1, v2, delta, delta1, ro;
   t_value x1,y1, xx1, yy1, mask;
   t_int64 z;

   if (result == NULL) return STOP_INVARG;

   if (arithmetic_op_debug) fprintf( stderr, "op=%02o, x=%015llo, y=%015llo\n", op_code, x, y );

   /* STEP 0. Prepare source data */

   switch( op_code) {
	case OPCODE_ADD_ROUND_NORM:         /* 001 = сложение с округлением и нормализацией */
	case OPCODE_ADD_NORM:               /* 021 = сложение без округления с нормализацией */
	case OPCODE_ADD_ROUND:              /* 041 = сложение с округлением без нормализации */
	case OPCODE_ADD:                    /* 061 = сложение без округления и без нормализации */
            math_op_type = MATH_OP_ADD;
	    break;
	case OPCODE_SUB_ROUND_NORM:         /* 002 = вычитание с округлением и нормализацией */
	case OPCODE_SUB_NORM:               /* 022 = вычитание без округления с нормализацией */
	case OPCODE_SUB_ROUND:              /* 042 = вычитание с округлением без нормализации */
	case OPCODE_SUB:                    /* 062 = вычитание без округления и без нормализации */
            math_op_type = MATH_OP_SUB;
	    break;
	case OPCODE_SUB_MOD_ROUND_NORM:     /* 003 = вычитание модулей с округлением и нормализацией */
	case OPCODE_SUB_MOD_NORM:           /* 023 = вычитание модулей без округления с нормализацией */
	case OPCODE_SUB_MOD_ROUND:          /* 043 = вычитание модулей с округлением без нормализации */
	case OPCODE_SUB_MOD:                /* 063 = вычитание модулей без округления и без нормализации */
            math_op_type = MATH_OP_SUB_MOD;
	    break;
	default:
            math_op_type = -1;
	    break;
   }

   if (math_op_type < 0) return STOP_INVARG;

   if (math_op_type == MATH_OP_SUB_MOD) {
       u = -1; u1 = 1;
   }

   if (math_op_type == MATH_OP_SUB) {
       sign1 = get_number_sign(x);
       sign2 = get_number_sign(y);
       u1 = sign1;
       u = (-1)*(sign1 * sign2);
   }

   if (math_op_type == MATH_OP_ADD) {
       sign1 = get_number_sign(x);
       sign2 = get_number_sign(y);
       u1 = sign1;
       u = (sign1 * sign2);
   }

   sign_x = get_number_sign(x);
   sign_y = get_number_sign(y);

   if (arithmetic_op_debug)
     fprintf( stderr, "math_op_type=%d u=%d u1=%d sign_x=%d sign_y=%d x=%015llo y=%015llo\n",
                       math_op_type, u, u1, sign_x, sign_y, x, y );

   p = (x & EXPONENT) >> BITS_36;
   q = (y & EXPONENT) >> BITS_36;

   x1 = (x & MANTISSA);
   y1 = (y & MANTISSA);

   beta_round = (op_code >> 4 & 1);
   beta_norm = (op_code >> 5 & 1);

   if (arithmetic_op_debug)
     fprintf( stderr, "p=%d q=%d round=%d norm=%d x1=%015llo y1=%015llo\n", p,q,beta_round,beta_norm,x1,y1 );

   /* STEP 1. Preliminary analysis */

   if (u == -1) sigma = 1;
   if (u1 == -1) sigma1 = 1;
   if (u == 1) sigma = 0;
   if (u1 == 1) sigma1 = 0;

   if (is_norm_zero(x)) v1=1;
   else v1 = 0;

   if (is_norm_zero(y)) v2=1;
   else v2 = 0;

   v = v1 || v2;

   delta = p - q;
   delta1 = (delta == 0);
   ro = (delta <= 0);

   if (arithmetic_op_debug)
     fprintf( stderr, "sigma=%d sigma1=%d v1=%d v2=%d v=%d delta=%d delta1=%d ro=%d\n",
                       sigma, sigma1, v1, v2, v, delta, delta1, ro );


   /* STEP 2. Exponent alignment and get preliminary result */

   rr = max(p,q);

   n0 = (!beta_round)*(!v)*(!sigma)*(!delta1);
   e0 = n0;

   if (arithmetic_op_debug) fprintf( stderr, "rr=%d n0=%d e0=%d\n", rr, n0, e0 );

   if (delta == 0) {
     xx1 = x1 << AUX_BIT_SHIFT;
     yy1 = y1 << AUX_BIT_SHIFT;
     if (arithmetic_op_debug) fprintf( stderr, "DELTA==0: xx1=%015llo yy1=%015llo\n", xx1, yy1 );
   }

   if (delta < 0) {
     if (delta < -36) xx1=0;
     	else xx1 = (x1 << AUX_BIT_SHIFT) >> -delta;
     yy1 = y1 << AUX_BIT_SHIFT;
     yy1 |= n0;
     if (arithmetic_op_debug) fprintf( stderr, "DELTA<0: xx1=%015llo yy1=%015llo\n", xx1, yy1 );
   }

   if (delta > 0) {
     xx1 = x1 << AUX_BIT_SHIFT;
     xx1 |= e0;
     if (delta > 36) yy1=0;
     	else yy1 = (y1 << AUX_BIT_SHIFT) >> delta;
     if (arithmetic_op_debug) fprintf( stderr, "DELTA>0: xx1=%015llo yy1=%015llo\n", xx1, yy1 );
   }


   if (sigma == 0) z=xx1 + yy1;
   if (sigma == 1) z=xx1 - yy1;
   /*в реальной М-20 вычитание производится в обратном коде, а не в дополнительном, как в Си!
   С циклическим переносом и без прибавления 1.
   Но т.к. мы переходим потом вновь к прямым кодам чисел, это не играет роли. */



   /* get sign of preliminary result*/
   sign_z = (z<0) ? -1 : 1;

   if (sigma1 == 0) {
     if (arithmetic_op_debug) fprintf( stderr, "SIGMA1==0: sign_z=%d, z=%015llo\n", sign_z, z );
   }

   if (sigma1 == 1) {
     sign_z = -sign_z;
     if (arithmetic_op_debug) fprintf( stderr, "SIGMA1==1: sign_z=%d, z=%015llo\n", sign_z, z );
   }


  //в дальнейших вычислениях нам нужен модуль z
   if (z<0) z=-z;

   if (arithmetic_op_debug) fprintf( stderr, "after negate z=%015llo\n", z );
   z &= (MANTISSA|BIT37|BIT38);
   if (arithmetic_op_debug) fprintf( stderr, "z=%015llo\n", z );



   /* STEP 3. Construction code result */

   if (z & BIT38) {
	   //нормализация вправо при выходе за мантиссу
     z = z + (!beta_round)*1;
     if (arithmetic_op_debug) fprintf( stderr, "A1: z=%015llo rr=%d\n", z, rr );
     rr = rr + 1;
     if (rr > 127) return STOP_ADDOVF;
     z >>= 1;
     if (arithmetic_op_debug) fprintf( stderr, "A2: z=%015llo rr=%d\n", z, rr );
   }

   if (arithmetic_op_debug) fprintf( stderr, "TEMP: z=%015llo rr=%d\n", z, rr );

   // Вакуленко (=?)
   //if (beta_norm && (z & BIT37)) {
   // Шура-Бура
   if (beta_norm || (z & BIT37)) {
	   //если нормализация не нужна
       if (arithmetic_op_debug) fprintf( stderr, "B0: z=%015llo rr=%d\n", z, rr );
       //z >>= AUX_BIT_SHIFT;             //сдвиг на Др будет дальше
       //if (z != 0) rr = rr;
       if (arithmetic_op_debug) fprintf( stderr, "B1: z=%015llo rr=%d\n", z, rr );
       if (z == 0) {
           z = norm_zero();
           *result = z;
           if (arithmetic_op_debug) fprintf( stderr, "B1A FINAL: t=%015llo\n\n", z );
           return SCPE_OK;
       }
   }

   if (arithmetic_op_debug) fprintf( stderr, "TEMP: z=%015llo rr=%d\n", z, rr );

   // not so strong
   //if ((!beta_norm)) {
   // Шура-Бура
   if (!beta_norm && !(z & BIT38) && !(z & BIT37)) {
	   //нормализация влево
        if (arithmetic_op_debug) fprintf( stderr, "C0: z=%015llo rr=%d\n", z, rr );
         j = BITS_36;
         mask = (t_value)1 << j;
         if (arithmetic_op_debug) fprintf( stderr, "C1: m=%015llo j=%d\n", mask, j );
         while( j>0 && !(z & mask)) {
             j--;
             mask >>= 1;
         }
         rr_shift = BITS_36-j;
         if (arithmetic_op_debug) fprintf( stderr, "C2: m=%015llo j=%d, rr=%d, rr_shift=%d, rr-rr_shift=%d\n", mask, j, rr, rr_shift, rr - rr_shift );
         if ((rr - rr_shift) < 0) {
            z = norm_zero();
            *result = z;
            if (arithmetic_op_debug) fprintf( stderr, "C2A FINAL: t=%015llo\n\n", z );
            return SCPE_OK;
         }
         rr = rr - rr_shift;
         z <<= rr_shift;
         if (arithmetic_op_debug) fprintf( stderr, "C3: rr=%d zz=%015llo\n", rr, z );
         if (z == 0) {
            z = norm_zero();
            *result = z;
            if (arithmetic_op_debug) fprintf( stderr, "C3A FINAL: t=%015llo\n\n", z );
            return SCPE_OK;
         }
   }

   /* Final result writing */
   if (arithmetic_op_debug) fprintf( stderr, "FINAL 0: z=%015llo rr=%d\n", z, rr );
   z >>= AUX_BIT_SHIFT;
   z &= MANTISSA;
   if (arithmetic_op_debug) fprintf( stderr, "FINAL 1: z=%015llo rr=%d\n", z, rr );

   z = z | ((t_value)rr << BITS_36);
   if (sign_z<0) z |= SIGN;
   z |= (x|y) & TAG;

   if (arithmetic_op_debug) fprintf( stderr, "FINAL 2: t=%015llo\n\n", z );

   *result = z;

   return SCPE_OK;
}




/*
 *  Square root arithmetic operation implementation.
 *  According to this book: Shura-Bura,Starkman pp. 86-89
 *  (russian edition, 1962)
 */
t_stat new_arithmetic_square_root (t_value *result, t_value x, int op_code)
{
    int beta_round, p, rr, do_shift_right_one, i;
    t_value m, t, us, qs, qqs, n, zero, x1, zz;

    if (arithmetic_op_debug) fprintf( stderr, "NEW_SQRT: op_type=%d x=%015llo\n", op_code, x );

    if (x & SIGN) {
	/* square root from negative number */
	return STOP_NEGSQRT;
    }

    /* rounding flag */
    beta_round = op_code >> 4 & 1;

    /* extract mantissa */
    m = x & MANTISSA;
    zz = 0;

    /* extract exponent */
    p = x >> BITS_36 & 0177;

    if (arithmetic_op_debug) fprintf( stderr, "round=%d p=%d m=%015llo\n", beta_round, p, m );

    /* make new exponent */
    do_shift_right_one = 0;
    rr = (p >> 1) + M20_MANTISSA_SHIFT/2;
    if (p & 1) {
        rr += 1;
       do_shift_right_one = 1;
    }

    if (arithmetic_op_debug) fprintf( stderr, "rr=%d shift_right=%d\n", rr, do_shift_right_one );

    /* Build 37-bit mantissa */

    x1 = m;
    if (x1 == 0) {
        t = norm_zero();
		*result = t | (x & TAG);
        if (arithmetic_op_debug) fprintf( stderr, "ZERO: t=%015llo\n\n", t );
        return SCPE_OK;
    }
    zero = 0;
    n = 0;
    us = ((t_value)1 << BITS_36);
    us >>= 1;
    qs = 0;
    if (do_shift_right_one) {
      qs = 0 - x1;
    }
    else {
      qs = 0 - (x1 + x1);
    }

    if (arithmetic_op_debug) fprintf( stderr, "INIT: us=%015llo qs=%015llo\n", us, qs );

    for( i=1; i<37; i++ ) {
       qqs = qs + us + (n << 1);
       if (qqs & SIGN) {
           qs = (qqs << 1);
           n = n + us;
       }
       else {
          t = qqs - n - n - us;
          qs = (t << 1);
          n = n;
       }
       us >>= 1;
       if (arithmetic_op_debug) fprintf( stderr, "LOOP: us=%015llo qs=%015llo qqs=%015llo, n=%015llo\n", us, qs, qqs, n );
    }

    if (arithmetic_op_debug) fprintf( stderr, "DONE: us=%015llo qs=%015llo qqs=%015llo, n=%015llo\n", us, qs, qqs, n );

    zz = n;
    if (arithmetic_op_debug) fprintf( stderr, "READY: zz=%015llo rr=%d\n", zz, rr );

    zz = zz + (!beta_round)*1;
    if (arithmetic_op_debug) fprintf( stderr, "READY: zz=%015llo rr=%d\n", zz, rr );

    zz &= MANTISSA;
    if (arithmetic_op_debug) fprintf( stderr, "FINAL 0: zz=%015llo rr=%d\n", zz, rr );

    if (zz == 0) {
        t = norm_zero();
        *result = t;
        if (arithmetic_op_debug) fprintf( stderr, "FINAL ZERO 1: t=%015llo\n\n", t );
        return SCPE_OK;
    }

    /* Make final result */
    if (arithmetic_op_debug) fprintf( stderr, "FINAL 10: zz=%015llo rr=%d\n", zz, rr );

    t = zz | ((t_value)rr) << BITS_36;
    t |= x & TAG;
    if (arithmetic_op_debug) fprintf( stderr, "FINAL 12: t=%015llo\n\n", t );

    *result = t;

    return SCPE_OK;
}





/*
 *  Multiply arithmetic operation implementation.
 *  According to this book: Shura-Bura,Starkman pp. 77-82
 *  (russian edition, 1962)
 *  For more straightforward description of the algorithm refer to:
 *  M.A.Kartsev. Arifmetika cifrovykh mashin. Moscow, 1969. Ch. 4.2.2.
 *  p.364.
 */
t_stat new_arithmetic_mult_op (t_value *result, t_value x, t_value y, int op_code)
{
    int beta_round, beta_norm, rr, sign_zz, p, q, sign_x, sign_y, i, sign_sigma_r, delta_r, delta_r_ind; //rc присвоил delta_r int, чтобы не сбивался вывод
    t_value t, x1, y1, rr_lo, rr_lo_tmp;
	t_int64 rr_hi;	//rc для того, чтобы сдвиг был арифметический
	const t_value mask = 3;
    int delta_arr[4];

   if (result == NULL) return STOP_INVARG;

   if (arithmetic_op_debug) fprintf( stderr, "NEW MULT: op=%02o, x=%015llo, y=%015llo\n", op_code, x, y );

   beta_round = (op_code >> 4 & 1);
   beta_norm = (op_code >> 5 & 1);

   sign_x = get_number_sign(x);
   sign_y = get_number_sign(y);

   if (arithmetic_op_debug)
       fprintf( stderr, "round=%d norm=%d sign_x=%d sign_y=%d\n", beta_round, beta_norm, sign_x, sign_y );

   p = (x & (EXPONENT|EXPONENT_SIGN)) >> BITS_36;
   q = (y & (EXPONENT|EXPONENT_SIGN)) >> BITS_36;

   x1 = (x & MANTISSA);
   y1 = (y & MANTISSA);

   if (arithmetic_op_debug) fprintf( stderr, "p=%d, q=%d, x1=%015llo, y1=%015llo\n", p, q, x1, y1 );

   /* Step 1. Preliminary production */

   sign_zz = sign_x * sign_y;
   rr = p + q - M20_MANTISSA_SHIFT;

   rr_lo = 0;
   rr_hi = (!beta_round)*0200000000000LL; //1-я стадия округления
   /*Как объясняется в техописании, после 18 сдвигов эта единица 35-го разряда, складываясь
   в процессе умножения с суммой частичных произведений, попадает в 35-й разряд младшего регистра,
   что эквивалентно прибавлению 1 к нему. */

   /*массив весов, назначаемых очередной паре разрядов множителя */
   delta_arr[0]=0;
   delta_arr[1]=1;
   delta_arr[2]=2;
   delta_arr[3]=-1;

   if (arithmetic_op_debug)
     fprintf( stderr, "rr=%d sign_zz=%d rr_hi=%015llo \n",
                       rr, sign_zz, rr_hi );

   for( i=1; i<20; i++ ) {
       sign_sigma_r = get_number_sign(rr_hi);
	   delta_r_ind=(int)(x1&mask);
	   /*для кода "11" предыдущего разряда, когда выполнялось вычитание, выполняется +1
	   к весам текущего разряда по mod4, т.е. к текущей паре цифр множителя как бы прибавляется 1
       перед их анализом. Иными словами, используется представление 11=100-01	   */
       if (sign_sigma_r == -1) {
		   delta_r_ind++;
		   delta_r_ind &= 3;
	   }
	   delta_r=delta_arr[delta_r_ind];

       if (arithmetic_op_debug) {
         fprintf( stderr, "i=%d sign_sigma_r=%d delta_r=%d \n",
                           i, sign_sigma_r, delta_r );
       }
       t = delta_r * y1;

	   /*заполняем регистры младшего и старшего произведения, выдавливая по 2 бита в младший регистр */

	   rr_hi  += t;	
	   /* В реальной М-20 для умножения образуется регистр из СмЧ (36разрядов+Др) и присоединенного к нему
	   справа Р1 (36разрядов+Др). Т.е. за СмЧ располагается аккурат 38 разрядов, считая два Др-а.
       В эмуляторе двух дополнительных разрядов нет. Поэтому выдавливание двух разрядов в rr_lo нужно делать не 19,
	   а 18 раз, равно как и арифметический сдвиг старшего регистра.  */
	   if (i<19) {
		    rr_lo >>= 2;
			rr_lo_tmp = (rr_hi & 03LL) << 34;	//два младших разряда, которые потеряются при сдвиге.
												//сдвиг на 34, т.к. выдавливаний будет 18, а попасть надо в первые 2 разряда rr_lo
			rr_lo |= rr_lo_tmp;							//младший регистр готов

			rr_hi >>= 2;   //сдвиг обязательно должен быть арифметический, чтобы не сломалась работа с вычитанием!
	   }

       if (arithmetic_op_debug) fprintf( stderr, "t=%015llo rr_lo=%015llo rr_hi=%015llo\n\n",
                                                  t, rr_lo, rr_hi );
	   x1 >>= 2;
   }

   /* Step 2. Produce final result */

      if (arithmetic_op_debug) fprintf( stderr, "rr=%d rr_lo=%015llo rr_hi=%015llo\n", rr, rr_lo, rr_hi );

	/* Нормализация на один разряд влево. */
   if (!beta_norm && ! (rr_hi & 0400000000000LL)) {
	--rr;
	rr_hi <<= 1;
	rr_lo <<= 1;
	if (rr_lo & BIT37) {
            rr_hi += 1;
	    rr_lo &= MANTISSA;
	}
        if (arithmetic_op_debug)
          fprintf( stderr, "NORM_DONE: rr=%d rr_hi=%015llo rr_lo=%015llo rr_hi=%015llo\n", rr, rr_hi, rr_lo, rr_hi );
   }
   else {
	/* Доокругление по 38 разряду. См. разъяснения в обычной реализации умножения.*/
	if ((rr_lo & 0200000000000LL) && !beta_round) {
		rr_lo += BIT36;
		if (rr_lo & BIT37) {
				rr_hi += 1;
				rr_lo &= MANTISSA;
		}
	}
        if (arithmetic_op_debug)
          fprintf( stderr, "2nd_ROUND_DONE: rr=%d rr_hi=%015llo rr_lo=%015llo rr_hi=%015llo\n", rr, rr_hi, rr_lo, rr_hi );
   }

   if (arithmetic_op_debug)
     fprintf( stderr, "rr=%d rr_hi=%015llo rr_lo=%015llo rr_hi=%015llo\n", rr, rr_hi, rr_lo, rr_hi );


 /* Make final result */

	/* переполнение при умножении */
	//if ((rr_hi != 0) && (rr > 127)) {
	   //not so strong!
	if (rr > 127) return STOP_MULOVF;


	/* higher word processing */
   if ((rr_hi == 0) || (rr < 0)) {
     t = norm_zero();
     if (arithmetic_op_debug) fprintf( stderr, "FINAL: return NORMZERO_hi: t=%015llo\n\n", t );
   }
   else {
	 if (arithmetic_op_debug)
		fprintf( stderr, "FINAL 1: rr=%d rr_lo=%015llo rr_hi=%015llo rr_hi=%015llo\n", rr, rr_lo, rr_hi, rr_hi );
	 t = rr_hi & MANTISSA;
     t |= ((t_value)rr << BITS_36);
     if (sign_zz < 0) t |= SIGN;
   }
	t |= ((x | y) & TAG);

   /* lower word processing. Note that rr<0 checking is not described in techref, but if don't check it,
   we can't pass Multiply Test 5   */
   if (rr_lo == 0 || (rr < 0)) {
	   regRMR=norm_zero();
	   if (arithmetic_op_debug)
		   fprintf( stderr, "FINAL: return NORMZERO_lo: rr=%d rr_hi=%015llo, t==%015llo, regRMR=%015llo\n\n", rr, rr_hi, t, regRMR );
   }
   else {
	   regRMR = rr_lo;
       regRMR |= (t_value) rr << BITS_36;
       regRMR |= ((x ^ y) & SIGN);
   }
   regRMR |= ((x | y) & TAG);

   if (arithmetic_op_debug)
     fprintf( stderr, "FINAL 8: rr=%d rr_hi=%015llo, t==%015llo, regRMR=%015llo\n\n", rr, rr_hi, t, regRMR );

   *result = t;

   return SCPE_OK;
}





/*
 *  Division arithmetic operation implementation.
 *  This is non-restoring division.
 *  According to this books:
 *  1) Shura-Bura, Starkman (russian edition, 1962), pp. 82-86.
 *  2) Kartsev, Arifmetika cifrovykn mashin (rus. 1969), p.499, 504-505.
 *  3) Lebedev, Melnikov, General descript. of BESM, vol.1 (rus. 1959),
 *     p.150-151.
 *  4) https://en.wikipedia.org/wiki/Division_algorithm#Non-restoring_division
 *  Note that Shura-Bura book contains an error on p.85:
 *  we need 38 digits, not 37 (see comment below).
 *
 */
t_stat new_arithmetic_div_op (t_value *result, t_value x, t_value y, int op_code)
{
    int beta_round, rr, sign_zz, p, q, sign_x, sign_y, i, ak;
    t_value t, x1, y1, zz, qk;

   if (result == NULL) return STOP_INVARG;

   if (arithmetic_op_debug) fprintf( stderr, "NEW DIV: op=%02o, x=%015llo, y=%015llo\n", op_code, x, y );

   beta_round = (op_code >> 4 & 1);

   sign_x = get_number_sign(x);
   sign_y = get_number_sign(y);

   if (arithmetic_op_debug) fprintf( stderr, "div01: no_round=%d sign_x=%d sign_y=%d\n", beta_round, sign_x, sign_y );

   p = (x & (EXPONENT|EXPONENT_SIGN)) >> BITS_36;
   q = (y & (EXPONENT|EXPONENT_SIGN)) >> BITS_36;

   x1 = (x & MANTISSA);
   y1 = (y & MANTISSA);

   if (arithmetic_op_debug) fprintf( stderr, "div02: p=%d, q=%d, x1=%015llo, y1=%015llo\n", p, q, x1, y1 );

   if (x1 >= 2*y1)
       /* mantissa overflow on division */
       return STOP_DIVMOVF;

   if (is_zero(y1))
       /* division by zero */
       return STOP_DIVZERO;

   /* Step 1. Preliminary quotient */
   zz = 0;
   qk = x1;

   sign_zz = sign_x * sign_y;
   rr = p - q + M20_MANTISSA_SHIFT;

   if (arithmetic_op_debug)
     fprintf( stderr, "div03: rr=%d sign_zz=%d \n",
                       rr, sign_zz );

   /* We need 38 digits of quotient. From LSB they are:
   - one auxilary digit for rounding
   - 36 usual mantissa digits
   - one extra digit for leading "1" if x1 > y1.
   Every pass - one quotient digit.

   But during 39th pass the last useful 38th digit may be
   corrected (10-1=01). We can't get rid of
   39th pass, but must get rid of last digit.*/
   for( i=1; i<40; i++ ) {

	  /* look at the sign of new remainder */
      ak = (qk & SIGN ? -1 : 1 );

	  /* add/subtract divisor, depending of sign of the remainder */
      qk -= ak*y1;
      if (arithmetic_op_debug) fprintf( stderr, "div04: i=%d: qk=%015llo\n", i, qk );

	  /* generate quotient digit */
	  zz <<= 1;
      zz += ak;
	  if (arithmetic_op_debug) fprintf( stderr, "div05: i=%d: ak=%d qk=%015llo zz=%015llo\n",
                                                    i, ak, qk, zz );
	  /*double the remainder */
	  qk<<=1;
	  if (arithmetic_op_debug) fprintf( stderr, "div06: i=%d: qk=%015llo \n",
                                                 i, qk );
   }
   /*get rid of last wrong digit*/
   zz>>= 1;
   if (arithmetic_op_debug) fprintf( stderr, "div07: rr=%d zz=%015llo \n", rr, zz );

   /* Step 2. Produce final result */

   /* Normalisation.
   Should be aware that work is accomplished with 38-bit digits
   (with auxilary LSB for rounding). Right-hand normalisation criterion
   in this case is "1" in MSB (BIT38). */
   if (zz & BIT38) {
      zz >>= 1;
      rr = rr + 1;
	  if (arithmetic_op_debug) fprintf( stderr, "div: NORM_R: rr=%d zz=%015llo\n", rr, zz );
   }
    /* Round */
    zz += !(beta_round)*1;
    if (arithmetic_op_debug) fprintf( stderr, "div: ROUND: rr=%d zz=%015llo\n", rr, zz );

   /*Auxilary LSB truncate*/
   zz = (zz>>1);

   if ((zz == 0) || (rr < 0)) {
	/* Machine zero */
        t = norm_zero();
        t |= ((x | y) & TAG);
        *result = t;
        if (arithmetic_op_debug) fprintf( stderr, "FINAL 15 ZERO: t=%015llo\n\n", t );
	return SCPE_OK;
   }

   if (rr >= 128)
   /* Division overflow */
	   return STOP_DIVOVF;

   if (arithmetic_op_debug) fprintf( stderr, "FINAL 20: rr=%d zz=%015llo\n", rr, zz );

   t = zz & MANTISSA;
   t |= ((t_value)rr << BITS_36);
   if (sign_zz < 0) t |= SIGN;
   t |= (x|y) & TAG;

   if (arithmetic_op_debug) fprintf( stderr, "FINAL 22: rr=%d zz=%015llo t=%015llo\n\n", rr, zz, t  );

   *result = t;

   return SCPE_OK;
}





/*
 * Подготовка обращения к внешнему устройству.
 * В условном числе должен быть задан один из пяти видов работы:
 * барабан, лента, разметка ленты, печать или перфорация.
 */
t_stat ext_io_setup (int a1, int a2, int a3)
{
    ext_io_op = a1;
    ext_io_dev_zone_addr = a2;
    ext_io_ram_end = a3;

    if (ext_io_op & EXT_PUNCH) {
        if (ext_io_op & (EXT_DRUM|EXT_TAPE|EXT_TAPE_FORMAT)) {
            return STOP_PUNCHBADBITS;
        }
        return SCPE_OK;
	/* Вывод на перфокарты не поддерживается */
	//return STOP_PUNCHUNSUPP;
    }

    if (ext_io_op & EXT_PRINT) {
        if (ext_io_op & (EXT_DRUM|EXT_TAPE|EXT_TAPE_FORMAT)) {
            return STOP_PRINTBADBITS;
	}
        return SCPE_OK;
	/* Вывод на печать не поддерживается */
	//return STOP_PRINTUNSUPP;
    }

    if (ext_io_op & EXT_DRUM) {
        if (ext_io_op & (EXT_TAPE|EXT_TAPE_FORMAT|EXT_PRINT|EXT_PUNCH)) {
            return STOP_DRUMBADBITS;
        }
        return SCPE_OK;
        /* Работа с магнитным барабаном не поддерживается */
        //return STOP_DRUMUNSUPP;
    }


    if (ext_io_op & EXT_TAPE) {
       if (ext_io_op & (EXT_DRUM|EXT_TAPE_FORMAT|EXT_PRINT|EXT_PUNCH)) {
           return STOP_TAPEBADBITS;
       }
        return SCPE_OK;
       /* Работа с магнитной лентой не поддерживается */
       //return STOP_TAPEUNSUPP;
    }

    if (ext_io_op & EXT_TAPE_FORMAT) {
        if (ext_io_op & (EXT_DRUM|EXT_TAPE|EXT_PRINT|EXT_PUNCH)) {
            return STOP_TAPEFMTBADBITS;
        }
        return SCPE_OK;
        /* Разметка ленты не поддерживается */
        //return STOP_TAPEFMTUNSUPP;
    }

    return SCPE_OK;
}



/*
 * Выполнение обращения к внешнему устройству.
 * В случае ошибки чтения возвращается STOP_READERR.
 * Контрольная сумма записи накапливается в параметре sum.
 * Блокировка памяти (EXT_DIS_RAM) и блокировка контроля (EXT_DIS_CHECK) поддерживаются.
 */
t_stat ext_io_operation (int a1, t_value * sum)
{
    t_stat err;
    int  codes_num;

    ext_io_ram_start = a1;
    *sum = 0;

    if (ext_io_op & EXT_PUNCH) {
         int add_only_flag = 0;
         int disable_mem_access = 0;
         int disable_checksum = 0;
         if (ext_io_op & EXT_DIS_RAM) disable_mem_access = 1;
         if (ext_io_op & EXT_DIS_CHECK) disable_checksum = 1;
         if ((ext_io_op & EXT_PRINT) && (ext_io_op & EXT_PUNCH)) {
            if ((active_cdp == 0) && (active_lpt == 0)) return STOP_NOT_READY_PUNCH;
            if (active_cdp == 0) goto check_print_op;
            add_only_flag = 1;
         }
         codes_num = 0;
         err = punch_card (ext_io_ram_start, ext_io_ram_end, ext_io_dev_zone_addr, add_only_flag,
                           disable_mem_access, disable_checksum, &codes_num, sum );
        if (sim_deb && cpu_dev.dctrl)
	    fprintf (sim_deb, "cpu: err=%d, codes_num=%04o\n", err,codes_num);
         delay += (100000*codes_num);
         return err;
	 /* Вывод на перфокарты не поддерживается */
	 //return STOP_PUNCHUNSUPP;
    }

  check_print_op:
    if (ext_io_op & EXT_PRINT) {
         int print_type = PRINT_TYPE_DECIMAL;
         int add_only_flag = 0;
         int disable_mem_access = 0;
         int disable_checksum = 0;
         if (ext_io_op & EXT_DIS_STOP) print_type = PRINT_TYPE_OCTAL;
         if (enable_m20_print_ascii_text) {
           if (ext_io_op & EXT_TAPE_REV) {
	       /* Текстовая печать */
               print_type = PRINT_TYPE_TEXT;
           }
         }
         if (ext_io_op & EXT_DIS_RAM) disable_mem_access = 1;
         if (ext_io_op & EXT_DIS_CHECK) disable_checksum = 1;
         if ((ext_io_op & EXT_PRINT) && (ext_io_op & EXT_PUNCH)) {
           if (active_lpt == 0) {
             return STOP_NOT_READY_PRINT;
           }
           add_only_flag = 1;
         }
         codes_num = 0;
         err = write_line_printer (ext_io_ram_start, ext_io_ram_end, ext_io_dev_zone_addr,
                                   print_type, add_only_flag, disable_mem_access, disable_checksum,
                                   &codes_num );
        if (sim_deb && cpu_dev.dctrl)
	    fprintf (sim_deb, "cpu: err=%d, codes_num=%04o\n", err,codes_num);
         delay += (50000*codes_num);
         return err;
	 /* Вывод на печать не поддерживается */
	 //return STOP_PRINTUNSUPP;
    }

    if (ext_io_op & EXT_DRUM) {
	/* Барабан (МБ) */
        codes_num = 0;
	err = drum_io (sum,&codes_num);
        if (sim_deb && cpu_dev.dctrl)
	    fprintf (sim_deb, "cpu: err=%d, codes_num=%04o sum=%015llo\n", err,codes_num,*sum);
        delay += (40000+((double)codes_num)/6400);
	return err;
        /* Работа с магнитным барабаном не поддерживается */
        //return STOP_DRUMUNSUPP;
    }

    if (ext_io_op & EXT_TAPE) {
	/* Магнитная лента (МЛ) */
        codes_num = 0;
	err = mt_tape_io (sum,&codes_num);
        if (sim_deb && cpu_dev.dctrl)
	    fprintf (sim_deb, "cpu: err=%d, codes_num=%04o sum=%015llo\n", err,codes_num,*sum);
	delay += (75000+((double)codes_num/2500));
	return err;
       /* Работа с магнитной лентой не поддерживается */
       //return STOP_TAPEUNSUPP;
    }

    if (ext_io_op & EXT_TAPE_FORMAT) {
        codes_num = 0;
	err = mt_format_tape (sum,&codes_num,ext_io_ram_start,ext_io_ram_end);
        if (sim_deb && cpu_dev.dctrl)
	    fprintf (sim_deb, "cpu: err=%d codes_num=%04o\n", err, codes_num );
	delay += (75000+((double)codes_num/2500));
	return err;
        /* Разметка ленты не поддерживается */
        //return STOP_TAPEFMTUNSUPP;
    }

    return SCPE_OK;
}





/*
 * Execute one instruction, contained in register RK.
 */
t_stat cpu_one_inst ()
{
	int addr_tags, op, a1, a2, a3, n = 0, force_round = 0;
	t_value x, y, t;
	t_stat err;
	//unsigned __int64 t1;

	addr_tags = regRK >> BITS_42 & MAX_ADDR_TAG_VALUE;
	op = regRK >> BITS_36 & MAX_OPCODE_VALUE;
	a1 = regRK >> BITS_24 & MAX_ADDR_VALUE;
	a2 = regRK >> BITS_12 & MAX_ADDR_VALUE;
	a3 = regRK >> BITS_0  & MAX_ADDR_VALUE;

	/* Есля установлен соответствующий бит признака,
	 * к адресу добавляется значение регистра адреса. */
	if (addr_tags & 4) a1 = (a1 + regRA) & MAX_ADDR_VALUE;
	if (addr_tags & 2) a2 = (a2 + regRA) & MAX_ADDR_VALUE;
	if (addr_tags & 1) a3 = (a3 + regRA) & MAX_ADDR_VALUE;


	/* test for memory contents overflow */
        if (memory_45_checking) {
	  t = mosu_load(a1);
	  if (t & ~WORD45) {
            if (sim_deb && cpu_dev.dctrl)
	      fprintf (sim_deb, "cpu: OVERFLOW BEFORE: a1: t[%04o]=%018llo, t=%018llo\n", a1, t, t & ~WORD45 );
            return STOP_MEMORY_GARBAGE_DETECTED;
          }
	  t = mosu_load(a2);
	  if (t & ~WORD45) {
            if (sim_deb && cpu_dev.dctrl)
	      fprintf (sim_deb, "cpu: OVERFLOW BEFORE: a2: t[%04o]=%018llo, t=%018llo\n", a2, t, t & ~WORD45  );
            return STOP_MEMORY_GARBAGE_DETECTED;
          }
	  t = mosu_load(a3);
	  if (t & ~WORD45) {
            if (sim_deb && cpu_dev.dctrl)
	      fprintf (sim_deb, "cpu: OVERFLOW BEFORE: a3: t[%04o]=%018llo, t=%018llo\n", a3, t, t & ~WORD45  );
            return STOP_MEMORY_GARBAGE_DETECTED;
          }
        }

	switch (op) {
	default:
	        delay += 24.0;
		return STOP_BADCMD;

	/*
         *   Numbers Operations
         *   (операции над числами)
         */

	case OPCODE_ADD_ROUND_NORM:         /* 001 = сложение с округлением и нормализацией */
	case OPCODE_ADD_NORM:               /* 021 = сложение без округления с нормализацией */
	case OPCODE_ADD_ROUND:              /* 041 = сложение с округлением без нормализации */
	case OPCODE_ADD:                    /* 061 = сложение без округления и без нормализации */
add:	x = mosu_load (a1);
		y = mosu_load (a2);
		if (new_add) {
          err = new_arithmetic_op( &regRR, x, y, op );
		  goto add_final;
		}
add_v44:
        err = new_addition_v44 (&regRR, x, y, op >> 4 & 1, op >> 5 & 1, force_round);

add_final:
		if (err) return err;
		mosu_store (a3, regRR);
		trgSW = (regRR & SIGN) != 0;
		delay += 28.5;
		break;


	case OPCODE_SUB_ROUND_NORM:         /* 002 = вычитание с округлением и нормализацией */
	case OPCODE_SUB_NORM:               /* 022 = вычитание без округления с нормализацией */
	case OPCODE_SUB_ROUND:              /* 042 = вычитание с округлением без нормализации */
	case OPCODE_SUB:                    /* 062 = вычитание без округления и без нормализации */
        if (new_add) goto add;

		x = mosu_load (a1);
        y = mosu_load (a2);
		if (arithmetic_op_debug) fprintf(stderr,"sub01: y=%15llo \n",y);

		/* When one of operands is machine zero, rounding should not be performed.
		But if we take machine zero as 2nd operand, invert the sign and call addition,
		it already will not be machine zero and can be rounded. Opposite case also may occur,
		when the 2nd operand is -0 and will not be rounded after sign inversion (as machine zero).

		Non-inverting the sign of machine zero and then call addition is wrong decision,
		because in this case rounding logic (based on signs) may became broken for -0.
		Same problems if non-inverting the sign of both -0 and machine zero.
		Right way is to detect machine zero and in this case switch rounding off
		(by opcode correction), then call addition.

		When the 2nd operand is -0, and the 1st is positive, rounding should be ON.
		To do this, inverting/noninverting the sign is unsufficient. When to decide
		round or not, it should be known: ADD or SUB is calculating now.

		So, LOY introduced forced rounding as a feature of new_addition_v44,
		and set it as "1" on conditions listed above.

		This -0 subtraction didn't covered by General Arithmetic Test 6 of 1963,
		but tested in test_02_w.simh and also seperetely by LOY.*/


		if (is_norm_zero(y)) {
			//bit5 = 1 in opcode means round OFF
			if (arithmetic_op_debug) fprintf(stderr,"sub02a: NORMZERO DETECTED, opcode=%o, ",op);
			op |= 0b10000;
			if (arithmetic_op_debug) fprintf(stderr,"modified opcode=%o \n",op);
		}
		// "-0" processing
		if ( !((y & SIGN) == 0) && ((y & MANTISSA) == 0) && ((y & EXPONENT) == 0)) {
		    if ( ((x & SIGN) == 0) && !(is_norm_zero(x))) force_round = 1;
		    if (arithmetic_op_debug) fprintf(stderr,"sub02b: MINUSZERO DETECTED, force_round=%d \n",force_round);
		}


		y ^= SIGN;
		if (arithmetic_op_debug) fprintf(stderr,"sub03: y_after_inversion=%15llo \n",y);

		goto add_v44;


	case OPCODE_SUB_MOD_ROUND_NORM:     /* 003 = вычитание модулей с округлением и нормализацией */
	case OPCODE_SUB_MOD_NORM:           /* 023 = вычитание модулей без округления с нормализацией */
	case OPCODE_SUB_MOD_ROUND:          /* 043 = вычитание модулей с округлением без нормализации */
	case OPCODE_SUB_MOD:                /* 063 = вычитание модулей без округления и без нормализации */
	    {
        if (new_add) goto add;

        int no_norm = 1;
        if ((op==003) || (op==023)) no_norm=0;

		x = mosu_load (a1) & ~SIGN;
		y = mosu_load (a2) | SIGN;
        err = new_addition_v44 (&regRR, x, y, 1, no_norm, force_round);
		goto add_final;
	     }

	case OPCODE_MULT_ROUND_NORM:        /* 005 = умножение с округлением и нормализацией */
	case OPCODE_MULT_NORM:              /* 025 = умножение без округления с нормализацией */
	case OPCODE_MULT_ROUND:             /* 045 = умножение с округлением без нормализации */
	case OPCODE_MULT:                   /* 065 = умножение без округления и без нормализации */
		x = mosu_load (a1);
		y = mosu_load (a2);
                if (new_mult) err = new_arithmetic_mult_op (&regRR, x, y, op);
                else err = multiplication (&regRR, x, y, op >> 4 & 1, op >> 5 & 1);
		if (err) return err;
		mosu_store (a3, regRR);
		trgSW = (int) (regRR >> BITS_36 & 0177) > 0100;
		delay += 69.5;
		break;


	case OPCODE_DIV_ROUND_NORM:         /* 004 = деление с округлением */
	case OPCODE_DIV_NORM:               /* 024 = деление без округления */
		x = mosu_load (a1);
		y = mosu_load (a2);
                if (new_div) err = new_arithmetic_div_op (&regRR, x, y, op);
                else err = division (&regRR, x, y, op >> 4 & 1);
		if (err) return err;
		mosu_store (a3, regRR);
		trgSW = (int) (regRR >> BITS_36 & 0177) > 0100;
		delay += 136.5;
		break;


	case OPCODE_SQRT_ROUND_NORM:        /* 044 = извлечение корня с округлением */
	case OPCODE_SQRT_NORM:              /* 064 = извлечение корня без округления */
		x = mosu_load (a1);
                if (new_sqrt) err = new_arithmetic_square_root (&regRR, x, op);
                else err = square_root (&regRR, x, op >> 4 & 1);
		if (err) return err;
		mosu_store (a3, regRR);
		trgSW = (int) (regRR >> BITS_36 & 0177) > 0100;
		delay += 275.0;
		break;


	case OPCODE_OUT_LOWER_BITS_OF_MULT: /* 047 = выдача младших разрядов произведения */
                                            /* Use only after op 025 or 065, but in real all otherwise */
                switch( old_opcode ) {
	            case OPCODE_MULT_ROUND_NORM:        /* 005 = умножение с округлением и нормализацией */
	            case OPCODE_MULT_NORM:              /* 025 = умножение без округления с нормализацией */
	            case OPCODE_MULT_ROUND:             /* 045 = умножение с округлением без нормализации */
	            case OPCODE_MULT:                   /* 065 = умножение без округления и без нормализации */
		        regRR = regRMR;
                        trgSW = (int) (regRR >> BITS_36 & 0177) > 0100;
	                break;
	            default:
                        //t = regRR & EXP_SIGN_TAG;
                        //fprintf( stderr, "regP1=%015llo\n", regP1 );
                        t = (regRR & EXP_SIGN_TAG) | (regP1 & MANTISSA);
                        regRR = t;
                        trgSW = (regRR & MANTISSA) == 0;
	                break;
	        }
                //trgSW = (int) (regRR >> BITS_36 & 0177) > 0100;
		mosu_store (a3, regRR);
		//trgSW = (regRR & MANTISSA) == 0;
		//if (trgSW) goto sw1;
		//if (!trgSW) trgSW = (int) (regRR >> BITS_36 & 0177) > 0100;
             //sw1:
		delay += 24.0;
		break;


	case OPCODE_ADD_ADDR_TO_EXP:        /* 006 = сложение порядка с адресом */
		n = (a1 & 0177) - M20_MANTISSA_SHIFT;
		y = mosu_load (a2);
		delay += 61.5;
addexp:
                err = add_exponent (&regRR, y, n, op);
		if (err) return err;
		mosu_store (a3, regRR);
		trgSW = (int) (regRR >> BITS_36 & 0177) > 0100;
		break;

	case OPCODE_ADD_EXP_TO_EXP:         /* 026 = сложение порядков чисел */
		delay += 24.0;
                x = mosu_load (a1);
		n = (int) (x >> BITS_36 & 0177) - M20_MANTISSA_SHIFT;
                y = mosu_load (a2);
		goto addexp;

	case OPCODE_SUB_ADDR_FROM_EXP:      /* 046 = вычитание адреса из порядка */
		delay += 61.5;
		n = M20_MANTISSA_SHIFT - (a1 & 0177);
		y = mosu_load (a2);
		goto addexp;

	case OPCODE_SUB_EXP_FROM_EXP:       /* 066 = вычитание порядков чисел */
		delay += 24.0;
                x = mosu_load (a1);
		n = M20_MANTISSA_SHIFT - (int) (x >> BITS_36 & 0177);
                y = mosu_load (a2);
		goto addexp;


	/*
         *   Codes Operations
         *   (операции над кодами)
         */

	case OPCODE_TRANSFER_MEM2MEM: /* 000 = пересылка */
	    regRR = mosu_load (a1);
	    mosu_store (a3, regRR);
	    /* w не изменяется и нет авто-останов */
	    delay += 24.0;
	    break;


	case OPCODE_LOAD_FROM_KEY_REGISTER:    /* 020 = чтение пультовых тумблеров */
		switch (a1 & 7) {
		  case 0: regRR = 0;    break;
		  case 1: regRR = RPU1; break;
		  case 2: regRR = RPU2; break;
		  case 3: regRR = RPU3; break;
		  case 4: regRR = RPU4; break;
		  case 5: /* RR */   break;
		  default:
                    return STOP_INVARG; /* неверный аргумент команды СЧП */
		}
		mosu_store (a3, regRR);
		/* w не изменяется. */
		delay += 24.0;
		break;


	case OPCODE_BLANKING_040:           /* 040 = гашение */
#if 1
                if (enable_opcode_040_hack) {
		  delay += 24.0;
                  x = mosu_load (a1);
                  //n = (x >> 13) & 07777;
                  n = (x >> BITS_12) & 07777;
		  if (regRA < n) regKRA = a2;
		  regRA = a3;
	          break;
	        }
#endif
	        regRR = 0;
	        mosu_store( a3, regRR );
		/* w не изменяется. */
                delay += 24.0;
		break;

	case OPCODE_BLANKING_060:           /* 060 = гашение */
	        regRR = 0;
	        mosu_store( a3, regRR );
		/* w не изменяется. */
		delay += 24.0;
		break;


	case OPCODE_COMPARE:                /* 015 = поразрядное сравнение (исключающее или) */
	case OPCODE_COMPARE_WITH_STOP:      /* 035 = поразрядное сравнение с остановом */
		regRR = mosu_load (a1) ^ mosu_load (a2);
logop:
		trgSW = (regRR == 0);
		delay += 24.0;
		if (op == 035 && !trgSW)  return STOP_ASSERT; /* останов по несовпадению */
                mosu_store (a3, regRR);     /* 035 must no store result, only from engineering panel! */
		break;

	case OPCODE_LOGICAL_MULT:           /* 055  = логическое умножение (и) = AND */
		regRR = mosu_load (a1) & mosu_load (a2);
		goto logop;

	case OPCODE_LOGICAL_ADD:            /* 075 = логическое сложение (или) = OR */
		regRR = mosu_load (a1) | mosu_load (a2);
		goto logop;


	case OPCODE_ADD_CMDS:       /* 013 = сложение команд */
		x = mosu_load (a1);
		y = mosu_load (a2);
		y = (x & MANTISSA) + (y & MANTISSA);
addm:
                regRR = (x & ~MANTISSA & WORD45) | (y & MANTISSA);
		mosu_store (a3, regRR);
                trgSW = (y & BIT37) != 0;
		//if (op == 013) trgSW = (y & BIT37) != 0;
                //if (op == 033) trgSW = (regRR & SIGN) != 0; //???
		delay += 24.0;
		break;

	case OPCODE_SUB_CMDS:       /* 033 = вычитание команд */
		x = mosu_load (a1);
		y = mosu_load (a2);
		y = (x & MANTISSA) - (y & MANTISSA);
		goto addm;


	case OPCODE_ADD_OPCS:      /* 053 = сложение кодов операций */
		x = mosu_load (a1);
		y = mosu_load (a2);
		y = (x & ~MANTISSA) + (y & ~MANTISSA);
addop:
                regRR = (x & MANTISSA) | (y & ~MANTISSA & WORD45);
		mosu_store (a3, regRR);
                trgSW = (y & BIT46) != 0;
		//if (op == 053) trgSW = (y & BIT46) != 0;
                //if (op == 073) trgSW = (regRR & SIGN) != 0;
		delay += 24.0;
		break;

	case OPCODE_SUB_OPCS:      /* 073 = вычитание кодов операций */
		x = mosu_load (a1);
		y = mosu_load (a2);
		y = (x & ~MANTISSA) - (y & ~MANTISSA);
		goto addop;


	case OPCODE_SHIFT_MANTISSA_BY_ADDR:      /* 014 = сдвиг мантиссы по адресу */
		n = (a1 & 0177) - M20_MANTISSA_SHIFT;
		delay += 61.5 + 1.5 * (n>0 ? n : -n);
shm:
                y = mosu_load (a2);
		regRR = (y & ~MANTISSA);
		//fprintf( stderr, "n=%d y=%015lo, regRR=%015llo\n", n, y, regRR );
		if ((n < 36) && (-n < 36)) {		//linux bugfix. 36 is mantissa length
		    if (n >= 0) regRR |= (((y & MANTISSA) << n) & MANTISSA);
		    else if (n < 0) regRR |= (((y & MANTISSA) >> -n) & MANTISSA);
		}
                //regRR &= WORD45;
		mosu_store (a3, regRR);
		trgSW = ((regRR & MANTISSA) == 0);
		break;

	case OPCODE_SHIFT_MANTISSA_BY_EXP:    /* 034 = сдвиг мантиссы по порядку числа */
		n = (int) (mosu_load (a1) >> BITS_36 & 0177) - M20_MANTISSA_SHIFT;
		delay += 24.0 + 1.5 * (n>0 ? n : -n);
		goto shm;


	case OPCODE_SHIFT_CODE_BY_ADDR:       /* 054 = сдвиг по адресу */
		n = (a1 & 0177) - M20_MANTISSA_SHIFT;
		delay += 61.5 + 1.5 * (n>0 ? n : -n);
shift:
		if ((n < 45) && (-n < 45)) {		//linux bugfix. 45 is machine word length
            	    regRR = mosu_load (a2);
		    if (n > 0) regRR = (regRR << n);
		    else if (n < 0) regRR >>= -n;
            	    regRR &= WORD45;
                }
                else regRR = 0;
		mosu_store (a3, regRR);
		trgSW = (regRR == 0);
		break;

	case OPCODE_SHIFT_CODE_BY_EXP:        /* 074 = сдвиг по порядку числа */
		n = (int) (mosu_load (a1) >> BITS_36 & 0177) - M20_MANTISSA_SHIFT;
		delay += 24 + 1.5 * (n>0 ? n : -n);
		goto shift;

	case OPCODE_ADD_CYCLIC:        /* 007 = циклическое сложение */
		x = mosu_load (a1);
		y = mosu_load (a2);
	//cyclic_sum:
		regRR = (x & ~MANTISSA) + (y & ~MANTISSA);
		t = (x & MANTISSA) + (y & MANTISSA);
		trgSW = (t & BIT37) != 0;
                if (op == 007) {
                  if (regRR & BIT46) regRR += BIT37;
		  if (t & BIT37) t += 1;
                  regRR &= WORD45;
		}
		if (op == 027) {
                  if (regRR & BIT46) regRR &= ~BIT37;
		  if (t & BIT37) t &= ~1;
		}
		//regRR &= WORD45;
		regRR |= (t & MANTISSA);
		mosu_store (a3, regRR);
		delay += 24.0;
		break;

	case OPCODE_SUB_CYCLIC:        /* 027 = циклическое вычитание */
		x = mosu_load (a1);
		y = mosu_load (a2);
#if 0
		y = mosu_load (a2);
		y = BIT46 - y;
		goto cyclic_sum;
#endif
#if 0
		regRR = (x & ~MANTISSA) - (y & ~MANTISSA);
		t = (x & MANTISSA) - (y & MANTISSA);
                trgSW = (t & BIT37) != 0;
                //if (((x & MANTISSA) - (y & MANTISSA)) == 0) trgSW=1;
                if (regRR & BIT46) regRR -= BIT37;
		if (t & BIT37) t -= 1;
		regRR &= WORD45;
		regRR |= t & MANTISSA;
#endif
#if 0
		regRR = (x & ~MANTISSA) - (y & ~MANTISSA);
		t = (x & MANTISSA) - (y & MANTISSA);
                if (regRR & BIT46) {
                  regRR -= BIT37;
                  if (t & BIT37) t -= 1;
                  //t += BIT37;
                  //t = -1;
                }
		//if (t & BIT37) t -= 1;
#endif
                t = BIT37 + (x & MANTISSA) - (y & MANTISSA);
                //fprintf( stderr, "xm=%015llo ym=%015llo t=%015llo\n", x & MANTISSA, y&MANTISSA,t );
                if (t & BIT37) t -= BIT37;
                regRR = (x & ~MANTISSA) - (y & ~MANTISSA);
                if (regRR & BIT46) {
                  regRR -= BIT37;
                  t -= 1;
                }
                trgSW = (t & BIT37) != 0;
		regRR |= t & MANTISSA;
		regRR &= WORD45;
		mosu_store (a3, regRR);
		delay += 24.0;
		break;

	case OPCODE_SHIFT_CYCLIC:      /* 067 = циклический сдвиг */
		x = mosu_load (a1);
                //regRR = (x & 077777777) << BITS_24 | (x >> BITS_24 & 07777777);
                regRR = (x & 07777777)  << BITS_24 | (x >> BITS_24 & 07777777);
		//regRR &= WORD45;
		mosu_store (a3, regRR);
                trgSW = (a3 == 0);
		/* w не изменяется (неверно). */
		//delay += 60.0;
                delay += 24.0;
		break;



	/*
         *   Control Operations
         *   (операции управления)
         */

        case OPCODE_STOP_037:    /* 037 = останов машины, в ИТЭФ-режиме: ФА - формирование адресов */
	if (itep_mode) {
		t_value newcmd;
		x = mosu_load (a1) >> BITS_12 & MAX_ADDR_VALUE;
		y = mosu_load (a2) >> BITS_12 & MAX_ADDR_VALUE;
		t = mosu_load (a3) >> BITS_12 & MAX_ADDR_VALUE;
		fprintf(stderr,"itep_FA1: (A)2=%04llo, (B)2=%04llo, (C)2=%04llo\n",x,y,t);
		newcmd=mosu_load (regKRA);
		fprintf(stderr,"itep_FA2: updKRA=%04o, (updKRA)=%015llo\n",regKRA,newcmd);
		x += newcmd >> BITS_24 & MAX_ADDR_VALUE;
		x &= MAX_ADDR_VALUE;
		y += newcmd >> BITS_12 & MAX_ADDR_VALUE;
		y &= MAX_ADDR_VALUE;
		t += newcmd & MAX_ADDR_VALUE;
		t &= MAX_ADDR_VALUE;
		fprintf(stderr,"itep_FA3: (A)2+K=%04llo, (B)2+L=%04llo, (C)2+M=%04llo\n",x,y,t);
		regRK = newcmd & EXP_SIGN_TAG;
		regRK |= x << BITS_24;
		regRK |= y << BITS_12;
		regRK |=t;
		fprintf(stderr,"itep_FA4: regRK=%015llo\n",regRK);
		regKRA +=1;
		regKRA &= MAX_ADDR_VALUE;
		err = irregular_cmd();
		fprintf(stderr,"itep_FA5: nextKRA=%04o \n\n",regKRA);
		if (err) return err;
		break;
	} 
        case OPCODE_STOP_017:    /* 017 = останов машины */
        case OPCODE_STOP_057:    /* 057 = останов машины */
	case OPCODE_STOP_077:    /* 077 = останов машины */
		delay += 24.0;
		regRR = 0;
		mosu_store (a3, regRR);
		/* Если адреса равны 0, считаем что это штатная, "хорошая" остановка. (?!) */
		return STOP_STOP;


	case OPCODE_CHANGE_RA_BY_ADDR :     /* 052 = установка регистра адреса адресом */
		regRR = 052000000000000LL | (a1 << BITS_12);
		mosu_store (a3, regRR);
		regRA = a2;
		//delay += 24.0;
                delay += 28.5;
		break;

	case OPCODE_CHANGE_RA_BY_CODE :     /* 072 = установка регистра адреса числом */
		regRR = 052000000000000LL | (a1 << BITS_12);
		mosu_store (a3, regRR);
		regRA = mosu_load (a2) >> BITS_12 & MAX_ADDR_VALUE;
		//delay += 24.0;
                delay += 28.5;
		break;


	case OPCODE_JUMP_WITH_RETURN:       /* 016 = передача управления с возвратом */
		regRR = 016000000000000LL | (a1 << BITS_12);
		regKRA = a2;
		mosu_store (a3, regRR);
		delay += 24.0;
		break;

	case OPCODE_COND_JUMP_BY_SIG_W_1:   /* 036 = передача управления по условию w=1 */
		regRR = mosu_load (a1);
		if (trgSW) regKRA = a2;
		mosu_store (a3, regRR);
		delay += 24.0;
		break;

	case OPCODE_JUMP_BY_ADDR:           /* 056 = передача управления */
		regRR = mosu_load (a1);
		regKRA = a2;
		mosu_store (a3, regRR);
		delay += 24.0;
		break;

	case OPCODE_COND_JUMP_BY_SIG_W_0:   /* 076 = передача управления по условию w=0 */
		regRR = mosu_load (a1);
		if (!trgSW) regKRA = a2;
		mosu_store (a3, regRR);
		delay += 24.0;
		break;


	case OPCODE_GOTO_AFTER_CYCLE_BY_PA_012:   /* 012 = переход по < */
		if (regRA < (unsigned)a1) regKRA = a2;
		regRA = a3;
		delay += 24.0;
		break;

	case OPCODE_GOTO_AFTER_CYCLE_BY_PA_032:   /* 032 = переход по >= */
                if (regRA >= (unsigned)a1) regKRA = a2;
		regRA = a3;
		delay += 24.0;
		break;


	case OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_1_011:    /* 011 = переход по < и w=1 */
                if (regRA < (unsigned)a1 && trgSW) regKRA = a2;
		regRA = a3;
		delay += 24.0;
		break;

	case OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_1_031:    /* 031 = переход по >= и w=1 */
		if (regRA >= (unsigned)a1 && trgSW) regKRA = a2;
		regRA = a3;
		delay += 24.0;
		break;

	case OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_0_051:    /* 051 = переход по < и w=0 */
                if (regRA < (unsigned)a1 && !trgSW) regKRA = a2;
		regRA = a3;
		delay += 24.0;
		break;

	case OPCODE_GOTO_AFTER_CYCLE_BY_PA_SIG_W_0_071:    /* 071 = переход по >= и w=0 */
		if (regRA >= (unsigned)a1 && !trgSW) regKRA = a2;
		regRA = a3;
		delay += 24.0;
		break;



	/*
         *   Input/Output Operations
         *   (операции обмена между накопителями)
         */

	case OPCODE_INPUT_CODES_FROM_PUNCH_CARDS_WITH_STOP:   /* 010 = ввод с перфокарт */
                cr_io_addr_1 = a1;
                cr_io_addr_2 = a2;
                cr_io_addr_3 = a3;
                cdr_csum = 0;
                cdr_rsum = 0;
                cdr_rcodes = 0;
                cdr_stop_blocking = 0;
                cdr_control_blocking = 0;
                if (sim_deb && cpu_dev.dctrl)
	            fprintf (sim_deb, "cpu: opcode=10: regKRA=%d,a1=%d,a2=%d,a3=%d\n", regKRA,a1,a2,a3);
                /* check for boot operation request from card reader device */
                if (boot_device_req_cdr) {
                   if (sim_deb && cpu_dev.dctrl) fprintf (sim_deb, "cpu: cdr boot detected. Set regKRA=%d\n", a1);
                   regKRA = a1;
                   boot_device_req_cdr = 0;
                }
                err = read_card(&cdr_csum,&cdr_rsum,&cdr_rcodes,&cdr_stop_blocking,&cdr_control_blocking);
		if (err) {
		    if (err == STOP_CRBADSUM) {
		      /* A1 must contain last address code of input */
		    }
                    return err;
                }
		delay += (50000*cdr_rcodes);
		if (cdr_control_blocking) goto store_chksum;
		if (cdr_stop_blocking) {
                  regKRA = a2;
                  goto store_chksum;
		}
		if (cdr_csum != cdr_rsum) {
		  regKRA = a2;
                  return STOP_CRBADSUM;
                }
             store_chksum:
		mosu_store (a3, cdr_csum);
		break;

	case OPCODE_INPUT_CODES_FROM_PUNCH_CARDS:   /* 030 = ввод с перфокарт останова после проверки к.суммы */
                cr_io_addr_1 = a1;
                cr_io_addr_2 = a2;
                cr_io_addr_3 = a3;
                cdr_csum = 0;
                cdr_rsum = 0;
                cdr_rcodes = 0;
                cdr_stop_blocking = 0;
                cdr_control_blocking = 0;
                if (sim_deb && cpu_dev.dctrl)
	            fprintf (sim_deb, "cpu: opcode=30: regKRA=%d,a1=%d,a2=%d,a3=%d\n", regKRA,a1,a2,a3);
                err = read_card(&cdr_csum,&cdr_rsum,&cdr_rcodes,&cdr_stop_blocking,&cdr_control_blocking);
		if (err) return err;
		delay += (50000*cdr_rcodes);
		if (cdr_control_blocking) goto store_chksum_30;
		if (cdr_csum != cdr_rsum) {
		  regKRA = a2;
                }
             store_chksum_30:
		mosu_store (a3, cdr_csum);
		break;


	case OPCODE_IO_EXT_DEV_TO_MEM_050:  /* 050 = подготовка обращения к внешнему устройству */
		err = ext_io_setup (a1, a2, a3);
		if (err) return err;
		delay += 24.0;
		break;

	case OPCODE_IO_EXT_DEV_TO_MEM_070:  /* 070 = выполнение обращения к внешнему устройству */
                if (sim_deb && cpu_dev.dctrl)
	             fprintf (sim_deb, "cpu: ext_io_op=%04o\n", ext_io_op);
		if (ext_io_op == MAX_ADDR_VALUE) return STOP_IO_MISSING_SETUP;
		err = ext_io_operation (a1, &regRR);
                if (a3) mosu_store (a3, regRR);
		if (err) {
		   if (err == STOP_READERR) {
		       /* A1 must contain last location address of successful input */
		   }
		   if (err == STOP_TAPEREADERR) {
		       /* A1 must contain last zone number of successful input */
		   }
		   if (err != STOP_READERR || !(ext_io_op & EXT_DIS_STOP)) return err;
		   if (ext_io_op & (EXT_PUNCH|EXT_PRINT)) goto skip_done;
		   if (a2) regKRA = a2;
		  skip_done: ;
		}
		delay += 24.0;
		break;
	}


	/* test for memory contents overflow */
	if (memory_45_checking) {
	  t = mosu_load(a1);
	  if (t & ~WORD45) {
            if (sim_deb && cpu_dev.dctrl)
	      fprintf (sim_deb, "cpu: OVERFLOW AFTER: a1: t[%04o]=%018llo, t=%018llo\n", a1, t, t & ~WORD45 );
            return STOP_MEMORY_GARBAGE_DETECTED;
          }
	  t = mosu_load(a2);
	  if (t & ~WORD45) {
            if (sim_deb && cpu_dev.dctrl)
	      fprintf (sim_deb, "cpu: OVERFLOW AFTER: a2: t[%04o]=%018llo, t=%018llo\n", a2, t, t & ~WORD45  );
            return STOP_MEMORY_GARBAGE_DETECTED;
          }
	  t = mosu_load(a3);
	  if (t & ~WORD45) {
            if (sim_deb && cpu_dev.dctrl)
	      fprintf (sim_deb, "cpu: OVERFLOW AFTER: a3: t[%04o]=%018llo, t=%018llo\n", a3, t, t & ~WORD45  );
            return STOP_MEMORY_GARBAGE_DETECTED;
          }
        }

	//ext_io_op = MAX_ADDR_VALUE;

	return SCPE_OK;
}



void print_commad_run_profile_stat(void)
{
   int i, op;
   double sum_time, sum_count;

   printf("\n*** Command time profile stat ***\n");
   sum_time = sum_count = 0;
   for( i=0; i<M20_SYM_OPCODE_TABLE_SIZE; i++ ) {
       op = cmd_profile_table[i].op_code;
       if (cmd_profile_table[i].us_count > 0) {
         sum_time += cmd_profile_table[i].us_time;
         sum_count += cmd_profile_table[i].us_count;
         printf("opcode=%02o   count=%-9.0f  times=%-15.2f  avg_time=%-15.2f   (%s)\n",
                 op, cmd_profile_table[i].us_count, cmd_profile_table[i].us_time,
                 cmd_profile_table[i].us_time/cmd_profile_table[i].us_count, m20_opname[i] );
       }
   }
   printf("Summary:  times=%.2f  count=%.0f  avg_time=%.2f\n", sum_time, sum_count, sum_time/sum_count );
   printf("**********\n\n");
}



/*
 * Main instruction fetch/decode loop
 */
t_stat sim_instr (void)
{
    t_stat r;
    int ticks;
    int addr_tags, a1, a2, a3, t_sw, op, i;
    uint16 t_ra;
    t_value m1,m2,m3,t_rr;
    double old_delay, instr_time;

    /* Restore register state */
    regKRA = regKRA & MAX_ADDR_VALUE;	        /* mask KRA */
    sim_cancel_step ();				/* defang SCP step */
    delay = 0;

    /* Main instruction fetch/decode loop */
    for (;;) {
	if (sim_interval <= 0) {		/* check clock queue */
  	  r = sim_process_event ();
	 if (r) return r;
	}

	if (regKRA >= MAX_MEM_SIZE) {		/* выход за пределы памяти */
            return STOP_RUNOUT;			/* stop simulation */
	}

	if (sim_brk_summ &&			/* breakpoint? */
	    sim_brk_test (regKRA, SWMASK ('E'))) {
            if (print_stat_on_break) print_commad_run_profile_stat();
	    return STOP_IBKPT;			/* stop simulation */
	}

	regRK = MOSU[regKRA];				/* get instruction */

	op = -1;
	if (print_sys_stat) {
          old_delay = delay;
          op = regRK >> BITS_36 & MAX_OPCODE_VALUE;
	}

	trace_before_run(&a1,&a2,&a3,&t_ra,&t_sw,&t_rr,&m1,&m2,&m3,0);

	regKRA += 1;				/* increment RVK */

	if (0) fprintf( stderr, "regKRA=%04o\n", regKRA );
	r = cpu_one_inst ();
	//if (r) return r;			/* one instr; error? */
	if (0) fprintf( stderr, "regKRA=%04o\n", regKRA );

	// save some state
        old_trgSW = trgSW;
        old_opcode = (int) (regRK >> BITS_36) & MAX_OPCODE_VALUE;

	/* save reg P1 state */
	addr_tags = regRK >> BITS_42 & MAX_ADDR_TAG_VALUE;
	a1 = regRK >> BITS_24 & MAX_ADDR_VALUE;
	if (addr_tags & 4) a1 = (a1 + regRA) & MAX_ADDR_VALUE;
        regP1 = MOSU[a1];
        //fprintf( stderr, "1: P1=%015llo\n", regP1 );

	// special check for stop codes
        if ((r == STOP_NEGSQRT) || (r==STOP_CRBADSUM) || (r==STOP_READERR) || (r==STOP_STOP) ||
            (r==STOP_TAPEREADERR)) {
            if (regKRA > 0001) regKRA -= 1;	/* decrement RVK */
            regRK = MOSU[regKRA];
        }
        if ((r==STOP_ASSERT) || (r==STOP_NOCD) || (r == STOP_DIVMOVF) || (r==STOP_DIVZERO)) {
            //regKRA -= 1;	/* decrement RVK */
            regRK = MOSU[regKRA-1];
        }


	if (print_sys_stat) {
          instr_time = delay - old_delay;
          if (instr_time > 0) {
            for( i=0; i<M20_SYM_OPCODE_TABLE_SIZE; i++ ) {
              if (cmd_profile_table[i].op_code == op) {
                  cmd_profile_table[i].us_count += 1;
                  cmd_profile_table[i].us_time  += instr_time;
                  break;
              }
            }
          }
          if (r) {
            print_commad_run_profile_stat();
          }
	}

	trace_after_run(a1,a2,a3,t_ra,t_sw,t_rr,m1,m2,m3);

	//getchar();

	ticks = 1;

	if (delay > 0)				/* delay to next instr */
	    //ticks += delay - DBL_EPSILON;
	    ticks += (int)(delay - DBL_EPSILON);

	delay -= ticks;				/* count down delay */
	sim_interval -= ticks;

        if (r) return r;			/* one instr; error? */

	if (sim_step && (--sim_step <= 0))	/* do step count */
	   return SCPE_STOP;
    }

}
