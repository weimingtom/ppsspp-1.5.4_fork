﻿// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#include "i18n/i18n.h"
#include "math/math_util.h"
#include <algorithm>

#include "Core/Dialog/PSPOskDialog.h"
#include "Core/Util/PPGeDraw.h"
#include "Core/HLE/sceCtrl.h"
#include "Core/HLE/sceDisplay.h"
#include "Core/HLE/sceUtility.h"
#include "Core/Config.h"
#include "Core/Reporting.h"
#include "Common/ChunkFile.h"
#include "GPU/GPUState.h"

#if defined(USING_WIN_UI)
#include "base/NativeApp.h"
#endif

#ifndef _WIN32
#include <ctype.h>
#include <math.h>
#endif

// These are rough, it seems to take a long time to init, and probably depends on threads.
// TODO: This takes like 700ms on a PSP but that's annoyingly long.
const static int OSK_INIT_DELAY_US = 300000;
const static int OSK_SHUTDOWN_DELAY_US = 40000;

static std::map<std::string, std::pair<std::string, int>> languageMapping;

const int numKeyCols[OSK_KEYBOARD_COUNT] = {12, 12, 13, 13, 12, 12, 12, 12, 12};
const int numKeyRows[OSK_KEYBOARD_COUNT] = {4, 4, 5, 5, 5, 4, 4, 4, 4};

// Japanese (Kana) diacritics
static const wchar_t diacritics[2][103] =
{
	{L"かがきぎくぐけげこごさざしじすずせぜそぞただちぢつづてでとどはばぱばひびぴびふぶぷぶへべぺべほぼぽぼウヴカガキギクグケゲコゴサザシジスズセゼソゾタダチヂツヅテデトドハバパバヒビピビフブプブヘベペベホボポボ"},
	{L"はぱばぱひぴびぴふぷぶぷへぺべぺほぽぼぽハパバパヒピビピフプブプヘペベペホポボポ"}
};

// Korean (Hangul) consonant
static const wchar_t kor_cons[] = L"ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ";

// Korean (Hangul) vowels, Some vowels are not used, they will be spaces
static const wchar_t kor_vowel[] = L"ㅏㅐㅑㅒㅓㅔㅕㅖㅗ   ㅛㅜ   ㅠㅡ ㅣ";

// Korean (Hangul) vowel Combination key
const int kor_vowelCom[] = {0,8,9,1,8,10,20,8,11,4,13,14,5,13,15,20,13,16,20,18,19};

// Korean (Hangul) last consonant(diacritics)
static const wchar_t kor_lcons[] = L"ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ";

// Korean (Hangul) last consonant Combination key
const int kor_lconsCom[] = {18,0,2,21,3,4,26,3,5,0,7,8,15,7,9,16,7,10,18,7,11,24,7,12,25,7,13,26,7,14,18,16,17};

// Korean (Hangul) last consonant Separation key
const int kor_lconsSpr[] = {2,1,9,4,4,12,5,4,18,8,8,0,9,8,6,10,8,7,11,8,9,12,8,16,13,8,17,14,8,18,17,17,9};

static const wchar_t oskKeys[OSK_KEYBOARD_COUNT][5][14] =
{
	{
		// Latin Lowercase
		{L"1234567890-+"},
		{L"qwertyuiop[]"},
		{L"asdfghjkl;@~"},
		{L"zxcvbnm,./?\\"},
	},
	{
		// Latin Uppercase
		{L"!@#$%^&*()_+"},
		{L"QWERTYUIOP{}"},
		{L"ASDFGHJKL:\"`"},
		{L"ZXCVBNM<>/?|"},
	},
	{
		// Hiragana
		{L"あかさたなはまやらわぁゃっ"},
		{L"いきしちにひみ　り　ぃ　　"},
		{L"うくすつぬふむゆるをぅゅ゛"},
		{L"えけせてねへめ　れ　ぇ　゜"},
		{L"おこそとのほもよろんぉょー"},
	},
	{
		// Katakana
		{L"アカサタナハマヤラワァャッ"},
		{L"イキシチニヒミ　リ　ィ　　"},
		{L"ウクスツヌフムユルヲゥュ゛"},
		{L"エケセテネヘメ　レ　ェ　゜"},
		{L"オコソトノホモヨロンォョー"},
	},
	{
		// Korean(Hangul)
		{L"1234567890-+"},
		{L"ㅃㅉㄸㄲㅆ!@#$%^&"},
		{L"ㅂㅈㄷㄱㅅㅛㅕㅑㅐㅔ[]"},
		{L"ㅁㄴㅇㄹㅎㅗㅓㅏㅣ;@~"},
		{L"ㅋㅌㅊㅍㅠㅜㅡ<>/?|"},
	},
	{
		// Russian Lowercase
		{L"1234567890-+"},
		{L"йцукенгшщзхъ"},
		{L"фывапролджэё"},
		{L"ячсмитьбю/?|"},
	},
	{
		// Russian Uppercase
		{L"!@#$%^&*()_+"},
		{L"ЙЦУКЕНГШЩЗХЪ"},
		{L"ФЫВАПРОЛДЖЭЁ"},
		{L"ЯЧСМИТЬБЮ/?|"},
	},
	{
		// Latin Full-width Lowercase
		{ L"１２３４５６７８９０－＋" },
		{ L"ｑｗｅｒｔｙｕｉｏｐ［］" },
		{ L"ａｓｄｆｇｈｊｋｌ；＠～" },
		{ L"ｚｘｃｖｂｎｍ，．／？￥￥" },
	},
	{
		// Latin Full-width Uppercase
		{ L"！＠＃＄％＾＆＊（）＿＋" },
		{ L"ＱＷＥＲＴＹＵＩＯＰ｛｝" },
		{ L"ＡＳＤＦＧＨＪＫＬ：￥”‘" },
		{ L"ＺＸＣＶＢＮＭ＜＞／？｜" },
	},
};

