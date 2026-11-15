#include "iop.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct mem { uint32_t words[64]; } mem_t;
static int rd(void *o,uint32_t a,uint32_t *v){mem_t*m=o;if((a&3u)||a/4u>=64u)return -1;*v=m->words[a/4u];return 0;}
static int wr(void *o,uint32_t a,uint32_t v){mem_t*m=o;if((a&3u)||a/4u>=64u)return -1;m->words[a/4u]=v;return 0;}
int main(void)
{
    mem_t m;ps2_iop_state_t s;ps2_iop_bus_t bus={rd,wr,&m};
    memset(&m,0,sizeof m);
    m.words[0]=UINT32_C(0x24010005); /* addiu r1,zero,5 */
    m.words[1]=UINT32_C(0x24220003); /* addiu r2,r1,3 */
    m.words[2]=UINT32_C(0x00221821); /* addu r3,r1,r2 */
    m.words[3]=UINT32_C(0xac030040); /* sw r3,0x40(zero) */
    m.words[4]=UINT32_C(0x8c040040); /* lw r4,0x40(zero) */
    ps2_iop_reset(&s,0);
    assert(ps2_iop_step(&s,&bus)==1 && s.gpr[1]==5u);
    assert(ps2_iop_step(&s,&bus)==1 && s.gpr[2]==8u);
    assert(ps2_iop_step(&s,&bus)==1 && s.gpr[3]==13u);
    assert(ps2_iop_step(&s,&bus)==1 && m.words[16]==13u);
    assert(ps2_iop_step(&s,&bus)==1 && s.gpr[4]==13u);
    assert(s.gpr[0]==0u && s.cycle_count==5u);
    /* JAL writes the return address to r31 and executes its delay slot. */
    memset(&m,0,sizeof m);
    m.words[0]=UINT32_C(0x0c000004); /* jal 0x10 */
    m.words[1]=UINT32_C(0x24050009); /* delay slot: addiu r5,zero,9 */
    m.words[4]=UINT32_C(0x03e00008); /* jr r31 */
    ps2_iop_reset(&s,0);
    assert(ps2_iop_step(&s,&bus)==1);
    assert(s.gpr[31]==8u && s.pc==4u && s.next_pc==16u);
    assert(ps2_iop_step(&s,&bus)==1 && s.gpr[5]==9u && s.pc==16u);
    assert(ps2_iop_step(&s,&bus)==1 && s.next_pc==8u);
    puts("OK: IOP integer/load-store, JAL link and branch-delay tests.");
    return 0;
}
