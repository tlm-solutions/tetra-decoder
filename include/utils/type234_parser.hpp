/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "utils/bit_vector.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>

template <std::size_t N> struct BitVectorElement {
  public:
    BitVectorElement() = delete;
    explicit BitVectorElement(BitVector& data)
        : x_(data.take<N>()){};

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator unsigned _BitInt(N)() const noexcept { return x_; }

  private:
    unsigned _BitInt(N) x_;
};

template <typename ElementIdentifier> struct Type34Element {
    BitVector unparsed_bits;
    unsigned repeated_elements = 1;
};

template <typename ElementIdentifier>
inline auto operator<<(std::ostream& stream, const Type34Element<ElementIdentifier>& element) -> std::ostream& {
    stream << "#" << element.repeated_elements << ": " << element.unparsed_bits;
    return stream;
};

template <typename ElementIdentifier> class Type234Parser {
  public:
    using Map = std::map<ElementIdentifier, Type34Element<ElementIdentifier>>;

    Type234Parser() = delete;

    Type234Parser(BitVector& data, std::set<ElementIdentifier> allowed_type3_elements,
                  std::set<ElementIdentifier> allowed_type4_elements)
        // Extract the O-bit
        : present_(static_cast<bool>(data.take<1>()))
        , allowed_type3_elements_(allowed_type3_elements)
        , allowed_type4_elements_(allowed_type4_elements){};

    template <typename T> auto parse_type2(BitVector& data) -> std::optional<T> {
        if (!present_) {
            return {};
        }

        std::optional<T> type2_element;
        // P-bit present?
        if (data.take<1>()) {
            type2_element = T(data);
        }
        return type2_element;
    }

    auto parse_type34(BitVector& data) -> Map {
        if (!present_) {
            return {};
        }

        Map elements;

        while (data.bits_left()) {
            // M-bit present?
            if (data.take<1>() == 0) {
                break;
            }
            auto element_identifier = ElementIdentifier(data.take<4>());
            auto length_indicator = data.take<11>();
            // Is this a type 3 element?
            if (allowed_type3_elements_.count(element_identifier)) {
                const auto element_data = data.take_vector(length_indicator);
                if (elements.count(element_identifier)) {
                    throw std::runtime_error("This element identifier already occured.");
                }
                elements[element_identifier] = Type34Element<ElementIdentifier>{.unparsed_bits = element_data};
                continue;
            }
            // Is this a type 4 element?
            if (allowed_type4_elements_.count(element_identifier)) {
                const auto repeated_elements = data.take<6>();
                const auto element_data = data.take_vector(length_indicator - 6);
                if (elements.count(element_identifier)) {
                    throw std::runtime_error("This element identifier already occured.");
                }
                elements[element_identifier] = Type34Element<ElementIdentifier>{.unparsed_bits = element_data,
                                                                                .repeated_elements = repeated_elements};
                continue;
            }
            // Is this element invalid?
            throw std::runtime_error("This element identifier is not allowed in the current parser config.");
        }

        return elements;
    }

  private:
    bool present_ = false;
    std::set<ElementIdentifier> allowed_type3_elements_;
    std::set<ElementIdentifier> allowed_type4_elements_;
};