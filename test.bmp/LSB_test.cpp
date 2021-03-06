#define  _CRT_SECURE_NO_WARNINGS
#pragma  once
#include <string>
#include <stdio.h>
#include <Windows.h>
using namespace std;

class CBMPHide
{
public:
	CBMPHide();
	~CBMPHide();

	bool setBmpFileName(char* szFileName);	//设置Bmp文件名

	int getBmpWidth();	//获取宽度
	int getBmpHeight();	//获取高度
	int getBmpBitCount();	//获取Bit总数
	bool save();

	bool hideString2BMP(char* szStr2Hide);	//隐藏String到BMP文件中
	void showStringInBmp(char* szBmpFIleName = NULL);	//提取隐藏信息

	void savetxtFile(char* FileName);	//隐藏txt文件到bmp图像中
	void showtxtFile(char* szBmpFIleName = NULL);	//解密出txtFile
private:
	DWORD dwBmpSize;	//图片文件大小
	DWORD dwTxTSize;

	string sBmpFileName;
	string sTxTFileName;

	LPBYTE pBuf;	//用于存放图片信息的内存
	LPBYTE ptxtBuf;	//用于存放txt信息的内存

	BITMAPFILEHEADER* m_fileHdr;
	BITMAPINFOHEADER* m_infoHdr;
};

CBMPHide::CBMPHide()
{
	sBmpFileName = "";
	pBuf = 0;
	dwBmpSize = 0;
	ptxtBuf = 0;
}

CBMPHide::~CBMPHide()
{

}

bool CBMPHide::setBmpFileName(char* szFileName)
{
	this->sBmpFileName = szFileName;
	if (pBuf)	//如果已经生成就释放掉
	{
		delete[]pBuf;
	}

	HANDLE hfile = CreateFileA(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	//和struct BITMAPFILEHEADER bmfh里面的 bfSize的大小应该是一样的。
	dwBmpSize = GetFileSize(hfile, 0);	//获取文件的大小
	pBuf = new byte[dwBmpSize];
	DWORD dwRead = 0;
	ReadFile(hfile, pBuf, dwBmpSize, &dwRead, 0);
	if (dwRead != dwBmpSize)
	{
		delete[]pBuf;
		pBuf = 0;
		return false;
	}
	CloseHandle(hfile);
	m_fileHdr = (BITMAPFILEHEADER*)pBuf;
	m_infoHdr = (BITMAPINFOHEADER*)(pBuf + sizeof(BITMAPFILEHEADER));
	return true;	//成功话就是文件的内容读取到pBuf里面
}


int CBMPHide::getBmpWidth()
{
	return m_infoHdr->biWidth;
}

int CBMPHide::getBmpHeight()
{
	return m_infoHdr->biHeight;
}

int CBMPHide::getBmpBitCount()
{
	return m_infoHdr->biBitCount;
}

bool CBMPHide::save()
{
	string sDstFileName = sBmpFileName + ".hide.bmp";
	HANDLE hfile = CreateFileA(sDstFileName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS, 0, 0);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwWritten = 0;
	WriteFile(hfile, pBuf, dwBmpSize, &dwWritten, 0);
	if (dwBmpSize != dwWritten)
	{
		return false;
	}
	CloseHandle(hfile);
	return true;
}
//隐藏一个字符串到图片中，把字符串拆成字节，写入每个像素的alpha通道中
bool CBMPHide::hideString2BMP(char* szStr2Hide)
{
	LPBYTE pAlpha = pBuf + m_fileHdr->bfOffBits + 3;	//第一个像素的通道位置
	int nHide;	//成功隐藏的字节数
				//每次循环写入一个字节，写入alpha通道
				//(pAlpha - pBuf) < m_fileHdr->bfSize这个是判断字符串如果太大，图片不能隐藏
	for (nHide = 0; (pAlpha - pBuf) < m_fileHdr->bfSize && szStr2Hide[nHide] != 0; nHide++, pAlpha += 4)
	{
		*pAlpha = szStr2Hide[nHide];	//写入一个字节
	}

	return true;
}

void CBMPHide::showStringInBmp(char* szBmpFIleName/*=NULL*/)
{
	string sDstFileName = "";
	if (szBmpFIleName == 0)
	{
		sDstFileName = sBmpFileName + ".hide.bmp";
	}
	else
		sDstFileName = szBmpFIleName;

	HANDLE hfile = CreateFileA(sDstFileName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING, 0, 0);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		return;
	}
	DWORD dwSize = GetFileSize(hfile, 0);
	LPBYTE pBuf1 = new byte[dwSize];

	DWORD dwRead = 0;

	ReadFile(hfile, pBuf1, dwSize, &dwRead, 0);
	CloseHandle(hfile);

	//文件内容读取到pBuf1中
	BITMAPFILEHEADER *pHdr = (BITMAPFILEHEADER *)pBuf1;
	LPBYTE pStr = pBuf1 + pHdr->bfOffBits + 3;
	char szTmp[1280];
	RtlZeroMemory(szTmp, 1280);
	for (int i = 0; i < 1280; i++)
	{
		if (*pStr == 0 || *pStr == 0xFF)
		{
			break;
		}
		szTmp[i] = *pStr;
		pStr += 4;
	}
	printf_s(szTmp);

	delete[]pBuf1;
}

