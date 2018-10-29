#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>

enum RuleKind {
  RuleKind_Addition,
  RuleKind_Subtraction,
  RuleKind_Multiplication,
  RuleKind_Division,
  RuleKind_Assignment,
  RuleKind_Equality,
  RuleKind_Exponential,
};

extern std::vector<struct Rule *> rules;

struct Rule {
  struct Operation {
    std::vector<int> array_dimensions;
    RuleKind rule;
    int operation_i;
    int alignment;

    Operation( RuleKind ty, int oi ) : rule(ty), operation_i(oi) {}

    Operatiorn &add_array_dimension( int array_dim ) {
      array_dimensions.push_back( array_dim );
      return *this;
    }

    
  };
};
