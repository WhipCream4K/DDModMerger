#pragma once

#include "WindowContext.h"

class Widget
{
public:

	Widget(WindowContext windowContext = WindowContext())
		: m_WindowContext(windowContext)
	{
	}

	virtual void Draw() = 0;
	virtual ~Widget() = default;

protected:

	WindowContext m_WindowContext;
};

