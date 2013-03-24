#pragma once


#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#include <dinputd.h>

struct XINPUT_DEVICE_NODE
{
    DWORD dwVidPid;
    XINPUT_DEVICE_NODE* pNext;
};

struct DI_ENUM_CONTEXT
{
    DIJOYCONFIG* pPreferredJoyCfg;
    bool bPreferredJoyCfgValid;
};


class CDirectInput8Manager : public DIJOYSTATE2
{
private:
	HWND m_hWnd;
	LPDIRECTINPUT8       m_pDI; //              = NULL;         
	LPDIRECTINPUTDEVICE8 m_pJoystick;//        = NULL;     

	bool    m_bFilterOutXinputDevices; // = false;
	XINPUT_DEVICE_NODE* m_pXInputDeviceList; // = NULL;

	DI_ENUM_CONTEXT m_EnumContext;
  
public:
	HWND GetWindow() {return m_hWnd;}
	DI_ENUM_CONTEXT* GetEnumContext() {return &m_EnumContext;}
	LPDIRECTINPUT8 GetDirectInput8() {return m_pDI;}
	LPDIRECTINPUTDEVICE8 GetJoystick() {return m_pJoystick;}
	LPDIRECTINPUTDEVICE8* GetppJoyStick() {return &m_pJoystick;}
	bool GetFilterOutXinputDevices() {return m_bFilterOutXinputDevices;}
public:
	CDirectInput8Manager(HWND hWnd);
	~CDirectInput8Manager(void);

private:
	HRESULT InitDirectInput(HWND hWnd);
	VOID FreeDirectInput();
	HRESULT SetupForIsXInputDevice();
	void CleanupForIsXInputDevice();

public:
	HRESULT UpdateInputState( /*HWND hDlg*/ );
	
public:
	bool IsXInputDevice( const GUID* pGuidProductFromDirectInput );

};

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

