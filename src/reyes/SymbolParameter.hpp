#ifndef REYES_SYMBOLPARAMETER_HPP_INCLUDED
#define REYES_SYMBOLPARAMETER_HPP_INCLUDED

#include "ValueType.hpp"
#include "ValueStorage.hpp"

namespace sweet
{

namespace reyes
{

/**
// The type and storage of a parameter used to implement overloaded functions 
// and also to provide type checking, conversion, and promotion for parameters
// passed to functions.
*/
class SymbolParameter
{
    ValueType type_;
    ValueStorage storage_;
    
public:
    SymbolParameter( ValueType type, ValueStorage storage );
    ValueType type() const;
    ValueStorage storage() const;
    bool matches( ValueType type, ValueStorage storage ) const;
};

}

}

#endif
