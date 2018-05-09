#ifndef REYES_HYPERBOLOID_HPP_INCLUDED
#define REYES_HYPERBOLOID_HPP_INCLUDED

#include "declspec.hpp"
#include "Geometry.hpp"
#include <math/vec2.hpp>
#include <math/vec3.hpp>
#include <math/mat4x4.hpp>
#include <list>
#include <memory>

namespace sweet
{

namespace render
{

class Grid;

class REYES_DECLSPEC Hyperboloid : public Geometry
{
    math::vec3 point1_;
    math::vec3 point2_;
    float thetamax_;
    
public:
    Hyperboloid( const math::vec3& point1, const math::vec3& point2, float thetamax );
    Hyperboloid( const Hyperboloid& hyperboloid, const math::vec2& u_range, const math::vec2& v_range );

    bool boundable() const;
    void bound( const math::mat4x4& transform, math::vec3* minimum, math::vec3* maximum ) const;
    bool splittable() const;
    void split( std::list<std::shared_ptr<Geometry>>* primitives ) const;
    bool diceable() const;
    void dice( const math::mat4x4& transform, int width, int height, Grid* grid ) const;

private:
    math::vec3 position( float u, float v ) const;
};

}

}

#endif
