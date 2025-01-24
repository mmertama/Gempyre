#pragma once

#include <string>
#include <algorithm>
#include <cstring> // memcpy (windows)
#include <gempyre_types.h>
#include <array>
#include <optional>
#include <string_view>
#include <vector>
#include <type_traits>
#include <algorithm>

/**
  * @file
  * 
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  *
  * gempyre_utils_graphics.h API for common graphics classes 
  * 
  */


#ifdef WINDOWS_EXPORT
    #ifndef GEMPYRE_EX
        #define GGEMPYRE_EX __declspec( dllexport )
    //#else
    //    #define GEMPYRE_EX
    #endif
#endif

#define gempyre_graphics_assert(b, x) ((b) || GempyreUtils::do_fatal(x, nullptr, __FILE__, __LINE__));


namespace  Gempyre {
    class CanvasElement;

    /// @brief RGB handling
    namespace  Color {
        /// @brief pixel type
        using type = Gempyre::dataT;

        [[nodiscard]]
        /// @brief  Pack r,g b and a components by saturating value range. 
        static constexpr inline type rgba_clamped(type r, type g, type b, type a = 0xFF) {
            const type FF = 0xFF;
            return std::min(FF, r) | (std::min(FF, g) << 8) | (std::min(FF, b) << 16) | (std::min(FF, a) << 24);
        }

        [[nodiscard]] 
        /// @brief   Pack r,g b, and a components into pixel. Each component is expected to be less than 256. 
        static constexpr inline type rgba(type r, type g, type b, type a = 0xFF) {
            return r | (g << 8) | (b << 16) | (a << 24);
        }

        [[nodiscard]] 
        /// Pack r,g and b components into pixel. 
        static constexpr inline type rgb(type r, type g, type b) {
            return r | (g << 8) | (b << 16) | (static_cast<type>(0xFF) << 24);
        }

        [[nodiscard]]
        ///@brief   Get red component.
        static constexpr inline type r(type pixel) {
            return pixel & static_cast<type>(0xFF);
        }
        [[nodiscard]] 
        ///@brief   Get green component.
        static constexpr inline type g(type pixel) {
            return (pixel & static_cast<type>(0xFF00)) >> 8;
        }

        [[nodiscard]] 
        ///@brief   Get blue component.
        static constexpr inline type b(type pixel) {
            return (pixel & static_cast<type>(0xFF0000)) >> 16;
        }

        [[nodiscard]] 
        ///@brief   Get alpha component.
        static constexpr inline type alpha(type pixel) {
            return (pixel & 0xFF000000) >> 24;
        }

        [[nodiscard]] 
        ///@brief   Get pixel as a HTML string.
        static inline std::string rgba(type pixel) {
            constexpr auto c = "0123456789ABCDEF";
            std::string v("#RRGGBBAA");
            v[1] =  c[r(pixel) >> 4];
            v[2] =  c[r(pixel) & 0xF];
            v[3] =  c[g(pixel) >> 4];
            v[4] =  c[g(pixel) & 0xF];
            v[5] =  c[b(pixel) >> 4];
            v[6] =  c[b(pixel) & 0xF];
            v[7] =  c[alpha(pixel) >> 4];
            v[8] =  c[alpha(pixel) & 0xF];
            return v;
        }

        [[nodiscard]] 
        ////@brief  Get pixel as a HTML string.
        static inline std::string rgb(type pixel) {
            constexpr auto c = "0123456789ABCDEF";
            std::string v("#RRGGBB");
            v[1] =  c[r(pixel) >> 4];
            v[2] =  c[r(pixel) & 0xF];
            v[3] =  c[g(pixel) >> 4];
            v[4] =  c[g(pixel) & 0xF];
            v[5] =  c[b(pixel) >> 4];
            v[6] =  c[b(pixel) & 0xF];
        return v;
        }

        [[nodiscard]] 
        ////@brief  Get components as a HTML string 
        static inline std::string to_string(type r, type g, type b, type a = 0xFF) {
            return a == 0xFF ? Gempyre::Color::rgb(Gempyre::Color::rgb(r, g, b)) : Gempyre::Color::rgba(Gempyre::Color::rgba(r, g, b, a));
        }

        [[nodiscard]] 
        ///@brief  Get color as a HYML string 
        static inline std::string to_string(Gempyre::Color::type color) {
            return Gempyre::Color::to_string(
                Gempyre::Color::r(color),
                Gempyre::Color::g(color),
                Gempyre::Color::b(color),
                Gempyre::Color::alpha(color));
        }

