/* Debugging dump procedures for the kernel. */

#include "inc.h"
#include <timers.h>
#include <ibm/interrupt.h>
#include <minix/endpoint.h>
#include <minix/sysutil.h>
#include <minix/sys_config.h>
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/debug.h"
#include "../../kernel/type.h"
#include "../../kernel/proc.h"
#include "../../kernel/ipc.h"
#include "../pm/mproc.h"   /* AGREGADO */
#include <stdio.h>
#define LINES 22

#define PRINTRTS(rp) { \
	char *procname = "";	\
	printf(" %s", p_rts_flags_str(rp->p_rts_flags));	\
	if (rp->p_rts_flags & SENDING)				\
		procname = proc_name(_ENDPOINT_P(rp->p_sendto_e)); \
	else if (rp->p_rts_flags & RECEIVING)			\
		procname = proc_name(_ENDPOINT_P(rp->p_getfrom_e)); \
	printf(" %-7.7s", procname);	\
}

static int pagelines;

#define PROCLOOP(rp, oldrp) \
	pagelines = 0; \
	for (rp = oldrp; rp < END_PROC_ADDR; rp++) { \
	  oldrp = BEG_PROC_ADDR; \
	  if (isemptyp(rp)) continue; \
	  if (++pagelines > LINES) { oldrp = rp; printf("--more--\n"); break; }\
	  if (proc_nr(rp) == IDLE) 	printf("(%2d) ", proc_nr(rp));  \
	  else if (proc_nr(rp) < 0) 	printf("[%2d] ", proc_nr(rp)); 	\
	  else 				printf(" %2d  ", proc_nr(rp));

#define click_to_round_k(n) \
	((unsigned) ((((unsigned long) (n) << CLICK_SHIFT) + 512) / 1024))

/* Declare some local dump procedures. */
FORWARD _PROTOTYPE( char *proc_name, (int proc_nr)		);
FORWARD _PROTOTYPE( char *s_traps_str, (int flags)		);
FORWARD _PROTOTYPE( char *s_flags_str, (int flags)		);
FORWARD _PROTOTYPE( char *p_rts_flags_str, (int flags)		);

/* Some global data that is shared among several dumping procedures. 
 * Note that the process table copy has the same name as in the kernel
 * so that most macros and definitions from proc.h also apply here.
 */
PUBLIC struct proc proc[NR_TASKS + NR_PROCS];
PUBLIC struct priv priv[NR_SYS_PROCS];
PUBLIC struct boot_image image[NR_BOOT_PROCS];


/*===========================================================================*
 *				procesos_usuario_activo_dmp		     *     
 *===========================================================================*/
PUBLIC void procesos_usuario_activo_dmp()   /*FUNCION AGREGADA */
{
 struct mproc *process; //esta estructura almacena los procesos
 
  if(getsysinfo(PM_PROC_NR, SI_PROC_TAB, mproc, sizeof(mproc))){   
    
    /* 

    - getsysinfo() es un kernel call

    - argumentos de getsysinfo():
      endpoint_t who //from whom to request info
      int what //what info is requested
      void * where //where to put it
      size_t size // how big it should be

    - endpoint_t who: PM_PROC_NR para que PM se encarge de obtener info
    - int what : SI_PROC_TAB copia tabla de procesos entera
    - void *where: mproc guarde la info de un proceso 
    - size_t size:  sizeof(mproc)



    */


    perror("Error al obtener tabla de procesos en memoria \n");
    exit(1);
  }
  else{
     puts("TABLA DE PROCESOS DEL USUARIO ACTIVO \n")
  for(int i=0;i<NR_PROCS;i++){
    process=&mproc[i];
   
      puts("--------------------------- \n")
      printf("index: %i |",mp->mp_tracer);
      printf("nombre: %s|",mp->mp_name);
      printf("id: %u |",(unsigned int) mp->mp_id);
      printf("real uid: %u |",mp->mp_realuid);
      printf("effective uid: %u |",mp->mp_effuid);
      fflush(stdout);

  }
  
    
    
  }


}






/*===========================================================================*
 *				timing_dmp				     *
 *===========================================================================*/
PUBLIC void timing_dmp()
{
  static struct util_timingdata timingdata[TIMING_CATEGORIES];
  int r, c, f, skipped = 0, printed = 0, maxlines = 23, x = 0;
  static int offsetlines = 0;

  if ((r = sys_getlocktimings(&timingdata[0])) != OK) {
      report("IS","warning: couldn't get copy of lock timings", r);
      return;
  } 

  for(c = 0; c < TIMING_CATEGORIES; c++) {
	int b;
	if (!timingdata[c].lock_timings_range[0] || !timingdata[c].binsize)
		continue;
	x = printf("%-*s: misses %lu, resets %lu, measurements %lu: ",
	TIMING_NAME, timingdata[c].names,
		timingdata[c].misses,
		timingdata[c].resets,
		timingdata[c].measurements);
	for(b = 0; b < TIMING_POINTS; b++) {
		int w;
		if (!timingdata[c].lock_timings[b])
			continue;
		x += (w = printf(" %5d: %5d", timingdata[c].lock_timings_range[0] +
			b*timingdata[c].binsize,
			timingdata[c].lock_timings[b]));
	 	if (x + w >= 80) { printf("\n"); x = 0; }
	}
  	if (x > 0) printf("\n");
  }
}

/*===========================================================================*
 *				kmessages_dmp				     *
 *===========================================================================*/
PUBLIC void kmessages_dmp()
{
  struct kmessages kmess;		/* get copy of kernel messages */
  char print_buf[_KMESS_BUF_SIZE+1];	/* this one is used to print */
  int start;				/* calculate start of messages */
  int r;

  /* Try to get a copy of the kernel messages. */
  if ((r = sys_getkmessages(&kmess)) != OK) {
      report("IS","warning: couldn't get copy of kmessages", r);
      return;
  }

  /* Try to print the kernel messages. First determine start and copy the
   * buffer into a print-buffer. This is done because the messages in the
   * copy may wrap (the kernel buffer is circular).
   */
  start = ((kmess.km_next + _KMESS_BUF_SIZE) - kmess.km_size) % _KMESS_BUF_SIZE;
  r = 0;
  while (kmess.km_size > 0) {
  	print_buf[r] = kmess.km_buf[(start+r) % _KMESS_BUF_SIZE];
  	r ++;
  	kmess.km_size --;
  }
  print_buf[r] = 0;		/* make sure it terminates */
  printf("Dump of all messages generated by the kernel.\n\n"); 
  printf("%s", print_buf);		/* print the messages */
}

/*===========================================================================*
 *				monparams_dmp				     *
 *===========================================================================*/
PUBLIC void monparams_dmp()
{
  char val[1024];
  char *e;
  int r;

  /* Try to get a copy of the boot monitor parameters. */
  if ((r = sys_getmonparams(val, sizeof(val))) != OK) {
      report("IS","warning: couldn't get copy of monitor params", r);
      return;
  }

  /* Append new lines to the result. */
  e = val;
  do {
	e += strlen(e);
	*e++ = '\n';
  } while (*e != 0); 

  /* Finally, print the result. */
  printf("Dump of kernel environment strings set by boot monitor.\n");
  printf("\n%s\n", val);
}

/*===========================================================================*
 *				irqtab_dmp				     *
 *===========================================================================*/
PUBLIC void irqtab_dmp()
{
  int i,r;
  struct irq_hook irq_hooks[NR_IRQ_HOOKS];
  int irq_actids[NR_IRQ_VECTORS];
  struct irq_hook *e;	/* irq tab entry */

  if ((r = sys_getirqhooks(irq_hooks)) != OK) {
      report("IS","warning: couldn't get copy of irq hooks", r);
      return;
  }
  if ((r = sys_getirqactids(irq_actids)) != OK) {
      report("IS","warning: couldn't get copy of irq mask", r);
      return;
  }

#if 0
  printf("irq_actids:");
  for (i= 0; i<NR_IRQ_VECTORS; i++)
	printf(" [%d] = 0x%08x", i, irq_actids[i]);
  printf("\n");
#endif

  printf("IRQ policies dump shows use of kernel's IRQ hooks.\n");
  printf("-h.id- -proc.nr- -irq nr- -policy- -notify id-\n");
  for (i=0; i<NR_IRQ_HOOKS; i++) {
  	e = &irq_hooks[i];
  	printf("%3d", i);
  	if (e->proc_nr_e==NONE) {
  	    printf("    <unused>\n");
  	    continue;
  	}
  	printf("%10d  ", e->proc_nr_e); 
  	printf("    (%02d) ", e->irq); 
  	printf("  %s", (e->policy & IRQ_REENABLE) ? "reenable" : "    -   ");
  	printf("   %d", e->notify_id);
	if (irq_actids[e->irq] & (1 << i))
		printf("masked");
	printf("\n");
  }
  printf("\n");
}

/*===========================================================================*
 *				image_dmp				     *
 *===========================================================================*/
PUBLIC void image_dmp()
{
  int m, i,j,r;
  struct boot_image *ip;
  static char ipc_to[BITCHUNK_BITS*2];
	
  if ((r = sys_getimage(image)) != OK) {
      report("IS","warning: couldn't get copy of image table", r);
      return;
  }
  printf("Image table dump showing all processes included in system image.\n");
  printf("---name-- -nr- -flags- -traps- -sq- ----pc- -stack- -ipc_to[0]--------\n");
  for (m=0; m<NR_BOOT_PROCS; m++) { 
      ip = &image[m];
        for (i=j=0; i < BITCHUNK_BITS; i++, j++) {
       	    ipc_to[j] = (ip->ipc_to & (1<<i)) ? '1' : '0';
       	    if (i % 8 == 7) ipc_to[++j] = ' ';
       	}
        ipc_to[j] = '\0';
      printf("%8s %4d   %s   %s  %3d %7lu %7lu   %s\n",
          ip->proc_name, ip->proc_nr, 
	       s_flags_str(ip->flags), s_traps_str(ip->trap_mask), 
	ip->priority, (long)ip->initial_pc, ip->stksize, ipc_to); 
  }
  printf("\n");
}


/*===========================================================================*
 *				kenv_dmp				     *
 *===========================================================================*/
PUBLIC void kenv_dmp()
{
    struct kinfo kinfo;
    struct machine machine;
    int r;
    if ((r = sys_getkinfo(&kinfo)) != OK) {
    	report("IS","warning: couldn't get copy of kernel info struct", r);
    	return;
    }
    if ((r = sys_getmachine(&machine)) != OK) {
    	report("IS","warning: couldn't get copy of kernel machine struct", r);
    	return;
    }

    printf("Dump of kinfo and machine structures.\n\n");
    printf("Machine structure:\n");
    printf("- pc_at:      %3d\n", machine.pc_at); 
    printf("- ps_mca:     %3d\n", machine.ps_mca); 
    printf("- processor:  %3d\n", machine.processor); 
    printf("- vdu_ega:    %3d\n", machine.vdu_ega); 
    printf("- vdu_vga:    %3d\n\n", machine.vdu_vga); 
    printf("Kernel info structure:\n");
    printf("- code_base:  %5u\n", kinfo.code_base); 
    printf("- code_size:  %5u\n", kinfo.code_size); 
    printf("- data_base:  %5u\n", kinfo.data_base); 
    printf("- data_size:  %5u\n", kinfo.data_size); 
    printf("- proc_addr:  %5u\n", kinfo.proc_addr); 
    printf("- bootdev_base:  %5u\n", kinfo.bootdev_base); 
    printf("- bootdev_size:  %5u\n", kinfo.bootdev_size); 
    printf("- ramdev_base:   %5u\n", kinfo.ramdev_base); 
    printf("- ramdev_size:   %5u\n", kinfo.ramdev_size); 
    printf("- nr_procs:     %3u\n", kinfo.nr_procs); 
    printf("- nr_tasks:     %3u\n", kinfo.nr_tasks); 
    printf("- release:      %.6s\n", kinfo.release); 
    printf("- version:      %.6s\n", kinfo.version); 
    printf("\n");
}

PRIVATE char *s_flags_str(int flags)
{
	static char str[10];
	str[0] = (flags & PREEMPTIBLE) ? 'P' : '-';
	str[1] = '-';
	str[2] = (flags & BILLABLE)    ? 'B' : '-';
	str[3] = (flags & SYS_PROC)    ? 'S' : '-';
	str[4] = '-';
	str[5] = '\0';

	return str;
}

PRIVATE char *s_traps_str(int flags)
{
	static char str[10];
	str[0] = (flags & (1 << SEND))  ? 'S' : '-';
	str[1] = (flags & (1 << SENDA)) ? 'A' : '-';
	str[2] = (flags & (1 << RECEIVE))  ? 'R' : '-';
	str[3] = (flags & (1 << SENDREC))  ? 'B' : '-';
	str[4] = (flags & (1 << NOTIFY)) ? 'N' : '-';
	str[5] = '\0';

	return str;
}




PRIVATE char *p_rts_flags_str(int flags)
{
	static char str[10];
	str[0] = (flags & PROC_STOP) ? 's' : '-';
	str[1] = (flags & SENDING)  ? 'S' : '-';
	str[2] = (flags & RECEIVING)    ? 'R' : '-';
	str[3] = (flags & SIGNALED)    ? 'I' : '-';
	str[4] = (flags & SIG_PENDING)    ? 'P' : '-';
	str[5] = (flags & P_STOP)    ? 'T' : '-';
	str[6] = (flags & NO_PRIV) ? 'p' : '-';
	str[7] = '\0';

	return str;
}

/*===========================================================================*
 *				proctab_dmp    				     *
 *===========================================================================*/
#if (CHIP == INTEL)
PUBLIC void proctab_dmp()
{
/* Proc table dump */

  register struct proc *rp;
  static struct proc *oldrp = BEG_PROC_ADDR;
  int r, n = 0;
  phys_clicks text, data, size;

  /* First obtain a fresh copy of the current process table. */
  if ((r = sys_getproctab(proc)) != OK) {
      report("IS","warning: couldn't get copy of process table", r);
      return;
  }

  printf("\n-nr-----gen---endpoint-name--- -prior-quant- -user----sys-rtsflags-from/to-\n");

  PROCLOOP(rp, oldrp)
	text = rp->p_memmap[T].mem_phys;
	data = rp->p_memmap[D].mem_phys;
	size = rp->p_memmap[T].mem_len
		+ ((rp->p_memmap[S].mem_phys + rp->p_memmap[S].mem_len) - data);
	printf(" %5d %10d ", _ENDPOINT_G(rp->p_endpoint), rp->p_endpoint);
	printf("%-8.8s %02u/%02u %02d/%02u %6lu %6lu ",
	       rp->p_name,
	       rp->p_priority, rp->p_max_priority,
	       rp->p_ticks_left, rp->p_quantum_size, 
	       rp->p_user_time, rp->p_sys_time);
	PRINTRTS(rp);
	printf("\n");
  }
}
#endif				/* (CHIP == INTEL) */