void CBMPHide::savetxtFile(char* FileName)
{
	FILE* fp = fopen(FileName, "rb");
	if (fp == NULL)
	{
		printf_s("文件打开失败\n");
		return;
	}

	char buf[128];	//0xFF=255
	int n = fread(buf, 1, 128, fp);

	if (n == 0)
	{
		printf_s("文件为空，隐藏失败！");
		return;
	}
	fclose(fp);
	this->hideString2BMP(buf);
	this->save();
}

void CBMPHide::showtxtFile(char* szBmpFIleName/*=NULL*/)
{
	string sDstFileName = "";
	if (szBmpFIleName == 0)
	{
		sDstFileName = sBmpFileName + ".hide.bmp";
	}
	else
		sDstFileName = szBmpFIleName;

	HANDLE hfile = CreateFileA(sDstFileName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING, 0, 0);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		return;
	}
	DWORD dwSize = GetFileSize(hfile, 0);
	LPBYTE pBuf1 = new byte[dwSize];

	DWORD dwRead = 0;

	ReadFile(hfile, pBuf1, dwSize, &dwRead, 0);
	CloseHandle(hfile);

	//文件内容读取到pBuf1中
	BITMAPFILEHEADER *pHdr = (BITMAPFILEHEADER *)pBuf1;
	LPBYTE pStr = pBuf1 + pHdr->bfOffBits + 3;
	char szTmp[1280];
	RtlZeroMemory(szTmp, 1280);
	for (int i = 0; i < 1280; i++)
	{
		if (*pStr == 0 || *pStr == 0xFF || *pStr == 0xCC)
		{
			break;
		}
		szTmp[i] = *pStr;
		pStr += 4;
	}
	printf_s(szTmp);

	const char* filename = "from bmp.txt";
	FILE* fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		printf_s("txt文件保存失败!");
		return;
	}
	fwrite(szTmp, 1, strlen(szTmp), fp);
	fclose(fp);

	delete[]pBuf1;
}

int main()
{
	CBMPHide hide;
	hide.setBmpFileName("test.bmp");
	printf_s("test.bmp width:%d,height:%d,bitCount%d\n",
		hide.getBmpWidth(),
		hide.getBmpHeight(),
		hide.getBmpBitCount());
	//hide.hideString2BMP("Hello Word");
	//hide.save();
	//hide.showStringInBmp("test.bmp.hide.bmp");
	hide.savetxtFile("1.txt");
	hide.showtxtFile("test.bmp.hide.bmp");//提取隐藏信息并打印出来
	getchar();
	return 0;
}


