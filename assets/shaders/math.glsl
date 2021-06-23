float encodeNormal(in vec3 normal) {
    normal = (normal + 1.0f) * 0.5f;
    uvec3 unormal = uvec3(normal * 1023.0f);
    uint ret = (unormal.r << 20) | (unormal.g << 10) | unormal.b;
    return uintBitsToFloat(ret);
}

vec3 decodeNormal(in float fnormal) {
    uint normal = floatBitsToUint(fnormal);
    uint r = (normal >> 20);
    uint g = (normal >> 10) & 1023u;
    uint b = normal & 1023u;
    vec3 ret = vec3(r,g,b) / 1024.0f;
    return normalize(ret * 2.0f - 1.0f);
}