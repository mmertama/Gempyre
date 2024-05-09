#pragma once
#include "gempyre_types.h"
#include "data.h"

class Gempyre::CanvasData  {
private:
    enum DataTypes : dataT {
      CanvasId = 0xAAA
    };
public:
    static constexpr auto NO_ID = "";
    CanvasData(int w, int h, const std::string& owner);
    CanvasData(int w, int h) : CanvasData(w, h, NO_ID) {}
    ~CanvasData() = default;
    void put(int x, int y, dataT pixel) {
        data()[x + y * m_width] = pixel;
    }
    [[nodiscard]] dataT get(int x, int y) const {
        return data()[x + y * m_width];
    }
    int width() const {return m_width;}
    int height() const {return m_height;}
    size_t size() const {return m_data->size();}


    dataT* data() {return m_data->data();}
    const dataT* data() const {return m_data->data();}
    const Data& ref() const { return *m_data; }
    Data& ref() { return *m_data; }
    DataPtr ptr() const {return m_data;}   

 #ifdef GEMPYRE_IS_DEBUG
    std::string dump() const {return m_data->dump();}
#endif
private:
    std::shared_ptr<Data> m_data;
    const int m_width{0};
    const int m_height{0};  
};

