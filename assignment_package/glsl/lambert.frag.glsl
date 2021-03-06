#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

//uniform vec4 u_Color; // The color with which to render this instance of geometry.
uniform sampler2D u_Texture;
uniform int u_Time;


// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
//in vec4 fs_Col;
in vec2 fs_UV;
in float fs_animate;


out vec4 out_Col; // This is the final output color that you will see on your
                  // screen for the pixel that is currently being processed.

const vec4 lightDir = normalize(vec4(0, 0, -1.0, 0));  // The direction of our virtual light, which is used to compute the shading of
                                        // the geometry in the fragment shader.

//make the sun rotate around x axis depending on the u_Time
vec3 rotateX(vec3 p, float a) {
    return vec3(p.x, cos(a) * p.y + -sin(a) *p.z, sin(a) * p.y +cos(a) * p.z);
}

float random1(vec3 p) {
    return fract(sin(dot(p,vec3(127.1, 311.7, 191.999)))
                 *43758.5453);
}

float mySmoothStep(float a, float b, float t) {
    t = smoothstep(0, 1, t);
    return mix(a, b, t);
}

float cubicTriMix(vec3 p) {
    vec3 pFract = fract(p);
    float llb = random1(floor(p) + vec3(0,0,0));
    float lrb = random1(floor(p) + vec3(1,0,0));
    float ulb = random1(floor(p) + vec3(0,1,0));
    float urb = random1(floor(p) + vec3(1,1,0));

    float llf = random1(floor(p) + vec3(0,0,1));
    float lrf = random1(floor(p) + vec3(1,0,1));
    float ulf = random1(floor(p) + vec3(0,1,1));
    float urf = random1(floor(p) + vec3(1,1,1));

    float mixLoBack = mySmoothStep(llb, lrb, pFract.x);
    float mixHiBack = mySmoothStep(ulb, urb, pFract.x);
    float mixLoFront = mySmoothStep(llf, lrf, pFract.x);
    float mixHiFront = mySmoothStep(ulf, urf, pFract.x);

    float mixLo = mySmoothStep(mixLoBack, mixLoFront, pFract.z);
    float mixHi = mySmoothStep(mixHiBack, mixHiFront, pFract.z);

    return mySmoothStep(mixLo, mixHi, pFract.y);
}

float fbm(vec3 p) {
    float amp = 0.5;
    float freq = 4.0;
    float sum = 0.f;
    for(int i = 0; i < 8; i++) {
        sum += cubicTriMix(p * freq) * amp;
        amp *= 0.5;
        freq *= 2.0;
    }
    return sum;
}

void main()
{

    //if water or lava, it animates by changing uv slightly
    vec2 uv = fs_UV;
        if (fs_animate != 0.0f) {
            if (fs_Nor[0] != 0.0f || fs_Nor[2] != 0.0f) {
                uv = vec2(fs_UV.x + mod(u_Time / 8000.f, 1.f / 16.f), fs_UV.y);
            } else {
                uv = vec2(fs_UV.x + mod(u_Time / 8000.f, 1.f / 16.f), fs_UV.y);
            }
        }

    //day and night light
    vec3 sunDir = rotateX(lightDir.xyz, u_Time * 0.05);
    vec3 diffuseLight = vec3(dot(normalize(fs_Nor), vec4(normalize(sunDir), 0.0)));
    diffuseLight = clamp(diffuseLight, 0, 1) * vec3(255, 255, 190) / 255.0;
    vec4 diffuseColor = texture(u_Texture, uv);
    vec3 ambientLight = vec3(0.5) * vec3(144, 96, 144) /255.0;

    out_Col = vec4(diffuseLight + ambientLight, 1) * diffuseColor;
}
