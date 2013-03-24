// -*- C++ -*-
/*!
 * @file  DirectInput.cpp
 * @brief DirectInput Component
 * @date $Date$
 *
 * $Id$
 */

#include "DirectInput.h"

// Module specification
// <rtc-template block="module_spec">
static const char* directinput_spec[] =
  {
    "implementation_id", "DirectInput",
    "type_name",         "DirectInput",
    "description",       "DirectInput Component",
    "version",           "1.1",
    "vendor",            "ysuga_net",
    "category",          "Experimental",
    "activity_type",     "PERIODIC",
    "kind",              "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    // Configuration variables
    "conf.default.debug", "0",
    // Widget
    "conf.__widget__.debug", "text",
    // Constraints
    ""
  };
// </rtc-template>

/*!
 * @brief constructor
 * @param manager Maneger Object
 */
DirectInput::DirectInput(RTC::Manager* manager)
    // <rtc-template block="initializer">
  : RTC::DataFlowComponentBase(manager),
    m_lPOut("lP", m_lP),
    m_lROut("lR", m_lR),
    m_rglSlider0Out("rglSlider0", m_rglSlider0),
    m_rglSlider1Out("rglSlider1", m_rglSlider1),
    m_POV0Out("POV0", m_POV0),
    m_POV1Out("POV1", m_POV1),
    m_rgbButtonsOut("rgbButtons", m_rgbButtons)

    // </rtc-template>
{
}

/*!
 * @brief destructor
 */
DirectInput::~DirectInput()
{
}



