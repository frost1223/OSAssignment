#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define SIE_FL  (0x1ULL <<  1)
#define MIE_FL  (0x1ULL <<  3)
#define SPIE_FL (0x1ULL <<  5)
#define MPIE_FL (0x1ULL <<  7)
#define SPP_FL  (0x1ULL <<  8)
#define MPP_FL  (0x3ULL << 11)

// Struct to keep VM registers (Sample; feel free to change.)
struct vm_reg {
    int     code;
    int     mode;
    uint64  val;
};
# define U_MODE 0
# define S_MODE 1
# define M_MODE 2

int c = 0; 



// Keep the virtual state of the VM's privileged registers
struct vm_virtual_state {
    // User trap setup
    struct vm_reg ustatus, uie, utvec;

    // User trap handling
    struct vm_reg uscratch, uepc, ucause, ubadaddr, uip;

    // Supervisor trap setup
    struct vm_reg sstatus, sedeleg, sideleg, sie, stvec;

    // Supervisor trap handling
    struct vm_reg sscratch, sepc, scause, stval, sip;

    // Supervisor page table register
    struct vm_reg satp;

    // Machine information registers
    struct vm_reg misa, mvendorid, marchid, mimpid, mhartid;

    // Machine trap setup registers
    struct vm_reg mstatus, medeleg, mideleg, mie, mtvec;

    // Machine trap handling registers
    struct vm_reg mscratch, mepc, mcause, mbadaddr, mip;

    // Machine PMP registers
    struct vm_reg mpmp1, mpmp2;

    struct vm_reg totalregs[36];
    int exec_mode;
};

struct vm_virtual_state vm_state;

// int codeToVal(int code) {
//     for (int i = 0; i < 36; i++) {
//         if (vm_state.totalregs[i].code == code && vm_state.exec_mode >= vm_state.totalregs[i].mode) {
//             return vm_state.totalregs[i].val;
//             c = i;
//         }
//         break;
//     }
//     return -1; // Return a value indicating code not found (assuming mode cannot be negative)
// }

void trap_and_emulate_ecall() {
    struct proc *p = myproc();
    printf("(EC at %p)\n", p->trapframe->epc);

    // vm_state.sepc.val = p->trapframe->epc;
    // p->trapframe->epc = vm_state.stvec.val;
    // vm_state.exec_mode = S_MODE;

    vm_state.totalregs[14].val = p->trapframe->epc;
    p->trapframe->epc = vm_state.totalregs[12].val;
    vm_state.exec_mode = S_MODE;
}

// In your ECALL, add the following for prints
// struct proc* p = myproc();
// printf("(EC at %p)\n", p->trapframe->epc);