        [[nodiscard]]
        ///@brief Apply given alpha to a color
        static inline auto set_alpha(Gempyre::Color::type col, uint8_t alpha_val) {
            return Gempyre::Color::rgba(Gempyre::Color::r(col), Gempyre::Color::g(col), Gempyre::Color::b(col), alpha_val);
        }

        
        [[nodiscard]]
        ///@brief Apply given alpha to a color
        static inline auto rgb_value(uint32_t col) {
            return Gempyre::Color::rgba(0xFF & (col >> 16), 0xFF & (col >> 8), 0xFF & col, 0xFF);
        }

        [[nodiscard]]
        ///@brief Apply given alpha to a color
        static inline auto rgba_value(uint32_t col) {
            return Gempyre::Color::rgba(0xFF & (col >> 24), 0xFF & (col >> 16),  0xFF & (col >> 8),  0xFF & col);
        }


        /// @brief Black
        static constexpr Color::type Black      = Color::rgba(0, 0, 0, 0xFF);
        /// @brief White
        static constexpr Color::type White      = Color::rgba(0xFF, 0xFF, 0xFF, 0xFF);
        /// @brief Red
        static constexpr Color::type Red        = Color::rgba(0xFF, 0, 0, 0xFF);
        /// @brief Green
        static constexpr Color::type Green      = Color::rgba(0, 0xFF, 0, 0xFF);
        /// @brief Blue
        static constexpr Color::type Blue       = Color::rgba(0, 0, 0xFF, 0xFF);
        /// @brief Cyan
        static constexpr Color::type Cyan       = Color::rgba(0, 0xFF, 0xFF, 0xFF);
        /// @brief  Magenta
        static constexpr Color::type Magenta    = Color::rgba(0xFF, 0, 0xFF, 0xFF);
        /// @brief Yellow
        static constexpr Color::type Yellow     = Color::rgba(0xFF, 0xFF, 0, 0xFF);
        /// @brief Cyan
        static constexpr Color::type Aqua       = Cyan;
        /// @brief Fuchsia
        static constexpr Color::type Fuchsia    = Magenta;
        /// @brief Lime
        static constexpr Color::type Lime       = Green;
        /// @brief Transparent
        static constexpr Color::type Transparent = Color::rgba(0, 0, 0, 0);