// This isn't a complete representation of these flags, it just helps ensure we show the right keyboards.
int allowedInputFlagsMap[OSK_KEYBOARD_COUNT] = {
	PSP_UTILITY_OSK_INPUTTYPE_LATIN_LOWERCASE | PSP_UTILITY_OSK_INPUTTYPE_LATIN_SYMBOL | PSP_UTILITY_OSK_INPUTTYPE_LATIN_DIGIT,
	PSP_UTILITY_OSK_INPUTTYPE_LATIN_UPPERCASE | PSP_UTILITY_OSK_INPUTTYPE_LATIN_SYMBOL,
	PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_HIRAGANA,
	PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_KATAKANA | PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_HALF_KATAKANA,
	PSP_UTILITY_OSK_INPUTTYPE_KOREAN,
	PSP_UTILITY_OSK_INPUTTYPE_RUSSIAN_LOWERCASE,
	PSP_UTILITY_OSK_INPUTTYPE_RUSSIAN_UPPERCASE,
	PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_LOWERCASE | PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_SYMBOL | PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_DIGIT,
	PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_UPPERCASE | PSP_UTILITY_OSK_INPUTTYPE_JAPANESE_SYMBOL,
};

PSPOskDialog::PSPOskDialog() : PSPDialog() {
	// This can break all kinds of stuff, changing the decimal point in sprintf for example.
	// Not sure what the intended effect is so commented out for now.
	// setlocale(LC_ALL, "");
}

PSPOskDialog::~PSPOskDialog() {
}

void PSPOskDialog::ConvertUCS2ToUTF8(std::string& _string, const PSPPointer<u16_le>& em_address)
{
	if (!em_address.IsValid())
	{
		_string = "";
		return;
	}

	const size_t maxLength = 2047;
	char stringBuffer[maxLength + 1];
	char *string = stringBuffer;

	u16_le *input = &em_address[0];
	int c;
	while ((c = *input++) != 0 && string < stringBuffer + maxLength)
	{
		if (c < 0x80)
			*string++ = c;
		else if (c < 0x800) {
			*string++ = 0xC0 | (c >> 6);
			*string++ = 0x80 | (c & 0x3F);
		} else {
			*string++ = 0xE0 | (c >> 12);
			*string++ = 0x80 | ((c >> 6) & 0x3F);
			*string++ = 0x80 | (c & 0x3F);
		}
	}
	*string++ = '\0';
	_string = stringBuffer;
}

void GetWideStringFromPSPPointer(std::wstring& _string, const PSPPointer<u16_le>& em_address)
{
	if (!em_address.IsValid())
	{
		_string = L"";
		return;
	}

	const size_t maxLength = 2047;
	wchar_t stringBuffer[maxLength + 1];
	wchar_t *string = stringBuffer;

	u16_le *input = &em_address[0];
	int c;
	while ((c = *input++) != 0 && string < stringBuffer + maxLength)
		*string++ = c;
	*string++ = '\0';
	_string = stringBuffer;
}

void PSPOskDialog::ConvertUCS2ToUTF8(std::string& _string, const wchar_t *input)
{
	char stringBuffer[2048];
	char *string = stringBuffer;

	int c;
	while ((c = *input++) != 0)
	{
		if (c < 0x80)
			*string++ = c;
		else if (c < 0x800) {
			*string++ = 0xC0 | (c >> 6);
			*string++ = 0x80 | (c & 0x3F);
		} else {
			*string++ = 0xE0 | (c >> 12);
			*string++ = 0x80 | ((c >> 6) & 0x3F);
			*string++ = 0x80 | (c & 0x3F);
		}
	}
	*string++ = '\0';
	_string = stringBuffer;
}

static void FindValidKeyboard(s32 inputType, int direction, OskKeyboardLanguage &lang, OskKeyboardDisplay &disp) {
	OskKeyboardLanguage origLang = lang;
	OskKeyboardDisplay origDisp = disp;

	if (inputType == 0) {
		return;
	}

	// TODO: Limit by allowed keyboards properly... this is just an approximation.
	int tries = OSK_LANGUAGE_COUNT * 2;
	while (!(inputType & allowedInputFlagsMap[disp]) && tries > 0) {
		if ((--tries % 1) == 0) {
			lang = (OskKeyboardLanguage)((OSK_LANGUAGE_COUNT + lang + direction) % OSK_LANGUAGE_COUNT);
			disp = OskKeyboardCases[lang][LOWERCASE];
		} else {
			disp = OskKeyboardCases[lang][UPPERCASE];
		}
	}

	if (tries == 0) {
		// In case of error, let's just fall back to allowing all.
		lang = origLang;
		disp = origDisp;
	}
}

