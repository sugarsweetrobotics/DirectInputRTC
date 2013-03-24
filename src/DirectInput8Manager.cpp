#include <windows.h>
#include <commctrl.h>
#include <basetsd.h>
#include <assert.h>
#include <stdio.h>

#include "DirectInput8Manager.h"

#include <oleauto.h>
#include <shellapi.h>

#include <wbemidl.h>


BOOL CALLBACK    EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
BOOL CALLBACK    EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );



CDirectInput8Manager::CDirectInput8Manager(HWND hWnd) :
m_pDI(NULL), m_pJoystick(NULL), m_bFilterOutXinputDevices(false), m_pXInputDeviceList(NULL)
{
	m_hWnd = hWnd;
	HRESULT hres = InitDirectInput(hWnd);
}

CDirectInput8Manager::~CDirectInput8Manager(void)
{
	FreeDirectInput();
}


//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT CDirectInput8Manager::InitDirectInput( HWND hDlg )
{
    HRESULT hr;

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    // Create a DInput object
    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
                                         IID_IDirectInput8, (VOID**)&m_pDI, NULL ) ) )
        return hr;


    if( m_bFilterOutXinputDevices )
        SetupForIsXInputDevice();

    DIJOYCONFIG PreferredJoyCfg = {0};
//    DI_ENUM_CONTEXT m_enumContext;
    m_EnumContext.pPreferredJoyCfg = &PreferredJoyCfg;
    m_EnumContext.bPreferredJoyCfgValid = false;

    IDirectInputJoyConfig8* pJoyConfig = NULL;  
    if( FAILED( hr = m_pDI->QueryInterface( IID_IDirectInputJoyConfig8, (void **) &pJoyConfig ) ) )
        return hr;

    PreferredJoyCfg.dwSize = sizeof(PreferredJoyCfg);
    if( SUCCEEDED( pJoyConfig->GetConfig(0, &PreferredJoyCfg, DIJC_GUIDINSTANCE ) ) ) // This function is expected to fail if no Joystick is attached
        m_EnumContext.bPreferredJoyCfgValid = true;
    SAFE_RELEASE( pJoyConfig );

    // Look for a simple Joystick we can use for this sample program.
    if( FAILED( hr = m_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, 
                                         EnumJoysticksCallback,
                                         this, DIEDFL_ATTACHEDONLY ) ) )
        return hr;

    if( m_bFilterOutXinputDevices )
        CleanupForIsXInputDevice();

    // Make sure we got a Joystick
    if( NULL == m_pJoystick )
    {
		return E_FAIL;
    }

    // Set the data format to "simple Joystick" - a predefined data format 
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    if( FAILED( hr = m_pJoystick->SetDataFormat( &c_dfDIJoystick2 ) ) )
        return hr;

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    if( FAILED( hr = m_pJoystick->SetCooperativeLevel( hDlg, DISCL_EXCLUSIVE | 
                                                             DISCL_BACKGROUND ) ) )
        return hr;

    // Enumerate the Joystick objects. The callback function enabled user
    // interface elements for objects that are found, and sets the min/max
    // values property for discovered axes.
    if( FAILED( hr = m_pJoystick->EnumObjects( EnumObjectsCallback, 
                                                (VOID*)this, DIDFT_ALL ) ) )
        return hr;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
VOID CDirectInput8Manager::FreeDirectInput()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
    if( m_pJoystick ) 
        m_pJoystick->Unacquire();
    
    // Release any DirectInput objects.
    SAFE_RELEASE( m_pJoystick );
    SAFE_RELEASE( m_pDI );
}