    /// @brief HTML colors
    constexpr std::array<std::pair<std::string_view, type>, 147> html_colors = {{
    {"AliceBlue", 0xF0F8FF}, {"AntiqueWhite", 0xFAEBD7}, {"Aqua", 0x00FFFF},
    {"Aquamarine", 0x7FFFD4}, {"Azure", 0xF0FFFF}, {"Beige", 0xF5F5DC},
    {"Bisque", 0xFFE4C4}, {"Black", 0x000000}, {"BlanchedAlmond", 0xFFEBCD},
    {"Blue", 0x0000FF}, {"BlueViolet", 0x8A2BE2}, {"Brown", 0xA52A2A},
    {"BurlyWood", 0xDEB887}, {"CadetBlue", 0x5F9EA0}, {"Chartreuse", 0x7FFF00},
    {"Chocolate", 0xD2691E}, {"Coral", 0xFF7F50}, {"CornflowerBlue", 0x6495ED},
    {"Cornsilk", 0xFFF8DC}, {"Crimson", 0xDC143C}, {"Cyan", 0x00FFFF},
    {"DarkBlue", 0x00008B}, {"DarkCyan", 0x008B8B}, {"DarkGoldenRod", 0xB8860B},
    {"DarkGray", 0xA9A9A9}, {"DarkGreen", 0x006400}, {"DarkKhaki", 0xBDB76B},
    {"DarkMagenta", 0x8B008B}, {"DarkOliveGreen", 0x556B2F}, {"DarkOrange", 0xFF8C00},
    {"DarkOrchid", 0x9932CC}, {"DarkRed", 0x8B0000}, {"DarkSalmon", 0xE9967A},
    {"DarkSeaGreen", 0x8FBC8F}, {"DarkSlateBlue", 0x483D8B}, {"DarkSlateGray", 0x2F4F4F},
    {"DarkTurquoise", 0x00CED1}, {"DarkViolet", 0x9400D3}, {"DeepPink", 0xFF1493},
    {"DeepSkyBlue", 0x00BFFF}, {"DimGray", 0x696969}, {"DodgerBlue", 0x1E90FF},
    {"FireBrick", 0xB22222}, {"FloralWhite", 0xFFFAF0}, {"ForestGreen", 0x228B22},
    {"Fuchsia", 0xFF00FF}, {"Gainsboro", 0xDCDCDC}, {"GhostWhite", 0xF8F8FF},
    {"Gold", 0xFFD700}, {"GoldenRod", 0xDAA520}, {"Gray", 0x808080},
    {"Green", 0x008000}, {"GreenYellow", 0xADFF2F}, {"HoneyDew", 0xF0FFF0},
    {"HotPink", 0xFF69B4}, {"IndianRed", 0xCD5C5C}, {"Indigo", 0x4B0082},
    {"Ivory", 0xFFFFF0}, {"Khaki", 0xF0E68C}, {"Lavender", 0xE6E6FA},
    {"LavenderBlush", 0xFFF0F5}, {"LawnGreen", 0x7CFC00}, {"LemonChiffon", 0xFFFACD},
    {"LightBlue", 0xADD8E6}, {"LightCoral", 0xF08080}, {"LightCyan", 0xE0FFFF},
    {"LightGoldenRodYellow", 0xFAFAD2}, {"LightGray", 0xD3D3D3}, {"LightGreen", 0x90EE90},
    {"LightPink", 0xFFB6C1}, {"LightSalmon", 0xFFA07A}, {"LightSeaGreen", 0x20B2AA},
    {"LightSkyBlue", 0x87CEFA}, {"LightSlateGray", 0x778899}, {"LightSteelBlue", 0xB0C4DE},
    {"LightYellow", 0xFFFFE0}, {"Lime", 0x00FF00}, {"LimeGreen", 0x32CD32},
    {"Linen", 0xFAF0E6}, {"Magenta", 0xFF00FF}, {"Maroon", 0x800000},
    {"MediumAquaMarine", 0x66CDAA}, {"MediumBlue", 0x0000CD}, {"MediumOrchid", 0xBA55D3},
    {"MediumPurple", 0x9370DB}, {"MediumSeaGreen", 0x3CB371}, {"MediumSlateBlue", 0x7B68EE},
    {"MediumSpringGreen", 0x00FA9A}, {"MediumTurquoise", 0x48D1CC}, {"MediumVioletRed", 0xC71585},
    {"MidnightBlue", 0x191970}, {"MintCream", 0xF5FFFA}, {"MistyRose", 0xFFE4E1},
    {"Moccasin", 0xFFE4B5}, {"NavajoWhite", 0xFFDEAD}, {"Navy", 0x000080},
    {"OldLace", 0xFDF5E6}, {"Olive", 0x808000}, {"OliveDrab", 0x6B8E23},
    {"Orange", 0xFFA500}, {"OrangeRed", 0xFF4500}, {"Orchid", 0xDA70D6},
    {"PaleGoldenRod", 0xEEE8AA}, {"PaleGreen", 0x98FB98}, {"PaleTurquoise", 0xAFEEEE},
    {"PaleVioletRed", 0xDB7093}, {"PapayaWhip", 0xFFEFD5}, {"PeachPuff", 0xFFDAB9},
    {"Peru", 0xCD853F}, {"Pink", 0xFFC0CB}, {"Plum", 0xDDA0DD},
    {"PowderBlue", 0xB0E0E6}, {"Purple", 0x800080}, {"RebeccaPurple", 0x663399},
    {"Red", 0xFF0000}, {"RosyBrown", 0xBC8F8F}, {"RoyalBlue", 0x4169E1},
    {"SaddleBrown", 0x8B4513}, {"Salmon", 0xFA8072}, {"SandyBrown", 0xF4A460},
    {"SeaGreen", 0x2E8B57}, {"SeaShell", 0xFFF5EE}, {"Sienna", 0xA0522D},
    {"Silver", 0xC0C0C0}, {"SkyBlue", 0x87CEEB}, {"SlateBlue", 0x6A5ACD},
    {"SlateGray", 0x708090}, {"Snow", 0xFFFAFA}, {"SpringGreen", 0x00FF7F},
    {"SteelBlue", 0x4682B4}, {"Tan", 0xD2B48C}, {"Teal", 0x008080},
    {"Thistle", 0xD8BFD8}, {"Tomato", 0xFF6347}, {"Turquoise", 0x40E0D0},
    {"Violet", 0xEE82EE}, {"Wheat", 0xF5DEB3}, {"White", 0xFFFFFF},
    {"WhiteSmoke", 0xF5F5F5}, {"Yellow", 0xFFFF00}, {"YellowGreen", 0x9ACD32}
    }};

    [[nodiscard]] inline bool is_equal(type a, type b) {
        return (a & 0xFFFFFF) == (b & 0xFFFFFF);  
    }