static bool IsKeyboardShiftValid(s32 inputType, OskKeyboardLanguage lang, OskKeyboardDisplay disp) {
	// Swap disp and check if it's valid.
	if (disp == OskKeyboardCases[lang][UPPERCASE])
		disp = OskKeyboardCases[lang][LOWERCASE];
	else
		disp = OskKeyboardCases[lang][UPPERCASE];

	return inputType == 0 || (inputType & allowedInputFlagsMap[disp]) != 0;
}

int PSPOskDialog::Init(u32 oskPtr) {
	// Ignore if already running
	if (GetStatus() != SCE_UTILITY_STATUS_NONE) {
		ERROR_LOG_REPORT(SCEUTILITY, "sceUtilityOskInitStart: invalid status");
		return SCE_ERROR_UTILITY_INVALID_STATUS;
	}
	// Seems like this should crash?
	if (!Memory::IsValidAddress(oskPtr)) {
		ERROR_LOG_REPORT(SCEUTILITY, "sceUtilityOskInitStart: invalid params (%08x)", oskPtr);
		return -1;
	}

	oskParams = oskPtr;
	if (oskParams->base.size != sizeof(SceUtilityOskParams))
	{
		ERROR_LOG_REPORT(SCEUTILITY, "sceUtilityOskInitStart: invalid size %d", oskParams->base.size);
		return SCE_ERROR_UTILITY_INVALID_PARAM_SIZE;
	}
	// Also seems to crash.
	if (!oskParams->fields.IsValid())
	{
		ERROR_LOG_REPORT(SCEUTILITY, "sceUtilityOskInitStart: invalid field data (%08x)", oskParams->fields.ptr);
		return -1;
	}

	if (oskParams->unk_60 != 0)
		WARN_LOG_REPORT(SCEUTILITY, "sceUtilityOskInitStart: unknown param is non-zero (%08x)", oskParams->unk_60);
	if (oskParams->fieldCount != 1)
		WARN_LOG_REPORT(SCEUTILITY, "sceUtilityOskInitStart: unsupported field count %d", oskParams->fieldCount);

	ChangeStatusInit(OSK_INIT_DELAY_US);
	selectedChar = 0;
	currentKeyboardLanguage = OSK_LANGUAGE_ENGLISH;
	currentKeyboard = OSK_KEYBOARD_LATIN_LOWERCASE;
	FindValidKeyboard(oskParams->fields[0].inputtype, 1, currentKeyboardLanguage, currentKeyboard);

	ConvertUCS2ToUTF8(oskDesc, oskParams->fields[0].desc);
	ConvertUCS2ToUTF8(oskIntext, oskParams->fields[0].intext);
	ConvertUCS2ToUTF8(oskOuttext, oskParams->fields[0].outtext);

	i_level = 0;

	inputChars = L"";

	if (oskParams->fields[0].intext.IsValid()) {
		auto src = oskParams->fields[0].intext;
		int c;
		while ((c = *src++) != 0)
			inputChars += c;
	}

	languageMapping = GetLangValuesMapping();

	// Eat any keys pressed before the dialog inited.
	UpdateButtons();

	StartFade(true);
	return 0;
}

