//
// SyntaxNode.cpp
// Copyright (c) Charles Baker. All rights reserved.
//

#include "stdafx.hpp"
#include "SyntaxNode.hpp"
#include <math/vec2.ipp>
#include <math/vec3.ipp>
#include <math/mat4x4.ipp>
#include "assert.hpp"
#include <algorithm>

using std::equal;
using std::string;
using std::vector;
using std::shared_ptr;
using namespace reyes;

SyntaxNode::SyntaxNode()
: node_type_( SHADER_NODE_NULL ),
  lexeme_(),
  nodes_(),
  symbol_(),
  constant_index_( REGISTER_NULL ),
  expected_type_( TYPE_NULL ),
  original_type_( TYPE_NULL ),
  type_( TYPE_NULL ),
  expected_storage_( STORAGE_NULL ),
  original_storage_( STORAGE_NULL ),
  storage_( STORAGE_NULL ),
  instruction_( INSTRUCTION_NULL ),
  line_( 0 )
{
}

SyntaxNode::SyntaxNode( SyntaxNodeType node_type, int line )
: node_type_( node_type ),
  lexeme_(),
  nodes_(),
  symbol_(),
  constant_index_( REGISTER_NULL ),
  expected_type_( TYPE_NULL ),
  original_type_( TYPE_NULL ),
  type_( TYPE_NULL ),
  expected_storage_( STORAGE_NULL ),
  original_storage_( STORAGE_NULL ),
  storage_( STORAGE_NULL ),
  instruction_( INSTRUCTION_NULL ),
  line_( line )
{
}

SyntaxNode::SyntaxNode( SyntaxNodeType node_type, int line, const char* lexeme )
: node_type_( node_type ),
  lexeme_(),
  nodes_(),
  symbol_(),
  constant_index_( REGISTER_NULL ),
  expected_type_( TYPE_NULL ),
  original_type_( TYPE_NULL ),
  type_( TYPE_NULL ),
  expected_storage_( STORAGE_NULL ),
  original_storage_( STORAGE_NULL ),
  storage_( STORAGE_NULL ),
  instruction_( INSTRUCTION_NULL ),
  line_( line )
{
    if ( lexeme )
    {
        lexeme_ = lexeme;
    }
}

SyntaxNode::SyntaxNode( SyntaxNodeType node_type, int line, const std::string& lexeme )
: node_type_( node_type ),
  lexeme_( lexeme ),
  nodes_(),
  symbol_(),
  constant_index_( REGISTER_NULL ),
  expected_type_( TYPE_NULL ),
  original_type_( TYPE_NULL ),
  type_( TYPE_NULL ),
  expected_storage_( STORAGE_NULL ),
  original_storage_( STORAGE_NULL ),
  storage_( STORAGE_NULL ),
  instruction_( INSTRUCTION_NULL ),
  line_( line )
{
}

int SyntaxNode::line() const
{
    return line_;
}

void SyntaxNode::set_node_type( SyntaxNodeType node_type )
{
    node_type_ = node_type;
}

SyntaxNodeType SyntaxNode::node_type() const
{
    return node_type_;
}

void SyntaxNode::set_lexeme( const std::string& lexeme )
{
    lexeme_ = lexeme;
}

const std::string& SyntaxNode::lexeme() const
{
    return lexeme_;
}

float SyntaxNode::real() const
{
    REYES_ASSERT( node_type_ == SHADER_NODE_REAL || node_type_ == SHADER_NODE_INTEGER );
    return ::atof( lexeme_.c_str() );
}

int SyntaxNode::integer() const
{
    REYES_ASSERT( node_type_ == SHADER_NODE_INTEGER );
    return ::atoi( lexeme_.c_str() );
}

const std::string& SyntaxNode::string() const
{
    REYES_ASSERT( node_type_ == SHADER_NODE_STRING );
    return lexeme_;
}

math::vec3 SyntaxNode::vec3() const
{
    REYES_ASSERT( node_type_ == SHADER_NODE_TRIPLE );
    REYES_ASSERT( nodes_.size() == 3 );
    REYES_ASSERT( nodes_[0] && nodes_[1] && nodes_[2] );
    return math::vec3( node(0)->real(), node(1)->real(), node(2)->real() );
}

