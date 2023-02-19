#include <iterator>
#include <vector>
#include "gempyre.h"

namespace Gempyre {
class Data {
    public:
        template <class T> class iteratorT {
        public:
            using iterator_category = std::forward_iterator_tag; //could be upgraded, but I assume there is no need
            using value_type = T;
            using difference_type = void;
            using pointer = T*;
            using reference = T&;
            iteratorT(pointer data = nullptr) : m_data(data) {}
            iteratorT(const iteratorT& other) = default;
            iteratorT& operator=(const iteratorT& other) = default;
            bool operator==(const iteratorT& other) const  {return m_data == other.m_data;}
            bool operator!=(const iteratorT& other) const  {return m_data != other.m_data;}
            reference operator*() {return *m_data;}
            reference operator*() const {return *m_data;}
            pointer operator->() {return m_data;}
            value_type operator++() {++m_data ; return *m_data;}
            value_type operator++(int) {auto temp(m_data); ++m_data; return *temp;}
        private:
            pointer m_data;
        };
        using iterator = iteratorT<dataT>;
        using const_iterator = iteratorT<const dataT>;
    public:
        [[nodiscard]] dataT* data();
        [[nodiscard]] const dataT* data() const;
        [[nodiscard]] unsigned elements() const;
        [[nodiscard]] Data::iterator begin() {return data();}
        [[nodiscard]] Data::iterator end() {return data() + elements();}
        [[nodiscard]] const Data::const_iterator begin() const {return data();}
        [[nodiscard]] const Data::const_iterator end() const {return data() + elements();}
        [[nodiscard]] dataT& operator[](int index) {return (data()[index]);}
        [[nodiscard]] dataT operator[](int index) const {return (data()[index]);}
        [[nodiscard]] dataT* endPtr() {return data() + elements();}
        [[nodiscard]] const dataT* endPtr() const {return data() + elements();}
        void writeHeader(const std::vector<dataT>& header);
        [[nodiscard]] std::vector<dataT> header() const;
        [[nodiscard]] std::string owner() const;
        [[nodiscard]] DataPtr clone() const;
        [[nodiscard]] size_t size() const {return m_data.size() * sizeof(dataT);}
        virtual ~Data() = default;
        Data(size_t sz, dataT type, const std::string& owner, const std::vector<dataT>& header);
#ifdef GEMPYRE_IS_DEBUG
        std::string dump() const;
#endif
    private:
        std::tuple<const char*, size_t> payload() const;
        std::vector<dataT> m_data;
        friend class Element;
        friend class Ui;
    };
}