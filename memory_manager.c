/**********************************************************************
 * Authors: Ajay Tiwari
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AJAY TIWARI");

module_param(PID, int, 0);		//try this if not work: S_IRUSR

int PID;
unsigned long timer_interval_ns = 10e9;	//call functions every 10 seconds
static struct hrtimer hr_timer; 
struct task_struct *task;
unsigned long address;
int memoryCount = 0;
int accessed = 0;
int no_accessed = 0;
int invalid = 0;
int RSS;
int WSS;
int SWAP;
unsigned long helperSize; 
pgd_t *pgd;
p4d_t *p4d;
pud_t *pud;
pmd_t *pmd;
pte_t *ptep, pte;
struct vm_area_struct *vma;

int ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep) {
	int ret = 0;
	if (pte_young(*ptep))
		ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
	return ret;
}

int walk_page_table(struct task_struck *task) {
	for_each_process(task) {
		if(task != null && task->pid == PID) {
			vma = task->mm->mmap;
			
			while(vma) {
				for(address = vma->vm_start; address <= (vma->vm_end-PAGE_SIZE); address += PAGE_SIZE) {
					pgd = pgd_offset(task->mm, address);
					if(pgd_none(*pgd) || pgd_bad(*pgd)) {
						invalid = invalid + 1;
					}
					p4d = p4d_offset(pgd, address);
					if(p4d_none(*p4d) || p4d_bad(*p4d)) {
						invalid = invalid + 1;
					}
					pud = pud_offset(p4d, address);                   
                    			if(pud_none(*pud) || pud_bad(*pud)) {          
                        			invalid = invalid + 1;
                    			}
                    			pmd = pmd_offset(pud, address);               
                    			if(pmd_none(*pmd) || pmd_bad(*pmd)) {      
                        			invalid = invalid + 1;
                    			} 
                    			ptep = pte_offset_map(pmd, address);     
                    			if(!ptep) {
                        			invalid = invalid + 1;
                    			}
                    			pte = *ptep;
                    			if(ptep_test_and_clear_young(vma, address, ptep) == 1) {
                    				accessed = accessed + 1;
                    			} else {
                    				no_accessed = no_accessed + 1;
                    			}
                    			if(pte_present(pte) == 1) {
                    				memoryCount++;
                    			}  else {
                    				invalid = invalid + 1;
                    			}
				}
				vma = vma->vm_next;
			}
			RSS = 2*2*memoryCount;
			WSS = 2*2*accessed;
			SWAP = 2*2*invalid;
			
			printk("PID = %d: RSS = %d KB, WSS = %d KB, SWAP = %d KB", pid, RSS, WSS, SWAP);
		} 
	}
	return 0;
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
	printk("Works\n");
	ktime_t ktime;
	ktime = ktime_set(0, timer_interval_ns);
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &no_restart_callback;
	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);   	
	return 0;
}

static void __exit memory_manager_exit(void) {
	printk("Leaving\n");
	int ret;
	if(ret) {
		printk("Timer is still in use...\n");
	}
	printk("hr_timer is taken out.\n");
}

module_init(memory_manager);
module_exit(memory_manager);
