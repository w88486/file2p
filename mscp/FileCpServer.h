#include "../kernel/typedef.h"
#include "../kernel/Mhandler.h"

class FileCpServer : public Mhandler
{
    Mhandler *m_out = NULL;
    Arg arg;
public:
    FileCpServer(Arg _arg) :arg(_arg){};
    virtual MyinsMsg* InternelHandle(MyinsMsg* _input);
	virtual Mhandler* GetNext(MyinsMsg* _input);
    virtual ~FileCpServer(){if (NULL != m_out) delete m_out;}
};

class DownLoad : public Mhandler
{
    friend class ReDownLoad;
    Mhandler *m_out = NULL;
    Arg arg;
    EVP_MD_CTX *filectx;
public:
    DownLoad(Arg _arg) :arg(_arg){};
    virtual MyinsMsg* InternelHandle(MyinsMsg* _input);
	virtual Mhandler* GetNext(MyinsMsg* _input);
    virtual ~DownLoad(){if (NULL != m_out) delete m_out;}
};

class RedownLoad : public Mhandler
{
    Mhandler *m_out = NULL;
    Arg arg;
public:
    RedownLoad(Arg _arg) :arg(_arg){};
    virtual MyinsMsg* InternelHandle(MyinsMsg* _input);
	virtual Mhandler* GetNext(MyinsMsg* _input);
    virtual ~RedownLoad(){if (NULL != m_out) delete m_out;}
};

class CombineFile : public Mhandler
{
    Mhandler *m_out = NULL;
    Arg arg;
public:
    CombineFile(Arg _arg) :arg(_arg){};
    virtual MyinsMsg* InternelHandle(MyinsMsg* _input);
	virtual Mhandler* GetNext(MyinsMsg* _input);
    virtual ~CombineFile(){if (NULL != m_out) delete m_out;}
};

class UpLoad : public Mhandler
{

    Mhandler *m_out = NULL;
    Arg arg;
public:
    UpLoad(Arg _arg) :arg(_arg){}
    int checkupload(string filename);
    void reupload(long int blocksize);
    void writehash(int fd, string filename, int mdtxt);
    void open_new_block(int *out, string infile, int id, char *filename);
    CmdMsg splitfile(string infile, int *mdtxt, long blocksize);
    virtual MyinsMsg* InternelHandle(MyinsMsg* _input);
	virtual Mhandler* GetNext(MyinsMsg* _input);
    virtual ~UpLoad(){if (NULL != m_out) delete m_out;}
};

class DisplayFile : public Mhandler
{
    Mhandler *m_out = NULL;
    Arg arg;
public:
    DisplayFile(Arg _arg) :arg(_arg){};
    virtual MyinsMsg* InternelHandle(MyinsMsg* _input);
	virtual Mhandler* GetNext(MyinsMsg* _input);
    virtual ~DisplayFile(){if (NULL != m_out) delete m_out;}
};

class RedownLoad : public Mhandler
{
    Mhandler *m_out = NULL;
    Arg arg;
public:
    RedownLoad(Arg _arg) :arg(_arg){};
    virtual MyinsMsg* InternelHandle(MyinsMsg* _input);
	virtual Mhandler* GetNext(MyinsMsg* _input);
    virtual ~RedownLoad(){if (NULL != m_out) delete m_out;}
};