/*===========================================================================*
 *				procstack_dmp  				     *
 *===========================================================================*/
PUBLIC void procstack_dmp()
{
/* Proc table dump, with stack */

  register struct proc *rp;
  static struct proc *oldrp = BEG_PROC_ADDR;
  int r, n = 0;

  /* First obtain a fresh copy of the current process table. */
  if ((r = sys_getproctab(proc)) != OK) {
      report("IS","warning: couldn't get copy of process table", r);
      return;
  }

  printf("\n-nr-rts flags--      --stack--\n");

  PROCLOOP(rp, oldrp)
	PRINTRTS(rp);
	sys_sysctl_stacktrace(rp->p_endpoint);
  }
}

/*===========================================================================*
 *				memmap_dmp    				     *
 *===========================================================================*/
PUBLIC void memmap_dmp()
{
  register struct proc *rp;
  static struct proc *oldrp = proc;
  int r, n = 0;
  phys_clicks size;

  /* First obtain a fresh copy of the current process table. */
  if ((r = sys_getproctab(proc)) != OK) {
      report("IS","warning: couldn't get copy of process table", r);
      return;
  }

  printf("\n-nr/name--- --pc--   --sp-- -text---- -data---- -stack--- -cr3-\n");
  PROCLOOP(rp, oldrp)
	size = rp->p_memmap[T].mem_len
		+ ((rp->p_memmap[S].mem_phys + rp->p_memmap[S].mem_len)
						- rp->p_memmap[D].mem_phys);
	printf("%-7.7s%7lx %8lx %4x %4x %4x %4x %5x %5x %8lx\n",
	       rp->p_name,
	       (unsigned long) rp->p_reg.pc,
	       (unsigned long) rp->p_reg.sp,
	       rp->p_memmap[T].mem_phys, rp->p_memmap[T].mem_len,
	       rp->p_memmap[D].mem_phys, rp->p_memmap[D].mem_len,
	       rp->p_memmap[S].mem_phys, rp->p_memmap[S].mem_len,
	       rp->p_seg.p_cr3);
  }
}

/*===========================================================================*
 *				proc_name    				     *
 *===========================================================================*/
PRIVATE char *proc_name(proc_nr)
int proc_nr;
{
  struct proc *p;
  if (proc_nr == ANY) return "ANY";
  if (proc_nr == NONE) return "NONE";	/* bogus */
  if (proc_nr < -NR_TASKS || proc_nr >= NR_PROCS) return "BOGUS";
  p = cproc_addr(proc_nr);
  if (isemptyp(p)) return "EMPTY";	/* bogus */
  return p->p_name;
}