    [[nodiscard]]
    ///@brief find a HTML color as Color::type 
    std::optional<type> from_html_name(std::string_view name);

    [[nodiscard]]
    ///@brief get color from string, where string is a HTML color name, #RRGGBB, #RRGGBBAA, 0xRRGGBB or 0xRRGGBBAA
    std::optional<type> get_color(std::string_view color);

    }

    /// @brief Bitmap for Gempyre Graphics
    class GEMPYRE_EX Bitmap {
    public:
        /// @brief Constructor - uninitialized data
        /// @param width 
        /// @param height 
        Bitmap(int width, int height);

        /// @brief Constructor - with a single color data
        /// @param width 
        /// @param height 
        /// @param color 
        Bitmap(int width, int height, Gempyre::Color::type color);
        
        /// @brief Constructor - zero size, use @see create() to create the actual bitmap.
        Bitmap();
        
        /// @brief Move constructor. 
        Bitmap(Bitmap&& other) = default;
        
        /// Copy constructor - does not copy the data, for deep copy @see clone() 
        Bitmap(const Bitmap& other) = default;
        
        /// Destructor
        ~Bitmap();

        /// @brief Create bitmap from byte array. 
        /// @param image_data PNG image in bytes.
        /// @note
        /// @code{.cpp}
        /// const auto bytes = GempyreUtils::slurp<uint8_t>("image.png");
        /// Bitmap bmp(bytes);
        /// @endcode
        Bitmap(const std::vector<uint8_t>& image_data);

        /// @brief Convert a bitmap to PNG
        /// @return PNG bytes
        std::vector<uint8_t> png_image() const;

        /// Copy operator does only shallow copy, for deep copy @see clone() 
        Bitmap& operator=(const Bitmap& other) = default;
        
        /// Move operator. 
        Bitmap& operator=(Bitmap&& other) = default;

        /// @brief  Create bitmap bytes.
        /// @param width 
        /// @param height 
        void create(int width, int height);

        /// Deep copy bitmap bytes.
        Bitmap clone() const;

        /// Components to pixel type. 
        static constexpr Color::type pix(Color::type r, Color::type g, Color::type b, Color::type a = 0xFF) {return Color::rgba(r, g, b, a);}
        
        /// Set a single pixel.
        void set_pixel(int x, int y, Color::type color);

        /// Set a singe pixel's alpha value. 
        void set_alpha(int x, int y, Color::type alpha);

        /// Get a single pixel.
        Color::type pixel(int x, int y) const;

        /// Get width.
        [[nodiscard]] int width() const;

        /// Get height.
        [[nodiscard]] int height() const;

        /// Swap bitmap data with another. 
        void swap(Bitmap& other);

        /// Draw a rect with a color in bitmap.
        void draw_rect(const Gempyre::Rect& rect, Color::type color);

        /// Draw a Bitmap on this bitmap - merge alpha.  
        void merge(int x, int y, const Bitmap& other);

        /// Draw a Bitmap on this bitmap - merge alpha.  
        void merge(const Bitmap& other) {merge(0, 0, other);}

        /// Draw a Bitmap on this bitmap - replace area.  
        void tile(int x, int y, const Bitmap& other);

        /// Draw a Bitmap withing extents on this bitmap - replace area.  
        void tile(int x, int y, const Bitmap& other, int width, int height);

        /// Draw a Bitmap withing extents on this bitmap - replace area.  
        void tile(int x, int y, const Bitmap& other, int other_x, int other_y, int width, int height);

        /// Create a new bitmap from part of bitmap
        Bitmap clip(const Gempyre::Rect& rect) const;

        /// return true if there is not data  
        bool empty() const;

        /// underlaying data
        const uint8_t* const_data() const;

        /// @brief  Copy pixels into bitmap
        /// @param bytes (TODO will change to std::span when moving to C++20)
        /// @param offset 
        /// @return false on overflow, otherwise true
        template<class T, std::enable_if_t<std::is_same_v<typename T::value_type, Color::type>, int> = 0>
        bool set_data(const T& bytes, size_t offset = 0) {
            if(bytes.size() + offset > size())
                return false;
            std::memcpy(inner_data() + offset * sizeof(Color::type), bytes.data(), sizeof(Color::type) * bytes.size());
            return true;
        }

    protected:
        /// @cond INTERNAL
        void copy_from(const Bitmap& other);
        Color::type* inner_data();
        std::size_t size() const;
        /// @endcond
    private:
        friend class Gempyre::CanvasElement;
        Gempyre::CanvasDataPtr m_canvas{};
    };

}