#ifdef WII_PLATFORM

#include "WiiOnScreenKeyboard.h"

#include "SexyAppBase.h"
#include "graphics/Graphics.h"
#include "graphics/Font.h"
#include "misc/KeyCodes.h"
#include "widget/WidgetManager.h"
#include "widget/EditWidget.h"

#include <string.h>

using namespace Sexy;

namespace
{
// Rows of printable keys. Deliberately QWERTY rather than alphabetical: the
// point-and-click cost of finding a letter is the same either way, and QWERTY
// is what players already know the shape of.
const char* const kRows[] = {
	"1234567890",
	"qwertyuiop",
	"asdfghjkl",
	"zxcvbnm",
};
const int kNumRows = (int)(sizeof(kRows) / sizeof(kRows[0]));

char ToUpperAscii(char c)
{
	return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}
}

WiiOnScreenKeyboard::WiiOnScreenKeyboard()
	: mNumKeys(0), mPressedKey(-1), mHoverKey(-1), mShift(true)
{
	// Names read better capitalised, and the field is empty when the keyboard
	// opens, so start shifted like a phone keyboard does.
	memset(mKeys, 0, sizeof(mKeys));
	mWantsFocus = false;	// see the header: taking focus would dismiss us
	mMouseVisible = true;
}

WiiOnScreenKeyboard::~WiiOnScreenKeyboard()
{
}

void WiiOnScreenKeyboard::LayoutForScreen(int theScreenWidth, int theScreenHeight)
{
	const int aKeyW = 46;
	const int aKeyH = 38;
	const int aGap = 4;
	const int aPad = 10;

	const int aRowsHeight = (kNumRows + 1) * (aKeyH + aGap) - aGap;	// +1 command row
	const int aBoardH = aRowsHeight + aPad * 2;
	const int aBoardW = 10 * (aKeyW + aGap) - aGap + aPad * 2;
	const int aBoardX = (theScreenWidth - aBoardW) / 2;
	const int aBoardY = theScreenHeight - aBoardH - 12;

	Resize(aBoardX, aBoardY, aBoardW, aBoardH);

	mNumKeys = 0;
	int aY = aPad;
	for (int aRow = 0; aRow < kNumRows; aRow++)
	{
		const char* aChars = kRows[aRow];
		const int aLen = (int)strlen(aChars);
		// Centre short rows, so the layout keeps the familiar QWERTY stagger.
		int aX = aPad + ((10 - aLen) * (aKeyW + aGap)) / 2;
		for (int i = 0; i < aLen && mNumKeys < MAX_KEYS; i++)
		{
			Key& aKey = mKeys[mNumKeys++];
			aKey.mX = aX;
			aKey.mY = aY;
			aKey.mWidth = aKeyW;
			aKey.mHeight = aKeyH;
			aKey.mLower = aChars[i];
			aKey.mUpper = ToUpperAscii(aChars[i]);
			aKey.mCommand = KEYCMD_NONE;
			aKey.mLabel = NULL;
			aX += aKeyW + aGap;
		}
		aY += aKeyH + aGap;
	}

	// Command row: Shift | Space (wide) | Back | Done
	struct { int mCmd; const char* mLabel; int mUnits; } aCommands[] = {
		{ KEYCMD_SHIFT,     "Shift", 2 },
		{ KEYCMD_SPACE,     "Space", 4 },
		{ KEYCMD_BACKSPACE, "Back",  2 },
		{ KEYCMD_DONE,      "Done",  2 },
	};
	int aX = aPad;
	for (int i = 0; i < 4 && mNumKeys < MAX_KEYS; i++)
	{
		const int aWidth = aCommands[i].mUnits * aKeyW + (aCommands[i].mUnits - 1) * aGap;
		Key& aKey = mKeys[mNumKeys++];
		aKey.mX = aX;
		aKey.mY = aY;
		aKey.mWidth = aWidth;
		aKey.mHeight = aKeyH;
		aKey.mLower = 0;
		aKey.mUpper = 0;
		aKey.mCommand = aCommands[i].mCmd;
		aKey.mLabel = aCommands[i].mLabel;
		aX += aWidth + aGap;
	}

	mPressedKey = -1;
	mHoverKey = -1;
	MarkDirty();
}

// Borrow whatever font the field being typed into uses, so the keys match the
// dialog they serve. The framework has no default font of its own, and this
// avoids reaching into any particular game's resource set from platform code.
_Font* WiiOnScreenKeyboard::GetLabelFont() const
{
	if (mWidgetManager == NULL)
		return NULL;
	EditWidget* anEdit = dynamic_cast<EditWidget*>(mWidgetManager->mFocusWidget);
	return anEdit != NULL ? anEdit->mFont : NULL;
}

