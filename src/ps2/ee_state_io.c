#include "ee_state.h"

#include <string.h>

#define SNAPSHOT_HEADER_SIZE 16u
#define SNAPSHOT_MAGIC UINT32_C(0x53324545)

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
           ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static uint32_t checksum(const uint8_t *p, size_t n)
{
    uint32_t h=UINT32_C(2166136261); size_t i;
    for(i=0;i<n;i++){ h^=p[i]; h*=UINT32_C(16777619); }
    return h;
}
int ps2_ee_state_save(const ps2_ee_state_t *state, uint8_t *out,
                      size_t capacity, size_t *written)
{
    size_t payload=sizeof(*state), total=SNAPSHOT_HEADER_SIZE+payload;
    if(!state||!out||!written) return -1;
    *written=total;
    if(capacity<total) return -2;
    put_u32(out,SNAPSHOT_MAGIC);
    put_u32(out+4,PS2_EE_SNAPSHOT_VERSION);
    put_u32(out+8,(uint32_t)payload);
    memcpy(out+SNAPSHOT_HEADER_SIZE,state,payload);
    put_u32(out+12,checksum(out+SNAPSHOT_HEADER_SIZE,payload));
    return 0;
}
int ps2_ee_state_load(ps2_ee_state_t *state,const uint8_t *data,size_t size)
{
    ps2_ee_state_t candidate; uint32_t payload;
    if(!state||!data) return -1;
    if(size<SNAPSHOT_HEADER_SIZE||get_u32(data)!=SNAPSHOT_MAGIC) return -2;
    if(get_u32(data+4)!=PS2_EE_SNAPSHOT_VERSION) return -3;
    payload=get_u32(data+8);
    if(payload!=sizeof(candidate)||size!=SNAPSHOT_HEADER_SIZE+(size_t)payload) return -4;
    if(checksum(data+SNAPSHOT_HEADER_SIZE,payload)!=get_u32(data+12)) return -5;
    memcpy(&candidate,data+SNAPSHOT_HEADER_SIZE,sizeof(candidate));
    if(candidate.gpr[0]!=0u) return -6;
    *state=candidate;
    return 0;
}
