#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include   <unistd.h> 
#endif

#include "HCNetSDK.h"
#include "PlayM4.h"
#include "public.h"

#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <time.h>
#include <string.h>


LONG nPort = -1;
HWND hWnd = NULL;
cv::Mat image;


void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize,void* dwUser)
{
    printf("pyd---(private_v30)Get data,the size is %d,%d.\n", time(NULL), dwBufSize);
}

void CALLBACK g_HikDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize,DWORD dwUser)
{
    printf("pyd---(private)Get data,the size is %d.\n", dwBufSize);
}

void CALLBACK g_StdDataCallBack(int lRealHandle, unsigned int dwDataType, unsigned char *pBuffer, unsigned int dwBufSize, unsigned int dwUser)
{
    printf("pyd---(rtsp)Get data,the size is %d.\n", dwBufSize);
}


//解码回调 视频为YUV数据(YV12)，音频为PCM数据
void CALLBACK DecCBFun(int nPort, char * pBuf, int nSize, FRAME_INFO * pFrameInfo, void* nReserved1, int nReserved2)
{
	if (pFrameInfo->nType == T_YV12)
	{
		if (image.empty())
		{
			image.create(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);
		}
		cv::Mat YUVImage(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);
 
		cv::cvtColor(YUVImage, image, cv::COLOR_YUV2BGR_YV12);
		cv::resize(image, image, cv::Size(1000, 600));
		cv::imshow("view", image);
		cv::waitKey(15);
		YUVImage.~Mat();
	}
}


///实时流回调
void CALLBACK fRealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD:    //系统头
		if (!PlayM4_GetPort(&nPort)) //获取播放库未使用的通道号
		{
			nPort = -1;
			break;
		}
		if (dwBufSize > 0)
		{
			//实时流播放模式
			if (!PlayM4_SetStreamOpenMode(nPort, STREAME_REALTIME))
				break;
			//打开流接口
			if (!PlayM4_OpenStream(nPort, pBuffer, dwBufSize, 1024 * 1024 * 8))
				break;
			//播放开始
			if (!PlayM4_Play(nPort, NULL))
				break;
			//设置解码回调函数 仅仅解码不显示
			if (!PlayM4_SetDecCallBack(nPort, DecCBFun))
				break;
		}
		break;
	case NET_DVR_STREAMDATA:   //码流数据
		if (dwBufSize > 0 && nPort != -1)
		{
			if (!PlayM4_InputData(nPort, pBuffer, dwBufSize))
				break;
		}
		break;
	}

}


/*******************************************************************
      Function:   Demo_GetStream
   Description:   preview(no "_V30")
     Parameter:   (IN)   none  
        Return:   0--successful��-1--fail��   
**********************************************************************/
int Demo_GetStream()
{

    NET_DVR_Init();
    long lUserID;
    //login
    NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
    struLoginInfo.bUseAsynLogin = false;

    struLoginInfo.wPort = 8000;
    memcpy(struLoginInfo.sDeviceAddress, "192.168.3.204", NET_DVR_DEV_ADDRESS_MAX_LEN);
    memcpy(struLoginInfo.sUserName, "admin", NAME_LEN);
    memcpy(struLoginInfo.sPassword, "fj123456", NAME_LEN);
    lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);

    if (lUserID < 0)
    {
        printf("pyd1---Login error, %d\n", NET_DVR_GetLastError());
        return HPR_ERROR;
    }

    //Set callback function of getting stream.
    long lRealPlayHandle;
    NET_DVR_PREVIEWINFO struPlayInfo = {0};
    #if (defined(_WIN32) || defined(_WIN_WCE))
         struPlayInfo.hPlayWnd     = NULL;
    #elif defined(__linux__)
         struPlayInfo.hPlayWnd     = 0;
    #endif
    struPlayInfo.lChannel     = 1;  //channel NO
    struPlayInfo.dwLinkMode   = 0;
    struPlayInfo.bBlocked = 1;
    struPlayInfo.dwDisplayBufNum = 1;
    lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, NULL, NULL);

    if (lRealPlayHandle < 0)
    {
        printf("pyd1---NET_DVR_RealPlay_V40 error\n");
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return HPR_ERROR;
    }
    
    //Set callback function of getting stream.
    int iRet;
    iRet = NET_DVR_SetRealDataCallBack(lRealPlayHandle, g_HikDataCallBack, 0);
    if (!iRet)
    {
        printf("pyd1---NET_DVR_RealPlay_V40 error\n");
        NET_DVR_StopRealPlay(lRealPlayHandle);
        NET_DVR_Logout_V30(lUserID);
        NET_DVR_Cleanup();  
        return HPR_ERROR;
    }


