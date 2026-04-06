#version 450 

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 lightPos;
} ubo;

// Specialization constant to make each pipeline variant unique
layout (constant_id = 0) const int VARIANT_ID = 0;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) flat out vec3 outFlatNormal;

// Compute a rotation matrix around an arbitrary axis by angle (radians)
mat3 rotationMatrix(vec3 axis, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;
	vec3 a = normalize(axis);
	return mat3(
		oc * a.x * a.x + c,       oc * a.x * a.y - a.z * s, oc * a.z * a.x + a.y * s,
		oc * a.x * a.y + a.z * s, oc * a.y * a.y + c,       oc * a.y * a.z - a.x * s,
		oc * a.z * a.x - a.y * s, oc * a.y * a.z + a.x * s, oc * a.z * a.z + c
	);
}

// Schlick Fresnel approximation
float fresnelSchlick(float cosTheta, float F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() 
{
	// Use VARIANT_ID to create unique shader math per pipeline
	float variantScale = 1.0 + float(VARIANT_ID) * 0.001;
	float variantPhase = float(VARIANT_ID) * 0.0628318; // ~2*PI/100 per variant
	float variantBias  = float(VARIANT_ID) * 0.0005;

	vec4 pos = vec4(inPos.xyz * variantScale, 1.0);

	// Transform to world space
	vec4 worldPos = ubo.model * pos;
	mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
	vec3 worldNormal = normalize(normalMatrix * inNormal);

	// Compute tangent and bitangent from the world normal
	vec3 tangent = normalize(cross(worldNormal, vec3(0.0, 1.0, 0.0)));
	// If normal is nearly parallel to up, fall back to a different reference
	if (length(cross(worldNormal, vec3(0.0, 1.0, 0.0))) < 0.001)
		tangent = normalize(cross(worldNormal, vec3(1.0, 0.0, 0.0)));
	vec3 bitangent = normalize(cross(worldNormal, tangent));

	// Construct TBN matrix for tangent-space transforms
	mat3 TBN = mat3(tangent, bitangent, worldNormal);

	// Slight vertex displacement along the normal based on a sine wave pattern (varied per variant)
	float displacementFreq = 10.0 + float(VARIANT_ID) * 0.5;
	float displacementAmp = 0.002 + variantBias;
	float displacement = sin(worldPos.x * displacementFreq + variantPhase) * cos(worldPos.z * displacementFreq + variantPhase) * displacementAmp;
	worldPos.xyz += worldNormal * displacement;

	// Perturb the normal based on displacement gradient (analytical derivative)
	float dx = cos(worldPos.x * displacementFreq) * displacementFreq * cos(worldPos.z * displacementFreq) * displacementAmp;
	float dz = sin(worldPos.x * displacementFreq) * (-sin(worldPos.z * displacementFreq)) * displacementFreq * displacementAmp;
	vec3 perturbedNormal = normalize(worldNormal + TBN * vec3(-dx, -dz, 0.0));

	// Apply a subtle rotation to the normal based on view direction for a warped look
	vec3 viewDir = normalize(-worldPos.xyz);
	float angle = acos(clamp(dot(viewDir, perturbedNormal), -1.0, 1.0)) * 0.05;
	mat3 rot = rotationMatrix(cross(viewDir, perturbedNormal), angle);
	perturbedNormal = normalize(rot * perturbedNormal);

	// Lighting setup
	vec3 lPos = ubo.lightPos.xyz;
	vec3 lightVec = lPos - worldPos.xyz;
	float lightDist = length(lightVec);
	vec3 lightDir = lightVec / lightDist;

	// Quadratic attenuation
	float attenuation = 1.0 / (1.0 + 0.09 * lightDist + 0.032 * lightDist * lightDist);

	// Half-vector for Blinn-Phong
	vec3 halfVec = normalize(lightDir + viewDir);
	float NdotH = max(dot(perturbedNormal, halfVec), 0.0);
	float NdotL = max(dot(perturbedNormal, lightDir), 0.0);
	float NdotV = max(dot(perturbedNormal, viewDir), 0.0);

	// Fresnel term on the color
	float fresnel = fresnelSchlick(NdotV, 0.04);

	// Compute a rim lighting effect
	float rim = 1.0 - NdotV;
	rim = smoothstep(0.4, 1.0, rim);
	vec3 rimColor = inColor * rim * 0.3;

	// Specular highlight (Blinn-Phong) — power varies per variant
	float specPower = 64.0 + float(VARIANT_ID) * 2.0;
	float spec = pow(NdotH, specPower) * attenuation;

	// Diffuse with attenuation
	float diffuse = NdotL * attenuation;

	// Combine all lighting into the output color
	vec3 ambient = inColor * 0.15;
	vec3 diffuseColor = inColor * diffuse * (1.0 - fresnel);
	vec3 specColor = vec3(1.0) * spec * fresnel;
	outColor = ambient + diffuseColor + specColor + rimColor;

	// Final clip-space position (with displaced world position)
	gl_Position = ubo.projection * worldPos;

	// Pass outputs to fragment shader
	outNormal = perturbedNormal;
	outLightVec = lightVec;
	outViewVec = viewDir;
	outFlatNormal = worldNormal;
}