#include "iop.h"
#include <stddef.h>
#include <stdint.h>
void ps2_iop_reset(ps2_iop_state_t *s,uint32_t pc)
{
    unsigned i;if(s==0)return;
    for(i=0;i<32u;i++)s->gpr[i]=0u;
    s->hi=0;s->lo=0;s->pc=pc;s->next_pc=pc+4u;s->cycle_count=0;s->badvaddr=0;s->halted=0;
}
int ps2_iop_step(ps2_iop_state_t *s,const ps2_iop_bus_t *bus)
{
    uint32_t ins,cur,op,rs,rt,rd,imm,target,a,b,result=0; int write=0;
    if(s==0 || bus==0 || bus->read32==0 || bus->write32==0)return -1;
    if(s->halted)return 0;
    cur=s->pc;
    if(bus->read32(bus->opaque,cur,&ins)!=0){s->badvaddr=cur;return -2;}
    s->pc=s->next_pc;s->next_pc+=4u;
    op=ins>>26;rs=(ins>>21)&31u;rt=(ins>>16)&31u;rd=(ins>>11)&31u;
    imm=ins&0xffffu;a=s->gpr[rs];b=s->gpr[rt];
    switch(op){
    case 0u:
        switch(ins&63u){
        case 0x21u:result=a+b;write=1;break;
        case 0x23u:result=a-b;write=1;break;
        case 0x24u:result=a&b;write=1;break;
        case 0x25u:result=a|b;write=1;break;
        case 0x2au:result=(int32_t)a<(int32_t)b;write=1;break;
        case 0x00u:result=b<<((ins>>6)&31u);rd=(ins>>11)&31u;write=1;break;
        case 0x08u:s->next_pc=a;break;
        default:return -3;
        }break;
    case 0x09u:result=a+(uint32_t)(int32_t)(int16_t)imm;write=1;break;
    case 0x0cu:result=a&imm;write=1;break;
    case 0x0du:result=a|imm;write=1;break;
    case 0x0fu:result=imm<<16;write=1;break;
    case 0x23u:
        target=a+(uint32_t)(int32_t)(int16_t)imm;
        if((target&3u)!=0u || bus->read32(bus->opaque,target,&result)!=0){s->badvaddr=target;return -2;}
        write=1;break;
    case 0x2bu:
        target=a+(uint32_t)(int32_t)(int16_t)imm;
        if((target&3u)!=0u || bus->write32(bus->opaque,target,b)!=0){s->badvaddr=target;return -2;}
        break;
    case 0x04u:if(a==b)s->next_pc=s->pc+((uint32_t)(int32_t)(int16_t)imm<<2);break;
    case 0x05u:if(a!=b)s->next_pc=s->pc+((uint32_t)(int32_t)(int16_t)imm<<2);break;
    case 0x02u:target=((s->pc)&UINT32_C(0xf0000000))|((ins&UINT32_C(0x03ffffff))<<2);s->next_pc=target;break;
    case 0x03u:result=s->pc+4u;write=1;rd=31u;target=((s->pc)&UINT32_C(0xf0000000))|((ins&UINT32_C(0x03ffffff))<<2);s->next_pc=target;break;
    default:return -3;
    }
    if(write && rt!=0u)s->gpr[rt]=result;
    if(op==0u && write && rd!=0u)s->gpr[rd]=result;
    s->gpr[0]=0u;++s->cycle_count;return 1;
}
