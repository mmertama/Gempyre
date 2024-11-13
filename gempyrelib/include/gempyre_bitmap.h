#pragma once

#include <string>
#include <algorithm>
#include <cstring> // memcpy (windows)
#include <gempyre_types.h>
#include <array>

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