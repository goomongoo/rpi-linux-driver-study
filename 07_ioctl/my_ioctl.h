#ifndef MY_IOCTL_H_
#define MY_IOCTL_H_

struct mystruct {
    int repeat;
    char name[64];
};

#define MY_MAGIC    'a'
#define WR_VAL      _IOW(MY_MAGIC, 0x01, int)
#define RD_VAL      _IOR(MY_MAGIC, 0x02, int)
#define GREET       _IOW(MY_MAGIC, 0x03, struct mystruct)

#endif