#include "MouseEmulator.h"

#include <Windows.h>

namespace mouse {
	void leftClick() {
		INPUT inputs[2] = {};
		
		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		inputs[1].type = INPUT_MOUSE;
		inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		SendInput(2, inputs, sizeof(INPUT));
	}

	void rightClick() {
		INPUT inputs[2] = {};

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

		inputs[1].type = INPUT_MOUSE;
		inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;

		SendInput(2, inputs, sizeof(INPUT));
	}

	void scrollBy(int delX, int delY) {
		INPUT inputs[1] = {};

		if (delX != 0) {
			inputs[0].type = INPUT_MOUSE;
			inputs[0].mi.dwFlags = MOUSEEVENTF_HWHEEL;
			inputs[0].mi.mouseData = (UINT32)delX;
			SendInput(1, inputs, sizeof(INPUT));
		}

		if (delY != 0) {
			inputs[0].type = INPUT_MOUSE;
			inputs[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
			inputs[0].mi.mouseData = 0-(UINT32)delY;
			SendInput(1, inputs, sizeof(INPUT));
		}
	}

	void moveBy(int delX, int delY) {
		INPUT inputs[1] = {};

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;
		inputs[0].mi.dx = (UINT32)delX;
		inputs[0].mi.dy = (UINT32)delY;
		SendInput(1, inputs, sizeof(INPUT));
	}

	void dragBy(int delX, int delY) {
		INPUT inputs[3] = {};

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		SendInput(1, inputs, sizeof(INPUT));

		inputs[1].type = INPUT_MOUSE;
		inputs[1].mi.dwFlags = MOUSEEVENTF_MOVE;
		inputs[1].mi.dx = (UINT32)delX;
		inputs[1].mi.dy = (UINT32)delY;

		inputs[2].type = INPUT_MOUSE;
		inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		SendInput(3, inputs, sizeof(INPUT));
	}
}
