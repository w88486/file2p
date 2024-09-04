#include "zinx.h"

using namespace std;

void AZinxHandler::Handle(IZinxMsg & _oInput)
{
	IZinxMsg *poNextMsg = NULL;
	AZinxHandler *poNextHandler = NULL;

	poNextMsg = InternelHandle(_oInput);
	if (NULL != poNextMsg)
	{
		poNextHandler = GetNextHandler(*poNextMsg);
		if (NULL != poNextHandler)
		{
			poNextHandler->Handle(*poNextMsg);
		}

		//delete poNextMsg;
	}

	return;
}

IZinxMsg *Ichannel::InternelHandle(IZinxMsg & _oInput)
{
	IZinxMsg *poRet = NULL;
	GET_REF2DATA(SysIOReadyMsg, oIoStat, _oInput);
	if (oIoStat.m_emIoDic == SysIOReadyMsg::IN)
	{
		BytesMsg *poBytes = new BytesMsg(oIoStat);
		if (true == ReadFd(poBytes->szData))
		{
			poBytes->szInfo = GetChannelInfo();
			poRet = poBytes;
		}
		else
		{
			delete poBytes;
		}
	}
	else if (oIoStat.m_emIoDic == SysIOReadyMsg::OUT)
	{
		GET_REF2DATA(BytesMsg, oBytes, _oInput);
		//���뷢�ͻ�����
		Push(oBytes.szData);
		ZinxKernel::Zinx_SetChannelOut(*this);
		// 问题
		if (false == HasOutput())
		{
			ZinxKernel::Zinx_ClearChannelOut(*this);
		}
	}

	return poRet;
}

void Ichannel::FlushOut()
{
	while (false == IsEmpty())
	{
		std::string data = Pop();
		if (true == WriteFd(data))
		{
			
		}
		else
		{
			/*未发送成功，重新放入缓冲区*/
			Push(data);
			break;
		}
	}
}

AZinxHandler* Iprotocol::GetNextHandler(IZinxMsg & _oNextMsg)
{
	AZinxHandler *poRet = NULL;

	GET_REF2DATA(SysIOReadyMsg, oIoStat, _oNextMsg);
	if (oIoStat.m_emIoDic == SysIOReadyMsg::IN)
	{
		GET_REF2DATA(UserDataMsg, oUserDataMsg, _oNextMsg);
		poRet = GetMsgProcessor(oUserDataMsg);
	}
	else if (oIoStat.m_emIoDic == SysIOReadyMsg::OUT)
	{
		GET_REF2DATA(BytesMsg, obytes, _oNextMsg);
		poRet = GetMsgSender(obytes);
	}

	return poRet;
}

AZinxHandler *Ichannel::GetNextHandler(IZinxMsg & _oNextMsg)
{
	AZinxHandler *poRet = NULL;
	GET_REF2DATA(SysIOReadyMsg, oIoStat, _oNextMsg);
	if (oIoStat.m_emIoDic == SysIOReadyMsg::IN)
	{
		GET_REF2DATA(BytesMsg, oBytes, _oNextMsg);
		poRet = GetInputNextStage(oBytes);
	}

	return poRet;
}

IZinxMsg *Iprotocol::InternelHandle(IZinxMsg & _oInput)
{
	IZinxMsg *poRet = NULL;
	GET_REF2DATA(SysIOReadyMsg, oIoStat, _oInput);
	if (oIoStat.m_emIoDic == SysIOReadyMsg::IN)
	{
		GET_REF2DATA(BytesMsg, oBytes, _oInput);
		UserData *poUserData = raw2request(oBytes.szData);
		if (NULL != poUserData)
		{
			UserDataMsg *poUserDataMsg = new UserDataMsg(oBytes);
			poUserDataMsg->poUserData = poUserData;
			poRet = poUserDataMsg;
		}
	}
	else if (oIoStat.m_emIoDic == SysIOReadyMsg::OUT)
	{
		GET_REF2DATA(UserDataMsg, oUserDataMsg, _oInput);
		string *pszData = response2raw(*(oUserDataMsg.poUserData));
		if (NULL != pszData)
		{
			BytesMsg *poBytes = new BytesMsg(oIoStat);
			poBytes->szData = *pszData;
			delete pszData;
			poRet = poBytes;
		}
	}

	return poRet;
}

IZinxMsg *Irole::InternelHandle(IZinxMsg & _oInput)
{
	IZinxMsg *poRet = NULL;
	GET_REF2DATA(SysIOReadyMsg, oIoStat, _oInput);
	if (oIoStat.m_emIoDic == SysIOReadyMsg::IN)
	{
		GET_REF2DATA(UserDataMsg, oUserDataMsg, _oInput);
		UserData *poUserData = ProcMsg(*(oUserDataMsg.poUserData));
		if (NULL != poUserData)
		{
			UserDataMsg *poUserDataMsg = new UserDataMsg(oUserDataMsg);
			poUserDataMsg->poUserData = poUserData;
			poRet = poUserDataMsg;
		}
	}

	return poRet;
}

AZinxHandler *Irole::GetNextHandler(IZinxMsg & _oNextMsg)
{
	GET_REF2DATA(UserDataMsg, oUserDataMsg, _oNextMsg);
	return poNextProcessor;
}

std::string Ichannel::Convert2Printable(std::string &_szData)
{
	char *pcTemp = (char *)calloc(1UL, _szData.size() * 3 + 1);
	int iCurPos = 0;

	if (NULL != pcTemp)
	{
		for (int i = 0; i < _szData.size(); ++i)
		{
			iCurPos += sprintf(pcTemp + iCurPos, "%02X ", (unsigned char)_szData[i]);
		}
		pcTemp[iCurPos] = 0;
	}

	std::string szRet = std::string(pcTemp);
	free(pcTemp);

	return szRet;
}