void trap_and_emulate(void) {
    /* Comes here when a VM tries to execute a supervisor instruction. */
    struct proc *p = myproc();
    uint64 vadd = r_sepc();
    uint64 padd = walkaddr(p->pagetable, vadd) | (vadd & 0xFFF);

    
    uint32 taddr = *((uint32*)(padd));

    /* Retrieve all required values from the instruction */
    uint64 addr     = p->trapframe->epc;;
    uint32 op = taddr & 0x7F;      
    uint32 rd = (taddr >> 7) & 0x1F;   
    uint32 funct3 = (taddr >> 12) & 0x7;
    uint32 rs1 = (taddr >> 15) & 0x1F; 
    uint32 uimm = (taddr >> 20) & 0xFFF;

    // int value = codeToVal(uimm);

    /* Print the statement */
        printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", 
                addr, op, rd, funct3, rs1, uimm);

    if (funct3 == 0x0 && uimm == 0X102){

        if (vm_state.exec_mode == S_MODE || vm_state.exec_mode == M_MODE){
        int nexec = (vm_state.totalregs[8].val >> 8) & 0x1;

        vm_state.totalregs[8].val = vm_state.totalregs[8].val & (~SPP_FL);

        uint64 SPIE_bit = vm_state.totalregs[24].val & SPIE_FL;
        vm_state.totalregs[8].val = vm_state.totalregs[8].val & (~SIE_FL);
        vm_state.totalregs[8].val = vm_state.totalregs[8].val & (~SPIE_FL);
        vm_state.totalregs[8].val = vm_state.totalregs[8].val | (SPIE_bit >> 4);

        p->trapframe->epc = vm_state.totalregs[14].val;

        vm_state.exec_mode = nexec;
        }else {
        setkilled(p);
        }

    // if (vm_state.exec_mode >= S_MODE){
    //     int new_mode = (vm_state.totalregs[8].val >> 8) & 0x1;

    //     vm_state.totalregs[8].val &= (~SPP_FL);

    //     uint64 SPIE_bit =  vm_state.totalregs[24].val & SPIE_FL;

    //      vm_state.totalregs[8].val |= (SPIE_bit) << 1;

    //      vm_state.totalregs[8].val &= (1) << 5;
    //     p->trapframe->epc =  vm_state.totalregs[14].val;

    //      vm_state.exec_mode = new_mode;
    // } else {
    //     setkilled(p);
    // }
        // /* Print the statement */
        // printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", 
        //         addr, op, rd, funct3, rs1, uimm);

        
    }else if (funct3 == 0x0 && uimm == 0x302){
        if (vm_state.exec_mode >= M_MODE){
        int nexec = (vm_state.totalregs[24].val >> 11) & 0x1;

        vm_state.totalregs[24].val = vm_state.totalregs[24].val & (~MPP_FL);

        uint64 MPIE_bit = vm_state.totalregs[24].val & MPIE_FL;
        vm_state.totalregs[24].val = vm_state.totalregs[24].val & (~MIE_FL);
        vm_state.totalregs[24].val = vm_state.totalregs[24].val & (~MPIE_FL);
        vm_state.totalregs[24].val = vm_state.totalregs[24].val | (MPIE_bit >> 4);

        p->trapframe->epc = vm_state.totalregs[30].val;

        vm_state.exec_mode = nexec;

            // if (vm_state.exec_mode >= M_MODE){
            //     int new_mode = (vm_state.totalregs[24].val >> 11) & 0x1;

            //     vm_state.totalregs[24].val &= (~MPP_FL);

            //     uint64 MPIE_bit = vm_state.totalregs[24].val & MPIE_FL;
            //      vm_state.totalregs[24].val |= (MPIE_bit) << 3;
            //      vm_state.totalregs[24].val &= (1) << 7;
            //      vm_state.totalregs[24].val &= ~(1 << 17);
            //     p->trapframe->epc = vm_state.totalregs[30].val;
            //      vm_state.exec_mode = new_mode;

    } else {
        setkilled(p);
    }
        // /* Print the statement */
        // printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", 
        //         addr, op, rd, funct3, rs1, uimm);
        // }

    }else if (funct3 == 0x1) {
        //csrw
        /* Print the statement */
        // printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", 
        //         addr, op, rd, funct3, rs1, uimm);
        // for (int i=0 ; i<36 ; i++) {
        // if (vm_state.totalregs[i].code == uimm && vm_state.exec_mode >= vm_state.totalregs[i].mode ) {
        //     // if (vm_state.exec_mode >= vm_state.totalregs[i].mode) {
        //         uint64* bp = rs1 + &(p->trapframe->ra) - 1;
        //         vm_state.totalregs[i].val = (*bp);

        //         // if (*bp == 0x0 && vm_state.totalregs[i].code == 0xF11) {
        //         //     printf("Killing VM due to mvendorid being set to 0x0\n");
        //         //     setkilled(p);
        //         // }
        //     } else if(vm_state.exec_mode < vm_state.totalregs[i].mode) {
        //         setkilled(p);
        //     }
        //     break;
        // }
        // //}

    for (int i = 0; i < 36; i++) {
        if (vm_state.totalregs[i].code == uimm && vm_state.exec_mode >= vm_state.totalregs[i].mode) {
            uint64* bs = &p->trapframe->ra + rs1 - 1;
            vm_state.totalregs[i].val = *bs;

            // if (*rs1_pointer == 0x0 && vm.regs[i].code == 0xF11) {
            //     printf("Killing VM due to mvendorid being set to 0x0\n");
            //     setkilled(p);
            // }

            p->trapframe->epc += 4;
            return;  // Exit the loop since we found and processed the matching uimm
        }
    }

    // If no matching uimm is found, setkilled
    setkilled(p);
// }

    
        // uint64* bp = rs1 + &(p->trapframe->ra) - 1;
        // value = (*bp);
        // c = 0;

        // if (*bp == 0x0 && uimm == 0xF11) {
        //     printf("Killing VM due to mvendorid being set to 0x0\n");
        //     setkilled(p);
        //     }
        // // } else {
        // //     setkilled(p);
        // //     }

    

    p->trapframe->epc += 4;
    }
    else if(funct3 == 0x2){
        //csrr
        // /* Print the statement */
        // printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", 
        //         addr, op, rd, funct3, rs1, uimm);
        for (int i=0 ; i<36 ; i++) {
        if (vm_state.totalregs[i].code == uimm) {
            if (vm_state.exec_mode >= vm_state.totalregs[i].mode) {
                uint64 *d = rd + &(p->trapframe->ra) - 1;
                *d = vm_state.totalregs[i].val;
            } else {
                setkilled(p);
            }
            break;
        
    }
        }

    p->trapframe->epc += 4;
    }
    else{
        printf("trap_and_emulate: invalid\n");
        setkilled(p);
    }

    // /* Print the statement */
    // printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", 
    //             addr, op, rd, funct3, rs1, uimm);
}