RTC::ReturnCode_t DirectInput::onInitialize()
{
  // Registration: InPort/OutPort/Service
  // <rtc-template block="registration">
  // Set InPort buffers
  
  // Set OutPort buffer
  addOutPort("lP", m_lPOut);
  addOutPort("lR", m_lROut);
  addOutPort("rglSlider0", m_rglSlider0Out);
  addOutPort("rglSlider1", m_rglSlider1Out);
  addOutPort("POV0", m_POV0Out);
  addOutPort("POV1", m_POV1Out);
  addOutPort("rgbButtons", m_rgbButtonsOut);
  
  // Set service provider to Ports
  
  // Set service consumers to Ports
  
  // Set CORBA Service Ports
  
  // </rtc-template>

  // <rtc-template block="bind_config">
  // Bind variables and configuration variable
  bindParameter("debug", m_debug, "0");
  // </rtc-template>

  m_lP.data.length(3);
  m_lR.data.length(3);

  m_POV0.data.length(2);
  m_POV1.data.length(2);

  m_rgbButtons.data.length(8);

  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t DirectInput::onFinalize()
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t DirectInput::onStartup(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t DirectInput::onShutdown(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


RTC::ReturnCode_t DirectInput::onActivated(RTC::UniqueId ec_id)
{
	HWND g_hWnd;
	HANDLE g_hInstance;
#define BUFSIZE 1024
	char OldWindowTitle[BUFSIZE];
	char NewWindowTitle[BUFSIZE];

	::GetConsoleTitle(OldWindowTitle, BUFSIZE);
	wsprintf(NewWindowTitle, "ControllerComponent(%d/%d)",
		GetTickCount(), GetCurrentProcessId());
	::SetConsoleTitle(NewWindowTitle);

	Sleep(40);

	g_hWnd = FindWindow(NULL, NewWindowTitle);
	::SetConsoleTitle(OldWindowTitle);
	
	g_hInstance = ::GetModuleHandle(NULL); //GetWindowLong(g_hWnd, GWL_HINSTANCE);

	m_pDirectInputManager = new CDirectInput8Manager(g_hWnd);
	
	Sleep(1000);
	return RTC::RTC_OK;
}


RTC::ReturnCode_t DirectInput::onDeactivated(RTC::UniqueId ec_id)
{
	delete m_pDirectInputManager;

  return RTC::RTC_OK;
}


RTC::ReturnCode_t DirectInput::onExecute(RTC::UniqueId ec_id)
{
	m_pDirectInputManager->UpdateInputState(/*g_hWnd*/);

	
	m_lP.data[0] = m_pDirectInputManager->lX;
	m_lP.data[1] = m_pDirectInputManager->lY;
	m_lP.data[2] = m_pDirectInputManager->lZ;
	// For delay of initialization.
	if(m_lP.data[0] > 1000 || m_lP.data[0] < -1000) {
		return RTC::RTC_OK;
	}
	m_lPOut.write();

	m_lR.data[0] = m_pDirectInputManager->lRx;
	m_lR.data[1] = m_pDirectInputManager->lRy;
	m_lR.data[2] = m_pDirectInputManager->lRz;
	m_lROut.write();

	m_rglSlider0.data = m_pDirectInputManager->rglSlider[0];
	m_rglSlider0Out.write();
	m_rglSlider1.data = m_pDirectInputManager->rglSlider[1];
	m_rglSlider1Out.write();


	LONG POV0 = m_pDirectInputManager->rgdwPOV[0];
	switch(POV0) {
		case 0:
			m_POV0.data[0] = 0; m_POV0.data[1] = +1;break;
		case 4500:
			m_POV0.data[0] = +1; m_POV0.data[1] = +1;break;
		case 9000:
			m_POV0.data[0] = +1; m_POV0.data[1] = 0;break;
		case 13500:
			m_POV0.data[0] = +1; m_POV0.data[1] = -1;break;
		case 18000:
			m_POV0.data[0] = 0; m_POV0.data[1] = -1;break;
		case 22500:
			m_POV0.data[0] = -1; m_POV0.data[1] = -1;break;
		case 27000:
			m_POV0.data[0] = -1; m_POV0.data[1] = 0;break;
		case 31500:
			m_POV0.data[0] = -1; m_POV0.data[1] = +1;break;
		case -1:
		default:
			m_POV0.data[0] = 0; m_POV0.data[1] = 0;break;
	}
	m_POV0Out.write();

	
	LONG POV1 = m_pDirectInputManager->rgdwPOV[1];
	switch(POV1) {
		case 0:
			m_POV1.data[0] = 0; m_POV1.data[1] = +1;break;
		case 4500:
			m_POV1.data[0] = +1; m_POV1.data[1] = +1;break;
		case 9000:
			m_POV1.data[0] = +1; m_POV1.data[1] = 0;break;
		case 13500:
			m_POV1.data[0] = +1; m_POV1.data[1] = -1;break;
		case 18000:
			m_POV1.data[0] = 0; m_POV1.data[1] = -1;break;
		case 22500:
			m_POV1.data[0] = -1; m_POV1.data[1] = -1;break;
		case 27000:
			m_POV1.data[0] = -1; m_POV1.data[1] = 0;break;
		case 31500:
			m_POV1.data[0] = -1; m_POV1.data[1] = +1;break;
		case -1:
		default:
			m_POV1.data[0] = 0; m_POV1.data[1] = 0;break;
	}
	m_POV1Out.write(); 

	
	for(int i = 0;i < 8;i++) {
		m_rgbButtons.data[i] = m_pDirectInputManager->rgbButtons[i];
	}
	m_rgbButtonsOut.write();

	if(m_debug) {
		static int i;
		system("cls");
		printf("Counter = %10d\n", i++);
		printf("lX          lY          lZ          lRx         lRy         lRz\n"); 
		printf("%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d\n" ,
			m_pDirectInputManager->lX,
			m_pDirectInputManager->lY,
			m_pDirectInputManager->lZ,
			m_pDirectInputManager->lRx,
			m_pDirectInputManager->lRy,
			m_pDirectInputManager->lRz
			);

		printf("rglSlider0  rglSlider1  hPov0       vPov0       hPov1       vPov1\n");
		printf("%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d\n", 
			m_pDirectInputManager->rglSlider[0],
			m_pDirectInputManager->rglSlider[1],
			m_POV0.data[0], m_POV0.data[1], m_POV1.data[0], m_POV1.data[1]);

		printf("rgbButtons0 rgbButtons1 rgbButtons2 rgbButtons3\n");
		/*
		printf("%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d\n", 
			rgbButtons0.data, 
			rgbButtons1.data, 
			rgbButtons2.data, 
			rgbButtons3.data);
		printf("rgbButtons4 rgbButtons5 rgbButtons6 rgbButtons7\n");
		printf("%-+10d  "  "%-+10d  "  "%-+10d  "  "%-+10d\n", 
			rgbButtons4.data, 
			rgbButtons5.data, 
			rgbButtons6.data, 
			rgbButtons7.data);
			*/
		for(int i = 0;i < 8;i++) {
			printf("%-+10d ", m_rgbButtons.data[i]);
		}
		printf("\n");
	}

  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t DirectInput::onAborting(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t DirectInput::onError(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


RTC::ReturnCode_t DirectInput::onReset(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t DirectInput::onStateUpdate(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t DirectInput::onRateChanged(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/



extern "C"
{
 
  void DirectInputInit(RTC::Manager* manager)
  {
    coil::Properties profile(directinput_spec);
    manager->registerFactory(profile,
                             RTC::Create<DirectInput>,
                             RTC::Delete<DirectInput>);
  }
  
};


