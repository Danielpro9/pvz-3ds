#include <3ds.h>
#include <cstring>
#include <unordered_map>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"
#include "misc/SexyLog.h"

using namespace Sexy;

/*
static PadState pad;
static std::unordered_map<u64, KeyCode> keyMaps = {
	{HidNpadButton_Plus,  KEYCODE_ESCAPE},
	{HidNpadButton_Minus, KEYCODE_SPACE},
	{HidNpadButton_A,     KEYCODE_RETURN},
	{HidNpadButton_L,     KEYCODE_LBUTTON},
	{HidNpadButton_R,     KEYCODE_RBUTTON},
};
*/

// Last touched point in game coordinates. The panel reports nothing once the
// stylus lifts, so the release is sent where the stylus was last seen.
static int sTouchX = 0;
static int sTouchY = 0;

void SexyAppBase::InitInput()
{
	// libctru's default __appInit already started HID. The touchscreen is the
	// game's pointer, so the mouse always counts as inside the window.
	if (!mMouseIn)
		mMouseIn = true;
}

bool SexyAppBase::StartTextInput(std::string& theInput)
{
	// The system keyboard applet stands in for the PC keyboard. 12 characters
	// matches the profile name field (NewUserDialog), which refuses an empty
	// name and would otherwise block a fresh install.
	char aBuffer[64] = {};
	SwkbdState aKeyboard;
	swkbdInit(&aKeyboard, SWKBD_TYPE_NORMAL, 2, 12);

	SexyLogPrintf("[KBD] open\n");
	mGLInterface->SuspendFrame();
	SwkbdButton aButton = swkbdInputText(&aKeyboard, aBuffer, sizeof(aBuffer));
	SexyLogPrintf("[KBD] closed: button %d, %d bytes\n", (int)aButton, (int)strlen(aBuffer));
	mGLInterface->ResumeFrame();

	if (aButton != SWKBD_BUTTON_CONFIRM)
		return false;

	theInput = aBuffer;
	return true;
}

void SexyAppBase::StopTextInput()
{
	
}

bool SexyAppBase::ProcessDeferredMessages(bool singleMessage)
{
	// A HOME or power button press makes aptMainLoop jump to the HOME menu,
	// which takes the GPU over like swkbd does (see StartTextInput).
	bool aJumpToHome = mGLInterface != NULL && aptShouldJumpToHome();
	if (aJumpToHome)
		mGLInterface->SuspendFrame();

	if (!aptMainLoop())
	{
		mShutdown = true;
		return false;
	}

	if (aJumpToHome)
		mGLInterface->ResumeFrame();

	hidScanInput();
	u32 aKeysDown = hidKeysDown();
	u32 aKeysHeld = hidKeysHeld();
	u32 aKeysUp = hidKeysUp();

	// Touchscreen as the left mouse button: contact presses, dragging moves,
	// lifting releases. Panel pixels (320x240) are mapped to game coordinates
	// by RemapMouse, using the rect set up in MakeWindow.
	if (aKeysHeld & KEY_TOUCH)
	{
		touchPosition aTouch;
		hidTouchRead(&aTouch);
		sTouchX = aTouch.px;
		sTouchY = aTouch.py;
		mWidgetManager->RemapMouse(sTouchX, sTouchY);

		mLastUserInputTick = mLastTimerTime;
		mWidgetManager->MouseMove(sTouchX, sTouchY);
		if (aKeysDown & KEY_TOUCH)
			mWidgetManager->MouseDown(sTouchX, sTouchY, 1);
	}
	else if (aKeysUp & KEY_TOUCH)
	{
		mLastUserInputTick = mLastTimerTime;
		mWidgetManager->MouseUp(sTouchX, sTouchY, 1);
	}

	return false;
}
