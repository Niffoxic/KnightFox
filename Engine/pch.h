#ifndef PCH_H
#define PCH_H

// Reduce Windows header bloat
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
	#define NOMINMAX
#endif

#include <windows.h>

// DirectX 12 / DXGI / WRL
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

// STL core utilities
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <limits>
#include <algorithm>

// text utilities
#include <cwctype>
#include <cctype>
#include <sstream>
#include <iomanip>

#endif // PCH_H
