#include "device_fifo.h"
#include <string.h>
void ps2_device_fifos_reset(ps2_device_fifos_t *f)
{
    if(f!=0) memset(f,0,sizeof(*f));
}
int ps2_device_fifo_push(ps2_device_fifos_t *f,ps2_device_fifo_id_t id,const uint8_t q[16])
{
    ps2_device_fifo_t *fifo;
    if(f==0 || q==0 || (unsigned)id>=(unsigned)PS2_FIFO_COUNT)return -1;
    fifo=&f->fifo[id]; if(fifo->count>=PS2_DEVICE_FIFO_CAPACITY)return -2;
    memcpy(fifo->qword[fifo->write_index],q,16u);
    fifo->write_index=(fifo->write_index+1u)%PS2_DEVICE_FIFO_CAPACITY;
    ++fifo->count; return 0;
}
int ps2_device_fifo_pop(ps2_device_fifos_t *f,ps2_device_fifo_id_t id,uint8_t q[16])
{
    ps2_device_fifo_t *fifo;
    if(f==0 || q==0 || (unsigned)id>=(unsigned)PS2_FIFO_COUNT)return -1;
    fifo=&f->fifo[id]; if(fifo->count==0u)return -2;
    memcpy(q,fifo->qword[fifo->read_index],16u);
    fifo->read_index=(fifo->read_index+1u)%PS2_DEVICE_FIFO_CAPACITY;
    --fifo->count; return 0;
}
size_t ps2_device_fifo_count(const ps2_device_fifos_t *f,ps2_device_fifo_id_t id)
{
    if(f==0 || (unsigned)id>=(unsigned)PS2_FIFO_COUNT)return 0u;
    return f->fifo[id].count;
}
