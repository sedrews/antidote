#include "DropoutSemantics.h"
#include "ASTNode.h"
#include "ConcreteSemantics.h"
#include "data_common.h"
#include "information_math.h"
#include "Interval.h"
#include <map>
#include <utility>
#include <vector>

/**
 * DropoutSet member functions
 */

Interval<int> DropoutSet::countOnes() {
    int count = 0;
    for(int i = 0; i < data.size(); i++) {
        if(classificationBit(i)) {
            count++;
        }
    }
    return Interval<int>((count - num_dropout >= 0 ? count - num_dropout : 0), count);
}

std::pair<DropoutCounts, DropoutCounts> DropoutSet::splitCounts(const BitVectorPredicate &phi) {
    std::pair<DropoutCounts, DropoutCounts> ret({0, 0, 0}, {0, 0, 0});
    DropoutCounts *d_ptr;
    int *count_ptr;
    for(int i = 0; i < data.size(); i++) {
        d_ptr = phi.evaluate(getRow(i).first) ? &(ret.second) : &(ret.first);
        count_ptr = classificationBit(i) ? &(d_ptr->pos) : &(d_ptr->neg);
        *count_ptr += 1;
    }
    ret.first.num_dropout = (num_dropout < ret.first.pos + ret.first.neg ? num_dropout : ret.first.pos + ret.first.neg);
    ret.second.num_dropout = (num_dropout < ret.second.pos + ret.second.neg ? num_dropout : ret.second.pos + ret.second.neg);
    return ret;
}

DropoutSet* DropoutSet::pureSets(bool classification) {
    DataReferences<DataRow> datacopy = data;
    int num_removed = 0;
    for(int i = 0; i < datacopy.size(); i++) {
        if(datacopy[i].second != classification) {
            datacopy.remove(i);
            num_removed++;
            i--;
        }
    }
    if(num_removed <= num_dropout) {
        return new DropoutSet(datacopy, num_dropout - num_removed);
    } else {
        return NULL;
    }
};

bool couldBeEmpty(const DropoutCounts &counts) {
    return counts.pos + counts.neg <= counts.num_dropout;
}

bool mustBeEmpty(const DropoutCounts &counts) {
    return counts.pos + counts.neg == 0;
}

PredicatePointers DropoutSet::bestSplit(const PredicateSet *predicates) {
    std::map<BitVectorPredicate*, std::pair<DropoutCounts, DropoutCounts>> counts;
    PredicatePointers forall_nontrivial, exists_nontrivial;

    for(PredicateSet::iterator i = predicates->begin(); i != predicates->end(); i++) {
        counts.insert(std::make_pair(&(*i), splitCounts(*i)));
        if(!couldBeEmpty(counts[&(*i)].first) && !couldBeEmpty(counts[&(*i)].second)) {
            forall_nontrivial.push_back(&(*i));
        }
        if(!mustBeEmpty(counts[&(*i)].first) && !mustBeEmpty(counts[&(*i)].second)) {
            exists_nontrivial.push_back(&(*i));
        }
    }
    
    if(forall_nontrivial.size() == 0) {
        exists_nontrivial.push_back(NULL);
        return exists_nontrivial;
    }

    std::map<BitVectorPredicate*, Interval<double>> scores;
    double min_upper_bound;
    for(PredicatePointers::iterator i = forall_nontrivial.begin(); i != forall_nontrivial.end(); i++) {
        scores.insert(std::make_pair(*i, jointImpurity(std::make_pair(counts[*i].first.pos, counts[*i].first.neg),
                                                       counts[*i].first.num_dropout,
                                                       std::make_pair(counts[*i].second.pos, counts[*i].second.neg),
                                                       counts[*i].second.num_dropout)));
        if(i == forall_nontrivial.begin() || min_upper_bound > scores[*i].get_upper_bound()) {
            min_upper_bound = scores[*i].get_upper_bound();
        }
    }

    PredicatePointers ret;
    for(PredicatePointers::iterator i = exists_nontrivial.begin(); i != exists_nontrivial.end(); i++) {
        if(scores[*i].get_lower_bound() <= min_upper_bound) {
            ret.push_back(*i);
        }
    }
    return ret;
}

void DropoutSet::filter(const BitVectorPredicate &phi, bool mode) {
    bool remove, result;
    for(int i = 0; i < data.size(); i++) {
        result = phi.evaluate(data[i].first);
        remove = (mode != result);
        if(remove) {
            data.remove(i);
            i--;
        }
    }
    if(num_dropout > data.size()) {
        num_dropout = data.size();
    }
}

Interval<double> DropoutSet::summary() {
    if(num_dropout == data.size()) {
        return Interval<double>(0, 1);
    }

    Interval<int> c1 = countOnes();
    Interval<double> c1d(c1.get_lower_bound(), c1.get_upper_bound());
    Interval<int> t = size();
    Interval<double> td(t.get_lower_bound(), t.get_upper_bound());
    return c1d / td;
}

// In this (and other places), we're assuming an invariant that data.size() >= num_dropout
Interval<int> DropoutSet::size() {
    return Interval<int>(data.size() - num_dropout, data.size());
}