std::wstring PSPOskDialog::CombinationKorean(bool isInput)
{
	std::wstring string;

	isCombinated = true;

	int selectedRow = selectedChar / numKeyCols[currentKeyboard];
	int selectedCol = selectedChar % numKeyCols[currentKeyboard];

	if(inputChars.size() == 0) {
		wchar_t sw = oskKeys[currentKeyboard][selectedRow][selectedCol];

		if (inputChars.size() < FieldMaxLength()) {
			string += sw;

			i_value[0] = GetIndex(kor_cons, sw);

			if(i_value[0] != -1 && isInput == true)
				i_level = 1;
		} else {
			isCombinated = false;
		}
	} else {
		for(u32 i = 0; i < inputChars.size(); i++) {
			if(i + 1 == inputChars.size()) {
				wchar_t sw = oskKeys[currentKeyboard][selectedRow][selectedCol];

				if(i_level == 0) {
					string += inputChars[i];
					if (inputChars.size() < FieldMaxLength()) {
						string += sw;

						i_value[0] = GetIndex(kor_cons, sw);

						if(i_value[0] != -1 && isInput == true)
							i_level = 1;
					} else {
						isCombinated = false;
					}
				} else if(i_level == 1) {
					i_value[1] = GetIndex(kor_vowel, sw);

					if(i_value[1] == -1) {
						string += inputChars[i];
						if (inputChars.size() < FieldMaxLength()) {
							string += sw;

							if(isInput == true) {
								i_value[0] = GetIndex(kor_cons, sw);

								if(i_value[0] != -1)
									i_level = 1;
								else
									i_level = 0;
							}
						} else {
							isCombinated = false;
						}
					} else {
						u16 code = 0xAC00 + i_value[0] * 0x24C + i_value[1] * 0x1C;
						string += code;

						if(isInput == true) {
							i_level = 2;
						}
					}
				} else if(i_level == 2) {
					int tmp = GetIndex(kor_vowel, sw);
					if(tmp != -1) {
						int tmp2 = -1;
						for(size_t j = 0; j < sizeof(kor_vowelCom) / 4; j+=3) {
							if(kor_vowelCom[j] == tmp && kor_vowelCom[j + 1] == i_value[1]) {
								tmp2 = kor_vowelCom[j + 2];
								break;
							}
						}
						if(tmp2 != -1) {
							if(isInput == true) {
								i_value[1] = tmp2;
							}

							u16 code = 0xAC00 + i_value[0] * 0x24C + tmp2 * 0x1C;

							string += code;
						} else {
							string += inputChars[i];
							if (inputChars.size() < FieldMaxLength()) {
								string += sw;

								if(isInput == true) {
									i_level = 0;
								}
							} else {
								isCombinated = false;
							}
						}
					} else {
						int tmp2 = GetIndex(kor_lcons, sw);

						if (tmp2 == -1) {
							string += inputChars[i];
							if (inputChars.size() < FieldMaxLength()) {
								string += sw;

								if (isInput == true) {
									i_value[0] = GetIndex(kor_cons, sw);

									if(i_value[0] != -1)
										i_level = 1;
									else
										i_level = 0;
								}
							} else {
								isCombinated = false;
							}
						} else {
							u16 code = 0xAC00 + i_value[0] * 0x24C + i_value[1] * 0x1C + tmp2 + 1;

							string += code;

							if (isInput == true) {
								i_level = 3;
								i_value[2] = tmp2;
							}
						}
					}
				} else if(i_level == 3) {
					int tmp = GetIndex(kor_lcons, sw);
					if(tmp != -1) {
						int tmp2 = -1;
						for(size_t j = 0; j < sizeof(kor_lconsCom) / 4; j+=3) {
							if(kor_lconsCom[j] == tmp && kor_lconsCom[j + 1] == i_value[2]) {
								tmp2 = kor_lconsCom[j + 2];
								break;
							}
						}
						if(tmp2 != -1) {
							if(isInput == true) {
								i_value[2] = tmp2;
							}

							u16 code = 0xAC00 + i_value[0] * 0x24C + tmp2 * 0x1C + i_value[2] + 1;

							string += code;
						} else {
							string += inputChars[i];
							if (inputChars.size() < FieldMaxLength()) {
								string += sw;

								if(isInput == true) {
									i_value[0] = GetIndex(kor_cons, sw);

									if(i_value[0] != -1)
										i_level = 1;
									else
										i_level = 0;
								}
							} else {
								isCombinated = false;
							}
						}
					} else {
						int tmp3 = GetIndex(kor_vowel, sw);
						if (tmp3 == -1) {
							string += inputChars[i];
							if (inputChars.size() < FieldMaxLength()) {
								string += sw;

								if(isInput == true) {
									i_value[0] = GetIndex(kor_cons, sw);

									if(i_value[0] != -1)
										i_level = 1;
									else
										i_level = 0;
								}
							} else {
								isCombinated = false;
							}
						} else {
							if (inputChars.size() < FieldMaxLength()) {
								int tmp2 = -1;
								for(size_t j = 0; j < sizeof(kor_lconsSpr) / 4; j+=3) {
									if(kor_lconsSpr[j] == i_value[2]) {
										tmp2 = (int)j;
										break;
									}
								}
								if(tmp2 != -1) {
									u16 code = 0xAC00 + i_value[0] * 0x24C + i_value[1] * 0x1C + kor_lconsSpr[tmp2 + 1];
									string += code;

									code = 0xAC00 + kor_lconsSpr[tmp2 + 2] * 0x24C + tmp3 * 0x1C;
									string += code;

									if(isInput == true) {
										i_value[0] = kor_lconsSpr[tmp2 + 2];
										i_value[1] = tmp3;
										i_level = 2;
									}
								} else {
									int tmp4 = GetIndex(kor_cons, kor_lcons[i_value[2]]);

									if (tmp4 != -1) {
										u16 code = 0xAC00 + i_value[0] * 0x24C + i_value[1] * 0x1C;

										string += code;

										code = 0xAC00 + tmp4 * 0x24C + tmp3 * 0x1C;

										string += code;

										if(isInput == true) {
											i_value[0] = tmp4;
											i_value[1] = tmp3;
											i_level = 2;
										}
									} else {
										string += inputChars[i];
										string += sw;

										if(isInput == true) {
											i_level = 0;
										}
									}
								}
							} else {
								string += inputChars[i];
								isCombinated = false;
							}
						}
					}
				}
			} else {
				string += inputChars[i];
			}
		}
	}

	return string;
}

std::wstring PSPOskDialog::CombinationString(bool isInput)
{
	std::wstring string;

	isCombinated = false;

	int selectedRow = selectedChar / numKeyCols[currentKeyboard];
	int selectedCol = selectedChar % numKeyCols[currentKeyboard];

	if(currentKeyboard == OSK_KEYBOARD_KOREAN)
	{
		string = CombinationKorean(isInput);
	}
	else
	{
		if(isInput == true)
		{
			i_level = 0;
		}

		if(oskKeys[currentKeyboard][selectedRow][selectedCol] == L'゛')
		{
			for(u32 i = 0; i < inputChars.size(); i++)
			{
				if(i + 1 == inputChars.size())
				{
					for(u32 j = 0; j < wcslen(diacritics[0]); j+=2)
					{
						if(inputChars[i] == diacritics[0][j])
						{
							string += diacritics[0][j + 1];
							isCombinated = true;
							break;
						}
					}

					if(isCombinated == false)
					{
						string += inputChars[i];
					}
				}
				else
				{
					string += inputChars[i];
				}
			}
		}
		else if(oskKeys[currentKeyboard][selectedRow][selectedCol] == L'゜')
		{
			for(u32 i = 0; i < inputChars.size(); i++)
			{
				if(i + 1 == inputChars.size())
				{
					for(u32 j = 0; j < wcslen(diacritics[1]); j+=2)
					{
						if(inputChars[i] == diacritics[1][j])
						{
							string += diacritics[1][j + 1];
							isCombinated = true;
							break;
						}
					}

					if(isCombinated == false)
					{
						string += inputChars[i];
					}
				}
				else
				{
					string += inputChars[i];
				}
			}
		}
		else
		{
			for(u32 i = 0; i < inputChars.size(); i++)
			{
				string += inputChars[i];
			}

			if (string.size() <= FieldMaxLength())
			{
				string += oskKeys[currentKeyboard][selectedRow][selectedCol];
			}
			isCombinated = true;
		}
	}

	return string;
}