//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it’s an XInput device
// Unfortunately this information can not be found by just using DirectInput.
// Checking against a VID/PID of 0x028E/0x045E won't find 3rd party or future 
// XInput devices.
//
// This function stores the list of xinput devices in a linked list 
// at g_pXInputDeviceList, and IsXInputDevice() searchs that linked list
//-----------------------------------------------------------------------------
HRESULT CDirectInput8Manager::SetupForIsXInputDevice()
{
    IWbemServices*          pIWbemServices = NULL;
    IEnumWbemClassObject*   pEnumDevices   = NULL;
    IWbemLocator*           pIWbemLocator  = NULL;
    IWbemClassObject*       pDevices[20]   = {0};
    BSTR                    bstrDeviceID   = NULL;
    BSTR                    bstrClassName  = NULL;
    BSTR                    bstrNamespace  = NULL;
    DWORD                   uReturned      = 0;
    bool                    bCleanupCOM    = false;
    UINT                    iDevice        = 0;
    VARIANT                 var;
    HRESULT                 hr;

    // CoInit if needed
    hr = CoInitialize(NULL);
    bCleanupCOM = SUCCEEDED(hr);

    // Create WMI
    hr = CoCreateInstance( __uuidof(WbemLocator),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           __uuidof(IWbemLocator),
                           (LPVOID*) &pIWbemLocator);
    if( FAILED(hr) || pIWbemLocator == NULL ) 
        goto LCleanup; 

    // Create BSTRs for WMI
    bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" ); if( bstrNamespace == NULL ) goto LCleanup;
    bstrDeviceID  = SysAllocString( L"DeviceID" );           if( bstrDeviceID == NULL )  goto LCleanup;        
    bstrClassName = SysAllocString( L"Win32_PNPEntity" );    if( bstrClassName == NULL ) goto LCleanup;        
    
    // Connect to WMI 
    hr = pIWbemLocator->ConnectServer( bstrNamespace, NULL, NULL, 0L, 
                                       0L, NULL, NULL, &pIWbemServices );
    if( FAILED(hr) || pIWbemServices == NULL )
        goto LCleanup; 

    // Switch security level to IMPERSONATE
    CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0 );                    

    // Get list of Win32_PNPEntity devices
    hr = pIWbemServices->CreateInstanceEnum( bstrClassName, 0, NULL, &pEnumDevices ); 
    if( FAILED(hr) || pEnumDevices == NULL )
        goto LCleanup;

    // Loop over all devices
    for( ;; )
    {
        // Get 20 at a time
        hr = pEnumDevices->Next( 10000, 20, pDevices, &uReturned );
        if( FAILED(hr) )
            goto LCleanup;
        if( uReturned == 0 )
            break;

        for( iDevice=0; iDevice<uReturned; iDevice++ )
        {
            // For each device, get its device ID
            hr = pDevices[iDevice]->Get( bstrDeviceID, 0L, &var, NULL, NULL );
            if( SUCCEEDED( hr ) && var.vt == VT_BSTR && var.bstrVal != NULL )
            {
                // Check if the device ID contains "IG_".  If it does, then it’s an XInput device
                // Unfortunately this information can not be found by just using DirectInput 
                if( wcsstr( var.bstrVal, L"IG_" ) )
                {
                    // If it does, then get the VID/PID from var.bstrVal
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
                    if( strVid && swscanf( strVid, L"VID_%4X", &dwVid ) != 1 )
                        dwVid = 0;
                    WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
                    if( strPid && swscanf( strPid, L"PID_%4X", &dwPid ) != 1 )
                        dwPid = 0;

                    DWORD dwVidPid = MAKELONG( dwVid, dwPid );

                    // Add the VID/PID to a linked list
                    XINPUT_DEVICE_NODE* pNewNode = new XINPUT_DEVICE_NODE;
                    if( pNewNode )
                    {
                        pNewNode->dwVidPid = dwVidPid;
                        pNewNode->pNext = m_pXInputDeviceList;
                        m_pXInputDeviceList = pNewNode;
                    }
                }
            }   
            SAFE_RELEASE( pDevices[iDevice] );
        }
    }

LCleanup:
    if(bstrNamespace)
        SysFreeString(bstrNamespace);
    if(bstrDeviceID)
        SysFreeString(bstrDeviceID);
    if(bstrClassName)
        SysFreeString(bstrClassName);
    for( iDevice=0; iDevice<20; iDevice++ )
        SAFE_RELEASE( pDevices[iDevice] );
    SAFE_RELEASE( pEnumDevices );
    SAFE_RELEASE( pIWbemLocator );
    SAFE_RELEASE( pIWbemServices );

    return hr;
}



