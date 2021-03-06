#ifndef BOXBOUNDEDDISJUNCTSDOMAINDROPOUTINSTANTIATION_H
#define BOXBOUNDEDDISJUNCTSDOMAINDROPOUTINSTANTIATION_H

#include "BoxBoundedDisjunctsDomainTemplate.hpp"
#include "BoxStateDomainDropoutInstantiation.h"


class BoxBoundedDisjunctsDomainDropoutInstantiation : public BoxBoundedDisjunctsDomainTemplate<TrainingReferencesWithDropout, PredicateAbstraction, PosteriorDistributionAbstraction, double> {
public:
    using BoxBoundedDisjunctsDomainTemplate::BoxBoundedDisjunctsDomainTemplate; // Inherit constructors
    double joinPrecisionLoss(const Types::Single &e1, const Types::Single &e2) const;
};


#endif
