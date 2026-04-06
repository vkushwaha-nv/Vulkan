#version 450
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) flat in vec3 inFlatNormal;

layout (constant_id = 0) const int LIGHTING_MODEL = 0;
layout (constant_id = 1) const int VARIANT_ID = 0;

layout (location = 0) out vec4 outFragColor;

// ---- Helper functions ----

// GGX/Trowbridge-Reitz normal distribution function
float distributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH2 = NdotH * NdotH;
	float denom = NdotH2 * (a2 - 1.0) + 1.0;
	return a2 / (3.14159265 * denom * denom + 0.0001);
}

// Schlick-GGX geometry function (single direction)
float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith's geometry function (both directions)
float geometrySmith(float NdotV, float NdotL, float roughness)
{
	return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel-Schlick with roughness (for environment/ambient term)
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Hash function for pseudo-random noise
float hash(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

// Smooth value noise
float valueNoise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f); // smoothstep
	float a = hash(i);
	float b = hash(i + vec2(1.0, 0.0));
	float c = hash(i + vec2(0.0, 1.0));
	float d = hash(i + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal Brownian Motion (fBm) — 5 octaves
float fbm(vec2 p)
{
	float value = 0.0;
	float amplitude = 0.5;
	float frequency = 1.0;
	for (int i = 0; i < 5; i++)
	{
		value += amplitude * valueNoise(p * frequency);
		frequency *= 2.0;
		amplitude *= 0.5;
	}
	return value;
}

// Filmic tone mapping (ACES approximation)
vec3 acesToneMap(vec3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// sRGB gamma correction
vec3 linearToSRGB(vec3 color)
{
	return pow(color, vec3(1.0 / 2.2));
}

void main() 
{
	vec3 N = normalize(inNormal);
	vec3 Nflat = normalize(inFlatNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float HdotV = max(dot(H, V), 0.0);

	// Per-variant tweaks derived from VARIANT_ID
	float variantF = float(VARIANT_ID);
	float variantNoise  = variantF * 0.37;       // noise coordinate offset
	float variantMetal  = fract(variantF * 0.07); // metallic varies 0..1
	float variantRough  = 0.1 + fract(variantF * 0.13) * 0.8; // roughness 0.1..0.9
	float variantTint   = variantF * 0.0628318;   // color hue rotation angle

	// Material properties — varied per variant
	float metallic = variantMetal;
	float roughness = variantRough;
	// Apply a hue rotation to albedo so each variant has a different color tint
	float cosT = cos(variantTint);
	float sinT = sin(variantTint);
	mat3 hueRotation = mat3(
		0.299 + 0.701*cosT + 0.168*sinT, 0.587 - 0.587*cosT + 0.330*sinT, 0.114 - 0.114*cosT - 0.497*sinT,
		0.299 - 0.299*cosT - 0.328*sinT, 0.587 + 0.413*cosT + 0.035*sinT, 0.114 - 0.114*cosT + 0.292*sinT,
		0.299 - 0.300*cosT + 1.250*sinT, 0.587 - 0.588*cosT - 1.050*sinT, 0.114 + 0.886*cosT - 0.203*sinT
	);
	vec3 albedo = clamp(hueRotation * inColor, 0.0, 1.0);

	// Generate procedural detail using fBm noise on the normal direction (offset per variant)
	vec2 noiseCoord = N.xy * (8.0 + variantF * 0.5) + N.yz * 4.0 + vec2(variantNoise);
	float noiseVal = fbm(noiseCoord);

	// Modulate roughness and albedo with noise for surface variation
	roughness = clamp(roughness + (noiseVal - 0.5) * 0.3, 0.05, 1.0);
	albedo = mix(albedo, albedo * (0.8 + 0.4 * noiseVal), 0.4);

	// Base reflectance (dielectrics ~0.04, metals use albedo)
	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	switch (LIGHTING_MODEL) {
		case 0: // PBR Cook-Torrance
		{
			// Cook-Torrance BRDF
			float D = distributionGGX(NdotH, roughness);
			float G = geometrySmith(NdotV, NdotL, roughness);
			vec3  F = fresnelSchlick(HdotV, F0);

			vec3 numerator = D * G * F;
			float denominator = 4.0 * NdotV * NdotL + 0.0001;
			vec3 specular = numerator / denominator;

			// Energy conservation
			vec3 kS = F;
			vec3 kD = (1.0 - kS) * (1.0 - metallic);

			// Irradiance (single directional light)
			vec3 radiance = vec3(1.0) * (1.0 / (1.0 + 0.05 * dot(inLightVec, inLightVec)));

			vec3 Lo = (kD * albedo / 3.14159265 + specular) * radiance * NdotL;

			// Ambient with Fresnel-weighted environment approximation
			vec3 Fa = fresnelSchlickRoughness(NdotV, F0, roughness);
			vec3 kDa = (1.0 - Fa) * (1.0 - metallic);
			vec3 ambient = kDa * albedo * 0.15 + Fa * 0.03;

			// Subsurface scattering approximation (wrap lighting)
			float wrap = 0.3;
			float scatterNdotL = max((dot(N, L) + wrap) / (1.0 + wrap), 0.0);
			vec3 subsurface = albedo * scatterNdotL * 0.1 * (1.0 - metallic);

			outFragColor = vec4(ambient + Lo + subsurface, 1.0);
			break;
		}
		case 1: // Advanced Toon with edge detection and multi-band
		{
			// Use flat vs smooth normal difference for edge detection
			float edgeFactor = 1.0 - abs(dot(N, Nflat));
			float edge = smoothstep(0.2, 0.4, edgeFactor);

			float intensity = dot(N, L);

			// Multi-band quantization with smooth transitions
			float bands = 6.0;
			float quantized = floor(intensity * bands) / bands;
			float nextBand = ceil(intensity * bands) / bands;
			float t = fract(intensity * bands);
			t = smoothstep(0.4, 0.6, t); // smooth band transitions
			float smoothBand = mix(quantized, nextBand, t);
			smoothBand = max(smoothBand, 0.05);

			vec3 warmColor = vec3(1.0, 0.85, 0.7);
			vec3 coolColor = vec3(0.3, 0.35, 0.6);
			vec3 litColor = mix(coolColor, warmColor, smoothBand) * albedo;

			// Specular highlight with sharp toon edge
			float specAngle = max(dot(reflect(-L, N), V), 0.0);
			float toonSpec = smoothstep(0.9, 0.92, specAngle);

			// Rim light
			float rim = 1.0 - NdotV;
			float toonRim = smoothstep(0.55, 0.65, rim);

			// Combine with edge darkening
			vec3 color = litColor * (1.0 - edge * 0.8);
			color += vec3(1.0) * toonSpec * 0.5;
			color += albedo * toonRim * 0.3;

			outFragColor.rgb = color;
			break;
		}
		case 2: // Subsurface scattering / translucency
		{
			// Forward-scattering term (light passing through the object)
			vec3 scatterDir = normalize(L + N * 0.6);
			float VdotS = pow(clamp(dot(V, -scatterDir), 0.0, 1.0), 4.0);
			vec3 scatter = albedo * VdotS * 0.6;

			// Wrapped diffuse for soft shading
			float wrap = 0.5;
			float wrappedDiffuse = max((dot(N, L) + wrap) / (1.0 + wrap), 0.0);

			// Back-lighting contribution
			float backLight = max(dot(-N, L), 0.0) * 0.3;

			// Thickness approximation from flat normal deviation
			float thickness = 1.0 - abs(dot(Nflat, V));
			thickness = pow(thickness, 2.0);

			// Blinn-Phong specular on top
			float spec = pow(NdotH, 32.0) * 0.4;

			// Ambient occlusion approximation from normal curvature
			float ao = 0.5 + 0.5 * dot(N, Nflat);

			vec3 diffuse = albedo * wrappedDiffuse;
			vec3 backLit = albedo * vec3(1.0, 0.4, 0.2) * backLight * thickness;

			outFragColor.rgb = (diffuse + scatter + backLit) * ao + vec3(spec);
			break;
		}
		case 3: // Full post-process style (greyscale + vignette + chromatic aberration simulation)
		{
			// Start with luminance
			float lum = dot(albedo, vec3(0.2126, 0.7152, 0.0722));

			// Apply contrast curve (S-curve)
			lum = lum * lum * (3.0 - 2.0 * lum); // smoothstep-like contrast
			lum = pow(lum, 0.9); // slight gamma tweak

			// Simulate chromatic aberration by splitting luminance with offsets
			float lumR = dot(albedo * 1.02, vec3(0.2126, 0.7152, 0.0722));
			float lumG = lum;
			float lumB = dot(albedo * 0.98, vec3(0.2126, 0.7152, 0.0722));
			vec3 chromaticGrey = vec3(lumR, lumG, lumB);

			// Vignette based on view angle (simulates lens darkening)
			float vignette = smoothstep(0.0, 0.7, NdotV);
			vignette = mix(0.3, 1.0, vignette);

			// Film grain noise
			float grain = (hash(gl_FragCoord.xy * 0.5) - 0.5) * 0.08;

			// Sharpening via Laplacian-style high-frequency boost
			float sharpAmount = 0.15;
			float highFreq = (noiseVal - 0.5) * sharpAmount;

			outFragColor.rgb = (chromaticGrey + grain + highFreq) * vignette;
			break;
		}
	}

	// --- Post-processing pipeline applied to all lighting models ---

	// Bloom approximation: add glow from bright areas
	float brightness = dot(outFragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	float bloomThreshold = 0.7;
	float bloomStrength = 0.15;
	vec3 bloom = max(outFragColor.rgb - vec3(bloomThreshold), vec3(0.0)) * bloomStrength;
	outFragColor.rgb += bloom;

	// Color grading — subtle warm/cool split toning
	vec3 shadows = vec3(0.05, 0.05, 0.15);  // cool shadows
	vec3 highlights = vec3(0.15, 0.1, 0.0);  // warm highlights
	float luminance = dot(outFragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	outFragColor.rgb += mix(shadows, highlights, luminance) * 0.2;

	// ACES filmic tone mapping
	outFragColor.rgb = acesToneMap(outFragColor.rgb * 1.4);

	// Gamma correction (linear → sRGB)
	outFragColor.rgb = linearToSRGB(outFragColor.rgb);

	outFragColor.a = 1.0;
}