void PSPOskDialog::RemoveKorean()
{
	if(i_level == 1)
	{
		i_level = 0;
	}
	else if(i_level == 2)
	{
		int tmp = -1;
		for(size_t i = 2; i < sizeof(kor_vowelCom) / 4; i+=3)
		{
			if(kor_vowelCom[i] == i_value[1])
			{
				tmp = kor_vowelCom[i - 1];
				break;
			}
		}

		if(tmp != -1)
		{
			i_value[1] = tmp;
			u16 code = 0xAC00 + i_value[0] * 0x24C + i_value[1] * 0x1C;
			inputChars += code;
		}
		else
		{
			i_level = 1;
			inputChars += kor_cons[i_value[0]];
		}
	}
	else if(i_level == 3)
	{
		int tmp = -1;
		for(size_t i = 2; i < sizeof(kor_lconsCom) / 4; i+=3)
		{
			if(kor_lconsCom[i] == i_value[2])
			{
				tmp = kor_lconsCom[i - 1];
				break;
			}
		}

		if(tmp != -1)
		{
			i_value[2] = tmp;
			u16 code = 0xAC00 + i_value[0] * 0x24C + i_value[1] * 0x1C + i_value[2] + 1;
			inputChars += code;
		}
		else
		{
			i_level = 2;
			u16 code = 0xAC00 + i_value[0] * 0x24C + i_value[1] * 0x1C;
			inputChars += code;
		}
	}
}

int PSPOskDialog::GetIndex(const wchar_t* src, wchar_t ch)
{
	for(int i = 0, end = (int)wcslen(src); i < end; i++)
	{
		if(src[i] == ch)
		{
			return i;
		}
	}

	return -1;
}

u32 PSPOskDialog::FieldMaxLength()
{
	if ((oskParams->fields[0].outtextlimit > oskParams->fields[0].outtextlength - 1) || oskParams->fields[0].outtextlimit == 0)
		return oskParams->fields[0].outtextlength - 1;
	return oskParams->fields[0].outtextlimit;
}

void PSPOskDialog::RenderKeyboard()
{
	int selectedRow = selectedChar / numKeyCols[currentKeyboard];
	int selectedCol = selectedChar % numKeyCols[currentKeyboard];

	wchar_t temp[2];
	temp[1] = '\0';

	std::string buffer;

	static const u32 FIELDDRAWMAX = 16;
	u32 limit = FieldMaxLength();
	u32 drawLimit = std::min(FIELDDRAWMAX, limit);   // Field drew length limit.

	const float keyboardLeftSide = (480.0f - (24.0f * numKeyCols[currentKeyboard])) / 2.0f;
	const float characterWidth = 12.0f;
	float previewLeftSide = (480.0f - (12.0f * drawLimit)) / 2.0f;
	float title = (480.0f - (0.5f * drawLimit)) / 2.0f;


	PPGeDrawText(oskDesc.c_str(), title , 20, PPGE_ALIGN_CENTER, 0.5f, CalcFadedColor(0xFFFFFFFF));

	std::wstring result;

	result = CombinationString(false);

	u32 drawIndex = (u32)(result.size() > drawLimit ? result.size() - drawLimit : 0);
	drawIndex = result.size() == limit + 1 ? drawIndex - 1 : drawIndex;  // When the length reached limit, the last character don't fade in and out.
	for (u32 i = 0; i < drawLimit; ++i, ++drawIndex)
	{
		u32 color = CalcFadedColor(0xFFFFFFFF);
		if (drawIndex + 1 < result.size())
		{
			temp[0] = result[drawIndex];
			ConvertUCS2ToUTF8(buffer, temp);
			PPGeDrawText(buffer.c_str(), previewLeftSide + (i * characterWidth), 40.0f, PPGE_ALIGN_HCENTER, 0.5f, color);
		}
		else
		{
			if (drawIndex + 1 == result.size())
			{
				temp[0] = result[drawIndex];

				if(isCombinated == true)
				{
					float animStep = (float)(__DisplayGetNumVblanks() % 40) / 20.0f;
					// Fade in and out the next character so they know it's not part of the string yet.
					u32 alpha = (0.5f - (cosf(animStep * M_PI) / 2.0f)) * 128 + 127;
					color = CalcFadedColor((alpha << 24) | 0xFFFFFF);

					ConvertUCS2ToUTF8(buffer, temp);

					PPGeDrawText(buffer.c_str(), previewLeftSide + (i * characterWidth), 40.0f, PPGE_ALIGN_HCENTER, 0.5f, color);

					// Also draw the underline for the same reason.
					color = CalcFadedColor(0xFFFFFFFF);
					PPGeDrawText("_", previewLeftSide + (i * characterWidth), 40.0f, PPGE_ALIGN_HCENTER, 0.5f, color);
				}
				else
				{
					ConvertUCS2ToUTF8(buffer, temp);
					PPGeDrawText(buffer.c_str(), previewLeftSide + (i * characterWidth), 40.0f, PPGE_ALIGN_HCENTER, 0.5f, color);
				}
			}
			else
			{
				PPGeDrawText("_", previewLeftSide + (i * characterWidth), 40.0f, PPGE_ALIGN_HCENTER, 0.5f, color);
			}
		}
	}

	for (int row = 0; row < numKeyRows[currentKeyboard]; ++row)
	{
		for (int col = 0; col < numKeyCols[currentKeyboard]; ++col)
		{
			u32 color = CalcFadedColor(0xFFFFFFFF);
			if (selectedRow == row && col == selectedCol)
				color = CalcFadedColor(0xFF3060FF);

			temp[0] = oskKeys[currentKeyboard][row][col];

			ConvertUCS2ToUTF8(buffer, temp);
			PPGeDrawText(buffer.c_str(), keyboardLeftSide + (25.0f * col) + characterWidth / 2.0, 70.0f + (25.0f * row), PPGE_ALIGN_HCENTER, 0.6f, color);

			if (selectedRow == row && col == selectedCol)
				PPGeDrawText("_", keyboardLeftSide + (25.0f * col) + characterWidth / 2.0, 70.0f + (25.0f * row), PPGE_ALIGN_HCENTER, 0.6f, CalcFadedColor(0xFFFFFFFF));
		}
	}
}

