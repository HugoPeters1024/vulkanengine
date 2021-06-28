float percievedBrightness(vec3 rgbColor)
{
    return dot(rgbColor, vec3(0.299, 0.587, 0.114));
}