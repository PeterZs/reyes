//
// ShaderParser.cpp
// Copyright (c) Charles Baker. All rights reserved.
//

#include "stdafx.hpp"
#include "ShaderParser.hpp"
#include "ErrorCode.hpp"
#include "SyntaxNode.hpp"
#include "SymbolTable.hpp"
#include "Symbol.hpp"
#include "ErrorPolicy.hpp"
#include <lalr/Parser.hpp>
#include <lalr/PositionIterator.hpp>
#include "assert.hpp"
#include <fstream>
#include <iterator>
#include <functional>
#include <map>
#include <string>

using std::map;
using std::find;
using std::string;
using std::vector;
using std::istream_iterator;
using std::bind;
using std::shared_ptr;
using namespace std::placeholders;
using namespace reyes;

template <class Iterator>
class ShaderParserContext : public lalr::ErrorPolicy
{
    SymbolTable& symbol_table_;
    reyes::ErrorPolicy* error_policy_;
    const lalr::Parser<lalr::PositionIterator<Iterator>, shared_ptr<SyntaxNode>, char>* parser_;
    int solar_and_illuminate_statements_;
    int errors_;

public:
    ShaderParserContext( SymbolTable& symbol_table, reyes::ErrorPolicy* error_policy )
    : lalr::ErrorPolicy(),
      symbol_table_( symbol_table ),
      error_policy_( error_policy ),
      parser_( NULL ),
      solar_and_illuminate_statements_( 0 ),
      errors_( 0 )
    {    
    }

    void error( int line, const char* format, va_list args )
    {       
        ++errors_;
        if ( error_policy_ )
        {
            char message [1024];
            vsnprintf( message, sizeof(message), format, args );
            error_policy_->error( RENDER_ERROR_SYNTAX_ERROR, "(%d): %s", line, message );
        }
    }

    void error( int line, const char* format, ... )
    {       
        va_list args;
        va_start( args, format );
        error( line, format, args );
        va_end( args );
    }

    void lalr_error( int line, int /*error*/, const char* format, va_list args )
    {
        error( line, format, args );
        printf( "(%d): error: ", line );
        vprintf( format, args );
    }
    
    void lalr_vprintf( const char* format, va_list args )
    {
        vprintf( format, args );
    }
    
    void push_surface_scope()
    {
        // @todo
        //  Add remaining surface shader symbols to the symbol table when
        //  entering a surface shader scope in 
        //  ShaderParser::push_surface_scope().
        symbol_table_.push_scope();
        symbol_table_.add_symbols()
            ( "Cs", TYPE_COLOR )
            ( "Os", TYPE_COLOR )
            ( "P", TYPE_POINT )
            ( "N", TYPE_NORMAL )
            ( "I", TYPE_VECTOR )
            ( "s", TYPE_FLOAT )
            ( "t", TYPE_FLOAT )
            ( "Ci", TYPE_COLOR )
            ( "Oi", TYPE_COLOR )
        ;
    }
    
    void push_light_scope()
    {
        // @todo
        //  Add remaining light shader symbols to the symbol table when
        //  entering a light shader scope in 
        //  ShaderParser::push_light_scope().
        symbol_table_.push_scope();
        symbol_table_.add_symbols()
            ( "Ps", TYPE_POINT )
            ( "N", TYPE_NORMAL )
            ( "Cl", TYPE_COLOR )
            ( "Ol", TYPE_COLOR )
        ;
    }
    
    void push_volume_scope()
    {
        // @todo
        //  Add remaining volume shader symbols to the symbol table when
        //  entering a volume shader scope in 
        //  ShaderParser::push_volume_scope().
        symbol_table_.push_scope();
        symbol_table_.add_symbols()
            ( "P", TYPE_POINT )
            ( "I", TYPE_VECTOR )
            ( "Ci", TYPE_COLOR )
            ( "Oi", TYPE_COLOR )
        ;
    }
    
    void push_displacement_scope()
    {
        // @todo
        //  Add remaining displacement shader symbols to the symbol table when
        //  entering a displacement shader scope in 
        //  ShaderParser::push_displacement_scope().
        symbol_table_.push_scope();
        symbol_table_.add_symbols()
            ( "P", TYPE_POINT )
            ( "N", TYPE_NORMAL)
            ( "I", TYPE_VECTOR )
            ( "s", TYPE_FLOAT )
            ( "t", TYPE_FLOAT )
        ;        
    }
    
    void push_imager_scope()
    {
        // @todo
        //  Add remaining imager shader symbols to the symbol table when
        //  entering an imager shader scope in 
        //  ShaderParser::push_imager_scope().
        symbol_table_.push_scope();
        symbol_table_.add_symbols()
            ( "P", TYPE_POINT )
            ( "Ci", TYPE_COLOR )
            ( "Oi", TYPE_COLOR )
            ( "alpha", TYPE_FLOAT )
        ;        
    }
    
    void push_illuminance_scope()
    {
        symbol_table_.push_scope();
        symbol_table_.add_symbols()
            ( "L", TYPE_VECTOR )
            ( "Cl", TYPE_COLOR )
            ( "Ol", TYPE_COLOR )
        ;        
    }

    void push_illuminate_or_solar_scope()
    {
        ++solar_and_illuminate_statements_;
        
        symbol_table_.push_scope();
        symbol_table_.add_symbols()
            ( "L", TYPE_VECTOR )
        ;        
    }

    void pop_scope()
    {
        symbol_table_.pop_scope();
    }

    shared_ptr<Symbol> find_symbol( const std::string& identifier )
    {
        REYES_ASSERT( parser_ );
        REYES_ASSERT( !identifier.empty() );

        shared_ptr<Symbol> symbol = symbol_table_.find_symbol( identifier );
        if ( !symbol )
        {
            error( parser_->position().line(), "Unknown identifier '%s'", identifier.c_str() );
        }
        return symbol;
    }


