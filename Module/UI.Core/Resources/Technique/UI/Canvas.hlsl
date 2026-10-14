// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Embedded://Shader/Font.hlsl"

cbuffer cb_Global : register(b0)
{
    float4x4 u_Camera;
};

/// What a run of glyphs reads besides its atlas, one per effect drawn in the frame.
struct UIMaterial
{
    float2       Range;
    float2       Padding;
    ZyFontEffect Effect;
};

cbuffer cb_Material : register(b2)
{
    UIMaterial u_Materials[UI_MATERIALS];
};

cbuffer cb_Instance : register(b3)
{
    // Two rows a transform, the first always the identity; a row turns and moves x or y as (x, y, 1) dotted with it.
    float4 u_Transforms[UI_TRANSFORMS * 2];
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// How a quad's edge is cut, in the lowest two bits of its code.
#define UI_PLAIN    0u
#define UI_ROUNDED  1u
#define UI_SHADOW   2u
#define UI_INSET    3u

// What a quad is filled with, in the next two bits.
#define UI_COLOUR   0u
#define UI_IMAGE    1u
#define UI_LAYERED  2u
#define UI_GLYPH    3u

// Set when the colour runs from the tint to the other colour across the box.
#define UI_GRADIENT 16u

// Where the corners stand, in bits 13 and 14, as at the part drawn, around a line, or around a convex figure.
#define UI_BOX      0u
#define UI_SEGMENT  1u
#define UI_POLYGON  2u

// Set when a texture shows its colour alone, its alpha taken as whole.
#define UI_OPAQUE   32768u

struct vs_Input
{
    float4 Rect    : SLOT0;  // the part drawn in pixels before the transform, a line's points, a figure's first two
    float4 Box     : SLOT1;  // the box it rounds, fades and maps the texture over, a line's widths, a figure's last two
    float4 Source  : SLOT2;  // the texture coordinates of the box's left, top, right and bottom edges
    float4 Tint    : SLOT3;
    float4 Other   : SLOT4;  // the colour a gradient ends at
    uint4  Shape   : SLOT5;  // radius, outline or material, blur, angle or slice, in subpixel steps, and the code
    float  Depth   : SLOT6;  // how far it stands, later quads nearer, which only a canvas drawing with depth reads
    uint   Corner  : SV_VertexID;
};

struct ps_Input
{
    float4 Position : SV_POSITION;
    float2 Texture  : TEXCOORD0;  // or a figure's distances to its first two edges
    float4 Color    : COLOR0;
    float2 Local    : TEXCOORD1;  // or a figure's distances to its last two edges
    nointerpolation uint2  Code  : TEXCOORD2;  // the code, and the material a glyph reads or the slice a layer reads
    nointerpolation float4 Edge  : TEXCOORD3;  // the half size less the radius, the radius, and the outline
    nointerpolation float  Fade  : TEXCOORD4;  // what the distance is scaled by across a blur
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Colour
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

float3 sRGBDecode(float3 Color)
{
    const float3 Lower = Color / 12.92;
    const float3 Upper = pow(abs((Color + 0.055) / 1.055), 2.4);

    return lerp(Lower, Upper, step(0.04045, Color));
}

float3 sRGBEncode(float3 Color)
{
    const float3 Lower = Color * 12.92;
    const float3 Upper = 1.055 * pow(abs(Color), 1.0 / 2.4) - 0.055;

    return lerp(Lower, Upper, step(0.0031308, Color));
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef   VERTEX_SHADER

// The outward normal of the edge from one corner of a figure to the next and the distance it stands at from the origin,
// or no normal and a distance no pixel ever reaches for an edge of no length, as a triangle's fourth.
float3 Border(float2 From, float2 To, float2 Middle)
{
    const float2 Edge   = To - From;
    const float  Length = length(Edge);

    if (Length < 1.0e-4)
    {
        return float3(0.0, 0.0, 1.0e6);
    }

    // Either way round the figure, the normal points away from its middle.
    float2 Normal = float2(Edge.y, -Edge.x) / Length;
    Normal = (dot(Normal, Middle - From) > 0.0) ? -Normal : Normal;

    return float3(Normal, dot(Normal, From));
}

// The normal of an edge, or of the one beside it when the edge has no length.
float2 Facing(float3 Edge, float3 Beside)
{
    return (dot(Edge.xy, Edge.xy) > 0.5) ? Edge.xy : Beside.xy;
}

// A line stands as a box turned along it, its ends rounded by the radius, so the shader cuts it as it cuts a box.
void Segment(vs_Input Input, float2 Along, inout ps_Input Result, out float2 Position)
{
    const float2 From   = Input.Rect.xy;
    const float2 To     = Input.Rect.zw;
    const float2 Delta  = To - From;
    const float  Length = length(Delta);
    const float2 Axis   = (Length > 1.0e-6) ? Delta / Length : float2(1.0, 0.0);
    const float2 Normal = float2(-Axis.y, Axis.x);

    // Half its length and its reach past each point along it, half its width across it.
    const float2 Half  = float2(Length * 0.5 + Input.Box.y, Input.Box.x);
    const float  Round = Input.Box.z;

    // The quad stands a pixel past the edge on every side, so the edge fades out whole.
    const float2 Local = (Along * 2.0 - 1.0) * (Half + 1.0);

    Position = (From + To) * 0.5 + Axis * Local.x + Normal * Local.y;

    Result.Texture = float2(0.0, 0.0);
    Result.Local   = Local;
    Result.Edge    = float4(Half - Round, Round, 1.0e30);
    Result.Fade    = 1.0;
}

// A convex figure stands as its corners, each pushed a pixel out past both its edges, with its distance to every edge.
void Polygon(vs_Input Input, uint Corner, inout ps_Input Result, out float2 Position)
{
    // Around the figure the corners run the area's two, then the box's last and its first, as the triangles read them.
    const float2 First  = Input.Rect.xy;
    const float2 Second = Input.Rect.zw;
    const float2 Third  = Input.Box.zw;
    const float2 Fourth = Input.Box.xy;
    const float2 Middle = (First + Second + Third + Fourth) * 0.25;

    const float3 Edge0 = Border(First,  Second, Middle);
    const float3 Edge1 = Border(Second, Third,  Middle);
    const float3 Edge2 = Border(Third,  Fourth, Middle);
    const float3 Edge3 = Border(Fourth, First,  Middle);

    // The corner of this vertex, between the edge before it and the edge after it.
    float2 Point;
    float2 Before;
    float2 After;

    if (Corner == 0u)
    {
        Point  = First;
        Before = Facing(Edge3, Edge2);
        After  = Facing(Edge0, Edge1);
    }
    else if (Corner == 1u)
    {
        Point  = Second;
        Before = Facing(Edge0, Edge3);
        After  = Facing(Edge1, Edge2);
    }
    else if (Corner == 3u)
    {
        Point  = Third;
        Before = Facing(Edge1, Edge0);
        After  = Facing(Edge2, Edge3);
    }
    else
    {
        Point  = Fourth;
        Before = Facing(Edge2, Edge1);
        After  = Facing(Edge3, Edge0);
    }

    // Pushed out along the two normals, a pixel past each edge, held back at a corner so sharp it would reach far.
    Position = Point + (Before + After) / max(1.0 + dot(Before, After), 0.02);

    // A distance runs straight across the quad, so each one is worked out at the corners and carried between them.
    Result.Texture = float2(dot(Edge0.xy, Position) - Edge0.z, dot(Edge1.xy, Position) - Edge1.z);
    Result.Local   = float2(dot(Edge2.xy, Position) - Edge2.z, dot(Edge3.xy, Position) - Edge3.z);
    Result.Edge    = float4(0.0, 0.0, 0.0, 1.0e30);
    Result.Fade    = 1.0;
}

ps_Input main(vs_Input Input)
{
    // Each form fills in what it reads, so what another form leaves alone starts at nought.
    ps_Input Result = (ps_Input) 0;

    const uint Code  = Input.Shape.w;
    const uint Cut   = Code & 3u;
    const uint Paint = (Code >> 2u) & 3u;
    const uint Form  = (Code >> 13u) & 3u;

    // The four corners of a quad are numbered across and then down.
    const float2 Along = float2(Input.Corner & 1u, Input.Corner >> 1u);

    // A gradient's line passes through the middle at its angle, as long as CSS makes it, so it ends on two corners.
    float4 Color = Input.Tint;
    float2 Position;

    [branch]
    if (Form == UI_BOX)
    {
        Position = lerp(Input.Rect.xy, Input.Rect.zw, Along);

        // A part drawn smaller than its box, cut by a clip or one side of an outline, reads the box where it lies in it.
        const float2 Size   = Input.Box.zw - Input.Box.xy;
        const float2 Half   = Size * 0.5;
        const float2 Within = (Position - Input.Box.xy) / max(Size, 1.0e-6);

        Result.Texture = lerp(Input.Source.xy, Input.Source.zw, Within);
        Result.Local   = Position - (Input.Box.xy + Half);

        if (Code & UI_GRADIENT)
        {
            float Sine;
            float Cosine;
            sincos(float(Input.Shape.z) * (6.28318530718 / 65536.0), Sine, Cosine);

            const float Length = abs(Size.x * Sine) + abs(Size.y * Cosine);
            const float Ratio  = (Length > 0.0) ? dot(Result.Local, float2(Sine, -Cosine)) / Length + 0.5 : 0.5;

            Color = lerp(Input.Tint, Input.Other, saturate(Ratio));
        }

        // The lengths a cut reads are worked out once a quad: a shadow stands its blur inside the quad it was grown to.
        const float  Blur   = max(float(Input.Shape.z) * UI_SUBPIXEL, UI_SUBPIXEL);
        const float2 Inside = (Cut == UI_SHADOW) ? max(Half - Blur, 0.0) : Half;
        const float  Radius = min(float(Input.Shape.x) * UI_SUBPIXEL, min(Inside.x, Inside.y));

        // An outline keeps the band its thickness reaches in from the edge; none reaches past every pixel.
        const float  Line   = (Cut == UI_ROUNDED && Input.Shape.y > 0u) ? float(Input.Shape.y) * UI_SUBPIXEL : 1.0e30;

        Result.Edge = float4(Inside - Radius, Radius, Line);
        Result.Fade = (Cut == UI_SHADOW) ? 0.5 / Blur : 1.0 / Blur;
    }
    else if (Form == UI_SEGMENT)
    {
        Segment(Input, Along, Result, Position);
    }
    else
    {
        Polygon(Input, Input.Corner & 3u, Result, Position);
    }

    const uint   Slot  = ((Code >> 5u) & (UI_TRANSFORMS - 1u)) * 2u;
    const float2 Moved = float2(dot(u_Transforms[Slot].xyz, float3(Position, 1.0)), dot(u_Transforms[Slot + 1u].xyz, float3(Position, 1.0)));

    Result.Position   = mul(u_Camera, float4(Moved, 0.0f, 1.0f));
    Result.Position.z = Input.Depth * Result.Position.w;
    Result.Code     = uint2(Code, (Paint == UI_LAYERED) ? Input.Shape.z : Input.Shape.y);

#if   defined(ENABLE_SRGB_ENCODE)
    // Stored as sRGB and blended as written, so a colour alone passes through; only what it multiplies is decoded.
    if (Paint != UI_COLOUR)
    {
        Color.rgb = sRGBDecode(Color.rgb);
    }
#elif defined(ENABLE_SRGB_TARGET)
    // A gradient blends its ends as written and is decoded where it is drawn; any other colour is decoded once here.
    if (!(Code & UI_GRADIENT))
    {
        Color.rgb = sRGBDecode(Color.rgb);
    }
#endif // ENABLE_SRGB_ENCODE

    Result.Color = Color;

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2D      t_Atlas   : register(t0);
Texture2D      t_Image   : register(t1);
Texture2DArray t_Layer   : register(t2);

// A sampler for each texture, so an image or a layer may be read texel by texel while the atlas stays smooth.
SamplerState   s_Atlas   : register(s0);
SamplerState   s_Image   : register(s1);
SamplerState   s_Layer   : register(s2);

float Coverage(ps_Input Input, uint Cut)
{
    // The distance to a rounded box, negative inside it, in pixels.
    const float2 Corner   = abs(Input.Local) - Input.Edge.xy;
    const float  Distance = length(max(Corner, 0.0)) + min(max(Corner.x, Corner.y), 0.0) - Input.Edge.z;

    // Cut at the edge, half a pixel either side of it.
    float Result = saturate(0.5 - Distance);

    [branch]
    if (Cut == UI_ROUNDED)
    {
        // An outline keeps only the band of its thickness just inside the edge.
        Result -= saturate(0.5 - (Distance + Input.Edge.w));
    }
    else if (Cut == UI_SHADOW)
    {
        // Solid a blur inside the edge and gone a blur outside it, half faded on the edge itself.
        const float Ramp = saturate(Distance * Input.Fade + 0.5);
        Result = 1.0 - Ramp * Ramp * (3.0 - 2.0 * Ramp);
    }
    else if ((Input.Code.x >> 13u) & UI_POLYGON)
    {
        // A convex figure ends at whichever of its edges a pixel stands farthest past, each distance carried across it.
        const float Farthest = max(max(Input.Texture.x, Input.Texture.y), max(Input.Local.x, Input.Local.y));
        Result = saturate(0.5 - Farthest);
    }
    else
    {
        // Cut at the edge as a box is, then solid there and gone a blur inward.
        const float Ramp = saturate(-Distance * Input.Fade);
        Result *= 1.0 - Ramp * Ramp * (3.0 - 2.0 * Ramp);
    }
    return Result;
}

float4 Shade(ps_Input Input, uint Paint)
{
    [branch]
    if (Paint == UI_GLYPH)
    {
        const UIMaterial Material = u_Materials[Input.Code.y];
        const ZyFontEffect Effect = Material.Effect;

        const float4 Sample = t_Atlas.Sample(s_Atlas, Input.Texture);
        const float  Spread = ZyFontSpread(Input.Texture, Material.Range);

        float4 Shaded;

        // Text with no outline is only its stroke, which spares every glyph pixel the outline's fade.
        [branch]
        if (Effect.OutsetWidth == 0.0 && Effect.OutsetBias == 0.0)
        {
            const float Distance = lerp(ZyFontMedian(Sample.rgb), Sample.a, Effect.InsetRoundness) - Effect.InsetThreshold;

            Shaded = Input.Color * saturate(Spread * Distance + 0.5 + Effect.OutsetOffset);
        }
        else
        {
            Shaded = ZyFontShade(Effect, Sample, Spread, Input.Color);
        }

        // The font shades premultiplied, while the canvas blends straight alpha.
        return float4(Shaded.rgb / max(Shaded.a, 0.0001), Shaded.a);
    }
    else if (Paint == UI_IMAGE)
    {
        float4 Sample = t_Image.Sample(s_Image, Input.Texture);

        // A target whose alpha holds data shows its colour alone.
        Sample.a = (Input.Code.x & UI_OPAQUE) ? 1.0 : Sample.a;
        return Input.Color * Sample;
    }
    else
    {
        // An array is read at the slice its quad carries, which sprite art stored as an array of one leaves at the first.
        float4 Sample = t_Layer.Sample(s_Layer, float3(Input.Texture, float(Input.Code.y)));

        Sample.a = (Input.Code.x & UI_OPAQUE) ? 1.0 : Sample.a;
        return Input.Color * Sample;
    }
}

float4 main(ps_Input Input) : SV_Target
{
    const uint Code  = Input.Code.x;
    const uint Paint = (Code >> 2u) & 3u;
    const uint Cut   = Code & 3u;

    // A colour samples nothing, so a plain box costs no more than its colour.
    float4 Result = Input.Color;

    [branch]
    if (Paint != UI_COLOUR)
    {
        Result = Shade(Input, Paint);
    }

    // A plain box covers the whole of its quad, so only one with a shaped edge works out its distance.
    [branch]
    if (Cut != UI_PLAIN)
    {
        Result.a *= Coverage(Input, Cut);
    }

#if   defined(ENABLE_SRGB_ENCODE)
    if (Paint != UI_COLOUR)
    {
        Result.rgb = sRGBEncode(Result.rgb);
    }
#elif defined(ENABLE_SRGB_TARGET)
    if (Code & UI_GRADIENT)
    {
        Result.rgb = sRGBDecode(Result.rgb);
    }
#endif // ENABLE_SRGB_ENCODE

    return Result;
}

#endif // FRAGMENT_SHADER
