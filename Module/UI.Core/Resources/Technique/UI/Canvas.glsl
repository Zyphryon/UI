// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Embedded://Shader/Font.glsl"

layout(std140, binding = 0) uniform cb_Global
{
    mat4 u_Camera;
};

// What a run of glyphs reads besides its atlas, one per effect drawn in the frame.
struct UIMaterial
{
    vec2         Range;
    vec2         Padding;
    ZyFontEffect Effect;
};

layout(std140, binding = 2) uniform cb_Material
{
    UIMaterial u_Materials[UI_MATERIALS];
};

// Two rows a transform, the first always the identity; a row turns and moves x or y as (x, y, 1) dotted with it.
layout(std140, binding = 3) uniform cb_Instance
{
    vec4 u_Transforms[UI_TRANSFORMS * 2];
};

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

// Set when a texture holds its colour already scaled by its alpha.
#define UI_PREMULTIPLIED 4096u

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Colour
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

vec3 sRGBDecode(vec3 Color)
{
    vec3 Lower = Color / 12.92;
    vec3 Upper = pow(abs((Color + 0.055) / 1.055), vec3(2.4));

    return mix(Lower, Upper, step(vec3(0.04045), Color));
}

vec3 sRGBEncode(vec3 Color)
{
    vec3 Lower = Color * 12.92;
    vec3 Upper = 1.055 * pow(abs(Color), vec3(1.0 / 2.4)) - 0.055;

    return mix(Lower, Upper, step(vec3(0.0031308), Color));
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef   VERTEX_SHADER

// The driver binds each attribute at the location the technique's layout gives it, never by name.
layout(location = 0) in vec4  a_Rect;    // the part drawn in pixels before the transform, a line's points, a figure's first two
layout(location = 1) in vec4  a_Box;     // the box it rounds, fades and maps the texture over, a line's widths, a figure's last two
layout(location = 2) in vec4  a_Source;  // the texture coordinates of the box's left, top, right and bottom edges
layout(location = 3) in vec4  a_Tint;
layout(location = 4) in vec4  a_Other;   // the colour a gradient ends at
layout(location = 5) in uvec4 a_Shape;   // radius, outline or material, blur, angle or slice, in subpixel steps, and the code
layout(location = 6) in float a_Depth;   // how far it stands, later quads nearer, which only a canvas drawing with depth reads

out vec2 v_Texture;      // or a figure's distances to its first two edges
out vec4 v_Color;
out vec2 v_Local;        // or a figure's distances to its last two edges
flat out uvec2 v_Code;   // the code, and the material a glyph reads or the slice a layer reads
flat out vec4  v_Edge;   // the half size less the radius, the radius, and the outline
flat out float v_Fade;   // what the distance is scaled by across a blur

// The outward normal of the edge from one corner of a figure to the next and the distance it stands at from the origin,
// or no normal and a distance no pixel ever reaches for an edge of no length, as a triangle's fourth.
vec3 Border(vec2 From, vec2 To, vec2 Middle)
{
    vec2  Edge   = To - From;
    float Length = length(Edge);

    if (Length < 1.0e-4)
    {
        return vec3(0.0, 0.0, 1.0e6);
    }

    // Either way round the figure, the normal points away from its middle.
    vec2 Normal = vec2(Edge.y, -Edge.x) / Length;
    Normal = (dot(Normal, Middle - From) > 0.0) ? -Normal : Normal;

    return vec3(Normal, dot(Normal, From));
}

// The normal of an edge, or of the one beside it when the edge has no length.
vec2 Facing(vec3 Edge, vec3 Beside)
{
    return (dot(Edge.xy, Edge.xy) > 0.5) ? Edge.xy : Beside.xy;
}

// A line stands as a box turned along it, its ends rounded by the radius, so the shader cuts it as it cuts a box.
vec2 Segment(vec2 Along)
{
    vec2  From   = a_Rect.xy;
    vec2  To     = a_Rect.zw;
    vec2  Delta  = To - From;
    float Length = length(Delta);
    vec2  Axis   = (Length > 1.0e-6) ? Delta / Length : vec2(1.0, 0.0);
    vec2  Normal = vec2(-Axis.y, Axis.x);

    // Half its length and its reach past each point along it, half its width across it.
    vec2  Half  = vec2(Length * 0.5 + a_Box.y, a_Box.x);
    float Round = a_Box.z;

    // The quad stands a pixel past the edge on every side, so the edge fades out whole.
    vec2 Local = (Along * 2.0 - 1.0) * (Half + 1.0);

    v_Texture = vec2(0.0, 0.0);
    v_Local   = Local;
    v_Edge    = vec4(Half - Round, Round, 1.0e30);
    v_Fade    = 1.0;

    return (From + To) * 0.5 + Axis * Local.x + Normal * Local.y;
}

// A convex figure stands as its corners, each pushed a pixel out past both its edges, with its distance to every edge.
vec2 Polygon(uint Corner)
{
    // Around the figure the corners run the area's two, then the box's last and its first, as the triangles read them.
    vec2 First  = a_Rect.xy;
    vec2 Second = a_Rect.zw;
    vec2 Third  = a_Box.zw;
    vec2 Fourth = a_Box.xy;
    vec2 Middle = (First + Second + Third + Fourth) * 0.25;

    vec3 Edge0 = Border(First,  Second, Middle);
    vec3 Edge1 = Border(Second, Third,  Middle);
    vec3 Edge2 = Border(Third,  Fourth, Middle);
    vec3 Edge3 = Border(Fourth, First,  Middle);

    // The corner of this vertex, between the edge before it and the edge after it.
    vec2 Point;
    vec2 Before;
    vec2 After;

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
    vec2 Position = Point + (Before + After) / max(1.0 + dot(Before, After), 0.02);

    // A distance runs straight across the quad, so each one is worked out at the corners and carried between them.
    v_Texture = vec2(dot(Edge0.xy, Position) - Edge0.z, dot(Edge1.xy, Position) - Edge1.z);
    v_Local   = vec2(dot(Edge2.xy, Position) - Edge2.z, dot(Edge3.xy, Position) - Edge3.z);
    v_Edge    = vec4(0.0, 0.0, 0.0, 1.0e30);
    v_Fade    = 1.0;

    return Position;
}

void main()
{
    uint Code  = a_Shape.w;
    uint Cut   = Code & 3u;
    uint Paint = (Code >> 2u) & 3u;
    uint Form  = (Code >> 13u) & 3u;

    // The four corners of a quad are numbered across and then down.
    uint Corner = uint(gl_VertexID) & 3u;
    vec2 Along  = vec2(float(Corner & 1u), float(Corner >> 1u));

    // A gradient's line passes through the middle at its angle, as long as CSS makes it, so it ends on two corners.
    vec4 Color = a_Tint;
    vec2 Position;

    if (Form == UI_BOX)
    {
        Position = mix(a_Rect.xy, a_Rect.zw, Along);

        // A part drawn smaller than its box, cut by a clip or one side of an outline, reads the box where it lies in it.
        vec2 Size   = a_Box.zw - a_Box.xy;
        vec2 Half   = Size * 0.5;
        vec2 Within = (Position - a_Box.xy) / max(Size, vec2(1.0e-6));

        v_Texture = mix(a_Source.xy, a_Source.zw, Within);
        v_Local   = Position - (a_Box.xy + Half);

        if ((Code & UI_GRADIENT) != 0u)
        {
            float Turn   = float(a_Shape.z) * (6.28318530718 / 65536.0);
            float Sine   = sin(Turn);
            float Cosine = cos(Turn);

            float Length = abs(Size.x * Sine) + abs(Size.y * Cosine);
            float Ratio  = (Length > 0.0) ? dot(v_Local, vec2(Sine, -Cosine)) / Length + 0.5 : 0.5;

            Color = mix(a_Tint, a_Other, clamp(Ratio, 0.0, 1.0));
        }

        // The lengths a cut reads are worked out once a quad: a shadow stands its blur inside the quad it was grown to.
        float Blur   = max(float(a_Shape.z) * UI_SUBPIXEL, UI_SUBPIXEL);
        vec2  Inside = (Cut == UI_SHADOW) ? max(Half - Blur, 0.0) : Half;
        float Radius = min(float(a_Shape.x) * UI_SUBPIXEL, min(Inside.x, Inside.y));

        // An outline keeps the band its thickness reaches in from the edge; none reaches past every pixel.
        float Line   = (Cut == UI_ROUNDED && a_Shape.y > 0u) ? float(a_Shape.y) * UI_SUBPIXEL : 1.0e30;

        v_Edge = vec4(Inside - Radius, Radius, Line);
        v_Fade = (Cut == UI_SHADOW) ? 0.5 / Blur : 1.0 / Blur;
    }
    else if (Form == UI_SEGMENT)
    {
        Position = Segment(Along);
    }
    else
    {
        Position = Polygon(Corner);
    }

    int  Slot  = int((Code >> 5u) & uint(UI_TRANSFORMS - 1)) * 2;
    vec2 Moved = vec2(dot(u_Transforms[Slot].xyz, vec3(Position, 1.0)), dot(u_Transforms[Slot + 1].xyz, vec3(Position, 1.0)));

    gl_Position   = u_Camera * vec4(Moved, 0.0, 1.0);
    gl_Position.z = a_Depth * gl_Position.w;
    v_Code      = uvec2(Code, (Paint == UI_LAYERED) ? a_Shape.z : a_Shape.y);

#if   defined(ENABLE_SRGB_ENCODE)
    // Stored as sRGB and blended as written, so a colour alone passes through; only what it multiplies is decoded.
    if (Paint != UI_COLOUR)
    {
        Color.rgb = sRGBDecode(Color.rgb);
    }
#elif defined(ENABLE_SRGB_TARGET)
    // A gradient blends its ends as written and is decoded where it is drawn; any other colour is decoded once here.
    if ((Code & UI_GRADIENT) == 0u)
    {
        Color.rgb = sRGBDecode(Color.rgb);
    }
#endif // ENABLE_SRGB_ENCODE

    v_Color = Color;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 out_Color;

// Each texture unit holds a sampler of its own, so an image or a layer may be read texel by texel while the atlas stays smooth.
layout(binding = 0) uniform sampler2D      t_Atlas;
layout(binding = 1) uniform sampler2D      t_Image;
layout(binding = 2) uniform sampler2DArray t_Layer;

in vec2 v_Texture;
in vec4 v_Color;
in vec2 v_Local;
flat in uvec2 v_Code;
flat in vec4  v_Edge;
flat in float v_Fade;

float Coverage(uint Cut)
{
    // The distance to a rounded box, negative inside it, in pixels.
    vec2  Corner   = abs(v_Local) - v_Edge.xy;
    float Distance = length(max(Corner, 0.0)) + min(max(Corner.x, Corner.y), 0.0) - v_Edge.z;

    // Cut at the edge, half a pixel either side of it.
    float Result = clamp(0.5 - Distance, 0.0, 1.0);

    if (Cut == UI_ROUNDED)
    {
        // An outline keeps only the band of its thickness just inside the edge.
        Result -= clamp(0.5 - (Distance + v_Edge.w), 0.0, 1.0);
    }
    else if (Cut == UI_SHADOW)
    {
        // Solid a blur inside the edge and gone a blur outside it, half faded on the edge itself.
        float Ramp = clamp(Distance * v_Fade + 0.5, 0.0, 1.0);
        Result = 1.0 - Ramp * Ramp * (3.0 - 2.0 * Ramp);
    }
    else if (((v_Code.x >> 13u) & UI_POLYGON) != 0u)
    {
        // A convex figure ends at whichever of its edges a pixel stands farthest past, each distance carried across it.
        float Farthest = max(max(v_Texture.x, v_Texture.y), max(v_Local.x, v_Local.y));
        Result = clamp(0.5 - Farthest, 0.0, 1.0);
    }
    else
    {
        // Cut at the edge as a box is, then solid there and gone a blur inward.
        float Ramp = clamp(-Distance * v_Fade, 0.0, 1.0);
        Result *= 1.0 - Ramp * Ramp * (3.0 - 2.0 * Ramp);
    }
    return Result;
}

vec4 Shade(uint Paint)
{
    if (Paint == UI_GLYPH)
    {
        UIMaterial   Material = u_Materials[int(v_Code.y)];
        ZyFontEffect Effect   = Material.Effect;

        vec4  Sample = texture(t_Atlas, v_Texture);
        float Spread = ZyFontSpread(v_Texture, Material.Range);

        vec4 Shaded;

        // Text with no outline is only its stroke, which spares every glyph pixel the outline's fade.
        if (Effect.OutsetWidth == 0.0 && Effect.OutsetBias == 0.0)
        {
            float Distance = mix(ZyFontMedian(Sample.rgb), Sample.a, Effect.InsetRoundness) - Effect.InsetThreshold;

            Shaded = v_Color * clamp(Spread * Distance + 0.5 + Effect.OutsetOffset, 0.0, 1.0);
        }
        else
        {
            Shaded = ZyFontShade(Effect, Sample, Spread, v_Color);
        }

        // The font shades premultiplied, while the canvas blends straight alpha.
        return vec4(Shaded.rgb / max(Shaded.a, 0.0001), Shaded.a);
    }
    else if (Paint == UI_IMAGE)
    {
        vec4 Sample = texture(t_Image, v_Texture);

        // A target whose alpha holds data shows its colour alone.
        Sample.a = ((v_Code.x & UI_OPAQUE) != 0u) ? 1.0 : Sample.a;
        return v_Color * Sample;
    }
    else
    {
        // An array is read at the slice its quad carries, which sprite art stored as an array of one leaves at the first.
        vec4 Sample = texture(t_Layer, vec3(v_Texture, float(v_Code.y)));

        Sample.a = ((v_Code.x & UI_OPAQUE) != 0u) ? 1.0 : Sample.a;
        return v_Color * Sample;
    }
}

void main()
{
    uint Code  = v_Code.x;
    uint Paint = (Code >> 2u) & 3u;
    uint Cut   = Code & 3u;

    // A colour samples nothing, so a plain box costs no more than its colour.
    vec4 Result = v_Color;

    if (Paint != UI_COLOUR)
    {
        Result = Shade(Paint);
    }

    // A plain box covers the whole of its quad, so only one with a shaped edge works out its distance.
    float Cover = 1.0;

    if (Cut != UI_PLAIN)
    {
        Cover     = Coverage(Cut);
        Result.a *= Cover;
    }

#if   defined(ENABLE_SRGB_ENCODE)
    if (Paint != UI_COLOUR)
    {
        Result.rgb = sRGBEncode(Result.rgb);
    }
#elif defined(ENABLE_SRGB_TARGET)
    if ((Code & UI_GRADIENT) != 0u)
    {
        Result.rgb = sRGBDecode(Result.rgb);
    }
#endif // ENABLE_SRGB_ENCODE

    Result.rgb *= ((Code & UI_PREMULTIPLIED) != 0u) ? v_Color.a * Cover : Result.a;
    out_Color = Result;
}

#endif // FRAGMENT_SHADER
