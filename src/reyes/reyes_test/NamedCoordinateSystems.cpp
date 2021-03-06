
#include <UnitTest++/UnitTest++.h>
#include <reyes/Renderer.hpp>
#include <reyes/Shader.hpp>
#include <reyes/Grid.hpp>
#include <reyes/Value.hpp>
#include <reyes/SymbolTable.hpp>
#include <reyes/assert.hpp>
#include <math/vec3.ipp>
#include <math/mat4x4.ipp>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>

using std::vector;
using std::shared_ptr;
using namespace math;
using namespace reyes;

static const float TOLERANCE = 0.01f;

SUITE( NamedCoordinateSystems )
{
    struct RendererFixture
    {
        Renderer renderer;
        
        RendererFixture()
        : renderer()
        {
            renderer.begin();
            renderer.perspective( float(M_PI) / 8.0f );
            renderer.projection();
        }

        shared_ptr<Value> execute_shader( Shader& shader )
        {
            renderer.surface_shader( &shader );

            Grid grid;
            grid.resize( 1, 1 );  
            grid.value( "P", TYPE_POINT ).zero();
            grid.value( "N", TYPE_NORMAL ).zero();
            renderer.surface_shade( grid );
            return grid.find_value( "P" );
        }
        
        enum TransformType
        {
            TRANSFORM_WORLD_TO_CAMERA,
            TRANSFORM_CAMERA_TO_WORLD
        };
            
        void check_transform_between_coordinate_systems( const char* source, const vec4& value_to_transform, TransformType transform_type )
        {
            Shader shader( source, source + strlen(source), renderer.symbol_table(), renderer.error_policy() );
        
            const float MINIMUM_ROTATION = -float(M_PI);
            const float MAXIMUM_ROTATION = float(M_PI);
            const int RY_STEPS = 5;
            const int RX_STEPS = 8;
            
            for ( int ry = 0; ry < RY_STEPS; ++ry )
            {
                for ( int rx = 0; rx < RX_STEPS; ++rx )
                {
                    float rx_angle = float(rx) / float(RX_STEPS) * (MAXIMUM_ROTATION - MINIMUM_ROTATION) + MINIMUM_ROTATION;
                    float ry_angle = float(ry) / float(RY_STEPS) * (MAXIMUM_ROTATION - MINIMUM_ROTATION) + MINIMUM_ROTATION;
                
                    renderer.translate( vec3(0.0f, 0.0f, 1.0f) );
                    renderer.rotate( rx_angle, 1.0f, 0.0f, 0.0f );
                    renderer.rotate( ry_angle, 0.0f, 1.0f, 0.0f );
                    renderer.begin_world();
            
                    mat4x4 world_to_camera = translate( 0.0f, 0.0f, 1.0f ) * rotate( vec3(1.0f, 0.0f, 0.0f), rx_angle ) * rotate( vec3(0.0f, 1.0f, 0.0f), ry_angle );
                    mat4x4 camera_to_world = inverse( world_to_camera );             
                    const mat4x4& transform = transform_type == TRANSFORM_WORLD_TO_CAMERA ? world_to_camera : camera_to_world;
                    
                    shared_ptr<Value> positions = execute_shader( shader );
                    CHECK( positions );
                    if ( positions )
                    {
                        vec3 origin = positions->vec3_values()[0];
                        vec3 expected_origin = vec3( transform * value_to_transform );
                        CHECK_CLOSE( 1.0f, dot(origin, expected_origin), TOLERANCE );
                        CHECK_CLOSE( 0.0f, length(origin - expected_origin), TOLERANCE );
                    }

                    renderer.end_world();
                }                
            }        
        }
    };

    TEST_FIXTURE( RendererFixture, transform_to_world_point )
    {
        const char* source = "surface test( point origin = point \"current\" (0, 0, 0); )\n"
            "{\n"
            "   P = transform( \"world\", P );\n"
            "}\n"
        ;
        check_transform_between_coordinate_systems( source, vec4(0.0f, 0.0f, 0.0f, 1.0f), TRANSFORM_CAMERA_TO_WORLD );
    }    
    
    TEST_FIXTURE( RendererFixture, typecast_from_world_point )
    {
        const char* source = "surface test( point origin = point \"world\" (0, 0, 0); )\n"
            "{\n"
            "   P = origin;\n"
            "}\n"
        ;
        check_transform_between_coordinate_systems( source, vec4(0.0f, 0.0f, 0.0f, 1.0f), TRANSFORM_WORLD_TO_CAMERA );
    }
    
    TEST_FIXTURE( RendererFixture, typecast_from_world_vector )
    {
        const char* source = "surface test( vector forward = vector \"world\" (0, 0, 1); )\n"
            "{\n"
            "   P = forward;\n"
            "}\n"
        ;
        check_transform_between_coordinate_systems( source, vec4(0.0f, 0.0f, 1.0f, 0.0f), TRANSFORM_WORLD_TO_CAMERA );
    }

    TEST_FIXTURE( RendererFixture, typecast_from_world_normal )
    {
        const char* source = "surface test( vector forward = normal \"world\" (0, 0, 1); )\n"
            "{\n"
            "   P = forward;\n"
            "}\n"
        ;
        check_transform_between_coordinate_systems( source, vec4(0.0f, 0.0f, 1.0f, 0.0f), TRANSFORM_WORLD_TO_CAMERA );
    }
}
