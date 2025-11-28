#version 460 core

layout (location = 0) out vec4 color;

layout (location = 0) in vec4 out_color;
layout (location = 1) in vec2 out_uv;
layout (location = 2) in float in_shadow;
layout (location = 3) in float in_eye;
layout (location = 4) in float in_is_local;   // <--- NOVO

void main() {
    float l = length(out_uv * 2.0 - 1.0);
    float f = (1.0 - l);
    float o = smoothstep(0, 0.07, f);
    if (o == 0) discard;

    float shadow_strength = f * f * f;
    vec4 shadow_col = out_color * (f + shadow_strength);

    float v = pow(max(0, min(1, 1 - abs(out_uv.y - 0.5) / 0.5)), .35);
    float v2 = 1 - (l / 2);
    v += (v2 - v) * 0.375;
    v *= 1.22 - 0.44 * out_uv.x / 7;

    vec4 eye_col = out_color;
    eye_col.a *= o;

    vec4 base_col = mix(vec4(out_color.rgb * v, out_color.a * o), shadow_col, in_shadow);
    vec4 final_col = mix(base_col, eye_col, in_eye);

    /*
        NOVO: deixar o jogador local mais brilhante / destacado.
        in_is_local = 1 → jogador local
        in_is_local = 0 → outros jogadores
    */
    if (in_is_local > 0.5) {
        final_col.rgb *= 1.15;      // mais brilho
        final_col.a *= 1.10;        // levemente mais opaco
    }

    color = final_col;
}
