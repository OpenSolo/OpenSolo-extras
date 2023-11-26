#ifndef RINGBUF_H_
#define RINGBUF_H_

typedef struct rb RingBuf;

/**
 * Structure for basic ring char buffer, compiled in size
 */
struct rb {
    unsigned char* buf;                            ///< buffer storage
    int top;                                        ///< points to next open byte to push to
    int bot;                                        ///< points to oldest byte
    int cap;                                        ///< ring buffer capacity, passed in by user
    int (*push)(RingBuf* this, unsigned char c); ///< push function
    unsigned char (*pop)( RingBuf* this);         ///< pop function
    int (*size)(RingBuf* this);                  ///< current size
    int init;
};

void InitRingBuf(RingBuf* r, unsigned char* buf, int size);

#endif /* RINGBUF_H_ */
