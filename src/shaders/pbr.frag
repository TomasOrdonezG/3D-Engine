#version 330 core

// * Inputs / Outputs
in vec2 TexCoords;
out vec4 FragColor;

// * Macrodefinitions
#define FLOAT_MAX 3.402823466e+38
#define FLOAT_MIN 1.175494351e-38
#define PI 3.14159265358979323846
#define MAX_SPHERE_AMOUNT 20

// * Struct definitions
struct Ray { vec3 position, direction; };
struct Material { vec3 albedo; float roughness; vec3 emissionColour; float emissionStrength; float reflectivity; };
struct Sphere { Material material; vec3 position; float radius; };
struct RayHit { vec3 normal; float t; vec3 intersection; bool selected; Material material; };

// * Uniforms

// TODO: Camera UBO
uniform vec3 lookfrom;

// TODO: Viewport UBO
uniform vec3 pixelDH;
uniform vec3 pixelDV;
uniform vec3 pixelOrigin;

// TODO: Renderer settings UBO
uniform bool doTemporalAntiAliasing;
uniform bool doGammaCorrection;
uniform bool test;
uniform bool sky;
uniform float u_time;
uniform int maxRayBounce;
uniform int renderedFrameCount;
uniform int samplesPerPixel;
uniform sampler2D previousFrame;

// Scene UBO (only an array of spheres for now)
layout(std140) uniform Spheres { Sphere spheres[MAX_SPHERE_AMOUNT]; };
uniform int spheresSize;
uniform int selectedSphere;

// * Spheres
bool hitSphere(Sphere sphere, Ray ray, out RayHit hit) {

    vec3 oc = sphere.position - ray.position;
    float a = dot(ray.direction, ray.direction);
    float h = dot(ray.direction, oc);
    float c = dot(oc, oc) - sphere.radius*sphere.radius;

    float discriminant = h*h - a*c;
    if (discriminant < 0) return false;

    float sqrtd = sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range.
    float root = (h - sqrtd) / a;
    if (root <= 0.001 || FLOAT_MAX <= root) {
    
        root = (h + sqrtd) / a;
        if (root <= 0.001 || FLOAT_MAX <= root)
            return false;
    }

    hit.t = root;
    hit.intersection = ray.position + hit.t*ray.direction;
    hit.material = sphere.material;

    vec3 outwardNormal = (hit.intersection - sphere.position) / sphere.radius;  // Normalizes it
    bool frontFace = dot(ray.direction, outwardNormal) < 0;
    hit.normal = frontFace ? outwardNormal : -outwardNormal;

    return true;
}

// * Utility functions
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233)) * u_time) * 43758.5453);
}
float rand() {
    return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233)) * u_time) * 43758.5453);
}
vec2 boxMuller(vec2 u) {
    float r = sqrt(-2.0 * log(u.x));
    float theta = 2.0 * PI * u.y;
    return r * vec2(cos(theta), sin(theta));
}
vec3 randGaussianVec() {
    vec2 u1 = vec2(rand(gl_FragCoord.xy), rand(gl_FragCoord.yx));
    vec2 u2 = vec2(rand(gl_FragCoord.yx + 0.5), rand(gl_FragCoord.xy + 0.5));
    vec2 gauss1 = boxMuller(u1);
    vec2 gauss2 = boxMuller(u2);
    return vec3(gauss1.x, gauss1.y, gauss2.x);
}
vec3 randGaussianUnitVec() {
    return normalize(randGaussianVec());
}
vec3 randInHemisphere(vec3 normal) {
    vec3 randomDir = randGaussianUnitVec();
    return (dot(randomDir, normal) > 0.0) ? randomDir : -randomDir;
}
vec3 gammaCorrect(vec3 linear) {
    return vec3(sqrt(linear.x), sqrt(linear.y), sqrt(linear.z));
}

// * Ray tracing
int countLights() {
    int lights = 0;
    for (int i = 0; i < spheresSize; i++) {
        if (spheres[i].material.emissionStrength > 0.0) {
            lights++;
        }
    }
    return lights;
}

bool findClosestIntersection(Ray ray, out RayHit closestHit) {

    bool doesHit = false;
    float lowest_t = FLOAT_MAX;
    
    for (int i = 0; i < spheresSize; i++) {

        RayHit hit;
        if (hitSphere(spheres[i], ray, hit)) {

            doesHit = true;
            hit.selected = (selectedSphere == i);
            
            if (hit.t < lowest_t) {

                lowest_t = hit.t;
                closestHit = hit;
            }
        }
    }

    return doesHit;
}

