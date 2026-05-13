#version 330 core

void main() {
    // Intentionally empty: only gl_FragDepth (set automatically from gl_Position.z/w) is
    // needed to populate the shadow map — RTR4 §7.4 p. 234, "only z-buffering is required.
    // Lighting, texturing, and writing values into the color buffer can be turned off."
}