DropoutSet DropoutSet::join(const DropoutSet &e1, const DropoutSet &e2) {
    DataReferences<DataRow> d = DataReferences<DataRow>::set_union(e1.data, e2.data);
    int n1 = d.size() - e2.data.size() + e2.num_dropout; // Note |(T1 U T2) \ T1| = |T2 \ T1|
    int n2 = d.size() - e1.data.size() + e1.num_dropout;
    return DropoutSet(d, (n1 > n2 ? n1 : n2));
}

DropoutSet DropoutSet::join(const vector<DropoutSet> &elements) {
    DropoutSet ret;
    for(vector<DropoutSet>::const_iterator i = elements.begin(); i != elements.end(); i++) {
        ret = DropoutSet::join(ret, *i);
    }
    return ret;
}

/**
 * AbstractState member functions
 */

AbstractState join(const AbstractState &e1, const AbstractState &e2) {
    // TODO
}

AbstractState AbstractState::join(const vector<AbstractState> &elements) {
    AbstractState ret;
    for(vector<AbstractState>::const_iterator i = elements.begin(); i != elements.end(); i++) {
        ret = AbstractState::join(ret, *i);
    }
    return ret;
}

/**
 * DropoutSemantics member functions
 */

DropoutSemantics::DropoutSemantics() {
    // TODO
}

Interval<double> DropoutSemantics::execute(const Input test_input, DropoutSet *training_set, const PredicateSet *predicates, const ProgramNode *program) {
    this->test_input = test_input;
    current_state.training_set = *training_set;
    current_state.phis.clear();
    this->predicates = predicates;
    program->accept(*this);
    return return_value;
}

void DropoutSemantics::visit(const ProgramNode &node) {
    node.get_left_child()->accept(*this);
    node.get_right_child()->accept(*this);
}

void DropoutSemantics::visit(const SequenceNode &node) {
    node.get_left_child()->accept(*this);
    node.get_right_child()->accept(*this);
}

void DropoutSemantics::visit(const ITEImpurityNode &node) {
    DropoutSet *pure0, *pure1;
    std::vector<AbstractState> joins(0);
    AbstractState backup;

    pure0 = current_state.training_set.pureSets(false);
    pure1 = current_state.training_set.pureSets(true);

    if(pure0 != NULL) {
        backup = current_state;
        current_state.training_set = *pure0;
        node.get_left_child()->accept(*this);
        joins.push_back(current_state);
        current_state = backup;
    }
    
    if(pure1 != NULL) {
        backup = current_state;
        current_state.training_set = *pure1;
        node.get_left_child()->accept(*this);
        joins.push_back(current_state);
        current_state = backup;
    }

    node.get_right_child()->accept(*this);
    joins.push_back(current_state);

    current_state = AbstractState::join(joins);

    delete pure0;
    delete pure1;
}

void DropoutSemantics::visit(const ITENoPhiNode &node) {
    std::vector<AbstractState> joins(0);
    PredicatePointers::iterator i;
    bool contains_bot = false;
    AbstractState backup;

    for(i = current_state.phis.begin(); i != current_state.phis.end(); i++) {
        if((*i) == NULL) {
            contains_bot = true;
            break;
        }
    }

    if(contains_bot) {
        current_state.phis.erase(i);
    }

    backup = current_state;
    node.get_right_child()->accept(*this);
    joins.push_back(current_state);
    current_state = backup;

    if(contains_bot) {
        backup = current_state;
        current_state.phis = PredicatePointers({NULL});
        node.get_left_child()->accept(*this);
        joins.push_back(current_state);
        current_state = backup;
    }

    current_state = AbstractState::join(joins);
}

void DropoutSemantics::visit(const BestSplitNode &node) {
    current_state.phis = current_state.training_set.bestSplit(predicates);
}

void DropoutSemantics::visit(const SummaryNode &node) {
    current_state.posterior = current_state.training_set.summary();
}

void DropoutSemantics::visit(const UsePhiSequenceNode &node) {
    node.get_left_child()->accept(*this);
    node.get_right_child()->accept(*this);
}

void DropoutSemantics::visit(const ITEModelsNode &node) {
    std::vector<AbstractState> joins(0);
    AbstractState backup;
    PredicatePointers pos(0), neg(0);

    for(PredicatePointers::iterator i = current_state.phis.begin(); i != current_state.phis.end(); i++) {
        if((*i)->evaluate(test_input)) {
            pos.push_back(*i);
        } else {
            neg.push_back(*i);
        }
    }

    if(pos.size() > 0) {
        backup = current_state;
        current_state.phis = pos;
        node.get_left_child()->accept(*this);
        joins.push_back(current_state);
        current_state = backup;
    }

    if(neg.size() > 0) {
        backup = current_state;
        current_state.phis = neg;
        node.get_right_child()->accept(*this);
        joins.push_back(current_state);
        current_state = backup;
    }

    current_state = AbstractState::join(joins);
}

void DropoutSemantics::visit(const FilterNode &node) {
    std::vector<DropoutSet> joins(0);
    DropoutSet temp;
    for(PredicatePointers::iterator i = current_state.phis.begin(); i != current_state.phis.end(); i++) {
        temp = current_state.training_set;
        temp.filter(**i, node.get_mode());
        joins.push_back(temp);
    }
    current_state.training_set = DropoutSet::join(joins);
}

void DropoutSemantics::visit(const ReturnNode &node) {
    // Grammar enforces only a single return statement at the end, so we need not join
    return_value = current_state.posterior;
}