#include "canvas_data.h"

using namespace Gempyre;

CanvasData::CanvasData(int w, int h, std::string_view owner) :
    m_data{std::make_shared<Data>(
        static_cast<size_t>(w * h),
        static_cast<dataT>(CanvasId),
        owner,
        std::vector<dataT>{0, 0, static_cast<dataT>(w), static_cast<dataT>(h),
        false // no update cb
        }
        )},
    m_width{w},
    m_height{h} {}

