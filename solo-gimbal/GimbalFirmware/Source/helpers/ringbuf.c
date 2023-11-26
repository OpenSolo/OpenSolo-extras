#include "helpers/ringbuf.h"

/* Below ring buffers are ensured to only produce
 * in one thread and consume in another, thereby
 * not requiring the buffers to be thread safe
 * -RCM
 */
//#define RB_THREAD_SAFE

/**
 * Push data c into buffer object r
 */
static int internal_rb_push(RingBuf* r, unsigned char c)
{
    int ret = 1;

#ifdef RB_THREAD_SAFE
    unsigned int int_status;
    int_status = INTDisableInterrupts();
#endif

    r->buf[r->top++] = c;

    // wrap ptr
    if (r->top >= r->cap) {
        r->top = 0;
    }

    // make sure we didn't overrun
    if (r->top == r->bot) {
        if (r->top == 0) {
            r->top = r->cap;
        } else {
            r->top--;
        }

        ret = 0;
    }

#ifdef RB_THREAD_SAFE
    INTRestoreInterrupts(int_status);
#endif

    return ret;
}

/**
 * Pop and return data from buffer object r
 */
static unsigned char internal_rb_pop(RingBuf* r)
{
    unsigned char c;

#ifdef RB_THREAD_SAFE
    unsigned int int_status;
    int_status = INTDisableInterrupts();
#endif

    // note: no protection for empty buf
    c = r->buf[r->bot++];

    // wrap ptr
    if (r->bot >= r->cap) {
        r->bot = 0;
    }

#ifdef RB_THREAD_SAFE
    INTRestoreInterrupts(int_status);
#endif

    return c;
}

/**
 * Return number of elements in buffer object r
 */
static int internal_rb_size(RingBuf* r)
{
    if (r->top == r->bot) {
        return 0;
    } else if (r->top > r->bot) {
        return r->top - r->bot;
    } else {
        return r->top - r->bot + r->cap;
    }
}

/**
 * Initialize buffer object r
 */
void InitRingBuf(RingBuf* r, unsigned char* buf, int size)
{
    r->buf = buf;
    r->cap = size;
    r->top = 0;
    r->bot = 0;
    r->push = internal_rb_push;
    r->pop = internal_rb_pop;
    r->size = internal_rb_size;
    r->init = 1;
}