math::mat4x4 SyntaxNode::mat4x4() const
{
    REYES_ASSERT( node_type_ == SHADER_NODE_SIXTEENTUPLE );
    REYES_ASSERT( nodes_.size() == 16 );
    REYES_ASSERT( nodes_[0] && nodes_[1] && nodes_[2] && nodes_[3] );
    REYES_ASSERT( nodes_[4] && nodes_[5] && nodes_[6] && nodes_[7] );
    REYES_ASSERT( nodes_[8] && nodes_[9] && nodes_[10] && nodes_[11] );
    REYES_ASSERT( nodes_[12] && nodes_[13] && nodes_[14] && nodes_[15] );

    return math::mat4x4(
        node(0)->real(), node(1)->real(), node(2)->real(), node(3)->real(),
        node(4)->real(), node(5)->real(), node(6)->real(), node(7)->real(),
        node(8)->real(), node(9)->real(), node(10)->real(), node(11)->real(),
        node(12)->real(), node(13)->real(), node(14)->real(), node(15)->real()
    );    
}

void SyntaxNode::add_node( std::shared_ptr<SyntaxNode> node )
{
    REYES_ASSERT( node );
    nodes_.push_back( node );
}

void SyntaxNode::add_node_at_front( std::shared_ptr<SyntaxNode> node )
{
    REYES_ASSERT( node );
    nodes_.insert( nodes_.begin(), node );
}

void SyntaxNode::add_nodes_at_end( const std::vector<std::shared_ptr<SyntaxNode>>::const_iterator begin, const std::vector<std::shared_ptr<SyntaxNode>>::const_iterator end )
{
    nodes_.insert( nodes_.end(), begin, end );
}

SyntaxNode* SyntaxNode::node( int index ) const
{
    REYES_ASSERT( index >= 0 && index < int(nodes_.size()) );
    REYES_ASSERT( nodes_[index] );
    return nodes_[index].get();
}

const std::vector<std::shared_ptr<SyntaxNode>>& SyntaxNode::get_nodes() const
{
    return nodes_;
}

void SyntaxNode::set_symbol( std::shared_ptr<Symbol> symbol )
{
    symbol_ = symbol;
}

std::shared_ptr<Symbol> SyntaxNode::get_symbol() const
{
    return symbol_;
}

void SyntaxNode::set_constant_index( int index )
{
    constant_index_ = index;
}

int SyntaxNode::get_constant_index() const
{
    return constant_index_;
}

void SyntaxNode::set_expected_type( ValueType type )
{
    expected_type_ = type;
}

ValueType SyntaxNode::get_expected_type() const
{
    return expected_type_;
}

void SyntaxNode::set_type( ValueType type )
{
    REYES_ASSERT( type >= TYPE_NULL && type < TYPE_COUNT );
    type_ = type;
    original_type_ = type;
}

ValueType SyntaxNode::get_type() const
{
    return type_;
}

void SyntaxNode::set_type_for_conversion( ValueType type )
{
    REYES_ASSERT( type_ > TYPE_NULL && type_ < TYPE_COUNT );
    REYES_ASSERT( type > TYPE_NULL && type < TYPE_COUNT );
    original_type_ = type_;
    type_ = type;
}

ValueType SyntaxNode::get_original_type() const
{
    return original_type_;
}

void SyntaxNode::set_expected_storage( ValueStorage storage )
{
    expected_storage_ = storage;
}

ValueStorage SyntaxNode::get_expected_storage() const
{
    return expected_storage_;
}

void SyntaxNode::set_storage( ValueStorage storage )
{
    REYES_ASSERT( storage >= STORAGE_NULL && storage < STORAGE_COUNT );
    storage_ = storage;
}

ValueStorage SyntaxNode::get_storage() const
{
    return storage_;
}

void SyntaxNode::set_storage_for_promotion( ValueStorage storage )
{
    REYES_ASSERT( storage_ >= STORAGE_NULL && storage_ < STORAGE_COUNT );
    REYES_ASSERT( storage > STORAGE_NULL && storage < STORAGE_COUNT );
    original_storage_ = storage_;
    storage_ = storage;
}

ValueStorage SyntaxNode::get_original_storage() const
{
    return original_storage_;
}

void SyntaxNode::set_instruction( Instruction instruction )
{
    instruction_ = instruction;
}

Instruction SyntaxNode::get_instruction() const
{
    return instruction_;
}

namespace 
{

struct indirect_equal
{
    bool operator()( const shared_ptr<SyntaxNode>& lhs, const shared_ptr<SyntaxNode>& rhs ) const
    {
        REYES_ASSERT( lhs );
        REYES_ASSERT( rhs );
        return *lhs == *rhs;
    }
};

}

bool SyntaxNode::operator==( const SyntaxNode& node ) const
{
    return node_type_ == node.node_type_
        && lexeme_ == node.lexeme_
        && nodes_.size() == node.nodes_.size()
        && equal( nodes_.begin(), nodes_.end(), node.nodes_.begin(), indirect_equal() )
    ;
}