//-----------------------------------------------------------------------------
// Cleanup needed for IsXInputDevice()
//-----------------------------------------------------------------------------
void CDirectInput8Manager::CleanupForIsXInputDevice()
{
    // Cleanup linked list
    XINPUT_DEVICE_NODE* pNode = m_pXInputDeviceList;
    while( pNode )
    {
        XINPUT_DEVICE_NODE* pDelete = pNode;
        pNode = pNode->pNext;
        SAFE_DELETE( pDelete );
    }
}


//-----------------------------------------------------------------------------
// Name: EnumJoysticksCallback()
// Desc: Called once for each enumerated Joystick. If we find one, create a
//       device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
                                     VOID* pContext )
{
	CDirectInput8Manager* pDIManager = (CDirectInput8Manager*)pContext;
	DI_ENUM_CONTEXT* pEnumContext = (DI_ENUM_CONTEXT*) pDIManager->GetEnumContext();
    HRESULT hr;

    if( pDIManager->GetFilterOutXinputDevices() && pDIManager->IsXInputDevice( &pdidInstance->guidProduct ) )
        return DIENUM_CONTINUE;

    // Skip anything other than the perferred Joystick device as defined by the control panel.  
    // Instead you could store all the enumerated Joysticks and let the user pick.
    if( pEnumContext->bPreferredJoyCfgValid &&
        !IsEqualGUID( pdidInstance->guidInstance, pEnumContext->pPreferredJoyCfg->guidInstance ) )
        return DIENUM_CONTINUE;

    // Obtain an interface to the enumerated Joystick.
	hr = pDIManager->GetDirectInput8()->CreateDevice( pdidInstance->guidInstance, pDIManager->GetppJoyStick(), NULL );

    // If it failed, then we can't use this Joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if( FAILED(hr) ) 
        return DIENUM_CONTINUE;

    // Stop enumeration. Note: we're just taking the first Joystick we get. You
    // could store all the enumerated Joysticks and let the user pick.
    return DIENUM_STOP;
}


//-----------------------------------------------------------------------------
// Name: EnumObjectsCallback()
// Desc: Callback function for enumerating objects (axes, buttons, POVs) on a 
//       Joystick. This function enables user interface elements for objects
//       that are found to exist, and scales axes min/max values.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                   VOID* pContext )
{
	CDirectInput8Manager *pDIManager = (CDirectInput8Manager*)pContext;
	HWND hDlg = pDIManager->GetWindow();

    static int nSliderCount = 0;  // Number of returned slider controls
    static int nPOVCount = 0;     // Number of returned POV controls

    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
    if( pdidoi->dwType & DIDFT_AXIS )
    {
        DIPROPRANGE diprg; 
        diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
        diprg.diph.dwHow        = DIPH_BYID; 
        diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin              = -1000; 
        diprg.lMax              = +1000; 
    
        // Set the range for the axis
        if( FAILED( pDIManager->GetJoystick()->SetProperty( DIPROP_RANGE, &diprg.diph ) ) ) 
            return DIENUM_STOP;
         
    }

/***
    // Set the UI to reflect what objects the Joystick supports
    if (pdidoi->guidType == GUID_XAxis)
    {
        EnableWindow( GetDlgItem( hDlg, IDC_X_AXIS ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_X_AXIS_TEXT ), TRUE );
    }
    if (pdidoi->guidType == GUID_YAxis)
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Y_AXIS ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Y_AXIS_TEXT ), TRUE );
    }
    if (pdidoi->guidType == GUID_ZAxis)
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Z_AXIS ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Z_AXIS_TEXT ), TRUE );
    }
    if (pdidoi->guidType == GUID_RxAxis)
    {
        EnableWindow( GetDlgItem( hDlg, IDC_X_ROT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_X_ROT_TEXT ), TRUE );
    }
    if (pdidoi->guidType == GUID_RyAxis)
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Y_ROT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Y_ROT_TEXT ), TRUE );
    }
    if (pdidoi->guidType == GUID_RzAxis)
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Z_ROT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Z_ROT_TEXT ), TRUE );
    }
    if (pdidoi->guidType == GUID_Slider)
    {
        switch( nSliderCount++ )
        {
            case 0 :
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER0 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER0_TEXT ), TRUE );
                break;

            case 1 :
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER1 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER1_TEXT ), TRUE );
                break;
        }
    }
    if (pdidoi->guidType == GUID_POV)
    {
        switch( nPOVCount++ )
        {
            case 0 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV0 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV0_TEXT ), TRUE );
                break;

            case 1 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV1 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV1_TEXT ), TRUE );
                break;

            case 2 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV2 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV2_TEXT ), TRUE );
                break;

            case 3 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV3 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV3_TEXT ), TRUE );
                break;
        }
    }
*/
    return DIENUM_CONTINUE;
}



