#pragma once

#include <stdint.h>

namespace mouse {
	void leftClick();
	void rightClick();

	/// +delX = scroll right
	/// +delY = scroll down
	void scrollBy(int delX, int delY);

	/// +delX = move right
	/// +delY = move down
	void moveBy(int delX, int delY);

	/// +delX = drag right
	/// +delY = drag down
	void dragBy(int delX, int delY);
}