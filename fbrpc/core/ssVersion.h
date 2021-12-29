#pragma once

namespace fbrpc
{
	struct sVersion
	{
		int major;
		int minor;
		int patch;
	};

	// https://semver.org/
	constexpr sVersion getVersion() { return { 0, 1, 0 }; }
}