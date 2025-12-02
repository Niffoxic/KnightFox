#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#if defined(ENGINE_EXPORTS)
		#define KFE_API __declspec(dllexport)
	#else
		#define KFE_API __declspec(dllimport)
	#endif
#else
	#define KFE_API
#endif
