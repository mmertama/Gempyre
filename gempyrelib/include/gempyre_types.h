#pragma once

#include <cstdint>
#include <memory>

/// @cond INTERNAL
#ifdef WINDOWS_EXPORT
#define GEMPYRE_EX __declspec( dllexport )
#else
#define GEMPYRE_EX
#endif
/// @endcond

namespace Gempyre {
     /// @cond INTERNAL
    class Data;
    using DataPtr = std::shared_ptr<Data>;
    class CanvasData; 
    using dataT = uint32_t;
    using CanvasDataPtr = std::shared_ptr<CanvasData>;
    /// @endcond

    /// @brief Rect
    struct Rect {
        /// @brief rectangle x coordinate.
        int x;
        /// @brief rectangle y coordinate.
        int y;
        /// @brief rectangle width.
        int width;
        /// @brief rectangle height.
        int height;
    };
}