    ValueStorage storage_from_syntax_node( shared_ptr<SyntaxNode> syntax_node, ValueStorage default_storage ) const
    {
        REYES_ASSERT( syntax_node );

        ValueStorage storage = STORAGE_NULL;
        switch( syntax_node->node_type() )
        {    
            case SHADER_NODE_NULL:
                storage = default_storage;
                break;
                
            case SHADER_NODE_UNIFORM:
                storage = STORAGE_UNIFORM;
                break;
                
            case SHADER_NODE_VARYING:
                storage = STORAGE_VARYING;
                break;
                
            default:
                REYES_ASSERT( false );
                storage = STORAGE_NULL;
                break;
        }
        return storage;
    }

    ValueType type_from_syntax_node( shared_ptr<SyntaxNode> syntax_node ) const
    {
        REYES_ASSERT( syntax_node );

        ValueType type = TYPE_NULL;
        switch ( syntax_node->node_type() )
        {
            case SHADER_NODE_FLOAT_TYPE:
                type = TYPE_FLOAT;
                break;
                
            case SHADER_NODE_STRING_TYPE:
                type = TYPE_STRING;
                break;
                
            case SHADER_NODE_COLOR_TYPE:
                type = TYPE_COLOR;
                break;
                
            case SHADER_NODE_POINT_TYPE:
                type = TYPE_POINT;
                break;
                
            case SHADER_NODE_VECTOR_TYPE:
                type = TYPE_VECTOR;
                break;
                
            case SHADER_NODE_NORMAL_TYPE:
                type = TYPE_NORMAL;
                break;
                
            case SHADER_NODE_MATRIX_TYPE:
                type = TYPE_MATRIX;
                break;

            default:
                REYES_ASSERT( false );
                type = TYPE_NULL;
                break;            
        }    
        return type;
    }
    
    static void string_( lalr::PositionIterator<Iterator>* begin, lalr::PositionIterator<Iterator> end, std::string* lexeme, const void** /*symbol*/ )
    {
        REYES_ASSERT( begin );
        REYES_ASSERT( lexeme );
        REYES_ASSERT( lexeme->length() == 1 );

        lalr::PositionIterator<Iterator> position = *begin;
        int terminator = lexeme->at( 0 );
        REYES_ASSERT( terminator == '"' );
        lexeme->clear();
        
        while ( position != end && *position != terminator )
        {
            *lexeme += *position;
            ++position;
        }
        
        if ( position != end )
        {
            ++position;
        }
        
        *begin = position;
    }

    shared_ptr<SyntaxNode> shader_definition_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& shader = start[0].user_data();
        const shared_ptr<SyntaxNode>& formals = start[3].user_data();
        shader->add_node( formals );
        const shared_ptr<SyntaxNode>& statements = start[6].user_data();
        shader->add_node( statements );

        if ( shader->node_type() == SHADER_NODE_LIGHT_SHADER && solar_and_illuminate_statements_ == 0 )
        {
            shared_ptr<SyntaxNode> ambient( new SyntaxNode(SHADER_NODE_AMBIENT, shader->line()) );
            
            const char* LIGHT_COLOR = "Cl";
            shared_ptr<SyntaxNode> light_color( new SyntaxNode(SHADER_NODE_IDENTIFIER, shader->line(), LIGHT_COLOR) );
            light_color->set_symbol( find_symbol(LIGHT_COLOR) );
            ambient->add_node( light_color );

            const char* LIGHT_OPACITY = "Ol";
            shared_ptr<SyntaxNode> light_opacity( new SyntaxNode(SHADER_NODE_IDENTIFIER, shader->line(), LIGHT_OPACITY) );
            light_opacity->set_symbol( find_symbol(LIGHT_OPACITY) );
            ambient->add_node( light_opacity );
            
            statements->add_node_at_front( ambient );
        }
    
