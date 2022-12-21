#include "gempyre.h"
#include "gempyre_utils.h"
#include "core.h"

#include <random>
#include <chrono>


using namespace Gempyre;

const std::string Element::generateId(const std::string& prefix) {
    const auto seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution('a', 'z');
    std::string name = prefix + "_";
    for(int i = 0; i < 8; i++) {
        name += static_cast<char>(distribution(generator));
    }
    return name;
}

Element::Element(Ui& ui, const std::string& id) : m_ui(&ui), m_id(id) {
    if(m_ui->m_elements.find(id) == m_ui->m_elements.end())
        m_ui->m_elements.emplace(std::make_pair(id, Ui::HandlerMap{}));
}

Element::Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent) : Element(ui, id) {
    ui.send(parent, "create", std::unordered_map<std::string, std::string>{{"new_id", m_id}, {"html_element", htmlElement}});
}

Element::Element(Ui& ui, const std::string& htmlElement, const Element& parent) : Element(ui, generateId("__element")) {
    ui.send(parent, "create", std::unordered_map<std::string, std::string>{{"new_id", m_id}, {"html_element", htmlElement}});
}


Element& Element::subscribe(const std::string& name, std::function<void(const Event&)> handler, const std::vector<std::string>& properties, const std::chrono::milliseconds& throttle) {
    m_ui->m_elements[m_id].emplace(name, [handler](const Ui::Event& event) {
        std::unordered_map<std::string, std::string> property_map;
        for(const auto& [k, v] : event.properties)
            property_map.emplace(k, std::any_cast<std::string>(v));
        Gempyre::Event ev{event.element, std::move(property_map)};
        handler(ev);
    });
    m_ui->send(*this, "event", std::unordered_map<std::string, std::any>{
                   {"event", name}, {"properties", properties}, {"throttle", std::to_string(throttle.count())}});
    return *this;
}

Element& Element::set_html(const std::string& htmlText) {
    m_ui->send(*this, "html", htmlText);
    return *this;
}

Element& Element::set_attribute(const std::string &attr, const std::string &value) {
    m_ui->send(*this, "set_attribute", std::unordered_map<std::string, std::string>{{"attribute", attr}, {"value", value}});
    return *this;
}

Element& Element::remove_attribute(const std::string &attr) {
    m_ui->send(*this, "remove_attribute", std::unordered_map<std::string, std::string>{{"attribute", attr}});
    return *this;
}

Element& Element::set_style(const std::string &styleName, const std::string &value) {
    m_ui->send(*this, "set_style", std::unordered_map<std::string, std::string>{{"style", styleName}, {"value", value}});
    return *this;
}

Element& Element::removeStyle(const std::string &styleName) {
    m_ui->send(*this, "remove_style", std::unordered_map<std::string, std::string>{{"style", styleName}});
    return *this;
}

std::optional<Element::Attributes> Element::attributes() const {
    const auto attributes = m_ui->query<Element::Attributes>(m_id, "attributes");
    return m_ui->m_status == Ui::State::RUNNING ? attributes : std::nullopt;
}

std::optional<Element::Values> Element::styles(const std::vector<std::string>& keys) const {
    const auto styles = m_ui->query<Element::Values>(m_id, "styles", keys);
    return m_ui->m_status == Ui::State::RUNNING ? styles : std::nullopt;
}

std::optional<Element::Values> Element::values() const {
    const auto value = m_ui->query<Element::Values>(m_id, "value");
    return m_ui->m_status == Ui::State::RUNNING ? value : std::nullopt;
}

std::optional<std::string> Element::html() const {
    const auto value = m_ui->query<std::string>(m_id, "innerHTML");
    return m_ui->m_status == Ui::State::RUNNING ? value : std::nullopt;
}

std::optional<std::string> Element::type() const {
    const auto value = m_ui->query<std::string>(m_id, "element_type");
    return m_ui->m_status == Ui::State::RUNNING ? value : std::nullopt;
}

std::optional<Element::Rect> Element::rect() const {
    const auto value = m_ui->query<std::unordered_map<std::string, std::string>>(m_id, "bounding_rect");
    if(m_ui->m_status == Ui::State::RUNNING && value.has_value()) {
        return Rect{
            GempyreUtils::toOr<int>(value->at("x")).value(),
            GempyreUtils::toOr<int>(value->at("y")).value(),
            GempyreUtils::toOr<int>(value->at("width")).value(),
            GempyreUtils::toOr<int>(value->at("height")).value()};
    }
    return std::nullopt;
}

std::optional<Element::Elements> Element::children() const {
    Element::Elements childArray;
    const auto childIds = m_ui->query<std::vector<std::string>>(m_id, "children");
    if(!childIds.has_value())
        return std::nullopt;
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*m_ui, cid));
    }
    return m_ui->m_status == Ui::State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

void Element::remove() {
    m_ui->send(*this, "remove", m_id);
}

void Element::send(const DataPtr& data) {
    m_ui->send(data);
}

void Element::send(const std::string& type, const std::any& data, bool unique) {
    m_ui->send(*this, type, data, unique);
}


template <typename T> T align(T a) {return (a + 3U) & ~3U;}

constexpr auto fixedDataSize = 4;
Data::Data(size_t sz, dataT type, const std::string& owner, const std::vector<dataT>& header) :
    m_data(sz + (fixedDataSize + header.size()) + align(owner.size())) {
        m_data[0] = type;
        m_data[1] = static_cast<dataT>(sz);
        m_data[2] = align(static_cast<dataT>(owner.size()));
        m_data[3] = static_cast<dataT>(header.size());
        std::copy(header.begin(), header.end(), endPtr());
        auto idData = reinterpret_cast<uint16_t*>(endPtr() + header.size());
        std::copy(owner.begin(), owner.end(), idData);
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

std::vector<Gempyre::Data::dataT> Data::header() const {
    std::vector<dataT> out;
    std::copy(endPtr(), endPtr() + m_data[3], std::back_inserter(out));
    return out;
}

void Data::writeHeader(const std::vector<dataT>& header) {
     gempyre_utils_assert_x(header.size() == m_data[3], "Header sizes must match!");
     std::copy(header.begin(), header.end(), end());
}

std::tuple<const char*, size_t> Data::payload() const {
    return {reinterpret_cast<const char*>(m_data.data()), m_data.size() * sizeof(dataT)};
}

Data::dataT* Data::data() {
    return &m_data.data()[fixedDataSize];
}

const Data::dataT* Data::data() const {
    return &m_data.data()[fixedDataSize];
}

size_t Data::size() const {
    return m_data[1];
}

DataPtr Data::clone() const {
    auto ptr = std::shared_ptr<Data>(new Data(size(), m_data[0], owner(), header()));
    std::copy(begin(), end(), ptr->begin());
    return ptr;
}

