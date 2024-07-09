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
uniform sampler2D previousFrame;

layout(std140) uniform UniformBlock {

    vec3 lookfrom;
    vec3 pixelDH;
    vec3 pixelDV;
    vec3 pixelOrigin;

    float u_time;

    int maxRayBounce;
    int renderedFrameCount;
    int samplesPerPixel;

    bool test;
    bool sky;
    bool doGammaCorrection;
    bool doTemporalAntiAliasing;

};

layout(std140) uniform Spheres {
    Sphere spheres[MAX_SPHERE_AMOUNT];
};
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

    vec3 outwardNormal = (hit.intersection - sphere.position) / sphere.radius;
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
vec3 gammaCorrect(vec3 linear) {
    return vec3(sqrt(linear.x), sqrt(linear.y), sqrt(linear.z));
}

// * Ray tracing
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

vec3 traceRay(Ray ray) {

    vec3 lightDir = vec3(-1.0, -1.0, -1.0);

    vec3 incomingColour = vec3(0.0);
    vec3 rayColour = vec3(1.0);
    
    Ray currentRay = ray;
    RayHit closestHit;
    
    for (int i = 0; i < maxRayBounce; i++) {

        if (findClosestIntersection(currentRay, closestHit)) {

            Material material = closestHit.material;
            vec3 normal = normalize(closestHit.normal);
            currentRay = Ray(closestHit.intersection + normal*0.001, normal + material.roughness*randGaussianUnitVec());

            vec3 emittedLight = material.emissionColour * material.emissionStrength;
            incomingColour += emittedLight * rayColour;
            rayColour *= material.albedo*material.reflectivity*(max(dot(normalize(-lightDir), normal), 0.0));

        } else {

            if (sky) {

                vec3 unitDirection = normalize(ray.direction);
                float alpha = 0.5*(unitDirection.y + 1.0);
                incomingColour += ((1.0 - alpha)*vec3(1.0) + alpha*vec3(0.5, 0.7, 1.0))*rayColour;
            }
            else incomingColour += vec3(0.0)*rayColour;

            break;
        }
    }

    return incomingColour;
}

// * Main
void main() {

    // Create ray from camera
    vec4 currentColour = vec4(0.0);
    for (int i = 0; i < samplesPerPixel; i++) {

        // Create ray from window coordinates
        vec2 pos = vec2(gl_FragCoord.x, gl_FragCoord.y);
        vec3 offset = vec3(rand() - 0.5, rand() - 0.5, 0.0);
        vec3 pixelSample = pixelOrigin + ((pos.x + offset.x) * pixelDH) + ((pos.y + offset.y) * pixelDV);
        Ray ray = Ray(lookfrom, pixelSample - lookfrom);

        // Calculate ray colour
        currentColour += vec4(traceRay(ray), 1.0);
    }
    currentColour /= samplesPerPixel;
    currentColour = doGammaCorrection ? vec4(gammaCorrect(currentColour.xyz), 1.0) : currentColour;

    // Output averaged colour
    if (doTemporalAntiAliasing) {
        vec4 previousColour = texture(previousFrame, TexCoords);
        FragColor = mix(previousColour, currentColour, 1.0 / (renderedFrameCount + 1));
    } else FragColor = currentColour;
}
