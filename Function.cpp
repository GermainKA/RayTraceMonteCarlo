#include "Function.h"
#include "mesh.h"







// renvoie la ieme direction parmi n
Vector fibonacci(const int i, const int N)
{
    const float ratio = (std::sqrt(5) + 1) / 2;

    float phi = float(2 * M_PI) * fract(i / ratio);
    float cos_theta = 1 - float(2 * i + 1) / float(N * 2);
    float sin_theta = std::sqrt(1 - cos_theta * cos_theta);

    return Vector(std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta);
}

//Monte Carlo Const PDF
Vector mont_car_sampl_dir(Sampler& rng)
{
    float u1 = rng.sample();
    float u2 = rng.sample();
    return mont_car_dir(u1, u2);
}

Vector mont_car_dir(const float u1, const float u2)
{
    float cos_theta = u1;
    float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
    float phi = 2.0f * float(M_PI) * u2;

    return Vector(std::cos(phi) * sin_theta,
                  std::sin(phi) * sin_theta,
                  cos_theta);
}




float epsilon_point( const Point& p )
{
    // plus grande erreur
    float pmax= std::max(std::abs(p.x), std::max(std::abs(p.y), std::abs(p.z)));
    // evalue l'epsilon relatif du point d'intersection
    float pe= pmax * std::numeric_limits<float>::epsilon();
    return pe;
}

// renvoie la normale au point d'intersection
Vector normal( const Mesh& mesh, const Hit& hit )
{
    // recuperer le triangle du mesh
    const TriangleData& data= mesh.triangle(hit.triangle_id);
    
    // interpoler la normale avec les coordonnes barycentriques du point d'intersection
    float w= 1 - hit.u - hit.v;
    Vector n= w * Vector(data.na) + hit.u * Vector(data.nb) + hit.v * Vector(data.nc);
    return normalize(n);
}
