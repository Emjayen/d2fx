/*
 * ps.hlsl
 *
 */
#include "common.hlsli"


Texture2D<uint> tex : register(t0);
Texture1D pal : register(t1);




float4 main
(
    in ps_input ps_in
) : SV_Target
{

    const uint idx = tex.Load(int3(ps_in.uv, 0));
    
    // Color key
    if(idx == 0)
        discard;
    
    // Fetch from B8G8R8AX color table.
    const float4 color = pal.Load(int2(idx, 0));
    
    float4 final_color = color * ps_in.color;
    
    return final_color;

}