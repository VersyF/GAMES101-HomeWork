//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
#include <cmath>


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    return Scene::Shade(ray);
    
}

Vector3f Scene::Shade(Ray ray) const
{
    //Info
    Intersection hit = Scene::intersect(ray);
    if(!hit.happened){
        return Vector3f(0.f, 0.f, 0.f);
    }
    Vector3f wo = -ray.direction;

    Vector3f p = hit.coords;
    Vector3f N = hit.normal;
    Material *m = hit.m;

    //Direct Light
    Vector3f L_dir(0.f, 0.f, 0.f);
    Intersection L_hit;
    float pdf;
    sampleLight(L_hit, pdf);

    Vector3f castShadowRay = normalize(L_hit.coords - p);
    
    Intersection castShadow = Scene::intersect(Ray(p+N*0.01f, castShadowRay));

    Vector3f f_r = m->eval(castShadowRay, wo, hit.normal);      //XX used L_hit.normal
    float theta_p = dotProduct(N, castShadowRay);    //xx
    float theta_x = dotProduct(L_hit.normal, -castShadowRay);
    
    bool blocked = castShadow.happened && castShadow.distance < (hit.coords - L_hit.coords).norm() - 0.01f;

    if(!blocked){
        L_dir = L_hit.emit * f_r * theta_p * theta_x / std::pow((hit.coords - L_hit.coords).norm(), 2) / pdf;
    }

    //Indirect Light
    /*
    if(get_random_float() > RussianRoulette)
    {
        return L_dir;
    }
    Vector3f L_indir(0.f, 0.f, 0.f);
    Vector3f wi = m->sample(wo, N);
    float pdf_indir = m->pdf(wi, wo, N);

    L_indir = Shade(Ray())

    return L_dir + L_indir;
    */
    return L_dir;
}