#ifndef _LIBACC_H_
#define _LIBACC_H_

class ACCELERATOR{
    virtual bool bootstrap() = 0;
    virtual bool setServerKey() = 0;
};

#endif

