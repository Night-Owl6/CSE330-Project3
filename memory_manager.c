/**********************************************************************
 * Authors: Ajay Tiwari Bettina Thomas
 **********************************************************************/

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/sched/signal.h>
#include <asm/pgtable.h>
#include <asm/mmu_context.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AJAY TIWARI, Bettina Thomas");

static int PID = 1;

module_param(PID, int, 0);
unsigned long timer_interval_ns = 10e9;	//call functions every 10 seconds
static struct hrtimer hr_timer; 
static struct task_struct *task;
unsigned long address;
int memoryCount = 0;
int accessed = 0;
int no_accessed = 0;
int invalid = 0;
int helper = 0;
int RSS;
int WSS;
int SWAP;
pgd_t *pgd;
p4d_t *p4d;
pud_t *pud;
pmd_t *pmd;
pte_t *ptep, pte;
struct vm_area_struct *vma;

void counter(void) {
	invalid++;
}

void accessCounter(void) {
	accessed++;
}

void NoaccessCounter(void) {
	no_accessed++;
}

int ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep) {
	int ret = 0;
	if (pte_young(*ptep))
		ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
	return ret;
}

void walk_page_table(struct task_struct *task) {
	for_each_process(task) {
		if(task != NULL && task->pid == PID) {
			vma = task->mm->mmap;
			
			while(vma != NULL) {
				for(address = vma->vm_start; address < vma->vm_end; address += PAGE_SIZE) {
					helper++;
					pgd = pgd_offset(task->mm, address);
					if(pgd_none(*pgd) || pgd_bad(*pgd)) {
						counter();
					}
					p4d = p4d_offset(pgd, address);
					if(p4d_none(*p4d) || p4d_bad(*p4d)) {
						counter();
					}
					pud = pud_offset(p4d, address);                   
                    			if(pud_none(*pud) || pud_bad(*pud)) {          
                        			counter();
                    			}
                    			pmd = pmd_offset(pud, address);               
                    			if(pmd_none(*pmd) || pmd_bad(*pmd)) {      
                        			counter();
                    			} 
                    			ptep = pte_offset_map(pmd, address);     
                    			if(!ptep) {
                        			counter();
                    			}
                    			pte = *ptep;
                    			if(ptep_test_and_clear_young(vma, address, ptep) == 1) {
                    				accessCounter();
                    			} else {
                    				NoaccessCounter();
                    			}
                    			if(pte_present(pte) == 1) {
                    				memoryCount++;
                    			}  else {
                    				counter();
                    			}
				}
				vma = vma->vm_next;
			}
			RSS = memoryCount * 4;
			WSS = accessed * 4;
			SWAP = invalid * 4;
			
			printk("PID = %d: RSS = %d KB, WSS = %d KB, SWAP = %d KB", PID, RSS, WSS, SWAP);
		} 
	}
}

enum hrtimer_restart no_restart_callback(struct hrtimer *timer) {
	ktime_t currtime , interval;
	currtime  = ktime_get();
	interval = ktime_set(0, timer_interval_ns); 
	hrtimer_forward(timer, currtime , interval);
	walk_page_table(task);
   
	return HRTIMER_RESTART;
}

static int __init memory_manager_init(void) {
	ktime_t k_time;
	k_time = ktime_set(0, timer_interval_ns);
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &no_restart_callback;
	hrtimer_start(&hr_timer, k_time, HRTIMER_MODE_REL);   	
	return 0;
}

static void __exit memory_manager_exit(void) {
	int ret;
	ret = hrtimer_cancel(&hr_timer);
	if(ret) {
		printk("Timer is still working..\n");
	}
	printk("hr_timer is taken out.\n");
}

module_init(memory_manager_init);
module_exit(memory_manager_exit);
