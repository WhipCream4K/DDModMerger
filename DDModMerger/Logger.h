#pragma once

#include "Widget.h"

class Logger : public Widget
{
public:

	Logger() = default;

	void Draw() override;

	~Logger() = default;
};

