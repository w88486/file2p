#pragma once
#include "Mhandler.h"
#include "MRole.h"
#include "MChannel.h"
class MProtocol :
    public Mhandler
{
	/*ԭʼ���ݺ�ҵ�������໥��������������д�ú�����ʵ��Э��*/
	virtual UserData* raw2request(std::string _szInput) = 0;

	/*ԭʼ���ݺ�ҵ�������໥��������������д�ú�����ʵ��Э��*/
	virtual std::string* response2raw(UserData& _oUserData) = 0;
protected:
	/*��ȡ�����ɫ��������������Ӧ����д�ú���������ָ����ǰ�������û�������ϢӦ�ô��ݸ��ĸ���ɫ����*/
	virtual MRole* GetMsgProcessor(UserData& _oUserData) = 0;

	/*��ȡ����ͨ��������������Ӧ����д�ú���������ָ����ǰ�ֽ���Ӧ�����ĸ�ͨ�����󷢳�*/
	virtual MChannel* GetMsgSender(ByteMsg& _oBytes) = 0;
    // ͨ�� Mhandler �̳�
    MyinsMsg* InternelHandle(MyinsMsg* _input) override;
    Mhandler* GetNext(MyinsMsg* _input) override;
};

