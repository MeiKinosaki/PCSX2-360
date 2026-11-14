#include "ee_state.h"
#include "ee_memory.h"
#include <string.h>
#define H 16u
#define MAGIC_EE UINT32_C(0x53324545)
#define MAGIC_MACHINE UINT32_C(0x4d325345)
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static uint32_t get(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint32_t sum(const uint8_t*p,size_t n){uint32_t h=UINT32_C(2166136261);size_t i;for(i=0;i<n;i++){h^=p[i];h*=UINT32_C(16777619);}return h;}
int ps2_ee_state_save(const ps2_ee_state_t*s,uint8_t*out,size_t cap,size_t*w){size_t n=sizeof(*s),z=H+n;if(!s||!out||!w)return -1;*w=z;if(cap<z)return -2;put(out,MAGIC_EE);put(out+4,PS2_EE_SNAPSHOT_VERSION);put(out+8,(uint32_t)n);memcpy(out+H,s,n);put(out+12,sum(out+H,n));return 0;}
int ps2_ee_state_load(ps2_ee_state_t*s,const uint8_t*d,size_t z){ps2_ee_state_t c;uint32_t n;if(!s||!d)return -1;if(z<H||get(d)!=MAGIC_EE)return -2;if(get(d+4)!=PS2_EE_SNAPSHOT_VERSION)return -3;n=get(d+8);if(n!=sizeof(c)||z!=H+(size_t)n)return -4;if(sum(d+H,n)!=get(d+12))return -5;memcpy(&c,d+H,sizeof(c));if(c.gpr[0])return -6;*s=c;return 0;}
int ps2_ee_machine_snapshot_save(const ps2_ee_state_t*s,const ps2_ee_tlb_arch_t*t,const ps2_ee_memory_t*m,uint8_t*out,size_t cap,size_t*w){size_t n,z;uint8_t*p;if(!s||!t||!m||!m->ram||!out||!w)return -1;if(m->ram_size>UINT32_MAX||m->ram_size>SIZE_MAX-sizeof(*s)-sizeof(*t)-4u-H)return -2;n=sizeof(*s)+sizeof(*t)+4u+m->ram_size;z=H+n;*w=z;if(cap<z)return -3;put(out,MAGIC_MACHINE);put(out+4,1u);put(out+8,(uint32_t)n);p=out+H;memcpy(p,s,sizeof(*s));p+=sizeof(*s);memcpy(p,t,sizeof(*t));p+=sizeof(*t);put(p,(uint32_t)m->ram_size);p+=4;memcpy(p,m->ram,m->ram_size);put(out+12,sum(out+H,n));return 0;}
int ps2_ee_machine_snapshot_load(ps2_ee_state_t*s,ps2_ee_tlb_arch_t*t,ps2_ee_memory_t*m,const uint8_t*d,size_t z){ps2_ee_state_t ns;ps2_ee_tlb_arch_t nt;uint32_t n,r;const uint8_t*p;if(!s||!t||!m||!m->ram||!d)return -1;if(z<H||get(d)!=MAGIC_MACHINE)return -2;if(get(d+4)!=1u)return -3;n=get(d+8);if(z!=H+(size_t)n||sum(d+H,n)!=get(d+12))return -4;if(n<sizeof(ns)+sizeof(nt)+4u)return -5;p=d+H;memcpy(&ns,p,sizeof(ns));p+=sizeof(ns);memcpy(&nt,p,sizeof(nt));p+=sizeof(nt);r=get(p);p+=4;if(r!=m->ram_size||n!=sizeof(ns)+sizeof(nt)+4u+(size_t)r)return -6;if(ns.gpr[0])return -7;memcpy(m->ram,p,r);*s=ns;*t=nt;return 0;}