int WiiOnScreenKeyboard::HitTest(int x, int y) const
{
	for (int i = 0; i < mNumKeys; i++)
	{
		const Key& aKey = mKeys[i];
		if (x >= aKey.mX && x < aKey.mX + aKey.mWidth &&
			y >= aKey.mY && y < aKey.mY + aKey.mHeight)
			return i;
	}
	return -1;
}

void WiiOnScreenKeyboard::Draw(Graphics* g)
{
	_Font* aFont = GetLabelFont();

	// Panel. Opaque rather than translucent: it sits over the dialog it is
	// typing into, and a see-through panel makes small glyphs unreadable
	// against PvZ's busy backgrounds.
	g->SetColor(Color(24, 20, 40));
	g->FillRect(0, 0, mWidth, mHeight);
	g->SetColor(Color(120, 110, 160));
	g->FillRect(0, 0, mWidth, 2);
	g->FillRect(0, mHeight - 2, mWidth, 2);

	if (aFont != NULL)
		g->SetFont(aFont);

	for (int i = 0; i < mNumKeys; i++)
	{
		const Key& aKey = mKeys[i];

		const bool aDown = (i == mPressedKey);
		const bool aHot = (i == mHoverKey);
		const bool aActive = (aKey.mCommand == KEYCMD_SHIFT && mShift);

		if (aDown || aActive)
			g->SetColor(Color(210, 190, 90));
		else if (aHot)
			g->SetColor(Color(90, 84, 130));
		else
			g->SetColor(Color(58, 54, 86));
		g->FillRect(aKey.mX, aKey.mY, aKey.mWidth, aKey.mHeight);

		if (aFont == NULL)
			continue;

		SexyString aLabel;
		if (aKey.mLower != 0)
			aLabel.append(1, mShift ? aKey.mUpper : aKey.mLower);
		else
			aLabel = aKey.mLabel;

		const int aTextW = aFont->StringWidth(aLabel);
		const int aTextX = aKey.mX + (aKey.mWidth - aTextW) / 2;
		const int aTextY = aKey.mY + (aKey.mHeight + aFont->GetAscent()) / 2 - 2;
		g->SetColor((aDown || aActive) ? Color(20, 16, 30) : Color(240, 240, 240));
		g->DrawString(aLabel, aTextX, aTextY);
	}
}

void WiiOnScreenKeyboard::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	mPressedKey = HitTest(x, y);
	MarkDirty();
}

void WiiOnScreenKeyboard::MouseUp(int x, int y, int theClickCount)
{
	Widget::MouseUp(x, y, theClickCount);

	// Only fire when release lands on the same key that was pressed, so a
	// wobbling Wii Remote can slide off a key to cancel it.
	const int aKey = HitTest(x, y);
	if (aKey >= 0 && aKey == mPressedKey)
		EmitKey(mKeys[aKey]);

	mPressedKey = -1;
	MarkDirty();
}

void WiiOnScreenKeyboard::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	const int aKey = HitTest(x, y);
	if (aKey != mHoverKey)
	{
		mHoverKey = aKey;
		MarkDirty();
	}
}

void WiiOnScreenKeyboard::MouseLeave()
{
	Widget::MouseLeave();
	if (mHoverKey != -1 || mPressedKey != -1)
	{
		mHoverKey = -1;
		mPressedKey = -1;
		MarkDirty();
	}
}

void WiiOnScreenKeyboard::EmitKey(const Key& theKey)
{
	if (mWidgetManager == NULL)
		return;

	switch (theKey.mCommand)
	{
	case KEYCMD_SHIFT:
		mShift = !mShift;
		MarkDirty();
		return;

	case KEYCMD_BACKSPACE:
		mWidgetManager->KeyDown(KEYCODE_BACK);
		return;

	case KEYCMD_SPACE:
		mWidgetManager->KeyChar(__S(' '));
		return;

	case KEYCMD_DONE:
		// Enter is what the dialog's default button listens for, so Done
		// confirms exactly as pressing OK would.
		mWidgetManager->KeyDown(KEYCODE_RETURN);
		return;

	default:
		break;
	}

	if (theKey.mLower != 0)
	{
		mWidgetManager->KeyChar((SexyChar)(mShift ? theKey.mUpper : theKey.mLower));
		// One capital at the start of a name is the common case; drop shift
		// after it fires so the rest types lowercase without a second tap.
		if (mShift)
		{
			mShift = false;
			MarkDirty();
		}
	}
}

#endif // WII_PLATFORM