//-----------------------------------------------------------------------------
// Returns true if the DirectInput device is also an XInput device.
// Call SetupForIsXInputDevice() before, and CleanupForIsXInputDevice() after
//-----------------------------------------------------------------------------
bool CDirectInput8Manager::IsXInputDevice( const GUID* pGuidProductFromDirectInput )
{
    // Check each xinput device to see if this device's vid/pid matches
    XINPUT_DEVICE_NODE* pNode = m_pXInputDeviceList;
    while( pNode )
    {
        if( pNode->dwVidPid == pGuidProductFromDirectInput->Data1 )
            return true;
        pNode = pNode->pNext;
    }

    return false;
}


//-----------------------------------------------------------------------------
// Name: UpdateInputState()
// Desc: Get the input device's state and display it.
//-----------------------------------------------------------------------------
HRESULT CDirectInput8Manager::UpdateInputState( /* hDlg */ )
{
    HRESULT     hr;
    TCHAR       strText[512] = {0}; // Device state text
 //   DIJOYSTATE2 js;           // DInput Joystick state 

    if( NULL == m_pJoystick ) 
        return S_OK;

    // Poll the device to read the current state
    hr = m_pJoystick->Poll(); 
    if( FAILED(hr) )  
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = m_pJoystick->Acquire();
        while( hr == DIERR_INPUTLOST ) 
            hr = m_pJoystick->Acquire();

        // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of 
        // switching, so just try again later 
        return S_OK; 
    }

    // Get the input's device state
    if( FAILED( hr = m_pJoystick->GetDeviceState( sizeof(DIJOYSTATE2), this ) ) )
        return hr; // The device should have been acquired during the Poll()

//	printf("%ld, %ld, %ld\n", js.lX, js.lY, js.lZ);
	/**
    // Display Joystick state to dialog

    // Axes
    StringCchPrintf( strText, 512, TEXT("%ld"), js.lX ); 
    SetWindowText( GetDlgItem( hDlg, IDC_X_AXIS ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.lY ); 
    SetWindowText( GetDlgItem( hDlg, IDC_Y_AXIS ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.lZ ); 
    SetWindowText( GetDlgItem( hDlg, IDC_Z_AXIS ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.lRx ); 
    SetWindowText( GetDlgItem( hDlg, IDC_X_ROT ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.lRy ); 
    SetWindowText( GetDlgItem( hDlg, IDC_Y_ROT ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.lRz ); 
    SetWindowText( GetDlgItem( hDlg, IDC_Z_ROT ), strText );

    // Slider controls
    StringCchPrintf( strText, 512, TEXT("%ld"), js.rglSlider[0] ); 
    SetWindowText( GetDlgItem( hDlg, IDC_SLIDER0 ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.rglSlider[1] ); 
    SetWindowText( GetDlgItem( hDlg, IDC_SLIDER1 ), strText );

    // Points of view
    StringCchPrintf( strText, 512, TEXT("%ld"), js.rgdwPOV[0] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV0 ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.rgdwPOV[1] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV1 ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.rgdwPOV[2] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV2 ), strText );
    StringCchPrintf( strText, 512, TEXT("%ld"), js.rgdwPOV[3] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV3 ), strText );
 
   
    // Fill up text with which buttons are pressed
    StringCchCopy( strText, 512, TEXT("") );
    for( int i = 0; i < 128; i++ )
    {
        if ( js.rgbButtons[i] & 0x80 )
        {
            TCHAR sz[128];
            StringCchPrintf( sz, 128, TEXT("%02d "), i );
            StringCchCat( strText, 512, sz );
        }
    }

    SetWindowText( GetDlgItem( hDlg, IDC_BUTTONS ), strText );
	**/
    return S_OK;
}