vec3 missColour(Ray ray) {

    if (sky) {

        vec3 unitDirection = normalize(ray.direction);
        float alpha = 0.5*(2*unitDirection.y + 1.0);
        return ((1.0 - alpha)*vec3(1.0) + alpha*vec3(0.5, 0.7, 1.0));
    }
    else {

        return vec3(0.0);
    }
}

vec3 CookTorranceBRDF(vec3 P, vec3 L, vec3 V, vec3 N, RayHit hit) {
    
    float a = hit.material.roughness;
    vec3 h = normalize(L + V);

    // Normal distribution (D)
    float a2 = a*a;
    float NdotH = max(dot(N, h), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom = a2;
    float denom = NdotH2*(a2 - 1.0) + 1.0;
    denom = PI*denom*denom;
    float D = nom / denom;

    // Geometry Function (G)
    float k = a2 / 2.0;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = NdotV / (NdotV*(1.0 - k) + k);
    float ggx2 = NdotL / (NdotV*(1.0 - k) + k);
    float G = ggx1*ggx2;

    // Fresnel equation (F)
    vec3 F0 = vec3(0.04);
    vec3 F = F0 + (1.0 - F0)*pow(1.0 - NdotH, 5.0);

    // Reflection and refraction index
    vec3 Ks = F;
    vec3 Kd = vec3(1.0) - Ks;

    // Final BRDF calculation
    vec3 f_Lambert = hit.material.albedo / PI;
    vec3 f_CookTorrance = (D*F*G) / (2.0*NdotV*NdotL + 0.0001);
    vec3 BRDF = Kd*f_Lambert + f_CookTorrance;

    return BRDF;
}

vec4 directIllumination(Ray ray, int lightsCount) {
    
    RayHit hit;
    if (!findClosestIntersection(ray, hit))
        return vec4(missColour(ray), 1.0);

    vec3 V = -normalize(ray.direction);
    vec3 P = hit.intersection;
    vec3 N = hit.normal;

    vec3 outgoingRadiance = hit.material.emissionColour * hit.material.emissionStrength;
    float dWi = 1.0 / float(lightsCount);

    for (int i = 0; i < spheresSize; i++) {

        // Check if the sphere is emissive
        Material light = spheres[i].material;
        if (light.emissionStrength <= 0.0) continue;

        // Incoming light vector
        vec3 L = normalize(spheres[i].position - P);
        
        // Check for light obstruction
        int lightCoeff = 1;
        for (int j = 0; j < spheresSize; j++) {
            if (spheres[j] == spheres[i]) continue;
            Ray hitToLight = Ray(P, L);
            RayHit obstructionHit;
            if (hitSphere(spheres[j], hitToLight, obstructionHit))
                lightCoeff = 0;
        }

        // Integrate over each light
        float NdotL = max(dot(N, L), 0.0);
        vec3 fr = CookTorranceBRDF(P, L, V, N, hit); 
        vec3 Li = light.emissionColour * light.emissionStrength;
        outgoingRadiance += lightCoeff*fr*Li*NdotL*dWi;
    }

    return vec4(outgoingRadiance, 1.0);
}

// * Main
void main() {

    // Calculate number of lights
    int lightsCount = countLights();

    // Create ray from camera
    vec4 currentColour = vec4(0.0);
    for (int i = 0; i < samplesPerPixel; i++) {

        // Create ray from window coordinates
        vec2 pos = vec2(gl_FragCoord.x, gl_FragCoord.y);
        vec3 offset = vec3(rand() - 0.5, rand() - 0.5, 0.0);
        vec3 pixelSample = pixelOrigin + ((pos.x + offset.x) * pixelDH) + ((pos.y + offset.y) * pixelDV);
        Ray ray = Ray(lookfrom, pixelSample - lookfrom);

        // Calculate ray colour
        currentColour += directIllumination(ray, lightsCount);
    }
    currentColour /= samplesPerPixel;
    currentColour = doGammaCorrection ? vec4(gammaCorrect(currentColour.xyz), 1.0) : currentColour;

    // Output averaged colour
    if (doTemporalAntiAliasing) {
        vec4 previousColour = texture(previousFrame, TexCoords);
        FragColor = mix(previousColour, currentColour, 1.0 / (renderedFrameCount + 1));
    } else FragColor = currentColour;
}