#if defined(USING_WIN_UI)
// TODO: Why does this have a 2 button press lag/delay when
// re-opening the dialog box? I don't get it.
int PSPOskDialog::NativeKeyboard() {
	if (GetStatus() != SCE_UTILITY_STATUS_RUNNING) {
		return SCE_ERROR_UTILITY_INVALID_STATUS;
	}

	std::wstring titleText;
	GetWideStringFromPSPPointer(titleText, oskParams->fields[0].desc);

	std::wstring defaultText;
	GetWideStringFromPSPPointer(defaultText, oskParams->fields[0].intext);

	if (defaultText.empty())
		defaultText.assign(L"VALUE");

	if (System_InputBoxGetWString(titleText.c_str(), defaultText, inputChars)) {
		u32 maxLength = FieldMaxLength();
		if (inputChars.length() > maxLength) {
			ERROR_LOG(SCEUTILITY, "NativeKeyboard: input text too long(%d characters/glyphs max), truncating to game-requested length.", maxLength);
			inputChars.erase(maxLength, std::string::npos);
		}
	}
	ChangeStatus(SCE_UTILITY_STATUS_FINISHED, 0);
	
	u16_le *outText = oskParams->fields[0].outtext;

	size_t end = oskParams->fields[0].outtextlength;
	if (end > inputChars.size())
		end = inputChars.size() + 1;
	// Only write the bytes of the output and the null terminator, don't write the rest.
	for (size_t i = 0; i < end; ++i) {
		u16 value = 0;
		if (i < FieldMaxLength())
			value = inputChars[i];
		outText[i] = value;
	}

	oskParams->base.result = 0;
	oskParams->fields[0].result = PSP_UTILITY_OSK_RESULT_CHANGED;

	return 0;
}
#endif

