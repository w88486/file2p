#pragma once
#include"MChannel.h"
#include"MyinsKernel.h"
#include"Mhandler.h"

//��̬����ת����õ�������?
#define GET_REF_FROM_MSG(type, ref, msg) type* pref = dynamic_cast<type*>(&msg); if(NULL == pref){return NULL;}; type& ref = dynamic_cast<type&>(msg); 
