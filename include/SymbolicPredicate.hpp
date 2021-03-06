#ifndef SYMBOLICPREDICATE_HPP
#define SYMBOLICPREDICATE_HPP

/**
 * This file defines a three-value logic symbolic predicate
 * for numerical features---where a single predicate's threshold
 * could be from a range of values.
 * It largely mirrors Predicate.hpp
 *
 * We use optional<bool> for three-valued logic.
 */

#include "Feature.hpp"
#include <functional> // for std::hash
#include <optional>


class SymbolicPredicate {
private:
    unsigned int feature_index;
    FeatureType feature_type;
    // For when feature_type == FeatureType::NUMERIC.
    // Checks lb <= x < ub
    float threshold_lb, threshold_ub;

public:
    SymbolicPredicate(int feature_index); // Sets feature_type = FeatureType::BOOLEAN
    SymbolicPredicate(int feature_index, float threshold_lb, float threshold_ub); // Sets feature_Type = FeatureType::NUMERIC

    std::optional<bool> evaluate(const FeatureVector &x) const; // Does not check bounds

    // Our abstract transformers would like to be able to hash these objects, etc
    bool operator ==(const SymbolicPredicate &right) const;
    size_t hash() const;
};


// And a wrapper for SymbolicPredicate::hash so we can conveniently use std::unordered_map
// XXX hashing these may no longer be used
struct hash_SymbolicPredicate {
    size_t operator()(const SymbolicPredicate &p) const {
        return p.hash();
    }
};


/**
 * Member functions below
 */

inline SymbolicPredicate::SymbolicPredicate(int feature_index) {
    this->feature_index = feature_index;
    feature_type = FeatureType::BOOLEAN;
}

inline SymbolicPredicate::SymbolicPredicate(int feature_index, float threshold_lb, float threshold_ub) {
    this->feature_index = feature_index;
    this->threshold_lb = threshold_lb;
    this->threshold_ub = threshold_ub;
    feature_type = FeatureType::NUMERIC;
}

inline std::optional<bool> SymbolicPredicate::evaluate(const FeatureVector &x) const {
    // XXX does no check to ensure this->feature_type is consistent with x[feature_index]'s
    switch(feature_type) {
        case FeatureType::BOOLEAN:
            return x[feature_index].getBooleanValue();
        case FeatureType::NUMERIC:
            if(x[feature_index].getNumericValue() <= threshold_lb) {
                return true;
            } else if(x[feature_index].getNumericValue() < threshold_ub) {
                return {};
            } else {
                return false;
            }
        default:
            // XXX this shouldn't happen---it's here to suppress a warning.
            // If adding new FeatureTypes, make sure to add remaining cases.
            return false;
    }
}

inline bool SymbolicPredicate::operator ==(const SymbolicPredicate &right) const {
    if(this->feature_index != right.feature_index) {
        return false;
    }
    if(this->feature_type != right.feature_type) {
        return false;
    }
    if(this->feature_type == FeatureType::NUMERIC) {
        if(this->threshold_lb != right.threshold_lb ||
                this->threshold_ub != right.threshold_ub) {
            return false;
        }
    }
    return true;
}

inline size_t SymbolicPredicate::hash() const {
    if(feature_type == FeatureType::NUMERIC) {
        return std::hash<unsigned int>{}(feature_index)
            ^ std::hash<float>{}(threshold_lb)
            ^ std::hash<float>{}(threshold_ub);
    } else { // XXX assuming FeatureType::BOOLEAN
        return std::hash<unsigned int>{}(feature_index);
    }
}

#endif
