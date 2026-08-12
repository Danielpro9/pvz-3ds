#ifndef __WIIONSCREENKEYBOARD_H__
#define __WIIONSCREENKEYBOARD_H__
#ifdef WII_PLATFORM

// Pointer-driven text entry for the Wii.
//
// The console has no OS text input and, until this existed, no way at all to
// type: the new-profile dialog refuses an empty name, so a player with only a
// Wii Remote could not get past it.
//
// It is a plain Widget rather than anything special-cased, and it types by
// synthesising the same events a real keyboard would -- WidgetManager::KeyChar
// for characters, KeyDown for backspace and enter -- so the focused EditWidget
// needs no knowledge of it and every existing text field works unchanged.
//
// Two details make that work:
//
//   * mWantsFocus stays false (the Widget default). WidgetManager only moves
//     focus to a clicked widget when WantsFocus() is true, so tapping keys
//     leaves focus on the edit field. Were it otherwise, the first key press
//     would take focus, the field would fire LostFocus -> StopTextInput, and
//     the keyboard would dismiss itself.
//   * It is added and removed by SexyAppBase::StartTextInput/StopTextInput,
//     which EditWidget already calls on focus change -- so it appears exactly
//     when a field is ready for input, on any screen, with no per-dialog code.

#include "widget/Widget.h"

namespace Sexy
{

class _Font;

class WiiOnScreenKeyboard : public Widget
{
public:
	WiiOnScreenKeyboard();
	virtual ~WiiOnScreenKeyboard();

	// Lays the keyboard out for a screen of the given logical size and places
	// it along the bottom edge.
	void			LayoutForScreen(int theScreenWidth, int theScreenHeight);

	virtual void	Draw(Graphics* g);
	virtual void	MouseDown(int x, int y, int theClickCount);
	virtual void	MouseUp(int x, int y, int theClickCount);
	virtual void	MouseMove(int x, int y);
	virtual void	MouseLeave();

private:
	struct Key
	{
		int		mX, mY, mWidth, mHeight;
		char	mLower;			// 0 for the command keys below
		char	mUpper;
		int		mCommand;		// KeyCommand
		const char* mLabel;		// used when mLower is 0
	};

	enum KeyCommand
	{
		KEYCMD_NONE = 0,
		KEYCMD_SHIFT,
		KEYCMD_BACKSPACE,
		KEYCMD_SPACE,
		KEYCMD_DONE
	};

	static const int MAX_KEYS = 64;

	Key		mKeys[MAX_KEYS];
	int		mNumKeys;
	int		mPressedKey;		// index armed by MouseDown, -1 when none
	int		mHoverKey;
	bool	mShift;

	int		HitTest(int x, int y) const;
	void	EmitKey(const Key& theKey);
	_Font*	GetLabelFont() const;
};

}

#endif // WII_PLATFORM
#endif // __WIIONSCREENKEYBOARD_H__
