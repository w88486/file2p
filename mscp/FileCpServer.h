#ifndef FILECPSERVER_H
#define FILECPSERVER_H

#include "../kernel/typedef.h"
#include "../kernel/Mhandler.h"

class FileCpServer : public Mhandler
{
    Mhandler *m_out = NULL;
    Arg arg;

public:
    FileCpServer(Arg _arg) : arg(_arg) {};
    virtual MyinsMsg *InternelHandle(MyinsMsg *_input);
    virtual Mhandler *GetNext(MyinsMsg *_input);
    virtual ~FileCpServer()
    {
        if (NULL != m_out)
        {
            delete m_out;
            m_out = NULL;
        }
    }
};

class UpLoad : public Mhandler
{

    Mhandler *m_out = NULL;
    Arg arg;

public:
    UpLoad(Arg _arg = Arg(-1)) : arg(_arg) {}
    void writehash(int fd, string filename, int mdtxt);
    void open_new_block(int *out, string infile, int id, char *filename);
    DeviceMsg* splitfile(string infile, int *mdtxt, long blocksize);
    virtual MyinsMsg *InternelHandle(MyinsMsg *_input);
    virtual Mhandler *GetNext(MyinsMsg *_input);
    virtual ~UpLoad()
    {
        if (NULL != m_out)
        {
            delete m_out;
            m_out = NULL;
        }
    }
};

class ReUpLoad : public Mhandler
{
    Mhandler *m_out = NULL;
    Arg arg;

public:
    ReUpLoad(Arg _arg) : arg(_arg) {};
    virtual MyinsMsg *InternelHandle(MyinsMsg *_input);
    virtual Mhandler *GetNext(MyinsMsg *_input);
    virtual ~ReUpLoad()
    {
        if (NULL != m_out)
        {
            delete m_out;
            m_out = NULL;
        }
    }
};
#endif