        pop_scope();
        return shader;
    }

    static shared_ptr<SyntaxNode> shader_definition( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish, ShaderParserContext* context )
    {
        ShaderParserContext* shader_parser_context = reinterpret_cast<ShaderParserContext*>( context );
        return shader_parser_context->shader_definition_( start, finish );
    }
    
    shared_ptr<SyntaxNode> function_definition_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> function( new SyntaxNode(SHADER_NODE_FUNCTION, start[0].line(), start[1].lexeme()) );
        shared_ptr<Symbol> symbol = symbol_table_.add_symbol( start[1].lexeme() );
        symbol->set_type( type_from_syntax_node(start[0].user_data()) );
        function->set_symbol( symbol );
        const shared_ptr<SyntaxNode>& formals = start[3].user_data();
        function->add_node( formals );
        const shared_ptr<SyntaxNode>& statements = start[6].user_data();
        function->add_node( statements );
        return function;
    }
    
    static shared_ptr<SyntaxNode> add_to_list( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> list = start[0].user_data();
        const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* back = finish - 1;
        if ( back->user_data()->node_type() != SHADER_NODE_LIST )
        {
            list->add_node( back->user_data() );
        }
        else
        {
            list->add_nodes_at_end( back->user_data()->nodes().begin(), back->user_data()->nodes().end() );
        }
        return list;
    }

    static shared_ptr<SyntaxNode> create_list( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> list;
        if ( start[0].user_data()->node_type() != SHADER_NODE_LIST )
        {
            list.reset( new SyntaxNode(SHADER_NODE_LIST, start[0].line()) );
            list->add_node( start[0].user_data() );
        }
        else
        {
            list = start[0].user_data();
        }
        return list;
    }
    
    static shared_ptr<SyntaxNode> empty_list( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> list( new SyntaxNode(SHADER_NODE_LIST, start[0].line()) );
        return list;
    }
    
    shared_ptr<SyntaxNode> formal_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {       
        ValueStorage storage = storage_from_syntax_node( start[1].user_data(), STORAGE_UNIFORM );
        ValueType type = type_from_syntax_node( start[2].user_data() );
        
        const vector<shared_ptr<SyntaxNode> >& nodes = start[3].user_data()->nodes();
        for ( vector<shared_ptr<SyntaxNode> >::const_iterator i = nodes.begin(); i != nodes.end(); ++i )
        {
            SyntaxNode* variable_node = i->get();
            REYES_ASSERT( variable_node );
            REYES_ASSERT( variable_node->node_type() == SHADER_NODE_VARIABLE );
            shared_ptr<Symbol> symbol = symbol_table_.add_symbol( variable_node->lexeme() );
            symbol->set_type( type );
            symbol->set_storage( storage );
            variable_node->set_symbol( symbol );
        }
        
        shared_ptr<SyntaxNode> variable( new SyntaxNode(SHADER_NODE_LIST, start[0].line()) );
        variable->add_nodes_at_end( nodes.begin(), nodes.end() );
        return variable;
    }

    shared_ptr<SyntaxNode> variable_definition_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        ValueStorage storage = storage_from_syntax_node( start[1].user_data(), STORAGE_VARYING );
        ValueType type = type_from_syntax_node( start[2].user_data() );
        
        const vector<shared_ptr<SyntaxNode> >& nodes = start[3].user_data()->nodes();
        for ( vector<shared_ptr<SyntaxNode> >::const_iterator i = nodes.begin(); i != nodes.end(); ++i )
        {
            SyntaxNode* variable_node = i->get();
            REYES_ASSERT( variable_node );
            REYES_ASSERT( variable_node->node_type() == SHADER_NODE_VARIABLE );
            shared_ptr<Symbol> symbol = symbol_table_.add_symbol( variable_node->lexeme() );
            symbol->set_type( type );
            symbol->set_storage( storage );
            variable_node->set_symbol( symbol );
        }
        
        shared_ptr<SyntaxNode> variable( new SyntaxNode(SHADER_NODE_LIST, start[0].line()) );
        variable->add_nodes_at_end( nodes.begin(), nodes.end() );
        return variable;
    }

    shared_ptr<SyntaxNode> definition_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> variable( new SyntaxNode(SHADER_NODE_VARIABLE, start[0].line(), start[0].lexeme()) );
        shared_ptr<SyntaxNode> null( new SyntaxNode(SHADER_NODE_NULL, start[0].line()) );
        variable->add_node( null );
        return variable;
    }
    
    shared_ptr<SyntaxNode> definition_expression_with_assignment( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {        
        shared_ptr<SyntaxNode> variable( new SyntaxNode(SHADER_NODE_VARIABLE, start[0].line(), start[0].lexeme()) );
        const shared_ptr<SyntaxNode>& expression = start[2].user_data();
        variable->add_node( expression );
        return variable;
    }
    
    shared_ptr<SyntaxNode> light_shader( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_light_scope();       
        shared_ptr<SyntaxNode> light_shader( new SyntaxNode(SHADER_NODE_LIGHT_SHADER, start[0].line()) );
        return light_shader;
    }

    shared_ptr<SyntaxNode> surface_shader( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_surface_scope();
        shared_ptr<SyntaxNode> surface_shader( new SyntaxNode(SHADER_NODE_SURFACE_SHADER, start[0].line()) );
        return surface_shader;
    }

    shared_ptr<SyntaxNode> volume_shader( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_volume_scope();
        shared_ptr<SyntaxNode> volume_shader( new SyntaxNode(SHADER_NODE_VOLUME_SHADER, start[0].line()) );
        return volume_shader;
    }

    shared_ptr<SyntaxNode> displacement_shader( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_displacement_scope();
        shared_ptr<SyntaxNode> displacement_shader( new SyntaxNode(SHADER_NODE_DISPLACEMENT_SHADER, start[0].line()) );
        return displacement_shader;
    }

    shared_ptr<SyntaxNode> imager_shader( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_imager_scope();
        shared_ptr<SyntaxNode> imager_shader( new SyntaxNode(SHADER_NODE_IMAGER_SHADER, start[0].line()) );
        return imager_shader;
    }
    
    static shared_ptr<SyntaxNode> float_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> float_type( new SyntaxNode(SHADER_NODE_FLOAT_TYPE, start[0].line()) );
        return float_type;
    }

    static shared_ptr<SyntaxNode> string_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> string_type( new SyntaxNode(SHADER_NODE_STRING_TYPE, start[0].line()) );
        return string_type;
    }

    static shared_ptr<SyntaxNode> color_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> color_type( new SyntaxNode(SHADER_NODE_COLOR_TYPE, start[0].line()) );
        return color_type;
    }

    static shared_ptr<SyntaxNode> point_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> point_type( new SyntaxNode(SHADER_NODE_POINT_TYPE, start[0].line()) );
        return point_type;
    }

    static shared_ptr<SyntaxNode> vector_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> vector_type( new SyntaxNode(SHADER_NODE_VECTOR_TYPE, start[0].line()) );
        return vector_type;
    }

    static shared_ptr<SyntaxNode> normal_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> normal_type( new SyntaxNode(SHADER_NODE_NORMAL_TYPE, start[0].line()) );
        return normal_type;
    }

    static shared_ptr<SyntaxNode> matrix_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> matrix_type( new SyntaxNode(SHADER_NODE_MATRIX_TYPE, start[0].line()) );
        return matrix_type;
    }

    static shared_ptr<SyntaxNode> void_type( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> void_type( new SyntaxNode(SHADER_NODE_VOID_TYPE, start[0].line()) );
        return void_type;
    }

    static shared_ptr<SyntaxNode> varying( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> varying( new SyntaxNode(SHADER_NODE_VARYING, start[0].line()) );
        return varying;
    }

    static shared_ptr<SyntaxNode> uniform( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> uniform( new SyntaxNode(SHADER_NODE_UNIFORM, start[0].line()) );
        return uniform;
    }

    static shared_ptr<SyntaxNode> output( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> output( new SyntaxNode(SHADER_NODE_OUTPUT, start[0].line()) );
        return output;
    }

    static shared_ptr<SyntaxNode> extern_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> extern_( new SyntaxNode(SHADER_NODE_EXTERN, start[0].line()) );
        return extern_;
    }

    static shared_ptr<SyntaxNode> null( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> null( new SyntaxNode(SHADER_NODE_NULL, start[0].line()) );
        return null;
    }

    static shared_ptr<SyntaxNode> block_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& statements = start[1].user_data();
        return statements;
    }
    
    static shared_ptr<SyntaxNode> return_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> return_( new SyntaxNode(SHADER_NODE_RETURN, start[0].line()) );
        const shared_ptr<SyntaxNode>& expression = start[1].user_data();
        REYES_ASSERT( expression );
        return_->add_node( expression );
        return return_;
    }
    
    static shared_ptr<SyntaxNode> break_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> break_( new SyntaxNode(SHADER_NODE_BREAK, start[0].line()) );
        const shared_ptr<SyntaxNode>& level = start[1].user_data();
        if ( level )
        {
            break_->add_node( level );
        }
        return break_;
    }
    
    static shared_ptr<SyntaxNode> continue_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> continue_( new SyntaxNode(SHADER_NODE_CONTINUE, start[0].line()) );
        const shared_ptr<SyntaxNode>& level = start[1].user_data();
        if ( level )
        {
            continue_->add_node( level );
        }
        return continue_;
    }
    
    static shared_ptr<SyntaxNode> if_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& expression = start[2].user_data();
        REYES_ASSERT( expression );

        const shared_ptr<SyntaxNode>& statement = start[4].user_data();
        REYES_ASSERT( statement );
        REYES_ASSERT( statement->node_type() == SHADER_NODE_STATEMENT || statement->node_type() == SHADER_NODE_LIST );

        shared_ptr<SyntaxNode> if_( new SyntaxNode(SHADER_NODE_IF, start[0].line()) );
        if_->add_node( expression );
        if_->add_node( statement );
        return if_;
    }
    
    static shared_ptr<SyntaxNode> if_else_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& expression = start[2].user_data();
        REYES_ASSERT( expression );

        const shared_ptr<SyntaxNode>& statement = start[4].user_data();
        REYES_ASSERT( statement );
        REYES_ASSERT( statement->node_type() == SHADER_NODE_STATEMENT || statement->node_type() == SHADER_NODE_LIST );

        const shared_ptr<SyntaxNode>& else_statement = start[6].user_data();
        REYES_ASSERT( else_statement );
        REYES_ASSERT( else_statement->node_type() == SHADER_NODE_STATEMENT || statement->node_type() == SHADER_NODE_LIST );

        shared_ptr<SyntaxNode> if_else( new SyntaxNode(SHADER_NODE_IF_ELSE, start[0].line()) );
        if_else->add_node( expression );
        if_else->add_node( statement );
        if_else->add_node( else_statement );
        return if_else;
    }
    
    static shared_ptr<SyntaxNode> while_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& expression = start[2].user_data();
        REYES_ASSERT( expression );
        
        const shared_ptr<SyntaxNode>& statement = start[4].user_data();
        REYES_ASSERT( statement );
        REYES_ASSERT( statement->node_type() == SHADER_NODE_STATEMENT || statement->node_type() == SHADER_NODE_LIST );

        shared_ptr<SyntaxNode> while_( new SyntaxNode(SHADER_NODE_WHILE, start[0].line()) );
        while_->add_node( expression );
        while_->add_node( statement );
        return while_;
    }
    
    static shared_ptr<SyntaxNode> for_statement( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& initial_expression = start[2].user_data();
        REYES_ASSERT( initial_expression );
        
        const shared_ptr<SyntaxNode>& condition_expression = start[4].user_data();
        REYES_ASSERT( condition_expression );

        const shared_ptr<SyntaxNode>& increment_expression = start[6].user_data();
        REYES_ASSERT( increment_expression );

        const shared_ptr<SyntaxNode>& statement = start[8].user_data();
        REYES_ASSERT( statement );
        REYES_ASSERT( statement->node_type() == SHADER_NODE_STATEMENT || statement->node_type() == SHADER_NODE_LIST );

        shared_ptr<SyntaxNode> for_( new SyntaxNode(SHADER_NODE_FOR, start[0].line()) );
        for_->add_node( initial_expression );
        for_->add_node( condition_expression );
        for_->add_node( increment_expression );
        for_->add_node( statement );
        return for_;
    }
    
    shared_ptr<SyntaxNode> solar_statement_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> solar( new SyntaxNode(SHADER_NODE_SOLAR, start[0].line()) );
        
        const shared_ptr<SyntaxNode>& parameters = start[2].user_data();
        solar->add_node( parameters );
        
        const shared_ptr<SyntaxNode>& statement = start[4].user_data();
        solar->add_node( statement );

        const char* LIGHT_COLOR = "Cl";
        shared_ptr<SyntaxNode> light_color( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_COLOR) );
        light_color->set_symbol( find_symbol(LIGHT_COLOR) );
        solar->add_node( light_color );

        const char* LIGHT_OPACITY = "Ol";
        shared_ptr<SyntaxNode> light_opacity( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_OPACITY) );
        light_opacity->set_symbol( find_symbol(LIGHT_OPACITY) );
        solar->add_node( light_opacity );

        pop_scope();
        return solar;
    }    
    
    shared_ptr<SyntaxNode> illuminate_statement_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> illuminate( new SyntaxNode(SHADER_NODE_ILLUMINATE, start[0].line()) );

        const shared_ptr<SyntaxNode>& parameters = start[2].user_data();
        illuminate->add_node( parameters );

        const shared_ptr<SyntaxNode>& statement = start[4].user_data();
        illuminate->add_node( statement );

        const char* SURFACE_POSITION = "Ps";
        shared_ptr<SyntaxNode> surface_position( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), SURFACE_POSITION) );
        surface_position->set_symbol( find_symbol(SURFACE_POSITION) );
        illuminate->add_node( surface_position );

        const char* LIGHT_DIRECTION = "L";
        shared_ptr<SyntaxNode> light_direction( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_DIRECTION) );
        light_direction->set_symbol( find_symbol(LIGHT_DIRECTION) );
        illuminate->add_node( light_direction );

        const char* LIGHT_COLOR = "Cl";
        shared_ptr<SyntaxNode> light_color( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_COLOR) );
        light_color->set_symbol( find_symbol(LIGHT_COLOR) );
        illuminate->add_node( light_color );

        const char* LIGHT_OPACITY = "Ol";
        shared_ptr<SyntaxNode> light_opacity( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_OPACITY) );
        light_opacity->set_symbol( find_symbol(LIGHT_OPACITY) );
        illuminate->add_node( light_opacity );

        pop_scope();
        return illuminate;
    }
    
    shared_ptr<SyntaxNode> illuminance_statement_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> illuminance( new SyntaxNode(SHADER_NODE_ILLUMINANCE, start[0].line()) );

        const shared_ptr<SyntaxNode>& parameters = start[2].user_data();
        illuminance->add_node( parameters );
        
        const shared_ptr<SyntaxNode>& statement = start[4].user_data();
        illuminance->add_node( statement );

        const char* LIGHT_DIRECTION = "L";
        shared_ptr<SyntaxNode> light_direction( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_DIRECTION) );
        light_direction->set_symbol( find_symbol(LIGHT_DIRECTION) );
        illuminance->add_node( light_direction );

        const char* LIGHT_COLOR = "Cl";
        shared_ptr<SyntaxNode> light_color( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_COLOR) );
        light_color->set_symbol( find_symbol(LIGHT_COLOR) );
        illuminance->add_node( light_color );

        const char* LIGHT_OPACITY = "Ol";
        shared_ptr<SyntaxNode> light_opacity( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), LIGHT_OPACITY) );
        light_opacity->set_symbol( find_symbol(LIGHT_OPACITY) );
        illuminance->add_node( light_opacity );

        pop_scope();
        return illuminance;
    }
    
    shared_ptr<SyntaxNode> solar_keyword( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_illuminate_or_solar_scope();
        return shared_ptr<SyntaxNode>();
    }
    
    shared_ptr<SyntaxNode> illuminate_keyword( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_illuminate_or_solar_scope();
        return shared_ptr<SyntaxNode>();
    }
    
    shared_ptr<SyntaxNode> illuminance_keyword( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        push_illuminance_scope();
        return shared_ptr<SyntaxNode>();
    }
    
    static shared_ptr<SyntaxNode> statement_error( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        printf( "statement_error\n" );
        return shared_ptr<SyntaxNode>();
    }
    
    static shared_ptr<SyntaxNode> binary_operator( SyntaxNodeType type, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start )
    {
        const shared_ptr<SyntaxNode>& lhs = start[0].user_data();
        REYES_ASSERT( lhs );
        
        const shared_ptr<SyntaxNode>& rhs = start[2].user_data();
        REYES_ASSERT( rhs );
        
        shared_ptr<SyntaxNode> binary_operator( new SyntaxNode(type, start[0].line()) );
        binary_operator->add_node( lhs );
        binary_operator->add_node( rhs );
        return binary_operator;
    }

    static shared_ptr<SyntaxNode> dot_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_DOT, start );
    }
    
    static shared_ptr<SyntaxNode> cross_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_CROSS, start );
    }
    
    static shared_ptr<SyntaxNode> multiply_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_MULTIPLY, start );
    }
    
    static shared_ptr<SyntaxNode> divide_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_DIVIDE, start );
    }
    
    static shared_ptr<SyntaxNode> add_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_ADD, start );
    }
    
    static shared_ptr<SyntaxNode> subtract_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_SUBTRACT, start );
    }
    
    static shared_ptr<SyntaxNode> greater_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_GREATER, start );
    }
    
    static shared_ptr<SyntaxNode> greater_equal_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_GREATER_EQUAL, start );
    }
    
    static shared_ptr<SyntaxNode> less_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_LESS, start );
    }
    
    static shared_ptr<SyntaxNode> less_equal_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_LESS_EQUAL, start );
    }
    
    static shared_ptr<SyntaxNode> equal_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_EQUAL, start );
    }
    
    static shared_ptr<SyntaxNode> not_equal_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_NOT_EQUAL, start );
    }
    
    static shared_ptr<SyntaxNode> and_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_AND, start );
    }
    
    static shared_ptr<SyntaxNode> or_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return binary_operator( SHADER_NODE_OR, start );
    }
    
    static shared_ptr<SyntaxNode> negate_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& expression = start[1].user_data();
        REYES_ASSERT( expression );
        
        shared_ptr<SyntaxNode> negate( new SyntaxNode(SHADER_NODE_NEGATE, start[0].line()) );
        negate->add_node( expression );
        return negate;
    }
    
    static shared_ptr<SyntaxNode> ternary_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& condition_expression = start[0].user_data();
        REYES_ASSERT( condition_expression );
    
        const shared_ptr<SyntaxNode>& expression = start[0].user_data();
        REYES_ASSERT( expression );

        const shared_ptr<SyntaxNode>& else_expression = start[0].user_data();
        REYES_ASSERT( else_expression );

        shared_ptr<SyntaxNode> ternary_expression( new SyntaxNode(SHADER_NODE_TERNARY, start[0].line()) );
        ternary_expression->add_node( condition_expression );
        ternary_expression->add_node( expression );
        ternary_expression->add_node( else_expression );
        return ternary_expression;
    }
    
    static shared_ptr<SyntaxNode> typecast_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> typecast( new SyntaxNode(SHADER_NODE_TYPECAST, start[0].line()) );
        typecast->add_node( start[0].user_data() );
        typecast->add_node( start[1].user_data() );
        return typecast;
    }
    
    static shared_ptr<SyntaxNode> compound_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& expression = start[1].user_data();
        REYES_ASSERT( expression );
        return expression;
    }
    
    static shared_ptr<SyntaxNode> integer_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> integer( new SyntaxNode(SHADER_NODE_INTEGER, start[0].line(), start[0].lexeme()) );
        return integer;
    }
    
    static shared_ptr<SyntaxNode> real_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> real( new SyntaxNode(SHADER_NODE_REAL, start[0].line(), start[0].lexeme()) );
        return real;
    }
    
    static shared_ptr<SyntaxNode> string_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> string( new SyntaxNode(SHADER_NODE_STRING, start[0].line(), start[0].lexeme()) );
        return string;
    }
    
    shared_ptr<SyntaxNode> identifier_expression_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> identifier( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), start[0].lexeme()) );
        identifier->set_symbol( find_symbol(start[0].lexeme()) );
        return identifier;
    }

    static shared_ptr<SyntaxNode> index_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        REYES_ASSERT( false );
        return shared_ptr<SyntaxNode>();
    }
    
    static shared_ptr<SyntaxNode> pass( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& node = start[0].user_data();
        REYES_ASSERT( node );
        return node;
    }
    
    static shared_ptr<SyntaxNode> triple_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& first_expression = start[1].user_data();
        REYES_ASSERT( first_expression );

        const shared_ptr<SyntaxNode>& second_expression = start[3].user_data();
        REYES_ASSERT( second_expression );

        const shared_ptr<SyntaxNode>& third_expression = start[5].user_data();
        REYES_ASSERT( third_expression );
    
        shared_ptr<SyntaxNode> triple( new SyntaxNode(SHADER_NODE_TRIPLE, start[0].line()) );
        triple->add_node( first_expression );
        triple->add_node( second_expression );
        triple->add_node( third_expression );
        return triple;
    }
    
    static shared_ptr<SyntaxNode> sixteentuple_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> sixteentuple( new SyntaxNode(SHADER_NODE_SIXTEENTUPLE, start[0].line()) );
        for ( int i = 0; i < 16; ++i )
        {
            const shared_ptr<SyntaxNode>& expression = start[1 + i * 2].user_data();
            REYES_ASSERT( expression );
            sixteentuple->add_node( expression );
        }        
        return sixteentuple;
    }
    
    static shared_ptr<SyntaxNode> color_typecast( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> color_type( new SyntaxNode(SHADER_NODE_COLOR_TYPE, start[0].line()) );
        const shared_ptr<SyntaxNode>& space = start[1].user_data();
        if ( space )
        {
            color_type->add_node( space );
        }
        return color_type;
    }
    
    static shared_ptr<SyntaxNode> point_typecast( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> point_type( new SyntaxNode(SHADER_NODE_POINT_TYPE, start[0].line()) );
        const shared_ptr<SyntaxNode>& space = start[1].user_data();
        if ( space )
        {
            point_type->add_node( space );
        }
        return point_type;
    }

    static shared_ptr<SyntaxNode> vector_typecast( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> vector_type( new SyntaxNode(SHADER_NODE_VECTOR_TYPE, start[0].line()) );
        const shared_ptr<SyntaxNode>& space = start[1].user_data();
        if ( space )
        {
            vector_type->add_node( space );
        }
        return vector_type;
    }

    static shared_ptr<SyntaxNode> normal_typecast( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> normal_type( new SyntaxNode(SHADER_NODE_NORMAL_TYPE, start[0].line()) );
        const shared_ptr<SyntaxNode>& space = start[1].user_data();
        if ( space )
        {
            normal_type->add_node( space );
        }
        return normal_type;
    }

    static shared_ptr<SyntaxNode> matrix_typecast( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> matrix_type( new SyntaxNode(SHADER_NODE_MATRIX_TYPE, start[0].line()) );
        const shared_ptr<SyntaxNode>& space = start[1].user_data();
        if ( space )
        {
            matrix_type->add_node( space );
        }
        return matrix_type;
    }

    shared_ptr<SyntaxNode> assign_operator( SyntaxNodeType type, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start )
    {
        const shared_ptr<SyntaxNode>& expression = start[2].user_data();
        REYES_ASSERT( expression );
    
        shared_ptr<SyntaxNode> assign_operator( new SyntaxNode(type, start[0].line(), start[0].lexeme()) );
        assign_operator->add_node( expression );
        assign_operator->set_symbol( find_symbol(start[0].lexeme()) );
        return assign_operator;
    }
    
    shared_ptr<SyntaxNode> assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {        
        return assign_operator( SHADER_NODE_ASSIGN, start );
    }
    
    shared_ptr<SyntaxNode> add_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return assign_operator( SHADER_NODE_ADD_ASSIGN, start );
    }
    
    shared_ptr<SyntaxNode> subtract_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return assign_operator( SHADER_NODE_SUBTRACT_ASSIGN, start );
    }
    
    shared_ptr<SyntaxNode> multiply_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return assign_operator( SHADER_NODE_MULTIPLY_ASSIGN, start );
    }
    
    shared_ptr<SyntaxNode> divide_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        return assign_operator( SHADER_NODE_DIVIDE_ASSIGN, start );
    }
    
    shared_ptr<SyntaxNode> index_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        REYES_ASSERT( false );
        return shared_ptr<SyntaxNode>();
    }
    
    shared_ptr<SyntaxNode> index_add_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        REYES_ASSERT( false );
        return shared_ptr<SyntaxNode>();
    }
    
    shared_ptr<SyntaxNode> index_subtract_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        REYES_ASSERT( false );
        return shared_ptr<SyntaxNode>();
    }
    
    shared_ptr<SyntaxNode> index_multiply_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        REYES_ASSERT( false );
        return shared_ptr<SyntaxNode>();
    }
    
    shared_ptr<SyntaxNode> index_divide_assign_expression( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        REYES_ASSERT( false );
        return shared_ptr<SyntaxNode>();
    }
    
    shared_ptr<SyntaxNode> call_expression_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        const shared_ptr<SyntaxNode>& expressions = start[2].user_data();
        REYES_ASSERT( expressions );
        REYES_ASSERT( expressions->node_type() == SHADER_NODE_LIST );
    
        shared_ptr<SyntaxNode> call( new SyntaxNode(SHADER_NODE_CALL, start[0].line(), start[0].lexeme()) );
        call->add_nodes_at_end( expressions->nodes().begin(), expressions->nodes().end() );
        return call;
    }

    shared_ptr<SyntaxNode> texture_expression_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> texture( new SyntaxNode(SHADER_NODE_TEXTURE, start[0].line()) );
        const vector<shared_ptr<SyntaxNode> >& parameters = start[2].user_data()->nodes();
        texture->add_nodes_at_end( parameters.begin(), parameters.end() );
        if ( parameters.size() == 1 )
        {
            shared_ptr<SyntaxNode> s( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), "s") );
            s->set_symbol( symbol_table_.find_symbol("s") );
            texture->add_node( s );
            shared_ptr<SyntaxNode> t( new SyntaxNode(SHADER_NODE_IDENTIFIER, start[0].line(), "t") );
            t->set_symbol( symbol_table_.find_symbol("t") );
            texture->add_node( t );
        }
        return texture;
    }
    
    shared_ptr<SyntaxNode> shadow_expression_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> shadow( new SyntaxNode(SHADER_NODE_SHADOW, start[0].line()) );
        const vector<shared_ptr<SyntaxNode> >& parameters = start[2].user_data()->nodes();
        shadow->add_nodes_at_end( parameters.begin(), parameters.end() );
        return shadow;
    }
    
    shared_ptr<SyntaxNode> environment_expression_( const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* start, const lalr::ParserNode<shared_ptr<SyntaxNode>, char>* finish )
    {
        shared_ptr<SyntaxNode> environment( new SyntaxNode(SHADER_NODE_ENVIRONMENT, start[0].line()) );
        const vector<shared_ptr<SyntaxNode> >& parameters = start[2].user_data()->nodes();
        environment->add_nodes_at_end( parameters.begin(), parameters.end() );
        return environment;
    }
    
    shared_ptr<SyntaxNode> parse( Iterator start, Iterator finish, const char* name )
    {
        REYES_ASSERT( name );

        extern const lalr::ParserStateMachine* shader_parser_state_machine;
        lalr::Parser<lalr::PositionIterator<Iterator>, shared_ptr<SyntaxNode>, char> parser( shader_parser_state_machine, this );
        parser.lexer_action_handlers()
            ( "string", &string_ )
        ;
        parser.parser_action_handlers()
            ( "shader_definition", bind(&ShaderParserContext::shader_definition_, this, _1, _2) )
            ( "function_definition", bind(&ShaderParserContext::function_definition_, this, _1, _2) )
            ( "add_to_list", &add_to_list )
            ( "create_list", &create_list )
            ( "empty_list", &empty_list )
            ( "formal", bind(&ShaderParserContext::formal_, this, _1, _2) )
            ( "variable_definition", bind(&ShaderParserContext::variable_definition_, this, _1, _2) )
            ( "definition_expression", bind(&ShaderParserContext::definition_expression, this, _1, _2) )
            ( "definition_expression_with_assignment", bind(&ShaderParserContext::definition_expression_with_assignment, this, _1, _2) )
            ( "light_shader", bind(&ShaderParserContext::light_shader, this, _1, _2) )
            ( "surface_shader", bind(&ShaderParserContext::surface_shader, this, _1, _2) )
            ( "volume_shader", bind(&ShaderParserContext::volume_shader, this, _1, _2) )
            ( "displacement_shader", bind(&ShaderParserContext::displacement_shader, this, _1, _2) )
            ( "imager_shader", bind(&ShaderParserContext::imager_shader, this, _1, _2) )
            ( "float_type", &float_type )
            ( "string_type", &string_type )
            ( "color_type", &color_type )
            ( "point_type", &point_type )
            ( "vector_type", &vector_type )
            ( "normal_type", &normal_type )
            ( "normal_type", &normal_type )
            ( "matrix_type", &matrix_type )
            ( "void_type", &void_type )
            ( "varying", &varying )
            ( "uniform", &uniform )
            ( "output", &output )
            ( "extern", &extern_ )
            ( "null", &null )
            ( "block_statement", &block_statement )
            ( "return_statement", &return_statement )
            ( "break_statement", &break_statement )
            ( "continue_statement", &continue_statement )
            ( "if_else_statement", &if_else_statement )
            ( "if_statement", &if_statement )
            ( "while_statement", &while_statement )
            ( "for_statement", &for_statement )
            ( "solar_statement", bind(&ShaderParserContext::solar_statement_, this, _1, _2) )
            ( "illuminate_statement", bind(&ShaderParserContext::illuminate_statement_, this, _1, _2) )
            ( "illuminance_statement", bind(&ShaderParserContext::illuminance_statement_, this, _1, _2) )
            ( "solar_keyword", bind(&ShaderParserContext::solar_keyword, this, _1, _2) )
            ( "illuminate_keyword", bind(&ShaderParserContext::illuminate_keyword, this, _1, _2) )
            ( "illuminance_keyword", bind(&ShaderParserContext::illuminance_keyword, this, _1, _2) )
            ( "dot_expression", &dot_expression )
            ( "cross_expression", &cross_expression )
            ( "multiply_expression", &multiply_expression )
            ( "divide_expression", &divide_expression )
            ( "add_expression", &add_expression )
            ( "subtract_expression", &subtract_expression )
            ( "greater_expression", &greater_expression )
            ( "greater_equal_expression", &greater_equal_expression )
            ( "less_expression", &less_expression )
            ( "less_equal_expression", &less_equal_expression )
            ( "equal_expression", &equal_expression )
            ( "not_equal_expression", &not_equal_expression )
            ( "and_expression", &and_expression )
            ( "or_expression", &or_expression )
            ( "negate_expression", &negate_expression )
            ( "ternary_expression", &ternary_expression )
            ( "typecast_expression", &typecast_expression )
            ( "compound_expression", &compound_expression )
            ( "integer_expression", &integer_expression )
            ( "real_expression", &real_expression )
            ( "string_expression", &string_expression )
            ( "identifier_expression", bind(&ShaderParserContext::identifier_expression_, this, _1, _2) )
            ( "index_expression", &index_expression )
            ( "pass", &pass )
            ( "triple_expression", &triple_expression )
            ( "sixteentuple_expression", &sixteentuple_expression )
            ( "color_typecast", &color_typecast )
            ( "point_typecast", &point_typecast )
            ( "vector_typecast", &vector_typecast )
            ( "normal_typecast", &normal_typecast )
            ( "matrix_typecast", &matrix_typecast )
            ( "assign_expression", bind(&ShaderParserContext::assign_expression, this, _1, _2) )
            ( "add_assign_expression", bind(&ShaderParserContext::add_assign_expression, this, _1, _2) )
            ( "subtract_assign_expression", bind(&ShaderParserContext::subtract_assign_expression, this, _1, _2) )
            ( "multiply_assign_expression", bind(&ShaderParserContext::multiply_assign_expression, this, _1, _2) )
            ( "divide_assign_expression", bind(&ShaderParserContext::divide_assign_expression, this, _1, _2) )
            ( "index_assign_expression", bind(&ShaderParserContext::index_assign_expression, this, _1, _2) )
            ( "index_add_assign_expression", bind(&ShaderParserContext::index_add_assign_expression, this, _1, _2) )
            ( "index_subtract_assign_expression", bind(&ShaderParserContext::index_subtract_assign_expression, this, _1, _2) )
            ( "index_multiply_assign_expression", bind(&ShaderParserContext::index_multiply_assign_expression, this, _1, _2) )
            ( "index_divide_assign_expression", bind(&ShaderParserContext::index_divide_assign_expression, this, _1, _2) )
            ( "call_expression", bind(&ShaderParserContext::call_expression_, this, _1, _2) )
            ( "texture_expression", bind(&ShaderParserContext::texture_expression_, this, _1, _2) )
            ( "environment_expression", bind(&ShaderParserContext::environment_expression_, this, _1, _2) )
            ( "shadow_expression", bind(&ShaderParserContext::shadow_expression_, this, _1, _2) )
        ;

        errors_ = 0;
        parser_ = &parser;
        parser.parse( lalr::PositionIterator<Iterator>(start, finish), lalr::PositionIterator<Iterator>() );
        shared_ptr<SyntaxNode> syntax_node;
        if ( parser.accepted() && parser.full() && errors_ == 0 )
        {
            syntax_node = parser.user_data();
        }
        else if ( error_policy_ )
        {
            error_policy_->error( RENDER_ERROR_PARSING_FAILED, "Parsing shader '%s' failed", name );
        }
        return syntax_node;
    }
};