void trap_and_emulate_init(void) {
    /* Create and initialize all state for the VM */
    vm_state.ustatus.code = 0x000;
    vm_state.ustatus.mode = U_MODE;
    vm_state.ustatus.val = 0;
	vm_state.totalregs[0] =  vm_state.ustatus;

    vm_state.uie.code = 0x004;
    vm_state.uie.mode = U_MODE;
    vm_state.uie.val = 0;
	vm_state.totalregs[1] = vm_state.uie;

    vm_state.utvec.code = 0x005;
    vm_state.utvec.mode = U_MODE;
    vm_state.utvec.val = 0;
	vm_state.totalregs[2] = vm_state.utvec;

    // User trap handling
    vm_state.uscratch.code = 0x040;
    vm_state.uscratch.mode = U_MODE;
    vm_state.uscratch.val = 0;
	vm_state.totalregs[3] = vm_state.uscratch;

    vm_state.uepc.code = 0x041;
    vm_state.uepc.mode = U_MODE;
    vm_state.uepc.val = 0;
	vm_state.totalregs[4] = vm_state.uepc;

    vm_state.ucause.code = 0x042;
    vm_state.ucause.mode = U_MODE;
    vm_state.ucause.val = 0;
	vm_state.totalregs[5] = vm_state.ucause;

    vm_state.ubadaddr.code = 0x043;
    vm_state.ubadaddr.mode = U_MODE;
    vm_state.ubadaddr.val = 0;
	vm_state.totalregs[6] = vm_state.ubadaddr;

    vm_state.uip.code = 0x044;
    vm_state.uip.mode = U_MODE;
    vm_state.uip.val = 0;
	vm_state.totalregs[7] = vm_state.uip;

    // Supervisor trap setup
    vm_state.sstatus.code = 0x100;
    vm_state.sstatus.mode = S_MODE; // Supervisor mode
    vm_state.sstatus.val = 0;
	vm_state.totalregs[8] = vm_state.sstatus;

    vm_state.sedeleg.code = 0x102;
    vm_state.sedeleg.mode = S_MODE;
    vm_state.sedeleg.val = 0;
	vm_state.totalregs[9] = vm_state.sedeleg;

    vm_state.sideleg.code = 0x103;
    vm_state.sideleg.mode = S_MODE;
    vm_state.sideleg.val = 0;
	vm_state.totalregs[10] = vm_state.sideleg;

    vm_state.sie.code = 0x104;
    vm_state.sie.mode = S_MODE;
    vm_state.sie.val = 0;
	vm_state.totalregs[11] = vm_state.sie;

    vm_state.stvec.code = 0x105;
    vm_state.stvec.mode = S_MODE;
    vm_state.stvec.val = 0;
	vm_state.totalregs[12] = vm_state.stvec;

    // Supervisor trap handling
    vm_state.sscratch.code = 0x140;
    vm_state.sscratch.mode = S_MODE;
    vm_state.sscratch.val = 0;
	vm_state.totalregs[13] = vm_state.sscratch;

    vm_state.sepc.code = 0x141;
    vm_state.sepc.mode = S_MODE;
    vm_state.sepc.val = 0;
	vm_state.totalregs[14] = vm_state.sepc;

    vm_state.scause.code = 0x142;
    vm_state.scause.mode = S_MODE;
    vm_state.scause.val = 0;
	vm_state.totalregs[15] = vm_state.scause;

    vm_state.stval.code = 0x143;
    vm_state.stval.mode = S_MODE;
    vm_state.stval.val = 0;
	vm_state.totalregs[16] = vm_state.stval;

    vm_state.sip.code = 0x144;
    vm_state.sip.mode = S_MODE;
    vm_state.sip.val = 0;
	vm_state.totalregs[17] = vm_state.sip;

    // Supervisor page table register
    vm_state.satp.code = 0x180;
    vm_state.satp.mode = S_MODE;
    vm_state.satp.val = 0;
	vm_state.totalregs[18] = vm_state.satp;

    // Machine information registers
    vm_state.misa.code = 0x301;
    vm_state.misa.mode = M_MODE; // Machine mode
    vm_state.misa.val = 0;
	vm_state.totalregs[19] = vm_state.misa;

    vm_state.mvendorid.code = 0xF11;
    vm_state.mvendorid.mode = M_MODE;
    vm_state.mvendorid.val = 0xC5E536; // "cse536" in hexadecimal
	vm_state.totalregs[20] = vm_state.mvendorid;

    vm_state.marchid.code = 0xF12;
    vm_state.marchid.mode = M_MODE;
    vm_state.marchid.val = 0;
	vm_state.totalregs[21] = vm_state.marchid;

    vm_state.mimpid.code = 0xF13;
    vm_state.mimpid.mode = M_MODE;
    vm_state.mimpid.val = 0;
	vm_state.totalregs[22] = vm_state.mimpid;

    vm_state.mhartid.code = 0xF14;
    vm_state.mhartid.mode = M_MODE;
    vm_state.mhartid.val = 0;
	vm_state.totalregs[23] = vm_state.mhartid;

    // Machine trap setup registers
    vm_state.mstatus.code = 0x300;
    vm_state.mstatus.mode = M_MODE;
    vm_state.mstatus.val = 0;
	vm_state.totalregs[24] = vm_state.mstatus;

    vm_state.medeleg.code = 0x302;
    vm_state.medeleg.mode = M_MODE;
    vm_state.medeleg.val = 0;
	vm_state.totalregs[25] = vm_state.medeleg;

    vm_state.mideleg.code = 0x303;
    vm_state.mideleg.mode = M_MODE;
    vm_state.mideleg.val = 0;
	vm_state.totalregs[26] = vm_state.mideleg;

    vm_state.mie.code = 0x304;
    vm_state.mie.mode = M_MODE;
    vm_state.mie.val = 0;
	vm_state.totalregs[27] = vm_state.mie;

    vm_state.mtvec.code = 0x305;
    vm_state.mtvec.mode = M_MODE;
    vm_state.mtvec.val = 0;
	vm_state.totalregs[28] = vm_state.mtvec;

    // Machine trap handling registers
    vm_state.mscratch.code = 0x340;
    vm_state.mscratch.mode = M_MODE;
    vm_state.mscratch.val = 0;
	vm_state.totalregs[29] = vm_state.mscratch;

    vm_state.mepc.code = 0x341;
    vm_state.mepc.mode = M_MODE;
    vm_state.mepc.val = 0;
	vm_state.totalregs[30] = vm_state.mepc;
	
    vm_state.mcause.code = 0x342;
    vm_state.mcause.mode = M_MODE;
    vm_state.mcause.val = 0;
	vm_state.totalregs[31] = vm_state.mcause;

    vm_state.mbadaddr.code = 0x343;
    vm_state.mbadaddr.mode = M_MODE;
    vm_state.mbadaddr.val = 0;
	vm_state.totalregs[32] = vm_state.mbadaddr;

    vm_state.mip.code = 0x344;
    vm_state.mip.mode = M_MODE;
    vm_state.mip.val = 0;
	vm_state.totalregs[33] = vm_state.mip;

    // Machine PMP registers 
    vm_state.mpmp1.code = 0x3A0;
    vm_state.mpmp1.mode = M_MODE;
    vm_state.mpmp1.val = 0;
	vm_state.totalregs[34] = vm_state.mpmp1;

    vm_state.mpmp2.code = 0x3A1;
    vm_state.mpmp2.mode = M_MODE;
    vm_state.mpmp2.val = 0;
	vm_state.totalregs[35] = vm_state.mpmp2;

    vm_state.exec_mode = M_MODE;


    
}