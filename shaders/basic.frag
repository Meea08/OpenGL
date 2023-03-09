#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;
in float fragFogDensity;
in vec4 fragPosEye;
in vec4 fragPosLightSpace;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
uniform sampler2D shadowMap;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

//fog
//float fogDensity;

float computeFog(){
	float fragmentDistance = length(fragPosEye);
	float fogFactor = exp(-pow(fragmentDistance * fragFogDensity, 2));
	return clamp(fogFactor, 0.0f, 1.0f);
}

vec3 computeDirLight()
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    
	specular = specularStrength * specCoeff * lightColor;
	return (ambient + diffuse + specular);
}
float computeShadow()
{	

    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
    float currentDepth = normalizedCoords.z;
	float bias = 0.005f;
    float shadow = currentDepth - bias> closestDepth  ? 1.0f : 0.0f;

    return shadow;	
}

void main() 
{
    vec3 light = computeDirLight();
	float shadow = computeShadow();
    //compute final vertex color
    vec4 color = vec4((ambient + (1.0f - shadow) * diffuse),1.0f) * texture(diffuseTexture, fTexCoords) + vec4((1.0f - shadow) * specular, 1.0f) * texture(specularTexture, fTexCoords);
	if (color.a < 0.1) discard;
	else{
		//fColor = color;
		vec4 newColor = vec4(color.xyz,1.0f);
		float fogFactor = computeFog();
		vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
		fColor = mix(fogColor, newColor, fogFactor);
		//fColor = color;
	}
    
	//float fogFactor = computeFog();
	//vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	//if(fog == 1)
		//fColor = mix(fogColor, color, fogFactor);
	//else{
		//fColor = color;
	//}
	
}