ShaderParser::ShaderParser( SymbolTable& symbol_table, ErrorPolicy* error_policy )
: symbol_table_( symbol_table ),
  error_policy_( error_policy )
{
}

shared_ptr<SyntaxNode> ShaderParser::parse( const char* filename )
{    
    REYES_ASSERT( filename );

    shared_ptr<SyntaxNode> syntax_node;    
    std::ifstream stream( filename, std::ios::binary );
    if ( stream.is_open() )
    {
        stream.unsetf( std::iostream::skipws );
        stream.exceptions( std::iostream::badbit );        
        ShaderParserContext<istream_iterator<char>> shader_parser_context( symbol_table_, error_policy_ );
        syntax_node = shader_parser_context.parse( istream_iterator<char>(stream), istream_iterator<char>(), filename );
    }
    else if ( error_policy_ )
    {
        error_policy_->error( RENDER_ERROR_OPENING_FILE_FAILED, "Opening shader '%s' failed", filename );
    }
    return syntax_node;
}

shared_ptr<SyntaxNode> ShaderParser::parse( const char* start, const char* finish )
{    
    REYES_ASSERT( start );
    REYES_ASSERT( finish );
    REYES_ASSERT( start <= finish );

    ShaderParserContext<const char*> shader_parser_context( symbol_table_, error_policy_ );
    return shader_parser_context.parse( start, finish, "from memory" );
}
