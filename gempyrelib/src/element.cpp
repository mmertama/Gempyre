#include "gempyre.h"
#include "gempyre_utils.h"
#include "core.h"
#include "data.h"

#include <random>
#include <chrono>
#include <charconv>

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

using namespace Gempyre;

const std::string Element::generateId(const std::string& prefix) {
    const auto seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution('a', 'z');
    std::string name = prefix + "_";
    for(int i = 0; i < 4; i++) {
        name += static_cast<char>(distribution(generator));
    }
    char seed_str[216] = { 0 };
    std::to_chars(std::begin(seed_str), std::end(seed_str), seed, 32);
    name += seed_str;
    return name;
}

Element::Element(Ui& ui, const std::string& id) : m_ui(&ui), m_id(id) {
    m_ui->ref().ensure_element_exists(id);
}

Element::Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent) : Element(ui, id) {
    ref().send(parent, "create",
        "new_id", m_id,
        "html_element", htmlElement);
}

Element::Element(Ui& ui, const std::string& htmlElement, const Element& parent) : Element(ui, generateId("__element")) {
    ref().send(parent, "create",
        "new_id", m_id,
        "html_element", htmlElement);
}


Element& Element::subscribe(const std::string& name, const SubscribeFunction& handler, const std::vector<std::string>& properties, const std::chrono::milliseconds& throttle) {
   
    ref().add_handler(m_id, name, handler);
    ref().send(*this, "event",
        "event", name,
        "properties", properties,
        "throttle", std::to_string(throttle.count()));
    return *this;
}

Element& Element::set_html(const std::string& htmlText) {
    ref().send(*this, "html", htmlText);
    return *this;
}

Element& Element::set_attribute(const std::string &attr, const std::string &value) {
    ref().send(*this, "set_attribute", 
        "attribute", attr,
        "value", value);
    return *this;
}

Element& Element::set_attribute(const std::string &attr) {
    ref().send(*this, "set_attribute", 
        "attribute", attr,
        "value", "true");
    return *this;
}
/*
using AttrValueType = std::variant<bool, int, unsigned, float, double>;

template <typename T>
void Element::send(Element& el, const std::string& attr, const T& value) {
    el.ref().send(el, "set_attribute",  "attribute", attr, "value", value);
}

Element& Element::set_attribute(const std::string& attr, const AttrValueType& value) {
    std::visit(overloaded {
        [&, this](const bool& v) {send(*this, attr, v);},
        [&, this](const int& v) {send(*this, attr, v);},
        [&, this](const double& v) {send(*this, attr, v);},
        [&, this](const float& v) {send(*this, attr, v);},
        [&, this](const unsigned& v) {send(*this, attr, v);},
    }, value);
    return *this;
}
*/

Element& Element::remove_attribute(const std::string &attr) {
    ref().send(*this, "remove_attribute",
        "attribute", attr);
    return *this;
}

Element& Element::set_style(const std::string &styleName, const std::string &value) {
    ref().send(*this, "set_style",
        "style", styleName,
        "value", value);
    return *this;
}

#if 0
Element& Element::removeStyle(const std::string &styleName) {
    ref().send(*this, "remove_style",
        "style", styleName);
    return *this;
}
#endif

std::optional<Element::Attributes> Element::attributes() const {
    const auto attributes = m_ui->ref().query<Element::Attributes>(m_id, "attributes");
    return ref() == State::RUNNING ? attributes : std::nullopt;
}

std::optional<Element::Values> Element::styles(const std::vector<std::string>& keys) const {
    const auto styles = m_ui->ref().query<Element::Values>(m_id, "styles", keys);
    return ref() == State::RUNNING ? styles : std::nullopt;
}

std::optional<Element::Values> Element::values() const {
    const auto value = m_ui->ref().query<Element::Values>(m_id, "value");
    return ref() == State::RUNNING ? value : std::nullopt;
}

std::optional<std::string> Element::html() const {
    const auto value = m_ui->ref().query<std::string>(m_id, "innerHTML");
    return ref() == State::RUNNING ? value : std::nullopt;
}

std::optional<std::string> Element::type() const {
    const auto value = m_ui->ref().query<std::string>(m_id, "element_type");
    return ref() == State::RUNNING ? value : std::nullopt;
}

std::optional<Element::Rect> Element::rect() const {
    const auto value = m_ui->ref().query<std::unordered_map<std::string, std::string>>(m_id, "bounding_rect");
    if(ref() == State::RUNNING && value.has_value()) {
        assert(value->size() >= 4);
        assert(value->find("x") != value->end());
        assert(value->find("y") != value->end());
        assert(value->find("width") != value->end());
        assert(value->find("height") != value->end());
        return Rect{
            GempyreUtils::parse<int>(value->at("x")).value(),
            GempyreUtils::parse<int>(value->at("y")).value(),
            GempyreUtils::parse<int>(value->at("width")).value(),
            GempyreUtils::parse<int>(value->at("height")).value()};
    }
    return std::nullopt;
}

std::optional<Element::Elements> Element::children() const {
    Element::Elements childArray;
    const auto childIds = m_ui->ref().query<std::vector<std::string>>(m_id, "children");
    if(!childIds.has_value())
        return std::nullopt;
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*m_ui, cid));
    }
    return m_ui->ref() == State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

void Element::remove() {
    ref().send(*this, "remove", m_id);
}


std::optional<Element> Element::parent() const {
    if(m_id == m_ui->root().m_id) {
        return std::nullopt;
    }
    const auto pid = m_ui->ref().query<std::string>(m_id, "parent");
    if(!pid || pid->empty())
        return std::nullopt;
    if(*pid == ": :") // see comment in JS
        return m_ui->root();
    return Gempyre::Element(*m_ui, *pid);
} 
 

Element::~Element() {}


const GempyreInternal& Element::ref() const {return m_ui->ref();}

GempyreInternal& Element::ref() {return m_ui->ref();}    

template <typename T> T align(T a) {return (a + 3U) & ~3U;}

constexpr auto fixedDataSize = 4;

// all Data is indexed so send can keep their order in broadcaster
static unsigned g_index_couter = 0;

Data::Data(size_t sz, dataT type, const std::string& owner, const std::vector<dataT>& header) :
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