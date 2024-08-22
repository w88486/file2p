#include "Mhandler.h"

void Mhandler::Handle(MyinsMsg* _input)
{
	//ִ�е�ǰ����
	MyinsMsg *pret = InternelHandle(_input);
	
	if (nullptr != pret) {
		//��ȡ��һ������
		auto nextp = GetNext(pret);
		if (nullptr != nextp) {
			//ִ����һ��
			nextp->Handle(pret);
		}
		delete pret;
	}
}