#ifdef _WIN32
    Sleep(5000);  //millisecond
#elif  defined(__linux__)
    sleep(500);   //second
#endif

    //stop
    NET_DVR_StopRealPlay(lRealPlayHandle);
    NET_DVR_Logout_V30(lUserID);
    NET_DVR_Cleanup();
    return HPR_OK;

}

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
    char tempbuf[256] = {0};
    switch(dwType) 
    {
    case EXCEPTION_RECONNECT:			//Ԥ��ʱ����
        printf("pyd----------reconnect--------%d\n", time(NULL));
        break;
    default:
        break;
    }
};


/*******************************************************************
      Function:   Demo_GetStream_V30
   Description:   preview(_V30)
     Parameter:   (IN)   none  
        Return:   0--successful��-1--fail��   
**********************************************************************/
int Demo_GetStream_V30(LONG lUserID)
{
    //Set callback function of getting stream.
    long lRealPlayHandle;
    NET_DVR_PREVIEWINFO struPlayInfo = {0};
    #if (defined(_WIN32) || defined(_WIN_WCE))
         struPlayInfo.hPlayWnd     = NULL;
    #elif defined(__linux__)
         struPlayInfo.hPlayWnd     = 0;
    #endif
    struPlayInfo.lChannel     = 1;  //channel NO
    struPlayInfo.dwLinkMode   = 0;
    struPlayInfo.bBlocked = 1;
    struPlayInfo.dwDisplayBufNum = 1;
    lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, fRealDataCallBack_V30, NULL);
    NET_DVR_PTZControlWithSpeed(lRealPlayHandle, PAN_LEFT, 0, 2);
    sleep(1);
    NET_DVR_PTZControlWithSpeed(lRealPlayHandle, PAN_LEFT, 1, 2);
    //lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, NULL, NULL, 0);
    if (lRealPlayHandle < 0)
    {
        printf("pyd---NET_DVR_RealPlay_V40  error, %d\n", NET_DVR_GetLastError());

        return HPR_ERROR;
    }

    //Set rtsp callback function of getting stream.
    //NET_DVR_SetStandardDataCallBack(lRealPlayHandle, g_StdDataCallBack, 0);

#ifdef _WIN32
    Sleep(3000);  //millisecond
#elif  defined(__linux__) ||defined(__APPLE__)
    sleep(5);    //second
#endif
    //stop
    //NET_DVR_StopRealPlay(lRealPlayHandle);
	//NET_DVR_Logout(lUserID);
	//NET_DVR_Logout_V30(lUserID);
	//cleanup
    //cv::destroyWindow("view")
	//NET_DVR_Cleanup();
   
    return HPR_OK;
}



void Demo_SDK_Ability()
{
    NET_DVR_Init();
    NET_DVR_SDKABL struSDKABL = {0};
    if (NET_DVR_GetSDKAbility(&struSDKABL))
    {
        printf("SDK Max: %d\n", struSDKABL.dwMaxRealPlayNum);
        NET_DVR_Cleanup();
        return;
    }

    NET_DVR_Cleanup();
    return;
}

void Demo_SDK_Version()
{
    unsigned int uiVersion = NET_DVR_GetSDKBuildVersion();

    char strTemp[1024] = {0};
    sprintf(strTemp, "HCNetSDK V%d.%d.%d.%d\n", \
        (0xff000000 & uiVersion)>>24, \
        (0x00ff0000 & uiVersion)>>16, \
        (0x0000ff00 & uiVersion)>>8, \
        (0x000000ff & uiVersion));
    printf(strTemp);
}

