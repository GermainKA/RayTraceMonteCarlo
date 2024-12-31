#include "Scene.h"
#include <fstream>
#include <set>

Scene::Scene(const Mesh &mesh) : m_Mesh_(mesh), m_NbrTriangles(m_Mesh_.triangle_count())
{
    for (int i = 0; i < m_NbrTriangles; i++)
    {
        const TriangleData &t_data = m_Mesh_.triangle(i);
        m_Triangles_.emplace_back(t_data, i);

        const Material &material = m_Mesh_.triangle_material(i);

        if (material.emission.r + material.emission.g + material.emission.b > 0)
        {
            m_Sources_.emplace_back(t_data, material.emission, i);
        }
    }
    assert(m_Sources_.size() > 0);
}

Scene::~Scene()
{
}

Hit Scene::occluded(const Point &p, const Vector &n, const Vector &d)
{
    Ray shadow_ray(p + K * n * epsilon_point(p), d);
    return this->intersect(shadow_ray, shadow_ray.tmax);
}
Hit Scene::closestOccluded(const Point &p, const Vector &n, const Vector &d)
{
    Ray ray(p + K * n * epsilon_point(p), d);
    return this->closestHit(ray, ray.tmax);
}

Hit Scene::intersect(const Ray &ray, const float tmax)
{

    for (int j = 0; j < m_NbrTriangles; ++j)
    {
        Hit hit = m_Triangles_[j].intersect(ray, tmax);
        if (hit)
            return hit;
    }
    return Hit();
}

bool Scene::visible(const Point &p, const Point &q)
{
    Ray visibility_ray(p, q);
    return !(intersect(visibility_ray, visibility_ray.tmax));
}

Hit Scene::closestHit(const Ray &ray, float &tmax)
{
    Hit hit;
    for (int j = 0; j < m_NbrTriangles; ++j)
    {
        if (Hit h = m_Triangles_[j].intersect(ray, tmax))
        {
            assert(h.t > 0);
            hit = h;
            tmax = hit.t;
        }
    }
    return hit;
}

void Scene::withoutShadow(Color &color, const Hit &hit, bool bdrf)
{
    const Color &emission = Color(1.f, 1.f, 1.f) * I;
    const Vector &l = Vector({0.f, 1.f, 0.f, 0.f});
    const Vector &pn = normal(m_Mesh_, hit);
    const Color &fr = (bdrf) ? (diffuse_color(m_Mesh_, hit) / M_PI) : White() / M_PI;
    float cos_theta = std::max(0.0f, dot(normalize(pn), normalize(l)));
    color = Color((fr * emission * ((1 + cos_theta) / 2)), 1);
}

void Scene::fibonacciSampling(Color &color, const Point &p, const Hit &hit, bool withsky, bool bdrf, int N)
{
    Color emission;
    Vector pn = normal(m_Mesh_, hit);
    if (withsky)
        emission = Color(1.f, 1.f, 1.f) * 10;
    Vector f;
    Color fr = (bdrf) ? (diffuse_color(m_Mesh_, hit) / M_PI) : White() / M_PI;
    const Material &pmaterial = m_Mesh_.triangle_material(hit.triangle_id);
    emission = emission + pmaterial.emission;
    color = Black();

    const World &world(pn);
    for (int i = 0; i < N; i++)
    {
        f = world(fibonacci(i, N));

        if (Hit h = closestOccluded(p, pn, f))
        {
            if (withsky)
            {
                continue;
            }
            const Material &material = m_Mesh_.triangle_material(h.triangle_id);
            if (material.emission.max() > 0)
            {
                float cos_theta = std::max(0.0f, dot(normalize(pn), normalize(f)));
                color = color + (fr * material.emission * cos_theta);
                continue;
            }
            continue;
        }
        float cos_theta = std::max(0.0f, dot(normalize(pn), normalize(f)));
        color = color + (fr * emission * cos_theta);
    }
    color = Color(color / float(N), 1);
}

void Scene::montCarloConstPdf(Color &color, const Point &p, const Hit &hit, Sampler &rng, bool withsky, bool bdrf, int N)
{

    float v = 1.0; // visibility
    Color emission;
    if (withsky)
        emission = Color(1.f, 1.f, 1.f) * I;

    const Vector &pn = normal(m_Mesh_, hit);
    Vector d;
    const Color &fr = (bdrf) ? (diffuse_color(m_Mesh_, hit) / M_PI) : White();
    color = Black();

    const Material &pmaterial = m_Mesh_.triangle_material(hit.triangle_id);
    emission = emission + pmaterial.emission;

    const World &world(pn);

    const float pdf = mont_car_const_pdf();

    for (int i = 0; i < N; i++)
    {
        d = world(mont_car_sampl_dir(rng));
        v = 1.0f;
        if (Hit h = closestOccluded(p, pn, d))
        {
            if (withsky)
            {
                continue;
            }
            const Material &material = m_Mesh_.triangle_material(h.triangle_id);
            if (material.emission.max() > 0)
            {
                float cos_theta = std::max(0.0f, dot(normalize(pn), normalize(d)));
                color = color + (fr * material.emission * v * cos_theta * (1 / pdf));
                continue;
            }
            continue;
        }
        float cos_theta = std::max(0.0f, dot(normalize(pn), normalize(d)));
        color = color + (fr * emission * v * cos_theta * (1 / pdf));
    }
    color = Color(color / float(N), 1);
}

void Scene::montCarloAreaPdf(Color &color, const Point &p, const Hit &hit, Sampler &rng, bool bdrf, int N)
{

    Color emission;
    const Vector &pn = normal(m_Mesh_, hit);
    const Color &fr = (bdrf) ? (diffuse_color(m_Mesh_, hit) / M_PI) : White();
    color = Black();

    for (int i = 0; i < N; i++)
    {
        int s = rng.sample_range(m_Sources_.size());
        const Source &source = m_Sources_[s];
        emission = source.emission / 1.5;
        const Vector &qn = source.n;

        // place le point dans la source / triangle
        float b0 = rng.sample() / 2;
        float b1 = rng.sample() / 2;
        float offset = b1 - b0;

        if (offset > 0)
            b1 = b1 + offset;
        else
            b0 = b0 - offset;

        float b2 = 1 - b0 - b1;

        // construire le point
        const Point &q = b0 * source.a + b1 * source.b + b2 * source.c;

        float pdf = (1 / float(m_Sources_.size())) * (1 / source.area);
        if (visible(p + 0.001 * pn, q + 0.001 * qn))
        {
            float cos_theta = std::max(float(0), dot(normalize(pn), normalize(Vector(p, q))));
            float cos_theta_q = std::max(float(0), dot(normalize(qn), normalize(Vector(q, p))));
            color = color + emission * fr * cos_theta * cos_theta_q / distance2(p, q) / pdf;
        }
    }
    color = Color(color / float(N), 1);
}
