float percievedBrightness(in vec3 rgbColor)
{
    return dot(rgbColor, vec3(0.299, 0.587, 0.114));
}

vec3 bloomFilter(in vec3 rgbColor)
{
    return clamp(rgbColor * (percievedBrightness(rgbColor) - 1.0f), 0.0f, 10.f);
}