int PSPOskDialog::Update(int animSpeed) {
	if (GetStatus() != SCE_UTILITY_STATUS_RUNNING) {
		return SCE_ERROR_UTILITY_INVALID_STATUS;
	}

	int cancelButton = g_Config.iButtonPreference == PSP_SYSTEMPARAM_BUTTON_CROSS ? CTRL_CIRCLE : CTRL_CROSS;
	int confirmButton = cancelButton == CTRL_CROSS ? CTRL_CIRCLE : CTRL_CROSS;
	static int cancelBtnFramesHeld = 0;
	static int confirmBtnFramesHeld = 0;
	static int leftBtnFramesHeld = 0;
	static int upBtnFramesHeld = 0;
	static int downBtnFramesHeld = 0;
	static int rightBtnFramesHeld = 0;
	const int framesHeldThreshold = 10;
	const int framesHeldRepeatRate = 5;

	UpdateButtons();
	int selectedRow = selectedChar / numKeyCols[currentKeyboard];
	int selectedExtra = selectedChar % numKeyCols[currentKeyboard];

	// TODO: Add your platforms here when you have a NativeKeyboard func.

#if defined(USING_WIN_UI)
	// Windows: Fall back to the OSK/continue normally if we're in fullscreen.
	// The dialog box doesn't work right if in fullscreen.
	if(g_Config.bBypassOSKWithKeyboard && !g_Config.bFullScreen)
		return NativeKeyboard();
#endif

	UpdateFade(animSpeed);

	StartDraw();
	PPGeDrawRect(0, 0, 480, 272, CalcFadedColor(0x63636363));
	RenderKeyboard();

	I18NCategory *di = GetI18NCategory("Dialog");

	PPGeDrawImage(I_SQUARE, 365, 222, 16, 16, 0, CalcFadedColor(0xFFFFFFFF));
	PPGeDrawText(di->T("Space"), 390, 222, PPGE_ALIGN_LEFT, 0.5f, CalcFadedColor(0xFFFFFFFF));

	if (g_Config.iButtonPreference != PSP_SYSTEMPARAM_BUTTON_CIRCLE) {
		PPGeDrawImage(I_CROSS, 45, 222, 16, 16, 0, CalcFadedColor(0xFFFFFFFF));
		PPGeDrawImage(I_CIRCLE, 45, 247, 16, 16, 0, CalcFadedColor(0xFFFFFFFF));
	} else {
		PPGeDrawImage(I_CIRCLE, 45, 222, 16, 16, 0, CalcFadedColor(0xFFFFFFFF));
		PPGeDrawImage(I_CROSS, 45, 247, 16, 16, 0, CalcFadedColor(0xFFFFFFFF));
	}

	PPGeDrawText(di->T("Select"), 75, 222, PPGE_ALIGN_LEFT, 0.5f, CalcFadedColor(0xFFFFFFFF));
	PPGeDrawText(di->T("Delete"), 75, 247, PPGE_ALIGN_LEFT, 0.5f, CalcFadedColor(0xFFFFFFFF));

	PPGeDrawText("Start", 135, 220, PPGE_ALIGN_LEFT, 0.6f, CalcFadedColor(0xFFFFFFFF));
	PPGeDrawText(di->T("Finish"), 185, 222, PPGE_ALIGN_LEFT, 0.5f, CalcFadedColor(0xFFFFFFFF));

	auto lookupLangName = [&](int direction) {
		// First, find the valid one...
		OskKeyboardLanguage lang = (OskKeyboardLanguage)((OSK_LANGUAGE_COUNT + currentKeyboardLanguage + direction) % OSK_LANGUAGE_COUNT);
		OskKeyboardDisplay disp = OskKeyboardCases[lang][LOWERCASE];
		FindValidKeyboard(oskParams->fields[0].inputtype, direction, lang, disp);

		if (lang == currentKeyboardLanguage) {
			return (const char *)nullptr;
		}

		// Now, let's grab the name.
		const char *countryCode = OskKeyboardNames[lang].c_str();
		const char *language = languageMapping[countryCode].first.c_str();

		// It seems like this is a "fake" country code for extra keyboard purposes.
		if (!strcmp(countryCode, "English Full-width"))
			language = "English Full-width";

		return language;
	};

	if (OskKeyboardNames[currentKeyboardLanguage] != "ko_KR" && IsKeyboardShiftValid(oskParams->fields[0].inputtype, currentKeyboardLanguage, currentKeyboard)) {
		PPGeDrawText("Select", 135, 245, PPGE_ALIGN_LEFT, 0.6f, CalcFadedColor(0xFFFFFFFF));
		PPGeDrawText(di->T("Shift"), 185, 247, PPGE_ALIGN_LEFT, 0.5f, CalcFadedColor(0xFFFFFFFF));
	}

	const char *prevLang = lookupLangName(-1);
	if (prevLang) {
		PPGeDrawText("L", 235, 220, PPGE_ALIGN_LEFT, 0.6f, CalcFadedColor(0xFFFFFFFF));
		PPGeDrawText(prevLang, 255, 222, PPGE_ALIGN_LEFT, 0.5f, CalcFadedColor(0xFFFFFFFF));
	}

	const char *nextLang = lookupLangName(1);
	if (nextLang) {
		PPGeDrawText("R", 235, 245, PPGE_ALIGN_LEFT, 0.6f, CalcFadedColor(0xFFFFFFFF));
		PPGeDrawText(nextLang, 255, 247, PPGE_ALIGN_LEFT, 0.5f, CalcFadedColor(0xFFFFFFFF));
	}

	if (IsButtonPressed(CTRL_UP) || IsButtonHeld(CTRL_UP, upBtnFramesHeld, framesHeldThreshold, framesHeldRepeatRate)) {
		selectedChar -= numKeyCols[currentKeyboard];
	} else if (IsButtonPressed(CTRL_DOWN) || IsButtonHeld(CTRL_DOWN, downBtnFramesHeld, framesHeldThreshold, framesHeldRepeatRate)) {
		selectedChar += numKeyCols[currentKeyboard];
	} else if (IsButtonPressed(CTRL_LEFT) || IsButtonHeld(CTRL_LEFT, leftBtnFramesHeld, framesHeldThreshold, framesHeldRepeatRate)) {
		selectedChar--;
		if (((selectedChar + numKeyCols[currentKeyboard]) % numKeyCols[currentKeyboard]) == numKeyCols[currentKeyboard] - 1)
			selectedChar += numKeyCols[currentKeyboard];
	} else if (IsButtonPressed(CTRL_RIGHT) || IsButtonHeld(CTRL_RIGHT, rightBtnFramesHeld, framesHeldThreshold, framesHeldRepeatRate)) {
		selectedChar++;
		if ((selectedChar % numKeyCols[currentKeyboard]) == 0)
			selectedChar -= numKeyCols[currentKeyboard];
	}

	selectedChar = (selectedChar + (numKeyCols[currentKeyboard] * numKeyRows[currentKeyboard])) % (numKeyCols[currentKeyboard] * numKeyRows[currentKeyboard]);

	if (IsButtonPressed(confirmButton) || IsButtonHeld(confirmButton, confirmBtnFramesHeld, framesHeldThreshold, framesHeldRepeatRate)) {
		inputChars = CombinationString(true);
	} else if (IsButtonPressed(CTRL_SELECT)) {
		// Select now swaps case.
		if (IsKeyboardShiftValid(oskParams->fields[0].inputtype, currentKeyboardLanguage, currentKeyboard)) {
			if (currentKeyboard == OskKeyboardCases[currentKeyboardLanguage][UPPERCASE])
				currentKeyboard = OskKeyboardCases[currentKeyboardLanguage][LOWERCASE];
			else
				currentKeyboard = OskKeyboardCases[currentKeyboardLanguage][UPPERCASE];
		}

		if (selectedRow >= numKeyRows[currentKeyboard]) {
			selectedRow = numKeyRows[currentKeyboard] - 1;
		}

		if (selectedExtra >= numKeyCols[currentKeyboard]) {
			selectedExtra = numKeyCols[currentKeyboard] - 1;
		}

		selectedChar = selectedRow * numKeyCols[currentKeyboard] + selectedExtra;
	} else if (IsButtonPressed(CTRL_RTRIGGER)) {
		// RTRIGGER now cycles languages forward.
		currentKeyboardLanguage = (OskKeyboardLanguage)((currentKeyboardLanguage + 1) % OSK_LANGUAGE_COUNT);
		currentKeyboard = OskKeyboardCases[currentKeyboardLanguage][LOWERCASE];
		FindValidKeyboard(oskParams->fields[0].inputtype, 1, currentKeyboardLanguage, currentKeyboard);

		if (selectedRow >= numKeyRows[currentKeyboard]) {
			selectedRow = numKeyRows[currentKeyboard] - 1;
		}

		if (selectedExtra >= numKeyCols[currentKeyboard]) {
			selectedExtra = numKeyCols[currentKeyboard] - 1;
		}

		selectedChar = selectedRow * numKeyCols[currentKeyboard] + selectedExtra;
	} else if (IsButtonPressed(CTRL_LTRIGGER)) {
		// LTRIGGER now cycles languages backward.
		if (currentKeyboardLanguage - 1 >= 0)
			currentKeyboardLanguage = (OskKeyboardLanguage)((currentKeyboardLanguage - 1) % OSK_LANGUAGE_COUNT);
		else
			currentKeyboardLanguage = (OskKeyboardLanguage)(OSK_LANGUAGE_COUNT - 1);
		currentKeyboard = OskKeyboardCases[currentKeyboardLanguage][LOWERCASE];
		FindValidKeyboard(oskParams->fields[0].inputtype, -1, currentKeyboardLanguage, currentKeyboard);

		if (selectedRow >= numKeyRows[currentKeyboard]) {
			selectedRow = numKeyRows[currentKeyboard] - 1;
		}

		if (selectedExtra >= numKeyCols[currentKeyboard]) {
			selectedExtra = numKeyCols[currentKeyboard] - 1;
		}

		selectedChar = selectedRow * numKeyCols[currentKeyboard] + selectedExtra;
	} else if (IsButtonPressed(cancelButton) || IsButtonHeld(cancelButton, cancelBtnFramesHeld, framesHeldThreshold, framesHeldRepeatRate)) {
		if (inputChars.size() > 0) {
			inputChars.resize(inputChars.size() - 1);
			if (i_level != 0) {
				RemoveKorean();
			}
		}
	} else if (IsButtonPressed(CTRL_START)) {
		StartFade(false);
	} else if (IsButtonPressed(CTRL_SQUARE) && inputChars.size() < FieldMaxLength()) {
		// Use a regular space if the current keyboard isn't Japanese nor full-width English
		if (currentKeyboardLanguage != OSK_LANGUAGE_JAPANESE && currentKeyboardLanguage != OSK_LANGUAGE_ENGLISH_FW)
			inputChars += L" ";
		else
			inputChars += L"　";
	}

	EndDraw();

	u16_le *outText = oskParams->fields[0].outtext;
	size_t end = oskParams->fields[0].outtextlength;
	// Only write the bytes of the output and the null terminator, don't write the rest.
	if (end > inputChars.size())
		end = inputChars.size() + 1;
	for (size_t i = 0; i < end; ++i)
	{
		u16 value = 0;
		if (i < inputChars.size())
			value = inputChars[i];
		outText[i] = value;
	}

	oskParams->base.result = 0;
	oskParams->fields[0].result = PSP_UTILITY_OSK_RESULT_CHANGED;
	return 0;
}

int PSPOskDialog::Shutdown(bool force)
{
	if (GetStatus() != SCE_UTILITY_STATUS_FINISHED && !force)
		return SCE_ERROR_UTILITY_INVALID_STATUS;

	PSPDialog::Shutdown(force);
	if (!force) {
		ChangeStatusShutdown(OSK_SHUTDOWN_DELAY_US);
	}

	return 0;
}

void PSPOskDialog::DoState(PointerWrap &p)
{
	PSPDialog::DoState(p);

	auto s = p.Section("PSPOskDialog", 1);
	if (!s)
		return;

	p.Do(oskParams);
	p.Do(oskDesc);
	p.Do(oskIntext);
	p.Do(oskOuttext);
	p.Do(selectedChar);
	p.Do(inputChars);
}

pspUtilityDialogCommon *PSPOskDialog::GetCommonParam()
{
	return &oskParams->base;
}
