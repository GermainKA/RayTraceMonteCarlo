#pragma once
#include "vec.h"
#include "mesh.h"
#include <cfloat>
#include <math.h>
#include <limits>
#include <random>
#include <omp.h>


//STRUCT


struct Ray
{
    Point o;                // origine
    Vector d;               // direction
    float tmax;             // position de l'extremite, si elle existe. le rayon est un intervalle [0 tmax]
    
    // le rayon est un segment, on connait origine et extremite, et tmax= 1
    Ray( const Point& origine, const Point& extremite ) : o(origine), d(Vector(origine, extremite)), tmax(1) {}
    
    // le rayon est une demi droite, on connait origine et direction, et tmax= \inf
    Ray( const Point& origine, const Vector& direction ) : o(origine), d(direction), tmax(FLT_MAX) {}
    
    // renvoie le point sur le rayon pour t
    Point point( const float t ) const { return o + t * d; }
};


struct Hit
{
    float t;            // p(t)= o + td, position du point d'intersection sur le rayon
    float u, v;         // p(u, v), position du point d'intersection sur le triangle
    int triangle_id;    // indice du triangle dans le mesh
    
    Hit( ) : t(FLT_MAX), u(), v(), triangle_id(-1) {}
    Hit( const float _t, const float _u, const float _v, const int _id ) : t(_t), u(_u), v(_v), triangle_id(_id) {}
    
    // renvoie vrai si intersection
    operator bool ( ) { return (triangle_id != -1); }
};


struct Triangle
{
    Point p;            // sommet a du triangle
    Vector e1, e2;      // aretes ab, ac du triangle
    int id;             // indice du triangle
    
    Triangle( const TriangleData& data, const int _id ) : p(data.a), e1(Vector(data.a, data.b)), e2(Vector(data.a, data.c)), id(_id) {}
    
    /* calcule l'intersection ray/triangle
        cf "fast, minimum storage ray-triangle intersection" 
        
        renvoie faux s'il n'y a pas d'intersection valide (une intersection peut exister mais peut ne pas se trouver dans l'intervalle [0 tmax] du rayon.)
        renvoie vrai + les coordonnees barycentriques (u, v) du point d'intersection + sa position le long du rayon (t).
        convention barycentrique : p(u, v)= (1 - u - v) * a + u * b + v * c
    */
   // Hit intersect( const Point& o, const Vector& d, const float tmax );
    Hit intersect( const Ray &ray, const float tmax ) const
    {
        Vector pvec= cross(ray.d, e2);
        float det= dot(e1, pvec);
        
        if(std::fabs(det) < 1e-8f) return Hit();

        float inv_det= 1 / det;
        Vector tvec(p, ray.o);
        
        float u= dot(tvec, pvec) * inv_det;
        if(u < 0 || u > 1) return Hit();        // pas d'intersection
        
        Vector qvec= cross(tvec, e1);
        float v= dot(ray.d, qvec) * inv_det;
        if(v < 0 || u + v > 1) return Hit();    // pas d'intersection
        
        float t= dot(e2, qvec) * inv_det;
        if(t > tmax || t < 0) return Hit();     // pas d'intersection
        
        return Hit(t, u, v, id);                // p(u, v)= (1 - u - v) * a + u * b + v * c
    }
    
};

struct Sampler
{
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform;
    
    // initialiser une sequence aléatoire.
    Sampler( const unsigned seed ) : rng( seed ), uniform(0, 1) {}
    
    // renvoyer un réel entre 0 et 1 (exclus).
    float sample( ) 
    { 
        float u= uniform( rng );
        // verifier que u est strictement plus petit que 1      
        if(u >= 1)          
            u= 0.99999994f; // plus petit float < 1
        
        return u;
    }
    
    // renvoyer un entier entre 0 et n (exclus).
    int sample_range( const int n ) 
    { 
        int u= uniform( rng ) * n; 
        // vérifier que u est strictement plus petit que n
        if(u >= n)
            u= n -1;    // plus petit entier < n
            
        return u;
    }
};


struct Source {
    
    Point position;       
    Color emission;       
    int triangleId;       
    Vector n;             
    Point a;              
    Point b;              
    Point c;              
    float area;           

    Source(const TriangleData& t_data, const Color& e, int id)
        : position(),
          emission(e),
          triangleId(id),
          n(),
          a(t_data.a),
          b(t_data.b),
          c(t_data.c){
        Vector ng = cross(Vector(a, b), Vector(a, c));
        n = normalize(ng);
        area = length(ng) / 2;
        assert(area * emission.max() > 0);
        position = (a + b + c) / 3;
    }

    Point sample(Sampler& rng) const {
        float b0 = rng.sample() / 2;
        float b1 = rng.sample() / 2;
        float offset = b1 - b0;

        if (offset > 0)
            b1 = b1 + offset;
        else
            b0 = b0 - offset;

        float b2 = 1 - b0 - b1;
        return b0 * a + b1 * b + b2 * c;
    }

    float pdf(const Point& q) const {
        return 1 / area;
    }
};

struct World
{
    World( ) : t(), b(), n() {}
    
    World( const Vector& _n ) : n(_n) 
    {
        float s= std::copysign(float(1), n.z);  // s= 1 ou -1
        float a= -1 / (s + n.z);
        float d= n.x * n.y * a;
        t= Vector(1 + s * n.x * n.x * a, s * d, -s * n.x);
        b= Vector(d, s + n.y * n.y * a, -n.y);        
    }
    
    // transforme le vecteur du repere local vers le repere du monde
    Vector operator( ) ( const Vector& local )  const { return local.x * t + local.y * b + local.z * n; }
    
    // transforme le vecteur du repere du monde vers le repere local
    Vector local( const Vector& global ) const { return Vector(dot(global, t), dot(global, b), dot(global, n)); }
    
    Vector t;   // x
    Vector b;   // y
    Vector n;   // z
};


//INLINE DEF

inline Color diffuse_color( const Mesh& mesh, const Hit& hit )
{
    const Material& material= mesh.triangle_material(hit.triangle_id);
    return material.diffuse;
}



inline float fract(const float v) { return v - std::floor(v); }



inline vec3 operator+( const vec3& a, const vec3& b )
    { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }

inline vec3 operator/( const vec3& a, const float f )
    { return vec3(a.x / f, a.y / f, a.z / f); }

inline float mont_car_const_pdf(){
    return 1 / float(2 * M_PI);
}


//DECL
float epsilon_point( const Point& p );
Vector fibonacci(const int i, const int N);
Vector normal( const Mesh& mesh, const Hit& hit );

Vector mont_car_dir( const float u1, const float u2 );
Vector mont_car_sampl_dir(Sampler& rng) ;

