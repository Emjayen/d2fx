
struct vs_input
{
    int2 pos : POSITION;
    float z : DEPTH;
    int2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct ps_input
{
    noperspective float4 pos : SV_POSITION;
    noperspective float2 uv : TEXCOORD;
    noperspective float4 color : COLOR;
};

