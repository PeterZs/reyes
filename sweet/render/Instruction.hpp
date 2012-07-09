//
// Instruction.hpp
// Copyright (c) 2012 Charles Baker.  All rights reserved.
//

#ifndef SWEET_RENDER_INSTRUCTION_HPP_INCLUDED
#define SWEET_RENDER_INSTRUCTION_HPP_INCLUDED

namespace sweet
{

namespace render
{

/**
// Instructions interpreted by the virtual machine.
*/
enum Instruction
{
    INSTRUCTION_NULL,
    INSTRUCTION_HALT,
    INSTRUCTION_RESET,
    INSTRUCTION_CLEAR_MASK,
    INSTRUCTION_GENERATE_MASK,
    INSTRUCTION_INVERT_MASK,
    INSTRUCTION_JUMP_EMPTY,
    INSTRUCTION_JUMP_NOT_EMPTY,
    INSTRUCTION_JUMP_ILLUMINANCE,
    INSTRUCTION_JUMP,
    INSTRUCTION_TRANSFORM,
    INSTRUCTION_TRANSFORM_VECTOR,
    INSTRUCTION_TRANSFORM_NORMAL,
    INSTRUCTION_TRANSFORM_COLOR,
    INSTRUCTION_TRANSFORM_MATRIX,
    INSTRUCTION_DOT,
    INSTRUCTION_MULTIPLY_FLOAT,
    INSTRUCTION_MULTIPLY_VEC3,
    INSTRUCTION_DIVIDE_FLOAT,
    INSTRUCTION_DIVIDE_VEC3,
    INSTRUCTION_ADD_FLOAT,
    INSTRUCTION_ADD_VEC3,
    INSTRUCTION_SUBTRACT_FLOAT,
    INSTRUCTION_SUBTRACT_VEC3,
    INSTRUCTION_GREATER,
    INSTRUCTION_GREATER_EQUAL,
    INSTRUCTION_LESS,
    INSTRUCTION_LESS_EQUAL,
    INSTRUCTION_AND,
    INSTRUCTION_OR,
    INSTRUCTION_EQUAL_FLOAT,
    INSTRUCTION_EQUAL_VEC3,
    INSTRUCTION_NOT_EQUAL_FLOAT,
    INSTRUCTION_NOT_EQUAL_VEC3,
    INSTRUCTION_NEGATE_FLOAT,
    INSTRUCTION_NEGATE_VEC3,
    INSTRUCTION_PROMOTE_INTEGER,
    INSTRUCTION_PROMOTE_FLOAT,
    INSTRUCTION_PROMOTE_VEC3,
    INSTRUCTION_FLOAT_TO_COLOR,
    INSTRUCTION_FLOAT_TO_POINT,
    INSTRUCTION_FLOAT_TO_VECTOR,
    INSTRUCTION_FLOAT_TO_NORMAL,
    INSTRUCTION_FLOAT_TO_MATRIX,
    INSTRUCTION_ASSIGN_FLOAT,
    INSTRUCTION_ASSIGN_VEC3,
    INSTRUCTION_ASSIGN_MAT4X4,
    INSTRUCTION_ASSIGN_INTEGER,
    INSTRUCTION_ASSIGN_STRING,
    INSTRUCTION_ADD_ASSIGN_FLOAT,
    INSTRUCTION_ADD_ASSIGN_VEC3,
    INSTRUCTION_MULTIPLY_ASSIGN_FLOAT,
    INSTRUCTION_MULTIPLY_ASSIGN_VEC3,
    INSTRUCTION_FLOAT_TEXTURE,
    INSTRUCTION_VEC3_TEXTURE,
    INSTRUCTION_FLOAT_ENVIRONMENT,
    INSTRUCTION_VEC3_ENVIRONMENT,
    INSTRUCTION_SHADOW,
    INSTRUCTION_CALL_0,
    INSTRUCTION_CALL_1,
    INSTRUCTION_CALL_2,
    INSTRUCTION_CALL_3,
    INSTRUCTION_CALL_4,
    INSTRUCTION_CALL_5,
    INSTRUCTION_AMBIENT,
    INSTRUCTION_SOLAR,
    INSTRUCTION_SOLAR_AXIS_ANGLE,
    INSTRUCTION_ILLUMINATE,
    INSTRUCTION_ILLUMINATE_AXIS_ANGLE,
    INSTRUCTION_ILLUMINANCE_AXIS_ANGLE,
    INSTRUCTION_COUNT
};

}

}

#endif