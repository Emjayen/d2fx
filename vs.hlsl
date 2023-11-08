#include "common.hlsli"



float2 screen_size;



void main
(
	in vs_input vs_in,
    out ps_input ps_in
)
{
    float ndc_x = (vs_in.pos.x / screen_size.x) * 2 - 1;
    float ndc_y = -((vs_in.pos.y / screen_size.y) * 2 - 1);
    
    ps_in.pos = float4(ndc_x, ndc_y, vs_in.z, 1);
    ps_in.uv = vs_in.uv;
    ps_in.color = vs_in.color;
}