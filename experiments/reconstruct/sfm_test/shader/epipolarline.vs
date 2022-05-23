#version 460 core

in vec3 in_Position;

out VS_OUT {
  vec3 P;
} vs_out;

void main() {
  vs_out.P = in_Position;
}
