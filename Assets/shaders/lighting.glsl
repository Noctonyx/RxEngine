
#define MAX_SHADOW_CASCADES 4

struct shadowCascade {
    mat4 viewProjMatrix;
   //mat4 projMatrix;
    //mat4 viewMatrix;
    float splitDepth;
};

struct Lighting {
    float ambientStrength;
    float diff_amount;
    float specularStrength;
    float specular_power;    
    vec3 light_direction; 
    uint cascadeCount;
    shadowCascade[MAX_SHADOW_CASCADES] cascades;
};

