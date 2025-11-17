#version 150

uniform sampler2DRect tex0;
uniform float time;
uniform vec2 resolution;

in vec2 vTexCoord;
out vec4 fragColor;

void main() {
    vec2 uv = vTexCoord;
    vec4 color = texture(tex0, uv * resolution);
    
    // Simple glow by sampling neighbors with slight blur
    float glowRadius = 2.0;
    vec4 glow = vec4(0.0);
    float samples = 0.0;
    
    for(float x = -glowRadius; x <= glowRadius; x += 1.0) {
        for(float y = -glowRadius; y <= glowRadius; y += 1.0) {
            vec2 offset = vec2(x, y);
            float dist = length(offset);
            if(dist <= glowRadius) {
                vec2 sampleUV = (uv + offset / resolution) * resolution;
                glow += texture(tex0, sampleUV) * (1.0 - dist / glowRadius);
                samples += 1.0;
            }
        }
    }
    
    glow /= samples;
    
    // Combine original color with glow
    vec4 finalColor = color + glow * 0.5;
    
    // Add subtle pulsing
    float pulse = sin(time * 2.0) * 0.05 + 0.95;
    finalColor.rgb *= pulse;
    
    fragColor = finalColor;
}

