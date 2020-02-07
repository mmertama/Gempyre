#include "telex.h"
#include "telex_utils.h"

#include "telex_internal.h"

using namespace Telex;

Element::Element(Ui& ui, const std::string& id) : m_ui(&ui), m_id(id) {
    if(m_ui->m_elements.find(id) == m_ui->m_elements.end())
        m_ui->m_elements.emplace(std::make_pair(id, Ui::HandlerMap{}));
}

Element::Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent) : Element(ui, id) {
    ui.send(parent, "create", std::unordered_map<std::string, std::string>{{"new_id", m_id}, {"html_element", htmlElement}});
}


Element& Element::subscribe(const std::string& name, Handler handler, const std::vector<std::string>& properties, const std::chrono::milliseconds& throttle) {
    m_ui->m_elements[m_id].emplace(name, handler);
    m_ui->send(*this, "event", std::unordered_map<std::string, std::any>{
                   {"event", name}, {"properties", properties}, {"throttle", std::to_string(throttle.count())}});
    return *this;
}

Element& Element::setHTML(const std::string& htmlText) {
    m_ui->send(*this, "html", htmlText);
    return *this;
}

Element& Element::setAttribute(const std::string &attr, const std::string &value) {
    m_ui->send(*this, "attribute", std::unordered_map<std::string, std::string>{{"attribute", attr}, {"value", value}});
    return *this;
}

std::optional<Element::Attributes> Element::attributes() const {
    const auto attributes = m_ui->query<Element::Attributes>(m_id, "attributes");
    return m_ui->m_status == Ui::State::RUNNING ? attributes : std::nullopt;
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
            TelexUtils::toOr<int>(value->at("x")).value(),
            TelexUtils::toOr<int>(value->at("y")).value(),
            TelexUtils::toOr<int>(value->at("width")).value(),
            TelexUtils::toOr<int>(value->at("height")).value()};
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

void Element::send(const std::string& type, const std::any& data) {
    m_ui->send(*this, type, data);
}

template <typename T> T align(T a) {return (a + 3U) & ~3U;}

constexpr auto fixedDataSize = 4;
Data::Data(size_t sz, dataT type, const std::string& owner, const std::vector<dataT>& header) :
    m_data(sz + (fixedDataSize + header.size()) + align(owner.size())) {
        m_data[0] = type;
        m_data[1] = static_cast<dataT>(sz);
        m_data[2] = align(static_cast<dataT>(owner.size()));
        m_data[3] = static_cast<dataT>(header.size());
        std::copy(header.begin(), header.end(), end());
        auto idData = reinterpret_cast<uint16_t*>(end() + header.size());
        std::copy(owner.begin(), owner.end(), idData);
}

std::string Data::owner() const {
    std::string out;
    const auto pos = reinterpret_cast<const uint16_t*>(end() + m_data[3]);
    for(auto i = 0U; i < m_data[2]; i++) {
        const wchar_t c = pos[i];
        if(c == 0) //name is padded to alignement so there may be extra zeroes
            break;
        out += static_cast<std::string::value_type>(c);
    }
    return out;
}

std::vector<Telex::Data::dataT> Data::header() const {
    std::vector<dataT> out;
    std::copy(end(), end() + m_data[3], std::back_inserter(out));
    return out;
}

void Data::writeHeader(const std::vector<dataT>& header) {
     telex_utils_assert_x(header.size() == m_data[3], "Header sizes must match!");
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

