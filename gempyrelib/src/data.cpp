#include "data.h"
#include "gempyre_utils.h"
#include <cassert>

using namespace Gempyre;

template <typename T> T align(T a) {return (a + 3U) & ~3U;}

constexpr auto fixedDataSize = 4;

// all Data is indexed so send can keep their order in broadcaster
static unsigned g_index_couter = 0;


Data::Data(size_t sz, dataT type, std::string_view owner, const std::vector<dataT>& header) :
    m_data(sz + (fixedDataSize + header.size()) + align(owner.size())), m_index(g_index_couter++) {
        m_data[0] = type;
        m_data[1] = static_cast<dataT>(sz);
        m_data[2] = align(static_cast<dataT>(owner.size()));
        m_data[3] = static_cast<dataT>(header.size());
        std::copy(header.begin(), header.end(), endPtr());
        auto idData = reinterpret_cast<uint16_t*>(endPtr() + header.size());

        const auto owner_size = align(owner.size());
        for(auto i = 0U; i < owner_size; i++) {
            const auto c = i < owner.size() ?  owner[i] : 0; // todo better and handle endianness
            idData[i] = static_cast<uint16_t>(c);
        }
        assert(header.size() == 5);
/*#ifdef GEMPYRE_IS_DEBUG
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send-data_buffer", owner,
        reinterpret_cast<uint64_t>(idData) - reinterpret_cast<uint64_t>(&m_data[0]), owner.size(), dump());
#endif*/          
}

std::string Data::owner() const {
    std::string out;
    const auto pos = reinterpret_cast<const uint16_t*>(endPtr() + m_data[3]);
    for(auto i = 0U; i < m_data[2]; i++) {
        const wchar_t c = pos[i];
        if(c == 0) //name is padded to alignement so there may be extra zeroes
            break;
        out += static_cast<std::string::value_type>(c);
    }
    return out;
}

bool Data::has_owner() const {
    return m_data[2] > 0;
}

std::vector<Gempyre::dataT> Data::header() const {
    std::vector<dataT> out;
    std::copy(endPtr(), endPtr() + m_data[3], std::back_inserter(out));
    return out;
}

void Data::writeHeader(const std::vector<dataT>& header) {
     gempyre_utils_assert_x(header.size() == m_data[3], "Header sizes must match!");
     std::copy(header.begin(), header.end(), end());
}

std::tuple<const char*, size_t> Data::payload() const {
    return {reinterpret_cast<const char*>(m_data.data()), size()};
}

dataT* Data::data() {
    return &m_data.data()[fixedDataSize];
}

const dataT* Data::data() const {
    return &m_data.data()[fixedDataSize];
}

unsigned Data::elements() const {
    return m_data[1];
}

DataPtr Data::clone() const {
    auto ptr = std::shared_ptr<Data>(new Data(elements(), m_data[0], owner(), header()));
    std::copy(begin(), end(), ptr->begin());
    return ptr;
}


#ifdef GEMPYRE_IS_DEBUG
        std::string Data::dump() const {
            std::stringstream ss;
            const auto& bytes = m_data;
            const auto type = bytes[0];
            ss << "type: " << std::hex << type << std::endl;
            const auto datalen = bytes[1];
            ss << "datalen: " << std::dec << datalen * 4 << std::endl;
            const auto idLen = bytes[2];
             ss << "idLen: " << idLen << std::endl;
            const auto headerLen = bytes[3];
            ss << "headerLen: " << headerLen << std::endl;
            const auto dataOffset = 4; //id, datalen, idlen, headerlen, data<datalen>, header<headerlen>, id<idlen>
            ss << "dataOffset: " << dataOffset * 4 << std::endl;
            const auto headerOffset = bytes[1] + 4;
            ss << "headerOffset: " << headerOffset << std::endl;
            const auto x = bytes[headerOffset];
            ss << "x: " << x << std::endl;
            const auto y = bytes[headerOffset + 1];
            ss << "y: " << y << std::endl;
            const auto w = bytes[headerOffset + 2];
            ss << "w: " << w << std::endl;
            const auto h = bytes[headerOffset + 3];
            ss << "h: " << h << std::endl;
            const auto as_draw = bytes[headerOffset + 4];
            ss << "as_draw: " << as_draw << std::endl;
            const auto idOffset = 5 + dataOffset + datalen;
            ss << "idOffset: " << idOffset << std::endl;
            ss << "id: ";
            auto p = reinterpret_cast<const uint16_t*>(&m_data[idOffset]);
            for (auto i  = 0U; i < std::min(128U, idLen); i++) {
                const auto v = p[i];
                ss << static_cast<char>(v);
            }
            ss << '\n' << std::endl;
            const auto s =  ss.str();
            return s;
        }    
#endif
